

// lexer.c
// -----------------------------------------------------------------------------
// Vitte front-end lexer (tokenizer)
//
// Design goals:
//   - Single-pass, zero-copy tokens: tokens reference the original source buffer.
//   - Stable token positions: byte offsets + 1-based line/column.
//   - Robust literal parsing: ints (bin/oct/dec/hex), floats (dec + exponent),
//     strings/chars with escapes, underscore separators.
//   - Comment handling: // line, /* block */ (optionally nested).
//   - Keyword recognition for core + phrase surface.
//   - Optional integration with existing headers (lexer.h/diag types) via
//     __has_include.
//
// Conventions:
//   - Offsets are 0-based bytes.
//   - line/col are 1-based.
//   - Column is byte-based for speed (UTF-8 codepoint col can be computed later
//     via source_map if needed).
// -----------------------------------------------------------------------------

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(__has_include)
  #if __has_include("lexer.h")
    #include "lexer.h"
    #define VITTE_HAVE_LEXER_H 1
  #endif
#endif

#ifndef VITTE_HAVE_LEXER_H
// -----------------------------
// Fallback public-ish API types
// (If you have lexer.h already, it should define compatible equivalents.)
// -----------------------------

typedef enum vitte_token_kind_t {
  VITTE_TOK_EOF = 0,
  VITTE_TOK_ERROR,

  // Identifiers / keywords
  VITTE_TOK_IDENT,

  // Literals
  VITTE_TOK_INT,
  VITTE_TOK_FLOAT,
  VITTE_TOK_STRING,
  VITTE_TOK_CHAR,

  // Trivia (optionally emitted)
  VITTE_TOK_LINE_COMMENT,
  VITTE_TOK_BLOCK_COMMENT,

  // Punctuation
  VITTE_TOK_LPAREN,   // (
  VITTE_TOK_RPAREN,   // )
  VITTE_TOK_LBRACK,   // [
  VITTE_TOK_RBRACK,   // ]
  VITTE_TOK_LBRACE,   // {
  VITTE_TOK_RBRACE,   // }
  VITTE_TOK_COMMA,    // ,
  VITTE_TOK_SEMI,     // ;
  VITTE_TOK_COLON,    // :
  VITTE_TOK_COLON2,   // ::
  VITTE_TOK_DOT,      // .
  VITTE_TOK_DOT2,     // ..
  VITTE_TOK_DOT2_EQ,  // ..=
  VITTE_TOK_DOT_END,  // .end (block terminator)

  // Operators
  VITTE_TOK_PLUS,     // +
  VITTE_TOK_MINUS,    // -
  VITTE_TOK_STAR,     // *
  VITTE_TOK_SLASH,    // /
  VITTE_TOK_PERCENT,  // %
  VITTE_TOK_CARET,    // ^
  VITTE_TOK_AMP,      // &
  VITTE_TOK_PIPE,     // |
  VITTE_TOK_TILDE,    // ~
  VITTE_TOK_BANG,     // !
  VITTE_TOK_QUESTION, // ?

  VITTE_TOK_EQ,       // =
  VITTE_TOK_EQ2,      // ==
  VITTE_TOK_BANG_EQ,  // !=
  VITTE_TOK_LT,       // <
  VITTE_TOK_LE,       // <=
  VITTE_TOK_GT,       // >
  VITTE_TOK_GE,       // >=

  VITTE_TOK_AMP2,     // &&
  VITTE_TOK_PIPE2,    // ||
  VITTE_TOK_SHL,      // <<
  VITTE_TOK_SHR,      // >>

  VITTE_TOK_PLUS_EQ,  // +=
  VITTE_TOK_MINUS_EQ, // -=
  VITTE_TOK_STAR_EQ,  // *=
  VITTE_TOK_SLASH_EQ, // /=
  VITTE_TOK_PERCENT_EQ,// %=
  VITTE_TOK_AMP_EQ,   // &=
  VITTE_TOK_PIPE_EQ,  // |=
  VITTE_TOK_CARET_EQ, // ^=
  VITTE_TOK_SHL_EQ,   // <<=
  VITTE_TOK_SHR_EQ,   // >>=

  VITTE_TOK_ARROW,    // ->
  VITTE_TOK_FATARROW, // =>

  // Keywords (core + phrase surface)
  VITTE_KW_mod,
  VITTE_KW_use,
  VITTE_KW_export,
  VITTE_KW_from,

  VITTE_KW_type,
  VITTE_KW_struct,
  VITTE_KW_union,
  VITTE_KW_enum,

  VITTE_KW_fn,
  VITTE_KW_scenario,

  VITTE_KW_program,
  VITTE_KW_service,
  VITTE_KW_kernel,
  VITTE_KW_driver,
  VITTE_KW_tool,
  VITTE_KW_pipeline,

  VITTE_KW_let,
  VITTE_KW_const,

  VITTE_KW_if,
  VITTE_KW_elif,
  VITTE_KW_else,
  VITTE_KW_while,
  VITTE_KW_for,
  VITTE_KW_match,

  VITTE_KW_break,
  VITTE_KW_continue,
  VITTE_KW_return,

  // Phrase sugar
  VITTE_KW_set,
  VITTE_KW_say,
  VITTE_KW_do,
  VITTE_KW_ret,
  VITTE_KW_when,
  VITTE_KW_loop,

  // Literals-as-keywords
  VITTE_KW_true,
  VITTE_KW_false,
  VITTE_KW_null
} vitte_token_kind_t;

