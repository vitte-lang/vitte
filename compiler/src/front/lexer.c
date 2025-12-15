#include "vittec/front/lexer.h"
#include "vittec/support/str.h"
#include "vittec/support/assert.h"

static int is_space(char c) { return c==' '||c=='\t'||c=='\n'||c=='\r'; }
static int is_alpha(char c) { return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'; }
static int is_digit(char c) { return (c>='0'&&c<='9'); }

static vittec_token_kind_t kw_kind(vittec_sv_t s) {
  #define KW(x, k) do { \
    const char* t = (x); \
    uint64_t n = 0; while (t[n]) n++; \
    if (s.len == n) { \
      int ok = 1; \
      for (uint64_t i = 0; i < n; i++) if (s.data[i] != t[i]) { ok = 0; break; } \
      if (ok) return (k); \
    } \
  } while (0)

  KW("module", TK_KW_MODULE);
  KW("import", TK_KW_IMPORT);
  KW("export", TK_KW_EXPORT);
  KW("struct", TK_KW_STRUCT);
  KW("enum", TK_KW_ENUM);
  KW("type", TK_KW_TYPE);
  KW("fn", TK_KW_FN);
  KW("let", TK_KW_LET);
  KW("const", TK_KW_CONST);
  KW("if", TK_KW_IF);
  KW("elif", TK_KW_ELIF);
  KW("else", TK_KW_ELSE);
  KW("while", TK_KW_WHILE);
  KW("for", TK_KW_FOR);
  KW("match", TK_KW_MATCH);
  KW("break", TK_KW_BREAK);
  KW("continue", TK_KW_CONTINUE);
  KW("ret", TK_KW_RET);
  KW("say", TK_KW_SAY);
  KW("do", TK_KW_DO);
  KW("set", TK_KW_SET);
  KW("loop", TK_KW_LOOP);
  KW("when", TK_KW_WHEN);
  #undef KW
  return TK_IDENT;
}

void vittec_lexer_init(vittec_lexer_t* lx, const char* src, uint32_t len, uint32_t file_id, vittec_diag_sink_t* diags) {
  lx->src = src;
  lx->len = len;
  lx->i = 0;
  lx->file_id = file_id;
  lx->diags = diags;
}

static char peek(const vittec_lexer_t* lx) {
  if (lx->i >= lx->len) return 0;
  return lx->src[lx->i];
}

static char bump(vittec_lexer_t* lx) {
  if (lx->i >= lx->len) return 0;
  return lx->src[lx->i++];
}

static void skip_ws_and_comments(vittec_lexer_t* lx) {
  for (;;) {
    while (is_space(peek(lx))) bump(lx);

    /* line comment: # ... */
    if (peek(lx) == '#') {
      while (peek(lx) && peek(lx) != '\n') bump(lx);
      continue;
    }

    /* line comment: // ... */
    if (peek(lx) == '/' && lx->i + 1 < lx->len && lx->src[lx->i + 1] == '/') {
      bump(lx); bump(lx);
      while (peek(lx) && peek(lx) != '\n') bump(lx);
      continue;
    }

    break;
  }
}

vittec_token_t vittec_lex_next(vittec_lexer_t* lx) {
  skip_ws_and_comments(lx);

  uint32_t start = lx->i;
  char c = peek(lx);

  vittec_token_t t;
  t.span.file_id = lx->file_id;
  t.span.lo = start;
  t.span.hi = start;
  t.text = vittec_sv(lx->src + start, 0);
  t.kind = TK_EOF;

  if (!c) {
    t.kind = TK_EOF;
    return t;
  }

  /* .end special */
  if (c == '.' && lx->i + 3 < lx->len &&
      lx->src[lx->i+1] == 'e' &&
      lx->src[lx->i+2] == 'n' &&
      lx->src[lx->i+3] == 'd') {
    lx->i += 4;
    t.kind = TK_KW_END;
    t.span.hi = lx->i;
    t.text = vittec_sv(lx->src + start, t.span.hi - t.span.lo);
    return t;
  }

  /* punct */
  if (c == '.') { bump(lx); t.kind = TK_DOT; }
  else if (c == ',') { bump(lx); t.kind = TK_COMMA; }
  else if (c == ':') { bump(lx); t.kind = TK_COLON; }
  else if (c == ';') { bump(lx); t.kind = TK_SEMI; }
  else if (c == '(') { bump(lx); t.kind = TK_LPAREN; }
  else if (c == ')') { bump(lx); t.kind = TK_RPAREN; }
  else if (c == '[') { bump(lx); t.kind = TK_LBRACK; }
  else if (c == ']') { bump(lx); t.kind = TK_RBRACK; }
  else if (c == '=' ) { bump(lx); t.kind = TK_EQ; }
  else if (c == '-' && lx->i + 1 < lx->len && lx->src[lx->i+1] == '>') { bump(lx); bump(lx); t.kind = TK_ARROW; }
  else if (c == '"') {
    bump(lx);
    while (peek(lx) && peek(lx) != '"') {
      if (peek(lx) == '\\' && lx->i + 1 < lx->len) { bump(lx); bump(lx); }
      else bump(lx);
    }
    if (peek(lx) == '"') bump(lx);
    t.kind = TK_STRING;
  }
  else if (is_alpha(c)) {
    bump(lx);
    while (is_alpha(peek(lx)) || is_digit(peek(lx))) bump(lx);
    t.kind = kw_kind(vittec_sv(lx->src + start, lx->i - start));
  }
  else if (is_digit(c)) {
    bump(lx);
    while (is_digit(peek(lx))) bump(lx);
    if (peek(lx) == '.' && lx->i + 1 < lx->len && is_digit(lx->src[lx->i+1])) {
      bump(lx);
      while (is_digit(peek(lx))) bump(lx);
      t.kind = TK_FLOAT;
    } else {
      t.kind = TK_INT;
    }
  }
  else {
    /* unknown char */
    bump(lx);
    vittec_diag_push(lx->diags, VITTEC_SEV_ERROR,
      vittec_span(lx->file_id, start, lx->i),
      vittec_sv("unexpected character", 20)
    );
    t.kind = TK_EOF;
  }

  t.span.hi = lx->i;
  t.text = vittec_sv(lx->src + start, t.span.hi - t.span.lo);
  return t;
}
