#include "disassembler.h"
#include "../include/vitte_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool use_colors = true;

#define RESET   "\x1b[0m"
#define BOLD    "\x1b[1m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"

void disasm_init(void) {
    /* Initialize disassembler */
}

void disasm_enable_colors(bool enable) {
    use_colors = enable;
}

const char* opcode_name(uint8_t opcode) {
    switch (opcode) {
        case OP_CONST: return "OP_CONST";
        case OP_NIL: return "OP_NIL";
        case OP_TRUE: return "OP_TRUE";
        case OP_FALSE: return "OP_FALSE";
        case OP_POP: return "OP_POP";
        case OP_GET_LOCAL: return "OP_GET_LOCAL";
        case OP_SET_LOCAL: return "OP_SET_LOCAL";
        case OP_GET_GLOBAL: return "OP_GET_GLOBAL";
        case OP_SET_GLOBAL: return "OP_SET_GLOBAL";
        case OP_EQUAL: return "OP_EQUAL";
        case OP_GREATER: return "OP_GREATER";
        case OP_LESS: return "OP_LESS";
        case OP_ADD: return "OP_ADD";
        case OP_SUBTRACT: return "OP_SUBTRACT";
        case OP_MULTIPLY: return "OP_MULTIPLY";
        case OP_DIVIDE: return "OP_DIVIDE";
        case OP_NOT: return "OP_NOT";
        case OP_NEGATE: return "OP_NEGATE";
        case OP_PRINT: return "OP_PRINT";
        case OP_JUMP: return "OP_JUMP";
        case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OP_LOOP: return "OP_LOOP";
        case OP_CALL: return "OP_CALL";
        case OP_RETURN: return "OP_RETURN";
        case OP_CLASS: return "OP_CLASS";
        case OP_METHOD: return "OP_METHOD";
        case OP_INVOKE: return "OP_INVOKE";
        case OP_ARRAY: return "OP_ARRAY";
        case OP_INDEX_GET: return "OP_INDEX_GET";
        case OP_INDEX_SET: return "OP_INDEX_SET";
        case OP_LOAD_REG: return "OP_LOAD_REG";
        case OP_STORE_REG: return "OP_STORE_REG";
        case OP_HALT: return "OP_HALT";
        default: return "UNKNOWN";
    }
}

const char* opcode_description(uint8_t opcode) {
    switch (opcode) {
        case OP_CONST: return "Push constant value";
        case OP_NIL: return "Push nil";
        case OP_TRUE: return "Push true";
        case OP_FALSE: return "Push false";
        case OP_POP: return "Pop stack top";
        case OP_GET_LOCAL: return "Get local variable";
        case OP_SET_LOCAL: return "Set local variable";
        case OP_GET_GLOBAL: return "Get global variable";
        case OP_SET_GLOBAL: return "Set global variable";
        case OP_EQUAL: return "Equality comparison";
        case OP_GREATER: return "Greater than comparison";
        case OP_LESS: return "Less than comparison";
        case OP_ADD: return "Addition";
        case OP_SUBTRACT: return "Subtraction";
        case OP_MULTIPLY: return "Multiplication";
        case OP_DIVIDE: return "Division";
        case OP_NOT: return "Logical NOT";
        case OP_NEGATE: return "Negation";
        case OP_PRINT: return "Print value";
        case OP_JUMP: return "Jump forward";
        case OP_JUMP_IF_FALSE: return "Conditional jump";
        case OP_LOOP: return "Loop back";
        case OP_CALL: return "Function call";
        case OP_RETURN: return "Function return";
        case OP_CLASS: return "Class definition";
        case OP_METHOD: return "Method definition";
        case OP_INVOKE: return "Method invocation";
        case OP_ARRAY: return "Array creation";
        case OP_INDEX_GET: return "Array/object indexing";
        case OP_INDEX_SET: return "Array/object assignment";
        case OP_LOAD_REG: return "Load register";
        case OP_STORE_REG: return "Store register";
        case OP_HALT: return "Program halt";
        default: return "Unknown opcode";
    }
}

int opcode_operand_count(uint8_t opcode) {
    switch (opcode) {
        case OP_CONST:
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_LOAD_REG:
        case OP_STORE_REG:
        case OP_CALL:
        case OP_ARRAY:
        case OP_INVOKE:
            return 1;
        case OP_JUMP:
        case OP_JUMP_IF_FALSE:
        case OP_LOOP:
            return 2;
        default:
            return 0;
    }
}

void disasm_instruction(uint8_t *code, int offset) {
    if (!code) return;
    
    uint8_t opcode = code[offset];
    const char *color = use_colors ? CYAN : "";
    const char *reset = use_colors ? RESET : "";
    
    printf("%s%04d%s ", color, offset, reset);
    printf("%s%-20s%s", use_colors ? GREEN : "", opcode_name(opcode), reset);
    
    int operand_count = opcode_operand_count(opcode);
    for (int i = 0; i < operand_count; i++) {
        printf(" %d", code[offset + 1 + i]);
    }
    printf("\n");
}

void disasm_constants(chunk_t *chunk) {
    if (!chunk) return;
    
    printf("Constants:\n");
    for (int i = 0; i < chunk->constant_count; i++) {
        printf("[%d] ", i);
        vitte_print(chunk->constants[i]);
        printf("\n");
    }
}

void disasm_chunk(chunk_t *chunk, const char *name) {
    disasm_options_t opts = {
        .show_constants = true,
        .show_bytecode = true,
        .show_stack_effects = false,
        .verbose = false
    };
    disasm_chunk_with_options(chunk, name, opts);
}

void disasm_chunk_with_options(chunk_t *chunk, const char *name, disasm_options_t opts) {
    if (!chunk || !name) return;
    
    const char *color = use_colors ? BOLD : "";
    const char *reset = use_colors ? RESET : "";
    
    printf("%s== %s ==%s\n", color, name, reset);
    
    if (opts.show_constants && chunk->constant_count > 0) {
        disasm_constants(chunk);
        printf("\n");
    }
    
    if (opts.show_bytecode) {
        printf("Bytecode:\n");
        for (int i = 0; i < chunk->code_size; ) {
            disasm_instruction(chunk->code, i);
            i += 1 + opcode_operand_count(chunk->code[i]);
        }
    }
    
    printf("\n");
}
