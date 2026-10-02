#include "render_sarif.h"

#include "diagnostic.h"
#include "suggestion.h"
#include "label.h"
#include "registry.h"

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define VITTE_SARIF_VERSION "2.1.0"
#define VITTE_SARIF_SCHEMA \
    "https://json.schemastore.org/sarif-2.1.0.json"
#define VITTE_SARIF_TOOL_NAME "Vitte"
#define VITTE_SARIF_INFORMATION_URI "https://vitte-lang.org"
#define VITTE_SARIF_PROPERTIES_VERSION 1u

/* ========================================================================= */
/* Writer                                                                    */
/* ========================================================================= */

typedef struct vitte_sarif_writer {
    FILE *stream;
    bool pretty;
    unsigned indent;
    bool failed;
} vitte_sarif_writer_t;

static unsigned
vitte_sarif_size_to_unsigned(size_t value)
{
    return value > (size_t)UINT_MAX
        ? UINT_MAX
        : (unsigned)value;
}

static void
vitte_sarif_writer_init(
    vitte_sarif_writer_t *writer,
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
vitte_sarif_writer_ok(
    const vitte_sarif_writer_t *writer
)
{
    return writer != NULL &&
           writer->stream != NULL &&
           !writer->failed &&
           !ferror(writer->stream);
}

static void
vitte_sarif_putc(
    vitte_sarif_writer_t *writer,
    int character
)
{
    if (!vitte_sarif_writer_ok(writer)) {
        return;
    }

    if (fputc(character, writer->stream) == EOF) {
        writer->failed = true;
    }
}

static void
vitte_sarif_write(
    vitte_sarif_writer_t *writer,
    const char *text
)
{
    if (!vitte_sarif_writer_ok(writer) ||
        text == NULL) {
        return;
    }

    if (fputs(text, writer->stream) < 0) {
        writer->failed = true;
    }
}

static void
vitte_sarif_indent(
    vitte_sarif_writer_t *writer
)
{
    unsigned index;

    if (!vitte_sarif_writer_ok(writer) ||
        !writer->pretty) {
        return;
    }

    for (index = 0u;
         index < writer->indent;
         index++) {
        vitte_sarif_write(writer, "  ");
    }
}

static void
vitte_sarif_newline(
    vitte_sarif_writer_t *writer
)
{
    if (!vitte_sarif_writer_ok(writer) ||
        !writer->pretty) {
        return;
    }

    vitte_sarif_putc(writer, '\n');
    vitte_sarif_indent(writer);
}

static void
vitte_sarif_comma(
    vitte_sarif_writer_t *writer
)
{
    vitte_sarif_putc(writer, ',');
    vitte_sarif_newline(writer);
}

/* ========================================================================= */
/* JSON                                                                      */
/* ========================================================================= */

static bool
vitte_sarif_text_present(
    const char *text
)
{
    return text != NULL &&
           text[0] != '\0';
}

static void
vitte_sarif_string(
    vitte_sarif_writer_t *writer,
    const char *text
)
{
    const unsigned char *cursor;

    if (!vitte_sarif_writer_ok(writer)) {
        return;
    }

    if (text == NULL) {
        vitte_sarif_write(writer, "null");
        return;
    }

    vitte_sarif_putc(writer, '"');

    cursor = (const unsigned char *)text;

    while (*cursor != '\0' &&
           vitte_sarif_writer_ok(writer)) {

        unsigned char character = *cursor++;

        switch (character) {
            case '"':
                vitte_sarif_write(writer, "\\\"");
                break;

            case '\\':
                vitte_sarif_write(writer, "\\\\");
                break;

            case '\b':
                vitte_sarif_write(writer, "\\b");
                break;

            case '\f':
                vitte_sarif_write(writer, "\\f");
                break;

            case '\n':
                vitte_sarif_write(writer, "\\n");
                break;

            case '\r':
                vitte_sarif_write(writer, "\\r");
                break;

            case '\t':
                vitte_sarif_write(writer, "\\t");
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
                    vitte_sarif_putc(
                        writer,
                        (int)character
                    );
                }
                break;
        }
    }

    vitte_sarif_putc(writer, '"');
}

static void
vitte_sarif_bool(
    vitte_sarif_writer_t *writer,
    bool value
)
{
    vitte_sarif_write(
        writer,
        value ? "true" : "false"
    );
}

