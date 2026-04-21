#include "../include/parser.h"
#include "../include/codegen.h"
#include "../include/lexer.h"
#include "../include/symbols.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Bytecode operand limits ---
// These enforce the encoding limits of the current bytecode at the EMIT site,
// so overflows fail with a source-level error instead of silently wrapping
// the byte operand and corrupting runtime state.
#define MAX_CALL_ARGS 255    // OP_CALL argc is 1 byte
#define MAX_RET_VALUES 8     // FuncInfo.ret_types[8] / Field.ret_types[8]
#define MAX_STRUCT_FIELDS 64 // StructInfo.fields[64]
#define MAX_ENUM_VALUES 64   // EnumInfo.values[64]

static void check_args(int n, const char *func_name) {
  if (n > MAX_CALL_ARGS)
    fail("Too many arguments in call to '%s' (got %d, max %d)",
         func_name ? func_name : "<anonymous>", n, MAX_CALL_ARGS);
}

static void check_ret_count(int n) {
  if (n > MAX_RET_VALUES)
    fail("Too many return values (got %d, max %d)", n, MAX_RET_VALUES);
}

// --- Phase B: type compatibility ---
// Returns a readable name for a DataType/struct-id/array-depth triple.
static const char *type_name(DataType t, int sid, int ad) {
  (void)sid;
  (void)ad;
  if (ad > 0)
    return "array";
  switch (t) {
  case TYPE_VOID:
    return "void";
  case TYPE_INT:
    return "int";
  case TYPE_FLOAT:
    return "float";
  case TYPE_CHAR:
    return "char";
  case TYPE_STR:
    return "str";
  case TYPE_STRUCT:
    return "struct";
  case TYPE_ARRAY:
    return "array";
  }
  return "unknown";
}

// Check that `src` is assignable to `dst` under Option B semantics.
// Emits a conversion opcode if widening is needed (e.g. int -> float).
// Fails with a clear message on incompatible or narrowing assignments.
static void check_assign_compat(DataType dst, int dst_sid, int dst_ad,
                                DataType src, int src_sid, int src_ad,
                                const char *context) {
  // Null (encoded as int 0) is assignable to any pointer-like type.
  if (src == TYPE_INT && (dst == TYPE_STR || dst == TYPE_STRUCT ||
                          dst == TYPE_ARRAY || dst_ad > 0))
    return; // allow null to pointer

  // Exact match
  if (dst == src && dst_sid == src_sid && dst_ad == src_ad)
    return;

  // Arrays: any-element assignable only if ad matches; element type loose for
  // now
  if (dst_ad > 0 || src_ad > 0) {
    if (dst_ad != src_ad)
      fail("%s: cannot assign %s to %s (array depth mismatch)", context,
           type_name(src, src_sid, src_ad), type_name(dst, dst_sid, dst_ad));
    return;
  }

  // Struct: sid must match
  if (dst == TYPE_STRUCT || src == TYPE_STRUCT) {
    if (dst != src || dst_sid != src_sid)
      fail("%s: cannot assign %s to %s", context,
           type_name(src, src_sid, src_ad), type_name(dst, dst_sid, dst_ad));
    return;
  }

  // Widening: int <- char  (Option B, lossless direction)
  if (dst == TYPE_INT && src == TYPE_CHAR)
    return;

  // Widening: float <- int | char (conversion opcode handles it at caller site)
  if (dst == TYPE_FLOAT && (src == TYPE_INT || src == TYPE_CHAR)) {
    emit(OP_I2F);
    return;
  }

  // Narrowing: char <- int  (Option B, forbidden)
  if (dst == TYPE_CHAR && src == TYPE_INT)
    fail("%s: cannot assign int to char implicitly (narrowing, may lose data). "
         "If intentional, use explicit modulo: `c = val %% 256;`",
         context);

  // Narrowing: int <- float
  if (dst == TYPE_INT && src == TYPE_FLOAT)
    fail("%s: cannot assign float to int implicitly (narrowing, truncates). "
         "If intentional, multiply and cast through an integer helper.",
         context);

  // str mismatch
  if (dst == TYPE_STR && src != TYPE_STR)
    fail("%s: cannot assign %s to str (use `str(x)` or `\"\" + x` to convert)",
         context, type_name(src, src_sid, src_ad));

  // Unhandled combination
  fail("%s: cannot assign %s to %s", context, type_name(src, src_sid, src_ad),
       type_name(dst, dst_sid, dst_ad));
}

// --- FORWARD DECLARATIONS ---
DataType expression(int *struct_id, int *array_depth);
// --- NEW PRECEDENCE ENGINE ---
DataType factor(int *struct_id, int *array_depth);

void statement();
void parse_struct();
void parse_func();
void parse_enum();
void parse_interface();
void parse_func_tail(DataType *ret_types, int *ret_sids, int *ret_ads,
                     int ret_count, char *name);
void parse_type(DataType *type, int *struct_id, int *array_depth);

// --- Pass 1: signature scanner ---
static void scan_program(void);
static void scan_func_declaration(void);
static void scan_typed_declaration(void);
static void skip_brace_block(void);

// --- LOOP CONTEXT ---
typedef struct Loop {
  size_t continue_addr;
  size_t *break_patches;
  int break_count;
  int break_cap;
  struct Loop *prev;
} Loop;

Loop *current_loop = NULL;

void enter_loop(size_t continue_addr) {
  Loop *l = malloc(sizeof(Loop));
  l->continue_addr = continue_addr;
  l->break_patches = NULL;
  l->break_count = 0;
  l->break_cap = 0;
  l->prev = current_loop;
  current_loop = l;
}

void add_break() {
  if (!current_loop)
    fail("break outside of loop");
  emit(OP_JMP);
  if (current_loop->break_count >= current_loop->break_cap) {
    current_loop->break_cap =
        current_loop->break_cap ? current_loop->break_cap * 2 : 8;
    current_loop->break_patches = realloc(
        current_loop->break_patches, current_loop->break_cap * sizeof(size_t));
  }
  current_loop->break_patches[current_loop->break_count++] = code_sz;
  emit32(0);
}

void add_continue() {
  if (!current_loop)
    fail("continue outside of loop");
  emit(OP_JMP);
  emit32(current_loop->continue_addr);
}

void leave_loop(size_t end_addr) {
  for (int i = 0; i < current_loop->break_count; i++) {
    emit_patch(current_loop->break_patches[i], end_addr);
  }
  free(current_loop->break_patches);
  Loop *prev = current_loop->prev;
  free(current_loop);
  current_loop = prev;
}

void parse_type(DataType *type, int *struct_id, int *array_depth) {
  *struct_id = -1;
  *array_depth = 0;
  if (accept(TK_INT))
    *type = TYPE_INT;
  else if (accept(TK_FLOAT))
    *type = TYPE_FLOAT;
  else if (accept(TK_CHAR))
    *type = TYPE_CHAR;
  else if (accept(TK_STR_TYPE))
    *type = TYPE_STR;
  else if (accept(TK_VOID))
    *type = TYPE_VOID;
  else if (cur.kind == TK_ID) {
    int sid = find_struct(cur.text);
    int eid = find_enum(cur.text);
    if (sid != -1) {
      *type = TYPE_STRUCT;
      *struct_id = sid;
      next();
    } else if (eid != -1) {
      *type = TYPE_INT;
      next();
    } else
      fail("Unknown type '%s'", cur.text);
  } else
    fail("Expected type");
  while (accept(TK_LBRACKET)) {
    expect(TK_RBRACKET);
    *array_depth += 1;
    *type = TYPE_ARRAY;
  }
}

