// fuzz_vittec_parser_toplevel.c
// Fuzz target: vittec bootstrap top-level parser invariants.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

#include "vittec/front/lexer.h"
#include "vittec/front/parser.h"

static void check_span_in_bounds(vittec_span_t sp, uint32_t len) {
  FUZZ_ASSERT(sp.lo <= sp.hi);
  FUZZ_ASSERT(sp.hi <= len);
}

static void check_unit_invariants(const vittec_parse_unit_t* u, uint32_t len) {
  FUZZ_ASSERT(u);

  FUZZ_ASSERT(u->items_len <= u->items_cap);
  FUZZ_ASSERT(u->imports_len <= u->imports_cap);
  FUZZ_ASSERT(u->exports_len <= u->exports_cap);
  FUZZ_ASSERT(u->fns_len <= u->fns_cap);

  if (u->items) {
    for (uint32_t i = 0; i < u->items_len; ++i)
      check_span_in_bounds(u->items[i].span, len);
  }
  if (u->imports) {
    for (uint32_t i = 0; i < u->imports_len; ++i)
      check_span_in_bounds(u->imports[i].span, len);
  }
  if (u->exports) {
    for (uint32_t i = 0; i < u->exports_len; ++i)
      check_span_in_bounds(u->exports[i].span, len);
  }
  if (u->fns) {
    for (uint32_t i = 0; i < u->fns_len; ++i) {
      check_span_in_bounds(u->fns[i].name_span, len);
      check_span_in_bounds(u->fns[i].sig_span, len);
      check_span_in_bounds(u->fns[i].body_span, len);
    }
  }

  check_span_in_bounds(u->module_span, len);
}

FUZZ_TARGET(fuzz_vittec_parser_toplevel_target) {
  fuzz_reader r = fuzz_target_reader(data, size);
  uint8_t mode = fuzz_target_mode(&r);

  size_t cap = size;
  if (cap > (1u << 20))
    cap = (1u << 20);

  uint8_t* src = (uint8_t*)malloc(cap ? cap : 1);
  if (!src)
    return 0;
  if (cap)
    memcpy(src, data, cap);

  vittec_lexer_options_t lxopt;
  vittec_lexer_options_init(&lxopt);
  // Parser expects NEWLINE tokens for stmt boundaries; keep them by default.
  lxopt.flags |= VITTEC_LEX_KEEP_NEWLINES;
  if (mode & 1u) lxopt.flags |= VITTEC_LEX_KEEP_COMMENTS;
  if (mode & 2u) lxopt.flags |= VITTEC_LEX_ACCEPT_CRLF;
  if (mode & 4u) lxopt.flags |= VITTEC_LEX_ACCEPT_TABS;

  vittec_lexer_t lx;
  vittec_lexer_init_ex(&lx, src, (uint32_t)cap, 0u, NULL, NULL, &lxopt);

  vittec_parse_options_t popt;
  vittec_parse_options_init(&popt);
  popt.flags |= VITTEC_PARSE_RECOVER | VITTEC_PARSE_CAPTURE_BODIES;
  // Hard safety caps to keep runtime bounded under arbitrary inputs.
  popt.max_top_items = (mode & 8u) ? 1024u : 256u;
  popt.max_params = (mode & 16u) ? 2048u : 512u;

  vittec_parse_unit_t unit;
  vittec_parse_unit_init(&unit);

  // Invariant: should never crash; always safe to free.
  (void)vittec_parse_unit_ex(&lx, &unit, &popt);

  check_unit_invariants(&unit, (uint32_t)cap);

  vittec_parse_unit_free(&unit);
  free(src);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_vittec_parser_toplevel_target
#include "fuzz/fuzz_driver.h"

