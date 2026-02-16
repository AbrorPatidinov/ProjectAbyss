#include <stdlib.h>
#include <string.h>
#include "../include/codegen.h"
#include "../include/utils.h"
#include "../include/common.h"

uint8_t *code = NULL;
size_t code_sz = 0;
static size_t code_cap = 0;

char **strs = NULL;
int str_count = 0;
static int str_cap = 0;

void codegen_init() {
    code_cap = 1024;
    code = malloc(code_cap);
    str_cap = INIT_CAP;
    strs = malloc(str_cap * sizeof(char*));
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
    if (str_count >= str_cap) {
        str_cap = str_cap ? str_cap * 2 : INIT_CAP;
        strs = realloc(strs, str_cap * sizeof(char*));
    }
    strs[str_count] = strdup(s);
    return str_count++;
}

size_t codegen_size(void) { return code_sz; }
uint8_t *codegen_buffer(void) { return code; }
