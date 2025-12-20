#include "vitte/codegen.h"
#include <string.h>
#include <stdlib.h>

void vitte_diag_bag_init(vitte_diag_bag* b) {
  if (!b) return;
  memset(b, 0, sizeof(*b));
}

void vitte_diag_bag_free(vitte_diag_bag* b) {
  if (!b) return;
  free(b->diags);
  memset(b, 0, sizeof(*b));
}

int vitte_diag_bag_has_errors(const vitte_diag_bag* b) {
  return b && b->errors != 0;
}

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
                                      vitte_file_id file_id,
                                      const char* src,
                                      size_t len,
                                      vitte_codegen_unit* out,
                                      vitte_diag_bag* diags) {
  (void)ctx;
  (void)file_id;
  (void)src;
  (void)len;
  (void)out;
  (void)diags;
  return VITTE_OK;
}
