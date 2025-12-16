#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include <stdint.h>

typedef enum {
    IR_OP_NOP,
    IR_OP_ADD,
    IR_OP_SUB,
    IR_OP_MUL,
    IR_OP_DIV,
    IR_OP_MOD,
    IR_OP_AND,
    IR_OP_OR,
    IR_OP_XOR,
    IR_OP_SHL,
    IR_OP_SHR,
    IR_OP_CMP,
    IR_OP_LOAD,
    IR_OP_STORE,
    IR_OP_CALL,
    IR_OP_RET,
    IR_OP_BR,
    IR_OP_BR_COND,
} ir_opcode_t;

typedef struct ir_value {
    uint32_t id;
    void *ty;
    ir_opcode_t op;
    void *operands;
    uint32_t operand_count;
} ir_value_t;

typedef struct ir_block {
    uint32_t id;
    ir_value_t **values;
    uint32_t value_count;
    uint32_t value_capacity;
} ir_block_t;

typedef struct ir_func {
    const char *name;
    void *ret_ty;
    void *param_tys;
    uint32_t param_count;
    ir_block_t **blocks;
    uint32_t block_count;
} ir_func_t;

typedef struct ir_module {
    ir_func_t **funcs;
    uint32_t func_count;
    uint32_t func_capacity;
} ir_module_t;

ir_module_t* ir_module_create(void);
void ir_module_destroy(ir_module_t *mod);
void ir_module_add_func(ir_module_t *mod, ir_func_t *func);

#endif /* COMPILER_IR_H */
