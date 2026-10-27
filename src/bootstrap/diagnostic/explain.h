#ifndef VITTE_DIAGNOSTIC_EXPLAIN_H
#define VITTE_DIAGNOSTIC_EXPLAIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "diagnostic.h"
#include "registry.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Vitte diagnostic explanation subsystem.
 *
 * This module is the public documentation, discovery and introspection layer
 * built on top of the diagnostic registry.
 *
 * Typical commands:
 *
 *     vitte explain E0501
 *     vitte explain VITTE_TYPE_E_MISMATCH
 *     vitte explain --format=json E0501
 *     vitte explain --list
 *     vitte explain --category type
 *     vitte explain --search mismatch
 *
 * The diagnostic registry remains authoritative for:
 *
 *     - stable public E/W/N/H codes;
 *     - internal symbolic identifiers;
 *     - default severity;
 *     - compiler origin/phase;
 *     - category;
 *     - title;
 *     - canonical explanation;
 *     - deprecation state.
 *
 * The explanation layer adds:
 *
 *     - tolerant lookup;
 *     - code normalization;
 *     - human-readable rendering;
 *     - Markdown rendering;
 *     - JSON rendering;
 *     - related-diagnostic discovery;
 *     - ranked full-text search;
 *     - typo correction;
 *     - category enumeration;
 *     - registry summary;
 *     - registry validation;
 *     - registry fingerprint access;
 *     - CLI/bootstrap adapters.
 *
 * Design properties:
 *
 *     - no global mutable state;
 *     - deterministic output;
 *     - immutable registry references;
 *     - no ownership transfer;
 *     - no compiler-state mutation;
 *     - C17;
 *     - safe for CLI/editor/LSP/documentation consumers.
 */

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

/*
 * Version of the JSON document shape emitted by this subsystem.
 *
 * This version describes the explanation/list/search JSON representation,
 * not the compiler diagnostic protocol as a whole.
 */
#define VITTE_DIAGNOSTIC_EXPLAIN_JSON_VERSION 1u

/*
 * Maximum number of related diagnostics returned by the default explanation
 * renderer.
 *
 * The implementation ranks related diagnostics and keeps the strongest
 * candidates.
 */
#define VITTE_DIAGNOSTIC_EXPLAIN_RELATED_LIMIT ((size_t)8u)

/* ========================================================================= */
/* Explanation format                                                        */
/* ========================================================================= */

typedef enum vitte_diagnostic_explain_format {
    /*
     * Human-oriented plain text suitable for terminals.
     */
    VITTE_DIAGNOSTIC_EXPLAIN_TEXT = 0,

    /*
     * Markdown suitable for generated documentation, repositories and
     * language-reference pages.
     */
    VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN,

    /*
     * Machine-readable JSON.
     */
    VITTE_DIAGNOSTIC_EXPLAIN_JSON
} vitte_diagnostic_explain_format_t;

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

/*
 * Resolve a diagnostic identifier.
 *
 * Accepted forms include:
 *
 *     E0501
 *     e0501
 *     " E0501 "
 *     VITTE_TYPE_E_MISMATCH
 *     vitte_type_e_mismatch
 *
 * Public diagnostic codes are normalized to uppercase.
 *
 * Internal symbolic identifiers are matched case-insensitively for
 * interactive convenience. The canonical identifier stored in the registry
 * is never modified.
 *
 * Returns:
 *
 *     pointer to immutable registry storage
 *         diagnostic found.
 *
 *     NULL
 *         invalid input or no matching diagnostic.
 *
 * Lifetime:
 *
 *     The returned pointer is owned by the registry and remains valid for the
 *     lifetime guaranteed by registry.c. The caller must not modify or free
 *     it.
 */
const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_explain_find(
    const char *code
);

/*
 * Test whether a public or internal diagnostic identifier exists.
 *
 * This is equivalent to:
 *
 *     vitte_diagnostic_explain_find(code) != NULL
 */
static inline bool
vitte_diagnostic_explain_exists(
    const char *code
)
{
    return vitte_diagnostic_explain_find(code) != NULL;
}

/* ========================================================================= */
/* Complete explanation                                                      */
/* ========================================================================= */

