#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LOCALS 256
#define MAX_GLOBALS 512
#define MAX_FUNCS 512
#define MAX_STRUCTS 128
#define MAX_FIELDS 64
#define MAGIC "ABYSSBC"
#define VERSION 11

typedef enum { TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_CHAR, TYPE_STR, TYPE_STRUCT, TYPE_ARRAY } DataType;

enum {
    OP_HALT = 0,
    OP_CONST_INT, OP_CONST_FLOAT, OP_CONST_STR,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_ADD_F, OP_SUB_F, OP_MUL_F, OP_DIV_F,
    OP_LT, OP_LE, OP_GT, OP_GE, OP_EQ, OP_NE,
    OP_LT_F, OP_LE_F, OP_GT_F, OP_GE_F, OP_EQ_F, OP_NE_F,
    OP_JMP, OP_JZ,
    OP_PRINT, OP_PRINT_F, OP_PRINT_STR, OP_PRINT_CHAR,
    OP_GET_GLOBAL, OP_SET_GLOBAL,
    OP_GET_LOCAL, OP_SET_LOCAL,
    OP_CALL, OP_RET, OP_POP,
    OP_ALLOC_STRUCT, OP_ALLOC_ARRAY, OP_FREE,
    OP_GET_FIELD, OP_SET_FIELD,
    OP_GET_INDEX, OP_SET_INDEX,
    OP_ABYSS_EYE,
    OP_MOD,
    OP_NEG,
    OP_NEG_F,
    OP_PRINT_FMT,
    OP_TRY,
    OP_END_TRY,
    OP_THROW,
    OP_NATIVE,
    OP_DUP // <--- NEW OPCODE
};

typedef enum {
    TK_EOF, TK_ID, TK_NUM_INT, TK_NUM_FLOAT, TK_STR,
    TK_INT, TK_FLOAT, TK_CHAR, TK_VOID, TK_STR_TYPE,
    TK_STRUCT, TK_NEW, TK_FREE, TK_EYE,
    TK_IF, TK_ELSE, TK_WHILE, TK_FOR,
    TK_FUNCTION, TK_TRY, TK_CATCH, TK_THROW,
    TK_RETURN, TK_PRINT, TK_PRINT_CHAR,
    TK_LPAREN, TK_RPAREN, TK_LBRACE, TK_RBRACE, TK_LBRACKET, TK_RBRACKET,
    TK_SEMI, TK_COMMA, TK_ASSIGN, TK_DOT, TK_COLON,
    TK_PLUS, TK_MINUS, TK_MUL, TK_DIV, TK_MOD,
    TK_EQ, TK_NE, TK_LT, TK_GT, TK_LE, TK_GE,
    TK_INC, TK_DEC, TK_PLUS_ASSIGN, TK_MINUS_ASSIGN,
    TK_QUESTION
} TkKind;

typedef struct {
    TkKind kind;
    char *text;
    int64_t ival;
    double fval;
    int line;
    int col;
} Token;

typedef struct {
    char *name;
    DataType type;
    int struct_id;
    int array_depth;
    int offset;
} Symbol;

typedef struct {
    char *name;
    int offset;
    DataType type;
    int struct_id;
    int array_depth;
} Field;

typedef struct {
    char *name;
    Field fields[MAX_FIELDS];
    int field_count;
    int size;
} StructInfo;

typedef struct {
    char *name;
    uint32_t addr;
    DataType ret_types[8];
    int ret_struct_ids[8];
    int ret_array_depths[8];
    int ret_count;
    int arg_count;
} FuncInfo;

static char *src;
static size_t pos = 0;
static int line = 1;
static int col = 1;
static Token cur;

static uint8_t *code = NULL;
static size_t code_sz = 0, code_cap = 0;

static char *strs[2048];
static int str_count = 0;

static Symbol globals[MAX_GLOBALS];
static int global_count = 0;

static Symbol locals[MAX_LOCALS];
static int local_count = 0;

static FuncInfo funcs[MAX_FUNCS];
static int func_count = 0;

static StructInfo structs[MAX_STRUCTS];
static int struct_count = 0;