DataType factor(int *struct_id, int *array_depth) {
  *struct_id = -1;
  *array_depth = 0;
  if (accept(TK_MINUS)) {
    int d1, d2;
    DataType t = factor(&d1, &d2);
    if (t == TYPE_INT)
      emit(OP_NEG);
    else if (t == TYPE_FLOAT)
      emit(OP_NEG_F);
    else
      fail("Cannot negate type %d", t);
    return t;
  }
  if (cur.kind == TK_NUM_INT) {
    emit(OP_CONST_INT);
    emit32((uint32_t)cur.ival);
    next();
    return TYPE_INT;
  }
  if (accept(TK_NULL)) {
    // null is a typed pointer zero. At the bytecode level it's a 0 int.
    // The type system treats it as pointer-compatible with str/struct/array.
    emit(OP_CONST_INT);
    emit32(0);
    return TYPE_STR; // any pointer type accepts null; TYPE_STR as neutral
    // pointer
  }
  if (cur.kind == TK_NUM_FLOAT) {
    emit(OP_CONST_FLOAT);
    uint64_t v;
    memcpy(&v, &cur.fval, 8);
    emit64(v);
    next();
    return TYPE_FLOAT;
  }
  if (cur.kind == TK_STR) {
    int idx = add_str(cur.text);
    emit(OP_CONST_STR);
    emit32(idx);
    next();
    return TYPE_STR;
  }

  if (accept(TK_STACK)) {
    expect(TK_LPAREN);
    DataType t;
    int sid;
    int ad;
    parse_type(&t, &sid, &ad);

    int comment_idx = 0xFFFFFFFF; // Default: No comment
    if (accept(TK_COMMA)) {
      if (cur.kind != TK_STR)
        fail("Expected string comment for memory allocation");
      comment_idx = add_str(cur.text);
      next();
    }

    expect(TK_RPAREN);
    if (t != TYPE_STRUCT)
      fail("stack() expects struct");
    emit(OP_ALLOC_STACK);
    emit32(sid);
    emit32(comment_idx); // Emit comment index
    *struct_id = sid;
    return TYPE_STRUCT;
  }

  if (accept(TK_NEW)) {
    expect(TK_LPAREN);
    DataType t;
    int sid;
    int ad;
    parse_type(&t, &sid, &ad);

    int comment_idx = 0xFFFFFFFF;

    if (accept(TK_COMMA)) {
      if (cur.kind == TK_STR) {
        // new(Type, "comment")
        comment_idx = add_str(cur.text);
        next();
        expect(TK_RPAREN);
        if (t != TYPE_STRUCT)
          fail("new() expects struct");
        emit(OP_ALLOC_STRUCT);
        emit32(sid);
        emit32(comment_idx);
        *struct_id = sid;
        return TYPE_STRUCT;
      } else {
        // new(Type, size) OR new(Type, size, "comment")
        int d1, d2;
        DataType size_t = expression(&d1, &d2);
        if (size_t != TYPE_INT)
          fail("Array size must be int");

        if (accept(TK_COMMA)) {
          if (cur.kind != TK_STR)
            fail("Expected string comment for array allocation");
          comment_idx = add_str(cur.text);
          next();
        }

        expect(TK_RPAREN);
        emit(OP_ALLOC_ARRAY);
        emit32(8);
        emit32(comment_idx);
        *struct_id = sid;
        *array_depth = ad + 1;
        return TYPE_ARRAY;
      }
    } else {
      expect(TK_RPAREN);
      if (t != TYPE_STRUCT)
        fail("new() expects struct");
      emit(OP_ALLOC_STRUCT);
      emit32(sid);
      emit32(comment_idx);
      *struct_id = sid;
      return TYPE_STRUCT;
    }
  }

  if (cur.kind == TK_ID) {
    char *name = strdup(cur.text);
    next();

    int eid = find_enum(name);
    if (eid != -1) {
      expect(TK_DOT);
      if (cur.kind != TK_ID)
        fail("Expected enum value");
      int val = get_enum_value(eid, cur.text);
      if (val == -1)
        fail("Unknown enum value '%s' in enum '%s'", cur.text, name);
      emit(OP_CONST_INT);
      emit32(val);
      next();
      free(name);
      return TYPE_INT;
    }

    // --- NAMESPACE RESOLUTION LOGIC ---
    int lid = find_local(name);
    int gid = find_global(name);

    if (lid == -1 && gid == -1) {
      while (accept(TK_DOT)) {
        if (cur.kind != TK_ID)
          fail("Expected identifier after dot");
        size_t len = strlen(name) + 1 + strlen(cur.text) + 1;
        name = realloc(name, len);
        strcat(name, ".");
        strcat(name, cur.text);
        next();
      }
    }

    if (accept(TK_LPAREN)) {
      int nid = -1;
      DataType ret = TYPE_VOID;
      if (!strcmp(name, "clock")) {
        nid = 0;
        ret = TYPE_FLOAT;
      } else if (!strcmp(name, "input_int")) {
        nid = 1;
        ret = TYPE_INT;
      }
      // Bridge Calls
      else if (!strcmp(name, "__bridge_print_str")) {
        nid = 10;
        ret = TYPE_VOID;
      } else if (!strcmp(name, "__bridge_print_int")) {
        nid = 11;
        ret = TYPE_VOID;
      } else if (!strcmp(name, "__bridge_print_float")) {
        nid = 12;
        ret = TYPE_VOID;
      } else if (!strcmp(name, "__bridge_pow")) {
        nid = 20;
        ret = TYPE_INT;
      } else if (!strcmp(name, "__bridge_abs")) {
        nid = 21;
        ret = TYPE_INT;
      }

      if (nid != -1) {
        if (cur.kind != TK_RPAREN) {
          do {
            int d1, d2;
            expression(&d1, &d2);
          } while (accept(TK_COMMA));
        }
        expect(TK_RPAREN);
        emit(OP_NATIVE);
        emit32(nid);
        free(name);
        return ret;
      }

      int fid = find_func(name);
      if (fid == -1)
        fail("Undefined function '%s'", name);

      int args = 0;
      DataType arg_t[32];
      int arg_s[32];
      int arg_a[32];
      if (cur.kind != TK_RPAREN) {
        do {
          if (args < 32) {
            arg_t[args] = expression(&arg_s[args], &arg_a[args]);
          } else {
            int d1, d2;
            expression(&d1, &d2);
          }
          args++;
        } while (accept(TK_COMMA));
      }
      expect(TK_RPAREN);
      if (args != funcs[fid].arg_count)
        fail("Arg count mismatch calling '%s' (expected %d, got %d)", name,
             funcs[fid].arg_count, args);
      // Option B arg-type checking: each arg must match declared signature.
      for (int i = 0; i < args && i < 32 && i < funcs[fid].arg_count; i++) {
        char ctx[128];
        snprintf(ctx, sizeof(ctx), "call to '%s' arg %d", name, i + 1);
        check_assign_compat(
            funcs[fid].arg_types[i], funcs[fid].arg_struct_ids[i],
            funcs[fid].arg_array_depths[i], arg_t[i], arg_s[i], arg_a[i], ctx);
      }
      check_args(args, name);
      emit_call_to(fid, args);
      free(name);
      *struct_id = funcs[fid].ret_struct_ids[0];
      *array_depth = funcs[fid].ret_array_depths[0];
      return funcs[fid].ret_types[0];
    }

    if (lid == -1 && gid == -1) {
      int fid = find_func(name);
      if (fid != -1) {
        emit(OP_CONST_INT);
        emit32(funcs[fid].addr);
        free(name);
        *struct_id = -1;
        *array_depth = 0;
        return TYPE_INT;
      }
      fail("Undefined variable '%s'", name);
    }

    DataType t = TYPE_VOID;
    int sid = -1;
    int ad = 0;
    if (lid != -1) {
      emit(OP_GET_LOCAL);
      emit(locals[lid].offset);
      t = locals[lid].type;
      sid = locals[lid].struct_id;
      ad = locals[lid].array_depth;
    } else {
      emit(OP_GET_GLOBAL);
      emit(gid);
      t = globals[gid].type;
      sid = globals[gid].struct_id;
      ad = globals[gid].array_depth;
    }

    while (1) {
      if (accept(TK_DOT)) {
        if (t != TYPE_STRUCT)
          fail("Dot on non-struct");
        if (cur.kind != TK_ID)
          fail("Expected field name");
        int parent_sid = sid;
        int field_idx = -1;
        for (int i = 0; i < structs[parent_sid].field_count; i++) {
          if (!strcmp(structs[parent_sid].fields[i].name, cur.text)) {
            field_idx = i;
            t = structs[parent_sid].fields[i].type;
            sid = structs[parent_sid].fields[i].struct_id;
            ad = structs[parent_sid].fields[i].array_depth;
            break;
          }
        }
        if (field_idx == -1)
          fail("Unknown field '%s'", cur.text);
        next();

        // --- INTERFACE METHOD CALL ---
        if (structs[parent_sid].is_interface && cur.kind == TK_LPAREN) {
          accept(TK_LPAREN);
          emit(OP_GET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);

          int args = 0;
          if (cur.kind != TK_RPAREN) {
            do {
              int d1, d2;
              expression(&d1, &d2);
              args++;
            } while (accept(TK_COMMA));
          }
          expect(TK_RPAREN);

          if (args != structs[parent_sid].fields[field_idx].arg_count)
            fail("Interface method arg count mismatch");

          check_args(args, structs[parent_sid].fields[field_idx].name);
          emit(OP_CALL_DYN_BOT);
          emit(args);

          *struct_id = structs[parent_sid].fields[field_idx].ret_sids[0];
          *array_depth = structs[parent_sid].fields[field_idx].ret_ads[0];
          free(name);
          return structs[parent_sid].fields[field_idx].ret_types[0];
        }
        // -----------------------------

        if (cur.kind == TK_ASSIGN) {
          next();
          int d1, d2;
          expression(&d1, &d2);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          expect(TK_SEMI);
          free(name);
          return t;
        } else if (cur.kind == TK_PLUS_ASSIGN) {
          next();
          emit(OP_DUP);
          emit(OP_GET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          int d1, d2;
          DataType rhs_t = expression(&d1, &d2);
          if (t == TYPE_FLOAT || rhs_t == TYPE_FLOAT)
            emit(OP_ADD_F);
          else
            emit(OP_ADD);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          expect(TK_SEMI);
          free(name);
          return t;
        } else if (cur.kind == TK_MINUS_ASSIGN) {
          next();
          emit(OP_DUP);
          emit(OP_GET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          int d1, d2;
          DataType rhs_t = expression(&d1, &d2);
          if (t == TYPE_FLOAT || rhs_t == TYPE_FLOAT)
            emit(OP_SUB_F);
          else
            emit(OP_SUB);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          expect(TK_SEMI);
          free(name);
          return t;
        } else if (cur.kind == TK_INC) {
          next();
          emit(OP_DUP);
          emit(OP_GET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          emit(OP_CONST_INT);
          emit32(1);
          emit(OP_ADD);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          expect(TK_SEMI);
          free(name);
          return t;
        } else if (cur.kind == TK_DEC) {
          next();
          emit(OP_DUP);
          emit(OP_GET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          emit(OP_CONST_INT);
          emit32(1);
          emit(OP_SUB);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          expect(TK_SEMI);
          free(name);
          return t;
        }
        emit(OP_GET_FIELD);
        emit(structs[parent_sid].fields[field_idx].offset);
      } else if (accept(TK_LBRACKET)) {
        int d1, d2;
        expression(&d1, &d2);
        expect(TK_RBRACKET);
        if (cur.kind == TK_ASSIGN) {
          next();
          int d1, d2;
          expression(&d1, &d2);
          emit(OP_SET_INDEX);
          expect(TK_SEMI);
          free(name);
          return t;
        } else if (cur.kind == TK_INC) {
          next();
          emit(OP_INC_INDEX);
          expect(TK_SEMI);
          free(name);
          return t;
        } else if (cur.kind == TK_DEC) {
          next();
          emit(OP_DEC_INDEX);
          expect(TK_SEMI);
          free(name);
          return t;
        }
        emit(OP_GET_INDEX);
        t = TYPE_INT;
      } else
        break;
    }

    if (accept(TK_ASSIGN)) {
      emit(OP_POP);
      int d1, d2;
      expression(&d1, &d2);
      expect(TK_SEMI);
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      free(name);
      return t;
    } else if (accept(TK_INC)) {
      // --- FIX: Push the variable to the stack first ---
      if (lid != -1) {
        emit(OP_GET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_GET_GLOBAL);
        emit(gid);
      }
      // -------------------------------------------------
      emit(OP_CONST_INT);
      emit32(1);
      emit(OP_ADD);
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      expect(TK_SEMI);
      free(name);
      return t;
    } else if (accept(TK_DEC)) {
      // --- FIX: Push the variable to the stack first ---
      if (lid != -1) {
        emit(OP_GET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_GET_GLOBAL);
        emit(gid);
      }
      // -------------------------------------------------
      emit(OP_CONST_INT);
      emit32(1);
      emit(OP_SUB);
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      expect(TK_SEMI);
      free(name);
      return t;
    } else if (accept(TK_PLUS_ASSIGN)) {
      // --- FIX: Push the variable to the stack first ---
      if (lid != -1) {
        emit(OP_GET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_GET_GLOBAL);
        emit(gid);
      }
      // -------------------------------------------------
      int d1, d2;
      expression(&d1, &d2);
      emit(OP_ADD);
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      expect(TK_SEMI);
      free(name);
      return t;
    } else if (accept(TK_MINUS_ASSIGN)) {
      // --- FIX: Push the variable to the stack first ---
      if (lid != -1) {
        emit(OP_GET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_GET_GLOBAL);
        emit(gid);
      }
      // -------------------------------------------------
      int d1, d2;
      expression(&d1, &d2);
      emit(OP_SUB);
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      expect(TK_SEMI);
      free(name);
      return t;
    }

    *struct_id = sid;
    *array_depth = ad;
    free(name);
    return t;
  }
  if (accept(TK_LPAREN)) {
    DataType t = expression(struct_id, array_depth);
    expect(TK_RPAREN);
    return t;
  }
  fail("Unexpected token '%s'", cur.text ? cur.text : "EOF");
  return TYPE_VOID;
}

DataType unary(int *struct_id, int *array_depth) {
  if (accept(TK_MINUS)) {
    DataType t = unary(struct_id, array_depth);
    if (t == TYPE_INT)
      emit(OP_NEG);
    else if (t == TYPE_FLOAT)
      emit(OP_NEG_F);
    else
      fail("Cannot negate type %d", t);
    return t;
  }
  if (accept(TK_NOT)) {
    unary(struct_id, array_depth);
    emit(OP_NOT);
    return TYPE_INT;
  }
  if (accept(TK_BIT_NOT)) {
    unary(struct_id, array_depth);
    emit(OP_BIT_NOT);
    return TYPE_INT;
  }
  return factor(struct_id, array_depth);
}

DataType term(int *struct_id, int *array_depth) {
  DataType t = unary(struct_id, array_depth);
  while (cur.kind == TK_MUL || cur.kind == TK_DIV || cur.kind == TK_MOD) {
    int op = cur.kind;
    next();
    int d1, d2;
    DataType t2 = unary(&d1, &d2);
    if (op == TK_MOD) {
      // Modulo is integer-only. Auto-promotion makes no sense here.
      if (t == TYPE_FLOAT || t2 == TYPE_FLOAT)
        fail(
            "Modulo (%%) is integer-only; use std.math for floating-point mod");
      emit(OP_MOD);
    } else if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) {
      // Promote the integer side(s) to float BEFORE the float op.
      // Stack layout: [LHS][RHS]. RHS is on top.
      if (t == TYPE_INT && t2 == TYPE_FLOAT) {
        // Convert LHS (under RHS): swap, convert, swap back.
        emit(OP_SWAP);
        emit(OP_I2F);
        emit(OP_SWAP);
      } else if (t == TYPE_FLOAT && t2 == TYPE_INT) {
        // RHS on top; convert in place.
        emit(OP_I2F);
      }
      emit(op == TK_MUL ? OP_MUL_F : OP_DIV_F);
      t = TYPE_FLOAT;
    } else
      emit(op == TK_MUL ? OP_MUL : OP_DIV);
  }
  return t;
}

DataType add_expr(int *struct_id, int *array_depth) {
  DataType t = term(struct_id, array_depth);
  while (cur.kind == TK_PLUS || cur.kind == TK_MINUS) {
    int op = cur.kind;
    next();
    int d1, d2;
    DataType t2 = term(&d1, &d2);

    // --- Dynamic string concatenation with auto-conversion of numeric side ---
    if (op == TK_PLUS && (t == TYPE_STR || t2 == TYPE_STR)) {
      if (t == TYPE_STR && t2 == TYPE_STR) {
        emit(OP_STR_CAT);
      } else if (t == TYPE_STR && t2 == TYPE_INT) {
        emit(OP_INT_TO_STR);
        emit(OP_STR_CAT);
      } else if (t == TYPE_STR && t2 == TYPE_FLOAT) {
        emit(OP_FLOAT_TO_STR);
        emit(OP_STR_CAT);
      } else if (t == TYPE_INT && t2 == TYPE_STR) {
        // LHS int is under the str. Swap, convert LHS to str, swap back, then
        // cat.
        emit(OP_SWAP);
        emit(OP_INT_TO_STR);
        emit(OP_SWAP);
        emit(OP_STR_CAT);
      } else if (t == TYPE_FLOAT && t2 == TYPE_STR) {
        emit(OP_SWAP);
        emit(OP_FLOAT_TO_STR);
        emit(OP_SWAP);
        emit(OP_STR_CAT);
      } else {
        fail("Cannot concatenate str with this type");
      }
      t = TYPE_STR;
    } else if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) {
      // Int-to-float promotion for mixed arithmetic.
      if (t == TYPE_INT && t2 == TYPE_FLOAT) {
        emit(OP_SWAP);
        emit(OP_I2F);
        emit(OP_SWAP);
      } else if (t == TYPE_FLOAT && t2 == TYPE_INT) {
        emit(OP_I2F);
      }
      emit(op == TK_PLUS ? OP_ADD_F : OP_SUB_F);
      t = TYPE_FLOAT;
    } else
      emit(op == TK_PLUS ? OP_ADD : OP_SUB);
  }
  return t;
}

DataType shift_expr(int *struct_id, int *array_depth) {
  DataType t = add_expr(struct_id, array_depth);
  while (cur.kind == TK_SHL || cur.kind == TK_SHR) {
    int op = cur.kind;
    next();
    int d1, d2;
    add_expr(&d1, &d2);
    emit(op == TK_SHL ? OP_SHL : OP_SHR);
  }
  return t;
}

DataType rel_expr(int *struct_id, int *array_depth) {
  DataType t = shift_expr(struct_id, array_depth);
  if (cur.kind == TK_LT || cur.kind == TK_LE || cur.kind == TK_GT ||
      cur.kind == TK_GE) {
    int op = cur.kind;
    next();
    int d1, d2;
    DataType t2 = shift_expr(&d1, &d2);
    if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) {
      // Promote int side to float before float comparison.
      if (t == TYPE_INT && t2 == TYPE_FLOAT) {
        emit(OP_SWAP);
        emit(OP_I2F);
        emit(OP_SWAP);
      } else if (t == TYPE_FLOAT && t2 == TYPE_INT) {
        emit(OP_I2F);
      }
      if (op == TK_LT)
        emit(OP_LT_F);
      else if (op == TK_LE)
        emit(OP_LE_F);
      else if (op == TK_GT)
        emit(OP_GT_F);
      else if (op == TK_GE)
        emit(OP_GE_F);
    } else {
      if (op == TK_LT)
        emit(OP_LT);
      else if (op == TK_LE)
        emit(OP_LE);
      else if (op == TK_GT)
        emit(OP_GT);
      else if (op == TK_GE)
        emit(OP_GE);
    }
    return TYPE_INT;
  }
  return t;
}

DataType equality_expr(int *struct_id, int *array_depth) {
  DataType t = rel_expr(struct_id, array_depth);
  while (cur.kind == TK_EQ || cur.kind == TK_NE) {
    int op = cur.kind;
    next();
    int d1, d2;
    DataType t2 = rel_expr(&d1, &d2);
    if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) {
      if (t == TYPE_INT && t2 == TYPE_FLOAT) {
        emit(OP_SWAP);
        emit(OP_I2F);
        emit(OP_SWAP);
      } else if (t == TYPE_FLOAT && t2 == TYPE_INT) {
        emit(OP_I2F);
      }
      emit(op == TK_EQ ? OP_EQ_F : OP_NE_F);
    } else {
      emit(op == TK_EQ ? OP_EQ : OP_NE);
    }
    t = TYPE_INT;
  }
  return t;
}

