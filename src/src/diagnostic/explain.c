#include "explain.h"

#include "diagnostic.h"
#include "registry.h"
#include "diagnostic.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Vitte diagnostic explanation subsystem.
 *
 * This module provides the documentation/query layer behind commands such as:
 *
 *     vitte explain E0501
 *     vitte explain VITTE_TYPE_E_MISMATCH
 *     vitte explain --list
 *     vitte explain --category type
 *     vitte explain --search "constant"
 *
 * The diagnostic registry remains authoritative for:
 *
 *     - public diagnostic code;
 *     - internal symbolic identifier;
 *     - default severity;
 *     - compiler phase;
 *     - category;
 *     - title;
 *     - canonical short explanation.
 *
 * This file intentionally does not duplicate the registry.
 *
 * Responsibilities:
 *
 *     - diagnostic-code normalization;
 *     - exact lookup;
 *     - related diagnostic discovery;
 *     - text/Markdown/JSON rendering;
 *     - registry listing;
 *     - category filtering;
 *     - full-text search;
 *     - machine-readable metadata;
 *     - robust escaping;
 *     - deterministic output.
 *
 * C17.
 */

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_EXPLAIN_CODE_CAPACITY      ((size_t)128u)
#define VITTE_EXPLAIN_QUERY_CAPACITY     ((size_t)256u)
#define VITTE_EXPLAIN_RELATED_LIMIT      ((size_t)8u)

#define VITTE_EXPLAIN_JSON_VERSION       1u

/* ========================================================================= */
/* Small helpers                                                             */
/* ========================================================================= */

static bool
vitte_explain_text_present(
    const char *text
)
{
    return text != NULL &&
           text[0] != '\0';
}

static bool
vitte_explain_ascii_equal_case_insensitive(
    const char *left,
    const char *right
)
{
    unsigned char a;
    unsigned char b;

    if (left == NULL ||
        right == NULL) {
        return left == right;
    }

    while (*left != '\0' &&
           *right != '\0') {

        a = (unsigned char)*left;
        b = (unsigned char)*right;

        if (tolower(a) !=
            tolower(b)) {
            return false;
        }

        left++;
        right++;
    }

    return *left == '\0' &&
           *right == '\0';
}

static bool
vitte_explain_ascii_contains_case_insensitive(
    const char *text,
    const char *needle
)
{
    const char *start;

    if (text == NULL ||
        needle == NULL) {
        return false;
    }

    if (needle[0] == '\0') {
        return true;
    }

    for (start = text;
         *start != '\0';
         start++) {

        const char *left;
        const char *right;

        left = start;
        right = needle;

        while (*left != '\0' &&
               *right != '\0' &&
               tolower((unsigned char)*left) ==
               tolower((unsigned char)*right)) {

            left++;
            right++;
        }

        if (*right == '\0') {
            return true;
        }
    }

    return false;
}

static bool
vitte_explain_stream_ok(
    FILE *stream
)
{
    return stream != NULL &&
           !ferror(stream);
}

/* ========================================================================= */
/* Code normalization                                                        */
/* ========================================================================= */

/*
 * Normalize common human input:
 *
 *     "e0501"     -> "E0501"
 *     " E0501 "   -> "E0501"
 *     "w0001"     -> "W0001"
 *
 * Internal symbolic identifiers are preserved except surrounding whitespace.
 */
static bool
vitte_explain_normalize_code(
    const char *input,
    char *output,
    size_t capacity
)
{
    const char *begin;
    const char *end;
    size_t length;
    size_t index;
    bool looks_public;

    if (input == NULL ||
        output == NULL ||
        capacity == 0u) {
        return false;
    }

    output[0] = '\0';

    begin = input;

    while (*begin != '\0' &&
           isspace((unsigned char)*begin)) {
        begin++;
    }

    end = begin + strlen(begin);

    while (end > begin &&
           isspace((unsigned char)end[-1])) {
        end--;
    }

    length = (size_t)(end - begin);

    if (length == 0u ||
        length >= capacity) {
        return false;
    }

    looks_public =
        length == 5u &&
        (begin[0] == 'E' ||
         begin[0] == 'e' ||
         begin[0] == 'W' ||
         begin[0] == 'w' ||
         begin[0] == 'N' ||
         begin[0] == 'n' ||
         begin[0] == 'H' ||
         begin[0] == 'h');

    if (looks_public) {
        for (index = 1u;
             index < length;
             index++) {

            if (!isdigit(
                    (unsigned char)begin[index]
                )) {
                looks_public = false;
                break;
            }
        }
    }

    for (index = 0u;
         index < length;
         index++) {

        unsigned char character;

        character =
            (unsigned char)begin[index];

        if (looks_public) {
            output[index] =
                (char)toupper(character);
        } else {
            output[index] =
                (char)character;
        }
    }

    output[length] = '\0';

    return true;
}

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_explain_find(
    const char *code
)
{
    char normalized[
        VITTE_EXPLAIN_CODE_CAPACITY
    ];

    const vitte_diagnostic_registry_entry_t *entry;

    if (!vitte_explain_normalize_code(
            code,
            normalized,
            sizeof(normalized)
        )) {
        return NULL;
    }

    entry =
        vitte_diagnostic_registry_find(
            normalized
        );

    if (entry != NULL) {
        return entry;
    }

    /*
     * Internal symbolic identifiers are normally uppercase. Accept
     * case-insensitive input for interactive CLI convenience without changing
     * the registry's canonical identifiers.
     */
    {
        size_t index;
        size_t count;

        count =
            vitte_diagnostic_registry_count();

        for (index = 0u;
             index < count;
             index++) {

            const vitte_diagnostic_registry_entry_t *candidate;

            candidate =
                vitte_diagnostic_registry_at(
                    index
                );

            if (candidate == NULL) {
                continue;
            }

            if (vitte_explain_ascii_equal_case_insensitive(
                    candidate->internal_code,
                    normalized
                )) {
                return candidate;
            }
        }
    }

    return NULL;
}

