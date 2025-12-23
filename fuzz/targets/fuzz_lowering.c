// fuzz_lowering.c
// Fuzz target: phrase -> core desugaring + codegen unit build.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"

#include "vitte/codegen.h"
#include "vitte/desugar_phrase.h"
#include "vitte/diag.h"
#include "vitte/parser_phrase.h"
#include "vitte/vitte.h"

FUZZ_TARGET(fuzz_lowering_target) {
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

  // Phase 1: parse phrase
  vitte_diag_bag diags;
  vitte_diag_bag_init(&diags);
  vitte_ast* phrase = NULL;
  vitte_result pr = vitte_parse_phrase(&ctx, 0u, src, cap, &phrase, &diags);
  if (pr == VITTE_OK && phrase) {
    // Phase 2: desugar to core
    vitte_ast* core = NULL;
    vitte_result dr = vitte_desugar_phrase(&ctx, phrase, &core, &diags);
    if (dr == VITTE_OK) {
      FUZZ_ASSERT(core != NULL);
      FUZZ_ASSERT(core->kind == VITTE_AST_CORE_UNIT);
    }
    if (core)
      vitte_ast_free(&ctx, core);
    vitte_ast_free(&ctx, phrase);
  } else if (phrase) {
    vitte_ast_free(&ctx, phrase);
  }

  // Phase 3: full unit build (parse + desugar + indexing). This is cheap and
  // catches cross-phase memory issues.
  vitte_codegen_unit unit;
  vitte_codegen_unit_init(&unit);
  (void)vitte_codegen_unit_build(&ctx, 0u, src, cap, &unit, &diags);
  vitte_codegen_unit_reset(&ctx, &unit);

  vitte_diag_bag_free(&diags);
  vitte_ctx_free(&ctx);
  free(src);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_lowering_target
#include "fuzz/fuzz_driver.h"
