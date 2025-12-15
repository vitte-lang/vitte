#ifndef STEEL_STACK_H
#define STEEL_STACK_H

#include "value.h"
#include <stddef.h>

typedef struct {
    Value *values;
    size_t size;
    size_t capacity;
} Stack;

Stack *stack_create(size_t capacity);
void stack_push(Stack *s, Value v);
Value stack_pop(Stack *s);
void stack_destroy(Stack *s);

#endif
