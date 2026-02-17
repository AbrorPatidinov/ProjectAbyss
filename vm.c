#define _POSIX_C_SOURCE 200809L
#include "include/common.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)
#define CALL_STACK_SIZE 1024
#define EXCEPTION_STACK_SIZE 256

// --- ANSI Colors ---
#define C_RESET "\033[0m"
#define C_BOLD "\033[1m"
#define C_RED "\033[31m"
#define C_GRAY "\033[90m"

// ... (Keep AllocInfo, StructMeta, globals as before) ...
typedef struct AllocInfo {
  void *ptr;
  uint32_t struct_id;
  uint32_t size;
  size_t alloc_fp;
  int is_stack;
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

static int64_t globals[256];
static size_t ip = 0;

// --- DEBUG HELPER ---
void dump_stack() {
  fprintf(stderr, "\n" C_BOLD "--- STACK DUMP ---" C_RESET "\n");
  for (size_t i = 0; i < sp; i++) {
    fprintf(stderr, "[%zu] %ld (0x%lx)\n", i, stack[i], stack[i]);
  }
  fprintf(stderr, "SP: %zu, FP: %zu\n", sp, fp);
  fprintf(stderr, "------------------\n");
}

static inline void push(int64_t v) {
  if (sp >= STACK_SIZE) {
    fprintf(stderr, C_RED "FATAL: Stack Overflow" C_RESET "\n");
    exit(1);
  }
  stack[sp++] = v;
}
static inline int64_t pop() {
  if (sp == 0) {
    fprintf(stderr, C_RED "FATAL: Stack Underflow" C_RESET "\n");
    exit(1);
  }
  return stack[--sp];
}

static void fast_print_int(int64_t n) { printf("%ld\n", n); }

// ... (Keep abyss_eye, track_alloc, untrack_alloc, free_stack_allocs) ...
void abyss_eye() {
  AllocInfo *curr = alloc_head;
  int count = 0;
  size_t total_mem = 0;
  printf("\n" C_GRAY "  ╭" C_BOLD " 👁️  ABYSS EYE " C_RESET C_GRAY
         "──────────────────────────────────────────╮" C_RESET "\n");
  while (curr) {
    printf(C_GRAY "  │ " C_RESET "0x%-16lx" C_GRAY " │ " C_RESET
                  "%-4u bytes" C_GRAY "     │" C_RESET "\n",
           (uintptr_t)curr->ptr, curr->size);
    total_mem += curr->size;
    count++;
    curr = curr->next;
  }
  printf(C_GRAY
         "  ╰──────────────────────────────────────────────────────╯" C_RESET
         "\n\n");
  fflush(stdout);
}

void track_alloc(void *ptr, uint32_t sid, uint32_t size, int is_stack) {
  AllocInfo *node = malloc(sizeof(AllocInfo));
  node->ptr = ptr;
  node->struct_id = sid;
  node->size = size;
  node->alloc_fp = fp;
  node->is_stack = is_stack;
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
void free_stack_allocs(size_t current_fp) {
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
  code = malloc(code_size);
  fread(code, 1, code_size, f);
  fclose(f);

  while (ip < code_size) {
    uint8_t op = code[ip++];
    switch (op) {
    case OP_HALT:
      goto cleanup;
    case OP_CONST_INT: {
      int32_t v;
      memcpy(&v, code + ip, 4);
      ip += 4;
      push(v);
      break;
    }
    case OP_CONST_FLOAT: {
      int64_t v;
      memcpy(&v, code + ip, 8);
      ip += 8;
      push(v);
      break;
    }
    case OP_CONST_STR: {
      uint32_t idx;
      memcpy(&idx, code + ip, 4);
      ip += 4;
      push((int64_t)strs[idx]);
      break;
    }
    case OP_ADD: {
      int64_t b = pop();
      stack[sp - 1] += b;
      break;
    }
    case OP_SUB: {
      int64_t b = pop();
      stack[sp - 1] -= b;
      break;
    }
    case OP_MUL: {
      int64_t b = pop();
      stack[sp - 1] *= b;
      break;
    }

    case OP_DIV: {
      int64_t b = pop();
      if (b == 0) {
        fprintf(stderr,
                "\n" C_RED "[RUNTIME ERROR]" C_RESET " Division by zero\n");
        exit(1);
      }
      stack[sp - 1] /= b;
      break;
    }

    case OP_MOD: {
      int64_t b = pop();
      if (b == 0) {
        fprintf(stderr, "Div by 0\n");
        exit(1);
      }
      stack[sp - 1] %= b;
      break;
    }
    case OP_NEG: {
      stack[sp - 1] = -stack[sp - 1];
      break;
    }
    case OP_ADD_F: {
      int64_t ib = pop();
      int64_t ia = pop();
      double b, a;
      memcpy(&b, &ib, 8);
      memcpy(&a, &ia, 8);
      double r = a + b;
      int64_t res;
      memcpy(&res, &r, 8);
      push(res);
      break;
    }
    case OP_SUB_F: {
      int64_t ib = pop();
      int64_t ia = pop();
      double b, a;
      memcpy(&b, &ib, 8);
      memcpy(&a, &ia, 8);
      double r = a - b;
      int64_t res;
      memcpy(&res, &r, 8);
      push(res);
      break;
    }
    case OP_MUL_F: {
      int64_t ib = pop();
      int64_t ia = pop();
      double b, a;
      memcpy(&b, &ib, 8);
      memcpy(&a, &ia, 8);
      double r = a * b;
      int64_t res;
      memcpy(&res, &r, 8);
      push(res);
      break;
    }
    case OP_DIV_F: {
      int64_t ib = pop();
      int64_t ia = pop();
      double b, a;
      memcpy(&b, &ib, 8);
      memcpy(&a, &ia, 8);
      double r = a / b;
      int64_t res;
      memcpy(&res, &r, 8);
      push(res);
      break;
    }
    case OP_NEG_F: {
      int64_t ia = pop();
      double a;
      memcpy(&a, &ia, 8);
      a = -a;
      int64_t res;
      memcpy(&res, &a, 8);
      push(res);
      break;
    }
    case OP_LT_F: {
      int64_t ib = pop();
      int64_t ia = pop();
      double b, a;
      memcpy(&b, &ib, 8);
      memcpy(&a, &ia, 8);
      push(a < b);
      break;
    }
    case OP_GT_F: {
      int64_t ib = pop();
      int64_t ia = pop();
      double b, a;
      memcpy(&b, &ib, 8);
      memcpy(&a, &ia, 8);
      push(a > b);
      break;
    }
    case OP_LT: {
      int64_t b = pop();
      stack[sp - 1] = (stack[sp - 1] < b);
      break;
    }
    case OP_GT: {
      int64_t b = pop();
      stack[sp - 1] = (stack[sp - 1] > b);
      break;
    }
    case OP_EQ: {
      int64_t b = pop();
      stack[sp - 1] = (stack[sp - 1] == b);
      break;
    }
    case OP_NE: {
      int64_t b = pop();
      stack[sp - 1] = (stack[sp - 1] != b);
      break;
    }
    case OP_JMP: {
      uint32_t t;
      memcpy(&t, code + ip, 4);
      ip = t;
      break;
    }
    case OP_JZ: {
      uint32_t t;
      memcpy(&t, code + ip, 4);
      ip += 4;
      if (!pop())
        ip = t;
      break;
    }
    case OP_PRINT:
      fast_print_int(pop());
      break;
    case OP_PRINT_F: {
      int64_t iv = pop();
      double v;
      memcpy(&v, &iv, 8);
      printf("%.6f\n", v);
      break;
    }
    case OP_PRINT_CHAR: {
      printf("%c", (char)pop());
      break;
    }
    case OP_PRINT_STR: {
      char *s = (char *)pop();
      printf("%s\n", s);
      break;
    }

    case OP_PRINT_FMT: {
      uint8_t argc = code[ip++];
      if (sp < (size_t)argc + 1) {
        fprintf(stderr, "Stack underflow in PRINT_FMT\n");
        exit(1);
      }
      int64_t fmt_ptr_val = stack[sp - 1 - argc];
      char *fmt = (char *)fmt_ptr_val;
      if (!fmt) {
        fprintf(stderr, "NULL format string\n");
        exit(1);
      }
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
      break;
    }

    case OP_GET_GLOBAL:
      push(globals[code[ip++]]);
      break;
    case OP_SET_GLOBAL:
      globals[code[ip++]] = pop();
      break;
    case OP_GET_LOCAL: {
      uint8_t off = code[ip++];
      push(stack[fp + off]);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t off = code[ip++];
      stack[fp + off] = pop();
      break;
    }
    case OP_CALL: {
      uint32_t addr;
      memcpy(&addr, code + ip, 4);
      ip += 4;
      uint8_t argc = code[ip++];
      call_stack[csp].ret_addr = ip;
      call_stack[csp].old_fp = fp;
      csp++;
      fp = sp - argc;
      ip = addr;
      break;
    }
    case OP_RET: {
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
      break;
    }
    case OP_POP:
      sp--;
      break;
    case OP_ALLOC_STRUCT: {
      uint32_t sid;
      memcpy(&sid, code + ip, 4);
      ip += 4;
      uint32_t size = structs[sid].size * 8;
      int64_t *ptr = malloc(size);
      memset(ptr, 0, size);
      track_alloc(ptr, sid, size, 0);
      push((int64_t)ptr);
      break;
    }
    case OP_ALLOC_ARRAY: {
      uint32_t elem_size;
      memcpy(&elem_size, code + ip, 4);
      ip += 4;
      int64_t count = pop();
      uint32_t total_size = count * 8;
      int64_t *ptr = malloc(total_size);
      memset(ptr, 0, total_size);
      track_alloc(ptr, 0xFFFFFFFF, total_size, 0);
      push((int64_t)ptr);
      break;
    }
    case OP_FREE: {
      int64_t *ptr = (int64_t *)pop();
      untrack_alloc(ptr);
      free(ptr);
      break;
    }
    case OP_GET_FIELD: {
      uint8_t offset = code[ip++];
      int64_t *ptr = (int64_t *)pop();
      push(ptr[offset]);
      break;
    }
    case OP_SET_FIELD: {
      uint8_t offset = code[ip++];
      int64_t val = pop();
      int64_t *ptr = (int64_t *)pop();
      ptr[offset] = val;
      break;
    }
    case OP_GET_INDEX: {
      int64_t idx = pop();
      int64_t *ptr = (int64_t *)pop();
      push(ptr[idx]);
      break;
    }

    case OP_SET_INDEX: {
      int64_t val = pop();
      int64_t idx = pop();
      int64_t *ptr = (int64_t *)pop();

      // --- DEBUG CHECK ---
      if (idx > 1000000 || idx < 0) {
        fprintf(stderr, C_RED "FATAL: Invalid Index %ld\n" C_RESET, idx);
        dump_stack();
        exit(1);
      }

      ptr[idx] = val;
      break;
    }

    case OP_ABYSS_EYE:
      abyss_eye();
      break;

    case OP_TRY: {
      uint32_t catch_addr;
      memcpy(&catch_addr, code + ip, 4);
      ip += 4;
      if (esp >= EXCEPTION_STACK_SIZE) {
        fprintf(stderr, "Exception Stack Overflow\n");
        exit(1);
      }
      exception_stack[esp].catch_addr = catch_addr;
      exception_stack[esp].old_sp = sp;
      exception_stack[esp].old_fp = fp;
      exception_stack[esp].old_csp = csp;
      esp++;
      break;
    }
    case OP_END_TRY: {
      if (esp > 0)
        esp--;
      break;
    }
    case OP_THROW: {
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
      break;
    }
    case OP_NATIVE: {
      uint32_t nid;
      memcpy(&nid, code + ip, 4);
      ip += 4;
      if (nid == 0) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        double t = ts.tv_sec + ts.tv_nsec / 1e9;
        int64_t v;
        memcpy(&v, &t, 8);
        push(v);
      } else if (nid == 1) {
        int64_t v;
        printf("Input: ");
        fflush(stdout);
        char buf[64];
        if (fgets(buf, sizeof(buf), stdin))
          v = atoll(buf);
        else
          v = 0;
        push(v);
      }
      break;
    }
    case OP_DUP: {
      if (sp == 0) {
        fprintf(stderr, "Stack Underflow on DUP\n");
        exit(1);
      }
      int64_t v = stack[sp - 1];
      push(v);
      break;
    }
    case OP_ALLOC_STACK: {
      uint32_t sid;
      memcpy(&sid, code + ip, 4);
      ip += 4;
      uint32_t size = structs[sid].size * 8;
      int64_t *ptr = malloc(size);
      memset(ptr, 0, size);
      track_alloc(ptr, sid, size, 1);
      push((int64_t)ptr);
      break;
    }
    case OP_INC_INDEX: {
      int64_t idx = pop();
      int64_t *ptr = (int64_t *)pop();
      ptr[idx]++;
      break;
    }
    case OP_DEC_INDEX: {
      int64_t idx = pop();
      int64_t *ptr = (int64_t *)pop();
      ptr[idx]--;
      break;
    }
    }
  }
cleanup:
  free(code);
  for (uint32_t i = 0; i < str_count; i++) {
    free(strs[i]);
  }
  free(strs);
  for (uint32_t i = 0; i < struct_count; i++) {
    free(structs[i].name);
  }
  free(structs);
  return 0;
}
