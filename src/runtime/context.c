#include "../../include/runtime/context.h"
#include <stdlib.h>

ExecutionContext *context_create(void) {
    ExecutionContext *ctx = malloc(sizeof(ExecutionContext));
    ctx->stack = stack_create(1024);
    ctx->gc = gc_create();
    ctx->variables = malloc(sizeof(Value) * 256);
    ctx->var_count = 0;
    return ctx;
}

void context_destroy(ExecutionContext *ctx) {
    stack_destroy(ctx->stack);
    gc_destroy(ctx->gc);
    free(ctx->variables);
    free(ctx);
}
