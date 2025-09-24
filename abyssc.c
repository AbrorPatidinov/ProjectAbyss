#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARS 256
#define MAGIC "ABYSSBC"
#define VERSION 1

// --- Type Definitions (mirrored from VM) ---
typedef enum {
    CONST_INT = 1,
    CONST_STR = 2
} ConstType;

typedef struct {
    ConstType type;
    int64_t ival;
    char *sval;
} Const;

// Opcodes (must match vm.c)
enum {
    OP_HALT = 0, OP_PUSH_CONST_INT, OP_PUSH_CONST, OP_LOAD_VAR, OP_STORE_VAR,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_LT, OP_LE, OP_GT, OP_GE, OP_EQ, OP_NE,
    OP_JMP, OP_JMP_IF_FALSE, OP_PRINT, OP_PRINT_SHOW_CONST, OP_POP
};

// --- Tokenizer ---
typedef enum {
    TK_EOF, TK_IDENT, TK_NUMBER, TK_STRING, TK_SEMI, TK_LPAREN, TK_RPAREN,
    TK_LBRACE, TK_RBRACE, TK_PLUS, TK_MINUS, TK_MUL, TK_DIV, TK_ASSIGN,
    TK_LT, TK_GT, TK_BANG, TK_EQEQ, TK_NOTEQ, TK_LE, TK_GE, TK_UNKNOWN
} TK;

typedef struct {
    TK kind;
    char *text;
    int64_t num;
} Tok;

static char *src_text = NULL;
static size_t src_len = 0;
static size_t ppos = 0;
static Tok cur = {TK_EOF, NULL, 0};

// --- Compiler State ---
static Const *cpool = NULL;
static uint32_t cpool_count = 0;
static uint32_t cpool_cap = 0;

static uint8_t *out_code = NULL;
static size_t out_cap = 0;
static size_t out_sz = 0;

typedef struct VMap {
    char *name;
    uint8_t idx;
    struct VMap *next;
} VMap;
static VMap *vars = NULL;
static uint8_t var_count = 0;

// --- Forward Declarations ---
static void compile_stmt_list(void);
static void compile_stmt(void);
static void compile_expr(void);
static void compile_relational(void);
static void tk_next(void);

// --- Utility Functions ---
static char *xstrndup(const char *s, size_t n) {
    char *r = malloc(n + 1);
    if (!r) {
        perror("malloc");
        exit(1);
    }
    memcpy(r, s, n);
    r[n] = '\0';
    return r;
}

// --- Tokenizer Implementation ---
static char peekc(void) { return ppos < src_len ? src_text[ppos] : '\0'; }
static char nextc(void) { return ppos < src_len ? src_text[ppos++] : '\0'; }
static void tk_free(Tok *t) {
    if (t->text) free(t->text);
    t->text = NULL;
}

static void skipws(void) {
    for (;;) {
        while (isspace((unsigned char)peekc())) nextc();
        if (peekc() == '/' && ppos + 1 < src_len && src_text[ppos + 1] == '/') {
            while (peekc() && peekc() != '\n') nextc();
            continue;
        }
        break;
    }
}