typedef struct vitte_token_t {
  vitte_token_kind_t kind;
  uint32_t line;   // 1-based
  uint32_t col;    // 1-based
  size_t   off;    // 0-based byte offset
  size_t   len;    // token length in bytes
} vitte_token_t;

typedef struct vitte_lexer_t {
  const char *path; // optional
  const char *src;
  size_t len;

  size_t pos;       // current byte offset
  uint32_t line;    // 1-based
  uint32_t col;     // 1-based

  bool emit_comments;
  bool nested_block_comments;

  // Sticky error info (best-effort)
  const char *err_msg;
  size_t err_off;

  // Lookahead cache
  bool has_peek;
  vitte_token_t peek_tok;
} vitte_lexer_t;

#endif // !VITTE_HAVE_LEXER_H

// -----------------------------------------------------------------------------
// Internal character helpers
// -----------------------------------------------------------------------------

static inline bool lx_is_eof(const vitte_lexer_t *lx) {
  return !lx || lx->pos >= lx->len;
}

static inline unsigned char lx_peek_u8(const vitte_lexer_t *lx) {
  if (!lx || lx->pos >= lx->len) return 0;
  return (unsigned char)lx->src[lx->pos];
}

static inline unsigned char lx_peek_u8_n(const vitte_lexer_t *lx, size_t n) {
  size_t p = lx ? lx->pos + n : 0;
  if (!lx || p >= lx->len) return 0;
  return (unsigned char)lx->src[p];
}

static inline void lx_advance(vitte_lexer_t *lx, size_t n) {
  if (!lx) return;
  while (n-- && lx->pos < lx->len) {
    unsigned char c = (unsigned char)lx->src[lx->pos++];
    if (c == '\n') {
      lx->line += 1;
      lx->col = 1;
    } else {
      lx->col += 1;
    }
  }
}

static inline bool lx_match(vitte_lexer_t *lx, char ch) {
  if (!lx || lx->pos >= lx->len) return false;
  if (lx->src[lx->pos] != ch) return false;
  lx_advance(lx, 1);
  return true;
}

static inline bool lx_starts_with(const vitte_lexer_t *lx, const char *s) {
  if (!lx || !s) return false;
  size_t n = strlen(s);
  if (lx->pos + n > lx->len) return false;
  return memcmp(lx->src + lx->pos, s, n) == 0;
}

static inline bool lx_is_alpha_ascii(unsigned char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline bool lx_is_digit_ascii(unsigned char c) {
  return (c >= '0' && c <= '9');
}

static inline bool lx_is_hex_ascii(unsigned char c) {
  return (c >= '0' && c <= '9') ||
         (c >= 'A' && c <= 'F') ||
         (c >= 'a' && c <= 'f');
}

static inline bool lx_is_ident_start(unsigned char c) {
  // ASCII fast path: letter or underscore
  if (lx_is_alpha_ascii(c) || c == '_') return true;
  // Best-effort UTF-8 allowance: treat any non-ASCII byte as start; validation
  // is delegated to later unicode passes.
  if (c >= 0x80) return true;
  return false;
}

static inline bool lx_is_ident_continue(unsigned char c) {
  if (lx_is_alpha_ascii(c) || lx_is_digit_ascii(c) || c == '_') return true;
  if (c >= 0x80) return true;
  return false;
}

static void lx_set_error(vitte_lexer_t *lx, const char *msg, size_t off) {
  if (!lx) return;
  lx->err_msg = msg;
  lx->err_off = off;
}

static vitte_token_t lx_make_tok(vitte_token_kind_t k, uint32_t line, uint32_t col, size_t off, size_t len) {
  vitte_token_t t;
  t.kind = k;
  t.line = line;
  t.col = col;
  t.off = off;
  t.len = len;
  return t;
}

// -----------------------------------------------------------------------------
// Keyword table
// -----------------------------------------------------------------------------

typedef struct { const char *s; vitte_token_kind_t k; } lx_kw_t;

static const lx_kw_t k_keywords[] = {
  {"mod",      VITTE_KW_mod},
  {"use",      VITTE_KW_use},
  {"export",   VITTE_KW_export},
  {"from",     VITTE_KW_from},

  {"type",     VITTE_KW_type},
  {"struct",   VITTE_KW_struct},
  {"union",    VITTE_KW_union},
  {"enum",     VITTE_KW_enum},

  {"fn",       VITTE_KW_fn},
  {"scenario", VITTE_KW_scenario},

  {"program",  VITTE_KW_program},
  {"service",  VITTE_KW_service},
  {"kernel",   VITTE_KW_kernel},
  {"driver",   VITTE_KW_driver},
  {"tool",     VITTE_KW_tool},
  {"pipeline", VITTE_KW_pipeline},

  {"let",      VITTE_KW_let},
  {"const",    VITTE_KW_const},

  {"if",       VITTE_KW_if},
  {"elif",     VITTE_KW_elif},
  {"else",     VITTE_KW_else},
  {"while",    VITTE_KW_while},
  {"for",      VITTE_KW_for},
  {"match",    VITTE_KW_match},

  {"break",    VITTE_KW_break},
  {"continue", VITTE_KW_continue},
  {"return",   VITTE_KW_return},

  // phrase surface
  {"set",      VITTE_KW_set},
  {"say",      VITTE_KW_say},
  {"do",       VITTE_KW_do},
  {"ret",      VITTE_KW_ret},
  {"when",     VITTE_KW_when},
  {"loop",     VITTE_KW_loop},

  // literals-as-keywords
  {"true",     VITTE_KW_true},
  {"false",    VITTE_KW_false},
  {"null",     VITTE_KW_null},
};

static vitte_token_kind_t lx_ident_to_keyword(const char *p, size_t n) {
  // Linear scan: keyword count is small; can be swapped for perfect hash later.
  for (size_t i = 0; i < sizeof(k_keywords) / sizeof(k_keywords[0]); i++) {
    const char *s = k_keywords[i].s;
    size_t m = strlen(s);
    if (m == n && memcmp(p, s, n) == 0) return k_keywords[i].k;
  }
  return VITTE_TOK_IDENT;
}

// -----------------------------------------------------------------------------
// Whitespace / comments
// -----------------------------------------------------------------------------

static void lx_skip_ws(vitte_lexer_t *lx) {
  while (!lx_is_eof(lx)) {
    unsigned char c = lx_peek_u8(lx);
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v') {
      lx_advance(lx, 1);
      continue;
    }
    break;
  }
}

