#include "render_json.h"

#include "diagnostic.h"
#include "suggestion.h"
#include "label.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Vitte JSON diagnostic renderer.
 *
 * Goals:
 *
 *   - deterministic machine-readable output;
 *   - complete diagnostic provenance;
 *   - no terminal formatting or ANSI sequences;
 *   - valid JSON for arbitrary diagnostic text;
 *   - explicit schema version;
 *   - byte offsets preserved exactly;
 *   - labels, causes, annotations and suggestions retained;
 *   - cascade/suppression information retained;
 *   - contract diagnostics represented structurally;
 *   - suitable for tooling, CI, editors and tests.
 *
 * Source byte offsets are intentionally emitted as byte offsets.
 * Consumers requiring UTF-16/LSP coordinates must use the LSP renderer.
 */

#define VITTE_DIAGNOSTIC_JSON_SCHEMA_VERSION 1u

/* ========================================================================= */
/* Writer                                                                    */
/* ========================================================================= */

typedef struct vitte_json_writer {
    FILE *stream;
    bool pretty;
    unsigned indent;
    bool failed;
} vitte_json_writer_t;

static void
vitte_json_writer_init(
    vitte_json_writer_t *writer,
    FILE *stream,
    bool pretty
)
{
    if (writer == NULL) {
        return;
    }

    writer->stream = stream;
    writer->pretty = pretty;
    writer->indent = 0u;
    writer->failed = stream == NULL;
}

static bool
vitte_json_ok(
    const vitte_json_writer_t *writer
)
{
    return writer != NULL &&
           writer->stream != NULL &&
           !writer->failed &&
           !ferror(writer->stream);
}

static void
vitte_json_putc(
    vitte_json_writer_t *writer,
    int character
)
{
    if (!vitte_json_ok(writer)) {
        return;
    }

    if (fputc(character, writer->stream) == EOF) {
        writer->failed = true;
    }
}

static void
vitte_json_write(
    vitte_json_writer_t *writer,
    const char *text
)
{
    if (!vitte_json_ok(writer) ||
        text == NULL) {
        return;
    }

    if (fputs(text, writer->stream) < 0) {
        writer->failed = true;
    }
}

static void
vitte_json_indent(
    vitte_json_writer_t *writer
)
{
    unsigned index;

    if (!vitte_json_ok(writer) ||
        !writer->pretty) {
        return;
    }

    for (index = 0u;
         index < writer->indent;
         index++) {

        vitte_json_write(
            writer,
            "  "
        );
    }
}

static void
vitte_json_newline(
    vitte_json_writer_t *writer
)
{
    if (!vitte_json_ok(writer) ||
        !writer->pretty) {
        return;
    }

    vitte_json_putc(writer, '\n');
    vitte_json_indent(writer);
}

static void
vitte_json_after_comma(
    vitte_json_writer_t *writer
)
{
    vitte_json_putc(writer, ',');
    vitte_json_newline(writer);
}

/* ========================================================================= */
/* Strings                                                                   */
/* ========================================================================= */

static void
vitte_json_string(
    vitte_json_writer_t *writer,
    const char *text
)
{
    const unsigned char *cursor;

    if (!vitte_json_ok(writer)) {
        return;
    }

    if (text == NULL) {
        vitte_json_write(writer, "null");
        return;
    }

    vitte_json_putc(writer, '"');

    cursor =
        (const unsigned char *)text;

    while (*cursor != '\0' &&
           vitte_json_ok(writer)) {

        unsigned char character;

        character = *cursor++;

        switch (character) {
            case '"':
                vitte_json_write(writer, "\\\"");
                break;

            case '\\':
                vitte_json_write(writer, "\\\\");
                break;

            case '\b':
                vitte_json_write(writer, "\\b");
                break;

            case '\f':
                vitte_json_write(writer, "\\f");
                break;

            case '\n':
                vitte_json_write(writer, "\\n");
                break;

            case '\r':
                vitte_json_write(writer, "\\r");
                break;

            case '\t':
                vitte_json_write(writer, "\\t");
                break;

            default:
                if (character < 0x20u) {
                    if (fprintf(
                            writer->stream,
                            "\\u%04x",
                            (unsigned)character
                        ) < 0) {

                        writer->failed = true;
                    }
                } else {
                    vitte_json_putc(
                        writer,
                        (int)character
                    );
                }
                break;
        }
    }

    vitte_json_putc(writer, '"');
}

