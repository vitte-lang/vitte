#include "vittec/front/parser.h"
#include "vittec/support/str.h"

static int tok_is(vittec_token_t t, vittec_token_kind_t k) { return t.kind == k; }

int vittec_parse_unit(vittec_lexer_t* lx, vittec_parse_unit_t* out) {
  out->has_main = 0;

  vittec_token_t t = vittec_lex_next(lx);
  while (!tok_is(t, TK_EOF)) {
    /* detect: fn main */
    if (tok_is(t, TK_KW_FN)) {
      vittec_token_t name = vittec_lex_next(lx);
      if (tok_is(name, TK_IDENT)) {
        /* compare text to "main" */
        vittec_sv_t s = name.text;
        if (s.len == 4 && s.data[0]=='m' && s.data[1]=='a' && s.data[2]=='i' && s.data[3]=='n') {
          out->has_main = 1;
        }
      }

      /* naive: skip tokens until `.end` (TK_KW_END) */
      for (;;) {
        vittec_token_t u = vittec_lex_next(lx);
        if (tok_is(u, TK_EOF)) break;
        if (tok_is(u, TK_KW_END)) break;
      }
      t = vittec_lex_next(lx);
      continue;
    }

    t = vittec_lex_next(lx);
  }

  return 0;
}
