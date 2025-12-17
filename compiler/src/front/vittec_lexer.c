#include "vittec/front/lexer.h"
#include "vittec/front/token.h"
#include "vittec/support/str.h"
#include "vittec/diag/diagnostic.h"
#include "vittec/diag/span.h"
#include <ctype.h>
#include <stddef.h>
#include <string.h>

static const uint8_t* as_bytes(const void* src) {
  return src ? (const uint8_t*)src : (const uint8_t*)"";
}

void vittec_lexer_options_init(vittec_lexer_options_t* opt) {
  if (!opt) return;
  opt->size = sizeof(*opt);
  opt->flags = 0u;
  opt->max_token_bytes = 0u;
  opt->max_string_bytes = 0u;
}

static void vittec_lexer_reset_state(
  vittec_lexer_t* lx,
  const void* src,
  uint32_t len,
  vittec_file_id_t file_id,
  vittec_diag_sink_t* diags,
  vittec_diag_bag_t* bag,
  const vittec_lexer_options_t* opt) {
  lx->src = as_bytes(src);
  lx->len = src ? len : 0u;
  lx->i = 0u;
  lx->file_id = file_id;
  lx->diags = diags;
  lx->diag_bag = bag;
  if (opt) {
    lx->opt = *opt;
  } else {
    vittec_lexer_options_t tmp;
    vittec_lexer_options_init(&tmp);
    lx->opt = tmp;
  }
  lx->line = 1u;
  lx->col = 1u;
}

void vittec_lexer_init(
  vittec_lexer_t* lx,
  const char* src,
  uint32_t len,
  uint32_t file_id,
  vittec_diag_sink_t* diags) {
  if (!lx) return;
  vittec_lexer_options_t opt;
  vittec_lexer_options_init(&opt);
  vittec_lexer_reset_state(lx, src, len, file_id, diags, NULL, &opt);
}

void vittec_lexer_init_ex(
  vittec_lexer_t* lx,
  const void* src,
  uint32_t len,
  vittec_file_id_t file_id,
  vittec_diag_sink_t* diags,
  vittec_diag_bag_t* diag_bag,
  const vittec_lexer_options_t* opt) {
  if (!lx) return;
  vittec_lexer_reset_state(lx, src, len, file_id, diags, diag_bag, opt);
}

static uint8_t vittec_lexer_peek_byte(const vittec_lexer_t* lx) {
  return vittec_lexer_eof(lx) ? 0u : lx->src[lx->i];
}

static uint8_t vittec_lexer_peek_byte_n(const vittec_lexer_t* lx, uint32_t n) {
  if (!lx) return 0u;
  uint32_t j = lx->i + n;
  return (j >= lx->len) ? 0u : lx->src[j];
}

static uint8_t vittec_lexer_advance(vittec_lexer_t* lx) {
  if (vittec_lexer_eof(lx)) return 0u;
  uint8_t c = lx->src[lx->i++];
  if (c == '\n') {
    lx->line++;
    lx->col = 1u;
  } else {
    lx->col++;
  }
  return c;
}

static void vittec_diag_report_unexpected(vittec_lexer_t* lx, uint32_t start, const char* msg) {
  if (!lx || (!lx->diags && !lx->diag_bag)) return;
  vittec_span_t sp = vittec_span_with_file_id(lx->file_id, start, start + 1u);
  vittec_sv_t text = vittec_sv(msg, (uint64_t)strlen(msg));
  if (lx->diags) {
    vittec_diag_push(lx->diags, VITTEC_SEV_ERROR, sp, text);
  }
  if (lx->diag_bag) {
    vittec_diag_t d;
    vittec_diag_init(&d, VITTEC_SEV_ERROR, vittec_sv("", 0), sp, text);
    vittec_diag_bag_push(lx->diag_bag, &d);
    vittec_diag_free(&d);
  }
}

