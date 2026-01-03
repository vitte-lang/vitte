#include "../include/vitte_vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* Chunk operations */
chunk_t* chunk_create(void) {
    chunk_t *chunk = (chunk_t *)malloc(sizeof(chunk_t));
    if (!chunk) return NULL;
    
    chunk->code = (uint8_t *)malloc(256);
    chunk->constants = (vitte_value_t *)malloc(sizeof(vitte_value_t) * 256);
    chunk->lines = (int *)malloc(sizeof(int) * 256);
    
    chunk->code_size = 0;
    chunk->code_capacity = 256;
    chunk->constant_count = 0;
    chunk->constant_capacity = 256;
    
    return chunk;
}

void chunk_free(chunk_t *chunk) {
    if (!chunk) return;
    free(chunk->code);
    free(chunk->constants);
    free(chunk->lines);
    free(chunk);
}

void chunk_write(chunk_t *chunk, uint8_t byte, int line) {
    if (chunk->code_size >= chunk->code_capacity) {
        chunk->code_capacity *= 2;
        chunk->code = (uint8_t *)realloc(chunk->code, chunk->code_capacity);
    }
    
    chunk->code[chunk->code_size] = byte;
    chunk->lines[chunk->code_size] = line;
    chunk->code_size++;
}

int chunk_add_constant(chunk_t *chunk, vitte_value_t value) {
    if (chunk->constant_count >= chunk->constant_capacity) {
        chunk->constant_capacity *= 2;
        chunk->constants = (vitte_value_t *)realloc(chunk->constants, 
                                                     sizeof(vitte_value_t) * chunk->constant_capacity);
    }
    
    chunk->constants[chunk->constant_count] = value;
    return chunk->constant_count++;
}

/* VM operations */
vitte_vm_t* vitte_vm_create(void) {
    vitte_vm_t *vm = (vitte_vm_t *)malloc(sizeof(vitte_vm_t));
    if (!vm) return NULL;
    
    vm->stack_top = 0;
    vm->heap = (uint8_t *)malloc(VITTE_HEAP_SIZE);
    vm->heap_size = VITTE_HEAP_SIZE;
    vm->heap_used = 0;
    vm->ip = NULL;
    vm->chunk = NULL;
    vm->globals = NULL;
    vm->frame_count = 0;
    vm->instruction_count = 0;
    vm->had_error = false;
    memset(vm->error_buffer, 0, sizeof(vm->error_buffer));
    
    return vm;
}

void vitte_vm_free(vitte_vm_t *vm) {
    if (!vm) return;
    free(vm->heap);
    free(vm);
}

void vitte_vm_reset(vitte_vm_t *vm) {
    vm->stack_top = 0;
    vm->frame_count = 0;
    vm->instruction_count = 0;
    vm->had_error = false;
    vm->heap_used = 0;
}

/* Stack operations */
void vitte_push(vitte_vm_t *vm, vitte_value_t value) {
    if (vm->stack_top >= VITTE_STACK_MAX) {
        strcpy(vm->error_buffer, "Stack overflow");
        vm->had_error = true;
        return;
    }
    vm->stack[vm->stack_top++] = value;
}

vitte_value_t vitte_pop(vitte_vm_t *vm) {
    if (vm->stack_top <= 0) {
        strcpy(vm->error_buffer, "Stack underflow");
        vm->had_error = true;
        return vitte_nil();
    }
    return vm->stack[--vm->stack_top];
}

vitte_value_t vitte_peek(vitte_vm_t *vm, int distance) {
    if (vm->stack_top - distance - 1 < 0) {
        return vitte_nil();
    }
    return vm->stack[vm->stack_top - distance - 1];
}

/* Register operations */
void vitte_load_register(vitte_vm_t *vm, uint8_t reg, vitte_value_t value) {
    if (reg >= VITTE_REGISTERS) {
        strcpy(vm->error_buffer, "Invalid register");
        vm->had_error = true;
        return;
    }
    vm->registers[reg] = value;
}

vitte_value_t vitte_get_register(vitte_vm_t *vm, uint8_t reg) {
    if (reg >= VITTE_REGISTERS) {
        strcpy(vm->error_buffer, "Invalid register");
        vm->had_error = true;
        return vitte_nil();
    }
    return vm->registers[reg];
}

/* Memory allocation */
void* vitte_allocate(vitte_vm_t *vm, size_t size) {
    if (vm->heap_used + size > vm->heap_size) {
        strcpy(vm->error_buffer, "Heap overflow");
        vm->had_error = true;
        return NULL;
    }
    
    void *ptr = vm->heap + vm->heap_used;
    vm->heap_used += size;
    return ptr;
}

void vitte_free_object(vitte_vm_t *vm, void *ptr) {
    (void)vm;
    (void)ptr;
    /* Simple implementation - real GC would track allocations */
}

/* Error handling */
void vitte_runtime_error(vitte_vm_t *vm, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(vm->error_buffer, sizeof(vm->error_buffer), format, args);
    va_end(args);
    vm->had_error = true;
}

const char* vitte_get_error(vitte_vm_t *vm) {
    return vm->error_buffer;
}