static vitte_token_t lx_lex_line_comment(vitte_lexer_t *lx, uint32_t line, uint32_t col, size_t start_off) {
  // assumes // already matched
  while (!lx_is_eof(lx)) {
    unsigned char c = lx_peek_u8(lx);
    if (c == '\n') break;
    lx_advance(lx, 1);
  }
  size_t end_off = lx->pos;
  return lx_make_tok(VITTE_TOK_LINE_COMMENT, line, col, start_off, end_off - start_off);
}

static vitte_token_t lx_lex_block_comment(vitte_lexer_t *lx, uint32_t line, uint32_t col, size_t start_off) {
  // assumes /* already matched
  int depth = 1;
  while (!lx_is_eof(lx)) {
    if (lx_peek_u8(lx) == '/' && lx_peek_u8_n(lx, 1) == '*') {
      if (lx->nested_block_comments) {
        lx_advance(lx, 2);
        depth++;
        continue;
      }
    }
    if (lx_peek_u8(lx) == '*' && lx_peek_u8_n(lx, 1) == '/') {
      lx_advance(lx, 2);
      depth--;
      if (depth == 0) {
        size_t end_off = lx->pos;
        return lx_make_tok(VITTE_TOK_BLOCK_COMMENT, line, col, start_off, end_off - start_off);
      }
      continue;
    }
    lx_advance(lx, 1);
  }

  // Unterminated
  lx_set_error(lx, "unterminated block comment", start_off);
  return lx_make_tok(VITTE_TOK_ERROR, line, col, start_off, lx->pos - start_off);
}

// If emit_comments=false, comments are skipped as trivia and lexing continues.
static bool lx_try_skip_or_emit_comment(vitte_lexer_t *lx, vitte_token_t *out) {
  if (!lx || lx_is_eof(lx)) return false;
  if (lx_peek_u8(lx) != '/') return false;

  unsigned char n1 = lx_peek_u8_n(lx, 1);
  if (n1 != '/' && n1 != '*') return false;

  uint32_t line = lx->line;
  uint32_t col  = lx->col;
  size_t start  = lx->pos;

  if (n1 == '/') {
    lx_advance(lx, 2);
    vitte_token_t t = lx_lex_line_comment(lx, line, col, start);
    if (lx->emit_comments) {
      if (out) *out = t;
      return true;
    }
    // else skip
    return true;
  }

  // /* */
  lx_advance(lx, 2);
  vitte_token_t t = lx_lex_block_comment(lx, line, col, start);
  if (lx->emit_comments) {
    if (out) *out = t;
    return true;
  }
  return true;
}

// -----------------------------------------------------------------------------
// Literals
// -----------------------------------------------------------------------------

static bool lx_is_num_sep(unsigned char c) { return c == '_'; }

static void lx_consume_digits(vitte_lexer_t *lx, int base) {
  while (!lx_is_eof(lx)) {
    unsigned char c = lx_peek_u8(lx);
    if (lx_is_num_sep(c)) { lx_advance(lx, 1); continue; }
    if (base == 10) {
      if (!lx_is_digit_ascii(c)) break;
    } else if (base == 16) {
      if (!lx_is_hex_ascii(c)) break;
    } else if (base == 8) {
      if (!(c >= '0' && c <= '7')) break;
    } else if (base == 2) {
      if (!(c == '0' || c == '1')) break;
    } else {
      break;
    }
    lx_advance(lx, 1);
  }
}