/* ========================================================================= */
/* Scalars                                                                   */
/* ========================================================================= */

static void
vitte_json_bool(
    vitte_json_writer_t *writer,
    bool value
)
{
    vitte_json_write(
        writer,
        value ? "true" : "false"
    );
}

static void
vitte_json_size(
    vitte_json_writer_t *writer,
    size_t value
)
{
    if (!vitte_json_ok(writer)) {
        return;
    }

    if (fprintf(
            writer->stream,
            "%zu",
            value
        ) < 0) {

        writer->failed = true;
    }
}

static void
vitte_json_u64_hex(
    vitte_json_writer_t *writer,
    uint64_t value
)
{
    char buffer[32];

    (void)snprintf(
        buffer,
        sizeof(buffer),
        "%016" PRIx64,
        value
    );

    vitte_json_string(
        writer,
        buffer
    );
}

/* ========================================================================= */
/* Object fields                                                             */
/* ========================================================================= */

static void
vitte_json_key(
    vitte_json_writer_t *writer,
    const char *key
)
{
    vitte_json_string(writer, key);
    vitte_json_putc(writer, ':');

    if (writer != NULL &&
        writer->pretty) {
        vitte_json_putc(writer, ' ');
    }
}

static void
vitte_json_field_string(
    vitte_json_writer_t *writer,
    const char *key,
    const char *value
)
{
    vitte_json_key(writer, key);
    vitte_json_string(writer, value);
}

static void
vitte_json_field_bool(
    vitte_json_writer_t *writer,
    const char *key,
    bool value
)
{
    vitte_json_key(writer, key);
    vitte_json_bool(writer, value);
}

static void
vitte_json_field_size(
    vitte_json_writer_t *writer,
    const char *key,
    size_t value
)
{
    vitte_json_key(writer, key);
    vitte_json_size(writer, value);
}

/* ========================================================================= */
/* Source ID                                                                 */
/* ========================================================================= */

static bool
vitte_json_source_id_valid(
    vitte_source_id_t source_id
)
{
#ifdef VITTE_SOURCE_ID_INVALID
    return source_id !=
           VITTE_SOURCE_ID_INVALID;
#else
    return source_id !=
           (vitte_source_id_t)0;
#endif
}

/* ========================================================================= */
/* Span                                                                      */
/* ========================================================================= */

static void
vitte_json_span(
    vitte_json_writer_t *writer,
    const vitte_ast_span_t *span
)
{
    if (span == NULL ||
        !vitte_ast_span_is_valid(*span)) {

        vitte_json_write(writer, "null");
        return;
    }

    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_string(
        writer,
        "source",
        span->source_name
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "source_id");

    if (vitte_json_source_id_valid(
            span->source_id
        )) {

        vitte_json_size(
            writer,
            (size_t)span->source_id
        );
    } else {
        vitte_json_write(writer, "null");
    }

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "start_byte",
        span->start_offset
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "end_byte",
        span->end_offset
    );

    /*
     * Line/column information is emitted when it exists in the canonical
     * Vitte span representation.
     *
     * These are source-layer coordinates, not LSP UTF-16 positions.
     */
    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "start_line",
        span->start_line
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "start_column",
        span->start_column
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "end_line",
        span->end_line
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "end_column",
        span->end_column
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

/* ========================================================================= */
/* Label                                                                     */
/* ========================================================================= */

