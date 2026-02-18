#include "../include/parser.h"
#include "../include/codegen.h"
#include "../include/lexer.h"
#include "../include/symbols.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- FORWARD DECLARATIONS ---
DataType expression(int *struct_id, int *array_depth);
void statement();
void parse_struct();
void parse_func();
void parse_func_tail(DataType *ret_types, int *ret_sids, int *ret_ads,
                     int ret_count, char *name);

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
    if (sid != -1) {
      *type = TYPE_STRUCT;
      *struct_id = sid;
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
    expect(TK_RPAREN);
    if (t != TYPE_STRUCT)
      fail("stack() expects struct");
    emit(OP_ALLOC_STACK);
    emit32(sid);
    *struct_id = sid;
    return TYPE_STRUCT;
  }
  if (accept(TK_NEW)) {
    expect(TK_LPAREN);
    DataType t;
    int sid;
    int ad;
    parse_type(&t, &sid, &ad);
    if (accept(TK_COMMA)) {
      int d1, d2;
      DataType size_t = expression(&d1, &d2);
      if (size_t != TYPE_INT)
        fail("Array size must be int");
      expect(TK_RPAREN);
      emit(OP_ALLOC_ARRAY);
      emit32(8);
      *struct_id = sid;
      *array_depth = ad + 1;
      return TYPE_ARRAY;
    } else {
      expect(TK_RPAREN);
      if (t != TYPE_STRUCT)
        fail("new() expects struct");
      emit(OP_ALLOC_STRUCT);
      emit32(sid);
      *struct_id = sid;
      return TYPE_STRUCT;
    }
  }

  if (cur.kind == TK_ID) {
    char *name = strdup(cur.text);
    next();

    // --- NAMESPACE RESOLUTION LOGIC (EXPRESSIONS) ---
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
      if (cur.kind != TK_RPAREN) {
        do {
          int d1, d2;
          expression(&d1, &d2);
          args++;
        } while (accept(TK_COMMA));
      }
      expect(TK_RPAREN);
      if (args != funcs[fid].arg_count)
        fail("Arg count mismatch");
      emit(OP_CALL);
      emit32(funcs[fid].addr);
      emit(args);
      free(name);
      *struct_id = funcs[fid].ret_struct_ids[0];
      *array_depth = funcs[fid].ret_array_depths[0];
      return funcs[fid].ret_types[0];
    }

    if (lid == -1 && gid == -1)
      fail("Undefined variable '%s'", name);

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
        if (cur.kind == TK_ASSIGN) {
          next();
          int d1, d2;
          expression(&d1, &d2);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
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
          free(name);
          return t;
        } else if (cur.kind == TK_INC) {
          next();
          emit(OP_INC_INDEX);
          free(name);
          return t;
        } else if (cur.kind == TK_DEC) {
          next();
          emit(OP_DEC_INDEX);
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
      free(name);
      return t;
    } else if (accept(TK_DEC)) {
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
      free(name);
      return t;
    } else if (accept(TK_PLUS_ASSIGN)) {
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
      free(name);
      return t;
    } else if (accept(TK_MINUS_ASSIGN)) {
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

DataType term(int *struct_id, int *array_depth) {
  DataType t = factor(struct_id, array_depth);
  while (cur.kind == TK_MUL || cur.kind == TK_DIV || cur.kind == TK_MOD) {
    int op = cur.kind;
    next();
    int d1, d2;
    DataType t2 = factor(&d1, &d2);
    if (op == TK_MOD)
      emit(OP_MOD);
    else if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) {
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
    if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) {
      emit(op == TK_PLUS ? OP_ADD_F : OP_SUB_F);
      t = TYPE_FLOAT;
    } else
      emit(op == TK_PLUS ? OP_ADD : OP_SUB);
  }
  return t;
}

