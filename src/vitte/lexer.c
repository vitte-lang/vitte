#include "steel/vitte/lexer.h"
#include <string.h>

static int is_ws(char c) { return c==' ' || c=='\t' || c=='\n' || c=='\r'; }
static int is_alpha(char c) { return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'; }
static int is_digit(char c) { return c>='0'&&c<='9'; }
static int is_alnum(char c) { return is_alpha(c)||is_digit(c); }

void vitte_lexer_init(vitte_lexer* lx, const char* src, uint32_t len, uint32_t file_id) {
  lx->src = src; lx->len = len; lx->i = 0; lx->file_id = file_id;
}

static char peek(vitte_lexer* lx) { return (lx->i < lx->len) ? lx->src[lx->i] : 0; }
static char nextc(vitte_lexer* lx) { return (lx->i < lx->len) ? lx->src[lx->i++] : 0; }

static void skip_ws_and_comments(vitte_lexer* lx) {
  for (;;) {
    while (is_ws(peek(lx))) nextc(lx);
    if (peek(lx) == '#') { while (peek(lx) && peek(lx) != '\n') nextc(lx); continue; }
    break;
  }
}

static vitte_token tok(vitte_tok_kind k, uint32_t lo, uint32_t hi, const char* lex, uint32_t len, int64_t iv) {
  vitte_token t;
  t.kind = k;
  t.span = (steel_span){0, lo, hi};
  t.lexeme = lex;
  t.len = len;
  t.int_val = iv;
  return t;
}

vitte_token vitte_lex_next(vitte_lexer* lx) {
  skip_ws_and_comments(lx);
  uint32_t lo = lx->i;
  char c = peek(lx);
  if (!c) return tok(TK_EOF, lo, lo, lx->src + lo, 0, 0);

  /* punctuation */
  if (c == ':') { nextc(lx); return tok(TK_COLON, lo, lx->i, lx->src+lo, 1, 0); }
  if (c == '.') { nextc(lx); return tok(TK_DOT, lo, lx->i, lx->src+lo, 1, 0); }
  if (c == '(') { nextc(lx); return tok(TK_LPAREN, lo, lx->i, lx->src+lo, 1, 0); }
  if (c == ')') { nextc(lx); return tok(TK_RPAREN, lo, lx->i, lx->src+lo, 1, 0); }
  if (c == ';') { nextc(lx); return tok(TK_SEMI, lo, lx->i, lx->src+lo, 1, 0); }

  if (c == '-' && lx->i + 1 < lx->len && lx->src[lx->i+1] == '>') {
    lx->i += 2;
    return tok(TK_ARROW, lo, lx->i, lx->src+lo, 2, 0);
  }

  /* int */
  if (is_digit(c)) {
    int64_t v = 0;
    while (is_digit(peek(lx))) { v = v*10 + (nextc(lx)-'0'); }
    uint32_t hi = lx->i;
    return tok(TK_INT, lo, hi, lx->src+lo, hi-lo, v);
  }

  /* ident/kw */
  if (is_alpha(c)) {
    nextc(lx);
    while (is_alnum(peek(lx))) nextc(lx);
    uint32_t hi = lx->i;
    const char* s = lx->src + lo;
    uint32_t n = hi - lo;
    if (n==6 && strncmp(s,"module",6)==0) return tok(TK_KW_MODULE, lo, hi, s, n, 0);
    if (n==2 && strncmp(s,"fn",2)==0) return tok(TK_KW_FN, lo, hi, s, n, 0);
    if (n==6 && strncmp(s,"return",6)==0) return tok(TK_KW_RETURN, lo, hi, s, n, 0);
    return tok(TK_IDENT, lo, hi, s, n, 0);
  }

  /* unknown */
  nextc(lx);
  return tok(TK_EOF, lo, lx->i, lx->src+lo, 1, 0);
}
