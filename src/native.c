#include "../include/native.h"
#include "../include/codegen.h"
#include "../include/common.h"
#include "../include/symbols.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void generate_native_code(const char *out_filename) {
  FILE *f = fopen(out_filename, "w");
  if (!f) {
    fprintf(stderr, "Could not open %s for writing.\n", out_filename);
    exit(1);
  }

  // --- 1. C HEADERS & VM STATE ---
  fprintf(f, "#define _POSIX_C_SOURCE 200809L\n");
  fprintf(f, "#include \"include/bridge.h\"\n");
  fprintf(
      f,
      "#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include "
      "<string.h>\n#include <time.h>\n#include <ctype.h>\n\n");

  fprintf(f,
          "#define C_RESET \"\\033[0m\"\n#define C_BOLD \"\\033[1m\"\n#define "
          "C_RED \"\\033[31m\"\n#define C_GRAY \"\\033[90m\"\n#define C_CYAN "
          "\"\\033[36m\"\n#define C_GREEN \"\\033[32m\"\n#define C_YELLOW "
          "\"\\033[33m\"\n#define C_MAGENTA \"\\033[35m\"\n\n");

  // --- NEW: THE VALUE UNION (Eliminates memcpy overhead!) ---
  fprintf(f, "typedef union { int64_t i; double f; void *p; } Value;\n");
  fprintf(f, "Value stack[1024 * 1024];\n");
  fprintf(f, "Value globals[1024];\n\n");

  fprintf(f, "typedef struct { size_t ret_addr; size_t old_fp; } Frame;\n");
  fprintf(f, "Frame call_stack[4096]; size_t csp = 0;\n");
  fprintf(f, "typedef struct { size_t catch_addr; size_t old_sp; size_t "
             "old_fp; size_t old_csp; } ExceptionFrame;\n");
  fprintf(f, "ExceptionFrame exception_stack[256]; size_t esp = 0;\n\n");

  // --- 2. STRINGS & STRUCTS ---
  fprintf(f, "char *strs[%d] = {\n", str_count == 0 ? 1 : str_count);
  for (int i = 0; i < str_count; i++) {
    fprintf(f, "  \"");
    for (int j = 0; strs[i][j]; j++) {
      if (strs[i][j] == '\n')
        fprintf(f, "\\n");
      else if (strs[i][j] == '"')
        fprintf(f, "\\\"");
      else if (strs[i][j] == '\\')
        fprintf(f, "\\\\");
      else
        fprintf(f, "%c", strs[i][j]);
    }
    fprintf(f, "\",\n");
  }
  fprintf(f, "};\n\n");

  fprintf(f, "typedef struct { char *name; uint32_t size; } StructMeta;\n");
  fprintf(f, "StructMeta structs[%d] = {\n",
          struct_count == 0 ? 1 : struct_count);
  for (int i = 0; i < struct_count; i++) {
    fprintf(f, "  {\"%s\", %d},\n", structs[i].name, structs[i].size);
  }
  fprintf(f, "};\n\n");

  // --- 3. ABYSS EYE TRACKER (Embedded) ---
  fprintf(f, "typedef struct AllocInfo { void *ptr; uint32_t struct_id; "
             "uint32_t size; size_t alloc_fp; size_t alloc_ip; size_t free_ip; "
             "char *tag; char *comment; int is_stack; int is_freed; struct "
             "AllocInfo *next; } AllocInfo;\n");
  fprintf(f, "AllocInfo *alloc_head = NULL;\n");
  fprintf(f, "void track_alloc(void *ptr, uint32_t sid, uint32_t size, int "
             "is_stack, char *comment, size_t ip, size_t fp) {\n");
  fprintf(f,
          "  AllocInfo *node = malloc(sizeof(AllocInfo)); node->ptr = ptr; "
          "node->struct_id = sid; node->size = size; node->alloc_fp = fp; "
          "node->alloc_ip = ip; node->free_ip = 0; node->tag = NULL; "
          "node->comment = comment; node->is_stack = is_stack; node->is_freed "
          "= 0; node->next = alloc_head; alloc_head = node;\n}\n");
  fprintf(f, "void untrack_alloc(void *ptr, size_t ip) {\n  AllocInfo *curr = "
             "alloc_head; while(curr) { if(curr->ptr == ptr && "
             "!curr->is_freed) { curr->is_freed = 1; curr->free_ip = ip; "
             "return; } curr = curr->next; }\n}\n");
  fprintf(
      f, "void free_stack_allocs(size_t current_fp, size_t ip) {\n  AllocInfo "
         "*curr = alloc_head; while(curr) { if(curr->is_stack && "
         "!curr->is_freed && curr->alloc_fp >= current_fp) { curr->is_freed = "
         "1; curr->free_ip = ip; free(curr->ptr); } curr = curr->next; }\n}\n");

  fprintf(f, "void abyss_eye() {\n");
  fprintf(
      f,
      "  uint32_t as=0, ah=0, tf=0, ta=0; int ac=0; AllocInfo *c = alloc_head; "
      "while(c){ ta++; if(c->is_freed) tf+=c->size; else { ac++; "
      "if(c->is_stack) as+=c->size; else ah+=c->size; } c=c->next; }\n");
  fprintf(f,
          "  printf(\"\\n\" C_BOLD C_CYAN \"  "
          "╭───────────────────────────────────────────────────────────────────"
          "───────────────────────────────────────────╮\\n\" C_RESET);\n");
  fprintf(f,
          "  printf(C_BOLD C_CYAN \"  │\" C_RESET C_BOLD \"  📊   S Y S T E M  "
          " R E S O U R C E S                                                  "
          "                     \" C_BOLD C_CYAN \"│\\n\" C_RESET);\n");
  fprintf(f,
          "  printf(C_BOLD C_CYAN \"  "
          "├───────────────────────────────────────────────────────────────────"
          "───────────────────────────────────────────┤\\n\" C_RESET);\n");
  fprintf(f, "  printf(C_BOLD C_CYAN \"  │\" C_RESET \"  Active Stack Memory : "
             "\" C_BOLD C_GREEN \"%%-8u bytes\" C_RESET \"                     "
             "                                                  \" C_BOLD "
             "C_CYAN \"│\\n\" C_RESET, as);\n");
  fprintf(f, "  printf(C_BOLD C_CYAN \"  │\" C_RESET \"  Active Heap Memory  : "
             "\" C_BOLD C_YELLOW \"%%-8u bytes\" C_RESET \"                    "
             "                                                   \" C_BOLD "
             "C_CYAN \"│\\n\" C_RESET, ah);\n");
  fprintf(f, "  printf(C_BOLD C_CYAN \"  │\" C_RESET \"  Total Memory Freed  : "
             "\" C_BOLD C_GRAY \"%%-8u bytes\" C_RESET \"                      "
             "                                                 \" C_BOLD "
             "C_CYAN \"│\\n\" C_RESET, tf);\n");
  fprintf(f, "  printf(C_BOLD C_CYAN \"  │\" C_RESET \"  Total Allocations   : "
             "\" C_BOLD \"%%-8u\" C_RESET \"                                   "
             "                                          \" C_BOLD C_CYAN "
             "\"│\\n\" C_RESET, ta);\n");
  fprintf(f,
          "  printf(C_BOLD C_CYAN \"  "
          "╰───────────────────────────────────────────────────────────────────"
          "───────────────────────────────────────────╯\\n\\n\" C_RESET);\n");
  fprintf(f, "}\n\n");

  // --- 4. MAIN FUNCTION & JUMP TABLE ---
  fprintf(f, "int main() {\n");
  fprintf(f, "  register size_t sp = 0, fp = 0;\n"); // Hint to GCC to keep
                                                     // these in CPU registers!

  // Generate Jump Table for dynamic dispatch
  fprintf(f, "  static void *jump_table[%zu] = {0};\n", code_sz + 1);
  size_t ip = 0;
  while (ip < code_sz) {
    fprintf(f, "  jump_table[%zu] = &&L_%zu;\n", ip, ip);
    uint8_t op = code[ip++];

    // PERFECTLY SYNCED BYTE SIZES
    if (op == OP_CONST_INT || op == OP_CONST_STR || op == OP_JMP ||
        op == OP_JZ || op == OP_TRY || op == OP_NATIVE || op == OP_TAG_ALLOC)
      ip += 4;
    else if (op == OP_CONST_FLOAT)
      ip += 8;
    else if (op == OP_CALL)
      ip += 5;
    else if (op == OP_ALLOC_STRUCT || op == OP_ALLOC_ARRAY ||
             op == OP_ALLOC_STACK)
      ip += 8;
    else if (op == OP_GET_GLOBAL || op == OP_SET_GLOBAL || op == OP_GET_LOCAL ||
             op == OP_SET_LOCAL || op == OP_RET || op == OP_GET_FIELD ||
             op == OP_SET_FIELD || op == OP_PRINT_FMT || op == OP_CALL_DYN_BOT)
      ip += 1;
  }

  // --- 5. BYTECODE TRANSLATION LOOP ---
  ip = 0;
  while (ip < code_sz) {
    fprintf(f, "L_%zu:\n", ip);
    uint8_t op = code[ip++];

    switch (op) {
    case OP_HALT:
      fprintf(f, "  goto cleanup;\n");
      break;
    case OP_CONST_INT: {
      int32_t v;
      memcpy(&v, code + ip, 4);
      ip += 4;
      fprintf(f, "  stack[sp++].i = %d;\n", v);
      break;
    }
    case OP_CONST_FLOAT: {
      int64_t v;
      memcpy(&v, code + ip, 8);
      ip += 8;
      fprintf(f, "  stack[sp++].i = %ldLL;\n", v);
      break;
    }
    case OP_CONST_STR: {
      uint32_t v;
      memcpy(&v, code + ip, 4);
      ip += 4;
      fprintf(f, "  stack[sp++].p = strs[%u];\n", v);
      break;
    }

    case OP_ADD:
      fprintf(f, "  stack[sp-2].i += stack[sp-1].i; sp--;\n");
      break;
    case OP_SUB:
      fprintf(f, "  stack[sp-2].i -= stack[sp-1].i; sp--;\n");
      break;
    case OP_MUL:
      fprintf(f, "  stack[sp-2].i *= stack[sp-1].i; sp--;\n");
      break;
    case OP_DIV:
      fprintf(f, "  stack[sp-2].i /= stack[sp-1].i; sp--;\n");
      break;
    case OP_MOD:
      fprintf(f, "  stack[sp-2].i %%= stack[sp-1].i; sp--;\n");
      break;
    case OP_NEG:
      fprintf(f, "  stack[sp-1].i = -stack[sp-1].i;\n");
      break;

    case OP_ADD_F:
      fprintf(f, "  stack[sp-2].f += stack[sp-1].f; sp--;\n");
      break;
    case OP_SUB_F:
      fprintf(f, "  stack[sp-2].f -= stack[sp-1].f; sp--;\n");
      break;
    case OP_MUL_F:
      fprintf(f, "  stack[sp-2].f *= stack[sp-1].f; sp--;\n");
      break;
    case OP_DIV_F:
      fprintf(f, "  stack[sp-2].f /= stack[sp-1].f; sp--;\n");
      break;
    case OP_NEG_F:
      fprintf(f, "  stack[sp-1].f = -stack[sp-1].f;\n");
      break;

    case OP_LT:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].i < stack[sp-1].i); sp--;\n");
      break;
    case OP_LE:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].i <= stack[sp-1].i); sp--;\n");
      break;
    case OP_GT:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].i > stack[sp-1].i); sp--;\n");
      break;
    case OP_GE:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].i >= stack[sp-1].i); sp--;\n");
      break;
    case OP_EQ:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].i == stack[sp-1].i); sp--;\n");
      break;
    case OP_NE:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].i != stack[sp-1].i); sp--;\n");
      break;

    case OP_LT_F:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].f < stack[sp-1].f); sp--;\n");
      break;
    case OP_LE_F:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].f <= stack[sp-1].f); sp--;\n");
      break;
    case OP_GT_F:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].f > stack[sp-1].f); sp--;\n");
      break;
    case OP_GE_F:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].f >= stack[sp-1].f); sp--;\n");
      break;
    case OP_EQ_F:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].f == stack[sp-1].f); sp--;\n");
      break;
    case OP_NE_F:
      fprintf(f, "  stack[sp-2].i = (stack[sp-2].f != stack[sp-1].f); sp--;\n");
      break;

    case OP_AND:
      fprintf(f, "  stack[sp-2].i = stack[sp-2].i && stack[sp-1].i; sp--;\n");
      break;
    case OP_OR:
      fprintf(f, "  stack[sp-2].i = stack[sp-2].i || stack[sp-1].i; sp--;\n");
      break;
    case OP_NOT:
      fprintf(f, "  stack[sp-1].i = !stack[sp-1].i;\n");
      break;

    case OP_BIT_AND:
      fprintf(f, "  stack[sp-2].i &= stack[sp-1].i; sp--;\n");
      break;
    case OP_BIT_OR:
      fprintf(f, "  stack[sp-2].i |= stack[sp-1].i; sp--;\n");
      break;
    case OP_BIT_XOR:
      fprintf(f, "  stack[sp-2].i ^= stack[sp-1].i; sp--;\n");
      break;
    case OP_SHL:
      fprintf(f, "  stack[sp-2].i <<= stack[sp-1].i; sp--;\n");
      break;
    case OP_SHR:
      fprintf(f, "  stack[sp-2].i >>= stack[sp-1].i; sp--;\n");
      break;
    case OP_BIT_NOT:
      fprintf(f, "  stack[sp-1].i = ~stack[sp-1].i;\n");
      break;

    case OP_JMP: {
      uint32_t t;
      memcpy(&t, code + ip, 4);
      ip += 4;
      fprintf(f, "  goto L_%u;\n", t);
      break;
    }
    case OP_JZ: {
      uint32_t t;
      memcpy(&t, code + ip, 4);
      ip += 4;
      fprintf(f, "  if (!stack[--sp].i) goto L_%u;\n", t);
      break;
    }

    case OP_PRINT:
      fprintf(f, "  printf(\"%%ld\\n\", stack[--sp].i);\n");
      break;
    case OP_PRINT_F:
      fprintf(f, "  printf(\"%%.6f\\n\", stack[--sp].f);\n");
      break;
    case OP_PRINT_STR:
      fprintf(f, "  printf(\"%%s\\n\", (char*)stack[--sp].p);\n");
      break;
    case OP_PRINT_CHAR:
      fprintf(f, "  printf(\"%%c\", (char)stack[--sp].i);\n");
      break;
    case OP_PRINT_FMT: {
      uint8_t argc = code[ip++];
      fprintf(
          f,
          "  { uint8_t argc = %u; char *fmt = (char *)stack[sp - 1 - argc].p; "
          "int current_arg = 0; for (int i = 0; fmt[i]; i++) { if (fmt[i] == "
          "'%%') { i++; if (fmt[i] == '{') i++; char type_buf[32]; int ti = 0; "
          "while (fmt[i] && isalpha(fmt[i]) && ti < 31) type_buf[ti++] = "
          "fmt[i++]; type_buf[ti] = 0; if (fmt[i] == '}') i++; else i--; if "
          "(current_arg < argc) { Value val = stack[sp - argc + current_arg]; "
          "if (!strcmp(type_buf, \"int\") || !strcmp(type_buf, \"integer\")) "
          "printf(\"%%ld\", val.i); else if (!strcmp(type_buf, \"float\")) "
          "printf(\"%%.6f\", val.f); else if (!strcmp(type_buf, \"str\") || "
          "!strcmp(type_buf, \"string\")) printf(\"%%s\", (char *)val.p); else "
          "if (!strcmp(type_buf, \"char\")) printf(\"%%c\", (char)val.i); "
          "current_arg++; } } else putchar(fmt[i]); } printf(\"\\n\"); sp -= "
          "(argc + 1); }\n",
          argc);
      break;
    }

    case OP_GET_GLOBAL:
      fprintf(f, "  stack[sp++] = globals[%u];\n", code[ip++]);
      break;
    case OP_SET_GLOBAL:
      fprintf(f, "  globals[%u] = stack[--sp];\n", code[ip++]);
      break;
    case OP_GET_LOCAL:
      fprintf(f, "  stack[sp++] = stack[fp + %u];\n", code[ip++]);
      break;
    case OP_SET_LOCAL:
      fprintf(f, "  stack[fp + %u] = stack[--sp];\n", code[ip++]);
      break;

    case OP_CALL: {
      uint32_t addr;
      memcpy(&addr, code + ip, 4);
      ip += 4;
      uint8_t argc = code[ip++];
      fprintf(f,
              "  call_stack[csp].ret_addr = %zu; call_stack[csp].old_fp = fp; "
              "csp++; fp = sp - %u; goto L_%u;\n",
              ip, argc, addr);
      break;
    }
    case OP_RET: {
      uint8_t count = code[ip++];
      fprintf(
          f,
          "  { Value rets[8]; for(int i=0; i<%u; i++) rets[i] = stack[--sp]; "
          "free_stack_allocs(fp, %zu); if(csp==0) goto cleanup; csp--; size_t "
          "r = call_stack[csp].ret_addr; fp = call_stack[csp].old_fp; for(int "
          "i=%u-1; i>=0; i--) stack[sp++] = rets[i]; goto *jump_table[r]; }\n",
          count, ip, count);
      break;
    }
    case OP_CALL_DYN_BOT: {
      uint8_t argc = code[ip++];
      fprintf(f,
              "  { uint32_t addr = stack[sp - %u - 1].i; for(int i=0; i<%u; "
              "i++) stack[sp - %u - 1 + i] = stack[sp - %u + i]; sp--; "
              "call_stack[csp].ret_addr = %zu; call_stack[csp].old_fp = fp; "
              "csp++; fp = sp - %u; goto *jump_table[addr]; }\n",
              argc, argc, argc, argc, ip, argc);
      break;
    }
    case OP_POP:
      fprintf(f, "  sp--;\n");
      break;
    case OP_DUP:
      fprintf(f, "  stack[sp] = stack[sp-1]; sp++;\n");
      break;

    case OP_ALLOC_STRUCT: {
      uint32_t sid, cidx;
      memcpy(&sid, code + ip, 4);
      ip += 4;
      memcpy(&cidx, code + ip, 4);
      ip += 4;
      fprintf(
          f,
          "  { uint32_t sz = structs[%u].size * 8; int64_t *p = calloc(1, sz); "
          "track_alloc(p, %u, sz, 0, %s, %zu, fp); stack[sp++].p = p; }\n",
          sid, sid, cidx == 0xFFFFFFFF ? "NULL" : "strs[cidx]", ip);
      break;
    }
    case OP_ALLOC_ARRAY: {
      uint32_t es, cidx;
      memcpy(&es, code + ip, 4);
      ip += 4;
      memcpy(&cidx, code + ip, 4);
      ip += 4;
      fprintf(f,
              "  { int64_t c = stack[--sp].i; uint32_t sz = c * 8; int64_t *p "
              "= calloc(1, sz); track_alloc(p, 0xFFFFFFFF, sz, 0, %s, %zu, "
              "fp); stack[sp++].p = p; }\n",
              cidx == 0xFFFFFFFF ? "NULL" : "strs[cidx]", ip);
      break;
    }
    case OP_ALLOC_STACK: {
      uint32_t sid, cidx;
      memcpy(&sid, code + ip, 4);
      ip += 4;
      memcpy(&cidx, code + ip, 4);
      ip += 4;
      fprintf(
          f,
          "  { uint32_t sz = structs[%u].size * 8; int64_t *p = calloc(1, sz); "
          "track_alloc(p, %u, sz, 1, %s, %zu, fp); stack[sp++].p = p; }\n",
          sid, sid, cidx == 0xFFFFFFFF ? "NULL" : "strs[cidx]", ip);
      break;
    }
    case OP_FREE:
      fprintf(
          f, "  { void *p = stack[--sp].p; untrack_alloc(p, %zu); free(p); }\n",
          ip);
      break;

    case OP_GET_FIELD:
      fprintf(f, "  { int64_t *p = stack[sp-1].p; stack[sp-1].i = p[%u]; }\n",
              code[ip++]);
      break;
    case OP_SET_FIELD:
      fprintf(f,
              "  { int64_t v = stack[--sp].i; int64_t *p = stack[--sp].p; "
              "p[%u] = v; }\n",
              code[ip++]);
      break;
    case OP_GET_INDEX:
      fprintf(f, "  { int64_t i = stack[--sp].i; int64_t *p = stack[--sp].p; "
                 "stack[sp++].i = p[i]; }\n");
      break;
    case OP_SET_INDEX:
      fprintf(f, "  { int64_t v = stack[--sp].i; int64_t i = stack[--sp].i; "
                 "int64_t *p = stack[--sp].p; p[i] = v; }\n");
      break;
    case OP_INC_INDEX:
      fprintf(f, "  { int64_t i = stack[--sp].i; int64_t *p = stack[--sp].p; "
                 "p[i]++; }\n");
      break;
    case OP_DEC_INDEX:
      fprintf(f, "  { int64_t i = stack[--sp].i; int64_t *p = stack[--sp].p; "
                 "p[i]--; }\n");
      break;

    case OP_STR_CAT:
      fprintf(
          f,
          "  { char *s2 = stack[--sp].p; char *s1 = stack[--sp].p; size_t l1 = "
          "strlen(s1), l2 = strlen(s2); char *n = malloc(l1+l2+1); strcpy(n, "
          "s1); strcat(n, s2); track_alloc(n, 0xFFFFFFFE, l1+l2+1, 0, "
          "\"Dynamic String\", %zu, fp); stack[sp++].p = n; }\n",
          ip);
      break;

    case OP_ABYSS_EYE:
      fprintf(f, "  abyss_eye();\n");
      break;

    case OP_TRY: {
      uint32_t catch_addr;
      memcpy(&catch_addr, code + ip, 4);
      ip += 4;
      fprintf(f,
              "  exception_stack[esp++] = (ExceptionFrame){%u, sp, fp, csp};\n",
              catch_addr);
      break;
    }
    case OP_END_TRY:
      fprintf(f, "  if (esp > 0) esp--;\n");
      break;
    case OP_THROW:
      fprintf(
          f, "  { Value err_val = stack[--sp]; if (esp == 0) { fprintf(stderr, "
             "\"Uncaught Exception: %%s\\n\", (char *)err_val.p); exit(1); } "
             "esp--; size_t ca = exception_stack[esp].catch_addr; sp = "
             "exception_stack[esp].old_sp; fp = exception_stack[esp].old_fp; "
             "csp = exception_stack[esp].old_csp; stack[sp++] = err_val; goto "
             "*jump_table[ca]; }\n");
      break;

    case OP_NATIVE: {
      uint32_t nid;
      memcpy(&nid, code + ip, 4);
      ip += 4;
      if (nid == 0)
        fprintf(
            f,
            "  { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); "
            "double t = ts.tv_sec + ts.tv_nsec / 1e9; stack[sp++].f = t; }\n");
      else if (nid == 1)
        fprintf(f, "  { char buf[64]; int64_t v=0; if(fgets(buf, sizeof(buf), "
                   "stdin)) v = atoll(buf); stack[sp++].i = v; }\n");
      else if (nid == 10)
        fprintf(f, "  abyss_io_print_str(stack[--sp].p);\n");
      else if (nid == 11)
        fprintf(f, "  abyss_io_print_int(stack[--sp].i);\n");
      else if (nid == 12)
        fprintf(f, "  abyss_io_print_float(stack[--sp].f);\n");
      else if (nid == 20)
        fprintf(f, "  { int64_t e = stack[--sp].i; int64_t b = stack[--sp].i; "
                   "stack[sp++].i = abyss_math_pow(b, e); }\n");
      else if (nid == 21)
        fprintf(f, "  { stack[sp-1].i = abyss_math_abs(stack[sp-1].i); }\n");
      break;
    }
    case OP_TAG_ALLOC: {
      uint32_t sidx;
      memcpy(&sidx, code + ip, 4);
      ip += 4;
      fprintf(f,
              "  { void *p = stack[sp-1].p; AllocInfo *c = alloc_head; "
              "while(c) { if(c->ptr == p && !c->is_freed) { c->tag = strs[%u]; "
              "break; } c = c->next; } }\n",
              sidx);
      break;
    }
    default:
      fprintf(stderr, "FATAL: Unimplemented opcode %d in native transpiler!\n",
              op);
      exit(1);
    }
  }

  fprintf(f, "cleanup:\n  return 0;\n}\n");
  fclose(f);
}
