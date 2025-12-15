#ifndef STEEL_VM_H
#define STEEL_VM_H

#include <stddef.h>

typedef struct VM {
    void *stack;
    size_t stack_size;
    size_t stack_ptr;
} VM;

VM *vm_create(size_t stack_size);
void vm_execute(VM *vm, const void *bytecode);
void vm_destroy(VM *vm);

#endif
