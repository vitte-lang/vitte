#include "compiler/lowering.h"
#include <stdlib.h>

lowering_ctx_t* lowering_ctx_create(void) {
    lowering_ctx_t *ctx = (lowering_ctx_t *)malloc(sizeof(lowering_ctx_t));
    if (!ctx) return NULL;
    
    ctx->type_table = NULL;
    ctx->target_info = NULL;
    
    return ctx;
}

void lowering_ctx_destroy(lowering_ctx_t *ctx) {
    if (!ctx) return;
    free(ctx);
}

void* lower_hir_to_ir(lowering_ctx_t *ctx, void *hir) {
    if (!ctx || !hir) return NULL;
    
    // TODO: Implement HIR to IR lowering
    return NULL;
}
