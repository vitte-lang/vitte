#ifndef VITTE_DISASSEMBLER_H
#define VITTE_DISASSEMBLER_H

#include <stdint.h>

typedef struct chunk chunk_t;

/* Disassembly options */
typedef struct {
    bool show_constants;
    bool show_bytecode;
    bool show_stack_effects;
    bool verbose;
} disasm_options_t;

/* Disassembly output */
void disasm_init(void);
void disasm_enable_colors(bool enable);

void disasm_chunk(chunk_t *chunk, const char *name);
void disasm_chunk_with_options(chunk_t *chunk, const char *name, disasm_options_t opts);
void disasm_instruction(uint8_t *code, int offset);
void disasm_constants(chunk_t *chunk);

/* Opcode information */
const char* opcode_name(uint8_t opcode);
int opcode_operand_count(uint8_t opcode);
const char* opcode_description(uint8_t opcode);

#endif
