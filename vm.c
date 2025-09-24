#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAGIC "ABYSSBC"
#define VERSION 1
#define MAX_VARS 256
#define BUF_SIZE (1 << 20)

// --- Type Definitions ---
typedef enum {
    CONST_INT = 1,
    CONST_STR = 2
} ConstType;

typedef struct {
    ConstType type;
    int64_t ival;
    char *sval;
} Const;

typedef enum {
    VT_INT = 1,
    VT_STR = 2
} ValType;

typedef struct {
    ValType t;
    int64_t i;
    char *s;
} Value;

// --- Opcodes ---
enum {
    OP_HALT = 0, OP_PUSH_CONST_INT, OP_PUSH_CONST, OP_LOAD_VAR, OP_STORE_VAR,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_LT, OP_LE, OP_GT, OP_GE, OP_EQ, OP_NE,
    OP_JMP, OP_JMP_IF_FALSE, OP_PRINT, OP_PRINT_SHOW_CONST, OP_POP
};

// --- VM State ---
static uint8_t *code = NULL;
static size_t code_size = 0;
static size_t ip = 0;

static Const *consts = NULL;
static uint32_t const_count = 0;

static Value *stack = NULL;
static size_t sp = 0;
static size_t stack_cap = 0;

static int64_t locals[MAX_VARS];

static char *gbuf;
static size_t gpos = 0;

// --- Stack Operations ---
static void push_int(int64_t x) {
    if (sp >= stack_cap) {
        stack_cap = stack_cap ? stack_cap * 2 : 1024;
        stack = realloc(stack, stack_cap * sizeof(Value));
        if (!stack) {
            perror("realloc stack");
            exit(1);
        }
    }
    stack[sp].t = VT_INT;
    stack[sp].i = x;
    stack[sp].s = NULL;
    sp++;
}

static void push_str(char *s) {
    if (sp >= stack_cap) {
        stack_cap = stack_cap ? stack_cap * 2 : 1024;
        stack = realloc(stack, stack_cap * sizeof(Value));
        if (!stack) {
            perror("realloc stack");
            exit(1);
        }
    }
    stack[sp].t = VT_STR;
    stack[sp].s = s;
    stack[sp].i = 0;
    sp++;
}

static Value pop_val(void) {
    if (sp == 0) {
        fprintf(stderr, "VM Error: Stack underflow\n");
        exit(1);
    }
    sp--;
    return stack[sp];
}

// --- Buffered I/O ---
static void flush_buf(void) {
    if (gpos == 0) return;
    size_t total = gpos;
    size_t off = 0;
    while (total > 0) {
        ssize_t w = write(1, gbuf + off, total);
        if (w <= 0) break;
        off += w;
        total -= w;
    }
    gpos = 0;
}

static void print_show_buf(const char *s, size_t len) {
    if (len >= BUF_SIZE) {
        flush_buf();
        write(1, s, len);
        return;
    }
    if (gpos + len > BUF_SIZE) {
        flush_buf();
    }
    memcpy(gbuf + gpos, s, len);
    gpos += len;
}

// --- Bytecode Reader ---
static uint8_t read_u8(void) {
    if (ip >= code_size) {
        fprintf(stderr, "VM Error: Read past end of bytecode (u8)\n");
        exit(1);
    }
    return code[ip++];
}

static uint32_t read_u32(void) {
    if (ip + 4 > code_size) {
        fprintf(stderr, "VM Error: Read past end of bytecode (u32)\n");
        exit(1);
    }
    uint32_t v;
    memcpy(&v, &code[ip], sizeof(uint32_t));
    ip += 4;
    return v;
}

static int32_t read_i32(void) { return (int32_t)read_u32(); }

