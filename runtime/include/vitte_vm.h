#ifndef VITTE_VM_H
#define VITTE_VM_H

#include "vitte_types.h"
#include <stddef.h>

#define VITTE_STACK_MAX 256
#define VITTE_REGISTERS 16
#define VITTE_HEAP_SIZE (10 * 1024 * 1024) /* 10 MB */

/* Bytecode opcodes */
typedef enum {
    OP_CONST,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_RETURN,
    OP_CLASS,
    OP_METHOD,
    OP_INVOKE,
    OP_ARRAY,
    OP_INDEX_GET,
    OP_INDEX_SET,
    OP_LOAD_REG,
    OP_STORE_REG,
    OP_HALT,
} opcode_t;

typedef struct {
    uint8_t *code;
    vitte_value_t *constants;
    size_t code_size;
    size_t code_capacity;
    size_t constant_count;
    size_t constant_capacity;
    int *lines;
    size_t line_count;
} chunk_t;

typedef struct vitte_vm {
    /* Stack */
    vitte_value_t stack[VITTE_STACK_MAX];
    vitte_value_t *stack_top;
    
    /* Registers */
    vitte_value_t registers[VITTE_REGISTERS];
    
    /* Memory */
    uint8_t *heap;
    size_t heap_size;
    size_t heap_used;
    
    /* Execution */
    uint8_t *ip;  /* Instruction pointer */
    chunk_t *chunk;
    
    /* Globals */
    void *globals;  /* Hash table */
    
    /* Frame stack for function calls */
    struct call_frame {
        vitte_closure_t *closure;
        uint8_t *ip;
        int stack_offset;
    } frames[64];
    int frame_count;
    
    /* Timing */
    uint64_t instruction_count;
    
    /* Error handling */
    char error_buffer[512];
    bool had_error;
} vitte_vm_t;

typedef enum {
    VITTE_OK,
    VITTE_COMPILE_ERROR,
    VITTE_RUNTIME_ERROR,
    VITTE_STACK_OVERFLOW,
    VITTE_STACK_UNDERFLOW,
    VITTE_TYPE_ERROR,
} vitte_result_t;

/* VM creation/destruction */
vitte_vm_t* vitte_vm_create(void);
void vitte_vm_free(vitte_vm_t *vm);
void vitte_vm_reset(vitte_vm_t *vm);

/* Execution */
vitte_result_t vitte_vm_interpret(vitte_vm_t *vm, const char *source);
vitte_result_t vitte_vm_execute(vitte_vm_t *vm, chunk_t *chunk);

/* Stack operations */
void vitte_push(vitte_vm_t *vm, vitte_value_t value);
vitte_value_t vitte_pop(vitte_vm_t *vm);
vitte_value_t vitte_peek(vitte_vm_t *vm, int distance);

/* Register operations */
void vitte_load_register(vitte_vm_t *vm, int reg, vitte_value_t value);
vitte_value_t vitte_get_register(vitte_vm_t *vm, int reg);

/* Memory operations */
void* vitte_allocate(vitte_vm_t *vm, size_t size);
void vitte_free_object(vitte_vm_t *vm, void *ptr);

/* Chunk operations */
chunk_t* chunk_create(void);
void chunk_free(chunk_t *chunk);
void chunk_write(chunk_t *chunk, uint8_t byte, int line);
int chunk_add_constant(chunk_t *chunk, vitte_value_t value);
void chunk_disassemble(chunk_t *chunk, const char *name);

/* Error handling */
void vitte_runtime_error(vitte_vm_t *vm, const char *format, ...);
const char* vitte_get_error(vitte_vm_t *vm);

#endif