void fail(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[1;31mError at Line %d, Col %d:\033[0m ", cur.line, cur.col);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    if (cur.text) fprintf(stderr, "Context: Near token '%s'\n", cur.text);
    exit(1);
}

void emit(uint8_t b) {
    if (code_sz >= code_cap) {
        code_cap = code_cap ? code_cap * 2 : 1024;
        code = realloc(code, code_cap);
        if (!code) fail("Out of memory (code)");
    }
    code[code_sz++] = b;
}
void emit32(uint32_t v) { emit(v&0xFF); emit((v>>8)&0xFF); emit((v>>16)&0xFF); emit((v>>24)&0xFF); }
void emit64(uint64_t v) { emit32(v & 0xFFFFFFFF); emit32(v >> 32); }
void emit_patch(size_t addr, uint32_t v) {
    if (addr + 4 > code_sz) fail("Patch out of bounds");
    code[addr] = v&0xFF; code[addr+1] = (v>>8)&0xFF;
    code[addr+2] = (v>>16)&0xFF; code[addr+3] = (v>>24)&0xFF;
}

int add_str(const char *s) {
    if (str_count >= 2048) fail("Too many strings");
    strs[str_count] = strdup(s);
    return str_count++;
}

void next() {
    if (cur.text) free(cur.text);
    cur.text = NULL;
    while (src[pos] && isspace(src[pos])) {
        if (src[pos] == '\n') { line++; col = 1; } else { col++; }
        pos++;
    }
    cur.line = line; cur.col = col;

    if (!src[pos]) { cur.kind = TK_EOF; return; }
    char *start = src + pos;
    if (src[pos] == '/' && src[pos+1] == '/') {
        while (src[pos] && src[pos] != '\n') pos++;
        next(); return;
    }
    if (isalpha(src[pos])) {
        while (isalnum(src[pos]) || src[pos] == '_') { pos++; col++; }
        int len = (src + pos) - start;
        cur.text = strndup(start, len);
        if (!strcmp(cur.text, "int")) cur.kind = TK_INT;
        else if (!strcmp(cur.text, "float")) cur.kind = TK_FLOAT;
        else if (!strcmp(cur.text, "char")) cur.kind = TK_CHAR;
        else if (!strcmp(cur.text, "void")) cur.kind = TK_VOID;
        else if (!strcmp(cur.text, "str")) cur.kind = TK_STR_TYPE;
        else if (!strcmp(cur.text, "struct")) cur.kind = TK_STRUCT;
        else if (!strcmp(cur.text, "new")) cur.kind = TK_NEW;
        else if (!strcmp(cur.text, "free")) cur.kind = TK_FREE;
        else if (!strcmp(cur.text, "abyss_eye")) cur.kind = TK_EYE;
        else if (!strcmp(cur.text, "if")) cur.kind = TK_IF;
        else if (!strcmp(cur.text, "else")) cur.kind = TK_ELSE;
        else if (!strcmp(cur.text, "while")) cur.kind = TK_WHILE;
        else if (!strcmp(cur.text, "for")) cur.kind = TK_FOR;
        else if (!strcmp(cur.text, "function")) cur.kind = TK_FUNCTION;
        else if (!strcmp(cur.text, "try")) cur.kind = TK_TRY;
        else if (!strcmp(cur.text, "catch")) cur.kind = TK_CATCH;
        else if (!strcmp(cur.text, "throw")) cur.kind = TK_THROW;
        else if (!strcmp(cur.text, "return")) cur.kind = TK_RETURN;
        else if (!strcmp(cur.text, "print")) cur.kind = TK_PRINT;
        else if (!strcmp(cur.text, "print_char")) cur.kind = TK_PRINT_CHAR;
        else if (!strcmp(cur.text, "error")) cur.kind = TK_STR_TYPE;
        else cur.kind = TK_ID;
        return;
    }
    if (isdigit(src[pos])) {
        while (isdigit(src[pos])) { pos++; col++; }
        if (src[pos] == '.') {
            pos++; col++; while (isdigit(src[pos])) { pos++; col++; }
            cur.kind = TK_NUM_FLOAT;
            cur.text = strndup(start, (src+pos)-start);
            cur.fval = strtod(cur.text, NULL);
        } else {
            cur.kind = TK_NUM_INT;
            cur.text = strndup(start, (src+pos)-start);
            cur.ival = strtoll(cur.text, NULL, 10);
        }
        return;
    }
    if (src[pos] == '"') {
        pos++; col++; char *s = src + pos;
        while (src[pos] && src[pos] != '"') {
            if (src[pos] == '\n') { line++; col = 1; } else col++;
            pos++;
        }
        int len = (src + pos) - s;
        if (src[pos] == '"') { pos++; col++; }
        cur.text = strndup(s, len);
        cur.kind = TK_STR;
        return;
    }
    if (!strncmp(src+pos, "==", 2)) { pos+=2; col+=2; cur.kind = TK_EQ; return; }
    if (!strncmp(src+pos, "!=", 2)) { pos+=2; col+=2; cur.kind = TK_NE; return; }
    if (!strncmp(src+pos, "<=", 2)) { pos+=2; col+=2; cur.kind = TK_LE; return; }
    if (!strncmp(src+pos, ">=", 2)) { pos+=2; col+=2; cur.kind = TK_GE; return; }
    if (!strncmp(src+pos, "++", 2)) { pos+=2; col+=2; cur.kind = TK_INC; return; }
    if (!strncmp(src+pos, "--", 2)) { pos+=2; col+=2; cur.kind = TK_DEC; return; }
    if (!strncmp(src+pos, "+=", 2)) { pos+=2; col+=2; cur.kind = TK_PLUS_ASSIGN; return; }
    if (!strncmp(src+pos, "-=", 2)) { pos+=2; col+=2; cur.kind = TK_MINUS_ASSIGN; return; }

    char c = src[pos++]; col++;
    cur.text = malloc(2); cur.text[0] = c; cur.text[1] = 0;
    switch(c) {
        case '(': cur.kind = TK_LPAREN; break;
        case ')': cur.kind = TK_RPAREN; break;
        case '{': cur.kind = TK_LBRACE; break;
        case '}': cur.kind = TK_RBRACE; break;
        case '[': cur.kind = TK_LBRACKET; break;
        case ']': cur.kind = TK_RBRACKET; break;
        case ';': cur.kind = TK_SEMI; break;
        case ',': cur.kind = TK_COMMA; break;
        case '.': cur.kind = TK_DOT; break;
        case ':': cur.kind = TK_COLON; break;
        case '=': cur.kind = TK_ASSIGN; break;
        case '+': cur.kind = TK_PLUS; break;
        case '-': cur.kind = TK_MINUS; break;
        case '*': cur.kind = TK_MUL; break;
        case '/': cur.kind = TK_DIV; break;
        case '%': cur.kind = TK_MOD; break;
        case '<': cur.kind = TK_LT; break;
        case '>': cur.kind = TK_GT; break;
        case '?': cur.kind = TK_QUESTION; break;
        default: fail("Unknown char '%c'", c);
    }
}

int accept(TkKind k) { if (cur.kind == k) { next(); return 1; } return 0; }
void expect(TkKind k) { if (!accept(k)) fail("Expected token kind %d, got %d ('%s')", k, cur.kind, cur.text ? cur.text : "?"); }

int find_local(const char *name) {
    for (int i = local_count - 1; i >= 0; i--) if (!strcmp(locals[i].name, name)) return i;
    return -1;
}
int find_global(const char *name) {
    for (int i = 0; i < global_count; i++) if (!strcmp(globals[i].name, name)) return i;
    return -1;
}
int find_func(const char *name) {
    for (int i = 0; i < func_count; i++) if (!strcmp(funcs[i].name, name)) return i;
    return -1;
}
int find_struct(const char *name) {
    for (int i = 0; i < struct_count; i++) if (!strcmp(structs[i].name, name)) return i;
    return -1;
}

void parse_type(DataType *type, int *struct_id, int *array_depth) {
    *struct_id = -1; *array_depth = 0;
    if (accept(TK_INT)) *type = TYPE_INT;
    else if (accept(TK_FLOAT)) *type = TYPE_FLOAT;
    else if (accept(TK_CHAR)) *type = TYPE_CHAR;
    else if (accept(TK_STR_TYPE)) *type = TYPE_STR;
    else if (accept(TK_VOID)) *type = TYPE_VOID;
    else if (cur.kind == TK_ID) {
        int sid = find_struct(cur.text);
        if (sid != -1) { *type = TYPE_STRUCT; *struct_id = sid; next(); }
        else fail("Unknown type '%s'", cur.text);
    } else fail("Expected type");
    while (accept(TK_LBRACKET)) { expect(TK_RBRACKET); *array_depth += 1; *type = TYPE_ARRAY; }
}

DataType expression(int *struct_id, int *array_depth);

DataType factor(int *struct_id, int *array_depth) {
    *struct_id = -1; *array_depth = 0;
    if (accept(TK_MINUS)) {
        int d1, d2; DataType t = factor(&d1, &d2);
        if (t == TYPE_INT) emit(OP_NEG);
        else if (t == TYPE_FLOAT) emit(OP_NEG_F);
        else fail("Cannot negate type %d", t);
        return t;
    }
    if (cur.kind == TK_NUM_INT) { emit(OP_CONST_INT); emit32((uint32_t)cur.ival); next(); return TYPE_INT; }
    if (cur.kind == TK_NUM_FLOAT) { emit(OP_CONST_FLOAT); uint64_t v; memcpy(&v, &cur.fval, 8); emit64(v); next(); return TYPE_FLOAT; }
    if (cur.kind == TK_STR) { int idx = add_str(cur.text); emit(OP_CONST_STR); emit32(idx); next(); return TYPE_STR; }
    if (accept(TK_NEW)) {
        expect(TK_LPAREN); DataType t; int sid; int ad; parse_type(&t, &sid, &ad);
        if (accept(TK_COMMA)) {
            int d1, d2; DataType size_t = expression(&d1, &d2);
            if (size_t != TYPE_INT) fail("Array size must be int");
            expect(TK_RPAREN); emit(OP_ALLOC_ARRAY); emit32(8);
            *struct_id = sid; *array_depth = ad + 1; return TYPE_ARRAY;
        } else {
            expect(TK_RPAREN); if (t != TYPE_STRUCT) fail("new() expects struct or array size");
            emit(OP_ALLOC_STRUCT); emit32(sid); *struct_id = sid; return TYPE_STRUCT;
        }
    }
    if (cur.kind == TK_ID) {
        char *name = strdup(cur.text); next();
        if (accept(TK_LPAREN)) {
            int nid = -1; DataType ret = TYPE_VOID;
            if (!strcmp(name, "clock")) { nid = 0; ret = TYPE_FLOAT; }
            else if (!strcmp(name, "input_int")) { nid = 1; ret = TYPE_INT; }

            if (nid != -1) {
                expect(TK_RPAREN); emit(OP_NATIVE); emit32(nid); free(name);
                *struct_id = -1; *array_depth = 0; return ret;
            }

            int fid = find_func(name); if (fid == -1) fail("Undefined function '%s'", name);
            int args = 0;
            if (cur.kind != TK_RPAREN) {
                do { int d1, d2; expression(&d1, &d2); args++; } while (accept(TK_COMMA));
            }
            expect(TK_RPAREN); if (args != funcs[fid].arg_count) fail("Arg count mismatch");
            emit(OP_CALL); emit32(funcs[fid].addr); emit(args);

            // FIX: Removed OP_POP loop here. VM OP_RET cleans up args.

            free(name);

            if (funcs[fid].ret_count > 1) fail("Cannot use multi-return function in expression");

            *struct_id = funcs[fid].ret_struct_ids[0];
            *array_depth = funcs[fid].ret_array_depths[0];
            return funcs[fid].ret_types[0];
        }
        DataType t = TYPE_VOID; int sid = -1; int ad = 0;
        int lid = find_local(name);
        if (lid != -1) { emit(OP_GET_LOCAL); emit(locals[lid].offset); t = locals[lid].type; sid = locals[lid].struct_id; ad = locals[lid].array_depth; }
        else {
            int gid = find_global(name);
            if (gid != -1) { emit(OP_GET_GLOBAL); emit(gid); t = globals[gid].type; sid = globals[gid].struct_id; ad = globals[gid].array_depth; }
            else fail("Undefined variable '%s'", name);
        }
        free(name);
        while (1) {
            if (accept(TK_DOT)) {
                if (t != TYPE_STRUCT) fail("Dot on non-struct");
                if (sid < 0 || sid >= struct_count) fail("Invalid struct ID %d", sid);
                if (cur.kind != TK_ID) fail("Expected field name");
                int found = 0;
                for (int i=0; i<structs[sid].field_count; i++) {
                    if (!strcmp(structs[sid].fields[i].name, cur.text)) {
                        emit(OP_GET_FIELD); emit(structs[sid].fields[i].offset);
                        t = structs[sid].fields[i].type; sid = structs[sid].fields[i].struct_id; ad = structs[sid].fields[i].array_depth;
                        found = 1; break;
                    }
                }
                if (!found) fail("Unknown field '%s'", cur.text);
                next();
            } else if (accept(TK_LBRACKET)) {
                if (t != TYPE_ARRAY) fail("Indexing non-array");
                int d1, d2; DataType idx_t = expression(&d1, &d2);
                if (idx_t != TYPE_INT) fail("Index must be int");
                expect(TK_RBRACKET); emit(OP_GET_INDEX); t = TYPE_INT;
            } else break;
        }
        *struct_id = sid; *array_depth = ad; return t;
    }
    if (accept(TK_LPAREN)) { DataType t = expression(struct_id, array_depth); expect(TK_RPAREN); return t; }
    fail("Unexpected token '%s' (Kind: %d)", cur.text ? cur.text : "EOF", cur.kind);
    return TYPE_VOID;
}

DataType term(int *struct_id, int *array_depth) {
    DataType t = factor(struct_id, array_depth);
    while (cur.kind == TK_MUL || cur.kind == TK_DIV || cur.kind == TK_MOD) {
        int op = cur.kind; next(); int d1, d2; DataType t2 = factor(&d1, &d2);
        if (op == TK_MOD) {
            if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) fail("Modulo on float not supported");
            emit(OP_MOD);
        } else {
            if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) { emit(op == TK_MUL ? OP_MUL_F : OP_DIV_F); t = TYPE_FLOAT; }
            else emit(op == TK_MUL ? OP_MUL : OP_DIV);
        }
    }
    return t;
}

