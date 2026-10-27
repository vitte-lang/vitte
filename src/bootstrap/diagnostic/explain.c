#include "explain.h"
#include "cli_bridge.h"

#include "diagnostic.h"
#include "registry.h"
#include "help.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VITTE_EXPLAIN_CODE_CAPACITY 128u
#define VITTE_EXPLAIN_QUERY_CAPACITY 256u
#define VITTE_EXPLAIN_RELATED_LIMIT 8u
#define VITTE_EXPLAIN_JSON_VERSION 1u

#ifndef VITTE_DIAGNOSTIC_INDEX_NONE
#define VITTE_DIAGNOSTIC_INDEX_NONE SIZE_MAX
#endif

typedef struct vitte_explain_match {
    const vitte_diagnostic_registry_entry_t *entry;
    size_t registry_index;
    size_t score;
} vitte_explain_match_t;

static bool
vitte_explain_text_present(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static bool
vitte_explain_stream_ok(FILE *stream)
{
    return stream != NULL && ferror(stream) == 0;
}

static unsigned char
vitte_explain_ascii_lower(unsigned char character)
{
    if (character >= (unsigned char)'A' &&
        character <= (unsigned char)'Z') {
        return (unsigned char)(
            character - (unsigned char)'A' + (unsigned char)'a'
        );
    }

    return character;
}

static unsigned char
vitte_explain_ascii_upper(unsigned char character)
{
    if (character >= (unsigned char)'a' &&
        character <= (unsigned char)'z') {
        return (unsigned char)(
            character - (unsigned char)'a' + (unsigned char)'A'
        );
    }

    return character;
}

static bool
vitte_explain_ascii_equal_case_insensitive(
    const char *left,
    const char *right)
{
    if (left == NULL || right == NULL) {
        return left == right;
    }

    while (*left != '\0' && *right != '\0') {
        if (vitte_explain_ascii_lower((unsigned char)*left) !=
            vitte_explain_ascii_lower((unsigned char)*right)) {
            return false;
        }

        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static bool
vitte_explain_ascii_contains_case_insensitive(
    const char *text,
    const char *needle)
{
    const char *start;

    if (text == NULL || needle == NULL) {
        return false;
    }

    if (*needle == '\0') {
        return true;
    }

    for (start = text; *start != '\0'; start++) {
        const char *left;
        const char *right;

        left = start;
        right = needle;

        while (*left != '\0' &&
               *right != '\0' &&
               vitte_explain_ascii_lower((unsigned char)*left) ==
               vitte_explain_ascii_lower((unsigned char)*right)) {
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
vitte_explain_trim(
    const char *input,
    char *output,
    size_t capacity)
{
    const char *begin;
    const char *end;
    size_t length;

    if (input == NULL || output == NULL || capacity == 0u) {
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

    if (length == 0u || length >= capacity) {
        return false;
    }

    memcpy(output, begin, length);
    output[length] = '\0';

    return true;
}

static bool
vitte_explain_is_public_code(const char *text)
{
    size_t index;

    if (text == NULL || strlen(text) != 5u) {
        return false;
    }

    if (vitte_explain_ascii_upper((unsigned char)text[0]) != 'E' &&
        vitte_explain_ascii_upper((unsigned char)text[0]) != 'W' &&
        vitte_explain_ascii_upper((unsigned char)text[0]) != 'N' &&
        vitte_explain_ascii_upper((unsigned char)text[0]) != 'H') {
        return false;
    }

    for (index = 1u; index < 5u; index++) {
        if (text[index] < '0' || text[index] > '9') {
            return false;
        }
    }

    return true;
}

static bool
vitte_explain_normalize_code(
    const char *input,
    char *output,
    size_t capacity)
{
    size_t index;

    if (!vitte_explain_trim(input, output, capacity)) {
        return false;
    }

    if (vitte_explain_is_public_code(output)) {
        for (index = 0u; output[index] != '\0'; index++) {
            output[index] = (char)vitte_explain_ascii_upper(
                (unsigned char)output[index]
            );
        }
    }

    return true;
}

static bool
vitte_explain_normalize_query(
    const char *input,
    char *output,
    size_t capacity)
{
    return vitte_explain_trim(input, output, capacity);
}

static const char *
vitte_explain_safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static const char *
vitte_explain_safe_severity_name(
    vitte_diagnostic_severity_t severity)
{
    const char *name;

    name = vitte_diagnostic_severity_name(severity);

    return name != NULL ? name : "unknown";
}

static const char *
vitte_explain_safe_origin_name(
    vitte_diagnostic_origin_t origin)
{
    const char *name;

    name = vitte_diagnostic_origin_name(origin);

    return name != NULL ? name : "unknown";
}

static const char *
vitte_explain_safe_range_name(
    vitte_diagnostic_registry_range_t range)
{
    const char *name;

    name = vitte_diagnostic_registry_range_name(range);

    return name != NULL ? name : "unknown";
}

static bool
vitte_explain_entry_usable(
    const vitte_diagnostic_registry_entry_t *entry)
{
    return entry != NULL &&
           vitte_explain_text_present(entry->public_code) &&
           vitte_explain_text_present(entry->internal_code) &&
           vitte_explain_text_present(entry->title) &&
           vitte_explain_text_present(entry->category) &&
           vitte_explain_text_present(entry->explanation);
}

const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_explain_find(const char *code)
{
    char normalized[VITTE_EXPLAIN_CODE_CAPACITY];
    const vitte_diagnostic_registry_entry_t *entry;
    size_t index;
    size_t count;

    if (!vitte_explain_normalize_code(
            code,
            normalized,
            sizeof(normalized))) {
        return NULL;
    }

    entry = vitte_diagnostic_registry_find(normalized);

    if (entry != NULL) {
        return entry;
    }

    count = vitte_diagnostic_registry_count();

    for (index = 0u; index < count; index++) {
        const vitte_diagnostic_registry_entry_t *candidate;

        candidate = vitte_diagnostic_registry_at(index);

        if (candidate == NULL) {
            continue;
        }

        if (vitte_explain_ascii_equal_case_insensitive(
                candidate->public_code,
                normalized) ||
            vitte_explain_ascii_equal_case_insensitive(
                candidate->internal_code,
                normalized)) {
            return candidate;
        }
    }

    return NULL;
}

static bool
vitte_explain_same_range(
    const vitte_diagnostic_registry_entry_t *left,
    const vitte_diagnostic_registry_entry_t *right)
{
    vitte_diagnostic_registry_range_t left_range;
    vitte_diagnostic_registry_range_t right_range;

    if (left == NULL || right == NULL) {
        return false;
    }

    left_range = vitte_diagnostic_registry_range(left->public_code);
    right_range = vitte_diagnostic_registry_range(right->public_code);

    return left_range != VITTE_DIAGNOSTIC_RANGE_UNKNOWN &&
           left_range == right_range;
}

static bool
vitte_explain_same_category(
    const vitte_diagnostic_registry_entry_t *left,
    const vitte_diagnostic_registry_entry_t *right)
{
    return left != NULL &&
           right != NULL &&
           vitte_explain_text_present(left->category) &&
           vitte_explain_text_present(right->category) &&
           vitte_explain_ascii_equal_case_insensitive(
               left->category,
               right->category
           );
}

static bool
vitte_explain_same_origin(
    const vitte_diagnostic_registry_entry_t *left,
    const vitte_diagnostic_registry_entry_t *right)
{
    return left != NULL &&
           right != NULL &&
           left->origin == right->origin;
}

static bool
vitte_explain_related_contains(
    const vitte_diagnostic_registry_entry_t *const *related,
    size_t count,
    const vitte_diagnostic_registry_entry_t *entry)
{
    size_t index;

    for (index = 0u; index < count; index++) {
        if (related[index] == entry) {
            return true;
        }
    }

    return false;
}

static size_t
vitte_explain_related_score(
    const vitte_diagnostic_registry_entry_t *entry,
    const vitte_diagnostic_registry_entry_t *candidate)
{
    size_t score;

    if (entry == NULL ||
        candidate == NULL ||
        entry == candidate) {
        return 0u;
    }

    score = 0u;

    if (vitte_explain_same_range(entry, candidate)) {
        score += 100u;
    }

    if (vitte_explain_same_category(entry, candidate)) {
        score += 80u;
    }

    if (vitte_explain_same_origin(entry, candidate)) {
        score += 40u;
    }

    if (entry->default_severity == candidate->default_severity) {
        score += 10u;
    }

    if (vitte_explain_ascii_contains_case_insensitive(
            candidate->explanation,
            entry->category)) {
        score += 5u;
    }

    return score;
}

static size_t
vitte_explain_collect_related(
    const vitte_diagnostic_registry_entry_t *entry,
    const vitte_diagnostic_registry_entry_t **related,
    size_t capacity)
{
    size_t written;
    size_t count;
    size_t slot;

    if (entry == NULL ||
        related == NULL ||
        capacity == 0u) {
        return 0u;
    }

    written = 0u;
    count = vitte_diagnostic_registry_count();

    for (slot = 0u; slot < capacity; slot++) {
        const vitte_diagnostic_registry_entry_t *best;
        size_t best_score;
        size_t best_index;
        size_t index;

        best = NULL;
        best_score = 0u;
        best_index = SIZE_MAX;

        for (index = 0u; index < count; index++) {
            const vitte_diagnostic_registry_entry_t *candidate;
            size_t score;

            candidate = vitte_diagnostic_registry_at(index);

            if (candidate == NULL ||
                candidate == entry ||
                vitte_explain_related_contains(
                    related,
                    written,
                    candidate)) {
                continue;
            }

            score = vitte_explain_related_score(
                entry,
                candidate
            );

            if (score == 0u) {
                continue;
            }

            if (best == NULL ||
                score > best_score ||
                (score == best_score &&
                 strcmp(
                     vitte_explain_safe_text(candidate->public_code),
                     vitte_explain_safe_text(best->public_code)
                 ) < 0) ||
                (score == best_score &&
                 strcmp(
                     vitte_explain_safe_text(candidate->public_code),
                     vitte_explain_safe_text(best->public_code)
                 ) == 0 &&
                 index < best_index)) {
                best = candidate;
                best_score = score;
                best_index = index;
            }
        }

        if (best == NULL) {
            break;
        }

        related[written++] = best;
    }

    return written;
}

static const char *
vitte_explain_guidance(
    const vitte_diagnostic_registry_entry_t *entry)
{
    const char *guidance;

    if (entry == NULL ||
        !vitte_explain_text_present(entry->public_code)) {
        return NULL;
    }

    guidance = vitte_diagnostic_help_for_code(
        entry->public_code
    );

    return vitte_explain_text_present(guidance)
        ? guidance
        : NULL;
}

static bool
vitte_explain_json_string(
    FILE *stream,
    const char *text)
{
    const unsigned char *cursor;

    if (stream == NULL) {
        return false;
    }

    if (text == NULL) {
        return fputs("null", stream) >= 0;
    }

    if (fputc('"', stream) == EOF) {
        return false;
    }

    cursor = (const unsigned char *)text;

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
                            (unsigned)character) < 0) {
                        return false;
                    }
                } else {
                    if (fputc((int)character, stream) == EOF) {
                        return false;
                    }
                }
                break;
        }
    }

    return fputc('"', stream) != EOF;
}

static bool
vitte_explain_json_key(
    FILE *stream,
    const char *key)
{
    return vitte_explain_json_string(stream, key) &&
           fputc(':', stream) != EOF;
}

static bool
vitte_explain_json_string_field(
    FILE *stream,
    const char *key,
    const char *value)
{
    return vitte_explain_json_key(stream, key) &&
           vitte_explain_json_string(stream, value);
}

static bool
vitte_explain_json_bool_field(
    FILE *stream,
    const char *key,
    bool value)
{
    return vitte_explain_json_key(stream, key) &&
           fputs(value ? "true" : "false", stream) >= 0;
}

static bool
vitte_explain_json_size_field(
    FILE *stream,
    const char *key,
    size_t value)
{
    return vitte_explain_json_key(stream, key) &&
           fprintf(stream, "%zu", value) >= 0;
}

static bool
vitte_explain_json_comma(FILE *stream)
{
    return fputc(',', stream) != EOF;
}

static bool
vitte_explain_markdown_inline(
    FILE *stream,
    const char *text)
{
    const unsigned char *cursor;

    if (stream == NULL) {
        return false;
    }

    if (text == NULL) {
        return fputs("-", stream) >= 0;
    }

    cursor = (const unsigned char *)text;

    while (*cursor != '\0') {
        unsigned char character;

        character = *cursor++;

        if (character == '|' ||
            character == '\\' ||
            character == '`') {
            if (fputc('\\', stream) == EOF) {
                return false;
            }
        }

        if (character == '\n' || character == '\r') {
            if (fputc(' ', stream) == EOF) {
                return false;
            }
            continue;
        }

        if (fputc((int)character, stream) == EOF) {
            return false;
        }
    }

    return true;
}

static vitte_status_t
vitte_explain_render_text(
    FILE *stream,
    const vitte_diagnostic_registry_entry_t *entry,
    bool include_related)
{
    const vitte_diagnostic_registry_entry_t *
        related[VITTE_EXPLAIN_RELATED_LIMIT];

    vitte_diagnostic_registry_range_t range;
    const char *guidance;
    size_t related_count;
    size_t index;

    if (stream == NULL || !vitte_explain_entry_usable(entry)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    range = vitte_diagnostic_registry_range(
        entry->public_code
    );

    guidance = vitte_explain_guidance(entry);

    if (fprintf(
            stream,
            "%s: %s\n\n%s\n\n",
            entry->public_code,
            entry->title,
            entry->explanation) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (fputs(
            "Diagnostic information\n"
            "----------------------\n",
            stream) < 0) {
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
            vitte_explain_safe_severity_name(
                entry->default_severity),
            vitte_explain_safe_origin_name(
                entry->origin),
            entry->category,
            vitte_explain_safe_range_name(range),
            entry->deprecated ? "yes" : "no") < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (guidance != NULL) {
        if (fprintf(
                stream,
                "\nGuidance\n"
                "--------\n"
                "%s\n",
                guidance) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    if (include_related) {
        related_count = vitte_explain_collect_related(
            entry,
            related,
            VITTE_EXPLAIN_RELATED_LIMIT
        );

        if (related_count != 0u) {
            if (fputs(
                    "\nRelated diagnostics\n"
                    "-------------------\n",
                    stream) < 0) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            for (index = 0u; index < related_count; index++) {
                if (fprintf(
                        stream,
                        "  %-5s  %-18s  %s\n",
                        vitte_explain_safe_text(
                            related[index]->public_code),
                        vitte_explain_safe_text(
                            related[index]->category),
                        vitte_explain_safe_text(
                            related[index]->title)) < 0) {
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
            }
        }
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

static vitte_status_t
vitte_explain_render_markdown(
    FILE *stream,
    const vitte_diagnostic_registry_entry_t *entry,
    bool include_related)
{
    const vitte_diagnostic_registry_entry_t *
        related[VITTE_EXPLAIN_RELATED_LIMIT];

    vitte_diagnostic_registry_range_t range;
    const char *guidance;
    size_t related_count;
    size_t index;

    if (stream == NULL || !vitte_explain_entry_usable(entry)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    range = vitte_diagnostic_registry_range(
        entry->public_code
    );

    guidance = vitte_explain_guidance(entry);

    if (fprintf(
            stream,
            "# %s — %s\n\n%s\n\n"
            "## Diagnostic information\n\n"
            "| Field | Value |\n"
            "| --- | --- |\n",
            entry->public_code,
            entry->title,
            entry->explanation) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

#define VITTE_MD_ROW_LITERAL(label_, value_)                     \
    do {                                                         \
        if (fprintf(stream, "| %s | `", (label_)) < 0 ||         \
            !vitte_explain_markdown_inline(stream, (value_)) ||  \
            fputs("` |\n", stream) < 0) {                        \
            return VITTE_STATUS_ERROR_INVALID_STATE;             \
        }                                                        \
    } while (0)

    VITTE_MD_ROW_LITERAL("Code", entry->public_code);
    VITTE_MD_ROW_LITERAL("Internal identifier", entry->internal_code);
    VITTE_MD_ROW_LITERAL(
        "Severity",
        vitte_explain_safe_severity_name(
            entry->default_severity));
    VITTE_MD_ROW_LITERAL(
        "Phase",
        vitte_explain_safe_origin_name(
            entry->origin));
    VITTE_MD_ROW_LITERAL("Category", entry->category);
    VITTE_MD_ROW_LITERAL(
        "Range",
        vitte_explain_safe_range_name(range));
    VITTE_MD_ROW_LITERAL(
        "Deprecated",
        entry->deprecated ? "yes" : "no");

#undef VITTE_MD_ROW_LITERAL

    if (guidance != NULL) {
        if (fprintf(
                stream,
                "\n## Guidance\n\n%s\n",
                guidance) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    if (include_related) {
        related_count = vitte_explain_collect_related(
            entry,
            related,
            VITTE_EXPLAIN_RELATED_LIMIT
        );

        if (related_count != 0u) {
            if (fputs(
                    "\n## Related diagnostics\n\n",
                    stream) < 0) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            for (index = 0u; index < related_count; index++) {
                if (fprintf(
                        stream,
                        "- `%s` — %s\n",
                        vitte_explain_safe_text(
                            related[index]->public_code),
                        vitte_explain_safe_text(
                            related[index]->title)) < 0) {
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
            }
        }
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

static vitte_status_t
vitte_explain_render_json(
    FILE *stream,
    const vitte_diagnostic_registry_entry_t *entry,
    bool include_related)
{
    const vitte_diagnostic_registry_entry_t *
        related[VITTE_EXPLAIN_RELATED_LIMIT];

    vitte_diagnostic_registry_range_t range;
    const char *guidance;
    size_t related_count;
    size_t index;

    if (stream == NULL || !vitte_explain_entry_usable(entry)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    range = vitte_diagnostic_registry_range(
        entry->public_code
    );

    guidance = vitte_explain_guidance(entry);

    if (fputc('{', stream) == EOF ||
        !vitte_explain_json_size_field(
            stream,
            "version",
            VITTE_EXPLAIN_JSON_VERSION) ||
        !vitte_explain_json_comma(stream) ||
        !vitte_explain_json_key(stream, "diagnostic") ||
        fputc('{', stream) == EOF) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

#define VITTE_JSON_STRING_FIELD(name_, value_)                   \
    do {                                                         \
        if (!vitte_explain_json_string_field(                    \
                stream,                                          \
                (name_),                                         \
                (value_))) {                                     \
            return VITTE_STATUS_ERROR_INVALID_STATE;             \
        }                                                        \
    } while (0)

#define VITTE_JSON_COMMA()                                       \
    do {                                                         \
        if (!vitte_explain_json_comma(stream)) {                 \
            return VITTE_STATUS_ERROR_INVALID_STATE;             \
        }                                                        \
    } while (0)

    VITTE_JSON_STRING_FIELD("code", entry->public_code);
    VITTE_JSON_COMMA();

    VITTE_JSON_STRING_FIELD("internal", entry->internal_code);
    VITTE_JSON_COMMA();

    VITTE_JSON_STRING_FIELD("title", entry->title);
    VITTE_JSON_COMMA();

    VITTE_JSON_STRING_FIELD("explanation", entry->explanation);
    VITTE_JSON_COMMA();

    VITTE_JSON_STRING_FIELD(
        "severity",
        vitte_explain_safe_severity_name(
            entry->default_severity));
    VITTE_JSON_COMMA();

    VITTE_JSON_STRING_FIELD(
        "phase",
        vitte_explain_safe_origin_name(
            entry->origin));
    VITTE_JSON_COMMA();

    VITTE_JSON_STRING_FIELD("category", entry->category);
    VITTE_JSON_COMMA();

    VITTE_JSON_STRING_FIELD(
        "range",
        vitte_explain_safe_range_name(range));
    VITTE_JSON_COMMA();

    VITTE_JSON_STRING_FIELD("guidance", guidance);
    VITTE_JSON_COMMA();

    if (!vitte_explain_json_bool_field(
            stream,
            "deprecated",
            entry->deprecated)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    VITTE_JSON_COMMA();

    if (!vitte_explain_json_key(stream, "related") ||
        fputc('[', stream) == EOF) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    related_count = include_related
        ? vitte_explain_collect_related(
            entry,
            related,
            VITTE_EXPLAIN_RELATED_LIMIT)
        : 0u;

    for (index = 0u; index < related_count; index++) {
        if (index != 0u &&
            !vitte_explain_json_comma(stream)) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        if (fputc('{', stream) == EOF ||
            !vitte_explain_json_string_field(
                stream,
                "code",
                related[index]->public_code) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "title",
                related[index]->title) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "category",
                related[index]->category) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "phase",
                vitte_explain_safe_origin_name(
                    related[index]->origin)) ||
            fputc('}', stream) == EOF) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    if (fputs("]}}", stream) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

#undef VITTE_JSON_COMMA
#undef VITTE_JSON_STRING_FIELD

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

vitte_status_t
vitte_diagnostic_explain(
    FILE *stream,
    const char *code,
    vitte_diagnostic_explain_format_t format,
    bool include_related)
{
    const vitte_diagnostic_registry_entry_t *entry;

    if (stream == NULL || code == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    entry = vitte_diagnostic_explain_find(code);

    if (entry == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    switch (format) {
        case VITTE_DIAGNOSTIC_EXPLAIN_TEXT:
            return vitte_explain_render_text(
                stream,
                entry,
                include_related);

        case VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN:
            return vitte_explain_render_markdown(
                stream,
                entry,
                include_related);

        case VITTE_DIAGNOSTIC_EXPLAIN_JSON:
            return vitte_explain_render_json(
                stream,
                entry,
                include_related);

        default:
            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
}

vitte_status_t
vitte_diagnostic_explain_compact(
    FILE *stream,
    const char *code)
{
    const vitte_diagnostic_registry_entry_t *entry;
    const char *guidance;

    if (stream == NULL || code == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    entry = vitte_diagnostic_explain_find(code);

    if (entry == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    guidance = vitte_explain_guidance(entry);

    if (fprintf(
            stream,
            "%s: %s\n%s\n",
            vitte_explain_safe_text(entry->public_code),
            vitte_explain_safe_text(entry->title),
            vitte_explain_safe_text(entry->explanation)) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (guidance != NULL &&
        fprintf(stream, "help: %s\n", guidance) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

static bool
vitte_explain_entry_matches_category(
    const vitte_diagnostic_registry_entry_t *entry,
    const char *category)
{
    if (entry == NULL) {
        return false;
    }

    if (!vitte_explain_text_present(category)) {
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
    bool include_deprecated)
{
    char normalized_category[VITTE_EXPLAIN_QUERY_CAPACITY];
    const char *filter;
    size_t index;
    size_t count;

    if (stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    filter = NULL;

    if (vitte_explain_text_present(category)) {
        if (!vitte_explain_normalize_query(
                category,
                normalized_category,
                sizeof(normalized_category))) {
            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }

        filter = normalized_category;
    }

    count = vitte_diagnostic_registry_count();

    for (index = 0u; index < count; index++) {
        const vitte_diagnostic_registry_entry_t *entry;

        entry = vitte_diagnostic_registry_at(index);

        if (entry == NULL) {
            continue;
        }

        if (!include_deprecated && entry->deprecated) {
            continue;
        }

        if (!vitte_explain_entry_matches_category(
                entry,
                filter)) {
            continue;
        }

        if (fprintf(
                stream,
                "%-5s  %-18s  %-16s  %-18s  %s%s\n",
                vitte_explain_safe_text(entry->public_code),
                vitte_explain_safe_text(entry->category),
                vitte_explain_safe_severity_name(
                    entry->default_severity),
                vitte_explain_safe_origin_name(
                    entry->origin),
                vitte_explain_safe_text(entry->title),
                entry->deprecated ? " [deprecated]" : "") < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

vitte_status_t
vitte_diagnostic_explain_list_json(
    FILE *stream,
    const char *category,
    bool include_deprecated)
{
    char normalized_category[VITTE_EXPLAIN_QUERY_CAPACITY];
    const char *filter;
    size_t index;
    size_t count;
    size_t emitted;

    if (stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    filter = NULL;

    if (vitte_explain_text_present(category)) {
        if (!vitte_explain_normalize_query(
                category,
                normalized_category,
                sizeof(normalized_category))) {
            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }

        filter = normalized_category;
    }

    if (fputc('{', stream) == EOF ||
        !vitte_explain_json_size_field(
            stream,
            "version",
            VITTE_EXPLAIN_JSON_VERSION) ||
        !vitte_explain_json_comma(stream) ||
        !vitte_explain_json_string_field(
            stream,
            "category",
            filter) ||
        !vitte_explain_json_comma(stream) ||
        !vitte_explain_json_key(
            stream,
            "diagnostics") ||
        fputc('[', stream) == EOF) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    count = vitte_diagnostic_registry_count();
    emitted = 0u;

    for (index = 0u; index < count; index++) {
        const vitte_diagnostic_registry_entry_t *entry;
        vitte_diagnostic_registry_range_t range;

        entry = vitte_diagnostic_registry_at(index);

        if (entry == NULL) {
            continue;
        }

        if (!include_deprecated && entry->deprecated) {
            continue;
        }

        if (!vitte_explain_entry_matches_category(
                entry,
                filter)) {
            continue;
        }

        range = vitte_diagnostic_registry_range(
            entry->public_code
        );

        if (emitted != 0u &&
            !vitte_explain_json_comma(stream)) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        if (fputc('{', stream) == EOF ||
            !vitte_explain_json_string_field(
                stream,
                "code",
                entry->public_code) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "internal",
                entry->internal_code) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "title",
                entry->title) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "category",
                entry->category) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "severity",
                vitte_explain_safe_severity_name(
                    entry->default_severity)) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "phase",
                vitte_explain_safe_origin_name(
                    entry->origin)) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "range",
                vitte_explain_safe_range_name(range)) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_bool_field(
                stream,
                "deprecated",
                entry->deprecated) ||
            fputc('}', stream) == EOF) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        emitted++;
    }

    if (fputs("]}", stream) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

static size_t
vitte_explain_query_score(
    const vitte_diagnostic_registry_entry_t *entry,
    const char *query)
{
    size_t score;

    if (entry == NULL ||
        !vitte_explain_text_present(query)) {
        return 0u;
    }

    score = 0u;

    if (vitte_explain_ascii_equal_case_insensitive(
            entry->public_code,
            query)) {
        score += 10000u;
    }

    if (vitte_explain_ascii_equal_case_insensitive(
            entry->internal_code,
            query)) {
        score += 9000u;
    }

    if (vitte_explain_ascii_equal_case_insensitive(
            entry->title,
            query)) {
        score += 8000u;
    }

    if (vitte_explain_ascii_equal_case_insensitive(
            entry->category,
            query)) {
        score += 7000u;
    }

    if (vitte_explain_ascii_contains_case_insensitive(
            entry->public_code,
            query)) {
        score += 1000u;
    }

    if (vitte_explain_ascii_contains_case_insensitive(
            entry->internal_code,
            query)) {
        score += 900u;
    }

    if (vitte_explain_ascii_contains_case_insensitive(
            entry->title,
            query)) {
        score += 800u;
    }

    if (vitte_explain_ascii_contains_case_insensitive(
            entry->category,
            query)) {
        score += 700u;
    }

    if (vitte_explain_ascii_contains_case_insensitive(
            vitte_explain_safe_origin_name(entry->origin),
            query)) {
        score += 500u;
    }

    if (vitte_explain_ascii_contains_case_insensitive(
            vitte_explain_safe_severity_name(
                entry->default_severity),
            query)) {
        score += 400u;
    }

    if (vitte_explain_ascii_contains_case_insensitive(
            entry->explanation,
            query)) {
        score += 300u;
    }

    {
        const char *guidance;

        guidance = vitte_explain_guidance(entry);

        if (guidance != NULL &&
            vitte_explain_ascii_contains_case_insensitive(
                guidance,
                query)) {
            score += 200u;
        }
    }

    return score;
}

static int
vitte_explain_compare_matches(
    const void *left_pointer,
    const void *right_pointer)
{
    const vitte_explain_match_t *left;
    const vitte_explain_match_t *right;
    int comparison;

    left = (const vitte_explain_match_t *)left_pointer;
    right = (const vitte_explain_match_t *)right_pointer;

    if (left->score != right->score) {
        return left->score > right->score ? -1 : 1;
    }

    comparison = strcmp(
        vitte_explain_safe_text(left->entry->public_code),
        vitte_explain_safe_text(right->entry->public_code)
    );

    if (comparison != 0) {
        return comparison;
    }

    comparison = strcmp(
        vitte_explain_safe_text(left->entry->internal_code),
        vitte_explain_safe_text(right->entry->internal_code)
    );

    if (comparison != 0) {
        return comparison;
    }

    if (left->registry_index != right->registry_index) {
        return left->registry_index < right->registry_index
            ? -1
            : 1;
    }

    return 0;
}

static vitte_status_t
vitte_explain_collect_search_matches(
    const char *query,
    vitte_explain_match_t **matches_out,
    size_t *count_out)
{
    vitte_explain_match_t *matches;
    size_t registry_count;
    size_t index;
    size_t written;

    if (query == NULL ||
        matches_out == NULL ||
        count_out == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    *matches_out = NULL;
    *count_out = 0u;

    registry_count = vitte_diagnostic_registry_count();

    if (registry_count == 0u) {
        return VITTE_STATUS_OK;
    }

    if (registry_count >
        SIZE_MAX / sizeof(*matches)) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    matches = (vitte_explain_match_t *)calloc(
        registry_count,
        sizeof(*matches)
    );

    if (matches == NULL) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    written = 0u;

    for (index = 0u; index < registry_count; index++) {
        const vitte_diagnostic_registry_entry_t *entry;
        size_t score;

        entry = vitte_diagnostic_registry_at(index);

        if (entry == NULL) {
            continue;
        }

        score = vitte_explain_query_score(
            entry,
            query
        );

        if (score == 0u) {
            continue;
        }

        matches[written].entry = entry;
        matches[written].registry_index = index;
        matches[written].score = score;
        written++;
    }

    if (written > 1u) {
        qsort(
            matches,
            written,
            sizeof(*matches),
            vitte_explain_compare_matches
        );
    }

    *matches_out = matches;
    *count_out = written;

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_explain_search(
    FILE *stream,
    const char *query,
    size_t limit)
{
    char normalized[VITTE_EXPLAIN_QUERY_CAPACITY];
    vitte_explain_match_t *matches;
    size_t match_count;
    size_t emitted;
    size_t index;
    vitte_status_t status;

    if (stream == NULL ||
        !vitte_explain_normalize_query(
            query,
            normalized,
            sizeof(normalized))) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    matches = NULL;
    match_count = 0u;

    status = vitte_explain_collect_search_matches(
        normalized,
        &matches,
        &match_count
    );

    if (status != VITTE_STATUS_OK) {
        return status;
    }

    emitted = 0u;

    for (index = 0u; index < match_count; index++) {
        const vitte_diagnostic_registry_entry_t *entry;

        if (limit != 0u && emitted >= limit) {
            break;
        }

        entry = matches[index].entry;

        if (fprintf(
                stream,
                "%-5s  %-18s  %-16s  %-18s  %s\n",
                vitte_explain_safe_text(entry->public_code),
                vitte_explain_safe_text(entry->category),
                vitte_explain_safe_severity_name(
                    entry->default_severity),
                vitte_explain_safe_origin_name(
                    entry->origin),
                vitte_explain_safe_text(entry->title)) < 0) {
            free(matches);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        emitted++;
    }

    free(matches);

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

vitte_status_t
vitte_diagnostic_explain_search_json(
    FILE *stream,
    const char *query,
    size_t limit)
{
    char normalized[VITTE_EXPLAIN_QUERY_CAPACITY];
    vitte_explain_match_t *matches;
    size_t match_count;
    size_t emitted;
    size_t index;
    vitte_status_t status;

    if (stream == NULL ||
        !vitte_explain_normalize_query(
            query,
            normalized,
            sizeof(normalized))) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    matches = NULL;
    match_count = 0u;

    status = vitte_explain_collect_search_matches(
        normalized,
        &matches,
        &match_count
    );

    if (status != VITTE_STATUS_OK) {
        return status;
    }

    if (fputc('{', stream) == EOF ||
        !vitte_explain_json_size_field(
            stream,
            "version",
            VITTE_EXPLAIN_JSON_VERSION) ||
        !vitte_explain_json_comma(stream) ||
        !vitte_explain_json_string_field(
            stream,
            "query",
            normalized) ||
        !vitte_explain_json_comma(stream) ||
        !vitte_explain_json_size_field(
            stream,
            "totalMatches",
            match_count) ||
        !vitte_explain_json_comma(stream) ||
        !vitte_explain_json_key(
            stream,
            "results") ||
        fputc('[', stream) == EOF) {
        free(matches);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    emitted = 0u;

    for (index = 0u; index < match_count; index++) {
        const vitte_diagnostic_registry_entry_t *entry;

        if (limit != 0u && emitted >= limit) {
            break;
        }

        entry = matches[index].entry;

        if (emitted != 0u &&
            !vitte_explain_json_comma(stream)) {
            free(matches);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        if (fputc('{', stream) == EOF ||
            !vitte_explain_json_string_field(
                stream,
                "code",
                entry->public_code) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "internal",
                entry->internal_code) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "title",
                entry->title) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "category",
                entry->category) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "severity",
                vitte_explain_safe_severity_name(
                    entry->default_severity)) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "phase",
                vitte_explain_safe_origin_name(
                    entry->origin)) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_string_field(
                stream,
                "explanation",
                entry->explanation) ||
            !vitte_explain_json_comma(stream) ||
            !vitte_explain_json_size_field(
                stream,
                "score",
                matches[index].score) ||
            fputc('}', stream) == EOF) {
            free(matches);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        emitted++;
    }

    if (fputs("]}", stream) < 0) {
        free(matches);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    free(matches);

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

vitte_status_t
vitte_diagnostic_explain_categories(FILE *stream)
{
    size_t count;
    size_t index;

    if (stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    count = vitte_diagnostic_registry_count();

    for (index = 0u; index < count; index++) {
        const vitte_diagnostic_registry_entry_t *entry;
        size_t previous;
        bool seen;

        entry = vitte_diagnostic_registry_at(index);

        if (entry == NULL ||
            !vitte_explain_text_present(entry->category)) {
            continue;
        }

        seen = false;

        for (previous = 0u; previous < index; previous++) {
            const vitte_diagnostic_registry_entry_t *candidate;

            candidate = vitte_diagnostic_registry_at(previous);

            if (candidate != NULL &&
                vitte_explain_ascii_equal_case_insensitive(
                    candidate->category,
                    entry->category)) {
                seen = true;
                break;
            }
        }

        if (seen) {
            continue;
        }

        if (fprintf(stream, "%s\n", entry->category) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

vitte_status_t
vitte_diagnostic_explain_summary(FILE *stream)
{
    size_t total;
    size_t errors;
    size_t fatals;
    size_t warnings;
    size_t notes;
    size_t helps;
    size_t deprecated;
    size_t index;

    if (stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    total = vitte_diagnostic_registry_count();

    errors = vitte_diagnostic_registry_count_severity(
        VITTE_DIAGNOSTIC_ERROR
    );

    fatals = vitte_diagnostic_registry_count_severity(
        VITTE_DIAGNOSTIC_FATAL
    );

    warnings = vitte_diagnostic_registry_count_severity(
        VITTE_DIAGNOSTIC_WARNING
    );

    notes = vitte_diagnostic_registry_count_severity(
        VITTE_DIAGNOSTIC_NOTE
    );

    helps = vitte_diagnostic_registry_count_severity(
        VITTE_DIAGNOSTIC_HELP
    );

    deprecated = 0u;

    for (index = 0u; index < total; index++) {
        const vitte_diagnostic_registry_entry_t *entry;

        entry = vitte_diagnostic_registry_at(index);

        if (entry != NULL && entry->deprecated) {
            deprecated++;
        }
    }

    if (fprintf(
            stream,
            "Vitte diagnostic registry\n"
            "\n"
            "Total:      %zu\n"
            "Errors:     %zu\n"
            "Fatal:      %zu\n"
            "Warnings:   %zu\n"
            "Notes:      %zu\n"
            "Help:       %zu\n"
            "Deprecated: %zu\n"
            "Registry fingerprint: %016llx\n",
            total,
            errors,
            fatals,
            warnings,
            notes,
            helps,
            deprecated,
            (unsigned long long)
                vitte_diagnostic_registry_fingerprint()) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_explain_stream_ok(stream)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

static size_t
vitte_explain_edit_distance(
    const char *left,
    const char *right)
{
    size_t previous[VITTE_EXPLAIN_CODE_CAPACITY];
    size_t current[VITTE_EXPLAIN_CODE_CAPACITY];
    size_t left_length;
    size_t right_length;
    size_t row;
    size_t column;

    if (left == NULL || right == NULL) {
        return SIZE_MAX;
    }

    left_length = strlen(left);
    right_length = strlen(right);

    if (left_length >= VITTE_EXPLAIN_CODE_CAPACITY ||
        right_length >= VITTE_EXPLAIN_CODE_CAPACITY) {
        return SIZE_MAX;
    }

    for (column = 0u;
         column <= right_length;
         column++) {
        previous[column] = column;
    }

    for (row = 1u; row <= left_length; row++) {
        current[0] = row;

        for (column = 1u;
             column <= right_length;
             column++) {
            size_t deletion;
            size_t insertion;
            size_t substitution;
            size_t minimum;

            deletion = previous[column] + 1u;
            insertion = current[column - 1u] + 1u;

            substitution =
                previous[column - 1u] +
                (
                    vitte_explain_ascii_lower(
                        (unsigned char)left[row - 1u]
                    ) ==
                    vitte_explain_ascii_lower(
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

            current[column] = minimum;
        }

        for (column = 0u;
             column <= right_length;
             column++) {
            previous[column] = current[column];
        }
    }

    return previous[right_length];
}

static bool
vitte_explain_better_suggestion(
    const vitte_diagnostic_registry_entry_t *candidate,
    size_t candidate_distance,
    size_t candidate_index,
    const vitte_diagnostic_registry_entry_t *best,
    size_t best_distance,
    size_t best_index)
{
    int comparison;

    if (candidate == NULL) {
        return false;
    }

    if (best == NULL) {
        return true;
    }

    if (candidate_distance != best_distance) {
        return candidate_distance < best_distance;
    }

    comparison = strcmp(
        vitte_explain_safe_text(candidate->public_code),
        vitte_explain_safe_text(best->public_code)
    );

    if (comparison != 0) {
        return comparison < 0;
    }

    return candidate_index < best_index;
}

const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_explain_suggest(const char *unknown_code)
{
    char normalized[VITTE_EXPLAIN_CODE_CAPACITY];
    const vitte_diagnostic_registry_entry_t *exact;
    const vitte_diagnostic_registry_entry_t *best;
    size_t best_distance;
    size_t best_index;
    size_t index;
    size_t count;
    bool public_like;

    if (!vitte_explain_normalize_code(
            unknown_code,
            normalized,
            sizeof(normalized))) {
        return NULL;
    }

    exact = vitte_diagnostic_explain_find(normalized);

    if (exact != NULL) {
        return exact;
    }

    public_like = vitte_explain_is_public_code(normalized);

    best = NULL;
    best_distance = SIZE_MAX;
    best_index = SIZE_MAX;

    count = vitte_diagnostic_registry_count();

    for (index = 0u; index < count; index++) {
        const vitte_diagnostic_registry_entry_t *entry;
        size_t public_distance;
        size_t internal_distance;
        size_t distance;

        entry = vitte_diagnostic_registry_at(index);

        if (entry == NULL) {
            continue;
        }

        public_distance = vitte_explain_edit_distance(
            normalized,
            entry->public_code
        );

        internal_distance = vitte_explain_edit_distance(
            normalized,
            entry->internal_code
        );

        if (public_like) {
            distance = public_distance;
        } else {
            distance = internal_distance;

            if (public_distance < distance) {
                distance = public_distance;
            }
        }

        if (vitte_explain_better_suggestion(
                entry,
                distance,
                index,
                best,
                best_distance,
                best_index)) {
            best = entry;
            best_distance = distance;
            best_index = index;
        }
    }

    if (best == NULL ||
        best_distance == SIZE_MAX) {
        return NULL;
    }

    if (public_like) {
        return best_distance <= 2u
            ? best
            : NULL;
    }

    {
        size_t length;
        size_t threshold;

        length = strlen(normalized);

        if (length <= 5u) {
            threshold = 2u;
        } else if (length <= 12u) {
            threshold = 3u;
        } else {
            threshold = 4u;
        }

        if (best_distance > threshold) {
            return NULL;
        }
    }

    return best;
}

vitte_status_t
vitte_diagnostic_explain_or_suggest(
    FILE *stream,
    const char *code,
    vitte_diagnostic_explain_format_t format,
    bool include_related)
{
    const vitte_diagnostic_registry_entry_t *entry;

    if (stream == NULL || code == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    entry = vitte_diagnostic_explain_find(code);

    if (entry != NULL) {
        return vitte_diagnostic_explain(
            stream,
            entry->public_code,
            format,
            include_related
        );
    }

    entry = vitte_diagnostic_explain_suggest(code);

    switch (format) {
        case VITTE_DIAGNOSTIC_EXPLAIN_JSON:
            if (fputc('{', stream) == EOF ||
                !vitte_explain_json_string_field(
                    stream,
                    "error",
                    "unknown diagnostic code") ||
                !vitte_explain_json_comma(stream) ||
                !vitte_explain_json_string_field(
                    stream,
                    "query",
                    code) ||
                !vitte_explain_json_comma(stream) ||
                !vitte_explain_json_key(
                    stream,
                    "suggestion")) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            if (entry != NULL) {
                if (fputc('{', stream) == EOF ||
                    !vitte_explain_json_string_field(
                        stream,
                        "code",
                        entry->public_code) ||
                    !vitte_explain_json_comma(stream) ||
                    !vitte_explain_json_string_field(
                        stream,
                        "internal",
                        entry->internal_code) ||
                    !vitte_explain_json_comma(stream) ||
                    !vitte_explain_json_string_field(
                        stream,
                        "title",
                        entry->title) ||
                    fputc('}', stream) == EOF) {
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
            } else if (fputs("null", stream) < 0) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
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
                    code) < 0) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            if (entry != NULL &&
                fprintf(
                    stream,
                    "\nDid you mean `%s` — %s?\n",
                    entry->public_code,
                    entry->title) < 0) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            break;

        case VITTE_DIAGNOSTIC_EXPLAIN_TEXT:
            if (fprintf(
                    stream,
                    "unknown diagnostic code: %s\n",
                    code) < 0) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }

            if (entry != NULL &&
                fprintf(
                    stream,
                    "did you mean %s: %s?\n",
                    entry->public_code,
                    entry->title) < 0) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            break;

        default:
            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (!vitte_explain_stream_ok(stream)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
}

vitte_status_t
vitte_diagnostic_explain_validate_registry(FILE *stream)
{
    vitte_diagnostic_registry_validation_t validation;
    vitte_status_t status;

    memset(&validation, 0, sizeof(validation));

    status = vitte_diagnostic_registry_validate(
        &validation
    );

    if (stream == NULL) {
        return status;
    }

    if (fprintf(
            stream,
            "diagnostic registry: %s\n"
            "entries: %zu\n"
            "invalid entries: %zu\n"
            "duplicate internal identifiers: %zu\n"
            "duplicate public codes: %zu\n",
            validation.valid ? "valid" : "invalid",
            validation.entry_count,
            validation.invalid_entry_count,
            validation.duplicate_internal_count,
            validation.duplicate_public_count) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (!validation.valid &&
        validation.first_invalid_index !=
            VITTE_DIAGNOSTIC_INDEX_NONE) {
        if (fprintf(
                stream,
                "first invalid entry: %zu\n",
                validation.first_invalid_index) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    if (fprintf(
            stream,
            "registry fingerprint: %016llx\n",
            (unsigned long long)
                vitte_diagnostic_registry_fingerprint()) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (!vitte_explain_stream_ok(stream)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return status;
}

const char *
vitte_diagnostic_explain_format_name(
    vitte_diagnostic_explain_format_t format)
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

bool
vitte_diagnostic_explain_parse_format(
    const char *text,
    vitte_diagnostic_explain_format_t *format)
{
    char normalized[32];

    if (format == NULL ||
        !vitte_explain_trim(
            text,
            normalized,
            sizeof(normalized))) {
        return false;
    }

    if (vitte_explain_ascii_equal_case_insensitive(
            normalized,
            "text") ||
        vitte_explain_ascii_equal_case_insensitive(
            normalized,
            "human") ||
        vitte_explain_ascii_equal_case_insensitive(
            normalized,
            "plain")) {
        *format = VITTE_DIAGNOSTIC_EXPLAIN_TEXT;
        return true;
    }

    if (vitte_explain_ascii_equal_case_insensitive(
            normalized,
            "markdown") ||
        vitte_explain_ascii_equal_case_insensitive(
            normalized,
            "md")) {
        *format = VITTE_DIAGNOSTIC_EXPLAIN_MARKDOWN;
        return true;
    }

    if (vitte_explain_ascii_equal_case_insensitive(
            normalized,
            "json")) {
        *format = VITTE_DIAGNOSTIC_EXPLAIN_JSON;
        return true;
    }

    return false;
}

uint64_t
vitte_diagnostic_explain_registry_fingerprint(void)
{
    return vitte_diagnostic_registry_fingerprint();
}

bool
vitte_diagnostic_cli_code_exists(const char *code)
{
    return vitte_diagnostic_explain_find(code) != NULL;
}

bool
vitte_diagnostic_explain_cli(
    FILE *stream,
    const char *code,
    bool json)
{
    vitte_diagnostic_explain_format_t format;

    format = json
        ? VITTE_DIAGNOSTIC_EXPLAIN_JSON
        : VITTE_DIAGNOSTIC_EXPLAIN_TEXT;

    return vitte_diagnostic_explain(
        stream,
        code,
        format,
        false
    ) == VITTE_STATUS_OK;
}