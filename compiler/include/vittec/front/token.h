

#ifndef VITTEC_INCLUDE_VITTEC_FRONT_TOKEN_H
#define VITTEC_INCLUDE_VITTEC_FRONT_TOKEN_H

/*
  token.h — vittec token model (max, bootstrap-friendly)

  Goals:
  - Deterministic token stream with precise byte spans.
  - Minimal dependencies: span + string-view.
  - A stable token surface for bootstrap parser (top-level scanning).

  Conventions:
  - Token text is usually a slice into the original source buffer.
  - Offsets are byte offsets; spans are half-open [lo, hi).
  - Keyword detection is based on ASCII matches on the identifier text.

  Notes:
  - The lexer may emit a dedicated keyword token for `.end` (recommended), so the
    parser can close blocks without having to interpret '.' + ident.
*/

#include <stdint.h>
#include <stddef.h>

#include "vittec/diag/span.h"
#include "vittec/support/str.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTEC_TOKEN_API_VERSION 1u

/* -------------------------------------------------------------------------
 * Token kinds
 * ------------------------------------------------------------------------- */

typedef enum vittec_token_kind {
  VITTEC_TOK_ERROR   = 0,
  VITTEC_TOK_EOF     = 1,

  /* Structural */
  VITTEC_TOK_NEWLINE = 2,
  VITTEC_TOK_COMMENT = 3,

  /* Names & keywords */
  VITTEC_TOK_IDENT   = 10,
  VITTEC_TOK_KEYWORD = 11,

  /* Literals */
  VITTEC_TOK_INT     = 20,
  VITTEC_TOK_FLOAT   = 21,
  VITTEC_TOK_STRING  = 22,

  /* Punctuation / operators */
  VITTEC_TOK_PUNCT   = 30,
} vittec_token_kind_t;

/* -------------------------------------------------------------------------
 * Keywords
 * -------------------------------------------------------------------------
 *
 * Keep this list aligned with the language surface and bootstrap parser.
 * New keywords should be appended (do not reorder existing values).
 */

typedef enum vittec_keyword {
  VITTEC_KW_UNKNOWN = 0,

  /* module system */
  VITTEC_KW_MODULE,
  VITTEC_KW_IMPORT,
  VITTEC_KW_EXPORT,
  VITTEC_KW_USE,

  /* types */
  VITTEC_KW_TYPE,
  VITTEC_KW_STRUCT,
  VITTEC_KW_ENUM,
  VITTEC_KW_UNION,

  /* items */
  VITTEC_KW_FN,
  VITTEC_KW_SCN,
  VITTEC_KW_SCENARIO,

  /* entrypoints / kinds */
  VITTEC_KW_PROGRAM,
  VITTEC_KW_SERVICE,
  VITTEC_KW_KERNEL,
  VITTEC_KW_DRIVER,
  VITTEC_KW_TOOL,
  VITTEC_KW_PIPELINE,

  /* statements */
  VITTEC_KW_LET,
  VITTEC_KW_CONST,
  VITTEC_KW_IF,
  VITTEC_KW_ELIF,
  VITTEC_KW_ELSE,
  VITTEC_KW_WHILE,
  VITTEC_KW_FOR,
  VITTEC_KW_MATCH,
  VITTEC_KW_BREAK,
  VITTEC_KW_CONTINUE,
  VITTEC_KW_RETURN,
  VITTEC_KW_RET,

  /* “phrase” sugar */
  VITTEC_KW_SET,
  VITTEC_KW_SAY,
  VITTEC_KW_DO,
  VITTEC_KW_WHEN,
  VITTEC_KW_LOOP,

  /* literals */
  VITTEC_KW_TRUE,
  VITTEC_KW_FALSE,
  VITTEC_KW_NULL,

  /* block delimiter (recommended to be emitted for `.end`) */
  VITTEC_KW_END,
} vittec_keyword_t;

/* -------------------------------------------------------------------------
 * Punctuation / operators
 * -------------------------------------------------------------------------
 *
 * The lexer emits VITTEC_TOK_PUNCT with a punct kind.
 */

