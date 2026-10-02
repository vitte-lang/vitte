#include "render_lsp.h"

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
 * Vitte Language Server Protocol diagnostic renderer.
 *
 * This module serializes Vitte diagnostics into JSON structures compatible
 * with LSP diagnostics.
 *
 * Important:
 *
 * Vitte compiler spans use source byte offsets.
 *
 * LSP positions normally use UTF-16 code units:
 *
 *     Position.line
 *     Position.character
 *
 * Byte offsets MUST NOT be copied directly into LSP "character" fields.
 *
 * Conversion is delegated through the configured position callback so the
 * source manager remains the authoritative source-position implementation.
 *
 * This renderer supports:
 *
 *   - Diagnostic;
 *   - Range;
 *   - DiagnosticRelatedInformation;
 *   - tags;
 *   - code;
 *   - source;
 *   - data;
 *   - structured Vitte metadata;
 *   - labels;
 *   - causes;
 *   - suggestions;
 *   - contract metadata;
 *   - cascade metadata;
 *   - publishDiagnostics payloads.
 */

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_LSP_SOURCE_NAME "vitte"
#define VITTE_LSP_DATA_VERSION 1u

/* ========================================================================= */
/* JSON writer                                                               */
/* ========================================================================= */

typedef struct vitte_lsp_json_writer {
    FILE *stream;
    bool pretty;
    unsigned indent;
    bool failed;
} vitte_lsp_json_writer_t;

static void
vitte_lsp_writer_init(
    vitte_lsp_json_writer_t *writer,
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
vitte_lsp_writer_ok(
    const vitte_lsp_json_writer_t *writer
)
{
    return writer != NULL &&
           writer->stream != NULL &&
           !writer->failed &&
           !ferror(writer->stream);
}

static void
vitte_lsp_putc(
    vitte_lsp_json_writer_t *writer,
    int character
)
{
    if (!vitte_lsp_writer_ok(writer)) {
        return;
    }

    if (fputc(character, writer->stream) == EOF) {
        writer->failed = true;
    }
}

static void
vitte_lsp_write(
    vitte_lsp_json_writer_t *writer,
    const char *text
)
{
    if (!vitte_lsp_writer_ok(writer) ||
        text == NULL) {
        return;
    }

    if (fputs(text, writer->stream) < 0) {
        writer->failed = true;
    }
}

static void
vitte_lsp_indent(
    vitte_lsp_json_writer_t *writer
)
{
    unsigned index;

    if (!vitte_lsp_writer_ok(writer) ||
        !writer->pretty) {
        return;
    }

    for (index = 0u;
         index < writer->indent;
         index++) {

        vitte_lsp_write(
            writer,
            "  "
        );
    }
}

static void
vitte_lsp_newline(
    vitte_lsp_json_writer_t *writer
)
{
    if (!vitte_lsp_writer_ok(writer) ||
        !writer->pretty) {
        return;
    }

    vitte_lsp_putc(writer, '\n');
    vitte_lsp_indent(writer);
}

static void
vitte_lsp_comma(
    vitte_lsp_json_writer_t *writer
)
{
    vitte_lsp_putc(writer, ',');
    vitte_lsp_newline(writer);
}

/* ========================================================================= */
/* JSON strings                                                              */
/* ========================================================================= */

static bool
vitte_lsp_text_present(
    const char *text
)
{
    return text != NULL &&
           text[0] != '\0';
}

static void
vitte_lsp_json_string(
    vitte_lsp_json_writer_t *writer,
    const char *text
)
{
    const unsigned char *cursor;

    if (!vitte_lsp_writer_ok(writer)) {
        return;
    }

    if (text == NULL) {
        vitte_lsp_write(writer, "null");
        return;
    }

    vitte_lsp_putc(writer, '"');

    cursor =
        (const unsigned char *)text;

    while (*cursor != '\0' &&
           vitte_lsp_writer_ok(writer)) {

        unsigned char character;

        character = *cursor++;

        switch (character) {
            case '"':
                vitte_lsp_write(writer, "\\\"");
                break;

            case '\\':
                vitte_lsp_write(writer, "\\\\");
                break;

            case '\b':
                vitte_lsp_write(writer, "\\b");
                break;

            case '\f':
                vitte_lsp_write(writer, "\\f");
                break;

            case '\n':
                vitte_lsp_write(writer, "\\n");
                break;

            case '\r':
                vitte_lsp_write(writer, "\\r");
                break;

            case '\t':
                vitte_lsp_write(writer, "\\t");
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
                    vitte_lsp_putc(
                        writer,
                        (int)character
                    );
                }
                break;
        }
    }

    vitte_lsp_putc(writer, '"');
}

/* ========================================================================= */
/* JSON scalars                                                              */
/* ========================================================================= */

static void
vitte_lsp_json_bool(
    vitte_lsp_json_writer_t *writer,
    bool value
)
{
    vitte_lsp_write(
        writer,
        value ? "true" : "false"
    );
}

