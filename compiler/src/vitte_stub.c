#include "vitte/codegen.h"
#include <string.h>

void vitte_ctx_init(vitte_ctx* ctx) {
  if (!ctx) return;
  ctx->reserved = 0;
}

void vitte_ctx_free(vitte_ctx* ctx) {
  (void)ctx;
}

void vitte_ast_free(vitte_ctx* ctx, vitte_ast* ast) {
  (void)ctx;
  (void)ast;
}

void vitte_codegen_unit_init(vitte_codegen_unit* unit) {
  if (!unit) return;
  memset(unit, 0, sizeof(*unit));
}

void vitte_codegen_unit_reset(vitte_ctx* ctx, vitte_codegen_unit* unit) {
  (void)ctx;
  if (!unit) return;
  vitte_codegen_unit_init(unit);
}

vitte_result vitte_codegen_unit_build(vitte_ctx* ctx,
                                      const char* src,
                                      size_t len,
                                      vitte_codegen_unit* out,
                                      vitte_error* err) {
  (void)ctx;
  (void)src;
  (void)len;
  (void)out;
  if (err) {
    err->code = VITTE_ERRC_NONE;
    err->line = 0;
    err->col = 0;
    err->message[0] = '\0';
  }
  return VITTE_OK;
}