typedef enum vittec_punct {
  VITTEC_PUNCT_UNKNOWN = 0,

  /* Delimiters */
  VITTEC_PUNCT_LPAREN,   /* ( */
  VITTEC_PUNCT_RPAREN,   /* ) */
  VITTEC_PUNCT_LBRACK,   /* [ */
  VITTEC_PUNCT_RBRACK,   /* ] */
  VITTEC_PUNCT_COMMA,    /* , */
  VITTEC_PUNCT_DOT,      /* . */
  VITTEC_PUNCT_COLON,    /* : */
  VITTEC_PUNCT_SEMI,     /* ; */

  /* Assignment */
  VITTEC_PUNCT_EQ,       /* = */

  /* Arithmetic */
  VITTEC_PUNCT_PLUS,     /* + */
  VITTEC_PUNCT_MINUS,    /* - */
  VITTEC_PUNCT_STAR,     /* * */
  VITTEC_PUNCT_SLASH,    /* / */
  VITTEC_PUNCT_PERCENT,  /* % */

  /* Bitwise */
  VITTEC_PUNCT_AMP,      /* & */
  VITTEC_PUNCT_PIPE,     /* | */
  VITTEC_PUNCT_CARET,    /* ^ */
  VITTEC_PUNCT_TILDE,    /* ~ */

  /* Logical */
  VITTEC_PUNCT_BANG,     /* ! */

  /* Comparisons */
  VITTEC_PUNCT_LT,       /* < */
  VITTEC_PUNCT_GT,       /* > */
  VITTEC_PUNCT_LE,       /* <= */
  VITTEC_PUNCT_GE,       /* >= */
  VITTEC_PUNCT_EQEQ,     /* == */
  VITTEC_PUNCT_NE,       /* != */

  /* Short-circuit */
  VITTEC_PUNCT_ANDAND,   /* && */
  VITTEC_PUNCT_OROR,     /* || */

  /* Arrows */
  VITTEC_PUNCT_ARROW,    /* -> */
  VITTEC_PUNCT_FATARROW, /* => */

  /* Others (optional) */
  VITTEC_PUNCT_QUESTION, /* ? */
} vittec_punct_t;

/* -------------------------------------------------------------------------
 * Token
 * -------------------------------------------------------------------------
 *
 * Token payload strategy:
 * - For bootstrap, we keep `text` as a slice into the source (no allocation).
 * - `int/float` can be parsed later; lexer may fill numeric payload optionally.
 */

typedef enum vittec_token_flags {
  VITTEC_TOKF_NONE            = 0u,

  /* Token came from a `.end` sequence (keyword END). */
  VITTEC_TOKF_FROM_DOT_END    = 1u << 0,

  /* Lexer had to recover (token synthesized). */
  VITTEC_TOKF_RECOVERED       = 1u << 1,
} vittec_token_flags_t;

typedef struct vittec_token {
  vittec_token_kind_t kind;
  vittec_span_t span;

  /* Raw token text slice (usually points into source buffer). */
  vittec_sv_t text;

  /* Flags for tooling / bootstrap. */
  uint32_t flags;

  /* Discriminated payload for common kinds. */
  union {
    vittec_keyword_t kw; /* when kind==KEYWORD */
    vittec_punct_t punct;/* when kind==PUNCT */
    uint64_t u64;        /* optional numeric payload */
    double f64;          /* optional numeric payload */
  } as;
} vittec_token_t;

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

static inline vittec_token_t vittec_token_make(vittec_token_kind_t k, vittec_span_t sp, vittec_sv_t text) {
  vittec_token_t t;
  t.kind = k;
  t.span = sp;
  t.text = text;
  t.flags = 0u;
  t.as.u64 = 0ull;
  return t;
}

static inline int vittec_token_is_eof(vittec_token_t t) {
  return t.kind == VITTEC_TOK_EOF;
}

static inline int vittec_token_is_error(vittec_token_t t) {
  return t.kind == VITTEC_TOK_ERROR;
}

static inline int vittec_token_is_ident(vittec_token_t t) {
  return t.kind == VITTEC_TOK_IDENT;
}

static inline int vittec_token_is_keyword(vittec_token_t t, vittec_keyword_t kw) {
  return t.kind == VITTEC_TOK_KEYWORD && t.as.kw == kw;
}

static inline int vittec_token_is_punct(vittec_token_t t, vittec_punct_t p) {
  return t.kind == VITTEC_TOK_PUNCT && t.as.punct == p;
}

/* Stable token kind name for debugging/tests. */
static inline const char* vittec_token_kind_name(vittec_token_kind_t k) {
  switch(k) {
    case VITTEC_TOK_ERROR:   return "error";
    case VITTEC_TOK_EOF:     return "eof";
    case VITTEC_TOK_NEWLINE: return "newline";
    case VITTEC_TOK_COMMENT: return "comment";
    case VITTEC_TOK_IDENT:   return "ident";
    case VITTEC_TOK_KEYWORD: return "keyword";
    case VITTEC_TOK_INT:     return "int";
    case VITTEC_TOK_FLOAT:   return "float";
    case VITTEC_TOK_STRING:  return "string";
    case VITTEC_TOK_PUNCT:   return "punct";
    default:                 return "unknown";
  }
}

