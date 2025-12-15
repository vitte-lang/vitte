#ifndef STEEL_CONTEXT_H
#define STEEL_CONTEXT_H

#include "stack.h"
#include "gc.h"
#include "value.h"

typedef struct {
    Stack *stack;
    GC *gc;
    Value *variables;
    size_t var_count;
} ExecutionContext;

ExecutionContext *context_create(void);
void context_destroy(ExecutionContext *ctx);

#endif