/* ========================================================================= */
/* Range helpers                                                             */
/* ========================================================================= */

static bool
vitte_explain_same_range(
    const vitte_diagnostic_registry_entry_t *left,
    const vitte_diagnostic_registry_entry_t *right
)
{
    vitte_diagnostic_registry_range_t left_range;
    vitte_diagnostic_registry_range_t right_range;

    if (left == NULL ||
        right == NULL) {
        return false;
    }

    left_range =
        vitte_diagnostic_registry_range(
            left->public_code
        );

    right_range =
        vitte_diagnostic_registry_range(
            right->public_code
        );

    return left_range !=
               VITTE_DIAGNOSTIC_RANGE_UNKNOWN &&
           left_range == right_range;
}

/* ========================================================================= */
/* Related diagnostics                                                       */
/* ========================================================================= */

static size_t
vitte_explain_collect_related(
    const vitte_diagnostic_registry_entry_t *entry,
    const vitte_diagnostic_registry_entry_t **related,
    size_t capacity
)
{
    size_t index;
    size_t count;
    size_t written;

    if (entry == NULL ||
        related == NULL ||
        capacity == 0u) {
        return 0u;
    }

    count =
        vitte_diagnostic_registry_count();

    written = 0u;

    /*
     * First pass:
     * same category and same public range.
     */
    for (index = 0u;
         index < count &&
         written < capacity;
         index++) {

        const vitte_diagnostic_registry_entry_t *candidate;

        candidate =
            vitte_diagnostic_registry_at(
                index
            );

        if (candidate == NULL ||
            candidate == entry) {
            continue;
        }

        if (!vitte_explain_same_range(
                entry,
                candidate
            )) {
            continue;
        }

        if (strcmp(
                entry->category,
                candidate->category
            ) != 0) {
            continue;
        }

        related[written++] =
            candidate;
    }

    /*
     * Second pass:
     * fill remaining slots with diagnostics from the same public range.
     */
    for (index = 0u;
         index < count &&
         written < capacity;
         index++) {

        const vitte_diagnostic_registry_entry_t *candidate;
        size_t duplicate_index;
        bool duplicate;

        candidate =
            vitte_diagnostic_registry_at(
                index
            );

        if (candidate == NULL ||
            candidate == entry) {
            continue;
        }

        if (!vitte_explain_same_range(
                entry,
                candidate
            )) {
            continue;
        }

        duplicate = false;

        for (duplicate_index = 0u;
             duplicate_index < written;
             duplicate_index++) {

            if (related[duplicate_index] ==
                candidate) {
                duplicate = true;
                break;
            }
        }

        if (duplicate) {
            continue;
        }

        related[written++] =
            candidate;
    }

    return written;
}

/* ========================================================================= */
/* Generic guidance                                                          */
/* ========================================================================= */

/*
 * Registry explanations remain canonical and compact.
 *
 * These category/range hints provide general remediation guidance without
 * pretending to know source-specific facts that belong in the actual
 * diagnostic instance.
 */
