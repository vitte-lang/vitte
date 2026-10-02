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
 * Vitte diagnostic explanation API.
 *
 * This module provides the documentation and discovery layer for the
 * diagnostic registry.
 *
 * Typical uses:
 *
 *     vitte explain E0501
 *     vitte explain VITTE_TYPE_E_MISMATCH
 *     vitte explain --format=json E0501
 *     vitte explain --list
 *     vitte explain --category type
 *     vitte explain --search mismatch
 *
 * The registry remains the authoritative source for diagnostic metadata.
 * The explanation subsystem provides:
 *
 *     - tolerant diagnostic lookup;
 *     - human-readable explanations;
 *     - Markdown documentation;
 *     - JSON output;
 *     - diagnostic listing;
 *     - category filtering;
 *     - full-text search;
 *     - related diagnostics;
 *     - typo suggestions;
 *     - registry validation;
 *     - registry summary/fingerprint.
 */

/* ========================================================================= */
/* Explanation format                                                        */
/* ========================================================================= */

typedef enum vitte_diagnostic_explain_format {
    /*
     * Human-oriented terminal/plain-text documentation.
     */
    VITTE_DIAGNOSTIC_EXPLAIN_TEXT = 0,

    /*
     * Markdown documentation suitable for generated documentation,
     * repositories and language-reference pages.
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
 * Resolve a public or internal diagnostic identifier.
 *
 * Accepted examples:
 *
 *     E0501
 *     e0501
 *     " E0501 "
 *     VITTE_TYPE_E_MISMATCH
 *
 * Public codes are normalized to uppercase.
 *
 * Internal symbolic identifiers are also accepted case-insensitively for
 * interactive CLI convenience.
 *
 * Returns:
 *
 *     pointer to immutable registry entry on success;
 *     NULL if no diagnostic matches.
 *
 * The returned pointer refers to static registry storage and must not be
 * modified or freed.
 */
const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_explain_find(
    const char *code
);

/* ========================================================================= */
/* Main explanation                                                          */
/* ========================================================================= */

/*
 * Render a complete explanation for one diagnostic.
 *
 * include_related:
 *
 *     true:
 *         include related diagnostics from the same category/range.
 *
 *     false:
 *         omit related diagnostics.
 *
 * format selects plain text, Markdown or JSON output.
 *
 * Returns:
 *
 *     VITTE_STATUS_OK
 *         explanation successfully written.
 *
 *     VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *         invalid stream, code, format, or unknown diagnostic.
 *
 *     VITTE_STATUS_ERROR_INVALID_STATE
 *         output stream/rendering failure.
 */
vitte_status_t
vitte_diagnostic_explain(
    FILE *stream,
    const char *code,
    vitte_diagnostic_explain_format_t format,
    bool include_related
);

/*
 * Render only the compact canonical form:
 *
 *     E0501: type mismatch
 *     The actual type does not satisfy the expected type.
 *
 * Useful for:
 *
 *     - command-line quick help;
 *     - editor hover fallback;
 *     - short compiler documentation.
 */
vitte_status_t
vitte_diagnostic_explain_compact(
    FILE *stream,
    const char *code
);

/* ========================================================================= */
/* Unknown-code handling                                                     */
/* ========================================================================= */

/*
 * Explain a diagnostic if it exists.
 *
 * Otherwise, render an "unknown diagnostic code" response and, when a close
 * registered identifier exists, include a suggested code.
 *
 * Example:
 *
 *     vitte explain E051
 *
 * may produce:
 *
 *     unknown diagnostic code: E051
 *     did you mean E0501: type mismatch?
 *
 * The function returns VITTE_STATUS_ERROR_INVALID_ARGUMENT when the requested
 * code is unknown even if a suggestion was successfully rendered.
 */
vitte_status_t
vitte_diagnostic_explain_or_suggest(
    FILE *stream,
    const char *code,
    vitte_diagnostic_explain_format_t format,
    bool include_related
);

/*
 * Find the closest registered diagnostic identifier.
 *
 * Both public codes and internal symbolic identifiers participate.
 *
 * Conservative edit-distance thresholds are used to avoid unrelated
 * suggestions.
 *
 * Returns NULL when no sufficiently close candidate exists.
 */
const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_explain_suggest(
    const char *unknown_code
);

/* ========================================================================= */
/* Registry listing                                                          */
/* ========================================================================= */

/*
 * List registered diagnostics in deterministic registry order.
 *
 * category:
 *
 *     NULL or "":
 *         all categories.
 *
 *     otherwise:
 *         only diagnostics whose category matches case-insensitively.
 *
 * include_deprecated controls whether deprecated registry entries are shown.
 *
 * Human output example:
 *
 *     E0501  type                error             type mismatch
 *     E0502  type                error             type cannot be inferred
 */
vitte_status_t
vitte_diagnostic_explain_list(
    FILE *stream,
    const char *category,
    bool include_deprecated
);

/*
 * Machine-readable equivalent of vitte_diagnostic_explain_list().
 *
 * Output shape:
 *
 * {
 *   "version": 1,
 *   "diagnostics": [
 *     {
 *       "code": "E0501",
 *       "internal": "VITTE_TYPE_E_MISMATCH",
 *       "title": "type mismatch",
 *       "category": "type",
 *       "severity": "error",
 *       "phase": "type-check",
 *       "deprecated": false
 *     }
 *   ]
 * }
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
 * Matching is case-insensitive and currently examines:
 *
 *     - public code;
 *     - internal identifier;
 *     - title;
 *     - category;
 *     - explanation;
 *     - compiler phase name.
 *
 * limit:
 *
 *     0:
 *         unlimited.
 *
 *     > 0:
 *         maximum number of results.
 */
vitte_status_t
vitte_diagnostic_explain_search(
    FILE *stream,
    const char *query,
    size_t limit
);

/*
 * JSON equivalent of vitte_diagnostic_explain_search().
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
 * Print each unique diagnostic category exactly once.
 *
 * Registry order determines category order, keeping output deterministic.
 *
 * Example:
 *
 *     compiler
 *     source
 *     encoding
 *     lexer
 *     syntax
 *     module
 *     type
 *     contract
 *     ...
 */
vitte_status_t
vitte_diagnostic_explain_categories(
    FILE *stream
);

/* ========================================================================= */
/* Registry summary                                                          */
/* ========================================================================= */

/*
 * Print a compact registry summary including:
 *
 *     - total registered diagnostics;
 *     - error/fatal count;
 *     - warning count;
 *     - note count;
 *     - help count;
 *     - registry fingerprint.
 *
 * The fingerprint describes registry metadata. It is not a public diagnostic
 * identifier and must never replace stable E/W/N/H codes.
 */
vitte_status_t
vitte_diagnostic_explain_summary(
    FILE *stream
);

/* ========================================================================= */
/* Registry validation                                                       */
/* ========================================================================= */

/*
 * Validate the diagnostic registry and optionally print a validation report.
 *
 * Checks performed by registry.c may include:
 *
 *     - required fields;
 *     - valid public-code syntax;
 *     - valid severity;
 *     - valid origin;
 *     - severity/public-prefix consistency;
 *     - phase/range consistency;
 *     - duplicate public codes;
 *     - duplicate internal identifiers.
 *
 * stream may be NULL when only the status is needed.
 */
vitte_status_t
vitte_diagnostic_explain_validate_registry(
    FILE *stream
);

/* ========================================================================= */
/* Format utilities                                                          */
/* ========================================================================= */

/*
 * Return the stable textual name of an explanation format.
 *
 * Returns:
 *
 *     "text"
 *     "markdown"
 *     "json"
 *     "unknown"
 */
const char *
vitte_diagnostic_explain_format_name(
    vitte_diagnostic_explain_format_t format
);

/*
 * Parse an explanation format name.
 *
 * Accepted names:
 *
 *     text
 *     human
 *     markdown
 *     md
 *     json
 *
 * Matching is ASCII case-insensitive.
 *
 * Returns true on success.
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
 * Return the fingerprint of the diagnostic registry used by the explanation
 * subsystem.
 *
 * This is a convenience wrapper around:
 *
 *     vitte_diagnostic_registry_fingerprint()
 *
 * Possible uses:
 *
 *     - generated documentation cache invalidation;
 *     - LSP metadata cache invalidation;
 *     - diagnostic protocol compatibility checks;
 *     - golden-test metadata.
 *
 * It must not be used as a user-facing diagnostic code.
 */
uint64_t
vitte_diagnostic_explain_registry_fingerprint(void);

/* ========================================================================= */
/* Convenience wrappers                                                      */
/* ========================================================================= */

/*
 * Human-oriented explanation with related diagnostics enabled.
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
 * Markdown explanation with related diagnostics enabled.
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
 * JSON explanation with related diagnostics enabled.
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
 * Human-oriented explanation without related-diagnostic discovery.
 *
 * Useful for deterministic compact documentation or editor integration where
 * the caller does not want additional recommendations.
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
 * Test whether a diagnostic identifier exists in the registry.
 */
static inline bool
vitte_diagnostic_explain_exists(
    const char *code
)
{
    return vitte_diagnostic_explain_find(
        code
    ) != NULL;
}

/* ========================================================================= */
/* ABI/API notes                                                              */
/* ========================================================================= */

/*
 * Public callers must treat all returned registry entries as immutable.
 *
 * The explanation subsystem does not own FILE streams supplied by callers and
 * never closes them.
 *
 * Search/list functions preserve deterministic registry order.
 *
 * This interface performs no compiler-state mutation and is suitable for:
 *
 *     - CLI commands;
 *     - documentation generators;
 *     - language servers;
 *     - editor integrations;
 *     - test tools;
 *     - compiler debugging utilities.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_EXPLAIN_H */
