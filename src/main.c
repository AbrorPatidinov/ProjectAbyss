#include "../include/codegen.h"
#include "../include/common.h"
#include "../include/lexer.h"
#include "../include/native.h"
#include "../include/parser.h"
#include "../include/symbols.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ABYSS_VERSION "0.1.0"

int main(int argc, char **argv) {
  if (argc > 1) {
    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
      printf("AbyssLang Compiler (abyssc) version %s\n", ABYSS_VERSION);
      return 0;
    }
  }

  int is_native = 0;
  int enable_eye = 0;
  char *src_file = NULL;
  char *out_file = NULL;

  // --- FLAG PARSING ---
  int argi = 1;
  while (argi < argc && argv[argi][0] == '-') {
    if (strcmp(argv[argi], "--native") == 0) {
      is_native = 1;
      argi++;
    } else if (strcmp(argv[argi], "--eye") == 0) {
      enable_eye = 1;
      argi++;
    } else {
      argi++;
    }
  }
  if (argi + 2 > argc) {
    fprintf(stderr, "Usage: %s [--native] [--eye] <source.al> <output>\n",
            argv[0]);
    return 1;
  }
  src_file = argv[argi];
  out_file = argv[argi + 1];

  // Read Source
  FILE *f = fopen(src_file, "rb");
  if (!f) {
    fprintf(stderr, "Cannot open file: %s\n", src_file);
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *src = malloc(sz + 1);
  fread(src, 1, sz, f);
  src[sz] = 0;
  fclose(f);

  // Init Modules
  lexer_init(src);
  codegen_init();
  symbols_init();

  // Compile
  parse_program();

  // Finalize
  int main_idx = find_func("main");
  if (main_idx != -1) {
    emit(OP_CALL);
    emit32(funcs[main_idx].addr);
    emit(0);
  }
  emit(OP_HALT);

  // --- NEW: NATIVE COMPILATION ROUTINE ---
  if (is_native) {
    printf("⚡ Translating AbyssLang to Native C...%s\n",
           enable_eye ? " (Abyss Eye: ON)" : "");
    generate_native_code("abyss_native_temp.c", enable_eye);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "gcc -O3 -flto=auto -march=native -fno-strict-aliasing -Wall "
             "-Wno-unused-variable -Wno-unused-label -Wno-unused-parameter "
             "-D_POSIX_C_SOURCE=200809L "
             "-o %s abyss_native_temp.c -lm",
             out_file);
    printf("🔨 Compiling Native Binary: %s\n", out_file);
    int res = system(cmd);

    if (res == 0) {
      printf("✅ Native compilation successful! Run with: ./%s\n", out_file);
      remove("abyss_native_temp.c"); // Clean up the temp file
    } else {
      printf("❌ Native compilation failed.\n");
    }
    return res;
  }

  // --- STANDARD BYTECODE OUTPUT ---
  f = fopen(out_file, "wb");
  if (!f) {
    fprintf(stderr, "Cannot open output file: %s\n", out_file);
    return 1;
  }

  fwrite(MAGIC, 1, 7, f);
  uint8_t ver = VERSION;
  fwrite(&ver, 1, 1, f);

  fwrite(&str_count, 4, 1, f);
  for (int i = 0; i < str_count; i++) {
    uint32_t len = strlen(strs[i]);
    fwrite(&len, 4, 1, f);
    fwrite(strs[i], 1, len, f);
  }

  fwrite(&struct_count, 4, 1, f);
  for (int i = 0; i < struct_count; i++) {
    uint32_t len = strlen(structs[i].name);
    fwrite(&len, 4, 1, f);
    fwrite(structs[i].name, 1, len, f);
    uint32_t size = structs[i].size;
    fwrite(&size, 4, 1, f);
  }

  size_t size = codegen_size();
  uint8_t *buffer = codegen_buffer();

  fwrite(&size, 4, 1, f);
  fwrite(buffer, 1, size, f);

  fclose(f);

  printf("Compiled %s -> %s (%zu bytes)\n", src_file, out_file, code_sz);
  return 0;
}
