// fuzz_parser.c
// Fuzz target: Vitte phrase parser (include/vitte/parser_phrase.h).

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

#include "vitte/diag.h"
#include "vitte/parser_phrase.h"
#include "vitte/vitte.h"

FUZZ_TARGET(fuzz_parser_target) {
  fuzz_reader r = fuzz_target_reader(data, size);
  uint8_t mode = fuzz_target_mode(&r);

  size_t cap = size;
  if (cap > (1u << 20))
    cap = (1u << 20);

  char* src = (char*)malloc(cap + 1);
  if (!src)
    return 0;
  if (cap)
    memcpy(src, data, cap);
  src[cap] = '\0';

  // Optionally ensure trailing newline to hit more "StmtEnd" paths.
  if ((mode & 1u) && cap > 0 && src[cap - 1] != '\n') {
    // best-effort: if we have room to grow, do it; otherwise leave.
    char* grown = (char*)realloc(src, cap + 2);
    if (grown) {
      src = grown;
      src[cap] = '\n';
      src[cap + 1] = '\0';
      cap += 1;
    }
  }

  vitte_ctx ctx;
  vitte_ctx_init(&ctx);
  vitte_diag_bag diags;
  vitte_diag_bag_init(&diags);

  vitte_ast* ast = NULL;
  vitte_result pr = vitte_parse_phrase(&ctx, 0u, src, cap, &ast, &diags);

  if (pr == VITTE_OK) {
    FUZZ_ASSERT(ast != NULL);
    FUZZ_ASSERT(!vitte_diag_bag_has_errors(&diags));
    FUZZ_ASSERT(ast->kind == VITTE_AST_PHR_UNIT);
  } else if (pr == VITTE_ERR_PARSE) {
    // Contract: parse errors should be accompanied by diagnostics.
    FUZZ_ASSERT(vitte_diag_bag_has_errors(&diags));
  }

  if (ast)
    vitte_ast_free(&ctx, ast);
  vitte_diag_bag_free(&diags);
  vitte_ctx_free(&ctx);
  free(src);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_parser_target
#include "fuzz/fuzz_driver.h"
