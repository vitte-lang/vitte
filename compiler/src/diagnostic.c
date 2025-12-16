#include "compiler/diagnostic.h"
#include <stdlib.h>
#include <string.h>

diagnostic_ctx_t* diagnostic_ctx_create(void) {
    diagnostic_ctx_t *ctx = (diagnostic_ctx_t *)malloc(sizeof(diagnostic_ctx_t));
    if (!ctx) return NULL;
    
    ctx->capacity = 64;
    ctx->count = 0;
    ctx->diags = (diagnostic_t **)calloc(ctx->capacity, sizeof(diagnostic_t *));
    
    if (!ctx->diags) {
        free(ctx);
        return NULL;
    }
    
    return ctx;
}

void diagnostic_ctx_destroy(diagnostic_ctx_t *ctx) {
    if (!ctx) return;
    free(ctx->diags);
    free(ctx);
}

void diagnostic_emit(diagnostic_ctx_t *ctx, diagnostic_level_t level,
                     const char *message, uint32_t line, uint32_t col) {
    if (!ctx || !message) return;
    
    if (ctx->count >= ctx->capacity) {
        ctx->capacity *= 2;
        ctx->diags = (diagnostic_t **)realloc(ctx->diags, ctx->capacity * sizeof(diagnostic_t *));
    }
    
    diagnostic_t *d = (diagnostic_t *)malloc(sizeof(diagnostic_t));
    if (!d) return;
    
    d->level = level;
    d->message = message;
    d->line = line;
    d->col = col;
    d->source_file = NULL;
    
    ctx->diags[ctx->count++] = d;
}
