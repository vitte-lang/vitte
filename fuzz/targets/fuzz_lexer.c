// fuzz_lexer.c
// Fuzz target: Vitte phrase lexer (include/vitte/lexer.h).
//
// This harness is built as a standalone binary by fuzz/scripts/build_targets.sh
// once FUZZ_DRIVER_TARGET is defined.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

#include "vitte/diag.h"
#include "vitte/lexer.h"
#include "vitte/vitte.h"

FUZZ_TARGET(fuzz_lexer_target) {
  fuzz_reader r = fuzz_target_reader(data, size);
  (void)fuzz_target_mode(&r);

  // Ensure NUL-terminated for lexer convenience (it uses explicit length).
  size_t cap = size;
  if (cap > (1u << 20))
    cap = (1u << 20); // keep allocations bounded

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

  vitte_token* toks = NULL;
  size_t tok_count = 0;
  vitte_result lr = vitte_lex_all(&ctx, 0u, src, cap, &toks, &tok_count, &diags);

  if (lr == VITTE_OK && toks && tok_count) {
    FUZZ_ASSERT(toks[tok_count - 1].kind == VITTE_TOK_EOF);
    // Span monotonicity (weak check): last token should not go backwards.
    FUZZ_ASSERT(toks[tok_count - 1].span.hi >= toks[tok_count - 1].span.lo);
  }

  free(toks);
  vitte_diag_bag_free(&diags);
  vitte_ctx_free(&ctx);
  free(src);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_lexer_target
#include "fuzz/fuzz_driver.h"