static void
vitte_json_label(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_label_t *label
)
{
    if (label == NULL) {
        vitte_json_write(writer, "null");
        return;
    }

    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_string(
        writer,
        "style",
        vitte_diagnostic_label_style_name(
            label->style
        )
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "message",
        label->message
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "span");

    vitte_json_span(
        writer,
        &label->span
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

/* ========================================================================= */
/* Labels                                                                    */
/* ========================================================================= */

static void
vitte_json_labels(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    vitte_json_putc(writer, '[');

    if (diagnostic == NULL ||
        diagnostic->label_count == 0u) {

        vitte_json_putc(writer, ']');
        return;
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        if (index != 0u) {
            vitte_json_after_comma(writer);
        }

        vitte_json_label(
            writer,
            &diagnostic->labels[index]
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, ']');
}

/* ========================================================================= */
/* Location                                                                  */
/* ========================================================================= */

static const char *
vitte_json_location_kind_name(
    vitte_diagnostic_location_kind_t kind
)
{
    switch (kind) {
        case VITTE_DIAGNOSTIC_LOCATION_PRIMARY:
            return "primary";

        case VITTE_DIAGNOSTIC_LOCATION_DECLARATION:
            return "declaration";

        case VITTE_DIAGNOSTIC_LOCATION_DEFINITION:
            return "definition";

        case VITTE_DIAGNOSTIC_LOCATION_ORIGIN:
            return "origin";

        case VITTE_DIAGNOSTIC_LOCATION_USE:
            return "use";

        case VITTE_DIAGNOSTIC_LOCATION_CALL_SITE:
            return "call-site";

        case VITTE_DIAGNOSTIC_LOCATION_INSTANTIATION:
            return "instantiation";

        case VITTE_DIAGNOSTIC_LOCATION_RELATED:
            return "related";

        default:
            return "unknown";
    }
}

static void
vitte_json_location(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_location_t *location
)
{
    if (location == NULL) {
        vitte_json_write(writer, "null");
        return;
    }

    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_string(
        writer,
        "kind",
        vitte_json_location_kind_name(
            location->kind
        )
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "label",
        location->label
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "span");

    vitte_json_span(
        writer,
        &location->span
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

static void
vitte_json_related_locations(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    vitte_json_putc(writer, '[');

    if (diagnostic == NULL ||
        diagnostic->related_count == 0u) {

        vitte_json_putc(writer, ']');
        return;
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    for (index = 0u;
         index < diagnostic->related_count;
         index++) {

        if (index != 0u) {
            vitte_json_after_comma(writer);
        }

        vitte_json_location(
            writer,
            &diagnostic->related[index]
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, ']');
}

/* ========================================================================= */
/* Cause                                                                     */
/* ========================================================================= */

static void
vitte_json_cause(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_cause_t *cause
)
{
    if (cause == NULL) {
        vitte_json_write(writer, "null");
        return;
    }

    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_string(
        writer,
        "message",
        cause->message
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "source",
        cause->has_span ? cause->span.source_name : NULL
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "span");

    if (cause->has_span) {
        vitte_json_span(
            writer,
            &cause->span
        );
    } else {
        vitte_json_write(writer, "null");
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

static void
vitte_json_causes(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    vitte_json_putc(writer, '[');

    if (diagnostic == NULL ||
        diagnostic->cause_count == 0u) {

        vitte_json_putc(writer, ']');
        return;
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    for (index = 0u;
         index < diagnostic->cause_count;
         index++) {

        if (index != 0u) {
            vitte_json_after_comma(writer);
        }

        vitte_json_cause(
            writer,
            &diagnostic->causes[index]
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, ']');
}

/* ========================================================================= */
/* Annotation                                                                */
/* ========================================================================= */

static const char *
vitte_json_annotation_kind_name(
    vitte_diagnostic_annotation_kind_t kind
)
{
    switch (kind) {
        case VITTE_DIAGNOSTIC_ANNOTATION_NOTE:
            return "note";

        case VITTE_DIAGNOSTIC_ANNOTATION_HELP:
            return "help";

        default:
            return "unknown";
    }
}

static void
vitte_json_annotation(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_annotation_t *annotation
)
{
    if (annotation == NULL) {
        vitte_json_write(writer, "null");
        return;
    }

    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_string(
        writer,
        "kind",
        vitte_json_annotation_kind_name(
            annotation->kind
        )
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "message",
        annotation->message
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "source",
        annotation->has_span ? annotation->span.source_name : NULL
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "span");

    if (annotation->has_span) {
        vitte_json_span(
            writer,
            &annotation->span
        );
    } else {
        vitte_json_write(writer, "null");
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

static void
vitte_json_annotations(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    vitte_json_putc(writer, '[');

    if (diagnostic == NULL ||
        diagnostic->annotation_count == 0u) {

        vitte_json_putc(writer, ']');
        return;
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    for (index = 0u;
         index < diagnostic->annotation_count;
         index++) {

        if (index != 0u) {
            vitte_json_after_comma(writer);
        }

        vitte_json_annotation(
            writer,
            &diagnostic->annotations[index]
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, ']');
}

/* ========================================================================= */
/* Suggestion                                                                */
/* ========================================================================= */

static void
vitte_json_suggestion(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    if (suggestion == NULL) {
        vitte_json_write(writer, "null");
        return;
    }

    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_string(
        writer,
        "message",
        suggestion->message
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "replacement",
        suggestion->replacement
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "applicability",
        vitte_diagnostic_applicability_name(
            suggestion->applicability
        )
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "span");

    if (suggestion->has_span) {
        vitte_json_span(
            writer,
            &suggestion->span
        );
    } else {
        vitte_json_write(writer, "null");
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

static void
vitte_json_suggestions(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    vitte_json_putc(writer, '[');

    if (diagnostic == NULL ||
        diagnostic->suggestion_count == 0u) {

        vitte_json_putc(writer, ']');
        return;
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    for (index = 0u;
         index < diagnostic->suggestion_count;
         index++) {

        if (index != 0u) {
            vitte_json_after_comma(writer);
        }

        vitte_json_suggestion(
            writer,
            &diagnostic->suggestions[index]
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, ']');
}

/* ========================================================================= */
/* Contract                                                                  */
/* ========================================================================= */

static const char *
vitte_json_contract_kind_name(
    vitte_diagnostic_contract_kind_t kind
)
{
    switch (kind) {
        case VITTE_DIAGNOSTIC_CONTRACT_NONE:
            return "none";

        case VITTE_DIAGNOSTIC_CONTRACT_REQUIRES:
            return "requires";

        case VITTE_DIAGNOSTIC_CONTRACT_ENSURES:
            return "ensures";

        case VITTE_DIAGNOSTIC_CONTRACT_INVARIANT:
            return "invariant";

        case VITTE_DIAGNOSTIC_CONTRACT_ASSERTION:
            return "assertion";

        default:
            return "unknown";
    }
}

static void
vitte_json_contract(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    const vitte_diagnostic_contract_t *contract;

    if (diagnostic == NULL ||
        !diagnostic->has_contract) {

        vitte_json_write(writer, "null");
        return;
    }

    contract =
        &diagnostic->contract;

    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_string(
        writer,
        "kind",
        vitte_json_contract_kind_name(
            contract->kind
        )
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "expression",
        contract->expression
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "procedure",
        contract->procedure
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "reason",
        contract->reason
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "contract_span");

    if (contract->has_contract_span) {
        vitte_json_span(
            writer,
            &contract->contract_span
        );
    } else {
        vitte_json_write(writer, "null");
    }

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "call_span");

    if (contract->has_call_span) {
        vitte_json_span(
            writer,
            &contract->call_span
        );
    } else {
        vitte_json_write(writer, "null");
    }

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "declaration_span");

    if (contract->has_declaration_span) {
        vitte_json_span(
            writer,
            &contract->declaration_span
        );
    } else {
        vitte_json_write(writer, "null");
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

static void
vitte_json_context(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_string(
        writer,
        "package",
        diagnostic->package
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "module",
        diagnostic->module
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "procedure",
        diagnostic->procedure
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "symbol",
        diagnostic->symbol
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "subject",
        diagnostic->symbol
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "context",
        diagnostic->context
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "expected_type",
        diagnostic->expected_type
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "actual_type",
        diagnostic->actual_type
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

/* ========================================================================= */
/* Cascade                                                                   */
/* ========================================================================= */

static void
vitte_json_optional_index(
    vitte_json_writer_t *writer,
    size_t index
)
{
    if (index ==
        VITTE_DIAGNOSTIC_INDEX_NONE) {

        vitte_json_write(writer, "null");
        return;
    }

    vitte_json_size(writer, index);
}

static void
vitte_json_cascade(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_bool(
        writer,
        "primary",
        diagnostic->is_primary
    );

    vitte_json_after_comma(writer);

    vitte_json_field_bool(
        writer,
        "cascade",
        diagnostic->is_cascade
    );

    vitte_json_after_comma(writer);

    vitte_json_field_bool(
        writer,
        "suppressed",
        diagnostic->is_suppressed
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "parent_index");

    vitte_json_optional_index(
        writer,
        diagnostic->parent_diagnostic
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "root_index");

    vitte_json_optional_index(
        writer,
        diagnostic->root_diagnostic
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

/* ========================================================================= */
/* Primary source position                                                   */
/* ========================================================================= */

static void
vitte_json_primary_position(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_string(
        writer,
        "source",
        diagnostic->source_name
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "source_id");

    if (vitte_json_source_id_valid(
            diagnostic->source_id
        )) {

        vitte_json_size(
            writer,
            (size_t)diagnostic->source_id
        );
    } else {
        vitte_json_write(writer, "null");
    }

    vitte_json_after_comma(writer);

    vitte_json_field_bool(
        writer,
        "has_span",
        diagnostic->has_span
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "start_byte",
        diagnostic->start_offset
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "end_byte",
        diagnostic->end_offset
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "start_line",
        diagnostic->start_line
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "start_column",
        diagnostic->start_column
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "end_line",
        diagnostic->end_line
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "end_column",
        diagnostic->end_column
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

/* ========================================================================= */
/* One diagnostic                                                           */
/* ========================================================================= */

static void
vitte_json_diagnostic(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic,
    size_t index,
    bool include_suppressed_metadata
)
{
    if (diagnostic == NULL) {
        vitte_json_write(writer, "null");
        return;
    }

    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_size(
        writer,
        "index",
        index
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "code",
        diagnostic->short_code
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "internal_code",
        diagnostic->code
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "severity",
        vitte_diagnostic_severity_name(
            diagnostic->severity
        )
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "phase",
        vitte_diagnostic_origin_name(
            diagnostic->origin
        )
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "category",
        diagnostic->category
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "message",
        diagnostic->message
    );

    vitte_json_after_comma(writer);

    vitte_json_field_string(
        writer,
        "details",
        diagnostic->details
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "fingerprint");
    vitte_json_u64_hex(
        writer,
        diagnostic->fingerprint
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "position");
    vitte_json_primary_position(
        writer,
        diagnostic
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "context");
    vitte_json_context(
        writer,
        diagnostic
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "labels");
    vitte_json_labels(
        writer,
        diagnostic
    );

    vitte_json_after_comma(writer);

    vitte_json_key(
        writer,
        "related_locations"
    );

    vitte_json_related_locations(
        writer,
        diagnostic
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "causes");
    vitte_json_causes(
        writer,
        diagnostic
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "annotations");
    vitte_json_annotations(
        writer,
        diagnostic
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "suggestions");
    vitte_json_suggestions(
        writer,
        diagnostic
    );

    vitte_json_after_comma(writer);

    vitte_json_key(writer, "contract");
    vitte_json_contract(
        writer,
        diagnostic
    );

    if (include_suppressed_metadata) {
        vitte_json_after_comma(writer);

        vitte_json_key(writer, "cascade");
        vitte_json_cascade(
            writer,
            diagnostic
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

/* ========================================================================= */
/* Counts                                                                    */
/* ========================================================================= */

static void
vitte_json_counts(
    vitte_json_writer_t *writer,
    const vitte_diagnostic_counts_t *counts
)
{
    vitte_json_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_json_newline(writer);
    }

    vitte_json_field_size(
        writer,
        "note",
        counts != NULL
            ? counts->note_count
            : 0u
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "help",
        counts != NULL
            ? counts->help_count
            : 0u
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "warning",
        counts != NULL
            ? counts->warning_count
            : 0u
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "error",
        counts != NULL
            ? counts->error_count
            : 0u
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "fatal",
        counts != NULL
            ? counts->fatal_count
            : 0u
    );

    vitte_json_after_comma(writer);

    vitte_json_field_size(
        writer,
        "suppressed",
        counts != NULL
            ? counts->suppressed_count
            : 0u
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_json_newline(writer);
    }

    vitte_json_putc(writer, '}');
}

/* ========================================================================= */
/* One diagnostic API                                                        */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_render_json_one_ex(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_json_options_t *options
)
{
    vitte_json_writer_t writer;
    vitte_diagnostic_json_options_t effective;

    if (stream == NULL ||
        diagnostic == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_json_options_init(
        &effective
    );

    if (options != NULL) {
        effective = *options;
    }

    vitte_json_writer_init(
        &writer,
        stream,
        effective.pretty
    );

    vitte_json_diagnostic(
        &writer,
        diagnostic,
        0u,
        effective.include_suppressed_metadata
    );

    if (effective.trailing_newline) {
        vitte_json_putc(
            &writer,
            '\n'
        );
    }

    return vitte_json_ok(&writer)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

vitte_status_t
vitte_diagnostic_render_json_one(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
)
{
    vitte_diagnostic_json_options_t options;

    vitte_diagnostic_json_options_init(
        &options
    );

    return vitte_diagnostic_render_json_one_ex(
        stream,
        diagnostic,
        &options
    );
}

/* ========================================================================= */
/* Bag rendering                                                             */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_render_json_bag(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_json_options_t *options
)
{
    vitte_json_writer_t writer;
    vitte_diagnostic_json_options_t effective;

    size_t index;
    size_t emitted;

    if (stream == NULL ||
        bag == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_json_options_init(
        &effective
    );

    if (options != NULL) {
        effective = *options;
    }

    vitte_json_writer_init(
        &writer,
        stream,
        effective.pretty
    );

    vitte_json_putc(&writer, '{');

    if (writer.pretty) {
        writer.indent++;
        vitte_json_newline(&writer);
    }

    vitte_json_key(
        &writer,
        "schema"
    );

    vitte_json_string(
        &writer,
        "vitte-diagnostics"
    );

    vitte_json_after_comma(&writer);

    vitte_json_key(
        &writer,
        "version"
    );

    vitte_json_size(
        &writer,
        VITTE_DIAGNOSTIC_JSON_SCHEMA_VERSION
    );

    vitte_json_after_comma(&writer);

    vitte_json_key(
        &writer,
        "counts"
    );

    vitte_json_counts(
        &writer,
        &bag->counts
    );

    vitte_json_after_comma(&writer);

    vitte_json_key(
        &writer,
        "stored"
    );

    vitte_json_size(
        &writer,
        bag->count
    );

    vitte_json_after_comma(&writer);

    vitte_json_key(
        &writer,
        "diagnostics"
    );

    vitte_json_putc(
        &writer,
        '['
    );

    emitted = 0u;

    for (index = 0u;
         index < bag->count;
         index++) {

        const vitte_diagnostic_t *diagnostic;

        diagnostic =
            &bag->storage[index];

        if (diagnostic->is_suppressed &&
            !effective.include_suppressed) {

            continue;
        }

        if (emitted != 0u) {
            vitte_json_after_comma(
                &writer
            );
        } else if (writer.pretty) {
            writer.indent++;
            vitte_json_newline(
                &writer
            );
        }

        vitte_json_diagnostic(
            &writer,
            diagnostic,
            index,
            effective.include_suppressed_metadata
        );

        emitted++;
    }

    if (writer.pretty &&
        emitted != 0u) {

        writer.indent--;
        vitte_json_newline(
            &writer
        );
    }

    vitte_json_putc(
        &writer,
        ']'
    );

    vitte_json_after_comma(
        &writer
    );

    vitte_json_key(
        &writer,
        "emitted"
    );

    vitte_json_size(
        &writer,
        emitted
    );

    if (writer.pretty) {
        writer.indent--;
        vitte_json_newline(
            &writer
        );
    }

    vitte_json_putc(
        &writer,
        '}'
    );

    if (effective.trailing_newline) {
        vitte_json_putc(
            &writer,
            '\n'
        );
    }

    return vitte_json_ok(&writer)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* JSON Lines                                                                */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_render_json_lines(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_json_options_t *options
)
{
    vitte_json_writer_t writer;
    vitte_diagnostic_json_options_t effective;

    size_t index;

    if (stream == NULL ||
        bag == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_json_options_init(
        &effective
    );

    if (options != NULL) {
        effective = *options;
    }

    /*
     * JSON Lines must keep one complete JSON value per physical line.
     */
    effective.pretty = false;

    vitte_json_writer_init(
        &writer,
        stream,
        false
    );

    for (index = 0u;
         index < bag->count;
         index++) {

        const vitte_diagnostic_t *diagnostic;

        diagnostic =
            &bag->storage[index];

        if (diagnostic->is_suppressed &&
            !effective.include_suppressed) {

            continue;
        }

        vitte_json_diagnostic(
            &writer,
            diagnostic,
            index,
            effective.include_suppressed_metadata
        );

        vitte_json_putc(
            &writer,
            '\n'
        );

        if (!vitte_json_ok(
                &writer
            )) {

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Options                                                                   */
/* ========================================================================= */

void
vitte_diagnostic_json_options_init(
    vitte_diagnostic_json_options_t *options
)
{
    if (options == NULL) {
        return;
    }

    memset(
        options,
        0,
        sizeof(*options)
    );

    options->pretty = false;
    options->include_suppressed = false;
    options->include_suppressed_metadata = true;
    options->trailing_newline = true;
}

/* ========================================================================= */
/* Schema                                                                    */
/* ========================================================================= */

unsigned
vitte_diagnostic_json_schema_version(void)
{
    return VITTE_DIAGNOSTIC_JSON_SCHEMA_VERSION;
}

const char *
vitte_diagnostic_json_schema_name(void)
{
    return "vitte-diagnostics";
}

/* ========================================================================= */
/* Compatibility adapter                                                     */
/* ========================================================================= */

/*
 * Adapter intended for the generic diagnostic rendering layer.
 *
 * This keeps JSON implementation details outside diagnostic.c.
 */
vitte_status_t
vitte_diagnostic_write_json(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_render_json_one(
        stream,
        diagnostic
    );
}

vitte_status_t
vitte_diagnostic_write_json_bag(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag
)
{
    vitte_diagnostic_json_options_t options;

    vitte_diagnostic_json_options_init(
        &options
    );

    return vitte_diagnostic_render_json_bag(
        stream,
        bag,
        &options
    );
}