static void
vitte_sarif_size(
    vitte_sarif_writer_t *writer,
    size_t value
)
{
    if (!vitte_sarif_writer_ok(writer)) {
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
vitte_sarif_u64_hex(
    vitte_sarif_writer_t *writer,
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

    vitte_sarif_string(
        writer,
        buffer
    );
}

static void
vitte_sarif_key(
    vitte_sarif_writer_t *writer,
    const char *key
)
{
    vitte_sarif_string(writer, key);
    vitte_sarif_putc(writer, ':');

    if (writer->pretty) {
        vitte_sarif_putc(writer, ' ');
    }
}

static void
vitte_sarif_field_string(
    vitte_sarif_writer_t *writer,
    const char *key,
    const char *value
)
{
    vitte_sarif_key(writer, key);
    vitte_sarif_string(writer, value);
}

static void
vitte_sarif_field_bool(
    vitte_sarif_writer_t *writer,
    const char *key,
    bool value
)
{
    vitte_sarif_key(writer, key);
    vitte_sarif_bool(writer, value);
}

static void
vitte_sarif_field_size(
    vitte_sarif_writer_t *writer,
    const char *key,
    size_t value
)
{
    vitte_sarif_key(writer, key);
    vitte_sarif_size(writer, value);
}

/* ========================================================================= */
/* Options                                                                   */
/* ========================================================================= */

void
vitte_diagnostic_sarif_options_init(
    vitte_diagnostic_sarif_options_t *options
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

    options->pretty = true;
    options->trailing_newline = true;
    options->include_suppressed = false;
    options->include_rules = true;
    options->include_related_locations = true;
    options->include_fixes = true;
    options->include_properties = true;
    options->include_causes = true;
    options->include_contract = true;
    options->include_cascade = true;

    options->tool_name = VITTE_SARIF_TOOL_NAME;
    options->tool_version = NULL;
    options->information_uri = VITTE_SARIF_INFORMATION_URI;

    options->uri_converter = NULL;
    options->uri_user_data = NULL;
}

/* ========================================================================= */
/* Severity                                                                  */
/* ========================================================================= */

const char *
vitte_diagnostic_sarif_level(
    vitte_diagnostic_severity_t severity
)
{
    switch (severity) {
        case VITTE_DIAGNOSTIC_FATAL:
        case VITTE_DIAGNOSTIC_ERROR:
            return "error";

        case VITTE_DIAGNOSTIC_WARNING:
            return "warning";

        case VITTE_DIAGNOSTIC_NOTE:
            return "note";

        case VITTE_DIAGNOSTIC_HELP:
            return "note";

        default:
            return "none";
    }
}

/* ========================================================================= */
/* URI                                                                       */
/* ========================================================================= */

static bool
vitte_sarif_uri(
    const vitte_diagnostic_sarif_options_t *options,
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

    if (!vitte_sarif_text_present(source_name)) {
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
/* Region                                                                    */
/* ========================================================================= */

static void
vitte_sarif_region_from_values(
    vitte_sarif_writer_t *writer,
    size_t start_offset,
    size_t end_offset,
    size_t start_line,
    size_t start_column,
    size_t end_line,
    size_t end_column
)
{
    bool has_line;

    /*
     * Vitte's line/column representation is assumed to be zero-based
     * internally. SARIF line/column coordinates are one-based.
     *
     * byteOffset/byteLength preserve the exact compiler source span even when
     * line/column information is unavailable.
     */
    has_line =
        start_line != SIZE_MAX &&
        start_column != SIZE_MAX;

    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_size(
        writer,
        "byteOffset",
        start_offset
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_size(
        writer,
        "byteLength",
        end_offset >= start_offset
            ? end_offset - start_offset
            : 0u
    );

    if (has_line) {
        vitte_sarif_comma(writer);

        vitte_sarif_field_size(
            writer,
            "startLine",
            start_line + 1u
        );

        vitte_sarif_comma(writer);

        vitte_sarif_field_size(
            writer,
            "startColumn",
            start_column + 1u
        );

        if (end_line != SIZE_MAX &&
            end_column != SIZE_MAX) {

            vitte_sarif_comma(writer);

            vitte_sarif_field_size(
                writer,
                "endLine",
                end_line + 1u
            );

            vitte_sarif_comma(writer);

            /*
             * Vitte end offsets/columns are expected to be exclusive.
             * SARIF endColumn is also exclusive.
             */
            vitte_sarif_field_size(
                writer,
                "endColumn",
                end_column + 1u
            );
        }
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

static void
vitte_sarif_region_from_span(
    vitte_sarif_writer_t *writer,
    const vitte_ast_span_t *span
)
{
    if (span == NULL ||
        !vitte_ast_span_is_valid(*span)) {

        vitte_sarif_write(writer, "{}");
        return;
    }

    vitte_sarif_region_from_values(
        writer,
        span->start_offset,
        span->end_offset,
        span->start_line,
        span->start_column,
        span->end_line,
        span->end_column
    );
}

/* ========================================================================= */
/* Artifact location                                                         */
/* ========================================================================= */

static void
vitte_sarif_artifact_location(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_sarif_options_t *options,
    vitte_source_id_t source_id,
    const char *source_name
)
{
    char uri[VITTE_DIAGNOSTIC_SARIF_URI_CAPACITY];

    uri[0] = '\0';

    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    if (vitte_sarif_uri(
            options,
            source_id,
            source_name,
            uri,
            sizeof(uri)
        )) {

        vitte_sarif_field_string(
            writer,
            "uri",
            uri
        );
    } else {
        vitte_sarif_field_string(
            writer,
            "uri",
            source_name
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

/* ========================================================================= */
/* Physical location                                                         */
/* ========================================================================= */

static void
vitte_sarif_physical_location_span(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_sarif_options_t *options,
    const vitte_ast_span_t *span
)
{
    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_key(
        writer,
        "artifactLocation"
    );

    vitte_sarif_artifact_location(
        writer,
        options,
        span->source_id,
        span->source_name
    );

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "region"
    );

    vitte_sarif_region_from_span(
        writer,
        span
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

/* ========================================================================= */
/* Message                                                                   */
/* ========================================================================= */

static void
vitte_sarif_message(
    vitte_sarif_writer_t *writer,
    const char *text
)
{
    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_string(
        writer,
        "text",
        vitte_sarif_text_present(text)
            ? text
            : ""
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

/* ========================================================================= */
/* Primary span                                                              */
/* ========================================================================= */

static const vitte_diagnostic_label_t *
vitte_sarif_primary_label(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return NULL;
    }

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        if (diagnostic->labels[index].style ==
            VITTE_DIAGNOSTIC_LABEL_PRIMARY) {

            return &diagnostic->labels[index];
        }
    }

    return NULL;
}

/* ========================================================================= */
/* Primary location                                                          */
/* ========================================================================= */

static void
vitte_sarif_primary_location(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_sarif_options_t *options
)
{
    const vitte_diagnostic_label_t *primary;

    primary =
        vitte_sarif_primary_label(
            diagnostic
        );

    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_key(
        writer,
        "physicalLocation"
    );

    if (primary != NULL) {
        vitte_sarif_physical_location_span(
            writer,
            options,
            &primary->span
        );
    } else {
        vitte_ast_span_t span;

        memset(
            &span,
            0,
            sizeof(span)
        );

        span.source_id =
            diagnostic->source_id;

        span.source_name =
            diagnostic->source_name;

        span.start_offset =
            diagnostic->start_offset;

        span.end_offset =
            diagnostic->end_offset;

        span.start_line =
            vitte_sarif_size_to_unsigned(
                diagnostic->start_line);

        span.start_column =
            vitte_sarif_size_to_unsigned(
                diagnostic->start_column);

        span.end_line =
            vitte_sarif_size_to_unsigned(
                diagnostic->end_line);

        span.end_column =
            vitte_sarif_size_to_unsigned(
                diagnostic->end_column);

        vitte_sarif_physical_location_span(
            writer,
            options,
            &span
        );
    }

    if (primary != NULL &&
        vitte_sarif_text_present(
            primary->message
        )) {

        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "message"
        );

        vitte_sarif_message(
            writer,
            primary->message
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

/* ========================================================================= */
/* Locations                                                                 */
/* ========================================================================= */

static void
vitte_sarif_locations(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_sarif_options_t *options
)
{
    vitte_sarif_putc(writer, '[');

    if (diagnostic == NULL ||
        !diagnostic->has_span) {

        const vitte_diagnostic_label_t *primary;

        primary =
            vitte_sarif_primary_label(
                diagnostic
            );

        if (primary == NULL) {
            vitte_sarif_putc(writer, ']');
            return;
        }
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_primary_location(
        writer,
        diagnostic,
        options
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, ']');
}

/* ========================================================================= */
/* Related locations                                                         */
/* ========================================================================= */

static void
vitte_sarif_related_location_span(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_sarif_options_t *options,
    size_t id,
    const vitte_ast_span_t *span,
    const char *message
)
{
    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_size(
        writer,
        "id",
        id
    );

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "physicalLocation"
    );

    vitte_sarif_physical_location_span(
        writer,
        options,
        span
    );

    if (vitte_sarif_text_present(message)) {
        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "message"
        );

        vitte_sarif_message(
            writer,
            message
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

static void
vitte_sarif_related_locations(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_sarif_options_t *options
)
{
    size_t index;
    size_t emitted;
    size_t id;

    emitted = 0u;
    id = 1u;

    vitte_sarif_putc(writer, '[');

    /*
     * Secondary labels.
     */
    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        const vitte_diagnostic_label_t *label;

        label =
            &diagnostic->labels[index];

        if (label->style ==
            VITTE_DIAGNOSTIC_LABEL_PRIMARY) {
            continue;
        }

        if (!vitte_ast_span_is_valid(label->span)) {
            continue;
        }

        if (emitted != 0u) {
            vitte_sarif_comma(writer);
        } else if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_related_location_span(
            writer,
            options,
            id++,
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

        location =
            &diagnostic->related[index];

        if (!vitte_ast_span_is_valid(location->span)) {
            continue;
        }

        if (emitted != 0u) {
            vitte_sarif_comma(writer);
        } else if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_related_location_span(
            writer,
            options,
            id++,
            &location->span,
            location->label
        );

        emitted++;
    }

    /*
     * Causes with source locations.
     */
    if (options->include_causes) {
        for (index = 0u;
             index < diagnostic->cause_count;
             index++) {

            const vitte_diagnostic_cause_t *cause;

            cause =
                &diagnostic->causes[index];

            if (!cause->has_span ||
                !vitte_ast_span_is_valid(cause->span)) {
                continue;
            }

            if (emitted != 0u) {
                vitte_sarif_comma(writer);
            } else if (writer->pretty) {
                writer->indent++;
                vitte_sarif_newline(writer);
            }

            vitte_sarif_related_location_span(
                writer,
                options,
                id++,
                &cause->span,
                cause->message
            );

            emitted++;
        }
    }

    if (writer->pretty &&
        emitted != 0u) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, ']');
}

/* ========================================================================= */
/* Applicability                                                             */
/* ========================================================================= */

static const char *
vitte_sarif_applicability(
    vitte_diagnostic_applicability_t applicability
)
{
    return vitte_diagnostic_applicability_name(
        applicability
    );
}

/* ========================================================================= */
/* Fixes                                                                     */
/* ========================================================================= */

static void
vitte_sarif_fix(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_suggestion_t *suggestion,
    const vitte_diagnostic_sarif_options_t *options
)
{
    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_key(
        writer,
        "description"
    );

    vitte_sarif_message(
        writer,
        suggestion->message
    );

    if (suggestion->has_span &&
        vitte_ast_span_is_valid(
            suggestion->span
        )) {

        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "artifactChanges"
        );

        vitte_sarif_putc(writer, '[');

        if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_putc(writer, '{');

        if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_key(
            writer,
            "artifactLocation"
        );

        vitte_sarif_artifact_location(
            writer,
            options,
            suggestion->span.source_id,
            suggestion->span.source_name
        );

        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "replacements"
        );

        vitte_sarif_putc(writer, '[');

        if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_putc(writer, '{');

        if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_key(
            writer,
            "deletedRegion"
        );

        vitte_sarif_region_from_span(
            writer,
            &suggestion->span
        );

        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "insertedContent"
        );

        vitte_sarif_putc(writer, '{');

        if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_field_string(
            writer,
            "text",
            suggestion->replacement
        );

        if (writer->pretty) {
            writer->indent--;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_putc(writer, '}');

        if (writer->pretty) {
            writer->indent--;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_putc(writer, '}');

        if (writer->pretty) {
            writer->indent--;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_putc(writer, ']');

        if (writer->pretty) {
            writer->indent--;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_putc(writer, '}');

        if (writer->pretty) {
            writer->indent--;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_putc(writer, ']');
    }

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "properties"
    );

    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_string(
        writer,
        "applicability",
        vitte_sarif_applicability(
            suggestion->applicability
        )
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

static void
vitte_sarif_fixes(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_sarif_options_t *options
)
{
    size_t index;
    size_t emitted;

    emitted = 0u;

    vitte_sarif_putc(writer, '[');

    for (index = 0u;
         index < diagnostic->suggestion_count;
         index++) {

        const vitte_diagnostic_suggestion_t *suggestion;

        suggestion =
            &diagnostic->suggestions[index];

        if (!suggestion->has_span) {
            continue;
        }

        if (emitted != 0u) {
            vitte_sarif_comma(writer);
        } else if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_fix(
            writer,
            suggestion,
            options
        );

        emitted++;
    }

    if (writer->pretty &&
        emitted != 0u) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, ']');
}

/* ========================================================================= */
/* Properties: causes                                                        */
/* ========================================================================= */

static void
vitte_sarif_property_causes(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    vitte_sarif_putc(writer, '[');

    if (diagnostic->cause_count == 0u) {
        vitte_sarif_putc(writer, ']');
        return;
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    for (index = 0u;
         index < diagnostic->cause_count;
         index++) {

        const vitte_diagnostic_cause_t *cause;

        cause =
            &diagnostic->causes[index];

        if (index != 0u) {
            vitte_sarif_comma(writer);
        }

        vitte_sarif_putc(writer, '{');

        if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_field_string(
            writer,
            "message",
            cause->message
        );

        vitte_sarif_comma(writer);

        vitte_sarif_field_string(
            writer,
            "source",
            cause->has_span ? cause->span.source_name : NULL
        );

        vitte_sarif_comma(writer);

        vitte_sarif_field_bool(
            writer,
            "hasSpan",
            cause->has_span
        );

        if (cause->has_span) {
            vitte_sarif_comma(writer);

            vitte_sarif_field_size(
                writer,
                "startByte",
                cause->span.start_offset
            );

            vitte_sarif_comma(writer);

            vitte_sarif_field_size(
                writer,
                "endByte",
                cause->span.end_offset
            );
        }

        if (writer->pretty) {
            writer->indent--;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_putc(writer, '}');
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, ']');
}

/* ========================================================================= */
/* Properties: contract                                                      */
/* ========================================================================= */

static const char *
vitte_sarif_contract_kind(
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
vitte_sarif_property_contract(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    const vitte_diagnostic_contract_t *contract;

    if (!diagnostic->has_contract) {
        vitte_sarif_write(writer, "null");
        return;
    }

    contract =
        &diagnostic->contract;

    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_string(
        writer,
        "kind",
        vitte_sarif_contract_kind(
            contract->kind
        )
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_string(
        writer,
        "expression",
        contract->expression
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_string(
        writer,
        "procedure",
        contract->procedure
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_string(
        writer,
        "reason",
        contract->reason
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_bool(
        writer,
        "hasContractSpan",
        contract->has_contract_span
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_bool(
        writer,
        "hasCallSpan",
        contract->has_call_span
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_bool(
        writer,
        "hasDeclarationSpan",
        contract->has_declaration_span
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

/* ========================================================================= */
/* Properties                                                                */
/* ========================================================================= */

static void
vitte_sarif_optional_index(
    vitte_sarif_writer_t *writer,
    size_t value
)
{
    if (value ==
        VITTE_DIAGNOSTIC_INDEX_NONE) {

        vitte_sarif_write(writer, "null");
    } else {
        vitte_sarif_size(writer, value);
    }
}

static void
vitte_sarif_properties(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_sarif_options_t *options
)
{
    bool first;

    first = true;

    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

#define VITTE_SARIF_NEXT_PROPERTY()              \
    do {                                          \
        if (!first) {                             \
            vitte_sarif_comma(writer);            \
        }                                         \
        first = false;                            \
    } while (0)

    VITTE_SARIF_NEXT_PROPERTY();

    vitte_sarif_field_size(
        writer,
        "vittePropertiesVersion",
        VITTE_SARIF_PROPERTIES_VERSION
    );

    VITTE_SARIF_NEXT_PROPERTY();

    vitte_sarif_field_string(
        writer,
        "internalCode",
        diagnostic->code
    );

    VITTE_SARIF_NEXT_PROPERTY();

    vitte_sarif_field_string(
        writer,
        "phase",
        vitte_diagnostic_origin_name(
            diagnostic->origin
        )
    );

    VITTE_SARIF_NEXT_PROPERTY();

    vitte_sarif_field_string(
        writer,
        "category",
        diagnostic->category
    );

    VITTE_SARIF_NEXT_PROPERTY();

    vitte_sarif_key(
        writer,
        "fingerprint"
    );

    vitte_sarif_u64_hex(
        writer,
        diagnostic->fingerprint
    );

    if (vitte_sarif_text_present(
            diagnostic->package
        )) {

        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_field_string(
            writer,
            "package",
            diagnostic->package
        );
    }

    if (vitte_sarif_text_present(
            diagnostic->module
        )) {

        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_field_string(
            writer,
            "module",
            diagnostic->module
        );
    }

    if (vitte_sarif_text_present(
            diagnostic->procedure
        )) {

        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_field_string(
            writer,
            "procedure",
            diagnostic->procedure
        );
    }

    if (vitte_sarif_text_present(
            diagnostic->symbol
        )) {

        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_field_string(
            writer,
            "symbol",
            diagnostic->symbol
        );
    }

    if (vitte_sarif_text_present(
            diagnostic->symbol
        )) {

        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_field_string(
            writer,
            "subject",
            diagnostic->symbol
        );
    }

    if (vitte_sarif_text_present(
            diagnostic->context
        )) {

        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_field_string(
            writer,
            "context",
            diagnostic->context
        );
    }

    if (vitte_sarif_text_present(
            diagnostic->expected_type
        )) {

        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_field_string(
            writer,
            "expectedType",
            diagnostic->expected_type
        );
    }

    if (vitte_sarif_text_present(
            diagnostic->actual_type
        )) {

        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_field_string(
            writer,
            "actualType",
            diagnostic->actual_type
        );
    }

    if (options->include_causes) {
        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_key(
            writer,
            "causes"
        );

        vitte_sarif_property_causes(
            writer,
            diagnostic
        );
    }

    if (options->include_contract) {
        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_key(
            writer,
            "contract"
        );

        vitte_sarif_property_contract(
            writer,
            diagnostic
        );
    }

    if (options->include_cascade) {
        VITTE_SARIF_NEXT_PROPERTY();

        vitte_sarif_key(
            writer,
            "cascade"
        );

        vitte_sarif_putc(writer, '{');

        if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_field_bool(
            writer,
            "primary",
            diagnostic->is_primary
        );

        vitte_sarif_comma(writer);

        vitte_sarif_field_bool(
            writer,
            "cascade",
            diagnostic->is_cascade
        );

        vitte_sarif_comma(writer);

        vitte_sarif_field_bool(
            writer,
            "suppressed",
            diagnostic->is_suppressed
        );

        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "parentIndex"
        );

        vitte_sarif_optional_index(
            writer,
            diagnostic->parent_diagnostic
        );

        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "rootIndex"
        );

        vitte_sarif_optional_index(
            writer,
            diagnostic->root_diagnostic
        );

        if (writer->pretty) {
            writer->indent--;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_putc(writer, '}');
    }

#undef VITTE_SARIF_NEXT_PROPERTY

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

/* ========================================================================= */
/* Fingerprints                                                              */
/* ========================================================================= */

static void
vitte_sarif_partial_fingerprints(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_t *diagnostic
)
{
    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_key(
        writer,
        "vitteDiagnosticFingerprint/v1"
    );

    vitte_sarif_u64_hex(
        writer,
        diagnostic->fingerprint
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

/* ========================================================================= */
/* Rule index                                                                */
/* ========================================================================= */

static size_t
vitte_sarif_rule_index(
    const char *code
)
{
    size_t index;
    size_t count;

    count =
        vitte_diagnostic_registry_count();

    for (index = 0u;
         index < count;
         index++) {

        const vitte_diagnostic_registry_entry_t *entry;

        entry =
            vitte_diagnostic_registry_at(
                index
            );

        if (entry == NULL ||
            entry->public_code == NULL ||
            code == NULL) {
            continue;
        }

        if (strcmp(
                entry->public_code,
                code
            ) == 0) {
            return index;
        }
    }

    return VITTE_DIAGNOSTIC_INDEX_NONE;
}

/* ========================================================================= */
/* Result                                                                    */
/* ========================================================================= */

static void
vitte_sarif_result(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_sarif_options_t *options
)
{
    size_t rule_index;

    rule_index =
        vitte_sarif_rule_index(
            diagnostic->short_code
        );

    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_string(
        writer,
        "ruleId",
        diagnostic->short_code
    );

    if (rule_index !=
        VITTE_DIAGNOSTIC_INDEX_NONE) {

        vitte_sarif_comma(writer);

        vitte_sarif_field_size(
            writer,
            "ruleIndex",
            rule_index
        );
    }

    vitte_sarif_comma(writer);

    vitte_sarif_field_string(
        writer,
        "level",
        vitte_diagnostic_sarif_level(
            diagnostic->severity
        )
    );

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "message"
    );

    vitte_sarif_message(
        writer,
        diagnostic->message
    );

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "locations"
    );

    vitte_sarif_locations(
        writer,
        diagnostic,
        options
    );

    if (options->include_related_locations) {
        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "relatedLocations"
        );

        vitte_sarif_related_locations(
            writer,
            diagnostic,
            options
        );
    }

    if (options->include_fixes &&
        diagnostic->suggestion_count != 0u) {

        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "fixes"
        );

        vitte_sarif_fixes(
            writer,
            diagnostic,
            options
        );
    }

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "partialFingerprints"
    );

    vitte_sarif_partial_fingerprints(
        writer,
        diagnostic
    );

    if (options->include_properties) {
        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "properties"
        );

        vitte_sarif_properties(
            writer,
            diagnostic,
            options
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

/* ========================================================================= */
/* Rules                                                                     */
/* ========================================================================= */

static void
vitte_sarif_rule(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_registry_entry_t *entry
)
{
    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_string(
        writer,
        "id",
        entry->public_code
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_string(
        writer,
        "name",
        entry->internal_code
    );

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "shortDescription"
    );

    vitte_sarif_message(
        writer,
        entry->title
    );

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "fullDescription"
    );

    vitte_sarif_message(
        writer,
        entry->explanation
    );

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "defaultConfiguration"
    );

    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_string(
        writer,
        "level",
        vitte_diagnostic_sarif_level(
            entry->default_severity
        )
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "properties"
    );

    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_string(
        writer,
        "category",
        entry->category
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_string(
        writer,
        "phase",
        vitte_diagnostic_origin_name(
            entry->origin
        )
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_bool(
        writer,
        "deprecated",
        entry->deprecated
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

static void
vitte_sarif_rules(
    vitte_sarif_writer_t *writer
)
{
    size_t index;
    size_t count;

    count =
        vitte_diagnostic_registry_count();

    vitte_sarif_putc(writer, '[');

    if (count == 0u) {
        vitte_sarif_putc(writer, ']');
        return;
    }

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

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

        if (index != 0u) {
            vitte_sarif_comma(writer);
        }

        vitte_sarif_rule(
            writer,
            entry
        );
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, ']');
}

/* ========================================================================= */
/* Tool                                                                      */
/* ========================================================================= */

static void
vitte_sarif_driver(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_sarif_options_t *options
)
{
    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_string(
        writer,
        "name",
        vitte_sarif_text_present(
            options->tool_name
        )
            ? options->tool_name
            : VITTE_SARIF_TOOL_NAME
    );

    if (vitte_sarif_text_present(
            options->tool_version
        )) {

        vitte_sarif_comma(writer);

        vitte_sarif_field_string(
            writer,
            "version",
            options->tool_version
        );
    }

    if (vitte_sarif_text_present(
            options->information_uri
        )) {

        vitte_sarif_comma(writer);

        vitte_sarif_field_string(
            writer,
            "informationUri",
            options->information_uri
        );
    }

    if (options->include_rules) {
        vitte_sarif_comma(writer);

        vitte_sarif_key(
            writer,
            "rules"
        );

        vitte_sarif_rules(writer);
    }

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

static void
vitte_sarif_tool(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_sarif_options_t *options
)
{
    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_key(
        writer,
        "driver"
    );

    vitte_sarif_driver(
        writer,
        options
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

/* ========================================================================= */
/* Results                                                                   */
/* ========================================================================= */

static void
vitte_sarif_results(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_sarif_options_t *options
)
{
    size_t index;
    size_t emitted;

    emitted = 0u;

    vitte_sarif_putc(writer, '[');

    for (index = 0u;
         index < bag->count;
         index++) {

        const vitte_diagnostic_t *diagnostic;

        diagnostic =
            &bag->storage[index];

        if (diagnostic->is_suppressed &&
            !options->include_suppressed) {
            continue;
        }

        if (emitted != 0u) {
            vitte_sarif_comma(writer);
        } else if (writer->pretty) {
            writer->indent++;
            vitte_sarif_newline(writer);
        }

        vitte_sarif_result(
            writer,
            diagnostic,
            options
        );

        emitted++;
    }

    if (writer->pretty &&
        emitted != 0u) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, ']');
}

/* ========================================================================= */
/* Run                                                                       */
/* ========================================================================= */

static void
vitte_sarif_run(
    vitte_sarif_writer_t *writer,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_sarif_options_t *options
)
{
    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_key(
        writer,
        "tool"
    );

    vitte_sarif_tool(
        writer,
        options
    );

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "results"
    );

    vitte_sarif_results(
        writer,
        bag,
        options
    );

    vitte_sarif_comma(writer);

    vitte_sarif_key(
        writer,
        "properties"
    );

    vitte_sarif_putc(writer, '{');

    if (writer->pretty) {
        writer->indent++;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_field_size(
        writer,
        "vittePropertiesVersion",
        VITTE_SARIF_PROPERTIES_VERSION
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_size(
        writer,
        "storedDiagnostics",
        bag->count
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_size(
        writer,
        "errors",
        bag->counts.error_count
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_size(
        writer,
        "fatals",
        bag->counts.fatal_count
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_size(
        writer,
        "warnings",
        bag->counts.warning_count
    );

    vitte_sarif_comma(writer);

    vitte_sarif_field_size(
        writer,
        "suppressed",
        bag->counts.suppressed_count
    );

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');

    if (writer->pretty) {
        writer->indent--;
        vitte_sarif_newline(writer);
    }

    vitte_sarif_putc(writer, '}');
}

/* ========================================================================= */
/* Public rendering                                                          */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_render_sarif(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_sarif_options_t *options
)
{
    vitte_diagnostic_sarif_options_t effective;
    vitte_sarif_writer_t writer;

    if (stream == NULL ||
        bag == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_sarif_options_init(
        &effective
    );

    if (options != NULL) {
        effective = *options;
    }

    vitte_sarif_writer_init(
        &writer,
        stream,
        effective.pretty
    );

    vitte_sarif_putc(
        &writer,
        '{'
    );

    if (writer.pretty) {
        writer.indent++;
        vitte_sarif_newline(
            &writer
        );
    }

    vitte_sarif_field_string(
        &writer,
        "$schema",
        VITTE_SARIF_SCHEMA
    );

    vitte_sarif_comma(
        &writer
    );

    vitte_sarif_field_string(
        &writer,
        "version",
        VITTE_SARIF_VERSION
    );

    vitte_sarif_comma(
        &writer
    );

    vitte_sarif_key(
        &writer,
        "runs"
    );

    vitte_sarif_putc(
        &writer,
        '['
    );

    if (writer.pretty) {
        writer.indent++;
        vitte_sarif_newline(
            &writer
        );
    }

    vitte_sarif_run(
        &writer,
        bag,
        &effective
    );

    if (writer.pretty) {
        writer.indent--;
        vitte_sarif_newline(
            &writer
        );
    }

    vitte_sarif_putc(
        &writer,
        ']'
    );

    if (writer.pretty) {
        writer.indent--;
        vitte_sarif_newline(
            &writer
        );
    }

    vitte_sarif_putc(
        &writer,
        '}'
    );

    if (effective.trailing_newline) {
        vitte_sarif_putc(
            &writer,
            '\n'
        );
    }

    return vitte_sarif_writer_ok(&writer)
        ? VITTE_STATUS_OK
        : VITTE_STATUS_ERROR_INVALID_STATE;
}

/* ========================================================================= */
/* One diagnostic                                                           */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_render_sarif_one(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_sarif_options_t *options
)
{
    vitte_diagnostic_bag_t bag;

    if (stream == NULL ||
        diagnostic == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(
        &bag,
        0,
        sizeof(bag)
    );

    /*
     * Borrow one diagnostic without copying self-referential diagnostic
     * storage. render_sarif() only reads the bag.
     */
    bag.initialized = true;
    bag.storage =
        (vitte_diagnostic_t *)(uintptr_t)diagnostic;
    bag.capacity = 1u;
    bag.count = 1u;

    switch (diagnostic->severity) {
        case VITTE_DIAGNOSTIC_NOTE:
            bag.counts.note_count = 1u;
            break;

        case VITTE_DIAGNOSTIC_HELP:
            bag.counts.help_count = 1u;
            break;

        case VITTE_DIAGNOSTIC_WARNING:
            bag.counts.warning_count = 1u;
            break;

        case VITTE_DIAGNOSTIC_ERROR:
            bag.counts.error_count = 1u;
            break;

        case VITTE_DIAGNOSTIC_FATAL:
            bag.counts.fatal_count = 1u;
            break;

        default:
            break;
    }

    if (diagnostic->is_suppressed) {
        bag.counts.suppressed_count = 1u;
    }

    return vitte_diagnostic_render_sarif(
        stream,
        &bag,
        options
    );
}

/* ========================================================================= */
/* Compatibility adapter                                                     */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_write_sarif(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag
)
{
    vitte_diagnostic_sarif_options_t options;

    vitte_diagnostic_sarif_options_init(
        &options
    );

    return vitte_diagnostic_render_sarif(
        stream,
        bag,
        &options
    );
}

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

const char *
vitte_diagnostic_sarif_version(void)
{
    return VITTE_SARIF_VERSION;
}

const char *
vitte_diagnostic_sarif_schema(void)
{
    return VITTE_SARIF_SCHEMA;
}

const char *
vitte_diagnostic_sarif_tool_name(void)
{
    return VITTE_SARIF_TOOL_NAME;
}