static vitte_token_t lx_lex_number(vitte_lexer_t *lx, uint32_t line, uint32_t col, size_t start_off) {
  // Supports:
  //   - 123, 1_000
  //   - 0b1010, 0o755, 0xFF
  //   - floats: 1.23, 1e10, 1.2e-3, .5 (handled elsewhere), 1.

  // base prefixes
  int base = 10;
  bool is_float = false;

  if (lx_peek_u8(lx) == '0') {
    unsigned char p1 = lx_peek_u8_n(lx, 1);
    if (p1 == 'x' || p1 == 'X') { base = 16; lx_advance(lx, 2); lx_consume_digits(lx, base); goto done; }
    if (p1 == 'b' || p1 == 'B') { base = 2;  lx_advance(lx, 2); lx_consume_digits(lx, base); goto done; }
    if (p1 == 'o' || p1 == 'O') { base = 8;  lx_advance(lx, 2); lx_consume_digits(lx, base); goto done; }
  }

  // decimal digits
  lx_consume_digits(lx, 10);

  // fractional part
  if (lx_peek_u8(lx) == '.' && lx_peek_u8_n(lx, 1) != '.' ) {
    // 1.23 or 1.
    is_float = true;
    lx_advance(lx, 1);
    lx_consume_digits(lx, 10);
  }

  // exponent
  {
    unsigned char c = lx_peek_u8(lx);
    if (c == 'e' || c == 'E') {
      is_float = true;
      lx_advance(lx, 1);
      if (lx_peek_u8(lx) == '+' || lx_peek_u8(lx) == '-') lx_advance(lx, 1);
      // require at least one digit
      if (!lx_is_digit_ascii(lx_peek_u8(lx))) {
        lx_set_error(lx, "invalid float exponent", start_off);
        size_t end_off = lx->pos;
        return lx_make_tok(VITTE_TOK_ERROR, line, col, start_off, end_off - start_off);
      }
      lx_consume_digits(lx, 10);
    }
  }

done:
  {
    size_t end_off = lx->pos;
    return lx_make_tok(is_float ? VITTE_TOK_FLOAT : VITTE_TOK_INT, line, col, start_off, end_off - start_off);
  }
}

static bool lx_is_escape_char(unsigned char c) {
  switch (c) {
    case '\\': case '\'' : case '"': case 'n': case 'r': case 't':
    case '0' : case 'b' : case 'f' : case 'v':
      return true;
    default:
      return false;
  }
}

static bool lx_consume_hex_n(vitte_lexer_t *lx, int n) {
  for (int i = 0; i < n; i++) {
    if (!lx_is_hex_ascii(lx_peek_u8(lx))) return false;
    lx_advance(lx, 1);
  }
  return true;
}

static bool lx_consume_unicode_braced(vitte_lexer_t *lx) {
  // expects '{' already consumed
  // read 1..6 hex digits then '}'
  int digits = 0;
  while (!lx_is_eof(lx)) {
    unsigned char c = lx_peek_u8(lx);
    if (c == '}') {
      lx_advance(lx, 1);
      return digits > 0;
    }
    if (!lx_is_hex_ascii(c)) return false;
    lx_advance(lx, 1);
    digits++;
    if (digits > 6) return false;
  }
  return false;
}

static vitte_token_t lx_lex_string_like(vitte_lexer_t *lx, vitte_token_kind_t kind, char quote, uint32_t line, uint32_t col, size_t start_off) {
  // quote already consumed
  while (!lx_is_eof(lx)) {
    unsigned char c = lx_peek_u8(lx);
    if (c == (unsigned char)quote) {
      lx_advance(lx, 1);
      size_t end_off = lx->pos;
      return lx_make_tok(kind, line, col, start_off, end_off - start_off);
    }

    if (c == '\\') {
      lx_advance(lx, 1);
      if (lx_is_eof(lx)) break;
      unsigned char e = lx_peek_u8(lx);
      if (lx_is_escape_char(e)) {
        lx_advance(lx, 1);
        continue;
      }
      // \xNN
      if (e == 'x') {
        lx_advance(lx, 1);
        if (!lx_consume_hex_n(lx, 2)) {
          lx_set_error(lx, "invalid \\x escape", start_off);
          return lx_make_tok(VITTE_TOK_ERROR, line, col, start_off, lx->pos - start_off);
        }
        continue;
      }
      // \uNNNN or \u{...}
      if (e == 'u') {
        lx_advance(lx, 1);
        if (lx_match(lx, '{')) {
          if (!lx_consume_unicode_braced(lx)) {
            lx_set_error(lx, "invalid \\u{..} escape", start_off);
            return lx_make_tok(VITTE_TOK_ERROR, line, col, start_off, lx->pos - start_off);
          }
          continue;
        }
        if (!lx_consume_hex_n(lx, 4)) {
          lx_set_error(lx, "invalid \\uNNNN escape", start_off);
          return lx_make_tok(VITTE_TOK_ERROR, line, col, start_off, lx->pos - start_off);
        }
        continue;
      }

      // Unknown escape
      lx_set_error(lx, "unknown escape sequence", start_off);
      return lx_make_tok(VITTE_TOK_ERROR, line, col, start_off, lx->pos - start_off);
    }

    // For string literals, allow raw newlines only if you want multiline strings.
    // Here we treat newline as error (more strict; can be relaxed later).
    if (kind == VITTE_TOK_STRING && c == '\n') {
      lx_set_error(lx, "unterminated string literal", start_off);
      return lx_make_tok(VITTE_TOK_ERROR, line, col, start_off, lx->pos - start_off);
    }

    lx_advance(lx, 1);
  }

  lx_set_error(lx, (kind == VITTE_TOK_STRING) ? "unterminated string literal" : "unterminated char literal", start_off);
  return lx_make_tok(VITTE_TOK_ERROR, line, col, start_off, lx->pos - start_off);
}