DataType add_expr(int *struct_id, int *array_depth) {
    DataType t = term(struct_id, array_depth);
    while (cur.kind == TK_PLUS || cur.kind == TK_MINUS) {
        int op = cur.kind; next(); int d1, d2; DataType t2 = term(&d1, &d2);
        if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) { emit(op == TK_PLUS ? OP_ADD_F : OP_SUB_F); t = TYPE_FLOAT; }
        else emit(op == TK_PLUS ? OP_ADD : OP_SUB);
    }
    return t;
}

DataType rel_expr(int *struct_id, int *array_depth) {
    DataType t = add_expr(struct_id, array_depth);
    if (cur.kind >= TK_EQ && cur.kind <= TK_GE) {
        int op = cur.kind; next(); int d1, d2; DataType t2 = add_expr(&d1, &d2);
        if (t == TYPE_FLOAT || t2 == TYPE_FLOAT) {
            switch(op) { case TK_EQ: emit(OP_EQ_F); break; case TK_NE: emit(OP_NE_F); break; case TK_LT: emit(OP_LT_F); break; case TK_LE: emit(OP_LE_F); break; case TK_GT: emit(OP_GT_F); break; case TK_GE: emit(OP_GE_F); break; }
        } else {
            switch(op) { case TK_EQ: emit(OP_EQ); break; case TK_NE: emit(OP_NE); break; case TK_LT: emit(OP_LT); break; case TK_LE: emit(OP_LE); break; case TK_GT: emit(OP_GT); break; case TK_GE: emit(OP_GE); break; }
        }
        return TYPE_INT;
    }
    return t;
}