DataType bitwise_and_expr(int *struct_id, int *array_depth) {
  DataType t = equality_expr(struct_id, array_depth);
  while (accept(TK_BIT_AND)) {
    int d1, d2;
    equality_expr(&d1, &d2);
    emit(OP_BIT_AND);
  }
  return t;
}

DataType bitwise_xor_expr(int *struct_id, int *array_depth) {
  DataType t = bitwise_and_expr(struct_id, array_depth);
  while (accept(TK_BIT_XOR)) {
    int d1, d2;
    bitwise_and_expr(&d1, &d2);
    emit(OP_BIT_XOR);
  }
  return t;
}

DataType bitwise_or_expr(int *struct_id, int *array_depth) {
  DataType t = bitwise_xor_expr(struct_id, array_depth);
  while (accept(TK_BIT_OR)) {
    int d1, d2;
    bitwise_xor_expr(&d1, &d2);
    emit(OP_BIT_OR);
  }
  return t;
}

// Short-circuit logical AND.
// Semantics: if LHS is false (zero), whole expression is 0; don't evaluate RHS.
// Emits: <LHS> ; DUP ; JZ end ; POP ; <RHS> ; NORMALIZE ; end:
// Result is always 0 or 1 (normalized), pushed once.
DataType logical_and_expr(int *struct_id, int *array_depth) {
  DataType t = bitwise_or_expr(struct_id, array_depth);
  while (accept(TK_AND)) {
    // LHS already on stack. Normalize it to 0 or 1 first.
    emit(OP_CONST_INT);
    emit32(0);
    emit(OP_NE);  // stack: (LHS != 0) ? 1 : 0
    emit(OP_DUP); // keep a copy for the jump test
    emit(OP_JZ);
    size_t patch_short = code_sz;
    emit32(0);
    emit(OP_POP); // discard the duplicate; evaluate RHS
    int d1, d2;
    bitwise_or_expr(&d1, &d2);
    // Normalize RHS to 0 or 1 as well
    emit(OP_CONST_INT);
    emit32(0);
    emit(OP_NE);
    emit_patch(patch_short, code_sz);
    t = TYPE_INT;
  }
  return t;
}

