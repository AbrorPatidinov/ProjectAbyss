#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/common.h"
#include "../include/codegen.h"
#include "../include/symbols.h"
#include "../include/parser.h"
#include "../include/lexer.h" // Needed for lexer_init

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source.al> <output.aby>\n", argv[0]);
        return 1;
    }

    // Read Source
    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "Cannot open file: %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *src = malloc(sz+1); fread(src, 1, sz, f); src[sz] = 0; fclose(f);

    // Init Modules
    lexer_init(src);
    codegen_init();
    symbols_init();

    // Compile
    parse_program();

    // Finalize
    int main_idx = find_func("main");
    if (main_idx != -1) {
        emit(OP_CALL); emit32(funcs[main_idx].addr); emit(0);
    }
    emit(OP_HALT);

    // Write Output
    f = fopen(argv[2], "wb");
    if (!f) { fprintf(stderr, "Cannot open output file: %s\n", argv[2]); return 1; }

    fwrite(MAGIC, 1, 7, f);
    uint8_t ver = VERSION;
    fwrite(&ver, 1, 1, f);

    fwrite(&str_count, 4, 1, f);
    for(int i=0; i<str_count; i++) {
        uint32_t len = strlen(strs[i]);
        fwrite(&len, 4, 1, f);
        fwrite(strs[i], 1, len, f);
    }

    fwrite(&struct_count, 4, 1, f);
    for(int i=0; i<struct_count; i++) {
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

    printf("Compiled %s -> %s (%zu bytes)\n", argv[1], argv[2], code_sz);
    return 0;
}