DataType expression(int *struct_id, int *array_depth) {
    DataType t = rel_expr(struct_id, array_depth);

    // --- TERNARY OPERATOR ---
    if (accept(TK_QUESTION)) {
        emit(OP_JZ); size_t patch_else = code_sz; emit32(0);

        int d1, d2;
        DataType t_true = expression(&d1, &d2); // Recurse for nested ternaries

        emit(OP_JMP); size_t patch_end = code_sz; emit32(0);

        emit_patch(patch_else, code_sz);
        expect(TK_COLON);

        DataType t_false = expression(&d1, &d2);

        emit_patch(patch_end, code_sz);

        if (t_true != t_false) fail("Ternary operator type mismatch");
        return t_true;
    }
    // ------------------------

    return t;
}

void statement() {
    if (accept(TK_LBRACE)) {
        int saved_locals = local_count;
        while (cur.kind != TK_RBRACE) statement();
        expect(TK_RBRACE);
        int diff = local_count - saved_locals;
        for(int i=0; i<diff; i++) emit(OP_POP);
        local_count = saved_locals;
        return;
    }

    if (accept(TK_TRY)) {
        expect(TK_LBRACE);
        emit(OP_TRY); size_t patch_catch = code_sz; emit32(0);

        int saved_locals = local_count;
        while (cur.kind != TK_RBRACE) statement();
        expect(TK_RBRACE);

        int diff = local_count - saved_locals;
        for(int i=0; i<diff; i++) emit(OP_POP);
        local_count = saved_locals;

        emit(OP_END_TRY);
        emit(OP_JMP); size_t patch_end = code_sz; emit32(0);

        emit_patch(patch_catch, code_sz);
        expect(TK_CATCH); expect(TK_LPAREN);
        if (cur.kind != TK_ID) fail("Expected error variable name");

        locals[local_count].name = strdup(cur.text);
        locals[local_count].type = TYPE_STR;
        locals[local_count].struct_id = -1;
        locals[local_count].array_depth = 0;
        locals[local_count].offset = local_count;
        local_count++;
        next();

        expect(TK_RPAREN); expect(TK_LBRACE);
        while (cur.kind != TK_RBRACE) statement();
        expect(TK_RBRACE);

        emit(OP_POP);
        local_count--;

        emit_patch(patch_end, code_sz);
        return;
    }

    if (accept(TK_THROW)) {
        int d1, d2; DataType t = expression(&d1, &d2);
        if (t != TYPE_STR) fail("Throw expects a string message");
        expect(TK_SEMI);
        emit(OP_THROW);
        return;
    }

    if (accept(TK_EYE)) { expect(TK_LPAREN); expect(TK_RPAREN); expect(TK_SEMI); emit(OP_ABYSS_EYE); return; }

    if (accept(TK_PRINT)) {
        expect(TK_LPAREN);
        int d1, d2;
        DataType t = expression(&d1, &d2);
        if (cur.kind == TK_COMMA) {
            if (t != TYPE_STR) fail("First argument of formatted print must be a string");
            int arg_count = 0;
            while (accept(TK_COMMA)) { expression(&d1, &d2); arg_count++; }
            expect(TK_RPAREN); expect(TK_SEMI);
            emit(OP_PRINT_FMT); emit(arg_count); return;
        }
        expect(TK_RPAREN); expect(TK_SEMI);
        if (t == TYPE_INT) emit(OP_PRINT);
        else if (t == TYPE_FLOAT) emit(OP_PRINT_F);
        else if (t == TYPE_CHAR) emit(OP_PRINT_CHAR);
        else if (t == TYPE_STR) emit(OP_PRINT_STR);
        else fail("Cannot print type %d", t);
        return;
    }

    if (accept(TK_PRINT_CHAR)) {
        expect(TK_LPAREN); int d1, d2; expression(&d1, &d2); expect(TK_RPAREN); expect(TK_SEMI);
        emit(OP_PRINT_CHAR); return;
    }

    if (accept(TK_FREE)) { expect(TK_LPAREN); int d1, d2; expression(&d1, &d2); expect(TK_RPAREN); expect(TK_SEMI); emit(OP_FREE); return; }

    if (accept(TK_RETURN)) {
        if (cur.kind == TK_SEMI) {
            expect(TK_SEMI); emit(OP_CONST_INT); emit32(0); emit(OP_RET); emit(0); return;
        }
        int count = 0;
        do {
            int d1, d2; expression(&d1, &d2); count++;
        } while (accept(TK_COMMA));
        expect(TK_SEMI);
        emit(OP_RET); emit(count);
        return;
    }

    if (accept(TK_WHILE)) {
        expect(TK_LPAREN); size_t start = code_sz; int d1, d2; expression(&d1, &d2); expect(TK_RPAREN);
        emit(OP_JZ); size_t patch = code_sz; emit32(0);
        statement();
        emit(OP_JMP); emit32(start); emit_patch(patch, code_sz); return;
    }

    if (accept(TK_FOR)) {
        expect(TK_LPAREN);
        int saved_locals = local_count;
        statement();
        size_t start_cond = code_sz;
        int d1, d2; expression(&d1, &d2); expect(TK_SEMI);
        emit(OP_JZ); size_t patch_end = code_sz; emit32(0);
        emit(OP_JMP); size_t patch_body = code_sz; emit32(0);
        size_t start_step = code_sz;
        if (cur.kind == TK_ID) {
            char *name = strdup(cur.text); next();
            int lid = find_local(name); int gid = find_global(name);
            if (lid == -1 && gid == -1) fail("Unknown variable in for-loop step");
            if (accept(TK_INC)) {
                if (lid != -1) { emit(OP_GET_LOCAL); emit(locals[lid].offset); emit(OP_CONST_INT); emit32(1); emit(OP_ADD); emit(OP_SET_LOCAL); emit(locals[lid].offset); }
                else { emit(OP_GET_GLOBAL); emit(gid); emit(OP_CONST_INT); emit32(1); emit(OP_ADD); emit(OP_SET_GLOBAL); emit(gid); }
            } else if (accept(TK_DEC)) {
                if (lid != -1) { emit(OP_GET_LOCAL); emit(locals[lid].offset); emit(OP_CONST_INT); emit32(1); emit(OP_SUB); emit(OP_SET_LOCAL); emit(locals[lid].offset); }
                else { emit(OP_GET_GLOBAL); emit(gid); emit(OP_CONST_INT); emit32(1); emit(OP_SUB); emit(OP_SET_GLOBAL); emit(gid); }
            } else if (accept(TK_ASSIGN)) {
                int d1, d2; expression(&d1, &d2);
                if (lid != -1) { emit(OP_SET_LOCAL); emit(locals[lid].offset); } else { emit(OP_SET_GLOBAL); emit(gid); }
            }
            free(name);
        }
        expect(TK_RPAREN);
        emit(OP_JMP); emit32(start_cond);
        emit_patch(patch_body, code_sz);
        statement();
        emit(OP_JMP); emit32(start_step);
        emit_patch(patch_end, code_sz);
        int diff = local_count - saved_locals;
        for(int i=0; i<diff; i++) emit(OP_POP);
        local_count = saved_locals;
        return;
    }

    if (accept(TK_IF)) {
        expect(TK_LPAREN); int d1, d2; expression(&d1, &d2); expect(TK_RPAREN);
        emit(OP_JZ); size_t patch_else = code_sz; emit32(0);
        statement();
        size_t patch_end = 0;
        if (accept(TK_ELSE)) {
            emit(OP_JMP); patch_end = code_sz; emit32(0);
            emit_patch(patch_else, code_sz);
            statement();
            emit_patch(patch_end, code_sz);
        } else {
            emit_patch(patch_else, code_sz);
        }
        return;
    }

    if (cur.kind == TK_INT || cur.kind == TK_FLOAT || cur.kind == TK_CHAR || cur.kind == TK_STR_TYPE || cur.kind == TK_ID) {
        int sid = -1; int ad = 0; DataType type = TYPE_VOID;
        int is_decl = 0;
        if (cur.kind == TK_INT || cur.kind == TK_FLOAT || cur.kind == TK_CHAR || cur.kind == TK_STR_TYPE) is_decl = 1;
        else if (cur.kind == TK_ID) { if (find_struct(cur.text) != -1) is_decl = 1; }
        if (is_decl) {
            parse_type(&type, &sid, &ad);
            if (cur.kind == TK_ID) {
                char *name = strdup(cur.text); next(); expect(TK_ASSIGN); int d1, d2; expression(&d1, &d2); expect(TK_SEMI);
                locals[local_count].name = name; locals[local_count].type = type; locals[local_count].struct_id = sid; locals[local_count].array_depth = ad; locals[local_count].offset = local_count; local_count++; return;
            }
        }
    }
    if (cur.kind == TK_ID) {
        char *name = strdup(cur.text); next();

        if (cur.kind == TK_COMMA) {
            char *names[8];
            names[0] = name;
            int count = 1;
            while (accept(TK_COMMA)) {
                if (cur.kind != TK_ID) fail("Expected variable name in unpacking");
                names[count++] = strdup(cur.text);
                next();
            }
            expect(TK_ASSIGN);

            if (cur.kind != TK_ID) fail("Expected function call for unpacking");
            char *func_name = strdup(cur.text); next();
            expect(TK_LPAREN);
            int fid = find_func(func_name);
            if (fid == -1) fail("Undefined function '%s'", func_name);

            int args = 0;
            if (cur.kind != TK_RPAREN) { do { int d1, d2; expression(&d1, &d2); args++; } while (accept(TK_COMMA)); }
            expect(TK_RPAREN); expect(TK_SEMI);

            if (args != funcs[fid].arg_count) fail("Arg count mismatch");
            if (funcs[fid].ret_count != count) fail("Return count mismatch: expected %d, got %d", count, funcs[fid].ret_count);

            emit(OP_CALL); emit32(funcs[fid].addr); emit(args);

            for (int i = count - 1; i >= 0; i--) {
                int lid = find_local(names[i]);
                int gid = find_global(names[i]);
                if (lid != -1) { emit(OP_SET_LOCAL); emit(locals[lid].offset); }
                else if (gid != -1) { emit(OP_SET_GLOBAL); emit(gid); }
                else fail("Unknown variable '%s'", names[i]);
                free(names[i]);
            }
            free(func_name);
            return;
        }

        if (accept(TK_LPAREN)) {
            int fid = find_func(name);
            if (fid == -1) fail("Undefined function '%s'", name);
            int args = 0;
            if (cur.kind != TK_RPAREN) {
                do { int d1, d2; expression(&d1, &d2); args++; } while (accept(TK_COMMA));
            }
            expect(TK_RPAREN); expect(TK_SEMI);
            if (args != funcs[fid].arg_count) fail("Arg count mismatch for '%s'", name);
            emit(OP_CALL); emit32(funcs[fid].addr); emit(args);
            for(int i=0; i<funcs[fid].ret_count; i++) emit(OP_POP);
            free(name); return;
        }

        int lid = find_local(name); int gid = find_global(name);
        DataType t; int sid; int ad;
        if (lid != -1) { emit(OP_GET_LOCAL); emit(locals[lid].offset); t = locals[lid].type; sid = locals[lid].struct_id; ad = locals[lid].array_depth; }
        else if (gid != -1) { emit(OP_GET_GLOBAL); emit(gid); t = globals[gid].type; sid = globals[gid].struct_id; ad = globals[gid].array_depth; }
        else fail("Undefined variable '%s'", name);

        (void)ad;

        if (accept(TK_INC)) {
            emit(OP_CONST_INT); emit32(1); emit(OP_ADD);
            if (lid != -1) { emit(OP_SET_LOCAL); emit(locals[lid].offset); } else { emit(OP_SET_GLOBAL); emit(gid); }
            expect(TK_SEMI); free(name); return;
        }
        if (accept(TK_DEC)) {
            emit(OP_CONST_INT); emit32(1); emit(OP_SUB);
            if (lid != -1) { emit(OP_SET_LOCAL); emit(locals[lid].offset); } else { emit(OP_SET_GLOBAL); emit(gid); }
            expect(TK_SEMI); free(name); return;
        }
        if (accept(TK_PLUS_ASSIGN)) {
            int d1, d2; expression(&d1, &d2); emit(OP_ADD);
            if (lid != -1) { emit(OP_SET_LOCAL); emit(locals[lid].offset); } else { emit(OP_SET_GLOBAL); emit(gid); }
            expect(TK_SEMI); free(name); return;
        }
        if (accept(TK_MINUS_ASSIGN)) {
            int d1, d2; expression(&d1, &d2); emit(OP_SUB);
            if (lid != -1) { emit(OP_SET_LOCAL); emit(locals[lid].offset); } else { emit(OP_SET_GLOBAL); emit(gid); }
            expect(TK_SEMI); free(name); return;
        }

        while (1) {
            if (accept(TK_DOT)) {
                if (t != TYPE_STRUCT) fail("Dot on non-struct");
                if (sid < 0 || sid >= struct_count) fail("Invalid struct ID %d", sid);
                if (cur.kind != TK_ID) fail("Expected field name");

                int parent_sid = sid;
                int field_idx = -1;
                DataType new_t = TYPE_VOID;
                int new_sid = -1;
                int new_ad = 0;

                for (int i=0; i<structs[parent_sid].field_count; i++) {
                    if (!strcmp(structs[parent_sid].fields[i].name, cur.text)) {
                        field_idx = i;
                        new_t = structs[parent_sid].fields[i].type;
                        new_sid = structs[parent_sid].fields[i].struct_id;
                        new_ad = structs[parent_sid].fields[i].array_depth;
                        break;
                    }
                }
                if (field_idx == -1) fail("Unknown field '%s'", cur.text);

                next();

                /* --- field assignment: supports =, +=, -= --- */
                if (cur.kind == TK_ASSIGN) {
                    next();
                    int d1, d2; expression(&d1, &d2);
                    expect(TK_SEMI);
                    emit(OP_SET_FIELD); emit(structs[parent_sid].fields[field_idx].offset);
                    free(name); return;
                }
                else if (cur.kind == TK_PLUS_ASSIGN) {
                    next();
                    emit(OP_DUP); // Duplicate struct pointer
                    emit(OP_GET_FIELD); emit(structs[parent_sid].fields[field_idx].offset); // Get current value
                    int d1, d2; DataType rhs_t = expression(&d1, &d2); // Get RHS
                    if (new_t == TYPE_FLOAT || rhs_t == TYPE_FLOAT) emit(OP_ADD_F);
                    else emit(OP_ADD);
                    expect(TK_SEMI);
                    emit(OP_SET_FIELD); emit(structs[parent_sid].fields[field_idx].offset);
                    free(name); return;
                }
                else if (cur.kind == TK_MINUS_ASSIGN) {
                    next();
                    emit(OP_DUP); // Duplicate struct pointer
                    emit(OP_GET_FIELD); emit(structs[parent_sid].fields[field_idx].offset); // Get current value
                    int d1, d2; DataType rhs_t = expression(&d1, &d2); // Get RHS
                    if (new_t == TYPE_FLOAT || rhs_t == TYPE_FLOAT) emit(OP_SUB_F);
                    else emit(OP_SUB);
                    expect(TK_SEMI);
                    emit(OP_SET_FIELD); emit(structs[parent_sid].fields[field_idx].offset);
                    free(name); return;
                }

                // FIX: Handle nested struct access (e.g. player.pos.x)
                // If not assignment, we are traversing. Emit GET_FIELD and update type info.
                emit(OP_GET_FIELD); emit(structs[parent_sid].fields[field_idx].offset);
                t = new_t;
                sid = new_sid;
                ad = new_ad;

            } else if (accept(TK_LBRACKET)) {
                int d1, d2; expression(&d1, &d2); expect(TK_RBRACKET);
                size_t p = pos; while(src[p] && isspace(src[p])) p++;

                if (src[p] == '=') {
                    next(); expect(TK_ASSIGN); int d1, d2; expression(&d1, &d2); expect(TK_SEMI);
                    emit(OP_SET_INDEX);
                    free(name); return;
                }
                else if (src[p] == '+' && src[p+1] == '=') {
                    next(); expect(TK_PLUS_ASSIGN); int d1, d2; expression(&d1, &d2);
                    emit(OP_ADD);
                    expect(TK_SEMI);
                    emit(OP_SET_INDEX);
                    free(name); return;
                }
                else if (src[p] == '-' && src[p+1] == '=') {
                    next(); expect(TK_MINUS_ASSIGN); int d1, d2; expression(&d1, &d2);
                    emit(OP_SUB);
                    expect(TK_SEMI);
                    emit(OP_SET_INDEX);
                    free(name); return;
                }


                emit(OP_GET_INDEX);
            } else break;
        }

        if (accept(TK_ASSIGN)) {
             emit(OP_POP);
             int d1, d2; expression(&d1, &d2);
             expect(TK_SEMI);
             if (lid != -1) { emit(OP_SET_LOCAL); emit(locals[lid].offset); }
             else { emit(OP_SET_GLOBAL); emit(gid); }
             free(name); return;
        }

        emit(OP_POP); expect(TK_SEMI); free(name); return;
    }
}