/*
 * Render a complete explanation for one registered diagnostic.
 *
 * include_related:
 *
 *     true
 *         discover and render related diagnostics.
 *
 *     false
 *         render only the requested diagnostic.
 *
 * Related diagnostics are ranked deterministically using registry metadata,
 * including:
 *
 *     - public diagnostic range;
 *     - category;
 *     - compiler phase;
 *     - severity.
 *
 * format:
 *
 *     VITTE_DIAGNOSTIC_EXPLAIN_TEXT
 *     VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN
 *     VITTE_DIAGNOSTIC_EXPLAIN_JSON
 *
 * Returns:
 *
 *     VITTE_STATUS_OK
 *         explanation rendered successfully.
 *
 *     VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *         invalid stream;
 *         invalid code;
 *         unknown diagnostic;
 *         unsupported format.
 *
 *     VITTE_STATUS_ERROR_INVALID_STATE
 *         stream/output failure.
 *
 * The function never closes stream.
 */
vitte_status_t
vitte_diagnostic_explain(
    FILE *stream,
    const char *code,
    vitte_diagnostic_explain_format_t format,
    bool include_related
);

/* ========================================================================= */
/* Compact explanation                                                       */
/* ========================================================================= */

/*
 * Render a compact explanation.
 *
 * Typical output:
 *
 *     E0501: type mismatch
 *     The actual type does not satisfy the expected type.
 *     help: convert the value or change the expected type.
 *
 * Guidance is emitted when help metadata exists for the diagnostic code.
 *
 * Intended consumers:
 *
 *     - quick CLI help;
 *     - editor hover fallback;
 *     - compiler debug tools;
 *     - compact generated documentation.
 */
vitte_status_t
vitte_diagnostic_explain_compact(
    FILE *stream,
    const char *code
);

/* ========================================================================= */
/* Unknown-code recovery                                                     */
/* ========================================================================= */

/*
 * Find the closest registered diagnostic identifier.
 *
 * Public codes and internal symbolic identifiers participate.
 *
 * Matching uses conservative case-insensitive edit distance. Thresholds are
 * intentionally restrictive so unrelated diagnostics are not recommended.
 *
 * Exact registered identifiers resolve to their own entry.
 *
 * Returns:
 *
 *     immutable registry entry
 *         sufficiently close match exists.
 *
 *     NULL
 *         input is invalid or no candidate is sufficiently close.
 */
const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_explain_suggest(
    const char *unknown_code
);

/*
 * Explain a diagnostic when it exists.
 *
 * When it does not exist, render a structured unknown-code response and,
 * where possible, a typo suggestion.
 *
 * Text example:
 *
 *     unknown diagnostic code: E051
 *     did you mean E0501: type mismatch?
 *
 * JSON example:
 *
 *     {
 *       "error": "unknown diagnostic code",
 *       "query": "E051",
 *       "suggestion": {
 *         "code": "E0501",
 *         "internal": "VITTE_TYPE_E_MISMATCH",
 *         "title": "type mismatch"
 *       }
 *     }
 *
 * Important:
 *
 *     Rendering a useful suggestion does not make the original identifier
 *     valid.
 *
 * Therefore an unknown diagnostic returns:
 *
 *     VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *
 * even when the error response itself was rendered successfully.
 */
vitte_status_t
vitte_diagnostic_explain_or_suggest(
    FILE *stream,
    const char *code,
    vitte_diagnostic_explain_format_t format,
    bool include_related
);

/* ========================================================================= */
/* Registry listing                                                          */
/* ========================================================================= */

/*
 * List diagnostics.
 *
 * category:
 *
 *     NULL
 *     ""
 *         list every category.
 *
 *     non-empty string
 *         case-insensitive exact category filter.
 *
 * Surrounding whitespace in a non-empty category filter is ignored.
 *
 * include_deprecated:
 *
 *     true
 *         include deprecated diagnostics.
 *
 *     false
 *         exclude deprecated diagnostics.
 *
 * Human output contains:
 *
 *     public code
 *     category
 *     severity
 *     compiler phase
 *     title
 *     deprecation marker when applicable
 *
 * Registry iteration order is deterministic.
 */
vitte_status_t
vitte_diagnostic_explain_list(
    FILE *stream,
    const char *category,
    bool include_deprecated
);

