#include "vitte/vitte.h"
#include <stdlib.h>

void vitte_ctx_init(vitte_ctx* ctx) {
    if (ctx) {
        ctx->reserved = 0;
    }
}

void vitte_ctx_free(vitte_ctx* ctx) {
    (void)ctx;
}

static void vitte_ast_free_list(vitte_ast* node) {
    vitte_ast* it = node;
    while (it) {
        vitte_ast* next = it->next;
        if (it->first_child) {
            vitte_ast_free_list(it->first_child);
        }
        free(it->text);
        free(it->aux_text);
        free(it);
        it = next;
    }
}

void vitte_ast_free(vitte_ctx* ctx, vitte_ast* ast) {
    (void)ctx;
    vitte_ast_free_list(ast);
}