static vittec_token_t vittec_make_token(vittec_lexer_t* lx, vittec_token_kind_t kind, uint32_t lo, uint32_t hi) {
  const char* text_ptr = (const char*)lx->src + lo;
  vittec_sv_t text = vittec_sv(text_ptr, (uint64_t)(hi > lo ? (hi - lo) : 0u));
  return vittec_token_make(kind, vittec_span_with_file_id(lx->file_id, lo, hi), text);
}

static int is_ident_start(uint8_t c) {
  return (c == '_') || isalpha((int)c);
}

static int is_ident_continue(uint8_t c) {
  return (c == '_') || isalnum((int)c);
}

static void vittec_skip_spaces(vittec_lexer_t* lx) {
  while (!vittec_lexer_eof(lx)) {
    uint8_t c = vittec_lexer_peek_byte(lx);
    if (c == ' ' || c == '\v' || c == '\f') {
      vittec_lexer_advance(lx);
      continue;
    }
    if (c == '\t' && (lx->opt.flags & VITTEC_LEX_ACCEPT_TABS)) {
      vittec_lexer_advance(lx);
      continue;
    }
    if (c == '\r') {
      if (lx->opt.flags & VITTEC_LEX_ACCEPT_CRLF) {
        vittec_lexer_advance(lx);
        if (vittec_lexer_peek_byte(lx) == '\n') vittec_lexer_advance(lx);
        lx->line++;
        lx->col = 1u;
        continue;
      }
      vittec_lexer_advance(lx);
      continue;
    }
    break;
  }
}

static vittec_token_t vittec_make_comment_token(vittec_lexer_t* lx, uint32_t start, uint32_t end) {
  return vittec_make_token(lx, VITTEC_TOK_COMMENT, start, end);
}

static vittec_token_t vittec_emit_newline(vittec_lexer_t* lx, uint32_t start, uint32_t end) {
  return vittec_make_token(lx, VITTEC_TOK_NEWLINE, start, end);
}

static int vittec_should_emit(uint32_t flags, vittec_lexer_flag_t f) {
  return (flags & f) != 0u;
}

static vittec_token_t vittec_scan_comment(vittec_lexer_t* lx, int keep, int block) {
  uint32_t start = lx->i;
  int closed = block ? 0 : 1;
  if (block) {
    vittec_lexer_advance(lx); /* / */
    vittec_lexer_advance(lx); /* * */
    while (!vittec_lexer_eof(lx)) {
      if (vittec_lexer_peek_byte(lx) == '*' && vittec_lexer_peek_byte_n(lx, 1u) == '/') {
        vittec_lexer_advance(lx);
        vittec_lexer_advance(lx);
        closed = 1;
        break;
      }
      vittec_lexer_advance(lx);
    }
    if (!closed) {
      vittec_diag_report_unexpected(lx, start, "unterminated block comment");
    }
  } else {
    vittec_lexer_advance(lx); /* / */
    vittec_lexer_advance(lx); /* / */
    while (!vittec_lexer_eof(lx) && vittec_lexer_peek_byte(lx) != '\n') {
      vittec_lexer_advance(lx);
    }
  }
  uint32_t end = lx->i;
  if (keep) return vittec_make_comment_token(lx, start, end);
  return vittec_make_token(lx, VITTEC_TOK_COMMENT, end, end); /* zero-length placeholder */
}

static vittec_token_t vittec_scan_number(vittec_lexer_t* lx) {
  uint32_t start = lx->i;
  int is_float = 0;
  while (!vittec_lexer_eof(lx)) {
    uint8_t c = vittec_lexer_peek_byte(lx);
    if (isdigit((int)c) || c == '_') {
      vittec_lexer_advance(lx);
      continue;
    }
    if (!is_float && c == '.' && isdigit((int)vittec_lexer_peek_byte_n(lx, 1u))) {
      is_float = 1;
      vittec_lexer_advance(lx);
      continue;
    }
    break;
  }
  uint32_t end = lx->i;
  vittec_token_kind_t kind = is_float ? VITTEC_TOK_FLOAT : VITTEC_TOK_INT;
  return vittec_make_token(lx, kind, start, end);
}