/*
 * JSON equivalent of vitte_diagnostic_explain_list().
 *
 * Output schema version 1:
 *
 * {
 *   "version": 1,
 *   "category": "type",
 *   "diagnostics": [
 *     {
 *       "code": "E0501",
 *       "internal": "VITTE_TYPE_E_MISMATCH",
 *       "title": "type mismatch",
 *       "category": "type",
 *       "severity": "error",
 *       "phase": "type-check",
 *       "range": "type",
 *       "deprecated": false
 *     }
 *   ]
 * }
 *
 * category is JSON null when no filter is active.
 */
vitte_status_t
vitte_diagnostic_explain_list_json(
    FILE *stream,
    const char *category,
    bool include_deprecated
);

/* ========================================================================= */
/* Search                                                                    */
/* ========================================================================= */

/*
 * Search the diagnostic registry.
 *
 * The query is trimmed before matching.
 *
 * Matching is ASCII case-insensitive.
 *
 * Search considers:
 *
 *     - exact public code;
 *     - exact internal identifier;
 *     - exact title;
 *     - exact category;
 *     - public-code substring;
 *     - internal-identifier substring;
 *     - title substring;
 *     - category substring;
 *     - compiler phase;
 *     - severity;
 *     - canonical explanation;
 *     - diagnostic guidance/help.
 *
 * Results are ranked rather than merely emitted in registry order.
 *
 * Stronger matches are preferred:
 *
 *     exact code
 *         >
 *     exact internal identifier
 *         >
 *     exact title/category
 *         >
 *     code/title/category substring
 *         >
 *     phase/severity
 *         >
 *     explanation/help text
 *
 * Ties are resolved deterministically.
 *
 * limit:
 *
 *     0
 *         unlimited.
 *
 *     > 0
 *         maximum number of rendered matches.
 *
 * The search may use temporary dynamic memory internally. Allocation failure
 * is reported through the normal Vitte status mechanism.
 */
vitte_status_t
vitte_diagnostic_explain_search(
    FILE *stream,
    const char *query,
    size_t limit
);

/*
 * Machine-readable equivalent of vitte_diagnostic_explain_search().
 *
 * Output schema version 1:
 *
 * {
 *   "version": 1,
 *   "query": "mismatch",
 *   "totalMatches": 4,
 *   "results": [
 *     {
 *       "code": "E0501",
 *       "internal": "VITTE_TYPE_E_MISMATCH",
 *       "title": "type mismatch",
 *       "category": "type",
 *       "severity": "error",
 *       "phase": "type-check",
 *       "explanation": "...",
 *       "score": 1100
 *     }
 *   ]
 * }
 *
 * totalMatches is the total number of registry matches before application of
 * limit.
 */
vitte_status_t
vitte_diagnostic_explain_search_json(
    FILE *stream,
    const char *query,
    size_t limit
);

/* ========================================================================= */
/* Categories                                                                */
/* ========================================================================= */

/*
 * Print every unique diagnostic category exactly once.
 *
 * Category equality is ASCII case-insensitive.
 *
 * The first registry occurrence determines output order, making enumeration
 * deterministic without requiring allocation or sorting.
 *
 * Example:
 *
 *     compiler
 *     source
 *     encoding
 *     lexer
 *     syntax
 *     import
 *     name
 *     type
 *     contract
 *     constant
 *     hir
 *     ir
 *     backend
 *     linker
 */
vitte_status_t
vitte_diagnostic_explain_categories(
    FILE *stream
);

/* ========================================================================= */
/* Registry summary                                                          */
/* ========================================================================= */

/*
 * Render a compact registry summary.
 *
 * Current information includes:
 *
 *     - total diagnostics;
 *     - non-fatal error count;
 *     - fatal count;
 *     - warning count;
 *     - note count;
 *     - help count;
 *     - deprecated entry count;
 *     - registry fingerprint.
 *
 * Example:
 *
 *     Vitte diagnostic registry
 *
 *     Total:      143
 *     Errors:     102
 *     Fatal:      4
 *     Warnings:   22
 *     Notes:      8
 *     Help:       7
 *     Deprecated: 0
 *     Registry fingerprint: 0123456789abcdef
 *
 * The registry fingerprint is metadata and must never be used as a
 * user-facing diagnostic identifier.
 */
