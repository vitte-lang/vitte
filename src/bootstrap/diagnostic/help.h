#ifndef VITTE_DIAGNOSTIC_HELP_H
#define VITTE_DIAGNOSTIC_HELP_H

#include <stdbool.h>
#include <stddef.h>

#include "diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Vitte diagnostic contextual-help API.
 *
 * This module provides the remediation layer of the diagnostic system.
 *
 * It converts:
 *
 *     diagnostic code
 *     + structured semantic context
 *     + source span
 *     + labels
 *     + causes
 *     + contract information
 *
 * into conservative, actionable help intended for:
 *
 *     - terminal diagnostics;
 *     - IDE/LSP diagnostics;
 *     - compiler explanations;
 *     - documentation;
 *     - tests;
 *     - debugging tools.
 *
 * The diagnostic registry remains authoritative for:
 *
 *     - public diagnostic identifiers;
 *     - internal identifiers;
 *     - severity;
 *     - origin;
 *     - category;
 *     - title;
 *     - explanation metadata.
 *
 * This module is authoritative only for generic remediation policy and
 * contextual help formatting.
 *
 * Important distinction:
 *
 *     help
 *         Human-readable guidance explaining what the user can do.
 *
 *     suggestion
 *         Structured recommendation that may have confidence/applicability.
 *
 *     fix-it
 *         Exact machine-applicable source edit.
 *
 * help.c must never fabricate a fix-it merely because a textual correction
 * appears likely.
 */

/* ========================================================================= */
/* Generic help policy                                                       */
/* ========================================================================= */

/*
 * Return the generic remediation policy associated with a stable public
 * diagnostic code.
 *
 * Example:
 *
 *     const char *help;
 *
 *     help = vitte_diagnostic_help_for_code("E0501");
 *
 * The returned text describes the generic action appropriate for the
 * diagnostic:
 *
 *     "Make the expression satisfy the required type; ..."
 *
 * The returned pointer refers to immutable static storage.
 *
 * The caller must not modify or free it.
 *
 * Lookup is ASCII case-insensitive for convenience:
 *
 *     E0501
 *     e0501
 *
 * both resolve to the same rule.
 *
 * Returns:
 *
 *     non-NULL
 *         registered remediation policy.
 *
 *     NULL
 *         code is NULL, empty, or has no registered help policy.
 *
 * This function performs no registry lookup and therefore accepts only the
 * public diagnostic identifier used by the help policy table.
 */
const char *
vitte_diagnostic_help_for_code(
    const char *code
);

/* ========================================================================= */
/* Explicit help detection                                                   */
/* ========================================================================= */

/*
 * Return true when a diagnostic already contains at least one non-empty
 * explicit HELP annotation.
 *
 * Explicit HELP annotations are source/phase-specific guidance supplied by
 * the producer that emitted the diagnostic.
 *
 * They have priority over generic help because the producer generally has
 * more precise information about the failure.
 *
 * Returns false when:
 *
 *     - diagnostic is NULL;
 *     - no HELP annotation exists;
 *     - HELP annotations exist but contain no message.
 */