// --- Execution Loop ---
static void vm_run(void) {
    gbuf = malloc(BUF_SIZE);
    if (!gbuf) {
        perror("malloc gbuf");
        exit(1);
    }

    ip = 0;
    while (ip < code_size) {
        uint8_t op = read_u8();
        switch (op) {
            case OP_HALT:
                goto cleanup;
            case OP_PUSH_CONST_INT: {
                int32_t v = read_i32();
                push_int((int64_t)v);
                break;
            }
            case OP_PUSH_CONST: {
                uint32_t idx = read_u32();
                if (idx >= const_count) {
                    fprintf(stderr, "VM Error: Constant index %u out of bounds\n", idx);
                    exit(1);
                }
                Const *c = &consts[idx];
                if (c->type == CONST_INT) push_int(c->ival);
                else push_str(c->sval);
                break;
            }
            case OP_LOAD_VAR: {
                uint8_t vid = read_u8();
                push_int(locals[vid]);
                break;
            }
            case OP_STORE_VAR: {
                uint8_t vid = read_u8();
                Value v = pop_val();
                if (v.t != VT_INT) {
                    fprintf(stderr, "Type Error: STORE expects an integer\n");
                    exit(1);
                }
                locals[vid] = v.i;
                break;
            }
            case OP_ADD: { Value b = pop_val(); Value a = pop_val(); push_int(a.i + b.i); break; }
            case OP_SUB: { Value b = pop_val(); Value a = pop_val(); push_int(a.i - b.i); break; }
            case OP_MUL: { Value b = pop_val(); Value a = pop_val(); push_int(a.i * b.i); break; }
            case OP_DIV: {
                Value b = pop_val();
                Value a = pop_val();
                if (b.i == 0) {
                    fprintf(stderr, "Runtime Error: Division by zero\n");
                    exit(1);
                }
                push_int(a.i / b.i);
                break;
            }
            case OP_LT: { Value b = pop_val(); Value a = pop_val(); push_int(a.i < b.i ? 1 : 0); break; }
            case OP_LE: { Value b = pop_val(); Value a = pop_val(); push_int(a.i <= b.i ? 1 : 0); break; }
            case OP_GT: { Value b = pop_val(); Value a = pop_val(); push_int(a.i > b.i ? 1 : 0); break; }
            case OP_GE: { Value b = pop_val(); Value a = pop_val(); push_int(a.i >= b.i ? 1 : 0); break; }
            case OP_EQ: { Value b = pop_val(); Value a = pop_val(); push_int(a.i == b.i ? 1 : 0); break; }
            case OP_NE: { Value b = pop_val(); Value a = pop_val(); push_int(a.i != b.i ? 1 : 0); break; }
            case OP_JMP: {
                uint32_t target = read_u32();
                ip = target;
                break;
            }
            case OP_JMP_IF_FALSE: {
                uint32_t target = read_u32();
                Value v = pop_val();
                if (v.t != VT_INT) {
                    fprintf(stderr, "Type Error: Conditional jump expects an integer\n");
                    exit(1);
                }
                if (v.i == 0) ip = target;
                break;
            }
            case OP_PRINT: {
                Value v = pop_val();
                if (v.t != VT_INT) {
                    fprintf(stderr, "Type Error: PRINT expects an integer\n");
                    exit(1);
                }
                char buf[64];
                int n = snprintf(buf, sizeof(buf), "%lld\n", (long long)v.i);
                print_show_buf(buf, n);
                break;
            }
            case OP_PRINT_SHOW_CONST: {
                uint32_t idx = read_u32();
                if (idx >= const_count) {
                    fprintf(stderr, "VM Error: Constant index %u out of bounds\n", idx);
                    exit(1);
                }
                Const *c = &consts[idx];
                if (c->type != CONST_STR) {
                    fprintf(stderr, "Type Error: PRINT_SHOW expects a string constant\n");
                    exit(1);
                }
                size_t len = strlen(c->sval);
                print_show_buf(c->sval, len);
                break;
            }
            case OP_POP: {
                pop_val();
                break;
            }
            default:
                fprintf(stderr, "VM Error: Unknown opcode %u at ip=%zu\n", op, ip - 1);
                goto cleanup;
        }
    }

cleanup:
    flush_buf();
    free(gbuf);
}

// --- Loader & Main ---
static void free_consts(void) {
    if (consts) {
        for (uint32_t i = 0; i < const_count; i++) {
            if (consts[i].type == CONST_STR) free(consts[i].sval);
        }
        free(consts);
        consts = NULL;
    }
}

static int load_bytecode(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("open bytecode file");
        return 0;
    }

    char magic[8];
    if (fread(magic, 1, 7, f) != 7) { fprintf(stderr, "Error: Invalid bytecode header\n"); fclose(f); return 0; }
    magic[7] = '\0';
    if (strcmp(magic, MAGIC) != 0) { fprintf(stderr, "Error: Not an Abyss bytecode file\n"); fclose(f); return 0; }

    uint8_t ver;
    if (fread(&ver, 1, 1, f) != 1) { fclose(f); return 0; }
    if (ver != VERSION) { fprintf(stderr, "Warning: Bytecode version mismatch (file: %u, vm: %u)\n", ver, VERSION); }

    if (fread(&const_count, 4, 1, f) != 1) { fclose(f); return 0; }
    consts = calloc(const_count, sizeof(Const));

    for (uint32_t i = 0; i < const_count; i++) {
        uint8_t t;
        if (fread(&t, 1, 1, f) != 1) { fclose(f); return 0; }
        if (t == CONST_INT) {
            int32_t vi;
            if (fread(&vi, 4, 1, f) != 1) { fclose(f); return 0; }
            consts[i].type = CONST_INT;
            consts[i].ival = vi;
        } else if (t == CONST_STR) {
            uint32_t len;
            if (fread(&len, 4, 1, f) != 1) { fclose(f); return 0; }
            char *s = malloc(len + 1);
            if (fread(s, 1, len, f) != len) { fclose(f); return 0; }
            s[len] = '\0';
            consts[i].type = CONST_STR;
            consts[i].sval = s;
        } else {
            fprintf(stderr, "Error: Unknown constant type %u\n", t);
            fclose(f);
            return 0;
        }
    }

    if (fread(&code_size, 4, 1, f) != 1) { fclose(f); return 0; }
    code = malloc(code_size);
    if (fread(code, 1, code_size, f) != code_size) { fclose(f); return 0; }

    fclose(f);
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program.aby>\n", argv[0]);
        return 1;
    }

    if (!load_bytecode(argv[1])) {
        return 1;
    }

    vm_run();

    free(code);
    free_consts();
    free(stack);

    return 0;
}