/* Interpreter loop */
static vitte_result_t vitte_execute_internal(vitte_vm_t *vm) {
    #define READ_BYTE() (*vm->ip++)
    #define READ_CONSTANT() (vm->chunk->constants[READ_BYTE()])
    #define BINARY_OP(op) do { \
        vitte_value_t b = vitte_pop(vm); \
        vitte_value_t a = vitte_pop(vm); \
        switch(op) { \
            case '+': vitte_push(vm, vitte_add(a, b)); break; \
            case '-': vitte_push(vm, vitte_subtract(a, b)); break; \
            case '*': vitte_push(vm, vitte_multiply(a, b)); break; \
            case '/': vitte_push(vm, vitte_divide(a, b)); break; \
            default: break; \
        } \
    } while(0)
    
    vm->ip = vm->chunk->code;
    
    while (vm->ip < vm->chunk->code + vm->chunk->code_size) {
        if (vm->had_error) return VITTE_RUNTIME_ERROR;
        
        uint8_t instruction = READ_BYTE();
        vm->instruction_count++;
        
        switch (instruction) {
            case OP_CONST: {
                vitte_value_t constant = READ_CONSTANT();
                vitte_push(vm, constant);
                break;
            }
            case OP_NIL:
                vitte_push(vm, vitte_nil());
                break;
            case OP_TRUE:
                vitte_push(vm, vitte_bool(true));
                break;
            case OP_FALSE:
                vitte_push(vm, vitte_bool(false));
                break;
            case OP_POP:
                vitte_pop(vm);
                break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                if (slot < vm->stack_top) {
                    vitte_push(vm, vm->stack[slot]);
                }
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                if (slot < VITTE_STACK_MAX) {
                    vm->stack[slot] = vitte_peek(vm, 0);
                }
                break;
            }
            case OP_EQUAL: {
                vitte_value_t b = vitte_pop(vm);
                vitte_value_t a = vitte_pop(vm);
                vitte_push(vm, vitte_bool(vitte_equal(a, b)));
                break;
            }
            case OP_GREATER: {
                vitte_value_t b = vitte_pop(vm);
                vitte_value_t a = vitte_pop(vm);
                vitte_push(vm, vitte_bool(!vitte_less(a, b) && !vitte_equal(a, b)));
                break;
            }
            case OP_LESS: {
                vitte_value_t b = vitte_pop(vm);
                vitte_value_t a = vitte_pop(vm);
                vitte_push(vm, vitte_bool(vitte_less(a, b)));
                break;
            }
            case OP_ADD: {
                BINARY_OP('+');
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP('-');
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP('*');
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP('/');
                break;
            }
            case OP_NOT: {
                vitte_value_t v = vitte_pop(vm);
                vitte_push(vm, vitte_bool(!vitte_is_truthy(v)));
                break;
            }
            case OP_NEGATE: {
                vitte_value_t v = vitte_pop(vm);
                vitte_push(vm, vitte_negate(v));
                break;
            }
            case OP_PRINT: {
                vitte_value_t v = vitte_pop(vm);
                vitte_print(v);
                printf("\n");
                break;
            }
            case OP_JUMP: {
                uint16_t offset = (READ_BYTE() << 8) | READ_BYTE();
                vm->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = (READ_BYTE() << 8) | READ_BYTE();
                if (!vitte_is_truthy(vitte_peek(vm, 0))) {
                    vm->ip += offset;
                }
                break;
            }
            case OP_LOOP: {
                uint16_t offset = (READ_BYTE() << 8) | READ_BYTE();
                vm->ip -= offset;
                break;
            }
            case OP_CALL: {
                uint8_t arg_count = READ_BYTE();
                (void)arg_count;
                break;
            }
            case OP_RETURN: {
                return VITTE_OK;
            }
            case OP_ARRAY: {
                uint8_t count = READ_BYTE();
                vitte_value_t arr = vitte_array();
                vitte_array_t *arr_obj = (vitte_array_t *)arr.as.pointer;
                for (int i = 0; i < count; i++) {
                    if (arr_obj->count >= arr_obj->capacity) {
                        arr_obj->capacity *= 2;
                        arr_obj->values = (vitte_value_t *)realloc(arr_obj->values, 
                                                                   sizeof(vitte_value_t) * arr_obj->capacity);
                    }
                    arr_obj->values[arr_obj->count++] = vitte_pop(vm);
                }
                vitte_push(vm, arr);
                break;
            }
            case OP_HALT:
                return VITTE_OK;
            default:
                return VITTE_RUNTIME_ERROR;
        }
    }
    
    return VITTE_OK;
}

vitte_result_t vitte_vm_execute(vitte_vm_t *vm, chunk_t *chunk) {
    vm->chunk = chunk;
    return vitte_execute_internal(vm);
}

/* Disassembly */
void chunk_disassemble(chunk_t *chunk, const char *name) {
    printf("== %s ==\n", name);
    
    for (int i = 0; i < chunk->code_size; ) {
        printf("%04d ", i);
        
        uint8_t instruction = chunk->code[i];
        switch (instruction) {
            case OP_CONST:
                printf("OP_CONST %d\n", chunk->code[i + 1]);
                i += 2;
                break;
            case OP_NIL:
                printf("OP_NIL\n");
                i++;
                break;
            case OP_TRUE:
                printf("OP_TRUE\n");
                i++;
                break;
            case OP_FALSE:
                printf("OP_FALSE\n");
                i++;
                break;
            case OP_POP:
                printf("OP_POP\n");
                i++;
                break;
            case OP_PRINT:
                printf("OP_PRINT\n");
                i++;
                break;
            case OP_RETURN:
                printf("OP_RETURN\n");
                i++;
                break;
            case OP_HALT:
                printf("OP_HALT\n");
                i++;
                break;
            default:
                printf("UNKNOWN %d\n", instruction);
                i++;
        }
    }
}