bool
vitte_diagnostic_has_help(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Contextual help generation                                                */
/* ========================================================================= */

/*
 * Generate contextual remediation help for one diagnostic.
 *
 * The formatter combines the best available information in this order:
 *
 *     1. explicit HELP annotation;
 *     2. diagnostic-specific structured remediation;
 *     3. generic remediation policy for the stable public code;
 *     4. conservative fallback guidance.
 *
 * Structured information may include:
 *
 *     - public diagnostic code;
 *     - internal diagnostic identifier;
 *     - severity;
 *     - origin;
 *     - source span;
 *     - source excerpt;
 *     - symbol;
 *     - expected type;
 *     - actual type;
 *     - primary label;
 *     - detailed failure message;
 *     - cause;
 *     - contract expression;
 *     - contract reason;
 *     - module;
 *     - procedure.
 *
 * Example result for a type mismatch:
 *
 *     Provide a value of type `i32` instead of `string` for `value`.
 *     If the text represents user or file input, parse it as a numeric
 *     value and handle invalid input before using the result.
 *     See `vitte explain E0501`.
 *
 * Example result for an unresolved name:
 *
 *     Declare or import `count` in this scope, or correct its spelling to
 *     match a visible declaration. See `vitte explain E0401`.
 *
 * sources:
 *
 *     Optional source manager used to extract the exact source fragment
 *     covered by the diagnostic's primary span.
 *
 *     May be NULL.
 *
 *     Source text is used only when:
 *
 *         - the source exists;
 *         - the span is valid;
 *         - the span is non-empty;
 *         - the complete excerpt fits in the internal bounded buffer;
 *         - the excerpt contains valid UTF-8;
 *         - the excerpt contains no unsafe control characters;
 *         - the excerpt is suitable for single-line quoted help.
 *
 *     If any of these conditions fail, the source excerpt is omitted rather
 *     than truncated or presented as exact source text.
 *
 * buffer:
 *
 *     Destination for the generated NUL-terminated help message.
 *
 * capacity:
 *
 *     Total size of buffer including the terminating NUL byte.
 *
 * Returns true when non-empty help was generated.
 *
 * Returns false when:
 *
 *     - buffer is NULL;
 *     - capacity is zero;
 *     - diagnostic is NULL;
 *     - the diagnostic severity does not require remediation help;
 *     - formatting failed;
 *     - no usable help could be generated.
 *
 * Buffer guarantees:
 *
 *     - buffer is NUL-terminated whenever capacity > 0;
 *     - no write occurs outside [buffer, buffer + capacity);
 *     - generated source excerpts are never intentionally split inside a
 *       UTF-8 sequence;
 *     - terminal control characters from source excerpts are rejected;
 *     - the function does not allocate memory;
 *     - the function does not mutate diagnostic or sources.
 *
 * NOTE and HELP diagnostics normally do not recursively generate remediation
 * help. WARNING, ERROR and FATAL diagnostics may generate it.
 *
 * This function produces textual guidance only. It does not add annotations
 * to diagnostic and does not create suggestions or fix-its.
 */
bool
vitte_diagnostic_format_help(
    const vitte_diagnostic_t *diagnostic,
    const vitte_source_context_t *sources,
    char *buffer,
    size_t capacity
);

/* ========================================================================= */
/* Help-rule introspection                                                   */
/* ========================================================================= */

/*
 * Return the number of generic remediation policies compiled into help.c.
 *
 * This API is primarily intended for:
 *
 *     - tests;
 *     - registry validation;
 *     - generated documentation;
 *     - compiler development tools.
 *
 * Rule order is deterministic.
 */
size_t
vitte_diagnostic_help_rule_count(void);

/*
 * Return the public diagnostic code of the remediation rule at index.
 *
 * Example:
 *
 *     "E0501"
 *
 * The returned pointer refers to immutable static storage.
 *
 * Returns NULL when index is outside the rule table.
 */
const char *
vitte_diagnostic_help_rule_code(
    size_t index
);

/*
 * Return the generic remediation text of the rule at index.
 *
 * The returned pointer refers to immutable static storage.
 *
 * Returns NULL when index is outside the rule table.
 */
const char *
vitte_diagnostic_help_rule_text(
    size_t index
);

/* ========================================================================= */
/* Help-policy validation                                                    */
/* ========================================================================= */

/*
 * Validate synchronization between the diagnostic registry and help policy.
 *
 * Checks:
 *
 *     1. every registered WARNING/ERROR/FATAL diagnostic requiring
 *        remediation has a help rule;
 *
 *     2. every help rule refers to an existing public registry code;
 *
 *     3. no public diagnostic code appears more than once in the help table.
 *
 * NOTE and HELP diagnostics are informational and are not required to have
 * remediation policies.
 *
 * missing_count:
 *
 *     Receives the number of registered diagnostics without a corresponding
 *     help policy.
 *
 * orphan_count:
 *
 *     Receives the number of help rules whose public diagnostic code does not
 *     exist in the registry.
 *
 * duplicate_count:
 *
 *     Receives the number of duplicate rule pairs detected in the help table.
 *
 * Any output pointer may be NULL.
 *
 * Returns true only when:
 *
 *     missing_count   == 0
 *     orphan_count    == 0
 *     duplicate_count == 0
 *
 * This function performs no allocation and does not mutate registry state.
 *
 * It is intended to be used by tests so adding a new diagnostic without
 * remediation policy becomes an immediately visible failure.
 */
bool
vitte_diagnostic_help_validate_rules(
    size_t *missing_count,
    size_t *orphan_count,
    size_t *duplicate_count
);

/* ========================================================================= */
/* Convenience                                                               */
/* ========================================================================= */

/*
 * Return true when a generic help policy exists for public_code.
 *
 * This does not inspect explicit HELP annotations attached to an individual
 * diagnostic.
 */
static inline bool
vitte_diagnostic_help_has_policy(
    const char *public_code
)
{
    return vitte_diagnostic_help_for_code(
        public_code
    ) != NULL;
}

/*
 * Return true when the diagnostic has either:
 *
 *     - an explicit HELP annotation; or
 *     - a generic policy directly identifiable from its public code.
 *
 * This is intentionally a cheap test.
 *
 * It does not perform the richer internal/public registry resolution used by
 * vitte_diagnostic_format_help().
 */
static inline bool
vitte_diagnostic_help_available(
    const vitte_diagnostic_t *diagnostic
)
{
    if (diagnostic == NULL) {
        return false;
    }

    if (vitte_diagnostic_has_help(
            diagnostic
        )) {
        return true;
    }

    if (diagnostic->short_code != NULL &&
        diagnostic->short_code[0] != '\0' &&
        vitte_diagnostic_help_for_code(
            diagnostic->short_code
        ) != NULL) {
        return true;
    }

    if (diagnostic->code != NULL &&
        diagnostic->code[0] != '\0' &&
        vitte_diagnostic_help_for_code(
            diagnostic->code
        ) != NULL) {
        return true;
    }

    return false;
}

/* ========================================================================= */
/* Ownership / ABI notes                                                     */
/* ========================================================================= */

/*
 * Ownership:
 *
 *     - returned rule strings are immutable static storage;
 *     - callers retain ownership of output buffers;
 *     - diagnostic objects are borrowed;
 *     - source contexts are borrowed;
 *     - no function in this interface frees caller-owned memory.
 *
 * Threading:
 *
 *     - policy lookup is read-only;
 *     - introspection is read-only;
 *     - formatting uses only caller-provided/local storage;
 *     - registry validation performed by help.c is read-only.
 *
 * Determinism:
 *
 *     - rule-table order is stable within a build;
 *     - formatting does not depend on locale;
 *     - no filesystem or environment lookup is performed here;
 *     - source text is used only through the supplied source context.
 *
 * Separation of responsibilities:
 *
 *     registry.c
 *         diagnostic identity and metadata.
 *
 *     help.c
 *         human remediation policy.
 *
 *     suggestion.c
 *         structured suggestions and confidence.
 *
 *     diagnostic.c
 *         diagnostic object construction and storage.
 *
 *     render_*.c
 *         terminal/JSON/SARIF/LSP serialization.
 *
 * Keeping these responsibilities separate prevents human help text from
 * becoming an implicit machine-edit protocol.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_HELP_H */