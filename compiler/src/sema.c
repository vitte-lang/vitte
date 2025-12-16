#include "compiler/sema.h"
#include <stdlib.h>

sema_ctx_t* sema_create(void) {
    sema_ctx_t *ctx = (sema_ctx_t *)malloc(sizeof(sema_ctx_t));
    if (!ctx) return NULL;
    
    ctx->type_table = NULL;
    ctx->symbol_table = NULL;
    ctx->diag = NULL;
    
    return ctx;
}

void sema_destroy(sema_ctx_t *ctx) {
    if (!ctx) return;
    free(ctx);
}

int sema_analyze(sema_ctx_t *ctx, void *ast) {
    if (!ctx || !ast) return -1;
    
    // TODO: Implement semantic analysis
    
    return 0;
}
