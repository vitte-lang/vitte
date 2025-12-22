// fuzz_diag_json.c
// Fuzz target: JSON diagnostic emitter (src/vitte/diag.c).

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

#include "vitte/diag.h"
#include "vitte/parser_phrase.h"
#include "vitte/vitte.h"

FUZZ_TARGET(fuzz_diag_json_target) {
  fuzz_reader r = fuzz_target_reader(data, size);
  (void)fuzz_target_mode(&r);

  size_t cap = size;
  if (cap > (1u << 20))
    cap = (1u << 20);

  char* src_buf = (char*)malloc(cap + 1);
  if (!src_buf)
    return 0;
  if (cap)
    memcpy(src_buf, data, cap);
  src_buf[cap] = '\0';

  vitte_ctx ctx;
  vitte_ctx_init(&ctx);

  vitte_diag_bag diags;
  vitte_diag_bag_init(&diags);
  vitte_ast* ast = NULL;
  (void)vitte_parse_phrase(&ctx, 0u, src_buf, cap, &ast, &diags);
  if (ast)
    vitte_ast_free(&ctx, ast);

  vitte_source src;
  src.file_id = 0u;
  src.path = "<fuzz>";
  src.data = src_buf;
  src.len = (uint32_t)cap;

  vitte_emit_options opt;
  vitte_emit_options_init(&opt);
  opt.json_one_per_line = 1;
  opt.json_pretty = 0;

  FILE* f = tmpfile();
  if (f) {
    vitte_emit_json(f, &src, &diags, &opt);
    fflush(f);
    fclose(f);
  }

  vitte_diag_bag_free(&diags);
  vitte_ctx_free(&ctx);
  free(src_buf);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_diag_json_target
#include "fuzz/fuzz_driver.h"