static vitte_token_t lx_lex_char(vitte_lexer_t *lx, uint32_t line, uint32_t col, size_t start_off) {
  // ' already consumed
  vitte_token_t t = lx_lex_string_like(lx, VITTE_TOK_CHAR, '\'', line, col, start_off);
  // Minimal sanity: must contain at least one payload byte between quotes.
  if (t.kind == VITTE_TOK_CHAR) {
    if (t.len < 3) { // '' is invalid, '\'' minimal is 3 bytes but that's OK
      lx_set_error(lx, "empty char literal", start_off);
      return lx_make_tok(VITTE_TOK_ERROR, line, col, start_off, t.len);
    }
  }
  return t;
}

static vitte_token_t lx_lex_string(vitte_lexer_t *lx, uint32_t line, uint32_t col, size_t start_off) {
  // " already consumed
  return lx_lex_string_like(lx, VITTE_TOK_STRING, '"', line, col, start_off);
}

// -----------------------------------------------------------------------------
// Identifiers
// -----------------------------------------------------------------------------

static vitte_token_t lx_lex_ident_or_kw(vitte_lexer_t *lx, uint32_t line, uint32_t col, size_t start_off) {
  // first char already consumed or ensured
  while (!lx_is_eof(lx)) {
    unsigned char c = lx_peek_u8(lx);
    if (!lx_is_ident_continue(c)) break;
    lx_advance(lx, 1);
  }
  size_t end_off = lx->pos;
  size_t n = end_off - start_off;
  const char *p = lx->src + start_off;
  vitte_token_kind_t k = lx_ident_to_keyword(p, n);
  return lx_make_tok(k, line, col, start_off, n);
}

// -----------------------------------------------------------------------------
// Punctuators / operators
// -----------------------------------------------------------------------------

static bool lx_is_ident_boundary(unsigned char c) {
  // boundary after .end, so that ".endX" is not treated as DOT_END.
  return !lx_is_ident_continue(c);
}