static void
vitte_lsp_json_size(
    vitte_lsp_json_writer_t *writer,
    size_t value
)
{
    if (!vitte_lsp_writer_ok(writer)) {
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
vitte_lsp_json_int(
    vitte_lsp_json_writer_t *writer,
    int value
)
{
    if (!vitte_lsp_writer_ok(writer)) {
        return;
    }

    if (fprintf(
            writer->stream,
            "%d",
            value
        ) < 0) {

        writer->failed = true;
    }
}

static void
vitte_lsp_json_u64_hex(
    vitte_lsp_json_writer_t *writer,
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

    vitte_lsp_json_string(
        writer,
        buffer
    );
}

/* ========================================================================= */
/* Object fields                                                             */
/* ========================================================================= */

static void
vitte_lsp_key(
    vitte_lsp_json_writer_t *writer,
    const char *key
)
{
    vitte_lsp_json_string(writer, key);
    vitte_lsp_putc(writer, ':');

    if (writer != NULL &&
        writer->pretty) {

        vitte_lsp_putc(writer, ' ');
    }
}

static void
vitte_lsp_field_string(
    vitte_lsp_json_writer_t *writer,
    const char *key,
    const char *value
)
{
    vitte_lsp_key(writer, key);
    vitte_lsp_json_string(writer, value);
}

static void
vitte_lsp_field_bool(
    vitte_lsp_json_writer_t *writer,
    const char *key,
    bool value
)
{
    vitte_lsp_key(writer, key);
    vitte_lsp_json_bool(writer, value);
}

static void
vitte_lsp_field_size(
    vitte_lsp_json_writer_t *writer,
    const char *key,
    size_t value
)
{
    vitte_lsp_key(writer, key);
    vitte_lsp_json_size(writer, value);
}

/* ========================================================================= */
/* Options                                                                   */
/* ========================================================================= */

void
vitte_diagnostic_lsp_options_init(
    vitte_diagnostic_lsp_options_t *options
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
    options->trailing_newline = true;

    options->include_suppressed = false;
    options->include_related_information = true;
    options->include_data = true;
    options->include_labels_in_data = true;
    options->include_suggestions_in_data = true;
    options->include_causes_in_data = true;
    options->include_contract_in_data = true;
    options->include_cascade_in_data = true;

    options->source_name =
        VITTE_LSP_SOURCE_NAME;

    options->position_encoding =
        VITTE_DIAGNOSTIC_LSP_POSITION_UTF16;

    options->position_converter = NULL;
    options->position_user_data = NULL;

    options->uri_converter = NULL;
    options->uri_user_data = NULL;
}

/* ========================================================================= */
/* Position encoding                                                         */
/* ========================================================================= */

const char *
vitte_diagnostic_lsp_position_encoding_name(
    vitte_diagnostic_lsp_position_encoding_t encoding
)
{
    switch (encoding) {
        case VITTE_DIAGNOSTIC_LSP_POSITION_UTF8:
            return "utf-8";

        case VITTE_DIAGNOSTIC_LSP_POSITION_UTF16:
            return "utf-16";

        case VITTE_DIAGNOSTIC_LSP_POSITION_UTF32:
            return "utf-32";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Severity                                                                  */
/* ========================================================================= */

/*
 * LSP DiagnosticSeverity:
 *
 *     1 Error
 *     2 Warning
 *     3 Information
 *     4 Hint
 */
int
vitte_diagnostic_lsp_severity(
    vitte_diagnostic_severity_t severity
)
{
    switch (severity) {
        case VITTE_DIAGNOSTIC_FATAL:
        case VITTE_DIAGNOSTIC_ERROR:
            return 1;

        case VITTE_DIAGNOSTIC_WARNING:
            return 2;

        case VITTE_DIAGNOSTIC_NOTE:
            return 3;

        case VITTE_DIAGNOSTIC_HELP:
            return 4;

        default:
            return 3;
    }
}

/* ========================================================================= */
/* Position conversion                                                       */
/* ========================================================================= */

static bool
vitte_lsp_position_valid(
    const vitte_diagnostic_lsp_position_t *position
)
{
    return position != NULL &&
           position->valid;
}

static bool
vitte_lsp_convert_position(
    const vitte_diagnostic_lsp_options_t *options,
    vitte_source_id_t source_id,
    const char *source_name,
    size_t byte_offset,
    vitte_diagnostic_lsp_position_t *position
)
{
    if (position == NULL) {
        return false;
    }

    memset(
        position,
        0,
        sizeof(*position)
    );

    if (options == NULL ||
        options->position_converter == NULL) {

        /*
         * No silent byte-offset -> UTF-16 conversion.
         *
         * The source manager must provide the correct conversion.
         */
        return false;
    }

    if (!options->position_converter(
            source_id,
            source_name,
            byte_offset,
            options->position_encoding,
            position,
            options->position_user_data
        )) {

        memset(
            position,
            0,
            sizeof(*position)
        );

        return false;
    }

    return vitte_lsp_position_valid(
        position
    );
}

/* ========================================================================= */
/* Range conversion                                                          */
/* ========================================================================= */

static bool
vitte_lsp_range_from_span(
    const vitte_diagnostic_lsp_options_t *options,
    const vitte_ast_span_t *span,
    vitte_diagnostic_lsp_range_t *range
)
{
    if (range == NULL) {
        return false;
    }

    memset(
        range,
        0,
        sizeof(*range)
    );

    if (span == NULL ||
        !vitte_ast_span_is_valid(*span)) {

        return false;
    }

    if (!vitte_lsp_convert_position(
            options,
            span->source_id,
            span->source_name,
            span->start_offset,
            &range->start
        )) {

        return false;
    }

    if (!vitte_lsp_convert_position(
            options,
            span->source_id,
            span->source_name,
            span->end_offset,
            &range->end
        )) {

        return false;
    }

    range->valid = true;

    return true;
}

/* ========================================================================= */
/* Diagnostic primary range                                                  */
/* ========================================================================= */

static bool
vitte_lsp_primary_range(
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_lsp_options_t *options,
    vitte_diagnostic_lsp_range_t *range
)
{
    size_t index;

    if (diagnostic == NULL ||
        range == NULL) {
        return false;
    }

    /*
     * Prefer an explicit primary label because it is the most precise
     * structured source range.
     */
    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        const vitte_diagnostic_label_t *label;

        label =
            &diagnostic->labels[index];

        if (label->style !=
            VITTE_DIAGNOSTIC_LABEL_PRIMARY) {

            continue;
        }

        if (vitte_lsp_range_from_span(
                options,
                &label->span,
                range
            )) {

            return true;
        }
    }

    /*
     * Fall back to the diagnostic's compact primary position.
     */
    if (diagnostic->has_span) {
        vitte_diagnostic_lsp_position_t start;
        vitte_diagnostic_lsp_position_t end;

        memset(&start, 0, sizeof(start));
        memset(&end, 0, sizeof(end));

        if (vitte_lsp_convert_position(
                options,
                diagnostic->source_id,
                diagnostic->source_name,
                diagnostic->start_offset,
                &start
            ) &&
            vitte_lsp_convert_position(
                options,
                diagnostic->source_id,
                diagnostic->source_name,
                diagnostic->end_offset,
                &end
            )) {

            range->start = start;
            range->end = end;
            range->valid = true;

            return true;
        }
    }

    return false;
}

/* ========================================================================= */
/* JSON position                                                             */
/* ========================================================================= */

static void
vitte_lsp_render_position(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_lsp_position_t *position
)
{
    vitte_lsp_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_field_size(
        writer,
        "line",
        position != NULL
            ? position->line
            : 0u
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_size(
        writer,
        "character",
        position != NULL
            ? position->character
            : 0u
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, '}');
}

/* ========================================================================= */
/* JSON range                                                                */
/* ========================================================================= */

static void
vitte_lsp_render_range(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_lsp_range_t *range
)
{
    vitte_lsp_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_key(writer, "start");

    vitte_lsp_render_position(
        writer,
        range != NULL
            ? &range->start
            : NULL
    );

    vitte_lsp_comma(writer);

    vitte_lsp_key(writer, "end");

    vitte_lsp_render_position(
        writer,
        range != NULL
            ? &range->end
            : NULL
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, '}');
}

/* ========================================================================= */
/* URI conversion                                                            */
/* ========================================================================= */

static bool
vitte_lsp_uri(
    const vitte_diagnostic_lsp_options_t *options,
    vitte_source_id_t source_id,
    const char *source_name,
    char *buffer,
    size_t capacity
)
{
    size_t length;

    if (buffer == NULL ||
        capacity == 0u) {

        return false;
    }

    buffer[0] = '\0';

    if (options != NULL &&
        options->uri_converter != NULL) {

        if (options->uri_converter(
                source_id,
                source_name,
                buffer,
                capacity,
                options->uri_user_data
            )) {

            return buffer[0] != '\0';
        }
    }

    /*
     * Do not manufacture an incorrect file:// URI.
     *
     * If no URI converter exists, preserve the source identifier verbatim.
     * The LSP integration layer should normally install a converter.
     */
    if (!vitte_lsp_text_present(
            source_name
        )) {

        return false;
    }

    length = strlen(source_name);

    if (length >= capacity) {
        return false;
    }

    memcpy(
        buffer,
        source_name,
        length + 1u
    );

    return true;
}

/* ========================================================================= */
/* Related information                                                       */
/* ========================================================================= */

static void
vitte_lsp_related_item(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_lsp_options_t *options,
    const vitte_ast_span_t *span,
    const char *message
)
{
    vitte_diagnostic_lsp_range_t range;
    char uri[VITTE_DIAGNOSTIC_LSP_URI_CAPACITY];

    memset(
        &range,
        0,
        sizeof(range)
    );

    uri[0] = '\0';

    if (!vitte_lsp_range_from_span(
            options,
            span,
            &range
        ) ||
        !vitte_lsp_uri(
            options,
            span->source_id,
            span->source_name,
            uri,
            sizeof(uri)
        )) {

        vitte_lsp_write(
            writer,
            "null"
        );

        return;
    }

    vitte_lsp_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_key(
        writer,
        "location"
    );

    vitte_lsp_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_field_string(
        writer,
        "uri",
        uri
    );

    vitte_lsp_comma(writer);

    vitte_lsp_key(
        writer,
        "range"
    );

    vitte_lsp_render_range(
        writer,
        &range
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, '}');

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "message",
        vitte_lsp_text_present(message)
            ? message
            : "related location"
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, '}');
}

/* ========================================================================= */
/* Related information array                                                 */
/* ========================================================================= */

static void
vitte_lsp_related_information(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_lsp_options_t *options
)
{
    size_t index;
    size_t emitted;

    vitte_lsp_putc(writer, '[');

    emitted = 0u;

    /*
     * Secondary labels are useful related locations.
     */
    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        const vitte_diagnostic_label_t *label;
        vitte_diagnostic_lsp_range_t range;
        char uri[VITTE_DIAGNOSTIC_LSP_URI_CAPACITY];

        label =
            &diagnostic->labels[index];

        if (label->style ==
            VITTE_DIAGNOSTIC_LABEL_PRIMARY) {

            continue;
        }

        if (!vitte_lsp_range_from_span(
                options,
                &label->span,
                &range
            )) {

            continue;
        }

        if (!vitte_lsp_uri(
                options,
                label->span.source_id,
                label->span.source_name,
                uri,
                sizeof(uri)
            )) {

            continue;
        }

        if (emitted != 0u) {
            vitte_lsp_comma(writer);
        } else if (writer->pretty) {
            writer->indent++;
            vitte_lsp_newline(writer);
        }

        vitte_lsp_related_item(
            writer,
            options,
            &label->span,
            label->message
        );

        emitted++;
    }

    /*
     * Explicit related locations.
     */
    for (index = 0u;
         index < diagnostic->related_count;
         index++) {

        const vitte_diagnostic_location_t *location;
        vitte_diagnostic_lsp_range_t range;
        char uri[VITTE_DIAGNOSTIC_LSP_URI_CAPACITY];

        location =
            &diagnostic->related[index];

        if (!vitte_lsp_range_from_span(
                options,
                &location->span,
                &range
            )) {

            continue;
        }

        if (!vitte_lsp_uri(
                options,
                location->span.source_id,
                location->span.source_name,
                uri,
                sizeof(uri)
            )) {

            continue;
        }

        if (emitted != 0u) {
            vitte_lsp_comma(writer);
        } else if (writer->pretty) {
            writer->indent++;
            vitte_lsp_newline(writer);
        }

        vitte_lsp_related_item(
            writer,
            options,
            &location->span,
            location->label
        );

        emitted++;
    }

    if (writer->pretty &&
        emitted != 0u) {

        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, ']');
}

/* ========================================================================= */
/* Data: labels                                                              */
/* ========================================================================= */

static void
vitte_lsp_data_labels(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    vitte_lsp_putc(writer, '[');

    if (diagnostic->label_count == 0u) {
        vitte_lsp_putc(writer, ']');
        return;
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_lsp_newline(writer);
    }

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        const vitte_diagnostic_label_t *label;

        label =
            &diagnostic->labels[index];

        if (index != 0u) {
            vitte_lsp_comma(writer);
        }

        vitte_lsp_putc(writer, '{');

        if (writer->pretty) {
            writer->indent++;
            vitte_lsp_newline(writer);
        }

        vitte_lsp_field_string(
            writer,
            "style",
            vitte_diagnostic_label_style_name(
                label->style
            )
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_string(
            writer,
            "message",
            label->message
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_string(
            writer,
            "source",
            label->span.source_name
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_size(
            writer,
            "startByte",
            label->span.start_offset
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_size(
            writer,
            "endByte",
            label->span.end_offset
        );

        if (writer->pretty) {
            writer->indent--;
            vitte_lsp_newline(writer);
        }

        vitte_lsp_putc(writer, '}');
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, ']');
}

/* ========================================================================= */
/* Data: causes                                                              */
/* ========================================================================= */

static void
vitte_lsp_data_causes(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    vitte_lsp_putc(writer, '[');

    if (diagnostic->cause_count == 0u) {
        vitte_lsp_putc(writer, ']');
        return;
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_lsp_newline(writer);
    }

    for (index = 0u;
         index < diagnostic->cause_count;
         index++) {

        const vitte_diagnostic_cause_t *cause;

        cause =
            &diagnostic->causes[index];

        if (index != 0u) {
            vitte_lsp_comma(writer);
        }

        vitte_lsp_putc(writer, '{');

        if (writer->pretty) {
            writer->indent++;
            vitte_lsp_newline(writer);
        }

        vitte_lsp_field_string(
            writer,
            "message",
            cause->message
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_string(
            writer,
            "source",
            cause->has_span ? cause->span.source_name : NULL
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_bool(
            writer,
            "hasSpan",
            cause->has_span
        );

        if (cause->has_span) {
            vitte_lsp_comma(writer);

            vitte_lsp_field_size(
                writer,
                "startByte",
                cause->span.start_offset
            );

            vitte_lsp_comma(writer);

            vitte_lsp_field_size(
                writer,
                "endByte",
                cause->span.end_offset
            );
        }

        if (writer->pretty) {
            writer->indent--;
            vitte_lsp_newline(writer);
        }

        vitte_lsp_putc(writer, '}');
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, ']');
}

/* ========================================================================= */
/* Data: suggestions                                                         */
/* ========================================================================= */

static void
vitte_lsp_data_suggestions(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    vitte_lsp_putc(writer, '[');

    if (diagnostic->suggestion_count == 0u) {
        vitte_lsp_putc(writer, ']');
        return;
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_lsp_newline(writer);
    }

    for (index = 0u;
         index < diagnostic->suggestion_count;
         index++) {

        const vitte_diagnostic_suggestion_t *suggestion;

        suggestion =
            &diagnostic->suggestions[index];

        if (index != 0u) {
            vitte_lsp_comma(writer);
        }

        vitte_lsp_putc(writer, '{');

        if (writer->pretty) {
            writer->indent++;
            vitte_lsp_newline(writer);
        }

        vitte_lsp_field_string(
            writer,
            "message",
            suggestion->message
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_string(
            writer,
            "replacement",
            suggestion->replacement
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_string(
            writer,
            "applicability",
            vitte_diagnostic_applicability_name(
                suggestion->applicability
            )
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_bool(
            writer,
            "hasSpan",
            suggestion->has_span
        );

        if (suggestion->has_span) {
            vitte_lsp_comma(writer);

            vitte_lsp_field_size(
                writer,
                "startByte",
                suggestion->span.start_offset
            );

            vitte_lsp_comma(writer);

            vitte_lsp_field_size(
                writer,
                "endByte",
                suggestion->span.end_offset
            );
        }

        if (writer->pretty) {
            writer->indent--;
            vitte_lsp_newline(writer);
        }

        vitte_lsp_putc(writer, '}');
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, ']');
}

/* ========================================================================= */
/* Contract kind                                                             */
/* ========================================================================= */

static const char *
vitte_lsp_contract_kind_name(
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

/* ========================================================================= */
/* Data: contract                                                            */
/* ========================================================================= */

static void
vitte_lsp_data_contract(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    const vitte_diagnostic_contract_t *contract;

    if (!diagnostic->has_contract) {
        vitte_lsp_write(writer, "null");
        return;
    }

    contract =
        &diagnostic->contract;

    vitte_lsp_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_field_string(
        writer,
        "kind",
        vitte_lsp_contract_kind_name(
            contract->kind
        )
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "expression",
        contract->expression
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "procedure",
        contract->procedure
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "reason",
        contract->reason
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_bool(
        writer,
        "hasContractSpan",
        contract->has_contract_span
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_bool(
        writer,
        "hasCallSpan",
        contract->has_call_span
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_bool(
        writer,
        "hasDeclarationSpan",
        contract->has_declaration_span
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, '}');
}

/* ========================================================================= */
/* Data: optional index                                                      */
/* ========================================================================= */

static void
vitte_lsp_optional_index(
    vitte_lsp_json_writer_t *writer,
    size_t index
)
{
    if (index ==
        VITTE_DIAGNOSTIC_INDEX_NONE) {

        vitte_lsp_write(writer, "null");
    } else {
        vitte_lsp_json_size(writer, index);
    }
}

/* ========================================================================= */
/* Diagnostic.data                                                           */
/* ========================================================================= */

static void
vitte_lsp_data(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_lsp_options_t *options
)
{
    vitte_lsp_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_field_size(
        writer,
        "version",
        VITTE_LSP_DATA_VERSION
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "internalCode",
        diagnostic->code
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "phase",
        vitte_diagnostic_origin_name(
            diagnostic->origin
        )
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "category",
        diagnostic->category
    );

    vitte_lsp_comma(writer);

    vitte_lsp_key(
        writer,
        "fingerprint"
    );

    vitte_lsp_json_u64_hex(
        writer,
        diagnostic->fingerprint
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "package",
        diagnostic->package
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "module",
        diagnostic->module
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "procedure",
        diagnostic->procedure
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "symbol",
        diagnostic->symbol
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "subject",
        diagnostic->symbol
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "context",
        diagnostic->context
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "expectedType",
        diagnostic->expected_type
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "actualType",
        diagnostic->actual_type
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "positionEncoding",
        vitte_diagnostic_lsp_position_encoding_name(
            options->position_encoding
        )
    );

    if (options->include_labels_in_data) {
        vitte_lsp_comma(writer);

        vitte_lsp_key(
            writer,
            "labels"
        );

        vitte_lsp_data_labels(
            writer,
            diagnostic
        );
    }

    if (options->include_causes_in_data) {
        vitte_lsp_comma(writer);

        vitte_lsp_key(
            writer,
            "causes"
        );

        vitte_lsp_data_causes(
            writer,
            diagnostic
        );
    }

    if (options->include_suggestions_in_data) {
        vitte_lsp_comma(writer);

        vitte_lsp_key(
            writer,
            "suggestions"
        );

        vitte_lsp_data_suggestions(
            writer,
            diagnostic
        );
    }

    if (options->include_contract_in_data) {
        vitte_lsp_comma(writer);

        vitte_lsp_key(
            writer,
            "contract"
        );

        vitte_lsp_data_contract(
            writer,
            diagnostic
        );
    }

    if (options->include_cascade_in_data) {
        vitte_lsp_comma(writer);

        vitte_lsp_key(
            writer,
            "cascade"
        );

        vitte_lsp_putc(writer, '{');

        if (writer->pretty) {
            writer->indent++;
            vitte_lsp_newline(writer);
        }

        vitte_lsp_field_bool(
            writer,
            "primary",
            diagnostic->is_primary
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_bool(
            writer,
            "cascade",
            diagnostic->is_cascade
        );

        vitte_lsp_comma(writer);

        vitte_lsp_field_bool(
            writer,
            "suppressed",
            diagnostic->is_suppressed
        );

        vitte_lsp_comma(writer);

        vitte_lsp_key(
            writer,
            "parentIndex"
        );

        vitte_lsp_optional_index(
            writer,
            diagnostic->parent_diagnostic
        );

        vitte_lsp_comma(writer);

        vitte_lsp_key(
            writer,
            "rootIndex"
        );

        vitte_lsp_optional_index(
            writer,
            diagnostic->root_diagnostic
        );

        if (writer->pretty) {
            writer->indent--;
            vitte_lsp_newline(writer);
        }

        vitte_lsp_putc(writer, '}');
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, '}');
}

/* ========================================================================= */
/* Tags                                                                      */
/* ========================================================================= */

/*
 * LSP DiagnosticTag:
 *
 *     1 Unnecessary
 *     2 Deprecated
 *
 * The current core diagnostic model does not yet carry explicit tags.
 * Keep an empty array instead of inferring tags from free-form text.
 */
static void
vitte_lsp_tags(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    (void)diagnostic;

    vitte_lsp_write(
        writer,
        "[]"
    );
}

/* ========================================================================= */
/* Render one Diagnostic                                                     */
/* ========================================================================= */

static bool
vitte_lsp_render_diagnostic_object(
    vitte_lsp_json_writer_t *writer,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_lsp_options_t *options
)
{
    vitte_diagnostic_lsp_range_t range;
    int severity;

    if (writer == NULL ||
        diagnostic == NULL ||
        options == NULL) {

        return false;
    }

    memset(
        &range,
        0,
        sizeof(range)
    );

    if (!vitte_lsp_primary_range(
            diagnostic,
            options,
            &range
        )) {

        /*
         * LSP Diagnostic.range is mandatory.
         *
         * Do not fabricate a position from byte offsets.
         */
        return false;
    }

    severity =
        vitte_diagnostic_lsp_severity(
            diagnostic->severity
        );

    vitte_lsp_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_key(
        writer,
        "range"
    );

    vitte_lsp_render_range(
        writer,
        &range
    );

    vitte_lsp_comma(writer);

    vitte_lsp_key(
        writer,
        "severity"
    );

    vitte_lsp_json_int(
        writer,
        severity
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "code",
        diagnostic->short_code
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "source",
        vitte_lsp_text_present(
            options->source_name
        )
            ? options->source_name
            : VITTE_LSP_SOURCE_NAME
    );

    vitte_lsp_comma(writer);

    vitte_lsp_field_string(
        writer,
        "message",
        diagnostic->message
    );

    vitte_lsp_comma(writer);

    vitte_lsp_key(
        writer,
        "tags"
    );

    vitte_lsp_tags(
        writer,
        diagnostic
    );

    if (options->include_related_information) {
        vitte_lsp_comma(writer);

        vitte_lsp_key(
            writer,
            "relatedInformation"
        );

        vitte_lsp_related_information(
            writer,
            diagnostic,
            options
        );
    }

    if (options->include_data) {
        vitte_lsp_comma(writer);

        vitte_lsp_key(
            writer,
            "data"
        );

        vitte_lsp_data(
            writer,
            diagnostic,
            options
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_lsp_newline(writer);
    }

    vitte_lsp_putc(writer, '}');

    return vitte_lsp_writer_ok(writer);
}

/* ========================================================================= */
/* Public single-diagnostic renderer                                         */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_render_lsp_one(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_lsp_options_t *options
)
{
    vitte_diagnostic_lsp_options_t effective;
    vitte_lsp_json_writer_t writer;

    if (stream == NULL ||
        diagnostic == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_lsp_options_init(
        &effective
    );

    if (options != NULL) {
        effective = *options;
    }

    if (effective.position_converter == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    vitte_lsp_writer_init(
        &writer,
        stream,
        effective.pretty
    );

    if (!vitte_lsp_render_diagnostic_object(
            &writer,
            diagnostic,
            &effective
        )) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (effective.trailing_newline) {
        vitte_lsp_putc(
            &writer,
            '\n'
        );
    }

    return vitte_lsp_writer_ok(&writer)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* Diagnostic array                                                          */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_render_lsp_array(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    const char *source_name,
    const vitte_diagnostic_lsp_options_t *options
)
{
    vitte_diagnostic_lsp_options_t effective;
    vitte_lsp_json_writer_t writer;

    size_t index;
    size_t emitted;

    if (stream == NULL ||
        bag == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_lsp_options_init(
        &effective
    );

    if (options != NULL) {
        effective = *options;
    }

    if (effective.position_converter == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    vitte_lsp_writer_init(
        &writer,
        stream,
        effective.pretty
    );

    vitte_lsp_putc(
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

        if (vitte_lsp_text_present(
                source_name
            )) {

            if (!vitte_lsp_text_present(
                    diagnostic->source_name
                ) ||
                strcmp(
                    diagnostic->source_name,
                    source_name
                ) != 0) {

                continue;
            }
        }

        /*
         * A diagnostic without a convertible LSP range cannot legally be
         * emitted as an LSP Diagnostic.
         */
        {
            vitte_diagnostic_lsp_range_t probe;

            memset(
                &probe,
                0,
                sizeof(probe)
            );

            if (!vitte_lsp_primary_range(
                    diagnostic,
                    &effective,
                    &probe
                )) {

                continue;
            }
        }

        if (emitted != 0u) {
            vitte_lsp_comma(
                &writer
            );
        } else if (writer.pretty) {
            writer.indent++;
            vitte_lsp_newline(
                &writer
            );
        }

        if (!vitte_lsp_render_diagnostic_object(
                &writer,
                diagnostic,
                &effective
            )) {

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        emitted++;
    }

    if (writer.pretty &&
        emitted != 0u) {

        writer.indent--;
        vitte_lsp_newline(
            &writer
        );
    }

    vitte_lsp_putc(
        &writer,
        ']'
    );

    if (effective.trailing_newline) {
        vitte_lsp_putc(
            &writer,
            '\n'
        );
    }

    return vitte_lsp_writer_ok(&writer)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* publishDiagnostics                                                        */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_render_lsp_publish(
    FILE *stream,
    const char *uri,
    const char *source_name,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_lsp_options_t *options
)
{
    vitte_diagnostic_lsp_options_t effective;
    vitte_lsp_json_writer_t writer;

    size_t index;
    size_t emitted;

    if (stream == NULL ||
        uri == NULL ||
        uri[0] == '\0' ||
        bag == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_lsp_options_init(
        &effective
    );

    if (options != NULL) {
        effective = *options;
    }

    if (effective.position_converter == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    vitte_lsp_writer_init(
        &writer,
        stream,
        effective.pretty
    );

    vitte_lsp_putc(
        &writer,
        '{'
    );

    if (writer.pretty) {
        writer.indent++;
        vitte_lsp_newline(
            &writer
        );
    }

    vitte_lsp_field_string(
        &writer,
        "uri",
        uri
    );

    vitte_lsp_comma(
        &writer
    );

    vitte_lsp_key(
        &writer,
        "diagnostics"
    );

    vitte_lsp_putc(
        &writer,
        '['
    );

    emitted = 0u;

    for (index = 0u;
         index < bag->count;
         index++) {

        const vitte_diagnostic_t *diagnostic;
        vitte_diagnostic_lsp_range_t probe;

        diagnostic =
            &bag->storage[index];

        if (diagnostic->is_suppressed &&
            !effective.include_suppressed) {

            continue;
        }

        if (vitte_lsp_text_present(
                source_name
            )) {

            if (!vitte_lsp_text_present(
                    diagnostic->source_name
                ) ||
                strcmp(
                    diagnostic->source_name,
                    source_name
                ) != 0) {

                continue;
            }
        }

        memset(
            &probe,
            0,
            sizeof(probe)
        );

        if (!vitte_lsp_primary_range(
                diagnostic,
                &effective,
                &probe
            )) {

            continue;
        }

        if (emitted != 0u) {
            vitte_lsp_comma(
                &writer
            );
        } else if (writer.pretty) {
            writer.indent++;
            vitte_lsp_newline(
                &writer
            );
        }

        if (!vitte_lsp_render_diagnostic_object(
                &writer,
                diagnostic,
                &effective
            )) {

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        emitted++;
    }

    if (writer.pretty &&
        emitted != 0u) {

        writer.indent--;
        vitte_lsp_newline(
            &writer
        );
    }

    vitte_lsp_putc(
        &writer,
        ']'
    );

    if (writer.pretty) {
        writer.indent--;
        vitte_lsp_newline(
            &writer
        );
    }

    vitte_lsp_putc(
        &writer,
        '}'
    );

    if (effective.trailing_newline) {
        vitte_lsp_putc(
            &writer,
            '\n'
        );
    }

    return vitte_lsp_writer_ok(&writer)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* JSON-RPC notification                                                     */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_render_lsp_notification(
    FILE *stream,
    const char *uri,
    const char *source_name,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_lsp_options_t *options
)
{
    vitte_diagnostic_lsp_options_t effective;
    vitte_lsp_json_writer_t writer;

    size_t index;
    size_t emitted;

    if (stream == NULL ||
        uri == NULL ||
        uri[0] == '\0' ||
        bag == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_lsp_options_init(
        &effective
    );

    if (options != NULL) {
        effective = *options;
    }

    if (effective.position_converter == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    vitte_lsp_writer_init(
        &writer,
        stream,
        effective.pretty
    );

    vitte_lsp_putc(
        &writer,
        '{'
    );

    if (writer.pretty) {
        writer.indent++;
        vitte_lsp_newline(
            &writer
        );
    }

    vitte_lsp_field_string(
        &writer,
        "jsonrpc",
        "2.0"
    );

    vitte_lsp_comma(
        &writer
    );

    vitte_lsp_field_string(
        &writer,
        "method",
        "textDocument/publishDiagnostics"
    );

    vitte_lsp_comma(
        &writer
    );

    vitte_lsp_key(
        &writer,
        "params"
    );

    vitte_lsp_putc(
        &writer,
        '{'
    );

    if (writer.pretty) {
        writer.indent++;
        vitte_lsp_newline(
            &writer
        );
    }

    vitte_lsp_field_string(
        &writer,
        "uri",
        uri
    );

    vitte_lsp_comma(
        &writer
    );

    vitte_lsp_key(
        &writer,
        "diagnostics"
    );

    vitte_lsp_putc(
        &writer,
        '['
    );

    emitted = 0u;

    for (index = 0u;
         index < bag->count;
         index++) {

        const vitte_diagnostic_t *diagnostic;
        vitte_diagnostic_lsp_range_t probe;

        diagnostic =
            &bag->storage[index];

        if (diagnostic->is_suppressed &&
            !effective.include_suppressed) {

            continue;
        }

        if (vitte_lsp_text_present(
                source_name
            )) {

            if (!vitte_lsp_text_present(
                    diagnostic->source_name
                ) ||
                strcmp(
                    diagnostic->source_name,
                    source_name
                ) != 0) {

                continue;
            }
        }

        memset(
            &probe,
            0,
            sizeof(probe)
        );

        if (!vitte_lsp_primary_range(
                diagnostic,
                &effective,
                &probe
            )) {

            continue;
        }

        if (emitted != 0u) {
            vitte_lsp_comma(
                &writer
            );
        } else if (writer.pretty) {
            writer.indent++;
            vitte_lsp_newline(
                &writer
            );
        }

        if (!vitte_lsp_render_diagnostic_object(
                &writer,
                diagnostic,
                &effective
            )) {

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        emitted++;
    }

    if (writer.pretty &&
        emitted != 0u) {

        writer.indent--;
        vitte_lsp_newline(
            &writer
        );
    }

    vitte_lsp_putc(
        &writer,
        ']'
    );

    if (writer.pretty) {
        writer.indent--;
        vitte_lsp_newline(
            &writer
        );
    }

    vitte_lsp_putc(
        &writer,
        '}'
    );

    if (writer.pretty) {
        writer.indent--;
        vitte_lsp_newline(
            &writer
        );
    }

    vitte_lsp_putc(
        &writer,
        '}'
    );

    if (effective.trailing_newline) {
        vitte_lsp_putc(
            &writer,
            '\n'
        );
    }

    return vitte_lsp_writer_ok(&writer)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* Compatibility adapter                                                     */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_write_lsp(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_lsp_options_t *options
)
{
    return vitte_diagnostic_render_lsp_one(
        stream,
        diagnostic,
        options
    );
}

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

unsigned
vitte_diagnostic_lsp_data_version(void)
{
    return VITTE_LSP_DATA_VERSION;
}

const char *
vitte_diagnostic_lsp_source_name(void)
{
    return VITTE_LSP_SOURCE_NAME;
}