void parse_struct() {
    expect(TK_STRUCT); if (cur.kind != TK_ID) fail("Expected struct name");
    int sid = struct_count; structs[sid].name = strdup(cur.text); struct_count++;
    next(); expect(TK_LBRACE); int offset = 0;
    while (cur.kind != TK_RBRACE) {
        DataType t; int fsid; int ad; parse_type(&t, &fsid, &ad);
        if (cur.kind != TK_ID) fail("Expected field name");
        structs[sid].fields[offset].name = strdup(cur.text); structs[sid].fields[offset].type = t; structs[sid].fields[offset].struct_id = fsid; structs[sid].fields[offset].array_depth = ad; structs[sid].fields[offset].offset = offset; offset++;
        next(); expect(TK_SEMI);
    }
    structs[sid].field_count = offset; structs[sid].size = offset; expect(TK_RBRACE);
}

void parse_func_tail(DataType *ret_types, int *ret_sids, int *ret_ads, int ret_count, char *name) {
    funcs[func_count].name = name;
    funcs[func_count].addr = code_sz;
    funcs[func_count].ret_count = ret_count;
    for(int i=0; i<ret_count; i++) {
        funcs[func_count].ret_types[i] = ret_types[i];
        funcs[func_count].ret_struct_ids[i] = ret_sids[i];
        funcs[func_count].ret_array_depths[i] = ret_ads[i];
    }

    expect(TK_LPAREN); local_count = 0;
    if (cur.kind != TK_RPAREN) {
        do {
            DataType at; int asid; int aad; parse_type(&at, &asid, &aad);
            if (cur.kind != TK_ID) fail("Expected arg name");
            locals[local_count].name = strdup(cur.text); locals[local_count].type = at; locals[local_count].struct_id = asid; locals[local_count].array_depth = aad; locals[local_count].offset = local_count; local_count++; funcs[func_count].arg_count++;
            next();
        } while (accept(TK_COMMA));
    }
    expect(TK_RPAREN);

    func_count++;
    expect(TK_LBRACE);
    while (cur.kind != TK_RBRACE) statement();
    expect(TK_RBRACE);

    if (ret_types[0] == TYPE_VOID && ret_count == 1) {
        emit(OP_CONST_INT); emit32(0);
        emit(OP_RET); emit(1);
    }
}

