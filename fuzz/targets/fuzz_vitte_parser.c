// fuzz_vitte_parser.c
// Fuzz target: "vitte parser" (phrase parser + extra stress paths).

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

#include "vitte/diag.h"
#include "vitte/parser_phrase.h"
#include "vitte/vitte.h"

FUZZ_TARGET(fuzz_vitte_parser_target) {
  fuzz_reader rr = fuzz_target_reader(data, size);
  uint8_t mode = fuzz_target_mode(&rr);

  size_t cap = size;
  if (cap > (1u << 20))
    cap = (1u << 20);

  char* src = (char*)malloc(cap + 1);
  if (!src)
    return 0;
  if (cap)
    memcpy(src, data, cap);
  src[cap] = '\0';

  const char* prefix = (mode & 1u) ? "mod fuzz.demo\n" : "";
  size_t pre_len = strlen(prefix);
  char* buf = src;
  size_t buf_len = cap;
  if (pre_len) {
    char* merged = (char*)malloc(pre_len + cap + 1);
    if (merged) {
      memcpy(merged, prefix, pre_len);
      if (cap)
        memcpy(merged + pre_len, src, cap);
      merged[pre_len + cap] = '\0';
      buf = merged;
      buf_len = pre_len + cap;
    }
  }

  vitte_ctx ctx;
  vitte_ctx_init(&ctx);
  vitte_diag_bag diags;
  vitte_diag_bag_init(&diags);

  vitte_ast* ast = NULL;
  (void)vitte_parse_phrase(&ctx, 0u, buf, buf_len, &ast, &diags);
  if (ast)
    vitte_ast_free(&ctx, ast);

  vitte_diag_bag_free(&diags);
  vitte_ctx_free(&ctx);
  if (buf != src)
    free(buf);
  free(src);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_vitte_parser_target
#include "fuzz/fuzz_driver.h"