static const char *
vitte_explain_guidance(
    const vitte_diagnostic_registry_entry_t *entry
)
{
    vitte_diagnostic_registry_range_t range;

    if (entry == NULL) {
        return NULL;
    }

    range =
        vitte_diagnostic_registry_range(
            entry->public_code
        );

    switch (range) {
        case VITTE_DIAGNOSTIC_RANGE_INFRASTRUCTURE:
            return
                "Inspect the compiler invocation and the immediately preceding "
                "diagnostics. If this is an internal compiler failure, preserve "
                "the smallest reproducing source, compiler version, target and "
                "command line.";

        case VITTE_DIAGNOSTIC_RANGE_LEXER:
            return
                "Inspect the exact source bytes and token boundaries around "
                "the highlighted location. Check delimiters, escapes, numeric "
                "syntax, identifier spelling and Unicode encoding.";

        case VITTE_DIAGNOSTIC_RANGE_PARSER:
            return
                "Start at the first syntax error. Later parser errors may be "
                "recovery diagnostics caused by the same missing, unexpected "
                "or mismatched token.";

        case VITTE_DIAGNOSTIC_RANGE_MODULE:
            return
                "Check module names, package availability, visibility, import "
                "paths, version constraints and dependency cycles.";

        case VITTE_DIAGNOSTIC_RANGE_NAME:
            return
                "Check spelling, lexical scope, qualification, visibility and "
                "whether the declaration is available before or from the "
                "current use site.";

        case VITTE_DIAGNOSTIC_RANGE_TYPE:
            return
                "Compare the expected and actual types structurally. Inspect "
                "generic arguments, conversions, inferred constraints and the "
                "declaration that introduced the expected type.";

        case VITTE_DIAGNOSTIC_RANGE_CALL:
            return
                "Compare the call against each viable procedure signature. "
                "Check argument count, argument-to-parameter mapping, types, "
                "generic constraints and overload resolution.";

        case VITTE_DIAGNOSTIC_RANGE_CONTRACT:
            return
                "Inspect the contract declaration, the call or return site and "
                "the values available to the contract. Distinguish a statically "
                "disproved contract from one the compiler merely cannot prove.";

        case VITTE_DIAGNOSTIC_RANGE_GENERIC:
            return
                "Inspect generic arguments, bounds, trait requirements and the "
                "instantiation chain that selected this declaration.";

        case VITTE_DIAGNOSTIC_RANGE_BACKEND:
            return
                "Inspect the originating Vitte construct first. Generated C "
                "locations should be remapped to Vitte source whenever a valid "
                "source map exists.";

        case VITTE_DIAGNOSTIC_RANGE_IR:
            return
                "Inspect the lowering chain and provenance from source through "
                "HIR and IR. Invalid compiler-generated IR generally indicates "
                "a compiler defect rather than a source-level syntax problem.";

        case VITTE_DIAGNOSTIC_RANGE_CONTROL_FLOW:
            return
                "Inspect all control-flow paths reaching the highlighted "
                "construct, including early returns, loops, matches and "
                "unreachable branches.";

        case VITTE_DIAGNOSTIC_RANGE_MEMORY:
            return
                "Check pointer/reference type, nullability, alignment, object "
                "layout and the validity of the operation for the referenced "
                "storage.";

        case VITTE_DIAGNOSTIC_RANGE_UNSAFE:
            return
                "Confirm that the operation intentionally requires low-level "
                "semantics and that it occurs in an explicitly permitted "
                "unsafe context.";

        case VITTE_DIAGNOSTIC_RANGE_FFI:
            return
                "Check ABI, calling convention, symbol name, representation, "
                "layout, variadic arguments and target-specific foreign "
                "interface requirements.";

        case VITTE_DIAGNOSTIC_RANGE_CONST_EVAL:
            return
                "Inspect the compile-time dependency chain and operations for "
                "non-constant values, overflow, division by zero, recursion "
                "cycles or evaluation limits.";

        case VITTE_DIAGNOSTIC_RANGE_PATTERN:
            return
                "Compare patterns against the scrutinee type and earlier "
                "patterns. Check exhaustiveness, reachability and consistent "
                "bindings.";

        case VITTE_DIAGNOSTIC_RANGE_CONCURRENCY:
            return
                "Check the execution context, task/value type and restrictions "
                "that apply when values or operations cross concurrent or "
                "asynchronous boundaries.";

        case VITTE_DIAGNOSTIC_RANGE_MACRO:
            return
                "Inspect the expansion or compiler-pass chain and preserve "
                "source provenance so the diagnostic can identify both the "
                "generated construct and its originating source.";

        case VITTE_DIAGNOSTIC_RANGE_TARGET:
            return
                "Check the selected target triple, architecture, ABI, pointer "
                "width, byte order and requested target features.";

        case VITTE_DIAGNOSTIC_RANGE_WARNING:
            return
                "Review whether the construct is intentional. Warnings should "
                "remain individually controllable and may be promoted to errors "
                "by compiler policy.";

        case VITTE_DIAGNOSTIC_RANGE_NOTE:
            return
                "This diagnostic provides additional context for another "
                "compiler observation.";

        case VITTE_DIAGNOSTIC_RANGE_HELP:
            return
                "This diagnostic provides remediation guidance or a possible "
                "source change.";

        case VITTE_DIAGNOSTIC_RANGE_UNKNOWN:
        default:
            return NULL;
    }
}

/* ========================================================================= */
/* JSON                                                                      */
/* ========================================================================= */

static bool
vitte_explain_json_string(
    FILE *stream,
    const char *text
)
{
    const unsigned char *cursor;

    if (stream == NULL) {
        return false;
    }

    if (text == NULL) {
        return fputs(
            "null",
            stream
        ) >= 0;
    }

    if (fputc('"', stream) == EOF) {
        return false;
    }

    cursor =
        (const unsigned char *)text;

    while (*cursor != '\0') {
        unsigned char character;

        character = *cursor++;

        switch (character) {
            case '"':
                if (fputs("\\\"", stream) < 0) {
                    return false;
                }
                break;

            case '\\':
                if (fputs("\\\\", stream) < 0) {
                    return false;
                }
                break;

            case '\b':
                if (fputs("\\b", stream) < 0) {
                    return false;
                }
                break;

            case '\f':
                if (fputs("\\f", stream) < 0) {
                    return false;
                }
                break;

            case '\n':
                if (fputs("\\n", stream) < 0) {
                    return false;
                }
                break;

            case '\r':
                if (fputs("\\r", stream) < 0) {
                    return false;
                }
                break;

            case '\t':
                if (fputs("\\t", stream) < 0) {
                    return false;
                }
                break;

            default:
                if (character < 0x20u) {
                    if (fprintf(
                            stream,
                            "\\u%04x",
                            (unsigned)character
                        ) < 0) {
                        return false;
                    }
                } else {
                    if (fputc(
                            (int)character,
                            stream
                        ) == EOF) {
                        return false;
                    }
                }
                break;
        }
    }

    return fputc('"', stream) != EOF;
}

/* ========================================================================= */
/* Human renderer                                                            */
/* ========================================================================= */