static void tk_next(void) {
    tk_free(&cur);
    skipws();
    char c = peekc();
    if (!c) {
        cur.kind = TK_EOF;
        cur.text = NULL;
        return;
    }

    if (isalpha((unsigned char)c) || c == '_') {
        size_t st = ppos;
        while (isalnum((unsigned char)peekc()) || peekc() == '_') nextc();
        cur.text = xstrndup(src_text + st, ppos - st);
        cur.kind = TK_IDENT;
        return;
    }

    if (isdigit((unsigned char)c)) {
        int64_t v = 0;
        size_t st = ppos;
        while (isdigit((unsigned char)peekc())) {
            v = v * 10 + (nextc() - '0');
        }
        cur.kind = TK_NUMBER;
        cur.num = v;
        cur.text = xstrndup(src_text + st, ppos - st);
        return;
    }

    if (c == '"') {
        nextc();
        size_t st = ppos;
        while (peekc() && peekc() != '"') {
            if (peekc() == '\\') {
                nextc();
                if (peekc()) nextc();
            } else {
                nextc();
            }
        }
        size_t len = ppos - st;
        cur.text = xstrndup(src_text + st, len);
        cur.kind = TK_STRING;
        if (peekc() == '"') nextc();
        return;
    }

    if (peekc() == '=' && ppos + 1 < src_len && src_text[ppos + 1] == '=') { nextc(); nextc(); cur.kind = TK_EQEQ; cur.text = strdup("=="); return; }
    if (peekc() == '!' && ppos + 1 < src_len && src_text[ppos + 1] == '=') { nextc(); nextc(); cur.kind = TK_NOTEQ; cur.text = strdup("!="); return; }
    if (peekc() == '<' && ppos + 1 < src_len && src_text[ppos + 1] == '=') { nextc(); nextc(); cur.kind = TK_LE; cur.text = strdup("<="); return; }
    if (peekc() == '>' && ppos + 1 < src_len && src_text[ppos + 1] == '=') { nextc(); nextc(); cur.kind = TK_GE; cur.text = strdup(">="); return; }

    char ch = nextc();
    switch (ch) {
        case ';': cur.kind = TK_SEMI; return;
        case '(': cur.kind = TK_LPAREN; return;
        case ')': cur.kind = TK_RPAREN; return;
        case '{': cur.kind = TK_LBRACE; return;
        case '}': cur.kind = TK_RBRACE; return;
        case '+': cur.kind = TK_PLUS; return;
        case '-': cur.kind = TK_MINUS; return;
        case '*': cur.kind = TK_MUL; return;
        case '/': cur.kind = TK_DIV; return;
        case '=': cur.kind = TK_ASSIGN; return;
        case '<': cur.kind = TK_LT; return;
        case '>': cur.kind = TK_GT; return;
        case '!': cur.kind = TK_BANG; return;
        default: cur.kind = TK_UNKNOWN; return;
    }
}

// --- Parser ---
static int accept_tk(TK k) {
    if (cur.kind == k) {
        tk_next();
        return 1;
    }
    return 0;
}

static void expect_tk(TK k, const char *msg) {
    if (!accept_tk(k)) {
        fprintf(stderr, "Parse error: expected %s\n", msg);
        exit(1);
    }
}

// --- Code Generation ---
static void emit_u8(uint8_t x) {
    if (out_sz + 1 > out_cap) {
        out_cap = out_cap ? out_cap * 2 : 1024;
        out_code = realloc(out_code, out_cap);
        if (!out_code) { perror("realloc"); exit(1); }
    }
    out_code[out_sz++] = x;
}

static void emit_u32(uint32_t v) {
    emit_u8(v & 0xFF);
    emit_u8((v >> 8) & 0xFF);
    emit_u8((v >> 16) & 0xFF);
    emit_u8((v >> 24) & 0xFF);
}

static void emit_i32(int32_t v) { emit_u32((uint32_t)v); }

// --- Constant Pool & Variable Management ---
static uint32_t add_const_string(const char *s) {
    if (cpool_count + 1 > cpool_cap) {
        cpool_cap = cpool_cap ? cpool_cap * 2 : 8;
        cpool = realloc(cpool, cpool_cap * sizeof(Const));
        if (!cpool) { perror("realloc"); exit(1); }
    }
    uint32_t idx = cpool_count++;
    cpool[idx].type = CONST_STR;
    cpool[idx].ival = 0;
    cpool[idx].sval = strdup(s);
    if (!cpool[idx].sval) { perror("strdup"); exit(1); }
    return idx;
}

static int var_index(const char *name) {
    for (VMap *v = vars; v; v = v->next) {
        if (strcmp(v->name, name) == 0) return v->idx;
    }
    return -1;
}