DataType rel_expr(int *struct_id, int *array_depth) {
  DataType t = add_expr(struct_id, array_depth);
  if (cur.kind >= TK_EQ && cur.kind <= TK_GE) {
    int op = cur.kind;
    next();
    int d1, d2;
    DataType t2 = add_expr(&d1, &d2);
    if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) {
      if (op == TK_EQ)
        emit(OP_EQ_F);
      else if (op == TK_NE)
        emit(OP_NE_F);
      else if (op == TK_LT)
        emit(OP_LT_F);
      else if (op == TK_LE)
        emit(OP_LE_F);
      else if (op == TK_GT)
        emit(OP_GT_F);
      else if (op == TK_GE)
        emit(OP_GE_F);
    } else {
      if (op == TK_EQ)
        emit(OP_EQ);
      else if (op == TK_NE)
        emit(OP_NE);
      else if (op == TK_LT)
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

DataType expression(int *struct_id, int *array_depth) {
  DataType t = rel_expr(struct_id, array_depth);
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
      if (accept(TK_INC)) {
        if (lid != -1) {
          emit(OP_GET_LOCAL);
          emit(locals[lid].offset);
          emit(OP_CONST_INT);
          emit32(1);
          emit(OP_ADD);
          emit(OP_SET_LOCAL);
          emit(locals[lid].offset);
        } else {
          emit(OP_GET_GLOBAL);
          emit(gid);
          emit(OP_CONST_INT);
          emit32(1);
          emit(OP_ADD);
          emit(OP_SET_GLOBAL);
          emit(gid);
        }
      } else if (accept(TK_DEC)) {
        if (lid != -1) {
          emit(OP_GET_LOCAL);
          emit(locals[lid].offset);
          emit(OP_CONST_INT);
          emit32(1);
          emit(OP_SUB);
          emit(OP_SET_LOCAL);
          emit(locals[lid].offset);
        } else {
          emit(OP_GET_GLOBAL);
          emit(gid);
          emit(OP_CONST_INT);
          emit32(1);
          emit(OP_SUB);
          emit(OP_SET_GLOBAL);
          emit(gid);
        }
      } else if (accept(TK_ASSIGN)) {
        int d1, d2;
        expression(&d1, &d2);
        if (lid != -1) {
          emit(OP_SET_LOCAL);
          emit(locals[lid].offset);
        } else {
          emit(OP_SET_GLOBAL);
          emit(gid);
        }
      }
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
    if (cur.kind != TK_ID || find_struct(cur.text) != -1)
      is_decl = 1;
    if (is_decl) {
      parse_type(&type, &sid, &ad);
      if (cur.kind == TK_ID) {
        char *name = strdup(cur.text);
        next();
        if (accept(TK_ASSIGN)) {
          int rhs_sid, rhs_ad;
          expression(&rhs_sid, &rhs_ad);
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
      expect(TK_LPAREN);
      int fid = find_func(func_name);
      if (fid == -1)
        fail("Undefined func");
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
      emit(OP_CALL);
      emit32(funcs[fid].addr);
      emit(args);
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
      if (cur.kind != TK_RPAREN) {
        do {
          int d1, d2;
          expression(&d1, &d2);
          args++;
        } while (accept(TK_COMMA));
      }
      expect(TK_RPAREN);
      expect(TK_SEMI);
      emit(OP_CALL);
      emit32(funcs[fid].addr);
      emit(args);
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
        if (cur.kind != TK_RPAREN) {
          do {
            int d1, d2;
            expression(&d1, &d2);
            args++;
          } while (accept(TK_COMMA));
        }
        expect(TK_RPAREN);
        expect(TK_SEMI);
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
        if (cur.kind == TK_ASSIGN) {
          next();
          int d1, d2;
          expression(&d1, &d2);
          emit(OP_SET_FIELD);
          emit(structs[parent_sid].fields[field_idx].offset);
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
          emit(OP_SET_INDEX);
          free(name);
          return;
        } else if (cur.kind == TK_INC) {
          next();
          emit(OP_INC_INDEX);
          free(name);
          return;
        } else if (cur.kind == TK_DEC) {
          next();
          emit(OP_DEC_INDEX);
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
  if (cur.kind != TK_RPAREN) {
    do {
      DataType at;
      int asid;
      int aad;
      parse_type(&at, &asid, &aad);
      if (cur.kind != TK_ID)
        fail("Expected arg name");
      add_local(strdup(cur.text), at, asid, aad);
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
    if (cur.kind != TK_RPAREN) {
      do {
        DataType at;
        int asid;
        int aad;
        parse_type(&at, &asid, &aad);
        if (cur.kind != TK_ID)
          fail("Expected arg name");
        add_local(strdup(cur.text), at, asid, aad);
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

void parse_program() {
  next();
  while (cur.kind != TK_EOF) {
    if (cur.kind == TK_IMPORT) {
      next(); // skip 'import'
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
          emit(OP_SET_GLOBAL);
          emit(gid);
        }
        expect(TK_SEMI);
      }
    }
  }
}