vitte_status_t
vitte_diagnostic_explain_summary(
    FILE *stream
);

/* ========================================================================= */
/* Registry validation                                                       */
/* ========================================================================= */

/*
 * Validate the complete diagnostic registry.
 *
 * Validation itself is delegated to registry.c so there is one authoritative
 * implementation of registry invariants.
 *
 * Depending on registry.c, validation may check:
 *
 *     - required metadata;
 *     - stable public-code syntax;
 *     - valid severity;
 *     - valid compiler origin;
 *     - severity/public-prefix consistency;
 *     - public-range/origin consistency;
 *     - duplicate public codes;
 *     - duplicate internal identifiers;
 *     - deprecated-entry invariants;
 *     - registry ordering invariants.
 *
 * stream:
 *
 *     NULL
 *         validate silently.
 *
 *     non-NULL
 *         additionally render a validation report.
 *
 * The report includes:
 *
 *     - valid/invalid state;
 *     - entry count;
 *     - invalid entry count;
 *     - duplicate internal identifier count;
 *     - duplicate public-code count;
 *     - first invalid index when available;
 *     - registry fingerprint.
 *
 * The returned status is the status produced by registry validation unless
 * report rendering itself fails.
 */
vitte_status_t
vitte_diagnostic_explain_validate_registry(
    FILE *stream
);

/* ========================================================================= */
/* Format utilities                                                          */
/* ========================================================================= */

/*
 * Return the stable textual name for an explanation format.
 *
 * Returns:
 *
 *     VITTE_DIAGNOSTIC_EXPLAIN_TEXT
 *         "text"
 *
 *     VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN
 *         "markdown"
 *
 *     VITTE_DIAGNOSTIC_EXPLAIN_JSON
 *         "json"
 *
 *     unknown enum value
 *         "unknown"
 *
 * Returned strings have static storage duration.
 */
const char *
vitte_diagnostic_explain_format_name(
    vitte_diagnostic_explain_format_t format
);

/*
 * Parse an explanation-format name.
 *
 * Surrounding whitespace is ignored.
 *
 * Matching is ASCII case-insensitive.
 *
 * Accepted names:
 *
 *     text
 *     human
 *     plain
 *
 *     markdown
 *     md
 *
 *     json
 *
 * On success:
 *
 *     *format is updated;
 *     true is returned.
 *
 * On failure:
 *
 *     *format is not required to be modified;
 *     false is returned.
 */
bool
vitte_diagnostic_explain_parse_format(
    const char *text,
    vitte_diagnostic_explain_format_t *format
);

/* ========================================================================= */
/* Registry fingerprint                                                      */
/* ========================================================================= */

/*
 * Return the fingerprint of the registry observed by the explanation layer.
 *
 * This is a convenience wrapper around:
 *
 *     vitte_diagnostic_registry_fingerprint()
 *
 * Suitable uses:
 *
 *     - generated-documentation cache invalidation;
 *     - LSP metadata cache invalidation;
 *     - diagnostic metadata synchronization;
 *     - golden-test metadata;
 *     - compiler/tooling compatibility diagnostics.
 *
 * Unsuitable uses:
 *
 *     - user-facing diagnostic identifier;
 *     - replacement for stable E/W/N/H codes;
 *     - persistent semantic identity of one diagnostic.
 */
uint64_t
vitte_diagnostic_explain_registry_fingerprint(void);

/* ========================================================================= */
/* Convenience rendering wrappers                                            */
/* ========================================================================= */

/*
 * Render a complete human explanation including related diagnostics.
 */
static inline vitte_status_t
vitte_diagnostic_explain_text(
    FILE *stream,
    const char *code
)
{
    return vitte_diagnostic_explain(
        stream,
        code,
        VITTE_DIAGNOSTIC_EXPLAIN_TEXT,
        true
    );
}

/*
 * Render a complete Markdown explanation including related diagnostics.
 */
static inline vitte_status_t
vitte_diagnostic_explain_markdown(
    FILE *stream,
    const char *code
)
{
    return vitte_diagnostic_explain(
        stream,
        code,
        VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN,
        true
    );
}

