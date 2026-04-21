#ifndef CODEGEN_H
#define CODEGEN_H

#include <stddef.h>
#include <stdint.h>

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

// --- Forward-reference patch table ---
// Used by the two-pass compiler. When an OP_CALL is emitted, the target
// function address may not be known yet (forward reference to a function
// defined later in the file). Emit a placeholder address and record the
// patch location; after the full parse, resolve_call_patches() walks the
// list and writes the real address into every placeholder.
void add_call_patch(size_t patch_addr, int fid);
void resolve_call_patches(void);
void emit_call_to(int fid, uint8_t argc);

#endif
