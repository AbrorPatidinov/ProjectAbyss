#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAGIC "ABYSSBC"
#define VERSION 7  // Bumped Version
#define STACK_SIZE (1024 * 1024)
#define CALL_STACK_SIZE 1024

// --- ANSI Colors ---
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"
#define C_CYAN    "\033[36m"
#define C_GREEN   "\033[32m"
#define C_MAGENTA "\033[35m"
#define C_YELLOW  "\033[33m"
#define C_RED     "\033[31m"
#define C_GRAY    "\033[90m"

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
    OP_NEG,   // <--- NEW: Integer Negation
    OP_NEG_F, // <--- NEW: Float Negation
    OP_NATIVE
};

// --- Memory Tracking ---
typedef struct AllocInfo {
    void *ptr;
    uint32_t struct_id; // -1 for Array
    uint32_t size;      // Total bytes
    struct AllocInfo *next;
} AllocInfo;

static AllocInfo *alloc_head = NULL;

typedef struct {
    char *name;
    uint32_t size;
} StructMeta;

static StructMeta *structs = NULL;
static uint32_t struct_count = 0;

static uint8_t *code;
static size_t code_size;
static char **strs;
static uint32_t str_count;

static int64_t stack[STACK_SIZE];
static size_t sp = 0;
static size_t fp = 0;

typedef struct { size_t ret_addr; size_t old_fp; } Frame;
static Frame call_stack[CALL_STACK_SIZE];
static size_t csp = 0;

static int64_t globals[256];
static size_t ip = 0;

static inline void push(int64_t v) { stack[sp++] = v; }
static inline int64_t pop() { return stack[--sp]; }

static void fast_print_int(int64_t n) {
    printf("%ld\n", n);
}

void abyss_eye() {
    AllocInfo *curr = alloc_head;
    int count = 0;
    size_t total_mem = 0;

    printf("\n");
    printf(C_GRAY "  ╭" C_RESET C_BOLD C_MAGENTA " 👁️  ABYSS EYE " C_RESET C_GRAY "──────────────────────────────────────────╮" C_RESET "\n");
    printf(C_GRAY "  │ " C_RESET C_BOLD "%-18s" C_RESET C_GRAY " │ " C_RESET C_BOLD "%-14s" C_RESET C_GRAY " │ " C_RESET C_BOLD "%-8s" C_RESET C_GRAY "       │" C_RESET "\n", "ADDRESS", "TYPE", "SIZE");
    printf(C_GRAY "  ├────────────────────┼────────────────┼────────────────┤" C_RESET "\n");

    while (curr) {
        char *type_name = "Array";
        if (curr->struct_id != 0xFFFFFFFF && curr->struct_id < struct_count) {
            type_name = structs[curr->struct_id].name;
        }

        printf(C_GRAY "  │ " C_RESET C_DIM "0x" C_RESET C_CYAN "%-16lx" C_RESET C_GRAY " │ " C_RESET C_GREEN "%-14s" C_RESET C_GRAY " │ " C_RESET C_YELLOW "%-4u" C_RESET " bytes" C_GRAY "     │" C_RESET "\n",
               (uintptr_t)curr->ptr, type_name, curr->size);

        total_mem += curr->size;
        count++;
        curr = curr->next;
    }

    printf(C_GRAY "  ├────────────────────┴────────────────┴────────────────┤" C_RESET "\n");
    printf(C_GRAY "  │ " C_RESET "TOTAL: " C_BOLD "%d" C_RESET " Objects" C_GRAY "                                       │" C_RESET "\n", count);
    printf(C_GRAY "  │ " C_RESET "HEAP:  " C_BOLD "%-4zu" C_RESET " Bytes" C_GRAY "                                         │" C_RESET "\n", total_mem);
    printf(C_GRAY "  ╰──────────────────────────────────────────────────────╯" C_RESET "\n\n");
    fflush(stdout);
}

void track_alloc(void *ptr, uint32_t sid, uint32_t size) {
    AllocInfo *node = malloc(sizeof(AllocInfo));
    node->ptr = ptr;
    node->struct_id = sid;
    node->size = size;
    node->next = alloc_head;
    alloc_head = node;
}