static vitte_token_t lx_lex_punct_or_op(vitte_lexer_t *lx, uint32_t line, uint32_t col, size_t start_off) {
  unsigned char c = lx_peek_u8(lx);
  switch (c) {
    case '(' : lx_advance(lx,1); return lx_make_tok(VITTE_TOK_LPAREN, line,col,start_off,1);
    case ')' : lx_advance(lx,1); return lx_make_tok(VITTE_TOK_RPAREN, line,col,start_off,1);
    case '[' : lx_advance(lx,1); return lx_make_tok(VITTE_TOK_LBRACK, line,col,start_off,1);
    case ']' : lx_advance(lx,1); return lx_make_tok(VITTE_TOK_RBRACK, line,col,start_off,1);
    case '{' : lx_advance(lx,1); return lx_make_tok(VITTE_TOK_LBRACE, line,col,start_off,1);
    case '}' : lx_advance(lx,1); return lx_make_tok(VITTE_TOK_RBRACE, line,col,start_off,1);
    case ',' : lx_advance(lx,1); return lx_make_tok(VITTE_TOK_COMMA,  line,col,start_off,1);
    case ';' : lx_advance(lx,1); return lx_make_tok(VITTE_TOK_SEMI,   line,col,start_off,1);

    case ':' :
      lx_advance(lx,1);
      if (lx_match(lx, ':')) return lx_make_tok(VITTE_TOK_COLON2, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_COLON,  line,col,start_off,1);

    case '.' :
      lx_advance(lx,1);
      if (lx_match(lx, '.')) {
        if (lx_match(lx, '=')) return lx_make_tok(VITTE_TOK_DOT2_EQ, line,col,start_off,3);
        return lx_make_tok(VITTE_TOK_DOT2, line,col,start_off,2);
      }
      // .end special-case
      if (lx_starts_with(lx, "end")) {
        unsigned char b = lx_peek_u8_n(lx, 3);
        if (lx_is_ident_boundary(b)) {
          lx_advance(lx, 3);
          return lx_make_tok(VITTE_TOK_DOT_END, line,col,start_off,4);
        }
      }
      return lx_make_tok(VITTE_TOK_DOT, line,col,start_off,1);

    case '+':
      lx_advance(lx,1);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_PLUS_EQ, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_PLUS, line,col,start_off,1);

    case '-':
      lx_advance(lx,1);
      if (lx_match(lx,'>')) return lx_make_tok(VITTE_TOK_ARROW, line,col,start_off,2);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_MINUS_EQ, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_MINUS, line,col,start_off,1);

    case '*':
      lx_advance(lx,1);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_STAR_EQ, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_STAR, line,col,start_off,1);

    case '/':
      // comments handled earlier; here is division
      lx_advance(lx,1);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_SLASH_EQ, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_SLASH, line,col,start_off,1);

    case '%':
      lx_advance(lx,1);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_PERCENT_EQ, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_PERCENT, line,col,start_off,1);

    case '^':
      lx_advance(lx,1);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_CARET_EQ, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_CARET, line,col,start_off,1);

    case '~':
      lx_advance(lx,1);
      return lx_make_tok(VITTE_TOK_TILDE, line,col,start_off,1);

    case '?':
      lx_advance(lx,1);
      return lx_make_tok(VITTE_TOK_QUESTION, line,col,start_off,1);

    case '!':
      lx_advance(lx,1);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_BANG_EQ, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_BANG, line,col,start_off,1);

    case '=':
      lx_advance(lx,1);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_EQ2, line,col,start_off,2);
      if (lx_match(lx,'>')) return lx_make_tok(VITTE_TOK_FATARROW, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_EQ, line,col,start_off,1);

    case '<':
      lx_advance(lx,1);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_LE, line,col,start_off,2);
      if (lx_match(lx,'<')) {
        if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_SHL_EQ, line,col,start_off,3);
        return lx_make_tok(VITTE_TOK_SHL, line,col,start_off,2);
      }
      return lx_make_tok(VITTE_TOK_LT, line,col,start_off,1);

    case '>':
      lx_advance(lx,1);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_GE, line,col,start_off,2);
      if (lx_match(lx,'>')) {
        if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_SHR_EQ, line,col,start_off,3);
        return lx_make_tok(VITTE_TOK_SHR, line,col,start_off,2);
      }
      return lx_make_tok(VITTE_TOK_GT, line,col,start_off,1);

    case '&':
      lx_advance(lx,1);
      if (lx_match(lx,'&')) return lx_make_tok(VITTE_TOK_AMP2, line,col,start_off,2);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_AMP_EQ, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_AMP, line,col,start_off,1);

    case '|':
      lx_advance(lx,1);
      if (lx_match(lx,'|')) return lx_make_tok(VITTE_TOK_PIPE2, line,col,start_off,2);
      if (lx_match(lx,'=')) return lx_make_tok(VITTE_TOK_PIPE_EQ, line,col,start_off,2);
      return lx_make_tok(VITTE_TOK_PIPE, line,col,start_off,1);

    default:
      break;
  }

  // Unknown byte
  lx_set_error(lx, "unexpected character", start_off);
  lx_advance(lx, 1);
  return lx_make_tok(VITTE_TOK_ERROR, line, col, start_off, 1);
}

// -----------------------------------------------------------------------------
// Core lexing routine (no peek caching here)
// -----------------------------------------------------------------------------

