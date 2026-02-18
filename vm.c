#define _POSIX_C_SOURCE 200809L
#include "include/bridge.h"
#include "include/common.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// --- CONFIGURATION ---
#define STACK_SIZE (1024 * 1024)
#define CALL_STACK_SIZE 4096
#define EXCEPTION_STACK_SIZE 256

// --- ENABLE ABYSS EYE ---
#define ENABLE_ABYSS_EYE 1

// --- ANSI Colors ---
#define C_RESET "\033[0m"
#define C_BOLD "\033[1m"
#define C_RED "\033[31m"
#define C_GRAY "\033[90m"

// --- MEMORY TRACKING ---
typedef struct AllocInfo {
  void *ptr;
  uint32_t struct_id;
  uint32_t size;
  size_t alloc_fp;
  int is_stack;
  struct AllocInfo *next;
} AllocInfo;

static AllocInfo *alloc_head = NULL;

// --- VM STATE ---
static uint8_t *code;
static size_t code_size;
static char **strs;
static uint32_t str_count;

typedef struct {
  char *name;
  uint32_t size;
} StructMeta;
static StructMeta *structs = NULL;
static uint32_t struct_count = 0;

static int64_t stack[STACK_SIZE];
static int64_t globals[1024];
static size_t sp = 0;
static size_t fp = 0;
static size_t ip = 0;

typedef struct {
  size_t ret_addr;
  size_t old_fp;
} Frame;
static Frame call_stack[CALL_STACK_SIZE];
static size_t csp = 0;

typedef struct {
  size_t catch_addr;
  size_t old_sp;
  size_t old_fp;
  size_t old_csp;
} ExceptionFrame;
static ExceptionFrame exception_stack[EXCEPTION_STACK_SIZE];
static size_t esp = 0;

static inline __attribute__((always_inline)) void push(int64_t v) {
  stack[sp++] = v;
}
static inline __attribute__((always_inline)) int64_t pop() {
  return stack[--sp];
}

// --- MEMORY MANAGEMENT ---
void track_alloc(void *ptr, uint32_t sid, uint32_t size, int is_stack) {
#ifdef ENABLE_ABYSS_EYE
  AllocInfo *node = malloc(sizeof(AllocInfo));
  node->ptr = ptr;
  node->struct_id = sid;
  node->size = size;
  node->alloc_fp = fp;
  node->is_stack = is_stack;
  node->next = alloc_head;
  alloc_head = node;
#else
  (void)ptr;
  (void)sid;
  (void)size;
  (void)is_stack;
#endif
}

void untrack_alloc(void *ptr) {
#ifdef ENABLE_ABYSS_EYE
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
#else
  (void)ptr;
#endif
}

void free_stack_allocs(size_t current_fp) {
#ifdef ENABLE_ABYSS_EYE
  AllocInfo **curr = &alloc_head;
  while (*curr) {
    if ((*curr)->is_stack && (*curr)->alloc_fp >= current_fp) {
      AllocInfo *temp = *curr;
      *curr = (*curr)->next;
      free(temp->ptr);
      free(temp);
    } else {
      curr = &(*curr)->next;
    }
  }
#else
  (void)current_fp;
#endif
}

void abyss_eye() {
#ifdef ENABLE_ABYSS_EYE
  AllocInfo *curr = alloc_head;
  printf("\n" C_GRAY "  ╭" C_BOLD " 👁️  ABYSS EYE " C_RESET C_GRAY
         "──────────────────────────────────────────╮" C_RESET "\n");
  while (curr) {
    printf(C_GRAY "  │ " C_RESET "0x%-16lx" C_GRAY " │ " C_RESET
                  "%-4u bytes" C_GRAY "     │" C_RESET "\n",
           (uintptr_t)curr->ptr, curr->size);
    curr = curr->next;
  }
  printf(C_GRAY
         "  ╰──────────────────────────────────────────────────────╯" C_RESET
         "\n\n");
#endif
}

