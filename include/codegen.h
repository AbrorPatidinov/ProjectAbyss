#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdint.h>
#include <stddef.h>

extern uint8_t *code;
extern size_t code_sz;
extern char **strs;
extern int str_count;

void codegen_init();
void emit(uint8_t b);
void emit32(uint32_t v);
void emit64(uint64_t v);
void emit_patch(size_t addr, uint32_t v);
int add_str(const char *s);

size_t codegen_size(void);
uint8_t *codegen_buffer(void);

#endif