void untrack_alloc(void *ptr) {
    AllocInfo **curr = &alloc_head;
    while (*curr) {
        if ((*curr)->ptr == ptr) {
            AllocInfo *temp = *curr;
            *curr = (*curr)->next;
            free(temp);
            return;
        }
        curr = &(*curr)->next;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;
    char magic[8]; fread(magic, 1, 7, f); magic[7] = 0;
    if (strcmp(magic, MAGIC) != 0) return 1;
    uint8_t ver; fread(&ver, 1, 1, f);
    if (ver != VERSION) { fprintf(stderr, "Version mismatch (VM v%d vs File v%d)\n", VERSION, ver); return 1; }

    fread(&str_count, 4, 1, f);
    strs = malloc(sizeof(char*) * str_count);
    for(uint32_t i=0; i<str_count; i++) {
        uint32_t len; fread(&len, 4, 1, f);
        strs[i] = malloc(len+1); fread(strs[i], 1, len, f); strs[i][len] = 0;
    }

    fread(&struct_count, 4, 1, f);
    structs = malloc(sizeof(StructMeta) * struct_count);
    for(uint32_t i=0; i<struct_count; i++) {
        uint32_t len; fread(&len, 4, 1, f);
        structs[i].name = malloc(len+1); fread(structs[i].name, 1, len, f); structs[i].name[len] = 0;
        fread(&structs[i].size, 4, 1, f);
    }

    fread(&code_size, 4, 1, f);
    code = malloc(code_size); fread(code, 1, code_size, f); fclose(f);

    while (ip < code_size) {
        uint8_t op = code[ip++];
        switch(op) {
            case OP_HALT: goto cleanup;
            case OP_CONST_INT: { int32_t v; memcpy(&v, code+ip, 4); ip+=4; push(v); break; }
            case OP_CONST_FLOAT: { int64_t v; memcpy(&v, code+ip, 8); ip+=8; push(v); break; }
            case OP_CONST_STR: { uint32_t idx; memcpy(&idx, code+ip, 4); ip+=4; push((int64_t)strs[idx]); break; }

            // Integer Math
            case OP_ADD: { int64_t b=pop(); stack[sp-1] += b; break; }
            case OP_SUB: { int64_t b=pop(); stack[sp-1] -= b; break; }
            case OP_MUL: { int64_t b=pop(); stack[sp-1] *= b; break; }
            case OP_DIV: { int64_t b=pop(); stack[sp-1] /= b; break; }
            case OP_MOD: { int64_t b=pop(); if(b==0){fprintf(stderr,"Div by 0\n");exit(1);} stack[sp-1] %= b; break; }
            case OP_NEG: { stack[sp-1] = -stack[sp-1]; break; } // <--- NEW

            // Float Math (Safe Type Punning)
            case OP_ADD_F: {
                int64_t ib = pop(); int64_t ia = pop();
                double b, a; memcpy(&b, &ib, 8); memcpy(&a, &ia, 8);
                double r = a + b;
                int64_t res; memcpy(&res, &r, 8); push(res);
                break;
            }
            case OP_SUB_F: {
                int64_t ib = pop(); int64_t ia = pop();
                double b, a; memcpy(&b, &ib, 8); memcpy(&a, &ia, 8);
                double r = a - b;
                int64_t res; memcpy(&res, &r, 8); push(res);
                break;
            }
            case OP_MUL_F: {
                int64_t ib = pop(); int64_t ia = pop();
                double b, a; memcpy(&b, &ib, 8); memcpy(&a, &ia, 8);
                double r = a * b;
                int64_t res; memcpy(&res, &r, 8); push(res);
                break;
            }
            case OP_DIV_F: {
                int64_t ib = pop(); int64_t ia = pop();
                double b, a; memcpy(&b, &ib, 8); memcpy(&a, &ia, 8);
                double r = a / b;
                int64_t res; memcpy(&res, &r, 8); push(res);
                break;
            }
            case OP_NEG_F: { // <--- NEW
                int64_t ia = pop();
                double a; memcpy(&a, &ia, 8);
                a = -a;
                int64_t res; memcpy(&res, &a, 8); push(res);
                break;
            }

            case OP_LT_F: {
                int64_t ib = pop(); int64_t ia = pop();
                double b, a; memcpy(&b, &ib, 8); memcpy(&a, &ia, 8);
                push(a < b); break;
            }
            case OP_GT_F: {
                int64_t ib = pop(); int64_t ia = pop();
                double b, a; memcpy(&b, &ib, 8); memcpy(&a, &ia, 8);
                push(a > b); break;
            }

            case OP_LT: { int64_t b=pop(); stack[sp-1] = (stack[sp-1] < b); break; }
            case OP_GT: { int64_t b=pop(); stack[sp-1] = (stack[sp-1] > b); break; }
            case OP_EQ: { int64_t b=pop(); stack[sp-1] = (stack[sp-1] == b); break; }
            case OP_JMP: { uint32_t t; memcpy(&t, code+ip, 4); ip = t; break; }
            case OP_JZ:  { uint32_t t; memcpy(&t, code+ip, 4); ip+=4; if(!pop()) ip = t; break; }

            case OP_PRINT: fast_print_int(pop()); break;
            case OP_PRINT_F: {
                int64_t iv = pop();
                double v; memcpy(&v, &iv, 8);
                printf("%.6f\n", v);
                break;
            }
            case OP_PRINT_CHAR: { printf("%c", (char)pop()); break; }
            case OP_PRINT_STR: { char *s = (char*)pop(); printf("%s\n", s); break; }

            case OP_GET_GLOBAL: push(globals[code[ip++]]); break;
            case OP_SET_GLOBAL: globals[code[ip++]] = pop(); break;
            case OP_GET_LOCAL: { uint8_t off = code[ip++]; push(stack[fp + off]); break; }
            case OP_SET_LOCAL: { uint8_t off = code[ip++]; stack[fp + off] = pop(); break; }

            case OP_CALL: {
                uint32_t addr; memcpy(&addr, code+ip, 4); ip+=4;
                uint8_t argc = code[ip++];
                call_stack[csp].ret_addr = ip;
                call_stack[csp].old_fp = fp;
                csp++;
                fp = sp - argc;
                ip = addr;
                break;
            }
            case OP_RET: {
                if (csp == 0) goto cleanup;
                int64_t ret_val = pop();
                csp--;
                ip = call_stack[csp].ret_addr;
                sp = fp;
                fp = call_stack[csp].old_fp;
                push(ret_val);
                break;
            }
            case OP_POP: sp--; break;

            case OP_ALLOC_STRUCT: {
                uint32_t sid; memcpy(&sid, code+ip, 4); ip+=4;
                uint32_t size = structs[sid].size * 8;
                int64_t *ptr = malloc(size);
                memset(ptr, 0, size);
                track_alloc(ptr, sid, size);
                push((int64_t)ptr);
                break;
            }
            case OP_ALLOC_ARRAY: {
                uint32_t elem_size; memcpy(&elem_size, code+ip, 4); ip+=4;
                int64_t count = pop();
                uint32_t total_size = count * 8; // Force 8 byte alignment for now
                int64_t *ptr = malloc(total_size);
                memset(ptr, 0, total_size);
                track_alloc(ptr, 0xFFFFFFFF, total_size);
                push((int64_t)ptr);
                break;
            }
            case OP_FREE: {
                int64_t *ptr = (int64_t*)pop();
                untrack_alloc(ptr);
                free(ptr);
                break;
            }
            case OP_GET_FIELD: {
                uint8_t offset = code[ip++];
                int64_t *ptr = (int64_t*)pop();
                push(ptr[offset]);
                break;
            }
            case OP_SET_FIELD: {
                uint8_t offset = code[ip++];
                int64_t val = pop();
                int64_t *ptr = (int64_t*)pop();
                ptr[offset] = val;
                break;
            }
            case OP_GET_INDEX: {
                int64_t idx = pop();
                int64_t *ptr = (int64_t*)pop();
                push(ptr[idx]);
                break;
            }
            case OP_SET_INDEX: {
                int64_t val = pop();
                int64_t idx = pop();
                int64_t *ptr = (int64_t*)pop();
                ptr[idx] = val;
                break;
            }
            case OP_ABYSS_EYE: abyss_eye(); break;

            case OP_NATIVE: {
                uint32_t nid; memcpy(&nid, code+ip, 4); ip+=4;
                if (nid == 0) { // clock
                    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
                    double t = ts.tv_sec + ts.tv_nsec / 1e9;
                    int64_t v; memcpy(&v, &t, 8); push(v);
                } else if (nid == 1) { // input_int
                    int64_t v;
                    printf("Input: ");
                    fflush(stdout); // Explicit flush needed for input
                    if (scanf("%ld", &v) != 1) {
                        int c; while ((c = getchar()) != '\n' && c != EOF);
                        v = 0;
                    }
                    push(v);
                }
                break;
            }
        }
    }
cleanup:
    free(code);
    for(uint32_t i=0; i<str_count; i++) free(strs[i]);
    free(strs);
    for(uint32_t i=0; i<struct_count; i++) free(structs[i].name);
    free(structs);
    return 0;
}