void parse_func() {
    DataType ret_types[8];
    int ret_sids[8];
    int ret_ads[8];
    int ret_count = 0;

    if (accept(TK_FUNCTION)) {
        if (cur.kind != TK_ID) fail("Expected func name");
        char *name = strdup(cur.text);
        next();

        funcs[func_count].name = name;
        funcs[func_count].addr = code_sz;

        expect(TK_LPAREN); local_count = 0;
        if (cur.kind != TK_RPAREN) {
            do {
                DataType at; int asid; int aad; parse_type(&at, &asid, &aad);
                if (cur.kind != TK_ID) fail("Expected arg name");
                locals[local_count].name = strdup(cur.text); locals[local_count].type = at; locals[local_count].struct_id = asid; locals[local_count].array_depth = aad; locals[local_count].offset = local_count; local_count++; funcs[func_count].arg_count++;
                next();
            } while (accept(TK_COMMA));
        }
        expect(TK_RPAREN);

        if (accept(TK_COLON)) {
            if (accept(TK_LPAREN)) {
                do {
                    DataType t; int sid; int ad; parse_type(&t, &sid, &ad);
                    ret_types[ret_count] = t; ret_sids[ret_count] = sid; ret_ads[ret_count] = ad; ret_count++;
                    if (cur.kind == TK_ID) { // Optional name
                        locals[local_count].name = strdup(cur.text); locals[local_count].type = t; locals[local_count].struct_id = sid; locals[local_count].array_depth = ad; locals[local_count].offset = local_count; local_count++; next();
                    }
                } while (accept(TK_COMMA));
                expect(TK_RPAREN);
            } else {
                DataType t; int sid; int ad; parse_type(&t, &sid, &ad);
                ret_types[ret_count] = t; ret_sids[ret_count] = sid; ret_ads[ret_count] = ad; ret_count++;
            }
        } else {
            ret_types[0] = TYPE_VOID; ret_sids[0] = -1; ret_ads[0] = 0; ret_count = 1;
        }

        funcs[func_count].ret_count = ret_count;
        for(int i=0; i<ret_count; i++) {
            funcs[func_count].ret_types[i] = ret_types[i];
            funcs[func_count].ret_struct_ids[i] = ret_sids[i];
            funcs[func_count].ret_array_depths[i] = ret_ads[i];
        }

        func_count++;
        expect(TK_LBRACE);
        while (cur.kind != TK_RBRACE) statement();
        expect(TK_RBRACE);

        if (ret_types[0] == TYPE_VOID && ret_count == 1) {
            emit(OP_CONST_INT); emit32(0); emit(OP_RET); emit(1);
        }
        return;
    }

    fail("Internal compiler error: parse_func called without 'function' keyword");
}