static vittec_token_t vittec_scan_string(vittec_lexer_t* lx) {
  uint32_t start = lx->i;
  vittec_lexer_advance(lx); /* opening quote */
  while (!vittec_lexer_eof(lx)) {
    uint8_t c = vittec_lexer_peek_byte(lx);
    if (c == '"') {
      vittec_lexer_advance(lx);
      return vittec_make_token(lx, VITTEC_TOK_STRING, start, lx->i);
    }
    if (c == '\\') {
      vittec_lexer_advance(lx);
      if (!vittec_lexer_eof(lx)) vittec_lexer_advance(lx);
      continue;
    }
    vittec_lexer_advance(lx);
  }
  vittec_diag_report_unexpected(lx, start, "unterminated string literal");
  return vittec_make_token(lx, VITTEC_TOK_ERROR, start, lx->i);
}

static vittec_token_t vittec_scan_ident_or_keyword(vittec_lexer_t* lx) {
  uint32_t start = lx->i;
  while (!vittec_lexer_eof(lx) && is_ident_continue(vittec_lexer_peek_byte(lx))) {
    vittec_lexer_advance(lx);
  }
  uint32_t end = lx->i;
  vittec_token_t tok = vittec_make_token(lx, VITTEC_TOK_IDENT, start, end);
  vittec_keyword_t kw = vittec_keyword_from_sv(tok.text);
  if (kw != VITTEC_KW_UNKNOWN) {
    tok.kind = VITTEC_TOK_KEYWORD;
    tok.as.kw = kw;
  }
  return tok;
}

static vittec_punct_t vittec_punct_from_pair(uint8_t first, uint8_t second, int* consumed) {
  *consumed = 0;
  switch (first) {
    case ':':
      if (second == ':') { *consumed = 1; }
      return VITTEC_PUNCT_COLON;
    case '.':
      return VITTEC_PUNCT_DOT;
    case ',':
      return VITTEC_PUNCT_COMMA;
    case ';':
      return VITTEC_PUNCT_SEMI;
    case '(':
      return VITTEC_PUNCT_LPAREN;
    case ')':
      return VITTEC_PUNCT_RPAREN;
    case '[':
      return VITTEC_PUNCT_LBRACK;
    case ']':
      return VITTEC_PUNCT_RBRACK;
    case '=':
      if (second == '>') { *consumed = 1; return VITTEC_PUNCT_FATARROW; }
      if (second == '=') { *consumed = 1; return VITTEC_PUNCT_EQEQ; }
      return VITTEC_PUNCT_EQ;
    case '+':
      return VITTEC_PUNCT_PLUS;
    case '-':
      if (second == '>') { *consumed = 1; return VITTEC_PUNCT_ARROW; }
      return VITTEC_PUNCT_MINUS;
    case '*':
      return VITTEC_PUNCT_STAR;
    case '/':
      return VITTEC_PUNCT_SLASH;
    case '%':
      return VITTEC_PUNCT_PERCENT;
    case '&':
      if (second == '&') { *consumed = 1; return VITTEC_PUNCT_ANDAND; }
      return VITTEC_PUNCT_AMP;
    case '|':
      if (second == '|') { *consumed = 1; return VITTEC_PUNCT_OROR; }
      return VITTEC_PUNCT_PIPE;
    case '^':
      return VITTEC_PUNCT_CARET;
    case '~':
      return VITTEC_PUNCT_TILDE;
    case '!':
      if (second == '=') { *consumed = 1; return VITTEC_PUNCT_NE; }
      return VITTEC_PUNCT_BANG;
    case '<':
      if (second == '=') { *consumed = 1; return VITTEC_PUNCT_LE; }
      return VITTEC_PUNCT_LT;
    case '>':
      if (second == '=') { *consumed = 1; return VITTEC_PUNCT_GE; }
      return VITTEC_PUNCT_GT;
    case '?':
      return VITTEC_PUNCT_QUESTION;
    default:
      return VITTEC_PUNCT_UNKNOWN;
  }
}

