#include "compiler/compiler.h"
#include <stdlib.h>

compiler_ctx_t* compiler_create(void) {
    compiler_ctx_t *ctx = (compiler_ctx_t *)malloc(sizeof(compiler_ctx_t));
    if (!ctx) return NULL;
    
    ctx->alloc = NULL;
    ctx->ast_module = NULL;
    ctx->hir_module = NULL;
    ctx->ir_module = NULL;
    ctx->type_table = NULL;
    
    return ctx;
}

void compiler_destroy(compiler_ctx_t *ctx) {
    if (!ctx) return;
    free(ctx);
}