static int ensure_var(const char *name) {
    int idx = var_index(name);
    if (idx >= 0) return idx;
    if (var_count >= MAX_VARS) {
        fprintf(stderr, "Error: Too many variables (limit: %d)\n", MAX_VARS);
        exit(1);
    }
    VMap *n = malloc(sizeof(VMap));
    n->name = strdup(name);
    n->idx = var_count;
    n->next = vars;
    vars = n;
    return var_count++;
}

// --- Recursive Descent Parser ---
static void compile_factor(void) {
    if (cur.kind == TK_NUMBER) {
        int32_t v = (int32_t)cur.num;
        emit_u8(OP_PUSH_CONST_INT);
        emit_i32(v);
        tk_next();
        return;
    }
    if (cur.kind == TK_IDENT) {
        int idx = var_index(cur.text);
        if (idx < 0) {
            fprintf(stderr, "Error: undefined variable '%s'\n", cur.text);
            exit(1);
        }
        emit_u8(OP_LOAD_VAR);
        emit_u8((uint8_t)idx);
        tk_next();
        return;
    }
    if (accept_tk(TK_LPAREN)) {
        compile_relational();
        expect_tk(TK_RPAREN, ")");
        return;
    }
    fprintf(stderr, "Parse error: unexpected token in expression factor\n");
    exit(1);
}

static void compile_term(void) {
    compile_factor();
    while (cur.kind == TK_MUL || cur.kind == TK_DIV) {
        TK op = cur.kind;
        tk_next();
        compile_factor();
        if (op == TK_MUL) emit_u8(OP_MUL);
        else emit_u8(OP_DIV);
    }
}

static void compile_expr(void) {
    compile_term();
    while (cur.kind == TK_PLUS || cur.kind == TK_MINUS) {
        TK op = cur.kind;
        tk_next();
        compile_term();
        if (op == TK_PLUS) emit_u8(OP_ADD);
        else emit_u8(OP_SUB);
    }
}

static void compile_relational(void) {
    compile_expr();
    while (cur.kind == TK_LT || cur.kind == TK_LE || cur.kind == TK_GT || cur.kind == TK_GE || cur.kind == TK_EQEQ || cur.kind == TK_NOTEQ) {
        TK op = cur.kind;
        tk_next();
        compile_expr();
        if (op == TK_LT) emit_u8(OP_LT);
        else if (op == TK_LE) emit_u8(OP_LE);
        else if (op == TK_GT) emit_u8(OP_GT);
        else if (op == TK_GE) emit_u8(OP_GE);
        else if (op == TK_EQEQ) emit_u8(OP_EQ);
        else if (op == TK_NOTEQ) emit_u8(OP_NE);
    }
}

static void compile_stmt(void) {
    if (cur.kind == TK_IDENT) {
        if (strcmp(cur.text, "print_show") == 0) {
            tk_next();
            expect_tk(TK_LPAREN, "(");
            if (cur.kind != TK_STRING) {
                fprintf(stderr, "Error: print_show expects a string literal\n");
                exit(1);
            }
            uint32_t ci = add_const_string(cur.text);
            tk_next();
            expect_tk(TK_RPAREN, ")");
            expect_tk(TK_SEMI, ";");
            emit_u8(OP_PRINT_SHOW_CONST);
            emit_u32(ci);
            return;
        }
        if (strcmp(cur.text, "print") == 0) {
            tk_next();
            expect_tk(TK_LPAREN, "(");
            compile_relational();
            expect_tk(TK_RPAREN, ")");
            expect_tk(TK_SEMI, ";");
            emit_u8(OP_PRINT);
            return;
        }
        if (strcmp(cur.text, "while") == 0) {
            tk_next();
            expect_tk(TK_LPAREN, "(");
            size_t cond_pos = out_sz;
            compile_relational();
            expect_tk(TK_RPAREN, ")");
            expect_tk(TK_LBRACE, "{");

            emit_u8(OP_JMP_IF_FALSE);
            size_t patch_loc = out_sz;
            emit_u32(0); // Placeholder for jump address

            while (cur.kind != TK_RBRACE && cur.kind != TK_EOF) {
                compile_stmt();
            }
            expect_tk(TK_RBRACE, "}");

            emit_u8(OP_JMP);
            emit_u32((uint32_t)cond_pos);

            uint32_t after_body_pos = (uint32_t)out_sz;
            // Patch the placeholder
            out_code[patch_loc] = (after_body_pos) & 0xFF;
            out_code[patch_loc + 1] = (after_body_pos >> 8) & 0xFF;
            out_code[patch_loc + 2] = (after_body_pos >> 16) & 0xFF;
            out_code[patch_loc + 3] = (after_body_pos >> 24) & 0xFF;
            return;
        }

        // Default to assignment
        char *name = strdup(cur.text);
        tk_next();
        expect_tk(TK_ASSIGN, "=");
        compile_relational();
        expect_tk(TK_SEMI, ";");
        int idx = ensure_var(name);
        emit_u8(OP_STORE_VAR);
        emit_u8((uint8_t)idx);
        free(name);
        return;
    }

    if (cur.kind == TK_EOF) return;
    fprintf(stderr, "Parse error: unexpected token at statement start\n");
    exit(1);
}