static vittec_token_t vittec_scan_punct(vittec_lexer_t* lx) {
  uint32_t start = lx->i;
  uint8_t first = vittec_lexer_advance(lx);
  uint8_t look = vittec_lexer_peek_byte(lx);
  int extra = 0;
  vittec_punct_t p = vittec_punct_from_pair(first, look, &extra);
  if (extra) vittec_lexer_advance(lx);
  vittec_token_t tok = vittec_make_token(lx, VITTEC_TOK_PUNCT, start, lx->i);
  tok.as.punct = p;
  return tok;
}

static int vittec_is_decimal_start(uint8_t c) {
  return isdigit((int)c);
}

vittec_token_t vittec_lex_next(vittec_lexer_t* lx) {
  if (!lx) return vittec_token_make(VITTEC_TOK_EOF, vittec_span_with_file_id(0, 0, 0), vittec_sv(NULL, 0));

  for (;;) {
    if (vittec_lexer_eof(lx)) {
      return vittec_make_token(lx, VITTEC_TOK_EOF, lx->len, lx->len);
    }

    uint8_t c = vittec_lexer_peek_byte(lx);
    if (c == '\n') {
      uint32_t start = lx->i;
      vittec_lexer_advance(lx);
      if (vittec_should_emit(lx->opt.flags, VITTEC_LEX_KEEP_NEWLINES)) {
        return vittec_emit_newline(lx, start, lx->i);
      }
      continue;
    }

    if (c == ' ' || c == '\v' || c == '\f' || c == '\t' || c == '\r') {
      vittec_skip_spaces(lx);
      continue;
    }

    if (c == '/' && vittec_lexer_peek_byte_n(lx, 1u) == '/') {
      int keep = vittec_should_emit(lx->opt.flags, VITTEC_LEX_KEEP_COMMENTS);
      vittec_token_t tok = vittec_scan_comment(lx, keep, 0);
      if (keep) return tok;
      continue;
    }
    if (c == '/' && vittec_lexer_peek_byte_n(lx, 1u) == '*') {
      int keep = vittec_should_emit(lx->opt.flags, VITTEC_LEX_KEEP_COMMENTS);
      vittec_token_t tok = vittec_scan_comment(lx, keep, 1);
      if (keep) return tok;
      continue;
    }

    break;
  }

  uint8_t c = vittec_lexer_peek_byte(lx);
  if (is_ident_start(c)) {
    return vittec_scan_ident_or_keyword(lx);
  }
  if (vittec_is_decimal_start(c)) {
    return vittec_scan_number(lx);
  }
  if (c == '"') {
    return vittec_scan_string(lx);
  }
  switch (c) {
    case '.': case ',': case ';': case ':':
    case '(':
    case ')':
    case '[':
    case ']':
    case '=':
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '&':
    case '|':
    case '^':
    case '~':
    case '!':
    case '<':
    case '>':
    case '?':
      return vittec_scan_punct(lx);
    default:
      break;
  }

  uint32_t start = lx->i;
  vittec_lexer_advance(lx);
  vittec_diag_report_unexpected(lx, start, "unexpected character");
  return vittec_make_token(lx, VITTEC_TOK_ERROR, start, lx->i);
}

vittec_token_t vittec_lex_peek(vittec_lexer_t* lx) {
  if (!lx) return vittec_token_make(VITTEC_TOK_EOF, vittec_span_with_file_id(0, 0, 0), vittec_sv(NULL, 0));
  vittec_lexer_mark_t mark = vittec_lexer_mark(lx);
  vittec_token_t tok = vittec_lex_next(lx);
  vittec_lexer_restore(lx, mark);
  return tok;
}