static vitte_token_t lx_next_raw(vitte_lexer_t *lx) {
  if (!lx) return lx_make_tok(VITTE_TOK_EOF, 1, 1, 0, 0);

  // clear sticky error on successful progression; keep last error message for diagnostics
  lx->err_msg = NULL;

  for (;;) {
    lx_skip_ws(lx);

    if (lx_is_eof(lx)) {
      return lx_make_tok(VITTE_TOK_EOF, lx->line, lx->col, lx->pos, 0);
    }

    // comments
    vitte_token_t cmt;
    if (lx_try_skip_or_emit_comment(lx, &cmt)) {
      if (lx->emit_comments) return cmt;
      // if not emitting comments, we already skipped it; loop again
      continue;
    }

    break;
  }

  uint32_t line = lx->line;
  uint32_t col  = lx->col;
  size_t start  = lx->pos;

  unsigned char c = lx_peek_u8(lx);

  // identifiers / keywords
  if (lx_is_ident_start(c)) {
    lx_advance(lx, 1);
    return lx_lex_ident_or_kw(lx, line, col, start);
  }

  // numbers
  if (lx_is_digit_ascii(c)) {
    return lx_lex_number(lx, line, col, start);
  }

  // floats like .5
  if (c == '.' && lx_is_digit_ascii(lx_peek_u8_n(lx, 1))) {
    // consume '.' and digits => float
    lx_advance(lx, 1);
    lx_consume_digits(lx, 10);

    // exponent
    unsigned char e = lx_peek_u8(lx);
    if (e == 'e' || e == 'E') {
      lx_advance(lx, 1);
      if (lx_peek_u8(lx) == '+' || lx_peek_u8(lx) == '-') lx_advance(lx, 1);
      if (!lx_is_digit_ascii(lx_peek_u8(lx))) {
        lx_set_error(lx, "invalid float exponent", start);
        return lx_make_tok(VITTE_TOK_ERROR, line, col, start, lx->pos - start);
      }
      lx_consume_digits(lx, 10);
    }

    return lx_make_tok(VITTE_TOK_FLOAT, line, col, start, lx->pos - start);
  }

  // strings/chars
  if (c == '"') {
    lx_advance(lx, 1);
    return lx_lex_string(lx, line, col, start);
  }

  if (c == '\'') {
    lx_advance(lx, 1);
    return lx_lex_char(lx, line, col, start);
  }

  // operators/punct
  return lx_lex_punct_or_op(lx, line, col, start);
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

#ifndef VITTE_HAVE_LEXER_H

void vitte_lexer_init(vitte_lexer_t *lx, const char *path, const char *src, size_t len) {
  if (!lx) return;
  memset(lx, 0, sizeof(*lx));
  lx->path = path;
  lx->src  = src ? src : "";
  lx->len  = src ? len : 0;
  lx->pos  = 0;
  lx->line = 1;
  lx->col  = 1;
  lx->emit_comments = false;
  lx->nested_block_comments = true;
  lx->has_peek = false;
}

void vitte_lexer_set_emit_comments(vitte_lexer_t *lx, bool on) {
  if (!lx) return;
  lx->emit_comments = on;
  lx->has_peek = false;
}

void vitte_lexer_set_nested_block_comments(vitte_lexer_t *lx, bool on) {
  if (!lx) return;
  lx->nested_block_comments = on;
  lx->has_peek = false;
}

const char *vitte_lexer_last_error_message(const vitte_lexer_t *lx) {
  return lx ? lx->err_msg : NULL;
}

size_t vitte_lexer_last_error_offset(const vitte_lexer_t *lx) {
  return lx ? lx->err_off : 0;
}

vitte_token_t vitte_lexer_next(vitte_lexer_t *lx) {
  if (!lx) return lx_make_tok(VITTE_TOK_EOF, 1, 1, 0, 0);
  if (lx->has_peek) {
    lx->has_peek = false;
    return lx->peek_tok;
  }
  return lx_next_raw(lx);
}

vitte_token_t vitte_lexer_peek(vitte_lexer_t *lx) {
  if (!lx) return lx_make_tok(VITTE_TOK_EOF, 1, 1, 0, 0);
  if (!lx->has_peek) {
    lx->peek_tok = lx_next_raw(lx);
    lx->has_peek = true;
  }
  return lx->peek_tok;
}

// Returns a pointer to token text (not null-terminated). len in out_len.
const char *vitte_token_text(const vitte_lexer_t *lx, const vitte_token_t *t, size_t *out_len) {
  if (out_len) *out_len = 0;
  if (!lx || !t || !lx->src) return NULL;
  if (t->off > lx->len) return NULL;
  size_t n = t->len;
  if (t->off + n > lx->len) n = (lx->off > lx->len) ? 0 : (lx->len - t->off);
  if (out_len) *out_len = n;
  return lx->src + t->off;
}

const char *vitte_token_kind_name(vitte_token_kind_t k) {
  switch (k) {
    case VITTE_TOK_EOF: return "EOF";
    case VITTE_TOK_ERROR: return "ERROR";
    case VITTE_TOK_IDENT: return "IDENT";
    case VITTE_TOK_INT: return "INT";
    case VITTE_TOK_FLOAT: return "FLOAT";
    case VITTE_TOK_STRING: return "STRING";
    case VITTE_TOK_CHAR: return "CHAR";
    case VITTE_TOK_LINE_COMMENT: return "LINE_COMMENT";
    case VITTE_TOK_BLOCK_COMMENT: return "BLOCK_COMMENT";

    case VITTE_TOK_LPAREN: return "(";
    case VITTE_TOK_RPAREN: return ")";
    case VITTE_TOK_LBRACK: return "[";
    case VITTE_TOK_RBRACK: return "]";
    case VITTE_TOK_LBRACE: return "{";
    case VITTE_TOK_RBRACE: return "}";
    case VITTE_TOK_COMMA: return ",";
    case VITTE_TOK_SEMI: return ";";
    case VITTE_TOK_COLON: return ":";
    case VITTE_TOK_COLON2: return "::";
    case VITTE_TOK_DOT: return ".";
    case VITTE_TOK_DOT2: return "..";
    case VITTE_TOK_DOT2_EQ: return "..=";
    case VITTE_TOK_DOT_END: return ".end";

    case VITTE_TOK_PLUS: return "+";
    case VITTE_TOK_MINUS: return "-";
    case VITTE_TOK_STAR: return "*";
    case VITTE_TOK_SLASH: return "/";
    case VITTE_TOK_PERCENT: return "%";
    case VITTE_TOK_CARET: return "^";
    case VITTE_TOK_AMP: return "&";
    case VITTE_TOK_PIPE: return "|";
    case VITTE_TOK_TILDE: return "~";
    case VITTE_TOK_BANG: return "!";
    case VITTE_TOK_QUESTION: return "?";

    case VITTE_TOK_EQ: return "=";
    case VITTE_TOK_EQ2: return "==";
    case VITTE_TOK_BANG_EQ: return "!=";
    case VITTE_TOK_LT: return "<";
    case VITTE_TOK_LE: return "<=";
    case VITTE_TOK_GT: return ">";
    case VITTE_TOK_GE: return ">=";

    case VITTE_TOK_AMP2: return "&&";
    case VITTE_TOK_PIPE2: return "||";
    case VITTE_TOK_SHL: return "<<";
    case VITTE_TOK_SHR: return ">>";

    case VITTE_TOK_PLUS_EQ: return "+=";
    case VITTE_TOK_MINUS_EQ: return "-=";
    case VITTE_TOK_STAR_EQ: return "*=";
    case VITTE_TOK_SLASH_EQ: return "/=";
    case VITTE_TOK_PERCENT_EQ: return "%=";
    case VITTE_TOK_AMP_EQ: return "&=";
    case VITTE_TOK_PIPE_EQ: return "|=";
    case VITTE_TOK_CARET_EQ: return "^=";
    case VITTE_TOK_SHL_EQ: return "<<=";
    case VITTE_TOK_SHR_EQ: return ">>=";

    case VITTE_TOK_ARROW: return "->";
    case VITTE_TOK_FATARROW: return "=>";

    case VITTE_KW_mod: return "kw(mod)";
    case VITTE_KW_use: return "kw(use)";
    case VITTE_KW_export: return "kw(export)";
    case VITTE_KW_from: return "kw(from)";

    case VITTE_KW_type: return "kw(type)";
    case VITTE_KW_struct: return "kw(struct)";
    case VITTE_KW_union: return "kw(union)";
    case VITTE_KW_enum: return "kw(enum)";

    case VITTE_KW_fn: return "kw(fn)";
    case VITTE_KW_scenario: return "kw(scenario)";

    case VITTE_KW_program: return "kw(program)";
    case VITTE_KW_service: return "kw(service)";
    case VITTE_KW_kernel: return "kw(kernel)";
    case VITTE_KW_driver: return "kw(driver)";
    case VITTE_KW_tool: return "kw(tool)";
    case VITTE_KW_pipeline: return "kw(pipeline)";

    case VITTE_KW_let: return "kw(let)";
    case VITTE_KW_const: return "kw(const)";

    case VITTE_KW_if: return "kw(if)";
    case VITTE_KW_elif: return "kw(elif)";
    case VITTE_KW_else: return "kw(else)";
    case VITTE_KW_while: return "kw(while)";
    case VITTE_KW_for: return "kw(for)";
    case VITTE_KW_match: return "kw(match)";

    case VITTE_KW_break: return "kw(break)";
    case VITTE_KW_continue: return "kw(continue)";
    case VITTE_KW_return: return "kw(return)";

    case VITTE_KW_set: return "kw(set)";
    case VITTE_KW_say: return "kw(say)";
    case VITTE_KW_do: return "kw(do)";
    case VITTE_KW_ret: return "kw(ret)";
    case VITTE_KW_when: return "kw(when)";
    case VITTE_KW_loop: return "kw(loop)";

    case VITTE_KW_true: return "kw(true)";
    case VITTE_KW_false: return "kw(false)";
    case VITTE_KW_null: return "kw(null)";
  }
  return "<unknown>";
}

// Debug helper: dump tokens to stdout.
void vitte_lexer_dump_all(vitte_lexer_t *lx) {
  if (!lx) return;
  for (;;) {
    vitte_token_t t = vitte_lexer_next(lx);
    size_t n = 0;
    const char *p = vitte_token_text(lx, &t, &n);

    printf("%6u:%-4u  %-14s  off=%zu len=%zu",
      (unsigned)t.line,
      (unsigned)t.col,
      vitte_token_kind_name(t.kind),
      t.off,
      t.len
    );

    if (p && n) {
      printf("  text=\"");
      // print escaped fragment
      for (size_t i = 0; i < n && i < 80; i++) {
        unsigned char c = (unsigned char)p[i];
        if (c == '\\') { putchar('\\'); putchar('\\'); }
        else if (c == '"') { putchar('\\'); putchar('"'); }
        else if (c == '\n') { putchar('\\'); putchar('n'); }
        else if (c == '\r') { putchar('\\'); putchar('r'); }
        else if (c == '\t') { putchar('\\'); putchar('t'); }
        else if (c < 32 || c == 127) { putchar('.'); }
        else putchar((char)c);
      }
      if (n > 80) printf("...");
      printf("\"");
    }

    if (t.kind == VITTE_TOK_ERROR) {
      const char *m = vitte_lexer_last_error_message(lx);
      if (m) printf("  !! %s", m);
    }

    putchar('\n');

    if (t.kind == VITTE_TOK_EOF) break;
  }
}

#endif // !VITTE_HAVE_LEXER_H

// -----------------------------------------------------------------------------
// Self-test (disabled by default)
// -----------------------------------------------------------------------------

#ifdef VITTE_LEXER_TEST
int main(void) {
  const char *src =
    "mod core/basic\n"
    "fn add(a: i32, b: i32) -> i32\n"
    "  let x = 0x2A + 1_000\n"
    "  if x >= 10 && true\n"
    "    say \"ok\\n\"\n"
    "  .end\n"
    "  // comment\n"
    "  /* block\n"
    "     comment */\n"
    ".end\n";

  vitte_lexer_t lx;
  vitte_lexer_init(&lx, "<test>", src, strlen(src));
  vitte_lexer_set_emit_comments(&lx, true);
  vitte_lexer_dump_all(&lx);
  return 0;
}
#endif