/*
 * Render a complete JSON explanation including related diagnostics.
 */
static inline vitte_status_t
vitte_diagnostic_explain_json(
    FILE *stream,
    const char *code
)
{
    return vitte_diagnostic_explain(
        stream,
        code,
        VITTE_DIAGNOSTIC_EXPLAIN_JSON,
        true
    );
}

/*
 * Render a human explanation without related-diagnostic discovery.
 *
 * Useful when:
 *
 *     - exact deterministic single-entry documentation is required;
 *     - an editor already has its own related-diagnostic UI;
 *     - minimal output is desired.
 */
static inline vitte_status_t
vitte_diagnostic_explain_text_only(
    FILE *stream,
    const char *code
)
{
    return vitte_diagnostic_explain(
        stream,
        code,
        VITTE_DIAGNOSTIC_EXPLAIN_TEXT,
        false
    );
}

/*
 * Render Markdown without related diagnostics.
 */
static inline vitte_status_t
vitte_diagnostic_explain_markdown_only(
    FILE *stream,
    const char *code
)
{
    return vitte_diagnostic_explain(
        stream,
        code,
        VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN,
        false
    );
}

/*
 * Render JSON without related diagnostics.
 */
static inline vitte_status_t
vitte_diagnostic_explain_json_only(
    FILE *stream,
    const char *code
)
{
    return vitte_diagnostic_explain(
        stream,
        code,
        VITTE_DIAGNOSTIC_EXPLAIN_JSON,
        false
    );
}

/* ========================================================================= */
/* CLI/bootstrap bridge                                                      */
/* ========================================================================= */

/*
 * Type-neutral existence adapter used by bootstrap/CLI code.
 *
 * Kept as a real external function rather than an inline alias because
 * bootstrap components may intentionally depend on cli_bridge.h without
 * importing the complete explanation interface.
 */
bool
vitte_diagnostic_cli_code_exists(
    const char *code
);

/*
 * Type-neutral explanation adapter used by bootstrap/CLI code.
 *
 * json:
 *
 *     false
 *         render plain text.
 *
 *     true
 *         render JSON.
 *
 * Related-diagnostic discovery is disabled for this compact bridge.
 *
 * Returns true only when the requested diagnostic exists and rendering
 * succeeds.
 */
bool
vitte_diagnostic_explain_cli(
    FILE *stream,
    const char *code,
    bool json
);

/* ========================================================================= */
/* API invariants                                                            */
/* ========================================================================= */

/*
 * Registry ownership
 * ------------------
 *
 * Every vitte_diagnostic_registry_entry_t pointer returned by this interface
 * is borrowed immutable registry storage.
 *
 * Callers must never:
 *
 *     - free it;
 *     - mutate it;
 *     - assume ownership;
 *     - persist it beyond the registry's documented lifetime.
 *
 *
 * FILE ownership
 * --------------
 *
 * FILE streams are always owned by the caller.
 *
 * The explanation subsystem:
 *
 *     - writes to them;
 *     - checks write failures;
 *     - never closes them.
 *
 *
 * Determinism
 * -----------
 *
 * Exact lookup is registry-defined.
 *
 * Category enumeration follows first registry occurrence.
 *
 * Search results use deterministic ranking and deterministic tie-breaking.
 *
 * Related diagnostics use deterministic metadata ranking.
 *
 *
 * Compiler state
 * --------------
 *
 * This subsystem performs no mutation of:
 *
 *     - AST;
 *     - HIR;
 *     - IR;
 *     - source maps;
 *     - compiler context;
 *     - diagnostic bags;
 *     - diagnostic engine state.
 *
 * It is therefore suitable for:
 *
 *     - CLI commands;
 *     - compiler help;
 *     - documentation generators;
 *     - language servers;
 *     - editor integrations;
 *     - test tools;
 *     - golden tests;
 *     - bootstrap tooling;
 *     - diagnostic registry inspection.
 *
 *
 * Stable identity
 * ---------------
 *
 * Public E/W/N/H codes are the stable user-facing identity.
 *
 * Internal symbolic identifiers are implementation/tooling identifiers.
 *
 * Registry fingerprints identify a registry metadata state, not an
 * individual diagnostic.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_EXPLAIN_H */