/* Minimal ASCII string view compare (no <string.h>). */
static inline int vittec_sv_eq_cstr(vittec_sv_t a, const char* b) {
  if(!b) return 0;
  /* compute len of b */
  size_t n = 0;
  while(b[n] != '\0') n++;
  if(a.len != n) return 0;
  for(size_t i=0;i<n;i++) {
    if((unsigned char)a.data[i] != (unsigned char)b[i]) return 0;
  }
  return 1;
}

/* Classify an identifier as a keyword.
   Returns VITTEC_KW_UNKNOWN if not a keyword.
*/
static inline vittec_keyword_t vittec_keyword_from_sv(vittec_sv_t id) {
  /* module system */
  if(vittec_sv_eq_cstr(id, "module"))   return VITTEC_KW_MODULE;
  if(vittec_sv_eq_cstr(id, "import"))   return VITTEC_KW_IMPORT;
  if(vittec_sv_eq_cstr(id, "export"))   return VITTEC_KW_EXPORT;
  if(vittec_sv_eq_cstr(id, "use"))      return VITTEC_KW_USE;

  /* types */
  if(vittec_sv_eq_cstr(id, "type"))     return VITTEC_KW_TYPE;
  if(vittec_sv_eq_cstr(id, "struct"))   return VITTEC_KW_STRUCT;
  if(vittec_sv_eq_cstr(id, "enum"))     return VITTEC_KW_ENUM;
  if(vittec_sv_eq_cstr(id, "union"))    return VITTEC_KW_UNION;

  /* items */
  if(vittec_sv_eq_cstr(id, "fn"))       return VITTEC_KW_FN;
  if(vittec_sv_eq_cstr(id, "scn"))      return VITTEC_KW_SCN;
  if(vittec_sv_eq_cstr(id, "scenario")) return VITTEC_KW_SCENARIO;

  /* entrypoints */
  if(vittec_sv_eq_cstr(id, "program"))  return VITTEC_KW_PROGRAM;
  if(vittec_sv_eq_cstr(id, "service"))  return VITTEC_KW_SERVICE;
  if(vittec_sv_eq_cstr(id, "kernel"))   return VITTEC_KW_KERNEL;
  if(vittec_sv_eq_cstr(id, "driver"))   return VITTEC_KW_DRIVER;
  if(vittec_sv_eq_cstr(id, "tool"))     return VITTEC_KW_TOOL;
  if(vittec_sv_eq_cstr(id, "pipeline")) return VITTEC_KW_PIPELINE;

  /* statements */
  if(vittec_sv_eq_cstr(id, "let"))      return VITTEC_KW_LET;
  if(vittec_sv_eq_cstr(id, "const"))    return VITTEC_KW_CONST;
  if(vittec_sv_eq_cstr(id, "if"))       return VITTEC_KW_IF;
  if(vittec_sv_eq_cstr(id, "elif"))     return VITTEC_KW_ELIF;
  if(vittec_sv_eq_cstr(id, "else"))     return VITTEC_KW_ELSE;
  if(vittec_sv_eq_cstr(id, "while"))    return VITTEC_KW_WHILE;
  if(vittec_sv_eq_cstr(id, "for"))      return VITTEC_KW_FOR;
  if(vittec_sv_eq_cstr(id, "match"))    return VITTEC_KW_MATCH;
  if(vittec_sv_eq_cstr(id, "break"))    return VITTEC_KW_BREAK;
  if(vittec_sv_eq_cstr(id, "continue")) return VITTEC_KW_CONTINUE;
  if(vittec_sv_eq_cstr(id, "return"))   return VITTEC_KW_RETURN;
  if(vittec_sv_eq_cstr(id, "ret"))      return VITTEC_KW_RET;

  /* phrase sugar */
  if(vittec_sv_eq_cstr(id, "set"))      return VITTEC_KW_SET;
  if(vittec_sv_eq_cstr(id, "say"))      return VITTEC_KW_SAY;
  if(vittec_sv_eq_cstr(id, "do"))       return VITTEC_KW_DO;
  if(vittec_sv_eq_cstr(id, "when"))     return VITTEC_KW_WHEN;
  if(vittec_sv_eq_cstr(id, "loop"))     return VITTEC_KW_LOOP;

  /* literals */
  if(vittec_sv_eq_cstr(id, "true"))     return VITTEC_KW_TRUE;
  if(vittec_sv_eq_cstr(id, "false"))    return VITTEC_KW_FALSE;
  if(vittec_sv_eq_cstr(id, "null"))     return VITTEC_KW_NULL;

  /* block delimiter */
  if(vittec_sv_eq_cstr(id, "end"))      return VITTEC_KW_END;

  return VITTEC_KW_UNKNOWN;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTEC_INCLUDE_VITTEC_FRONT_TOKEN_H */