int main(int argc, char **argv) {
  if (argc < 2)
    return 1;
  FILE *f = fopen(argv[1], "rb");
  if (!f)
    return 1;

  char magic[8];
  fread(magic, 1, 7, f);
  magic[7] = 0;
  if (strcmp(magic, MAGIC) != 0)
    return 1;
  uint8_t ver;
  fread(&ver, 1, 1, f);

  fread(&str_count, 4, 1, f);
  strs = malloc(sizeof(char *) * str_count);
  for (uint32_t i = 0; i < str_count; i++) {
    uint32_t len;
    fread(&len, 4, 1, f);
    strs[i] = malloc(len + 1);
    fread(strs[i], 1, len, f);
    strs[i][len] = 0;
  }

  fread(&struct_count, 4, 1, f);
  structs = malloc(sizeof(StructMeta) * struct_count);
  for (uint32_t i = 0; i < struct_count; i++) {
    uint32_t len;
    fread(&len, 4, 1, f);
    structs[i].name = malloc(len + 1);
    fread(structs[i].name, 1, len, f);
    structs[i].name[len] = 0;
    fread(&structs[i].size, 4, 1, f);
  }

  fread(&code_size, 4, 1, f);
  code = malloc(code_size + 16);
  fread(code, 1, code_size, f);
  fclose(f);

  static void *dispatch_table[] = {[OP_HALT] = &&L_OP_HALT,
                                   [OP_CONST_INT] = &&L_OP_CONST_INT,
                                   [OP_CONST_FLOAT] = &&L_OP_CONST_FLOAT,
                                   [OP_CONST_STR] = &&L_OP_CONST_STR,
                                   [OP_ADD] = &&L_OP_ADD,
                                   [OP_SUB] = &&L_OP_SUB,
                                   [OP_MUL] = &&L_OP_MUL,
                                   [OP_DIV] = &&L_OP_DIV,
                                   [OP_ADD_F] = &&L_OP_ADD_F,
                                   [OP_SUB_F] = &&L_OP_SUB_F,
                                   [OP_MUL_F] = &&L_OP_MUL_F,
                                   [OP_DIV_F] = &&L_OP_DIV_F,
                                   [OP_LT] = &&L_OP_LT,
                                   [OP_LE] = &&L_OP_LE,
                                   [OP_GT] = &&L_OP_GT,
                                   [OP_GE] = &&L_OP_GE,
                                   [OP_EQ] = &&L_OP_EQ,
                                   [OP_NE] = &&L_OP_NE,
                                   [OP_LT_F] = &&L_OP_LT_F,
                                   [OP_LE_F] = &&L_OP_LE_F,
                                   [OP_GT_F] = &&L_OP_GT_F,
                                   [OP_GE_F] = &&L_OP_GE_F,
                                   [OP_EQ_F] = &&L_OP_EQ_F,
                                   [OP_NE_F] = &&L_OP_NE_F,
                                   [OP_JMP] = &&L_OP_JMP,
                                   [OP_JZ] = &&L_OP_JZ,
                                   [OP_PRINT] = &&L_OP_PRINT,
                                   [OP_PRINT_F] = &&L_OP_PRINT_F,
                                   [OP_PRINT_STR] = &&L_OP_PRINT_STR,
                                   [OP_PRINT_CHAR] = &&L_OP_PRINT_CHAR,
                                   [OP_GET_GLOBAL] = &&L_OP_GET_GLOBAL,
                                   [OP_SET_GLOBAL] = &&L_OP_SET_GLOBAL,
                                   [OP_GET_LOCAL] = &&L_OP_GET_LOCAL,
                                   [OP_SET_LOCAL] = &&L_OP_SET_LOCAL,
                                   [OP_CALL] = &&L_OP_CALL,
                                   [OP_RET] = &&L_OP_RET,
                                   [OP_POP] = &&L_OP_POP,
                                   [OP_ALLOC_STRUCT] = &&L_OP_ALLOC_STRUCT,
                                   [OP_ALLOC_ARRAY] = &&L_OP_ALLOC_ARRAY,
                                   [OP_FREE] = &&L_OP_FREE,
                                   [OP_GET_FIELD] = &&L_OP_GET_FIELD,
                                   [OP_SET_FIELD] = &&L_OP_SET_FIELD,
                                   [OP_GET_INDEX] = &&L_OP_GET_INDEX,
                                   [OP_SET_INDEX] = &&L_OP_SET_INDEX,
                                   [OP_ABYSS_EYE] = &&L_OP_ABYSS_EYE,
                                   [OP_MOD] = &&L_OP_MOD,
                                   [OP_NEG] = &&L_OP_NEG,
                                   [OP_NEG_F] = &&L_OP_NEG_F,
                                   [OP_PRINT_FMT] = &&L_OP_PRINT_FMT,
                                   [OP_TRY] = &&L_OP_TRY,
                                   [OP_END_TRY] = &&L_OP_END_TRY,
                                   [OP_THROW] = &&L_OP_THROW,
                                   [OP_NATIVE] = &&L_OP_NATIVE,
                                   [OP_DUP] = &&L_OP_DUP,
                                   [OP_ALLOC_STACK] = &&L_OP_ALLOC_STACK,
                                   [OP_INC_INDEX] = &&L_OP_INC_INDEX,
                                   [OP_DEC_INDEX] = &&L_OP_DEC_INDEX};

#define DISPATCH() goto *dispatch_table[code[ip++]]

  DISPATCH();

L_OP_HALT:
  goto cleanup;

L_OP_CONST_INT: {
  int32_t v;
  memcpy(&v, code + ip, 4);
  ip += 4;
  push(v);
  DISPATCH();
}
L_OP_CONST_FLOAT: {
  int64_t v;
  memcpy(&v, code + ip, 8);
  ip += 8;
  push(v);
  DISPATCH();
}
L_OP_CONST_STR: {
  uint32_t idx;
  memcpy(&idx, code + ip, 4);
  ip += 4;
  push((int64_t)strs[idx]);
  DISPATCH();
}
L_OP_ADD: {
  stack[sp - 2] += stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_SUB: {
  stack[sp - 2] -= stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_MUL: {
  stack[sp - 2] *= stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_DIV: {
  stack[sp - 2] /= stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_MOD: {
  stack[sp - 2] %= stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_NEG: {
  stack[sp - 1] = -stack[sp - 1];
  DISPATCH();
}

L_OP_ADD_F: {
  double a, b;
  memcpy(&b, &stack[--sp], 8);
  memcpy(&a, &stack[sp - 1], 8);
  a += b;
  memcpy(&stack[sp - 1], &a, 8);
  DISPATCH();
}
L_OP_SUB_F: {
  double a, b;
  memcpy(&b, &stack[--sp], 8);
  memcpy(&a, &stack[sp - 1], 8);
  a -= b;
  memcpy(&stack[sp - 1], &a, 8);
  DISPATCH();
}
L_OP_MUL_F: {
  double a, b;
  memcpy(&b, &stack[--sp], 8);
  memcpy(&a, &stack[sp - 1], 8);
  a *= b;
  memcpy(&stack[sp - 1], &a, 8);
  DISPATCH();
}
L_OP_DIV_F: {
  double a, b;
  memcpy(&b, &stack[--sp], 8);
  memcpy(&a, &stack[sp - 1], 8);
  a /= b;
  memcpy(&stack[sp - 1], &a, 8);
  DISPATCH();
}
L_OP_NEG_F: {
  double a;
  memcpy(&a, &stack[sp - 1], 8);
  a = -a;
  memcpy(&stack[sp - 1], &a, 8);
  DISPATCH();
}

L_OP_LT: {
  stack[sp - 2] = (stack[sp - 2] < stack[sp - 1]);
  sp--;
  DISPATCH();
}
L_OP_LE: {
  stack[sp - 2] = (stack[sp - 2] <= stack[sp - 1]);
  sp--;
  DISPATCH();
}
L_OP_GT: {
  stack[sp - 2] = (stack[sp - 2] > stack[sp - 1]);
  sp--;
  DISPATCH();
}
L_OP_GE: {
  stack[sp - 2] = (stack[sp - 2] >= stack[sp - 1]);
  sp--;
  DISPATCH();
}
L_OP_EQ: {
  stack[sp - 2] = (stack[sp - 2] == stack[sp - 1]);
  sp--;
  DISPATCH();
}
L_OP_NE: {
  stack[sp - 2] = (stack[sp - 2] != stack[sp - 1]);
  sp--;
  DISPATCH();
}

L_OP_LT_F: {
  double a, b;
  memcpy(&b, &stack[--sp], 8);
  memcpy(&a, &stack[sp - 1], 8);
  stack[sp - 1] = (a < b);
  DISPATCH();
}
L_OP_LE_F: {
  double a, b;
  memcpy(&b, &stack[--sp], 8);
  memcpy(&a, &stack[sp - 1], 8);
  stack[sp - 1] = (a <= b);
  DISPATCH();
}
L_OP_GT_F: {
  double a, b;
  memcpy(&b, &stack[--sp], 8);
  memcpy(&a, &stack[sp - 1], 8);
  stack[sp - 1] = (a > b);
  DISPATCH();
}
L_OP_GE_F: {
  double a, b;
  memcpy(&b, &stack[--sp], 8);
  memcpy(&a, &stack[sp - 1], 8);
  stack[sp - 1] = (a >= b);
  DISPATCH();
}
L_OP_EQ_F: {
  double a, b;
  memcpy(&b, &stack[--sp], 8);
  memcpy(&a, &stack[sp - 1], 8);
  stack[sp - 1] = (a == b);
  DISPATCH();
}
L_OP_NE_F: {
  double a, b;
  memcpy(&b, &stack[--sp], 8);
  memcpy(&a, &stack[sp - 1], 8);
  stack[sp - 1] = (a != b);
  DISPATCH();
}

L_OP_JMP: {
  uint32_t t;
  memcpy(&t, code + ip, 4);
  ip = t;
  DISPATCH();
}
L_OP_JZ: {
  uint32_t t;
  memcpy(&t, code + ip, 4);
  ip += 4;
  if (!pop())
    ip = t;
  DISPATCH();
}

L_OP_PRINT: {
  printf("%ld\n", pop());
  DISPATCH();
}
L_OP_PRINT_F: {
  double v;
  int64_t iv = pop();
  memcpy(&v, &iv, 8);
  printf("%.6f\n", v);
  DISPATCH();
}
L_OP_PRINT_CHAR: {
  printf("%c", (char)pop());
  DISPATCH();
}
L_OP_PRINT_STR: {
  printf("%s\n", (char *)pop());
  DISPATCH();
}
L_OP_PRINT_FMT: {
  uint8_t argc = code[ip++];
  int64_t fmt_ptr = stack[sp - 1 - argc];
  char *fmt = (char *)fmt_ptr;
  int current_arg = 0;
  for (int i = 0; fmt[i]; i++) {
    if (fmt[i] == '%') {
      i++;
      if (fmt[i] == '{')
        i++;
      char type_buf[32];
      int ti = 0;
      while (fmt[i] && isalpha(fmt[i]) && ti < 31)
        type_buf[ti++] = fmt[i++];
      type_buf[ti] = 0;
      if (fmt[i] == '}')
        i++;
      else
        i--;
      if (current_arg < argc) {
        int64_t val = stack[sp - argc + current_arg];
        if (!strcmp(type_buf, "int") || !strcmp(type_buf, "integer"))
          printf("%ld", val);
        else if (!strcmp(type_buf, "float")) {
          double f;
          memcpy(&f, &val, 8);
          printf("%.6f", f);
        } else if (!strcmp(type_buf, "str") || !strcmp(type_buf, "string"))
          printf("%s", (char *)val);
        else if (!strcmp(type_buf, "char"))
          printf("%c", (char)val);
        current_arg++;
      }
    } else
      putchar(fmt[i]);
  }
  printf("\n");
  sp -= (argc + 1);
  DISPATCH();
}

L_OP_GET_GLOBAL: {
  push(globals[code[ip++]]);
  DISPATCH();
}
L_OP_SET_GLOBAL: {
  globals[code[ip++]] = pop();
  DISPATCH();
}
L_OP_GET_LOCAL: {
  push(stack[fp + code[ip++]]);
  DISPATCH();
}
L_OP_SET_LOCAL: {
  stack[fp + code[ip++]] = pop();
  DISPATCH();
}

L_OP_CALL: {
  uint32_t addr;
  memcpy(&addr, code + ip, 4);
  ip += 4;
  uint8_t argc = code[ip++];
  call_stack[csp].ret_addr = ip;
  call_stack[csp].old_fp = fp;
  csp++;
  fp = sp - argc;
  ip = addr;
  DISPATCH();
}
L_OP_RET: {
  uint8_t count = code[ip++];
  int64_t rets[8];
  for (int i = 0; i < count; i++)
    rets[i] = pop();
  free_stack_allocs(fp);
  if (csp == 0)
    goto cleanup;
  csp--;
  ip = call_stack[csp].ret_addr;
  size_t old_fp = call_stack[csp].old_fp;
  sp = fp;
  fp = old_fp;
  for (int i = count - 1; i >= 0; i--)
    push(rets[i]);
  DISPATCH();
}
L_OP_POP: {
  sp--;
  DISPATCH();
}

L_OP_ALLOC_STRUCT: {
  uint32_t sid;
  memcpy(&sid, code + ip, 4);
  ip += 4;
  uint32_t size = structs[sid].size * 8;
  int64_t *ptr = malloc(size);
  memset(ptr, 0, size);
  track_alloc(ptr, sid, size, 0);
  push((int64_t)ptr);
  DISPATCH();
}
L_OP_ALLOC_ARRAY: {
  uint32_t elem_size;
  memcpy(&elem_size, code + ip, 4);
  ip += 4;
  int64_t count = pop();
  uint32_t total_size = count * 8;
  int64_t *ptr = malloc(total_size);
  memset(ptr, 0, total_size);
  track_alloc(ptr, 0xFFFFFFFF, total_size, 0);
  push((int64_t)ptr);
  DISPATCH();
}
L_OP_FREE: {
  int64_t *ptr = (int64_t *)pop();
  untrack_alloc(ptr);
  free(ptr);
  DISPATCH();
}
L_OP_GET_FIELD: {
  uint8_t offset = code[ip++];
  int64_t *ptr = (int64_t *)pop();
  push(ptr[offset]);
  DISPATCH();
}
L_OP_SET_FIELD: {
  uint8_t offset = code[ip++];
  int64_t val = pop();
  int64_t *ptr = (int64_t *)pop();
  ptr[offset] = val;
  DISPATCH();
}
L_OP_GET_INDEX: {
  int64_t idx = pop();
  int64_t *ptr = (int64_t *)pop();
  push(ptr[idx]);
  DISPATCH();
}
L_OP_SET_INDEX: {
  int64_t val = pop();
  int64_t idx = pop();
  int64_t *ptr = (int64_t *)pop();
  ptr[idx] = val;
  DISPATCH();
}
L_OP_ABYSS_EYE: {
  abyss_eye();
  DISPATCH();
}

L_OP_TRY: {
  uint32_t catch_addr;
  memcpy(&catch_addr, code + ip, 4);
  ip += 4;
  exception_stack[esp++] = (ExceptionFrame){catch_addr, sp, fp, csp};
  DISPATCH();
}
L_OP_END_TRY: {
  if (esp > 0)
    esp--;
  DISPATCH();
}
L_OP_THROW: {
  int64_t err_val = pop();
  if (esp == 0) {
    fprintf(stderr, "Uncaught Exception: %s\n", (char *)err_val);
    exit(1);
  }
  esp--;
  ip = exception_stack[esp].catch_addr;
  sp = exception_stack[esp].old_sp;
  fp = exception_stack[esp].old_fp;
  csp = exception_stack[esp].old_csp;
  push(err_val);
  DISPATCH();
}

L_OP_NATIVE: {
  uint32_t nid;
  memcpy(&nid, code + ip, 4);
  ip += 4;
  if (nid == 0) { // clock
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double t = ts.tv_sec + ts.tv_nsec / 1e9;
    int64_t v;
    memcpy(&v, &t, 8);
    push(v);
  } else if (nid == 1) { // input_int
    char buf[64];
    int64_t v = 0;
    if (fgets(buf, sizeof(buf), stdin))
      v = atoll(buf);
    push(v);
  }
  // --- BRIDGE CALLS ---
  else if (nid == 10) {
    abyss_io_print_str((char *)pop());
  } else if (nid == 11) {
    abyss_io_print_int(pop());
  } else if (nid == 12) {
    int64_t iv = pop();
    double v;
    memcpy(&v, &iv, 8);
    abyss_io_print_float(v); // <--- FIXED NAME
  } else if (nid == 20) {
    int64_t exp = pop();
    int64_t base = pop();
    push(abyss_math_pow(base, exp));
  } else if (nid == 21) {
    push(abyss_math_abs(pop()));
  }

  DISPATCH();
}
L_OP_DUP: {
  push(stack[sp - 1]);
  DISPATCH();
}
L_OP_ALLOC_STACK: {
  uint32_t sid;
  memcpy(&sid, code + ip, 4);
  ip += 4;
  uint32_t size = structs[sid].size * 8;
  int64_t *ptr = malloc(size);
  memset(ptr, 0, size);
  track_alloc(ptr, sid, size, 1);
  push((int64_t)ptr);
  DISPATCH();
}
L_OP_INC_INDEX: {
  int64_t idx = pop();
  int64_t *ptr = (int64_t *)pop();
  ptr[idx]++;
  DISPATCH();
}
L_OP_DEC_INDEX: {
  int64_t idx = pop();
  int64_t *ptr = (int64_t *)pop();
  ptr[idx]--;
  DISPATCH();
}

cleanup:
  free(code);
  for (uint32_t i = 0; i < str_count; i++)
    free(strs[i]);
  free(strs);
  for (uint32_t i = 0; i < struct_count; i++)
    free(structs[i].name);
  free(structs);
  return 0;
}
