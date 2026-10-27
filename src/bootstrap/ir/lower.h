#ifndef VITTE_IR_LOWER_H
#define VITTE_IR_LOWER_H

#include "ir.h"
#include "hir/hir.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lower a sealed HIR module into a sealed, validated IR context. */
bool vitte_ir_lower_hir(vitte_ir_context_t *ir, const vitte_hir_t *hir);

#ifdef __cplusplus
}
#endif

#endif
