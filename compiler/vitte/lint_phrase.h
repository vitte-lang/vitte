

// SPDX-License-Identifier: MIT
// lint_phrase.h
//
// Phrase-layer linter (max).
//
// The "phrase" surface syntax in Vitte is a sugar layer that desugars into the
// core AST. This linter runs early (typically right after parsing phrase
// statements/expressions) to detect common mistakes with high-quality
// diagnostics.
//
// Goals:
//  - Deterministic diagnostics (stable codes, stable wording hooks).
//  - No dependency on concrete AST layout (supports generic node interface).
//  - Works with either token streams or AST (depending on pipeline stage).
//  - Zero dynamic allocation for the hot path (except optional arena/allocator).
//
// This header intentionally avoids depending on internal struct fields.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Optional includes
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("diag_codes.h")
    #include "diag_codes.h"
    #define VITTE_HAS_DIAG_CODES 1
  #elif __has_include("../vitte/diag_codes.h")
    #include "../vitte/diag_codes.h"
    #define VITTE_HAS_DIAG_CODES 1
  #else
    #define VITTE_HAS_DIAG_CODES 0
  #endif
#else
  #define VITTE_HAS_DIAG_CODES 0
#endif

//------------------------------------------------------------------------------
// Source span
//------------------------------------------------------------------------------

typedef struct vitte_span
{
    // Byte offset into the original source.
    uint32_t off;
    // Byte length.
    uint32_t len;

    // Optional line/col. If unknown, keep 0.
    uint32_t line;
    uint32_t col;
} vitte_span;

static inline vitte_span vitte_span_make(uint32_t off, uint32_t len)
{
    vitte_span s;
    s.off = off;
    s.len = len;
    s.line = 0;
    s.col = 0;
    return s;
}

//------------------------------------------------------------------------------
// Diagnostics sink
//------------------------------------------------------------------------------

// Note: code is a u32, typically from diag_codes.h.
// If diag_codes.h isn't used yet, any non-zero value is acceptable.

typedef struct vitte_diag
{
    uint32_t code;
    uint32_t severity; // compatible with vitte_diag_severity if available
    vitte_span span;

    // Optional stable short name/message (may be NULL).
    const char* name;
    const char* message;

    // Optional details (may be NULL); should be stable across versions if used
    // in golden tests.
    const char* detail;
} vitte_diag;

typedef void (*vitte_diag_emit_fn)(void* user, const vitte_diag* d);

typedef struct vitte_diag_sink
{
    vitte_diag_emit_fn emit;
    void* user;

    // If true, linter may stop early after the first fatal diagnostic.
    bool stop_on_fatal;
} vitte_diag_sink;

static inline vitte_diag_sink vitte_diag_sink_make(vitte_diag_emit_fn emit, void* user)
{
    vitte_diag_sink s;
    s.emit = emit;
    s.user = user;
    s.stop_on_fatal = false;
    return s;
}

//------------------------------------------------------------------------------
// Token interface (optional)
//------------------------------------------------------------------------------

// If the pipeline wants to lint directly on tokens, it can provide tokens as a
// flat array.

typedef struct vitte_token_view
{
    uint32_t kind;
    vitte_span span;

    // Optional small string view for the token text.
    const char* text;
    uint32_t text_len;
} vitte_token_view;

//------------------------------------------------------------------------------
// Generic AST interface
//------------------------------------------------------------------------------

// To lint AST without knowing the node layout, provide this vtable.

typedef struct vitte_ast_any_vtbl
{
    // Required: returns stable kind name (e.g. "say_stmt").
    const char* (*kind_name)(const void* node);

    // Required: child enumeration.
    uint32_t    (*child_count)(const void* node);
    const void* (*child_at)(const void* node, uint32_t i);

    // Optional: name for each child edge (e.g. "lhs", "rhs").
    const char* (*field_name)(const void* node, uint32_t i);

    // Optional: text for leaf nodes (ident/lit).
    const char* (*node_text)(const void* node);

    // Optional: node span.
    vitte_span  (*node_span)(const void* node);

    // Optional: numeric id.
    uint64_t    (*node_id)(const void* node);
} vitte_ast_any_vtbl;

typedef struct vitte_ast_any
{
    const void* node;
    const vitte_ast_any_vtbl* v;
} vitte_ast_any;

//------------------------------------------------------------------------------
// Linter config / instance
//------------------------------------------------------------------------------