static vitte_status_t
vitte_explain_render_text(
    FILE *stream,
    const vitte_diagnostic_registry_entry_t *entry,
    bool include_related
)
{
    const char *guidance;
    vitte_diagnostic_registry_range_t range;

    const vitte_diagnostic_registry_entry_t *related[
        VITTE_EXPLAIN_RELATED_LIMIT
    ];

    size_t related_count;
    size_t index;

    if (stream == NULL ||
        entry == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    range =
        vitte_diagnostic_registry_range(
            entry->public_code
        );

    guidance =
        vitte_explain_guidance(
            entry
        );

    if (fprintf(
            stream,
            "%s: %s\n",
            entry->public_code,
            entry->title
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (fputc('\n', stream) == EOF) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (fprintf(
            stream,
            "%s\n",
            entry->explanation
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (fputs(
            "\nDiagnostic information\n"
            "----------------------\n",
            stream
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (fprintf(
            stream,
            "Code:       %s\n"
            "Internal:   %s\n"
            "Severity:   %s\n"
            "Phase:      %s\n"
            "Category:   %s\n"
            "Range:      %s\n"
            "Deprecated: %s\n",
            entry->public_code,
            entry->internal_code,
            vitte_diagnostic_severity_name(
                entry->default_severity
            ),
            vitte_diagnostic_origin_name(
                entry->origin
            ),
            entry->category,
            vitte_diagnostic_registry_range_name(
                range
            ),
            entry->deprecated
                ? "yes"
                : "no"
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (vitte_explain_text_present(
            guidance
        )) {

        if (fputs(
                "\nGuidance\n"
                "--------\n",
                stream
            ) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        if (fprintf(
                stream,
                "%s\n",
                guidance
            ) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    if (include_related) {
        related_count =
            vitte_explain_collect_related(
                entry,
                related,
                VITTE_EXPLAIN_RELATED_LIMIT
            );

        if (related_count != 0u) {
            if (fputs(
                    "\nRelated diagnostics\n"
                    "-------------------\n",
                    stream
                ) < 0) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            for (index = 0u;
                 index < related_count;
                 index++) {

                if (fprintf(
                        stream,
                        "  %-5s  %s\n",
                        related[index]->public_code,
                        related[index]->title
                    ) < 0) {
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
            }
        }
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* Markdown renderer                                                         */
/* ========================================================================= */

static vitte_status_t
vitte_explain_render_markdown(
    FILE *stream,
    const vitte_diagnostic_registry_entry_t *entry,
    bool include_related
)
{
    const char *guidance;
    vitte_diagnostic_registry_range_t range;

    const vitte_diagnostic_registry_entry_t *related[
        VITTE_EXPLAIN_RELATED_LIMIT
    ];

    size_t related_count;
    size_t index;

    if (stream == NULL ||
        entry == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    range =
        vitte_diagnostic_registry_range(
            entry->public_code
        );

    guidance =
        vitte_explain_guidance(
            entry
        );

    if (fprintf(
            stream,
            "# %s — %s\n\n"
            "%s\n\n"
            "## Diagnostic information\n\n"
            "| Field | Value |\n"
            "| --- | --- |\n"
            "| Code | `%s` |\n"
            "| Internal identifier | `%s` |\n"
            "| Severity | `%s` |\n"
            "| Phase | `%s` |\n"
            "| Category | `%s` |\n"
            "| Range | `%s` |\n"
            "| Deprecated | `%s` |\n",
            entry->public_code,
            entry->title,
            entry->explanation,
            entry->public_code,
            entry->internal_code,
            vitte_diagnostic_severity_name(
                entry->default_severity
            ),
            vitte_diagnostic_origin_name(
                entry->origin
            ),
            entry->category,
            vitte_diagnostic_registry_range_name(
                range
            ),
            entry->deprecated
                ? "yes"
                : "no"
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (vitte_explain_text_present(
            guidance
        )) {

        if (fprintf(
                stream,
                "\n## Guidance\n\n%s\n",
                guidance
            ) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    if (include_related) {
        related_count =
            vitte_explain_collect_related(
                entry,
                related,
                VITTE_EXPLAIN_RELATED_LIMIT
            );

        if (related_count != 0u) {
            if (fputs(
                    "\n## Related diagnostics\n\n",
                    stream
                ) < 0) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            for (index = 0u;
                 index < related_count;
                 index++) {

                if (fprintf(
                        stream,
                        "- `%s` — %s\n",
                        related[index]->public_code,
                        related[index]->title
                    ) < 0) {
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
            }
        }
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* JSON renderer                                                             */
/* ========================================================================= */

static vitte_status_t
vitte_explain_render_json(
    FILE *stream,
    const vitte_diagnostic_registry_entry_t *entry,
    bool include_related
)
{
    const char *guidance;
    vitte_diagnostic_registry_range_t range;

    const vitte_diagnostic_registry_entry_t *related[
        VITTE_EXPLAIN_RELATED_LIMIT
    ];

    size_t related_count;
    size_t index;

    if (stream == NULL ||
        entry == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    range =
        vitte_diagnostic_registry_range(
            entry->public_code
        );

    guidance =
        vitte_explain_guidance(
            entry
        );

    if (fprintf(
            stream,
            "{\"version\":%u,\"diagnostic\":{",
            VITTE_EXPLAIN_JSON_VERSION
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

#define JSON_FIELD_STRING(name_, value_)                         \
    do {                                                         \
        if (fputs("\"" name_ "\":", stream) < 0 ||               \
            !vitte_explain_json_string(stream, (value_))) {      \
            return VITTE_STATUS_ERROR_INVALID_STATE;             \
        }                                                        \
    } while (0)

#define JSON_COMMA()                                             \
    do {                                                         \
        if (fputc(',', stream) == EOF) {                         \
            return VITTE_STATUS_ERROR_INVALID_STATE;             \
        }                                                        \
    } while (0)

    JSON_FIELD_STRING(
        "code",
        entry->public_code
    );

    JSON_COMMA();

    JSON_FIELD_STRING(
        "internal",
        entry->internal_code
    );

    JSON_COMMA();

    JSON_FIELD_STRING(
        "title",
        entry->title
    );

    JSON_COMMA();

    JSON_FIELD_STRING(
        "explanation",
        entry->explanation
    );

    JSON_COMMA();

    JSON_FIELD_STRING(
        "severity",
        vitte_diagnostic_severity_name(
            entry->default_severity
        )
    );

    JSON_COMMA();

    JSON_FIELD_STRING(
        "phase",
        vitte_diagnostic_origin_name(
            entry->origin
        )
    );

    JSON_COMMA();

    JSON_FIELD_STRING(
        "category",
        entry->category
    );

    JSON_COMMA();

    JSON_FIELD_STRING(
        "range",
        vitte_diagnostic_registry_range_name(
            range
        )
    );

    JSON_COMMA();

    JSON_FIELD_STRING(
        "guidance",
        guidance
    );

    if (fputs(
            ",\"deprecated\":",
            stream
        ) < 0 ||
        fputs(
            entry->deprecated
                ? "true"
                : "false",
            stream
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (fputs(
            ",\"related\":[",
            stream
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    related_count = 0u;

    if (include_related) {
        related_count =
            vitte_explain_collect_related(
                entry,
                related,
                VITTE_EXPLAIN_RELATED_LIMIT
            );
    }

    for (index = 0u;
         index < related_count;
         index++) {

        if (index != 0u &&
            fputc(',', stream) == EOF) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        if (fputs(
                "{\"code\":",
                stream
            ) < 0 ||
            !vitte_explain_json_string(
                stream,
                related[index]->public_code
            ) ||
            fputs(
                ",\"title\":",
                stream
            ) < 0 ||
            !vitte_explain_json_string(
                stream,
                related[index]->title
            ) ||
            fputc('}', stream) == EOF) {

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    if (fputs(
            "]}}",
            stream
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

#undef JSON_COMMA
#undef JSON_FIELD_STRING

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* Main explanation API                                                      */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_explain(
    FILE *stream,
    const char *code,
    vitte_diagnostic_explain_format_t format,
    bool include_related
)
{
    const vitte_diagnostic_registry_entry_t *entry;

    if (stream == NULL ||
        code == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    entry =
        vitte_diagnostic_explain_find(
            code
        );

    if (entry == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    switch (format) {
        case VITTE_DIAGNOSTIC_EXPLAIN_TEXT:
            return vitte_explain_render_text(
                stream,
                entry,
                include_related
            );

        case VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN:
            return vitte_explain_render_markdown(
                stream,
                entry,
                include_related
            );

        case VITTE_DIAGNOSTIC_EXPLAIN_JSON:
            return vitte_explain_render_json(
                stream,
                entry,
                include_related
            );

        default:
            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
}

/* ========================================================================= */
/* Compact explanation                                                       */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_explain_compact(
    FILE *stream,
    const char *code
)
{
    const vitte_diagnostic_registry_entry_t *entry;

    if (stream == NULL ||
        code == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    entry =
        vitte_diagnostic_explain_find(
            code
        );

    if (entry == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (fprintf(
            stream,
            "%s: %s\n%s\n",
            entry->public_code,
            entry->title,
            entry->explanation
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Listing                                                                   */
/* ========================================================================= */

static bool
vitte_explain_entry_matches_category(
    const vitte_diagnostic_registry_entry_t *entry,
    const char *category
)
{
    if (entry == NULL) {
        return false;
    }

    if (!vitte_explain_text_present(
            category
        )) {
        return true;
    }

    return vitte_explain_ascii_equal_case_insensitive(
        entry->category,
        category
    );
}

vitte_status_t
vitte_diagnostic_explain_list(
    FILE *stream,
    const char *category,
    bool include_deprecated
)
{
    size_t index;
    size_t count;
    size_t emitted;

    if (stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    count =
        vitte_diagnostic_registry_count();

    emitted = 0u;

    for (index = 0u;
         index < count;
         index++) {

        const vitte_diagnostic_registry_entry_t *entry;

        entry =
            vitte_diagnostic_registry_at(
                index
            );

        if (entry == NULL) {
            continue;
        }

        if (!include_deprecated &&
            entry->deprecated) {
            continue;
        }

        if (!vitte_explain_entry_matches_category(
                entry,
                category
            )) {
            continue;
        }

        if (fprintf(
                stream,
                "%-5s  %-18s  %-16s  %s\n",
                entry->public_code,
                entry->category,
                vitte_diagnostic_severity_name(
                    entry->default_severity
                ),
                entry->title
            ) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        emitted++;
    }

    (void)emitted;

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* JSON listing                                                              */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_explain_list_json(
    FILE *stream,
    const char *category,
    bool include_deprecated
)
{
    size_t index;
    size_t count;
    size_t emitted;

    if (stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (fprintf(
            stream,
            "{\"version\":%u,\"diagnostics\":[",
            VITTE_EXPLAIN_JSON_VERSION
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    count =
        vitte_diagnostic_registry_count();

    emitted = 0u;

    for (index = 0u;
         index < count;
         index++) {

        const vitte_diagnostic_registry_entry_t *entry;

        entry =
            vitte_diagnostic_registry_at(
                index
            );

        if (entry == NULL) {
            continue;
        }

        if (!include_deprecated &&
            entry->deprecated) {
            continue;
        }

        if (!vitte_explain_entry_matches_category(
                entry,
                category
            )) {
            continue;
        }

        if (emitted != 0u &&
            fputc(',', stream) == EOF) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        if (fputs("{\"code\":", stream) < 0 ||
            !vitte_explain_json_string(
                stream,
                entry->public_code
            ) ||
            fputs(",\"internal\":", stream) < 0 ||
            !vitte_explain_json_string(
                stream,
                entry->internal_code
            ) ||
            fputs(",\"title\":", stream) < 0 ||
            !vitte_explain_json_string(
                stream,
                entry->title
            ) ||
            fputs(",\"category\":", stream) < 0 ||
            !vitte_explain_json_string(
                stream,
                entry->category
            ) ||
            fputs(",\"severity\":", stream) < 0 ||
            !vitte_explain_json_string(
                stream,
                vitte_diagnostic_severity_name(
                    entry->default_severity
                )
            ) ||
            fputs(",\"phase\":", stream) < 0 ||
            !vitte_explain_json_string(
                stream,
                vitte_diagnostic_origin_name(
                    entry->origin
                )
            ) ||
            fputs(",\"deprecated\":", stream) < 0 ||
            fputs(
                entry->deprecated
                    ? "true"
                    : "false",
                stream
            ) < 0 ||
            fputc('}', stream) == EOF) {

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        emitted++;
    }

    if (fputs(
            "]}",
            stream
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* Search                                                                    */
/* ========================================================================= */

static bool
vitte_explain_entry_matches_query(
    const vitte_diagnostic_registry_entry_t *entry,
    const char *query
)
{
    if (entry == NULL ||
        !vitte_explain_text_present(
            query
        )) {
        return false;
    }

    return
        vitte_explain_ascii_contains_case_insensitive(
            entry->public_code,
            query
        ) ||
        vitte_explain_ascii_contains_case_insensitive(
            entry->internal_code,
            query
        ) ||
        vitte_explain_ascii_contains_case_insensitive(
            entry->title,
            query
        ) ||
        vitte_explain_ascii_contains_case_insensitive(
            entry->category,
            query
        ) ||
        vitte_explain_ascii_contains_case_insensitive(
            entry->explanation,
            query
        ) ||
        vitte_explain_ascii_contains_case_insensitive(
            vitte_diagnostic_origin_name(
                entry->origin
            ),
            query
        );
}

vitte_status_t
vitte_diagnostic_explain_search(
    FILE *stream,
    const char *query,
    size_t limit
)
{
    char normalized[
        VITTE_EXPLAIN_QUERY_CAPACITY
    ];

    size_t index;
    size_t count;
    size_t emitted;

    if (stream == NULL ||
        query == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    /*
     * For search queries we only trim leading/trailing whitespace. Case is
     * handled by the matching routine.
     */
    {
        const char *begin;
        const char *end;
        size_t length;

        begin = query;

        while (*begin != '\0' &&
               isspace((unsigned char)*begin)) {
            begin++;
        }

        end = begin + strlen(begin);

        while (end > begin &&
               isspace((unsigned char)end[-1])) {
            end--;
        }

        length =
            (size_t)(end - begin);

        if (length == 0u ||
            length >= sizeof(normalized)) {
            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }

        memcpy(
            normalized,
            begin,
            length
        );

        normalized[length] = '\0';
    }

    count =
        vitte_diagnostic_registry_count();

    emitted = 0u;

    for (index = 0u;
         index < count;
         index++) {

        const vitte_diagnostic_registry_entry_t *entry;

        entry =
            vitte_diagnostic_registry_at(
                index
            );

        if (!vitte_explain_entry_matches_query(
                entry,
                normalized
            )) {
            continue;
        }

        if (fprintf(
                stream,
                "%-5s  %-18s  %s\n",
                entry->public_code,
                entry->category,
                entry->title
            ) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        emitted++;

        if (limit != 0u &&
            emitted >= limit) {
            break;
        }
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* Search JSON                                                               */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_explain_search_json(
    FILE *stream,
    const char *query,
    size_t limit
)
{
    size_t index;
    size_t count;
    size_t emitted;

    if (stream == NULL ||
        !vitte_explain_text_present(
            query
        )) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (fprintf(
            stream,
            "{\"version\":%u,\"query\":",
            VITTE_EXPLAIN_JSON_VERSION
        ) < 0 ||
        !vitte_explain_json_string(
            stream,
            query
        ) ||
        fputs(",\"results\":[", stream) < 0) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    count =
        vitte_diagnostic_registry_count();

    emitted = 0u;

    for (index = 0u;
         index < count;
         index++) {

        const vitte_diagnostic_registry_entry_t *entry;

        entry =
            vitte_diagnostic_registry_at(
                index
            );

        if (!vitte_explain_entry_matches_query(
                entry,
                query
            )) {
            continue;
        }

        if (emitted != 0u &&
            fputc(',', stream) == EOF) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        if (fputs("{\"code\":", stream) < 0 ||
            !vitte_explain_json_string(
                stream,
                entry->public_code
            ) ||
            fputs(",\"title\":", stream) < 0 ||
            !vitte_explain_json_string(
                stream,
                entry->title
            ) ||
            fputs(",\"category\":", stream) < 0 ||
            !vitte_explain_json_string(
                stream,
                entry->category
            ) ||
            fputs(",\"explanation\":", stream) < 0 ||
            !vitte_explain_json_string(
                stream,
                entry->explanation
            ) ||
            fputc('}', stream) == EOF) {

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        emitted++;

        if (limit != 0u &&
            emitted >= limit) {
            break;
        }
    }

    if (fputs(
            "]}",
            stream
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* Category listing                                                          */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_explain_categories(
    FILE *stream
)
{
    size_t index;
    size_t previous;
    size_t count;

    if (stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    count =
        vitte_diagnostic_registry_count();

    for (index = 0u;
         index < count;
         index++) {

        const vitte_diagnostic_registry_entry_t *entry;
        bool seen;

        entry =
            vitte_diagnostic_registry_at(
                index
            );

        if (entry == NULL ||
            !vitte_explain_text_present(
                entry->category
            )) {
            continue;
        }

        seen = false;

        for (previous = 0u;
             previous < index;
             previous++) {

            const vitte_diagnostic_registry_entry_t *candidate;

            candidate =
                vitte_diagnostic_registry_at(
                    previous
                );

            if (candidate != NULL &&
                candidate->category != NULL &&
                strcmp(
                    candidate->category,
                    entry->category
                ) == 0) {

                seen = true;
                break;
            }
        }

        if (seen) {
            continue;
        }

        if (fprintf(
                stream,
                "%s\n",
                entry->category
            ) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* Summary                                                                   */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_explain_summary(
    FILE *stream
)
{
    size_t total;
    size_t errors;
    size_t warnings;
    size_t notes;
    size_t helps;

    if (stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    total =
        vitte_diagnostic_registry_count();

    errors =
        vitte_diagnostic_registry_count_severity(
            VITTE_DIAGNOSTIC_ERROR
        ) +
        vitte_diagnostic_registry_count_severity(
            VITTE_DIAGNOSTIC_FATAL
        );

    warnings =
        vitte_diagnostic_registry_count_severity(
            VITTE_DIAGNOSTIC_WARNING
        );

    notes =
        vitte_diagnostic_registry_count_severity(
            VITTE_DIAGNOSTIC_NOTE
        );

    helps =
        vitte_diagnostic_registry_count_severity(
            VITTE_DIAGNOSTIC_HELP
        );

    if (fprintf(
            stream,
            "Vitte diagnostic registry\n"
            "\n"
            "Total:    %zu\n"
            "Errors:   %zu\n"
            "Warnings: %zu\n"
            "Notes:    %zu\n"
            "Help:     %zu\n"
            "Registry fingerprint: %016llx\n",
            total,
            errors,
            warnings,
            notes,
            helps,
            (unsigned long long)
                vitte_diagnostic_registry_fingerprint()
        ) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Suggest code                                                              */
/* ========================================================================= */

static size_t
vitte_explain_edit_distance(
    const char *left,
    const char *right
)
{
    size_t previous[
        VITTE_EXPLAIN_CODE_CAPACITY
    ];

    size_t current[
        VITTE_EXPLAIN_CODE_CAPACITY
    ];

    size_t left_length;
    size_t right_length;
    size_t row;
    size_t column;

    if (left == NULL ||
        right == NULL) {
        return SIZE_MAX;
    }

    left_length =
        strlen(left);

    right_length =
        strlen(right);

    if (left_length >=
            VITTE_EXPLAIN_CODE_CAPACITY ||
        right_length >=
            VITTE_EXPLAIN_CODE_CAPACITY) {
        return SIZE_MAX;
    }

    for (column = 0u;
         column <= right_length;
         column++) {
        previous[column] = column;
    }

    for (row = 1u;
         row <= left_length;
         row++) {

        current[0] = row;

        for (column = 1u;
             column <= right_length;
             column++) {

            size_t deletion;
            size_t insertion;
            size_t substitution;
            size_t minimum;

            deletion =
                previous[column] + 1u;

            insertion =
                current[column - 1u] + 1u;

            substitution =
                previous[column - 1u] +
                (
                    tolower(
                        (unsigned char)left[row - 1u]
                    ) ==
                    tolower(
                        (unsigned char)right[column - 1u]
                    )
                    ? 0u
                    : 1u
                );

            minimum = deletion;

            if (insertion < minimum) {
                minimum = insertion;
            }

            if (substitution < minimum) {
                minimum = substitution;
            }

            current[column] =
                minimum;
        }

        for (column = 0u;
             column <= right_length;
             column++) {
            previous[column] =
                current[column];
        }
    }

    return previous[right_length];
}

const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_explain_suggest(
    const char *unknown_code
)
{
    char normalized[
        VITTE_EXPLAIN_CODE_CAPACITY
    ];

    const vitte_diagnostic_registry_entry_t *best;
    size_t best_distance;
    size_t index;
    size_t count;

    if (!vitte_explain_normalize_code(
            unknown_code,
            normalized,
            sizeof(normalized)
        )) {
        return NULL;
    }

    best = NULL;
    best_distance = SIZE_MAX;

    count =
        vitte_diagnostic_registry_count();

    for (index = 0u;
         index < count;
         index++) {

        const vitte_diagnostic_registry_entry_t *entry;
        size_t public_distance;
        size_t internal_distance;
        size_t distance;

        entry =
            vitte_diagnostic_registry_at(
                index
            );

        if (entry == NULL) {
            continue;
        }

        public_distance =
            vitte_explain_edit_distance(
                normalized,
                entry->public_code
            );

        internal_distance =
            vitte_explain_edit_distance(
                normalized,
                entry->internal_code
            );

        distance =
            public_distance <
                internal_distance
            ? public_distance
            : internal_distance;

        if (distance <
            best_distance) {

            best_distance = distance;
            best = entry;
        }
    }

    /*
     * Do not suggest unrelated identifiers.
     *
     * Public codes are five characters, so distance <= 2 is useful.
     * Internal names are longer; <= 4 tolerates small spelling mistakes.
     */
    if (best != NULL) {
        size_t normalized_length;

        normalized_length =
            strlen(normalized);

        if (normalized_length == 5u) {
            if (best_distance > 2u) {
                return NULL;
            }
        } else if (best_distance > 4u) {
            return NULL;
        }
    }

    return best;
}

/* ========================================================================= */
/* Explain-or-suggest                                                        */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_explain_or_suggest(
    FILE *stream,
    const char *code,
    vitte_diagnostic_explain_format_t format,
    bool include_related
)
{
    const vitte_diagnostic_registry_entry_t *entry;

    if (stream == NULL ||
        code == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    entry =
        vitte_diagnostic_explain_find(
            code
        );

    if (entry != NULL) {
        return vitte_diagnostic_explain(
            stream,
            entry->public_code,
            format,
            include_related
        );
    }

    entry =
        vitte_diagnostic_explain_suggest(
            code
        );

    switch (format) {
        case VITTE_DIAGNOSTIC_EXPLAIN_JSON:
            if (fputs(
                    "{\"error\":\"unknown diagnostic code\",\"query\":",
                    stream
                ) < 0 ||
                !vitte_explain_json_string(
                    stream,
                    code
                )) {

                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            if (entry != NULL) {
                if (fputs(
                        ",\"suggestion\":",
                        stream
                    ) < 0 ||
                    !vitte_explain_json_string(
                        stream,
                        entry->public_code
                    )) {

                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
            } else {
                if (fputs(
                        ",\"suggestion\":null",
                        stream
                    ) < 0) {
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
            }

            if (fputc('}', stream) == EOF) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            break;

        case VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN:
            if (fprintf(
                    stream,
                    "# Unknown diagnostic code\n\n"
                    "No registered Vitte diagnostic matches `%s`.\n",
                    code
                ) < 0) {

                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            if (entry != NULL) {
                if (fprintf(
                        stream,
                        "\nDid you mean `%s` — %s?\n",
                        entry->public_code,
                        entry->title
                    ) < 0) {

                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
            }

            break;

        case VITTE_DIAGNOSTIC_EXPLAIN_TEXT:
            if (fprintf(
                    stream,
                    "unknown diagnostic code: %s\n",
                    code
                ) < 0) {

                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            if (entry != NULL) {
                if (fprintf(
                        stream,
                        "did you mean %s: %s?\n",
                        entry->public_code,
                        entry->title
                    ) < 0) {

                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
            }

            break;

        default:
            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
}

/* ========================================================================= */
/* Registry validation                                                       */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_explain_validate_registry(
    FILE *stream
)
{
    vitte_diagnostic_registry_validation_t validation;
    vitte_status_t status;

    memset(
        &validation,
        0,
        sizeof(validation)
    );

    status =
        vitte_diagnostic_registry_validate(
            &validation
        );

    if (stream != NULL) {
        if (fprintf(
                stream,
                "diagnostic registry: %s\n"
                "entries: %zu\n"
                "invalid entries: %zu\n"
                "duplicate internal identifiers: %zu\n"
                "duplicate public codes: %zu\n",
                validation.valid
                    ? "valid"
                    : "invalid",
                validation.entry_count,
                validation.invalid_entry_count,
                validation.duplicate_internal_count,
                validation.duplicate_public_count
            ) < 0) {

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        if (!validation.valid) {
            if (validation.first_invalid_index !=
                VITTE_DIAGNOSTIC_INDEX_NONE) {

                if (fprintf(
                        stream,
                        "first invalid entry: %zu\n",
                        validation.first_invalid_index
                    ) < 0) {

                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
            }
        }
    }

    return status;
}

/* ========================================================================= */
/* Format name                                                               */
/* ========================================================================= */

const char *
vitte_diagnostic_explain_format_name(
    vitte_diagnostic_explain_format_t format
)
{
    switch (format) {
        case VITTE_DIAGNOSTIC_EXPLAIN_TEXT:
            return "text";

        case VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN:
            return "markdown";

        case VITTE_DIAGNOSTIC_EXPLAIN_JSON:
            return "json";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Format parser                                                             */
/* ========================================================================= */

bool
vitte_diagnostic_explain_parse_format(
    const char *text,
    vitte_diagnostic_explain_format_t *format
)
{
    if (text == NULL ||
        format == NULL) {
        return false;
    }

    if (vitte_explain_ascii_equal_case_insensitive(
            text,
            "text"
        ) ||
        vitte_explain_ascii_equal_case_insensitive(
            text,
            "human"
        )) {

        *format =
            VITTE_DIAGNOSTIC_EXPLAIN_TEXT;

        return true;
    }

    if (vitte_explain_ascii_equal_case_insensitive(
            text,
            "markdown"
        ) ||
        vitte_explain_ascii_equal_case_insensitive(
            text,
            "md"
        )) {

        *format =
            VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN;

        return true;
    }

    if (vitte_explain_ascii_equal_case_insensitive(
            text,
            "json"
        )) {

        *format =
            VITTE_DIAGNOSTIC_EXPLAIN_JSON;

        return true;
    }

    return false;
}

/* ========================================================================= */
/* Registry fingerprint                                                      */
/* ========================================================================= */

uint64_t
vitte_diagnostic_explain_registry_fingerprint(void)
{
    return vitte_diagnostic_registry_fingerprint();
}
