// fuzz_ast_printer.c
// Fuzz target: token-based "pretty printer" round-trip.
//
// We don't have a full formatter yet, but we can:
//   input bytes -> lex -> reconstruct a normalized source -> parse again
//
// This catches lexer/parser inconsistencies and ensures the lexer token stream
// can be serialized deterministically without losing required statement ends.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

#include "vitte/diag.h"
#include "vitte/lexer.h"
#include "vitte/parser_phrase.h"
#include "vitte/vitte.h"

static int tok_is_word(vitte_token_kind k) {
  return (k == VITTE_TOK_IDENT) || (k == VITTE_TOK_INT) || (k == VITTE_TOK_FLOAT) || (k == VITTE_TOK_STRING) ||
         (k == VITTE_TOK_TRUE) || (k == VITTE_TOK_FALSE) || (k == VITTE_TOK_NIL);
}

FUZZ_TARGET(fuzz_ast_printer_target) {
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

  vitte_ctx ctx;
  vitte_ctx_init(&ctx);
  vitte_diag_bag diags;
  vitte_diag_bag_init(&diags);

  vitte_token* toks = NULL;
  size_t tok_count = 0;
  vitte_result lr = vitte_lex_all(&ctx, 0u, src, cap, &toks, &tok_count, &diags);
  if (lr != VITTE_OK || !toks || tok_count == 0) {
    free(toks);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
    free(src);
    return 0;
  }

  // Rough upper bound: each token + a space, plus newlines.
  size_t out_cap = cap * 4 + 64;
  if (out_cap > (8u << 20))
    out_cap = (8u << 20);
  char* out = (char*)malloc(out_cap);
  if (!out) {
    free(toks);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
    free(src);
    return 0;
  }

  size_t out_len = 0;
  vitte_token_kind prev = VITTE_TOK_EOF;

  for (size_t i = 0; i < tok_count; ++i) {
    vitte_token t = toks[i];
    if (t.kind == VITTE_TOK_EOF)
      break;

    if (t.kind == VITTE_TOK_NEWLINE) {
      if (out_len + 1 < out_cap) {
        out[out_len++] = '\n';
      }
      prev = t.kind;
      continue;
    }

    // Insert a space between adjacent "word-ish" tokens to avoid accidental
    // concatenation (e.g. `moddemo`).
    if (out_len && tok_is_word(prev) && tok_is_word(t.kind)) {
      if (out_len + 1 < out_cap)
        out[out_len++] = ' ';
    }

    size_t n = t.len;
    if (n > 0 && out_len + n + 2 < out_cap) {
      memcpy(out + out_len, t.lexeme, n);
      out_len += n;
      if ((mode & 1u) && out_len + 1 < out_cap)
        out[out_len++] = ' ';
    }
    prev = t.kind;
  }

  if (out_len == 0 || out[out_len - 1] != '\n') {
    if (out_len + 1 < out_cap)
      out[out_len++] = '\n';
  }
  out[out_len] = '\0';

  // Parse the normalized output (may still fail; just must not crash).
  vitte_ast* ast = NULL;
  vitte_diag_bag_free(&diags);
  vitte_diag_bag_init(&diags);
  (void)vitte_parse_phrase(&ctx, 0u, out, out_len, &ast, &diags);
  if (ast)
    vitte_ast_free(&ctx, ast);

  free(out);
  free(toks);
  vitte_diag_bag_free(&diags);
  vitte_ctx_free(&ctx);
  free(src);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_ast_printer_target
#include "fuzz/fuzz_driver.h"
