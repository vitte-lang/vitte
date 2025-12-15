#ifndef VITTEC_INCLUDE_VITTEC_FRONT_PARSER_H
#define VITTEC_INCLUDE_VITTEC_FRONT_PARSER_H

/*
  parser.h — vittec front-end parser (max, bootstrap-friendly)

  Purpose:
  - Early bootstrap parser that builds a *top-level index* without requiring the
    full Vitte grammar.

  Strategy (bootstrap mode):
  - Scan tokens at top-level:
      module / import / export / fn / scenario / entrypoint
  - For blocks, skip until the matching `.end` (canonical Vitte delimiter).

  Design goals:
  - Deterministic
  - Minimal dependencies
  - Precise spans for diagnostics
  - Forward-compatible options-based API

  Backward compatibility:
  - Keeps `vittec_parse_unit_t` typedef name.
  - Keeps legacy function:
        int vittec_parse_unit(vittec_lexer_t* lx, vittec_parse_unit_t* out);
*/

#include <stdint.h>
#include <stddef.h>

#include "vittec/front/lexer.h"
#include "vittec/diag/span.h"
#include "vittec/support/str.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTEC_PARSER_API_VERSION 1u

/* -------------------------------------------------------------------------
 * Small allocator hook (optional)
 * ------------------------------------------------------------------------- */

typedef void* (*vittec_alloc_fn)(void* user, size_t n);
typedef void* (*vittec_realloc_fn)(void* user, void* p, size_t n);
typedef void  (*vittec_free_fn)(void* user, void* p);

typedef struct vittec_alloc {
  vittec_alloc_fn alloc;
  vittec_realloc_fn realloc;
  vittec_free_fn free;
  void* user;
} vittec_alloc_t;

/* If all function pointers are NULL, implementation should default to malloc/realloc/free. */

/* -------------------------------------------------------------------------
 * Parse options
 * ------------------------------------------------------------------------- */

typedef enum vittec_parse_flag {
  /*
    When enabled, parser stores a token-range span for bodies (best-effort).
    In bootstrap mode, body is typically represented as a byte span (from first
    token after signature to token right before `.end`).
  */
  VITTEC_PARSE_CAPTURE_BODIES      = 1u << 0,

  /*
    Be tolerant: keep scanning even after an error, producing a partial index.
    Useful for IDE/LSP.
  */
  VITTEC_PARSE_RECOVER             = 1u << 1,

  /*
    Accept unknown top-level items by skipping until `.end` or newline.
  */
  VITTEC_PARSE_SKIP_UNKNOWN_TOP    = 1u << 2,

  /*
    Emit extra notes/hints explaining bootstrap limitations.
  */
  VITTEC_PARSE_BOOTSTRAP_NOTES     = 1u << 3,
} vittec_parse_flag_t;

typedef struct vittec_parse_options {
  /* Size of this struct in bytes (set by caller). If 0, treated as sizeof(vittec_parse_options_t). */
  uint32_t size;

  /* Bitmask of vittec_parse_flag_t. */
  uint32_t flags;

  /*
    Safety limits (0 => no extra limit):
    - max_top_items: stop indexing after N items
    - max_params: stop collecting params after N tokens (bootstrap signature)
  */
  uint32_t max_top_items;
  uint32_t max_params;

  /* Optional allocator for dynamic arrays in vittec_parse_unit_t. */
  vittec_alloc_t alloc;
} vittec_parse_options_t;

void vittec_parse_options_init(vittec_parse_options_t* opt);

/* -------------------------------------------------------------------------
 * Top-level index model
 * ------------------------------------------------------------------------- */

typedef enum vittec_top_kind {
  VITTEC_TOP_UNKNOWN   = 0,
  VITTEC_TOP_MODULE    = 1,
  VITTEC_TOP_IMPORT    = 2,
  VITTEC_TOP_EXPORT    = 3,
  VITTEC_TOP_TYPE      = 4,
  VITTEC_TOP_FN        = 5,
  VITTEC_TOP_SCENARIO  = 6,
  VITTEC_TOP_ENTRY     = 7,
} vittec_top_kind_t;

/* A “path” captured by the bootstrap parser (best-effort). */
typedef struct vittec_path {
  vittec_sv_t text;      /* raw slice from source buffer */
  vittec_span_t span;    /* span covering the path */
} vittec_path_t;

/* A function-like declaration captured at top-level. */
typedef struct vittec_fn_decl {
  vittec_sv_t name;      /* function name */
  vittec_span_t name_span;

  vittec_span_t sig_span;   /* span covering signature (best-effort) */
  vittec_span_t body_span;  /* span covering body (best-effort, may be empty) */

  uint8_t has_body;         /* 1 if `.end` was found */
  uint8_t is_main;          /* 1 if name == "main" (bootstrap convenience) */
  uint8_t reserved0;
  uint8_t reserved1;
} vittec_fn_decl_t;

/* A generic top-level item (minimal). */
typedef struct vittec_top_item {
  vittec_top_kind_t kind;
  vittec_span_t span;
  /* For quick tooling, store the keyword text (e.g. "fn", "import"). Optional. */
  vittec_sv_t keyword;
} vittec_top_item_t;

/* -------------------------------------------------------------------------
 * Parse unit
 * -------------------------------------------------------------------------
 *
 * IMPORTANT: This struct is intentionally simple (arrays + counts).
 * The parser implementation owns the allocations (via opt.alloc or defaults).
 */

typedef struct vittec_parse_unit {
  /* Bootstrap convenience: true if a `fn main` was detected. */
  int has_main;

  /* Module name (optional). */
  vittec_sv_t module_name;
  vittec_span_t module_span;

  /* Indexed items (optional, but useful for IDE and deterministic emission). */
  vittec_top_item_t* items;
  uint32_t items_len;
  uint32_t items_cap;

  /* Imports / exports captured as raw paths (best-effort). */
  vittec_path_t* imports;
  uint32_t imports_len;
  uint32_t imports_cap;

  vittec_path_t* exports;
  uint32_t exports_len;
  uint32_t exports_cap;

  /* Functions captured at top-level. */
  vittec_fn_decl_t* fns;
  uint32_t fns_len;
  uint32_t fns_cap;

  /* Allocator snapshot used for freeing. */
  vittec_alloc_t alloc;

  /* Reserved for future expansion without breaking ABI of callers that zero-init. */
  uint32_t reserved_u32[8];
} vittec_parse_unit_t;

/* Initialize / free parse unit (safe to call on zeroed struct). */
void vittec_parse_unit_init(vittec_parse_unit_t* u);
void vittec_parse_unit_free(vittec_parse_unit_t* u);

/* -------------------------------------------------------------------------
 * Parsing API
 * ------------------------------------------------------------------------- */

/*
  Legacy API (KEEP): minimal parser.

  Behavior:
  - Uses default options.
  - Detects `fn main` and skips blocks using `.end`.

  Returns 0 on success, non-zero on failure.
*/
int vittec_parse_unit(vittec_lexer_t* lx, vittec_parse_unit_t* out);

/*
  Options-based API.

  Returns 0 on success, non-zero on failure.
  If VITTEC_PARSE_RECOVER is enabled, the parser may return success while
  emitting diagnostics and producing a partial index.
*/
int vittec_parse_unit_ex(
  vittec_lexer_t* lx,
  vittec_parse_unit_t* out,
  const vittec_parse_options_t* opt
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTEC_INCLUDE_VITTEC_FRONT_PARSER_H */
