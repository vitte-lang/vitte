// fuzz_parser_recovery.c
// Fuzz target: parser recovery / "try again" behavior.
//
// The main invariant here is: parse must not crash, and a failed parse must not
// poison subsequent parses in the same process.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"

#include "vitte/diag.h"
#include "vitte/parser_phrase.h"
#include "vitte/vitte.h"

static void parse_once(vitte_ctx* ctx, const char* src, size_t len) {
  vitte_diag_bag diags;
  vitte_diag_bag_init(&diags);
  vitte_ast* ast = NULL;

  vitte_result r = vitte_parse_phrase(ctx, 0u, src, len, &ast, &diags);
  if (r == VITTE_OK) {
    FUZZ_ASSERT(ast != NULL);
    FUZZ_ASSERT(!vitte_diag_bag_has_errors(&diags));
  } else if (r == VITTE_ERR_PARSE) {
    FUZZ_ASSERT(vitte_diag_bag_has_errors(&diags));
  }

  if (ast)
    vitte_ast_free(ctx, ast);
  vitte_diag_bag_free(&diags);
}

FUZZ_TARGET(fuzz_parser_recovery_target) {
  // Keep allocations bounded.
  size_t cap = size;
  if (cap > (1u << 20))
    cap = (1u << 20);

  char* src = (char*)malloc(cap + 1);
  if (!src)
    return 0;
  if (cap)
    memcpy(src, data, cap);
  src[cap] = '\0';

  // Build a second input variant to exercise sync paths:
  // append a newline and ".end\n" (may turn some errors into successes).
  static const char k_suffix[] = "\n.end\n";
  size_t cap2 = cap + (sizeof(k_suffix) - 1);
  if (cap2 > (2u << 20))
    cap2 = (2u << 20);

  char* src2 = (char*)malloc(cap2 + 1);
  if (!src2) {
    free(src);
    return 0;
  }
  size_t copy2 = cap;
  if (copy2 > cap2)
    copy2 = cap2;
  if (copy2)
    memcpy(src2, src, copy2);
  size_t suf_len = sizeof(k_suffix) - 1;
  size_t remain = cap2 - copy2;
  if (remain > 0) {
    size_t take = remain < suf_len ? remain : suf_len;
    memcpy(src2 + copy2, k_suffix, take);
    copy2 += take;
  }
  src2[copy2] = '\0';

  vitte_ctx ctx;
  vitte_ctx_init(&ctx);

  // Parse twice in a row; order depends on first byte to vary deterministically.
  if (cap && ((uint8_t)src[0] & 1u)) {
    parse_once(&ctx, src2, copy2);
    parse_once(&ctx, src, cap);
  } else {
    parse_once(&ctx, src, cap);
    parse_once(&ctx, src2, copy2);
  }

  vitte_ctx_free(&ctx);
  free(src2);
  free(src);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_parser_recovery_target
#include "fuzz/fuzz_driver.h"