static void compile_stmt_list(void) {
    while (cur.kind != TK_EOF) {
        compile_stmt();
    }
}

// --- File I/O ---
static int write_aby(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("open output file");
        return 0;
    }

    fwrite(MAGIC, 1, 7, f);
    uint8_t ver = VERSION;
    fwrite(&ver, 1, 1, f);

    uint32_t cc = cpool_count;
    fwrite(&cc, 4, 1, f);
    for (uint32_t i = 0; i < cc; i++) {
        if (cpool[i].type == CONST_INT) {
            uint8_t t = CONST_INT;
            fwrite(&t, 1, 1, f);
            int32_t vi = (int32_t)cpool[i].ival;
            fwrite(&vi, 4, 1, f);
        } else {
            uint8_t t = CONST_STR;
            fwrite(&t, 1, 1, f);
            uint32_t len = (uint32_t)strlen(cpool[i].sval);
            fwrite(&len, 4, 1, f);
            fwrite(cpool[i].sval, 1, len, f);
        }
    }

    uint32_t cs = (uint32_t)out_sz;
    fwrite(&cs, 4, 1, f);
    fwrite(out_code, 1, out_sz, f);

    fclose(f);
    return 1;
}

static int compile_file_to_aby(const char *inpath, const char *outpath) {
    FILE *f = fopen(inpath, "rb");
    if (!f) {
        perror("open source file");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    src_text = malloc(sz + 1);
    if (!src_text) {
        perror("malloc source buffer");
        fclose(f);
        return 0;
    }
    if (fread(src_text, 1, sz, f) != (size_t)sz) {
        perror("fread source file");
        fclose(f);
        free(src_text);
        return 0;
    }
    src_text[sz] = '\0';
    src_len = sz;
    fclose(f);

    ppos = 0;
    tk_next();
    compile_stmt_list();
    emit_u8(OP_HALT);

    return write_aby(outpath);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.al> <output.aby>\n", argv[0]);
        return 1;
    }

    const char *inpath = argv[1];
    const char *outpath = argv[2];

    if (!compile_file_to_aby(inpath, outpath)) {
        fprintf(stderr, "Error: Failed to write %s\n", outpath);
        return 1;
    }

    printf("Wrote: %s (consts=%u code=%zu bytes)\n", outpath, (unsigned)cpool_count, out_sz);

    // --- Cleanup ---
    free(src_text);
    free(out_code);
    for (uint32_t i = 0; i < cpool_count; i++) { if (cpool[i].sval) free(cpool[i].sval); }
    free(cpool);
    VMap *v = vars;
    while(v) { VMap *next = v->next; free(v->name); free(v); v = next; }
    tk_free(&cur);

    return 0;
}