// fuzz_vittec_lexer_roundtrip.c
// Fuzz target: vittec front-end lexer roundtrip invariants.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

#include "vittec/front/lexer.h"
#include "vittec/front/token.h"

static int is_dot_end_token(const vittec_token_t t) {
  if (t.kind != VITTEC_TOK_KEYWORD)
    return 0;
  if (t.as.kw != VITTEC_KW_END)
    return 0;
  if ((t.flags & VITTEC_TOKF_FROM_DOT_END) == 0u)
    return 0;
  return t.text.data && t.text.len == 4u && t.text.data[0] == '.' && t.text.data[1] == 'e' && t.text.data[2] == 'n' && t.text.data[3] == 'd';
}

FUZZ_TARGET(fuzz_vittec_lexer_roundtrip_target) {
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

  vittec_lexer_options_t opt;
  vittec_lexer_options_init(&opt);
  if (mode & 1u) opt.flags |= VITTEC_LEX_KEEP_NEWLINES;
  if (mode & 2u) opt.flags |= VITTEC_LEX_KEEP_COMMENTS;
  if (mode & 4u) opt.flags |= VITTEC_LEX_ACCEPT_CRLF;
  if (mode & 8u) opt.flags |= VITTEC_LEX_ACCEPT_TABS;
  opt.max_token_bytes = (mode & 16u) ? 4096u : 0u;
  opt.max_string_bytes = (mode & 32u) ? 4096u : 0u;

  vittec_lexer_t lx;
  vittec_lexer_init_ex(&lx, src, (uint32_t)cap, 0u, NULL, NULL, &opt);

  uint32_t prev_hi = 0u;
  uint32_t prev_i = 0u;

  // Safety cap against accidental infinite loops: a healthy lexer should
  // produce O(len) tokens.
  const uint32_t kMaxTokens = (uint32_t)((cap < 1024u) ? 4096u : (cap * 8u));
  for (uint32_t n = 0; n < kMaxTokens; ++n) {
    prev_i = lx.i;
    vittec_token_t t = vittec_lex_next(&lx);

    FUZZ_ASSERT(t.span.lo <= t.span.hi);
    FUZZ_ASSERT(t.span.hi <= (uint32_t)cap);
    FUZZ_ASSERT(t.span.lo >= prev_hi); // monotone non-decreasing stream
    prev_hi = t.span.hi;

    // `.end` must be flagged (and the reverse: flagged tokens must be `.end`).
    if (t.text.data && t.text.len >= 4u && t.text.data[0] == '.' && t.text.len == 4u &&
        t.text.data[1] == 'e' && t.text.data[2] == 'n' && t.text.data[3] == 'd') {
      FUZZ_ASSERT(is_dot_end_token(t));
    }
    if (t.flags & VITTEC_TOKF_FROM_DOT_END) {
      FUZZ_ASSERT(is_dot_end_token(t));
    }

    // Progress: lexer index must move unless we reached EOF.
    if (t.kind == VITTEC_TOK_EOF) {
      FUZZ_ASSERT(lx.i == lx.len);
      break;
    }
    FUZZ_ASSERT(lx.i > prev_i || lx.i == lx.len);
  }

  free(src);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_vittec_lexer_roundtrip_target
#include "fuzz/fuzz_driver.h"