int main(int argc, char **argv) {
    if (argc < 3) return 1;
    FILE *f = fopen(argv[1], "rb");
    if (!f) fail("Cannot open file: %s", argv[1]);
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET); src = malloc(sz+1); fread(src, 1, sz, f); src[sz] = 0; fclose(f);

    next();

    while (cur.kind != TK_EOF) {
        if (cur.kind == TK_STRUCT) {
            parse_struct();
        } else if (cur.kind == TK_FUNCTION) {
            emit(OP_JMP); size_t patch = code_sz; emit32(0);
            parse_func();
            emit_patch(patch, code_sz);
        } else {
            DataType t; int sid; int ad;
            parse_type(&t, &sid, &ad);

            if (cur.kind != TK_ID) fail("Expected identifier");
            char *name = strdup(cur.text);
            next();

            if (cur.kind == TK_LPAREN) {
                emit(OP_JMP); size_t patch = code_sz; emit32(0);
                DataType ret_types[1] = {t};
                int ret_sids[1] = {sid};
                int ret_ads[1] = {ad};
                parse_func_tail(ret_types, ret_sids, ret_ads, 1, name);
                emit_patch(patch, code_sz);
            } else {
                globals[global_count].name = name;
                globals[global_count].type = t;
                globals[global_count].struct_id = sid;
                globals[global_count].array_depth = ad;
                int gid = global_count++;

                if (cur.kind == TK_ASSIGN) {
                    next();
                    int d1, d2; expression(&d1, &d2);
                    emit(OP_SET_GLOBAL); emit(gid);
                }
                expect(TK_SEMI);
            }
        }
    }

    int main_idx = find_func("main");
    if (main_idx != -1) {
        emit(OP_CALL); emit32(funcs[main_idx].addr); emit(0);
    }
    emit(OP_HALT);

    f = fopen(argv[2], "wb"); fwrite(MAGIC, 1, 7, f); uint8_t ver = VERSION; fwrite(&ver, 1, 1, f);
    fwrite(&str_count, 4, 1, f); for(int i=0; i<str_count; i++) { uint32_t len = strlen(strs[i]); fwrite(&len, 4, 1, f); fwrite(strs[i], 1, len, f); }
    fwrite(&struct_count, 4, 1, f); for(int i=0; i<struct_count; i++) { uint32_t len = strlen(structs[i].name); fwrite(&len, 4, 1, f); fwrite(structs[i].name, 1, len, f); uint32_t size = structs[i].size; fwrite(&size, 4, 1, f); }
    fwrite(&code_sz, 4, 1, f); fwrite(code, 1, code_sz, f); fclose(f);
    printf("Compiled %s -> %s (%zu bytes)\n", argv[1], argv[2], code_sz);
    return 0;
}