typedef struct vitte_lint_phrase_cfg
{
    // Max walk depth (protect against pathological trees).
    uint32_t max_depth;

    // If true, emit style warnings (naming, redundant tokens, etc.).
    bool enable_style;

    // If true, require explicit `.end` for blocks (phrase-specific rule).
    bool require_dot_end;

    // If true, treat unknown phrase constructs as errors (otherwise warnings).
    bool strict;

    // Optional: file name for diagnostics.
    const char* file_name;
} vitte_lint_phrase_cfg;

static inline void vitte_lint_phrase_cfg_init(vitte_lint_phrase_cfg* cfg)
{
    if (!cfg) return;
    cfg->max_depth = 256;
    cfg->enable_style = true;
    cfg->require_dot_end = true;
    cfg->strict = false;
    cfg->file_name = NULL;
}

// Opaque linter state (implementation in lint_phrase.c)
typedef struct vitte_lint_phrase vitte_lint_phrase;

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------

// Initializes an instance. `mem` may be NULL; implementation may malloc.
// Returns false on failure (see vitte_lint_phrase_last_error()).
bool vitte_lint_phrase_init(vitte_lint_phrase** out_lint,
                           const vitte_lint_phrase_cfg* cfg,
                           const vitte_diag_sink* sink,
                           void* mem);

// Dispose and free internal allocations. Safe to call with NULL.
void vitte_lint_phrase_dispose(vitte_lint_phrase* lint);

// Run linter on token stream.
// Returns true if no errors/fatals were emitted.
bool vitte_lint_phrase_run_tokens(vitte_lint_phrase* lint,
                                 const vitte_token_view* toks,
                                 size_t tok_count);

// Run linter on a generic AST.
// Returns true if no errors/fatals were emitted.
bool vitte_lint_phrase_run_ast_any(vitte_lint_phrase* lint,
                                  vitte_ast_any root);

// Convenience: lint without allocating an instance.
bool vitte_lint_phrase_lint_tokens(const vitte_lint_phrase_cfg* cfg,
                                  const vitte_diag_sink* sink,
                                  const vitte_token_view* toks,
                                  size_t tok_count);

bool vitte_lint_phrase_lint_ast_any(const vitte_lint_phrase_cfg* cfg,
                                   const vitte_diag_sink* sink,
                                   vitte_ast_any root);

// Thread-local last error.
const char* vitte_lint_phrase_last_error(void);

//------------------------------------------------------------------------------
// Recommended diagnostic codes (phrase)
//------------------------------------------------------------------------------

// If diag_codes.h exists, define stable codes inside PARSER/SEMA categories.
// Otherwise provide numeric constants.

#if VITTE_HAS_DIAG_CODES

// Parser-ish phrase layer
#define VITTE_DIAG_PHRASE_EXPECTED_DOT_END     VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 200)
#define VITTE_DIAG_PHRASE_UNEXPECTED_KEYWORD   VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 201)
#define VITTE_DIAG_PHRASE_MALFORMED_SET        VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 202)
#define VITTE_DIAG_PHRASE_MALFORMED_SAY        VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 203)
#define VITTE_DIAG_PHRASE_MALFORMED_DO         VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 204)
#define VITTE_DIAG_PHRASE_MALFORMED_WHEN       VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 205)
#define VITTE_DIAG_PHRASE_MALFORMED_LOOP       VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 206)

// Sema-ish phrase layer
#define VITTE_DIAG_PHRASE_UNKNOWN_BUILTIN      VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA,   200)
#define VITTE_DIAG_PHRASE_INVALID_TARGET       VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA,   201)
#define VITTE_DIAG_PHRASE_STYLE_REDUNDANT      VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA,   202)

#else

#define VITTE_DIAG_PHRASE_EXPECTED_DOT_END     0x000200C8u
#define VITTE_DIAG_PHRASE_UNEXPECTED_KEYWORD   0x000200C9u
#define VITTE_DIAG_PHRASE_MALFORMED_SET        0x000200CAu
#define VITTE_DIAG_PHRASE_MALFORMED_SAY        0x000200CBu
#define VITTE_DIAG_PHRASE_MALFORMED_DO         0x000200CCu
#define VITTE_DIAG_PHRASE_MALFORMED_WHEN       0x000200CDu
#define VITTE_DIAG_PHRASE_MALFORMED_LOOP       0x000200CEu
#define VITTE_DIAG_PHRASE_UNKNOWN_BUILTIN      0x000300C8u
#define VITTE_DIAG_PHRASE_INVALID_TARGET       0x000300C9u
#define VITTE_DIAG_PHRASE_STYLE_REDUNDANT      0x000300CAu

#endif

#ifdef __cplusplus
} // extern "C"
#endif
