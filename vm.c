#define _POSIX_C_SOURCE 200809L
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
#define C_CYAN "\033[36m"
#define C_GREEN "\033[32m"
#define C_YELLOW "\033[33m"
#define C_MAGENTA "\033[35m"

// --- MEMORY TRACKING ---
typedef struct AllocInfo {
  void *ptr;
  uint32_t struct_id;
  uint32_t size;
  size_t alloc_fp;
  size_t alloc_ip;
  size_t free_ip; // NEW: Tracks where it was freed
  char *tag;
  char *comment;
  int is_stack;
  int is_freed; // NEW: Tracks if it's active or historical
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
void track_alloc(void *ptr, uint32_t sid, uint32_t size, int is_stack,
                 char *comment) {
#ifdef ENABLE_ABYSS_EYE
  AllocInfo *node = malloc(sizeof(AllocInfo));
  node->ptr = ptr;
  node->struct_id = sid;
  node->size = size;
  node->alloc_fp = fp;
  node->alloc_ip = ip;
  node->free_ip = 0;
  node->tag = NULL;
  node->comment = comment;
  node->is_stack = is_stack;
  node->is_freed = 0;
  node->next = alloc_head;
  alloc_head = node;
#else
  (void)ptr;
  (void)sid;
  (void)size;
  (void)is_stack;
  (void)comment;
#endif
}

void untrack_alloc(void *ptr) {
#ifdef ENABLE_ABYSS_EYE
  AllocInfo *curr = alloc_head;
  while (curr) {
    if (curr->ptr == ptr && !curr->is_freed) {
      curr->is_freed = 1;
      curr->free_ip = ip; // Record exactly where it was freed!
      return;
    }
    curr = curr->next;
  }
#else
  (void)ptr;
#endif
}

void free_stack_allocs(size_t current_fp) {
#ifdef ENABLE_ABYSS_EYE
  AllocInfo *curr = alloc_head;
  while (curr) {
    if (curr->is_stack && !curr->is_freed && curr->alloc_fp >= current_fp) {
      curr->is_freed = 1;
      curr->free_ip = ip;
      free(curr->ptr); // Actually free the memory
    }
    curr = curr->next;
  }
#else
  (void)current_fp;
#endif
}

// --- REVOLUTIONARY ABYSS EYE HUD ---
void abyss_eye() {
#ifdef ENABLE_ABYSS_EYE
  AllocInfo *curr = alloc_head;

  uint32_t active_stack = 0;
  uint32_t active_heap = 0;
  uint32_t total_freed = 0;
  uint32_t total_allocs = 0;
  int active_count = 0;

  // Pass 1: Calculate Statistics
  while (curr) {
    total_allocs++;
    if (curr->is_freed) {
      total_freed += curr->size;
    } else {
      active_count++;
      if (curr->is_stack)
        active_stack += curr->size;
      else
        active_heap += curr->size;
    }
    curr = curr->next;
  }

  printf("\n");

  // ==========================================
  // SECTION 1: OVERALL RESOURCES
  // ==========================================
  printf(C_BOLD C_CYAN
         "  "
         "╭────────────────────────────────────────────────────────────────────"
         "──────────────────────────────────────────╮\n" C_RESET);
  printf(C_BOLD C_CYAN
         "  │" C_RESET C_BOLD
         "  📊   S Y S T E M   R E S O U R C E S                               "
         "                                        " C_BOLD C_CYAN
         "│\n" C_RESET);
  printf(C_BOLD C_CYAN
         "  "
         "├────────────────────────────────────────────────────────────────────"
         "──────────────────────────────────────────┤\n" C_RESET);
  printf(C_BOLD C_CYAN "  │" C_RESET "  Active Stack Memory : " C_BOLD C_GREEN
                       "%-8u bytes" C_RESET
                       "                                                       "
                       "                " C_BOLD C_CYAN "│\n" C_RESET,
         active_stack);
  printf(C_BOLD C_CYAN "  │" C_RESET "  Active Heap Memory  : " C_BOLD C_YELLOW
                       "%-8u bytes" C_RESET
                       "                                                       "
                       "                " C_BOLD C_CYAN "│\n" C_RESET,
         active_heap);
  printf(C_BOLD C_CYAN "  │" C_RESET "  Total Memory Freed  : " C_BOLD C_GRAY
                       "%-8u bytes" C_RESET
                       "                                                       "
                       "                " C_BOLD C_CYAN "│\n" C_RESET,
         total_freed);
  printf(C_BOLD C_CYAN "  │" C_RESET "  Total Allocations   : " C_BOLD
                       "%-8u" C_RESET
                       "                                                       "
                       "                      " C_BOLD C_CYAN "│\n" C_RESET,
         total_allocs);
  printf(C_BOLD C_CYAN
         "  "
         "╰────────────────────────────────────────────────────────────────────"
         "──────────────────────────────────────────╯\n\n" C_RESET);

  // ==========================================
  // SECTION 2: ACTIVE MEMORY (HEXDUMP)
  // ==========================================
  printf(C_BOLD C_RED
         "  "
         "╭────────────────────────────────────────────────────────────────────"
         "──────────────────────────────────────────╮\n" C_RESET);
  printf(C_BOLD C_RED
         "  │" C_RESET C_BOLD
         "  👁️   A B Y S S   E Y E   —   A C T I V E   M E M O R Y             "
         "                                      " C_BOLD C_RED "│\n" C_RESET);
  printf(C_BOLD C_RED
         "  "
         "├───────────────────┬───────┬──────────────┬────────────┬───────┬────"
         "─────┬──────────────────────────────────┤\n" C_RESET);
  printf(C_BOLD C_RED
         "  │" C_RESET " ADDRESS           " C_BOLD C_RED "│" C_RESET
         " BYTES " C_BOLD C_RED "│" C_RESET " TYPE         " C_BOLD C_RED
         "│" C_RESET " VARIABLE   " C_BOLD C_RED "│" C_RESET
         " SCOPE " C_BOLD C_RED "│" C_RESET " ORIGIN  " C_BOLD C_RED "│" C_RESET
         " COMMENT / HEXDUMP                " C_BOLD C_RED "│\n" C_RESET);
  printf(C_BOLD C_RED
         "  "
         "├───────────────────┼───────┼──────────────┼────────────┼───────┼────"
         "─────┼──────────────────────────────────┤\n" C_RESET);

  if (active_count == 0) {
    printf(C_BOLD C_RED
           "  │" C_RESET C_GRAY
           "  (The Abyss is empty. No active allocations.)                     "
           "                                           " C_BOLD C_RED
           "│\n" C_RESET);
  }

  curr = alloc_head;
  while (curr) {
    if (!curr->is_freed) {
      const char *icon = curr->is_stack ? "⚡" : "💎";
      const char *type_name = (curr->struct_id == 0xFFFFFFFF) ? "Array"
                              : (curr->struct_id == 0xFFFFFFFE)
                                  ? "DynString"
                                  : structs[curr->struct_id].name;
      const char *tag_name = curr->tag ? curr->tag : "-";
      const char *lifetime = curr->is_stack ? "Stack" : "Heap";
      const char *comment_str = curr->comment ? curr->comment : "-";

      char hex_buf[33];
      memset(hex_buf, ' ', 32);
      hex_buf[32] = '\0';
      uint8_t *bytes = (uint8_t *)curr->ptr;
      int offset = 0;
      for (uint32_t i = 0; i < 8 && i < curr->size; i++) {
        offset += sprintf(hex_buf + offset, "%02X ", bytes[i]);
      }
      if (curr->size > 8) {
        sprintf(hex_buf + offset, "...");
      } else if (offset < 32) {
        hex_buf[offset] = ' ';
      }

      printf(C_BOLD C_RED
             "  │ " C_RESET "%s " C_BOLD C_CYAN "0x%012lx" C_RESET C_BOLD C_RED
             " │ " C_RESET "%-5u" C_BOLD C_RED " │ " C_RESET
             "%-12.12s" C_BOLD C_RED " │ " C_RESET "%-10.10s" C_BOLD C_RED
             " │ " C_RESET "%-5s" C_BOLD C_RED " │ " C_RESET
             "IP:%-4lX" C_BOLD C_RED " │ " C_YELLOW "\"%-30.30s\"" C_BOLD C_RED
             " │\n" C_RESET,
             icon, (uintptr_t)curr->ptr, curr->size, type_name, tag_name,
             lifetime, curr->alloc_ip, comment_str);
      printf(C_BOLD C_RED "  │                   │       │              │      "
                          "      │       │         │ " C_GREEN
                          "%-32.32s" C_BOLD C_RED " │\n" C_RESET,
             hex_buf);

      // Print separator if not the last active item
      AllocInfo *next_active = curr->next;
      while (next_active && next_active->is_freed)
        next_active = next_active->next;
      if (next_active) {
        printf(C_BOLD C_RED
               "  "
               "├───────────────────┼───────┼──────────────┼────────────┼──────"
               "─┼─────────┼──────────────────────────────────┤\n" C_RESET);
      }
    }
    curr = curr->next;
  }
  printf(C_BOLD C_RED
         "  "
         "╰───────────────────┴───────┴──────────────┴────────────┴───────┴────"
         "─────┴──────────────────────────────────╯\n\n" C_RESET);

  // ==========================================
  // SECTION 3: LIFECYCLE & LEAK ANALYSIS
  // ==========================================
  printf(C_BOLD C_MAGENTA
         "  "
         "╭────────────────────────────────────────────────────────────────────"
         "──────────────────────────────────────────╮\n" C_RESET);
  printf(C_BOLD C_MAGENTA
         "  │" C_RESET C_BOLD
         "  🧬   L I F E C Y C L E   &   L E A K   A N A L Y S I S             "
         "                                      " C_BOLD C_MAGENTA
         "│\n" C_RESET);
  printf(C_BOLD C_MAGENTA
         "  "
         "├───────────────────┬───────┬──────────────┬────────────┬───────┬────"
         "───────────┬────────────────────────────┤\n" C_RESET);
  printf(C_BOLD C_MAGENTA
         "  │" C_RESET " ADDRESS           " C_BOLD C_MAGENTA "│" C_RESET
         " BYTES " C_BOLD C_MAGENTA "│" C_RESET
         " TYPE         " C_BOLD C_MAGENTA "│" C_RESET
         " VARIABLE   " C_BOLD C_MAGENTA "│" C_RESET " SCOPE " C_BOLD C_MAGENTA
         "│" C_RESET " STATUS        " C_BOLD C_MAGENTA "│" C_RESET
         " LIFESPAN                   " C_BOLD C_MAGENTA "│\n" C_RESET);
  printf(C_BOLD C_MAGENTA
         "  "
         "├───────────────────┼───────┼──────────────┼────────────┼───────┼────"
         "───────────┼────────────────────────────┤\n" C_RESET);

  curr = alloc_head;
  if (!curr) {
    printf(C_BOLD C_MAGENTA
           "  │" C_RESET C_GRAY
           "  (No allocations tracked.)                                        "
           "                                           " C_BOLD C_MAGENTA
           "│\n" C_RESET);
  }

  while (curr) {
    const char *type_name = (curr->struct_id == 0xFFFFFFFF) ? "Array"
                            : (curr->struct_id == 0xFFFFFFFE)
                                ? "DynString"
                                : structs[curr->struct_id].name;
    const char *tag_name = curr->tag ? curr->tag : "-";
    const char *lifetime = curr->is_stack ? "Stack" : "Heap";

    if (curr->is_freed) {
      printf(C_BOLD C_MAGENTA
             "  │ " C_RESET C_CYAN "0x%012lx" C_RESET C_BOLD C_MAGENTA
             " │ " C_RESET "%-5u" C_BOLD C_MAGENTA " │ " C_RESET
             "%-12.12s" C_BOLD C_MAGENTA " │ " C_RESET
             "%-10.10s" C_BOLD C_MAGENTA " │ " C_RESET "%-5s" C_BOLD C_MAGENTA
             " │ " C_BOLD C_GREEN "[OK] FREED   " C_RESET C_BOLD C_MAGENTA
             " │ " C_RESET "IP:%04lX -> IP:%04lX      " C_BOLD C_MAGENTA
             " │\n" C_RESET,
             (uintptr_t)curr->ptr, curr->size, type_name, tag_name, lifetime,
             curr->alloc_ip, curr->free_ip);
    } else {
      printf(C_BOLD C_MAGENTA
             "  │ " C_RESET C_CYAN "0x%012lx" C_RESET C_BOLD C_MAGENTA
             " │ " C_RESET "%-5u" C_BOLD C_MAGENTA " │ " C_RESET
             "%-12.12s" C_BOLD C_MAGENTA " │ " C_RESET
             "%-10.10s" C_BOLD C_MAGENTA " │ " C_RESET "%-5s" C_BOLD C_MAGENTA
             " │ " C_BOLD C_RED "[!!] ACTIVE  " C_RESET C_BOLD C_MAGENTA
             " │ " C_RESET "IP:%04lX -> " C_YELLOW
             "???             " C_RESET C_BOLD C_MAGENTA " │\n" C_RESET,
             (uintptr_t)curr->ptr, curr->size, type_name, tag_name, lifetime,
             curr->alloc_ip);
    }

    curr = curr->next;
  }
  printf(C_BOLD C_MAGENTA
         "  "
         "╰───────────────────┴───────┴──────────────┴────────────┴───────┴────"
         "───────────┴────────────────────────────╯\n\n" C_RESET);
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

  static void *dispatch_table[] = {
      [OP_HALT] = &&L_OP_HALT,
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
      [OP_DEC_INDEX] = &&L_OP_DEC_INDEX,
      [OP_CALL_DYN_BOT] = &&L_OP_CALL_DYN_BOT,
      [OP_TAG_ALLOC] = &&L_OP_TAG_ALLOC,
      [OP_AND] = &&L_OP_AND,
      [OP_OR] = &&L_OP_OR,
      [OP_NOT] = &&L_OP_NOT,
      [OP_BIT_AND] = &&L_OP_BIT_AND,
      [OP_BIT_OR] = &&L_OP_BIT_OR,
      [OP_BIT_XOR] = &&L_OP_BIT_XOR,
      [OP_SHL] = &&L_OP_SHL,
      [OP_SHR] = &&L_OP_SHR,
      [OP_BIT_NOT] = &&L_OP_BIT_NOT,
      [OP_STR_CAT] = &&L_OP_STR_CAT,
  };

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
  uint32_t sid, cidx;
  memcpy(&sid, code + ip, 4);
  ip += 4;
  memcpy(&cidx, code + ip, 4);
  ip += 4;
  char *comment = (cidx == 0xFFFFFFFF) ? NULL : strs[cidx];

  uint32_t size = structs[sid].size * 8;
  int64_t *ptr = malloc(size);
  memset(ptr, 0, size);
  track_alloc(ptr, sid, size, 0, comment);
  push((int64_t)ptr);
  DISPATCH();
}
L_OP_ALLOC_ARRAY: {
  uint32_t elem_size, cidx;
  memcpy(&elem_size, code + ip, 4);
  ip += 4;
  memcpy(&cidx, code + ip, 4);
  ip += 4;
  char *comment = (cidx == 0xFFFFFFFF) ? NULL : strs[cidx];

  int64_t count = pop();
  uint32_t total_size = count * 8;
  int64_t *ptr = malloc(total_size);
  memset(ptr, 0, total_size);
  track_alloc(ptr, 0xFFFFFFFF, total_size, 0, comment);
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

L_OP_NATIVE:
{
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
  }
  else if (nid == 1) { // input_int
    char buf[64];
    int64_t v = 0;
    if (fgets(buf, sizeof(buf), stdin))
      v = atoll(buf);
    push(v);
  }
  DISPATCH();
}
L_OP_DUP: {
  push(stack[sp - 1]);
  DISPATCH();
}
L_OP_ALLOC_STACK: {
  uint32_t sid, cidx;
  memcpy(&sid, code + ip, 4);
  ip += 4;
  memcpy(&cidx, code + ip, 4);
  ip += 4;
  char *comment = (cidx == 0xFFFFFFFF) ? NULL : strs[cidx];

  uint32_t size = structs[sid].size * 8;
  int64_t *ptr = malloc(size);
  memset(ptr, 0, size);
  track_alloc(ptr, sid, size, 1, comment);
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
L_OP_CALL_DYN_BOT: {
  uint8_t argc = code[ip++];
  uint32_t addr = (uint32_t)stack[sp - argc - 1];

  for (int i = 0; i < argc; i++) {
    stack[sp - argc - 1 + i] = stack[sp - argc + i];
  }
  sp--;

  call_stack[csp].ret_addr = ip;
  call_stack[csp].old_fp = fp;
  csp++;
  fp = sp - argc;
  ip = addr;
  DISPATCH();
}
L_OP_TAG_ALLOC: {
  uint32_t str_idx;
  memcpy(&str_idx, code + ip, 4);
  ip += 4;

  int64_t ptr_val = stack[sp - 1];

  AllocInfo *curr = alloc_head;
  while (curr) {
    if ((int64_t)curr->ptr == ptr_val && !curr->is_freed) {
      curr->tag = strs[str_idx];
      break;
    }
    curr = curr->next;
  }
  DISPATCH();
}
L_OP_AND: {
  stack[sp - 2] = stack[sp - 2] && stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_OR: {
  stack[sp - 2] = stack[sp - 2] || stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_NOT: {
  stack[sp - 1] = !stack[sp - 1];
  DISPATCH();
}
L_OP_BIT_AND: {
  stack[sp - 2] &= stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_BIT_OR: {
  stack[sp - 2] |= stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_BIT_XOR: {
  stack[sp - 2] ^= stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_SHL: {
  stack[sp - 2] <<= stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_SHR: {
  stack[sp - 2] >>= stack[sp - 1];
  sp--;
  DISPATCH();
}
L_OP_BIT_NOT: {
  stack[sp - 1] = ~stack[sp - 1];
  DISPATCH();
}
L_OP_STR_CAT: {
  char *s2 = (char *)pop();
  char *s1 = (char *)pop();
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  char *new_str = malloc(len1 + len2 + 1);
  strcpy(new_str, s1);
  strcat(new_str, s2);

  // 0xFFFFFFFE is our special ID for Dynamic Strings in Abyss Eye
  track_alloc(new_str, 0xFFFFFFFE, len1 + len2 + 1, 0, "Dynamic String Concat");
  push((int64_t)new_str);
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
