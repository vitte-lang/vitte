

#ifndef VITTEC_INCLUDE_VITTEC_FRONT_LEXER_H
#define VITTEC_INCLUDE_VITTEC_FRONT_LEXER_H

/*
  lexer.h — vittec front-end lexer (max)

  Goals (bootstrap-friendly):
  - Deterministic tokenization with precise byte spans.
  - Minimal dependencies and a stable public surface.
  - Backward-compatible API for early bootstrap code.
  - Forward-compatible options-based init.

  Conventions:
  - Offsets are byte offsets into the original source buffer.
  - Span ranges are half-open: [lo, hi)
  - Line/col (if tracked) are 1-based.
*/

#include <stdint.h>
#include <stddef.h>

#include "vittec/front/token.h"
#include "vittec/diag/span.h"
#include "vittec/diag/diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTEC_LEXER_API_VERSION 1u

/* -------------------------------------------------------------------------
 * Options
 * ------------------------------------------------------------------------- */

typedef enum vittec_lexer_flag {
  /* Emit NEWLINE tokens instead of treating newlines as whitespace. */
  VITTEC_LEX_KEEP_NEWLINES   = 1u << 0,

  /* Emit COMMENT tokens (if token model supports it); otherwise lexer may ignore. */
  VITTEC_LEX_KEEP_COMMENTS   = 1u << 1,

  /* Track (line,col) counters in the lexer (1-based). */
  VITTEC_LEX_TRACK_LINECOL   = 1u << 2,

  /* Accept CRLF (\r\n) as newline; otherwise treat '\r' as whitespace/invalid per impl. */
  VITTEC_LEX_ACCEPT_CRLF     = 1u << 3,

  /* Accept '\t' as whitespace (and indentation if applicable). */
  VITTEC_LEX_ACCEPT_TABS     = 1u << 4,

  /* Allow non-UTF8 bytes in identifiers/strings (bootstrap tolerance). */
  VITTEC_LEX_ALLOW_NON_UTF8  = 1u << 5,
} vittec_lexer_flag_t;

typedef struct vittec_lexer_options {
  /* Size of this struct in bytes (set by caller). If 0, treated as sizeof(vittec_lexer_options_t). */
  uint32_t size;

  /* Bitmask of vittec_lexer_flag_t. */
  uint32_t flags;

  /* Optional safety limits (0 means “no extra limit”). */
  uint32_t max_token_bytes;
  uint32_t max_string_bytes;
} vittec_lexer_options_t;

/* Initialize options with safe defaults. */
void vittec_lexer_options_init(vittec_lexer_options_t* opt);

/* -------------------------------------------------------------------------
 * Lexer state
 * ------------------------------------------------------------------------- */

typedef struct vittec_lexer {
  const uint8_t* src;   /* source bytes */
  uint32_t len;         /* byte length */
  uint32_t i;           /* current byte offset */

  vittec_file_id_t file_id;

  /* Legacy diagnostics sink (minimal). May be NULL. */
  vittec_diag_sink_t* diags;

  /* Modern diagnostics bag (structured). May be NULL. */
  vittec_diag_bag_t* diag_bag;

  /* Options copied at init. */
  vittec_lexer_options_t opt;

  /* Line/col tracking (only meaningful when VITTEC_LEX_TRACK_LINECOL is set). */
  uint32_t line; /* 1-based */
  uint32_t col;  /* 1-based (byte-based for now) */
} vittec_lexer_t;

/* Save/restore snapshot (useful for parser lookahead). */
typedef struct vittec_lexer_mark {
  uint32_t i;
  uint32_t line;
  uint32_t col;
} vittec_lexer_mark_t;

/* -------------------------------------------------------------------------
 * Inline helpers
 * ------------------------------------------------------------------------- */

static inline int vittec_lexer_eof(const vittec_lexer_t* lx) {
  return !lx || lx->i >= lx->len;
}

static inline uint8_t vittec_lexer_peek_u8(const vittec_lexer_t* lx) {
  return vittec_lexer_eof(lx) ? 0u : lx->src[lx->i];
}

static inline uint8_t vittec_lexer_peek_u8_n(const vittec_lexer_t* lx, uint32_t n) {
  if(!lx) return 0u;
  const uint32_t j = lx->i + n;
  return (j >= lx->len) ? 0u : lx->src[j];
}

static inline vittec_lexer_mark_t vittec_lexer_mark(const vittec_lexer_t* lx) {
  vittec_lexer_mark_t m;
  m.i = lx ? lx->i : 0u;
  m.line = lx ? lx->line : 1u;
  m.col = lx ? lx->col : 1u;
  return m;
}

static inline void vittec_lexer_restore(vittec_lexer_t* lx, vittec_lexer_mark_t m) {
  if(!lx) return;
  lx->i = m.i;
  lx->line = m.line;
  lx->col = m.col;
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/*
  Backward-compatible initializer (legacy).

  IMPORTANT: keep this signature stable.

  Defaults:
  - flags: 0 (skip newlines/comments, no line/col tracking)
  - max_* limits: 0 (no extra limit)
*/
void vittec_lexer_init(
  vittec_lexer_t* lx,
  const char* src,
  uint32_t len,
  uint32_t file_id,
  vittec_diag_sink_t* diags
);

/*
  Extended initializer.

  - `src` is a byte buffer (may contain 0 bytes).
  - `diags` and/or `diag_bag` may be NULL.
  - `opt` may be NULL (defaults).
*/
void vittec_lexer_init_ex(
  vittec_lexer_t* lx,
  const void* src,
  uint32_t len,
  vittec_file_id_t file_id,
  vittec_diag_sink_t* diags,
  vittec_diag_bag_t* diag_bag,
  const vittec_lexer_options_t* opt
);

/*
  Lex next token (consumes input).

  IMPORTANT: keep this name/signature stable.
*/
vittec_token_t vittec_lex_next(vittec_lexer_t* lx);

/* Peek next token without consuming input (mark → next → restore). */
vittec_token_t vittec_lex_peek(vittec_lexer_t* lx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTEC_INCLUDE_VITTEC_FRONT_LEXER_H */