// Short-circuit logical OR.
// Semantics: if LHS is true (non-zero), whole expression is 1; don't evaluate
// RHS.
DataType logical_or_expr(int *struct_id, int *array_depth) {
  DataType t = logical_and_expr(struct_id, array_depth);
  while (accept(TK_OR)) {
    // Normalize LHS to 0/1
    emit(OP_CONST_INT);
    emit32(0);
    emit(OP_NE);
    emit(OP_DUP);
    // Invert: JZ jumps if top is zero. We want to jump past RHS if LHS is TRUE.
    // So negate, JZ-on-false, i.e. use OP_NOT then OP_JZ.
    emit(OP_NOT);
    emit(OP_JZ);
    size_t patch_short = code_sz;
    emit32(0);
    emit(OP_POP); // LHS was false, discard; evaluate RHS
    int d1, d2;
    logical_and_expr(&d1, &d2);
    emit(OP_CONST_INT);
    emit32(0);
    emit(OP_NE);
    emit_patch(patch_short, code_sz);
    t = TYPE_INT;
  }
  return t;
}

DataType expression(int *struct_id, int *array_depth) {
  DataType t = logical_or_expr(struct_id, array_depth);
  if (accept(TK_QUESTION)) {
    emit(OP_JZ);
    size_t patch_else = code_sz;
    emit32(0);
    int d1, d2;
    DataType t_true = expression(&d1, &d2);
    emit(OP_JMP);
    size_t patch_end = code_sz;
    emit32(0);
    emit_patch(patch_else, code_sz);
    expect(TK_COLON);
    DataType t_false = expression(&d1, &d2);
    emit_patch(patch_end, code_sz);
    if (t_true != t_false)
      fail("Ternary operator type mismatch");
    return t_true;
  }
  return t;
}

