// fuzz_typecheck.c
// Fuzz target: typecheck placeholder.
//
// There is no full typechecker in the lightweight vitte C frontend yet, so we
// exercise the full parse + desugar + codegen indexing pipeline as a stand-in.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"

#include "vitte/codegen.h"
#include "vitte/diag.h"
#include "vitte/vitte.h"

FUZZ_TARGET(fuzz_typecheck_target) {
  size_t cap = size;
  if (cap > (1u << 20))
    cap = (1u << 20);

  char* src = (char*)malloc(cap + 1);
  if (!src)
    return 0;
  if (cap)
    memcpy(src, data, cap);
  src[cap] = '\0';

  vitte_ctx ctx;
  vitte_ctx_init(&ctx);
  vitte_diag_bag diags;
  vitte_diag_bag_init(&diags);

  vitte_codegen_unit unit;
  vitte_codegen_unit_init(&unit);
  (void)vitte_codegen_unit_build(&ctx, 0u, src, cap, &unit, &diags);

  if (unit.module_count > 0) {
    FUZZ_ASSERT(unit.modules != NULL);
  }
  if (unit.function_count > 0) {
    FUZZ_ASSERT(unit.functions != NULL);
  }

  vitte_codegen_unit_reset(&ctx, &unit);
  vitte_diag_bag_free(&diags);
  vitte_ctx_free(&ctx);
  free(src);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_typecheck_target
#include "fuzz/fuzz_driver.h"