void statement() {
  if (accept(TK_LBRACE)) {
    int saved_locals = local_count;
    while (cur.kind != TK_RBRACE)
      statement();
    expect(TK_RBRACE);
    int diff = local_count - saved_locals;
    for (int i = 0; i < diff; i++)
      emit(OP_POP);
    local_count = saved_locals;
    return;
  }
  if (accept(TK_BREAK)) {
    add_break();
    expect(TK_SEMI);
    return;
  }
  if (accept(TK_CONTINUE)) {
    add_continue();
    expect(TK_SEMI);
    return;
  }
  if (accept(TK_TRY)) {
    expect(TK_LBRACE);
    emit(OP_TRY);
    size_t patch_catch = code_sz;
    emit32(0);
    int saved_locals = local_count;
    while (cur.kind != TK_RBRACE)
      statement();
    expect(TK_RBRACE);
    int diff = local_count - saved_locals;
    for (int i = 0; i < diff; i++)
      emit(OP_POP);
    local_count = saved_locals;
    emit(OP_END_TRY);
    emit(OP_JMP);
    size_t patch_end = code_sz;
    emit32(0);
    emit_patch(patch_catch, code_sz);
    expect(TK_CATCH);
    expect(TK_LPAREN);
    if (cur.kind != TK_ID)
      fail("Expected error variable");
    add_local(strdup(cur.text), TYPE_STR, -1, 0);
    next();
    expect(TK_RPAREN);
    expect(TK_LBRACE);
    while (cur.kind != TK_RBRACE)
      statement();
    expect(TK_RBRACE);
    emit(OP_POP);
    local_count--;
    emit_patch(patch_end, code_sz);
    return;
  }
  if (accept(TK_THROW)) {
    int d1, d2;
    DataType t = expression(&d1, &d2);
    if (t != TYPE_STR)
      fail("Throw expects string");
    expect(TK_SEMI);
    emit(OP_THROW);
    return;
  }
  if (accept(TK_EYE)) {
    expect(TK_LPAREN);
    expect(TK_RPAREN);
    expect(TK_SEMI);
    emit(OP_ABYSS_EYE);
    return;
  }
  if (accept(TK_PRINT)) {
    expect(TK_LPAREN);
    int d1, d2;
    DataType t = expression(&d1, &d2);
    if (cur.kind == TK_COMMA) {
      if (t != TYPE_STR)
        fail("First arg of formatted print must be string");
      int arg_count = 0;
      while (accept(TK_COMMA)) {
        expression(&d1, &d2);
        arg_count++;
      }
      expect(TK_RPAREN);
      expect(TK_SEMI);
      emit(OP_PRINT_FMT);
      emit(arg_count);
      return;
    }
    expect(TK_RPAREN);
    expect(TK_SEMI);
    if (t == TYPE_INT)
      emit(OP_PRINT);
    else if (t == TYPE_FLOAT)
      emit(OP_PRINT_F);
    else if (t == TYPE_STR)
      emit(OP_PRINT_STR);
    else if (t == TYPE_CHAR)
      emit(OP_PRINT_CHAR);
    else
      fail("Cannot print type %d", t);
    return;
  }
  if (accept(TK_PRINT_CHAR)) {
    expect(TK_LPAREN);
    int d1, d2;
    expression(&d1, &d2);
    expect(TK_RPAREN);
    expect(TK_SEMI);
    emit(OP_PRINT_CHAR);
    return;
  }
  if (accept(TK_FREE)) {
    expect(TK_LPAREN);
    int d1, d2;
    expression(&d1, &d2);
    expect(TK_RPAREN);
    expect(TK_SEMI);
    emit(OP_FREE);
    return;
  }
  if (accept(TK_RETURN)) {
    if (cur.kind == TK_SEMI) {
      expect(TK_SEMI);
      emit(OP_CONST_INT);
      emit32(0);
      emit(OP_RET);
      emit(0);
      return;
    }
    int count = 0;
    do {
      int d1, d2;
      expression(&d1, &d2);
      count++;
    } while (accept(TK_COMMA));
    expect(TK_SEMI);
    check_ret_count(count);
    emit(OP_RET);
    emit(count);
    return;
  }
  if (accept(TK_WHILE)) {
    expect(TK_LPAREN);
    size_t start = code_sz;
    int d1, d2;
    expression(&d1, &d2);
    expect(TK_RPAREN);
    emit(OP_JZ);
    size_t patch = code_sz;
    emit32(0);
    enter_loop(start);
    statement();
    emit(OP_JMP);
    emit32(start);
    emit_patch(patch, code_sz);
    leave_loop(code_sz);
    return;
  }
  if (accept(TK_FOR)) {
    expect(TK_LPAREN);
    int saved_locals = local_count;
    statement();
    size_t start_cond = code_sz;
    int d1, d2;
    expression(&d1, &d2);
    expect(TK_SEMI);
    emit(OP_JZ);
    size_t patch_end = code_sz;
    emit32(0);
    emit(OP_JMP);
    size_t patch_body = code_sz;
    emit32(0);
    size_t start_step = code_sz;
    if (cur.kind == TK_ID) {
      char *name = strdup(cur.text);
      next();
      int lid = find_local(name);
      int gid = find_global(name);

// Emit a load of the variable for read-modify-write forms
#define LOAD_VAR()                                                             \
  do {                                                                         \
    if (lid != -1) {                                                           \
      emit(OP_GET_LOCAL);                                                      \
      emit(locals[lid].offset);                                                \
    } else {                                                                   \
      emit(OP_GET_GLOBAL);                                                     \
      emit(gid);                                                               \
    }                                                                          \
  } while (0)
#define STORE_VAR()                                                            \
  do {                                                                         \
    if (lid != -1) {                                                           \
      emit(OP_SET_LOCAL);                                                      \
      emit(locals[lid].offset);                                                \
    } else {                                                                   \
      emit(OP_SET_GLOBAL);                                                     \
      emit(gid);                                                               \
    }                                                                          \
  } while (0)

      if (accept(TK_INC)) {
        LOAD_VAR();
        emit(OP_CONST_INT);
        emit32(1);
        emit(OP_ADD);
        STORE_VAR();
      } else if (accept(TK_DEC)) {
        LOAD_VAR();
        emit(OP_CONST_INT);
        emit32(1);
        emit(OP_SUB);
        STORE_VAR();
      } else if (accept(TK_ASSIGN)) {
        int d1, d2;
        expression(&d1, &d2);
        STORE_VAR();
      } else if (cur.kind == TK_PLUS_ASSIGN || cur.kind == TK_MINUS_ASSIGN ||
                 cur.kind == TK_MUL_ASSIGN || cur.kind == TK_DIV_ASSIGN ||
                 cur.kind == TK_MOD_ASSIGN || cur.kind == TK_SHL_ASSIGN ||
                 cur.kind == TK_SHR_ASSIGN || cur.kind == TK_BIT_AND_ASSIGN ||
                 cur.kind == TK_BIT_OR_ASSIGN ||
                 cur.kind == TK_BIT_XOR_ASSIGN) {
        TkKind op = cur.kind;
        next();
        LOAD_VAR();
        int d1, d2;
        expression(&d1, &d2);
        switch (op) {
        case TK_PLUS_ASSIGN:
          emit(OP_ADD);
          break;
        case TK_MINUS_ASSIGN:
          emit(OP_SUB);
          break;
        case TK_MUL_ASSIGN:
          emit(OP_MUL);
          break;
        case TK_DIV_ASSIGN:
          emit(OP_DIV);
          break;
        case TK_MOD_ASSIGN:
          emit(OP_MOD);
          break;
        case TK_SHL_ASSIGN:
          emit(OP_SHL);
          break;
        case TK_SHR_ASSIGN:
          emit(OP_SHR);
          break;
        case TK_BIT_AND_ASSIGN:
          emit(OP_BIT_AND);
          break;
        case TK_BIT_OR_ASSIGN:
          emit(OP_BIT_OR);
          break;
        case TK_BIT_XOR_ASSIGN:
          emit(OP_BIT_XOR);
          break;
        default:
          fail("Internal: unreachable compound op");
        }
        STORE_VAR();
      }
#undef LOAD_VAR
#undef STORE_VAR
      free(name);
    }
    expect(TK_RPAREN);
    emit(OP_JMP);
    emit32(start_cond);
    emit_patch(patch_body, code_sz);
    enter_loop(start_step);
    statement();
    emit(OP_JMP);
    emit32(start_step);
    emit_patch(patch_end, code_sz);
    leave_loop(code_sz);
    int diff = local_count - saved_locals;
    for (int i = 0; i < diff; i++)
      emit(OP_POP);
    local_count = saved_locals;
    return;
  }
  if (accept(TK_IF)) {
    expect(TK_LPAREN);
    int d1, d2;
    expression(&d1, &d2);
    expect(TK_RPAREN);
    emit(OP_JZ);
    size_t patch_else = code_sz;
    emit32(0);
    statement();
    if (accept(TK_ELSE)) {
      emit(OP_JMP);
      size_t patch_end = code_sz;
      emit32(0);
      emit_patch(patch_else, code_sz);
      statement();
      emit_patch(patch_end, code_sz);
    } else {
      emit_patch(patch_else, code_sz);
    }
    return;
  }
  if (cur.kind == TK_INT || cur.kind == TK_FLOAT || cur.kind == TK_CHAR ||
      cur.kind == TK_STR_TYPE || cur.kind == TK_ID) {
    int sid = -1;
    int ad = 0;
    DataType type = TYPE_VOID;
    int is_decl = 0;

    if (cur.kind != TK_ID) {
      is_decl = 1;
    } else {
      if (find_struct(cur.text) != -1) {
        is_decl = 1;
      } else if (find_enum(cur.text) != -1) {
        TkKind pk = peek_kind();
        if (pk == TK_ID || pk == TK_LBRACKET) {
          is_decl = 1;
        }
      }
    }

    if (is_decl) {
      parse_type(&type, &sid, &ad);
      if (cur.kind == TK_ID) {
        char *name = strdup(cur.text);
        next();
        if (accept(TK_ASSIGN)) {
          int rhs_sid, rhs_ad;
          expression(&rhs_sid, &rhs_ad);
          // --- NEW: TAG THE ALLOCATION ---
          emit(OP_TAG_ALLOC);
          emit32(add_str(name));
        } else {
          emit(OP_CONST_INT);
          emit32(0);
        }
        expect(TK_SEMI);
        add_local(name, type, sid, ad);
        return;
      }
    }
  }

  // Tuple Unpacking
  if (cur.kind == TK_ID) {
    char *name = strdup(cur.text);
    next();
    if (cur.kind == TK_COMMA) {
      char *names[8];
      names[0] = name;
      int count = 1;
      while (accept(TK_COMMA)) {
        if (cur.kind != TK_ID)
          fail("Expected var name");
        names[count++] = strdup(cur.text);
        next();
      }
      expect(TK_ASSIGN);
      if (cur.kind != TK_ID)
        fail("Expected func call");
      char *func_name = strdup(cur.text);
      next();

      // --- Check if this is an interface method call: obj.method(...) ---
      if (cur.kind == TK_DOT) {
        // Resolve obj as a local/global; look up method via its interface type.
        int lid = find_local(func_name);
        int gid = find_global(func_name);
        if (lid == -1 && gid == -1)
          fail("Undefined variable '%s' in tuple-dispatch call", func_name);

        DataType ot = (lid != -1) ? locals[lid].type : globals[gid].type;
        int osid = (lid != -1) ? locals[lid].struct_id : globals[gid].struct_id;
        if (ot != TYPE_STRUCT || osid < 0 || !structs[osid].is_interface)
          fail("Tuple unpacking via '.' requires an interface variable");

        next(); // consume TK_DOT
        if (cur.kind != TK_ID)
          fail("Expected method name after '.'");
        char *method_name = strdup(cur.text);
        next();

        // Find field in interface
        int field_idx = -1;
        for (int i = 0; i < structs[osid].field_count; i++) {
          if (!strcmp(structs[osid].fields[i].name, method_name)) {
            field_idx = i;
            break;
          }
        }
        if (field_idx == -1)
          fail("Unknown interface method '%s'", method_name);

        // Push obj, then load method pointer from field.
        if (lid != -1) {
          emit(OP_GET_LOCAL);
          emit(locals[lid].offset);
        } else {
          emit(OP_GET_GLOBAL);
          emit(gid);
        }
        emit(OP_GET_FIELD);
        emit(structs[osid].fields[field_idx].offset);

        expect(TK_LPAREN);
        int args = 0;
        if (cur.kind != TK_RPAREN) {
          do {
            int d1, d2;
            expression(&d1, &d2);
            args++;
          } while (accept(TK_COMMA));
        }
        expect(TK_RPAREN);
        expect(TK_SEMI);

        if (args != structs[osid].fields[field_idx].arg_count)
          fail("Interface method arg count mismatch");
        if (count != structs[osid].fields[field_idx].ret_count)
          fail("Tuple unpack count does not match interface method return "
               "count");
        check_args(args, method_name);

        emit(OP_CALL_DYN_BOT);
        emit(args);

        for (int i = count - 1; i >= 0; i--) {
          int vlid = find_local(names[i]);
          int vgid = find_global(names[i]);
          if (vlid != -1) {
            emit(OP_SET_LOCAL);
            emit(locals[vlid].offset);
          } else if (vgid != -1) {
            emit(OP_SET_GLOBAL);
            emit(vgid);
          }
          free(names[i]);
        }
        free(method_name);
        free(func_name);
        return;
      }

      expect(TK_LPAREN);
      int fid = find_func(func_name);
      if (fid == -1)
        fail("Undefined func");
      int args = 0;
      DataType arg_t[32];
      int arg_s[32];
      int arg_a[32];
      if (cur.kind != TK_RPAREN) {
        do {
          if (args < 32)
            arg_t[args] = expression(&arg_s[args], &arg_a[args]);
          else {
            int d1, d2;
            expression(&d1, &d2);
          }
          args++;
        } while (accept(TK_COMMA));
      }
      expect(TK_RPAREN);
      expect(TK_SEMI);
      if (args != funcs[fid].arg_count)
        fail("Arg count mismatch calling '%s' (expected %d, got %d)", func_name,
             funcs[fid].arg_count, args);
      for (int i = 0; i < args && i < 32 && i < funcs[fid].arg_count; i++) {
        char ctx[128];
        snprintf(ctx, sizeof(ctx), "call to '%s' arg %d", func_name, i + 1);
        check_assign_compat(
            funcs[fid].arg_types[i], funcs[fid].arg_struct_ids[i],
            funcs[fid].arg_array_depths[i], arg_t[i], arg_s[i], arg_a[i], ctx);
      }
      check_args(args, func_name);
      emit_call_to(fid, args);
      for (int i = count - 1; i >= 0; i--) {
        int lid = find_local(names[i]);
        int gid = find_global(names[i]);
        if (lid != -1) {
          emit(OP_SET_LOCAL);
          emit(locals[lid].offset);
        } else if (gid != -1) {
          emit(OP_SET_GLOBAL);
          emit(gid);
        }
        free(names[i]);
      }
      free(func_name);
      return;
    }
    // Function call as statement
    if (accept(TK_LPAREN)) {
      // --- NATIVE/BRIDGE CALLS AS STATEMENTS ---
      int nid = -1;
      if (!strcmp(name, "clock"))
        nid = 0;
      else if (!strcmp(name, "input_int"))
        nid = 1;
      else if (!strcmp(name, "__bridge_print_str"))
        nid = 10;
      else if (!strcmp(name, "__bridge_print_int"))
        nid = 11;
      else if (!strcmp(name, "__bridge_print_float"))
        nid = 12;
      else if (!strcmp(name, "__bridge_pow"))
        nid = 20;
      else if (!strcmp(name, "__bridge_abs"))
        nid = 21;

      if (nid != -1) {
        if (cur.kind != TK_RPAREN) {
          do {
            int d1, d2;
            expression(&d1, &d2);
          } while (accept(TK_COMMA));
        }
        expect(TK_RPAREN);
        expect(TK_SEMI);
        emit(OP_NATIVE);
        emit32(nid);
        free(name);
        return;
      }
      // -----------------------------------------

      int fid = find_func(name);
      if (fid == -1)
        fail("Undefined func");
      int args = 0;
      DataType arg_t[32];
      int arg_s[32];
      int arg_a[32];
      if (cur.kind != TK_RPAREN) {
        do {
          if (args < 32)
            arg_t[args] = expression(&arg_s[args], &arg_a[args]);
          else {
            int d1, d2;
            expression(&d1, &d2);
          }
          args++;
        } while (accept(TK_COMMA));
      }
      expect(TK_RPAREN);
      expect(TK_SEMI);
      if (args != funcs[fid].arg_count)
        fail("Arg count mismatch calling '%s' (expected %d, got %d)", name,
             funcs[fid].arg_count, args);
      for (int i = 0; i < args && i < 32 && i < funcs[fid].arg_count; i++) {
        char ctx[128];
        snprintf(ctx, sizeof(ctx), "call to '%s' arg %d", name, i + 1);
        check_assign_compat(
            funcs[fid].arg_types[i], funcs[fid].arg_struct_ids[i],
            funcs[fid].arg_array_depths[i], arg_t[i], arg_s[i], arg_a[i], ctx);
      }
      check_args(args, name);
      emit_call_to(fid, args);
      for (int i = 0; i < funcs[fid].ret_count; i++)
        emit(OP_POP);
      free(name);
      return;
    }
    // Assignment or Inc/Dec
    int lid = find_local(name);
    int gid = find_global(name);

    // --- NAMESPACE RESOLUTION FOR STATEMENTS ---
    if (lid == -1 && gid == -1) {
      while (accept(TK_DOT)) {
        if (cur.kind != TK_ID)
          fail("Expected identifier after dot");
        size_t len = strlen(name) + 1 + strlen(cur.text) + 1;
        name = realloc(name, len);
        strcat(name, ".");
        strcat(name, cur.text);
        next();
      }
      // Re-check for function call after namespace resolution
      if (accept(TK_LPAREN)) {
        int nid = -1;
        if (!strcmp(name, "__bridge_print_str"))
          nid = 10;
        else if (!strcmp(name, "__bridge_print_int"))
          nid = 11;
        else if (!strcmp(name, "__bridge_print_float"))
          nid = 12;

        if (nid != -1) {
          if (cur.kind != TK_RPAREN) {
            do {
              int d1, d2;
              expression(&d1, &d2);
            } while (accept(TK_COMMA));
          }
          expect(TK_RPAREN);
          expect(TK_SEMI);
          emit(OP_NATIVE);
          emit32(nid);
          free(name);
          return;
        }

        int fid = find_func(name);
        if (fid == -1)
          fail("Undefined func '%s'", name);
        int args = 0;
        DataType arg_t[32];
        int arg_s[32];
        int arg_a[32];
        if (cur.kind != TK_RPAREN) {
          do {
            if (args < 32)
              arg_t[args] = expression(&arg_s[args], &arg_a[args]);
            else {
              int d1, d2;
              expression(&d1, &d2);
            }
            args++;
          } while (accept(TK_COMMA));
        }
        expect(TK_RPAREN);
        expect(TK_SEMI);
        if (args != funcs[fid].arg_count)
          fail("Arg count mismatch calling '%s' (expected %d, got %d)", name,
               funcs[fid].arg_count, args);
        for (int i = 0; i < args && i < 32 && i < funcs[fid].arg_count; i++) {
          char ctx[128];
          snprintf(ctx, sizeof(ctx), "call to '%s' arg %d", name, i + 1);
          check_assign_compat(funcs[fid].arg_types[i],
                              funcs[fid].arg_struct_ids[i],
                              funcs[fid].arg_array_depths[i], arg_t[i],
                              arg_s[i], arg_a[i], ctx);
        }
        check_args(args, name);
        emit(OP_CALL);
        emit32(funcs[fid].addr);
        emit(args);
        for (int i = 0; i < funcs[fid].ret_count; i++)
          emit(OP_POP);
        free(name);
        return;
      }
      fail("Undefined variable '%s'", name);
    }
    // -------------------------------------------

    if (accept(TK_INC)) {
      // --- FIX: Push the variable to the stack first ---
      if (lid != -1) {
        emit(OP_GET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_GET_GLOBAL);
        emit(gid);
      }
      // -------------------------------------------------
      emit(OP_CONST_INT);
      emit32(1);
      emit(OP_ADD);
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      expect(TK_SEMI);
      free(name);
      return;
    }
    if (accept(TK_DEC)) {
      // --- FIX: Push the variable to the stack first ---
      if (lid != -1) {
        emit(OP_GET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_GET_GLOBAL);
        emit(gid);
      }
      // -------------------------------------------------
      emit(OP_CONST_INT);
      emit32(1);
      emit(OP_SUB);
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      expect(TK_SEMI);
      free(name);
      return;
    }
    if (accept(TK_PLUS_ASSIGN)) {
      // --- FIX: Push the variable to the stack first ---
      if (lid != -1) {
        emit(OP_GET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_GET_GLOBAL);
        emit(gid);
      }
      // -------------------------------------------------
      int d1, d2;
      expression(&d1, &d2);
      emit(OP_ADD);
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      expect(TK_SEMI);
      free(name);
      return;
    }
    if (accept(TK_MINUS_ASSIGN)) {
      // --- FIX: Push the variable to the stack first ---
      if (lid != -1) {
        emit(OP_GET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_GET_GLOBAL);
        emit(gid);
      }
      // -------------------------------------------------
      int d1, d2;
      expression(&d1, &d2);
      emit(OP_SUB);
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      expect(TK_SEMI);
      free(name);
      return;
    }
    // --- Compound assigns: *=, /=, %=, <<=, >>=, &=, |=, ^= ---
    if (cur.kind == TK_MUL_ASSIGN || cur.kind == TK_DIV_ASSIGN ||
        cur.kind == TK_MOD_ASSIGN || cur.kind == TK_SHL_ASSIGN ||
        cur.kind == TK_SHR_ASSIGN || cur.kind == TK_BIT_AND_ASSIGN ||
        cur.kind == TK_BIT_OR_ASSIGN || cur.kind == TK_BIT_XOR_ASSIGN) {
      TkKind op = cur.kind;
      next();
      if (lid != -1) {
        emit(OP_GET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_GET_GLOBAL);
        emit(gid);
      }
      int d1, d2;
      expression(&d1, &d2);
      switch (op) {
      case TK_MUL_ASSIGN:
        emit(OP_MUL);
        break;
      case TK_DIV_ASSIGN:
        emit(OP_DIV);
        break;
      case TK_MOD_ASSIGN:
        emit(OP_MOD);
        break;
      case TK_SHL_ASSIGN:
        emit(OP_SHL);
        break;
      case TK_SHR_ASSIGN:
        emit(OP_SHR);
        break;
      case TK_BIT_AND_ASSIGN:
        emit(OP_BIT_AND);
        break;
      case TK_BIT_OR_ASSIGN:
        emit(OP_BIT_OR);
        break;
      case TK_BIT_XOR_ASSIGN:
        emit(OP_BIT_XOR);
        break;
      default:
        fail("internal: unhandled compound assign");
      }
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      expect(TK_SEMI);
      free(name);
      return;
    }

    // Field access assignment
    DataType t = TYPE_VOID;
    int sid = -1;
    if (lid != -1) {
      emit(OP_GET_LOCAL);
      emit(locals[lid].offset);
      t = locals[lid].type;
      sid = locals[lid].struct_id;
    } else {
      emit(OP_GET_GLOBAL);
      emit(gid);
      t = globals[gid].type;
      sid = globals[gid].struct_id;
    }

    while (1) {
      if (accept(TK_DOT)) {
        if (t != TYPE_STRUCT)
          fail("Dot on non-struct");
        if (cur.kind != TK_ID)
          fail("Expected field name");
        int parent_sid = sid;
        int field_idx = -1;
        for (int i = 0; i < structs[parent_sid].field_count; i++) {
          if (!strcmp(structs[parent_sid].fields[i].name, cur.text)) {
            field_idx = i;
            t = structs[parent_sid].fields[i].type;
            sid = structs[parent_sid].fields[i].struct_id;
            break;
          }
        }
        if (field_idx == -1)
          fail("Unknown field '%s'", cur.text);
        next();

        // --- INTERFACE METHOD CALL ---
        if (structs[parent_sid].is_interface && cur.kind == TK_LPAREN) {
          accept(TK_LPAREN);
          emit(OP_GET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);

          int args = 0;
          if (cur.kind != TK_RPAREN) {
            do {
              int d1, d2;
              expression(&d1, &d2);
              args++;
            } while (accept(TK_COMMA));
          }
          expect(TK_RPAREN);
          expect(TK_SEMI);

          if (args != structs[parent_sid].fields[field_idx].arg_count)
            fail("Interface method arg count mismatch");

          check_args(args, structs[parent_sid].fields[field_idx].name);
          emit(OP_CALL_DYN_BOT);
          emit(args);

          for (int i = 0; i < structs[parent_sid].fields[field_idx].ret_count;
               i++)
            emit(OP_POP);

          free(name);
          return;
        }
        // -----------------------------

        if (cur.kind == TK_ASSIGN) {
          next();
          int d1, d2;
          expression(&d1, &d2);
          // --- NEW: TAG THE ALLOCATION ---
          emit(OP_TAG_ALLOC);
          emit32(add_str(name));
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          expect(TK_SEMI);
          free(name);
          return;
        } else if (cur.kind == TK_PLUS_ASSIGN) {
          next();
          emit(OP_DUP);
          emit(OP_GET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          int d1, d2;
          DataType rhs_t = expression(&d1, &d2);
          if (t == TYPE_FLOAT || rhs_t == TYPE_FLOAT)
            emit(OP_ADD_F);
          else
            emit(OP_ADD);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          expect(TK_SEMI);
          free(name);
          return;
        } else if (cur.kind == TK_MINUS_ASSIGN) {
          next();
          emit(OP_DUP);
          emit(OP_GET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          int d1, d2;
          DataType rhs_t = expression(&d1, &d2);
          if (t == TYPE_FLOAT || rhs_t == TYPE_FLOAT)
            emit(OP_SUB_F);
          else
            emit(OP_SUB);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          expect(TK_SEMI);
          free(name);
          return;
        } else if (cur.kind == TK_INC) {
          next();
          emit(OP_DUP);
          emit(OP_GET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          emit(OP_CONST_INT);
          emit32(1);
          emit(OP_ADD);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          expect(TK_SEMI);
          free(name);
          return;
        } else if (cur.kind == TK_DEC) {
          next();
          emit(OP_DUP);
          emit(OP_GET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          emit(OP_CONST_INT);
          emit32(1);
          emit(OP_SUB);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
          expect(TK_SEMI);
          free(name);
          return;
        }
        emit(OP_GET_FIELD);
        emit(structs[parent_sid].fields[field_idx].offset);
      } else if (accept(TK_LBRACKET)) {
        int d1, d2;
        expression(&d1, &d2);
        expect(TK_RBRACKET);
        if (cur.kind == TK_ASSIGN) {
          next();
          int d1, d2;
          expression(&d1, &d2);
          // --- NEW: TAG THE ALLOCATION ---
          emit(OP_TAG_ALLOC);
          emit32(add_str(name));
          emit(OP_SET_INDEX);
          expect(TK_SEMI);
          free(name);
          return;
        } else if (cur.kind == TK_INC) {
          next();
          emit(OP_INC_INDEX);
          expect(TK_SEMI);
          free(name);
          return;
        } else if (cur.kind == TK_DEC) {
          next();
          emit(OP_DEC_INDEX);
          expect(TK_SEMI);
          free(name);
          return;
        }
        emit(OP_GET_INDEX);
        t = TYPE_INT;
      } else
        break;
    }

    if (accept(TK_ASSIGN)) {
      emit(OP_POP);
      int d1, d2;
      expression(&d1, &d2);
      // --- NEW: TAG THE ALLOCATION ---
      emit(OP_TAG_ALLOC);
      emit32(add_str(name));
      expect(TK_SEMI);
      if (lid != -1) {
        emit(OP_SET_LOCAL);
        emit(locals[lid].offset);
      } else {
        emit(OP_SET_GLOBAL);
        emit(gid);
      }
      free(name);
      return;
    }
    emit(OP_POP);
    expect(TK_SEMI);
    free(name);
    return;
  }

  int d1, d2;
  DataType expr_type = expression(&d1, &d2);
  if (expr_type != TYPE_VOID)
    emit(OP_POP);
  expect(TK_SEMI);
}

void parse_struct() {
  expect(TK_STRUCT);
  if (cur.kind != TK_ID)
    fail("Expected struct name");
  int sid = add_struct(strdup(cur.text));
  next();
  expect(TK_LBRACE);
  int offset = 0;
  while (cur.kind != TK_RBRACE) {
    if (offset >= MAX_STRUCT_FIELDS)
      fail("Struct '%s' has too many fields (max %d)", structs[sid].name,
           MAX_STRUCT_FIELDS);
    DataType t;
    int fsid;
    int ad;
    parse_type(&t, &fsid, &ad);
    if (cur.kind != TK_ID)
      fail("Expected field name");
    structs[sid].fields[offset].name = strdup(cur.text);
    structs[sid].fields[offset].type = t;
    structs[sid].fields[offset].struct_id = fsid;
    structs[sid].fields[offset].array_depth = ad;
    structs[sid].fields[offset].offset = offset;
    offset++;
    next();
    expect(TK_SEMI);
  }
  structs[sid].field_count = offset;
  structs[sid].size = offset;
  expect(TK_RBRACE);
}

void parse_enum() {
  expect(TK_ENUM);
  if (cur.kind != TK_ID)
    fail("Expected enum name");
  int eid = add_enum(strdup(cur.text));
  next();
  expect(TK_LBRACE);
  int current_val = 0;
  while (cur.kind != TK_RBRACE) {
    if (enums[eid].value_count >= MAX_ENUM_VALUES)
      fail("Enum '%s' has too many values (max %d)", enums[eid].name,
           MAX_ENUM_VALUES);
    if (cur.kind != TK_ID)
      fail("Expected enum value name");
    char *vname = strdup(cur.text);
    next();
    if (accept(TK_ASSIGN)) {
      int sign = 1;
      if (accept(TK_MINUS))
        sign = -1;
      if (cur.kind != TK_NUM_INT)
        fail("Expected integer for enum value");
      current_val = cur.ival * sign;
      next();
    }
    add_enum_value(eid, vname, current_val++);
    if (accept(TK_COMMA)) {
      // continue
    } else {
      break;
    }
  }
  expect(TK_RBRACE);
}

void parse_interface() {
  expect(TK_INTERFACE);
  if (cur.kind != TK_ID)
    fail("Expected interface name");
  int sid = add_struct(strdup(cur.text));
  structs[sid].is_interface = 1;
  next();
  expect(TK_LBRACE);
  int offset = 0;
  while (cur.kind != TK_RBRACE) {
    expect(TK_FUNCTION);
    if (cur.kind != TK_ID)
      fail("Expected method name");
    structs[sid].fields[offset].name = strdup(cur.text);
    structs[sid].fields[offset].type = TYPE_INT;
    structs[sid].fields[offset].struct_id = -1;
    structs[sid].fields[offset].array_depth = 0;
    structs[sid].fields[offset].offset = offset;
    next();

    expect(TK_LPAREN);
    int arg_count = 0;
    if (cur.kind != TK_RPAREN) {
      do {
        DataType at;
        int asid;
        int aad;
        parse_type(&at, &asid, &aad);
        if (cur.kind != TK_ID)
          fail("Expected arg name");
        next();
        arg_count++;
      } while (accept(TK_COMMA));
    }
    expect(TK_RPAREN);

    int ret_count = 0;
    DataType ret_types[8];
    int ret_sids[8];
    int ret_ads[8];

    if (accept(TK_COLON)) {
      if (accept(TK_LPAREN)) {
        do {
          parse_type(&ret_types[ret_count], &ret_sids[ret_count],
                     &ret_ads[ret_count]);
          ret_count++;
          if (cur.kind == TK_ID)
            next();
        } while (accept(TK_COMMA));
        expect(TK_RPAREN);
      } else {
        parse_type(&ret_types[ret_count], &ret_sids[ret_count],
                   &ret_ads[ret_count]);
        ret_count++;
      }
    } else {
      ret_types[0] = TYPE_VOID;
      ret_sids[0] = -1;
      ret_ads[0] = 0;
      ret_count = 1;
    }
    expect(TK_SEMI);

    structs[sid].fields[offset].arg_count = arg_count;
    structs[sid].fields[offset].ret_count = ret_count;
    for (int i = 0; i < ret_count; i++) {
      structs[sid].fields[offset].ret_types[i] = ret_types[i];
      structs[sid].fields[offset].ret_sids[i] = ret_sids[i];
      structs[sid].fields[offset].ret_ads[i] = ret_ads[i];
    }

    offset++;
  }
  structs[sid].field_count = offset;
  structs[sid].size = offset;
  expect(TK_RBRACE);
}

void parse_func_tail(DataType *ret_types, int *ret_sids, int *ret_ads,
                     int ret_count, char *name) {
  int fid = add_func(name, code_sz);
  funcs[fid].ret_count = ret_count;
  for (int i = 0; i < ret_count; i++) {
    funcs[fid].ret_types[i] = ret_types[i];
    funcs[fid].ret_struct_ids[i] = ret_sids[i];
    funcs[fid].ret_array_depths[i] = ret_ads[i];
  }
  expect(TK_LPAREN);
  local_count = 0;
  funcs[fid].arg_count = 0;
  if (cur.kind != TK_RPAREN) {
    do {
      DataType at;
      int asid;
      int aad;
      parse_type(&at, &asid, &aad);
      if (cur.kind != TK_ID)
        fail("Expected arg name");
      add_local(strdup(cur.text), at, asid, aad);
      int idx = funcs[fid].arg_count;
      if (idx < 32) {
        funcs[fid].arg_types[idx] = at;
        funcs[fid].arg_struct_ids[idx] = asid;
        funcs[fid].arg_array_depths[idx] = aad;
      }
      funcs[fid].arg_count++;
      next();
    } while (accept(TK_COMMA));
  }
  expect(TK_RPAREN);
  expect(TK_LBRACE);
  while (cur.kind != TK_RBRACE)
    statement();
  expect(TK_RBRACE);
  if (ret_types[0] == TYPE_VOID && ret_count == 1) {
    emit(OP_CONST_INT);
    emit32(0);
    emit(OP_RET);
    emit(1);
  }
}

void parse_func() {
  DataType ret_types[8];
  int ret_sids[8];
  int ret_ads[8];
  int ret_count = 0;
  if (accept(TK_FUNCTION)) {
    if (cur.kind != TK_ID)
      fail("Expected func name");
    char *name = strdup(cur.text);
    next();

    // --- NAMESPACE DEFINITION LOGIC ---
    while (accept(TK_DOT)) {
      if (cur.kind != TK_ID)
        fail("Expected identifier after dot");
      size_t len = strlen(name) + 1 + strlen(cur.text) + 1;
      name = realloc(name, len);
      strcat(name, ".");
      strcat(name, cur.text);
      next();
    }
    // ----------------------------------

    int fid = add_func(name, code_sz);
    expect(TK_LPAREN);
    local_count = 0;
    funcs[fid].arg_count = 0;
    if (cur.kind != TK_RPAREN) {
      do {
        DataType at;
        int asid;
        int aad;
        parse_type(&at, &asid, &aad);
        if (cur.kind != TK_ID)
          fail("Expected arg name");
        add_local(strdup(cur.text), at, asid, aad);
        int idx = funcs[fid].arg_count;
        if (idx < 32) {
          funcs[fid].arg_types[idx] = at;
          funcs[fid].arg_struct_ids[idx] = asid;
          funcs[fid].arg_array_depths[idx] = aad;
        }
        funcs[fid].arg_count++;
        next();
      } while (accept(TK_COMMA));
    }
    expect(TK_RPAREN);
    if (accept(TK_COLON)) {
      if (accept(TK_LPAREN)) {
        do {
          DataType t;
          int sid;
          int ad;
          parse_type(&t, &sid, &ad);
          ret_types[ret_count] = t;
          ret_sids[ret_count] = sid;
          ret_ads[ret_count] = ad;
          ret_count++;
          if (cur.kind == TK_ID) {
            add_local(strdup(cur.text), t, sid, ad);
            next();
            emit(OP_CONST_INT);
            emit32(0);
          }
        } while (accept(TK_COMMA));
        expect(TK_RPAREN);
      } else {
        DataType t;
        int sid;
        int ad;
        parse_type(&t, &sid, &ad);
        ret_types[ret_count] = t;
        ret_sids[ret_count] = sid;
        ret_ads[ret_count] = ad;
        ret_count++;
      }
    } else {
      ret_types[0] = TYPE_VOID;
      ret_sids[0] = -1;
      ret_ads[0] = 0;
      ret_count = 1;
    }
    funcs[fid].ret_count = ret_count;
    for (int i = 0; i < ret_count; i++) {
      funcs[fid].ret_types[i] = ret_types[i];
      funcs[fid].ret_struct_ids[i] = ret_sids[i];
      funcs[fid].ret_array_depths[i] = ret_ads[i];
    }
    expect(TK_LBRACE);
    while (cur.kind != TK_RBRACE)
      statement();
    expect(TK_RBRACE);
    if (ret_types[0] == TYPE_VOID && ret_count == 1) {
      emit(OP_CONST_INT);
      emit32(0);
      emit(OP_RET);
      emit(1);
    }
    return;
  }
  fail("Internal compiler error");
}

// ---------------------------------------------------------------------------
// Pass 1: signature scanner
// ---------------------------------------------------------------------------
// Walks the entire source before the real parse, registering every top-level
// symbol (structs, enums, interfaces, function signatures) so that pass 2 can
// call a function defined anywhere in the file — including mutual recursion.
//
// Pass 1 never emits bytecode. It only populates symbol tables. Function
// bodies are skipped by brace-counting. Globals are skipped entirely (their
// initializer bytecode is emitted in pass 2).
//
// Functions registered here have addr = 0xFFFFFFFF (sentinel). Pass 2 calls
// add_func again for the same name and overwrites addr with the real value.
// ---------------------------------------------------------------------------

static void skip_brace_block(void) {
  expect(TK_LBRACE);
  int depth = 1;
  while (depth > 0 && cur.kind != TK_EOF) {
    if (cur.kind == TK_LBRACE)
      depth++;
    else if (cur.kind == TK_RBRACE) {
      depth--;
      if (depth == 0)
        break;
    }
    next();
  }
  expect(TK_RBRACE);
}

static void scan_func_declaration(void) {
  expect(TK_FUNCTION);
  if (cur.kind != TK_ID)
    fail("Expected function name");
  char *name = strdup(cur.text);
  next();
  while (accept(TK_DOT)) {
    if (cur.kind != TK_ID)
      fail("Expected identifier after dot");
    size_t len = strlen(name) + 1 + strlen(cur.text) + 1;
    name = realloc(name, len);
    strcat(name, ".");
    strcat(name, cur.text);
    next();
  }
  int fid = add_func(name, 0xFFFFFFFF);
  funcs[fid].arg_count = 0;

  expect(TK_LPAREN);
  if (cur.kind != TK_RPAREN) {
    do {
      DataType at;
      int asid, aad;
      parse_type(&at, &asid, &aad);
      if (cur.kind != TK_ID)
        fail("Expected arg name");
      next();
      int idx = funcs[fid].arg_count;
      if (idx < 32) {
        funcs[fid].arg_types[idx] = at;
        funcs[fid].arg_struct_ids[idx] = asid;
        funcs[fid].arg_array_depths[idx] = aad;
      }
      funcs[fid].arg_count++;
    } while (accept(TK_COMMA));
  }
  expect(TK_RPAREN);

  int ret_count = 0;
  if (accept(TK_COLON)) {
    if (accept(TK_LPAREN)) {
      do {
        if (ret_count >= MAX_RET_VALUES)
          fail("Too many return values (max %d)", MAX_RET_VALUES);
        parse_type(&funcs[fid].ret_types[ret_count],
                   &funcs[fid].ret_struct_ids[ret_count],
                   &funcs[fid].ret_array_depths[ret_count]);
        ret_count++;
        if (cur.kind == TK_ID)
          next(); // skip name
      } while (accept(TK_COMMA));
      expect(TK_RPAREN);
    } else {
      parse_type(&funcs[fid].ret_types[0], &funcs[fid].ret_struct_ids[0],
                 &funcs[fid].ret_array_depths[0]);
      ret_count = 1;
    }
  } else {
    funcs[fid].ret_types[0] = TYPE_VOID;
    funcs[fid].ret_struct_ids[0] = -1;
    funcs[fid].ret_array_depths[0] = 0;
    ret_count = 1;
  }
  funcs[fid].ret_count = ret_count;

  skip_brace_block();
}

static void scan_typed_declaration(void) {
  DataType t;
  int sid, ad;
  parse_type(&t, &sid, &ad);
  if (cur.kind != TK_ID)
    fail("Expected identifier");
  char *name = strdup(cur.text);
  next();
  if (cur.kind == TK_LPAREN) {
    // `<type> name(args) { body }` — typed function form
    int fid = add_func(name, 0xFFFFFFFF);
    funcs[fid].arg_count = 0;
    expect(TK_LPAREN);
    if (cur.kind != TK_RPAREN) {
      do {
        DataType at;
        int asid, aad;
        parse_type(&at, &asid, &aad);
        if (cur.kind != TK_ID)
          fail("Expected arg name");
        next();
        int idx = funcs[fid].arg_count;
        if (idx < 32) {
          funcs[fid].arg_types[idx] = at;
          funcs[fid].arg_struct_ids[idx] = asid;
          funcs[fid].arg_array_depths[idx] = aad;
        }
        funcs[fid].arg_count++;
      } while (accept(TK_COMMA));
    }
    expect(TK_RPAREN);
    funcs[fid].ret_count = 1;
    funcs[fid].ret_types[0] = t;
    funcs[fid].ret_struct_ids[0] = sid;
    funcs[fid].ret_array_depths[0] = ad;
    skip_brace_block();
  } else {
    // Global — skip initializer, register in pass 2.
    while (cur.kind != TK_SEMI && cur.kind != TK_EOF)
      next();
    expect(TK_SEMI);
    free(name);
  }
}

static void scan_program(void) {
  next();
  while (cur.kind != TK_EOF) {
    if (cur.kind == TK_IMPORT) {
      next();
      char path[256] = {0};
      if (cur.kind != TK_ID)
        fail("Expected module name");
      strcat(path, cur.text);
      next();
      while (accept(TK_DOT)) {
        strcat(path, "/");
        if (cur.kind != TK_ID)
          fail("Expected module part");
        strcat(path, cur.text);
        next();
      }
      strcat(path, ".al");
      expect(TK_SEMI);
      lexer_include(path);
    } else if (cur.kind == TK_ENUM) {
      parse_enum();
    } else if (cur.kind == TK_INTERFACE) {
      parse_interface();
    } else if (cur.kind == TK_STRUCT) {
      parse_struct();
    } else if (cur.kind == TK_FUNCTION) {
      scan_func_declaration();
    } else {
      scan_typed_declaration();
    }
  }
}

void parse_program() {
  // Pass 1: register every top-level symbol. No bytecode emitted — parse_*
  // functions called here only touch symbol tables; the scanner skips function
  // bodies and global initializers (both are bytecode-producing).
  scan_program();

  // Reset lexer to the start of the top-level source for the second pass.
  lexer_reset_to_start();
  lexer_reset_imports();

  // Pass 2: full parse with bytecode emission. All forward references to
  // functions now resolve through the patch table (emit_call_to), which the
  // resolver at the end of this function rewrites to real addresses.
  next();
  while (cur.kind != TK_EOF) {
    if (cur.kind == TK_IMPORT) {
      next();
      char path[256] = {0};
      if (cur.kind != TK_ID)
        fail("Expected module name");
      strcat(path, cur.text);
      next();
      while (accept(TK_DOT)) {
        strcat(path, "/");
        if (cur.kind != TK_ID)
          fail("Expected module part");
        strcat(path, cur.text);
        next();
      }
      strcat(path, ".al");
      expect(TK_SEMI);
      lexer_include(path);
      continue;
    } else if (cur.kind == TK_ENUM) {
      parse_enum();
    } else if (cur.kind == TK_INTERFACE) {
      parse_interface();
    } else if (cur.kind == TK_STRUCT) {
      parse_struct();
    } else if (cur.kind == TK_FUNCTION) {
      emit(OP_JMP);
      size_t patch = code_sz;
      emit32(0);
      parse_func();
      emit_patch(patch, code_sz);
    } else {
      DataType t;
      int sid;
      int ad;
      parse_type(&t, &sid, &ad);
      if (cur.kind != TK_ID)
        fail("Expected identifier");
      char *name = strdup(cur.text);
      next();
      if (cur.kind == TK_LPAREN) {
        emit(OP_JMP);
        size_t patch = code_sz;
        emit32(0);
        DataType ret_types[1] = {t};
        int ret_sids[1] = {sid};
        int ret_ads[1] = {ad};
        parse_func_tail(ret_types, ret_sids, ret_ads, 1, name);
        emit_patch(patch, code_sz);
      } else {
        add_global(name, t, sid, ad);
        int gid = global_count - 1;
        if (cur.kind == TK_ASSIGN) {
          next();
          int d1, d2;
          expression(&d1, &d2);
          // --- NEW: TAG THE ALLOCATION ---
          emit(OP_TAG_ALLOC);
          emit32(add_str(name));
          emit(OP_SET_GLOBAL);
          emit(gid);
        }
        expect(TK_SEMI);
      }
    }
  }

  // Resolve every OP_CALL placeholder address recorded during pass 2.
  resolve_call_patches();
}
