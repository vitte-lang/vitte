#include "cli_bridge.h"

#include <stdlib.h>
#include <string.h>

typedef struct vitte_cli_diagnostic_transport_header {
    size_t count;
    char **owned_text;
    vitte_cli_diagnostic_suggestion_t *owned_suggestions;
    vitte_cli_diagnostic_cause_t *owned_causes;
} vitte_cli_diagnostic_transport_header_t;

enum {
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_TEXT_SLOTS = 11,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_CODE = 0,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_MESSAGE,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_HELP,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_EXPECTED,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_FOUND,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_SOURCE_NAME,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_SYMBOL,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_PROBABLE_CAUSE,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_CONTEXT,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_RELATED_SOURCE_NAME,
    VITTE_CLI_DIAGNOSTIC_TRANSPORT_RELATED_LABEL
};

#ifndef VITTE_NATIVE_FRONTEND_LINKED
static bool
vitte_cli_diagnostic_transport_copy_text(
    void *batch,
    size_t index,
    size_t slot,
    const char *source,
    const char **copy
)
{
    vitte_cli_diagnostic_transport_header_t *header;
    char *owned;
    size_t length;
    size_t slot_index;

    if (copy == NULL || batch == NULL ||
        slot >= VITTE_CLI_DIAGNOSTIC_TRANSPORT_TEXT_SLOTS) {
        return false;
    }
    *copy = NULL;
    if (source == NULL) {
        return true;
    }

    header = ((vitte_cli_diagnostic_transport_header_t *)batch) - 1;
    if (index >= header->count ||
        index > (SIZE_MAX - slot) /
            VITTE_CLI_DIAGNOSTIC_TRANSPORT_TEXT_SLOTS) {
        return false;
    }
    slot_index = index * VITTE_CLI_DIAGNOSTIC_TRANSPORT_TEXT_SLOTS + slot;
    length = strlen(source);
    if (length == SIZE_MAX) {
        return false;
    }
    owned = (char *)malloc(length + 1u);
    if (owned == NULL) {
        return false;
    }
    memcpy(owned, source, length + 1u);
    free(header->owned_text[slot_index]);
    header->owned_text[slot_index] = owned;
    *copy = owned;
    return true;
}

static void
vitte_cli_diagnostic_transport_add_parser_suggestion(
    vitte_cli_diagnostic_t *diagnostic,
    vitte_cli_diagnostic_suggestion_t *suggestion,
    uint64_t diagnostic_id)
{
    const char *replacement = NULL;
    const char *message = NULL;
    bool replace_token = false;
    bool allow_token_replacement;
    bool expression_list_missing_comma;

    if (diagnostic == NULL || suggestion == NULL ||
        !diagnostic->has_span || diagnostic->source_name == NULL) {
        return;
    }

    if (diagnostic->origin == VITTE_CLI_DIAGNOSTIC_ORIGIN_LEXER &&
        diagnostic->code != NULL &&
        strcmp(diagnostic->code, "VITTE_LEXER_E_INVALID_CHARACTER") == 0 &&
        diagnostic->details != NULL &&
        strncmp(diagnostic->details, "invalid character `'",
                sizeof("invalid character `'" ) - 1u) == 0) {
        memset(suggestion, 0, sizeof(*suggestion));
        suggestion->message = "Possible edit: add a closing `'` only if this was meant to be a one-character literal. The transport cannot verify that from this diagnostic, so review the source manually.";
        suggestion->applicability = VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MANUAL;
        suggestion->confidence = VITTE_CLI_DIAGNOSTIC_CONFIDENCE_LOW;
        suggestion->preferred = false;
        suggestion->suggestion_id = diagnostic_id;
        suggestion->has_span = true;
        suggestion->span.valid = true;
        suggestion->span.file_id = diagnostic->file_id;
        suggestion->span.source_id = diagnostic->source_id;
        suggestion->span.source_name = diagnostic->source_name;
        suggestion->span.start_offset = diagnostic->end_offset;
        suggestion->span.end_offset = diagnostic->end_offset;
        suggestion->span.start_line = diagnostic->end_line;
        suggestion->span.start_column = diagnostic->end_column;
        suggestion->span.end_line = diagnostic->end_line;
        suggestion->span.end_column = diagnostic->end_column;
        suggestion->has_replacement = true;
        suggestion->replacement = "'";
        diagnostic->suggestions = suggestion;
        diagnostic->suggestion_count = 1u;
        diagnostic->has_machine_fix = false;
        return;
    }

    expression_list_missing_comma =
        diagnostic->code != NULL &&
        strcmp(diagnostic->code,
               "VITTE_PARSER_E_INVALID_EXPRESSION_LIST") == 0 &&
        diagnostic->expected != NULL &&
        strcmp(diagnostic->expected, ",") == 0;
    if (diagnostic->origin != VITTE_CLI_DIAGNOSTIC_ORIGIN_PARSER ||
        diagnostic->code == NULL ||
        (strcmp(diagnostic->code, "VITTE_PARSER_E_EXPECTED_TOKEN") != 0 &&
         strcmp(diagnostic->code, "VITTE_PARSER_E_EXPECTED_EXPRESSION") != 0 &&
         !expression_list_missing_comma) ||
        diagnostic->found == NULL) {
        return;
    }

    allow_token_replacement = !expression_list_missing_comma;

    if (allow_token_replacement && strcmp(diagnostic->found, "&&") == 0) {
        replacement = "and";
        message = "If logical conjunction was intended, use `and`; review the surrounding expression because this edit changes the operator token.";
        replace_token = true;
    } else if (allow_token_replacement &&
               strcmp(diagnostic->found, "||") == 0) {
        replacement = "or";
        message = "If logical disjunction was intended, use `or`; review the surrounding expression because this edit changes the operator token.";
        replace_token = true;
    } else if (allow_token_replacement &&
               strcmp(diagnostic->found, "!") == 0) {
        replacement = "not";
        message = "If logical negation was intended, use `not`; review the operand and grouping because this edit changes the operator token.";
        replace_token = true;
    } else if (allow_token_replacement && diagnostic->expected != NULL &&
               strcmp(diagnostic->found, ".") == 0 &&
               strcmp(diagnostic->expected, "::") == 0) {
        replacement = "::";
        message = "If this dot separates import path segments, use `::`; review the path before changing its qualification.";
        replace_token = true;
    } else if (diagnostic->expected != NULL &&
               strcmp(diagnostic->expected, ")") == 0 &&
               (strcmp(diagnostic->found, "<eof>") == 0 ||
                strcmp(diagnostic->found, ";") == 0 ||
                strcmp(diagnostic->found, "}") == 0 ||
                strcmp(diagnostic->found, "]") == 0)) {
        replacement = ")";
    } else if (diagnostic->expected != NULL &&
               strcmp(diagnostic->expected, "]") == 0 &&
               (strcmp(diagnostic->found, "<eof>") == 0 ||
                strcmp(diagnostic->found, ";") == 0 ||
                strcmp(diagnostic->found, "}") == 0 ||
                strcmp(diagnostic->found, ")") == 0)) {
        replacement = "]";
    } else if (diagnostic->expected != NULL &&
               strcmp(diagnostic->expected, "}") == 0 &&
               (strcmp(diagnostic->found, "<eof>") == 0 ||
                strcmp(diagnostic->found, ";") == 0 ||
                strcmp(diagnostic->found, "else") == 0 ||
                strcmp(diagnostic->found, "elif") == 0)) {
        replacement = "}";
    } else if (diagnostic->expected != NULL &&
               strcmp(diagnostic->expected, ";") == 0 &&
               (strcmp(diagnostic->found, "<eof>") == 0 ||
                strcmp(diagnostic->found, "}") == 0 ||
                strcmp(diagnostic->found, "let") == 0 ||
                strcmp(diagnostic->found, "return") == 0 ||
                strcmp(diagnostic->found, "print") == 0 ||
                strcmp(diagnostic->found, "if") == 0 ||
                strcmp(diagnostic->found, "while") == 0 ||
                strcmp(diagnostic->found, "for") == 0 ||
                strcmp(diagnostic->found, "break") == 0 ||
                strcmp(diagnostic->found, "continue") == 0 ||
                strcmp(diagnostic->found, "defer") == 0 ||
                strcmp(diagnostic->found, "set") == 0 ||
                strcmp(diagnostic->found, "assert") == 0)) {
        replacement = ";";
    } else if (diagnostic->expected != NULL &&
               strcmp(diagnostic->expected, ",") == 0 &&
               (strcmp(diagnostic->found, "identifier") == 0 ||
                strcmp(diagnostic->found, "integer_literal") == 0 ||
                strcmp(diagnostic->found, "float_literal") == 0 ||
                strcmp(diagnostic->found, "string_literal") == 0 ||
                strcmp(diagnostic->found, "character_literal") == 0 ||
                strcmp(diagnostic->found, "true") == 0 ||
                strcmp(diagnostic->found, "false") == 0 ||
                strcmp(diagnostic->found, "null") == 0 ||
                strcmp(diagnostic->found, "self") == 0 ||
                strcmp(diagnostic->found, "if") == 0 ||
                strcmp(diagnostic->found, "elif") == 0 ||
                strcmp(diagnostic->found, "print") == 0 ||
                strcmp(diagnostic->found, "sizeof") == 0 ||
                strcmp(diagnostic->found, "alignof") == 0 ||
                strcmp(diagnostic->found, "(") == 0 ||
                strcmp(diagnostic->found, "[") == 0 ||
                strcmp(diagnostic->found, "{") == 0 ||
                strcmp(diagnostic->found, "+") == 0 ||
                strcmp(diagnostic->found, "-") == 0 ||
                strcmp(diagnostic->found, "~") == 0 ||
                strcmp(diagnostic->found, "*") == 0 ||
                strcmp(diagnostic->found, "&") == 0 ||
                strcmp(diagnostic->found, "not") == 0 ||
                strcmp(diagnostic->found, "move") == 0 ||
                strcmp(diagnostic->found, "ref") == 0 ||
                strcmp(diagnostic->found, "await") == 0)) {
        replacement = ",";
    }

    if (replacement == NULL) {
        return;
    }
    if (strcmp(replacement, ",") == 0) {
        message = expression_list_missing_comma
            ? "Insert a comma only if these are separate list items; if one expression was intended, add the intended operator or grouping instead."
            : "A comma may be missing here; apply it only if the following token starts a separate list item.";
    } else if (!replace_token) {
        message = strcmp(replacement, ")") == 0
            ? "Insert the missing `)` here."
            : strcmp(replacement, "]") == 0
                ? "Insert the missing `]` here."
                : strcmp(replacement, "}") == 0
                    ? "Insert the missing `}` here."
                    : strcmp(replacement, ";") == 0
                        ? "Insert the missing `;` here."
                        : "Insert the missing `,` here.";
    }

    memset(suggestion, 0, sizeof(*suggestion));
    suggestion->message = message;
    suggestion->applicability = (replace_token ||
        strcmp(replacement, ",") == 0)
        ? VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MAYBE
        : VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MACHINE;
    suggestion->confidence = (replace_token ||
        strcmp(replacement, ",") == 0)
        ? VITTE_CLI_DIAGNOSTIC_CONFIDENCE_MEDIUM
        : VITTE_CLI_DIAGNOSTIC_CONFIDENCE_HIGH;
    suggestion->preferred = !replace_token &&
        strcmp(replacement, ",") != 0;
    suggestion->suggestion_id = diagnostic_id;
    suggestion->has_span = true;
    suggestion->span.valid = true;
    suggestion->span.file_id = diagnostic->file_id;
    suggestion->span.source_id = diagnostic->source_id;
    suggestion->span.source_name = diagnostic->source_name;
    suggestion->span.start_offset = replace_token
        ? diagnostic->start_offset : diagnostic->start_offset;
    suggestion->span.end_offset = replace_token
        ? diagnostic->end_offset : diagnostic->start_offset;
    suggestion->span.start_line = diagnostic->start_line;
    suggestion->span.start_column = diagnostic->start_column;
    suggestion->span.end_line = replace_token
        ? diagnostic->end_line : diagnostic->start_line;
    suggestion->span.end_column = replace_token
        ? diagnostic->end_column : diagnostic->start_column;
    suggestion->has_replacement = true;
    suggestion->replacement = replacement;
    diagnostic->suggestions = suggestion;
    diagnostic->suggestion_count = 1u;
    diagnostic->has_machine_fix = !replace_token &&
        strcmp(replacement, ",") != 0;
}
#endif

bool
vitte_cli_diagnostic_format_is_valid(
    vitte_cli_diagnostic_format_t format
)
{
    return format >= VITTE_CLI_DIAGNOSTIC_TERMINAL &&
        format <= VITTE_CLI_DIAGNOSTIC_LSP;
}

void
vitte_cli_diagnostic_span_init(
    vitte_cli_diagnostic_span_t *span
)
{
    if (span == NULL) {
        return;
    }

    memset(span, 0, sizeof(*span));
    span->source_id = VITTE_SOURCE_INVALID_ID;
}

void
vitte_cli_diagnostic_init(
    vitte_cli_diagnostic_t *diagnostic
)
{
    if (diagnostic == NULL) {
        return;
    }

    memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->severity = VITTE_CLI_DIAGNOSTIC_ERROR;
    diagnostic->origin = VITTE_CLI_DIAGNOSTIC_ORIGIN_UNKNOWN;
    diagnostic->source_id = VITTE_SOURCE_INVALID_ID;
    diagnostic->parent_diagnostic = VITTE_CLI_DIAGNOSTIC_INDEX_NONE;
    diagnostic->root_diagnostic = VITTE_CLI_DIAGNOSTIC_INDEX_NONE;
    diagnostic->is_primary = true;
}

#ifndef VITTE_NATIVE_FRONTEND_LINKED
void *
vitte_cli_diagnostic_transport_create(
    size_t count
)
{
    vitte_cli_diagnostic_transport_header_t *header;
    vitte_cli_diagnostic_t *diagnostics;
    size_t index;

    if (count == 0u ||
        count > (SIZE_MAX - sizeof(*header)) / sizeof(*diagnostics) ||
        count > SIZE_MAX /
            VITTE_CLI_DIAGNOSTIC_TRANSPORT_TEXT_SLOTS / sizeof(char *)) {
        return NULL;
    }
    header = (vitte_cli_diagnostic_transport_header_t *)calloc(
        1u,
        sizeof(*header) + count * sizeof(*diagnostics));
    if (header == NULL) {
        return NULL;
    }
    header->count = count;
    header->owned_text = (char **)calloc(
        count * VITTE_CLI_DIAGNOSTIC_TRANSPORT_TEXT_SLOTS,
        sizeof(*header->owned_text));
    header->owned_suggestions = (vitte_cli_diagnostic_suggestion_t *)calloc(
        count, sizeof(*header->owned_suggestions));
    header->owned_causes = (vitte_cli_diagnostic_cause_t *)calloc(
        count, sizeof(*header->owned_causes));
    if (header->owned_text == NULL || header->owned_suggestions == NULL ||
        header->owned_causes == NULL) {
        free(header->owned_causes);
        free(header->owned_suggestions);
        free(header->owned_text);
        free(header);
        return NULL;
    }
    diagnostics = (vitte_cli_diagnostic_t *)(header + 1);
    for (index = 0u; index < count; ++index) {
        vitte_cli_diagnostic_init(&diagnostics[index]);
    }
    return diagnostics;
}

bool
vitte_cli_diagnostic_transport_set(
    void *batch,
    size_t count,
    size_t index,
    uint64_t diagnostic_id,
    const char *code,
    uint32_t severity,
    uint32_t origin,
    const char *message,
    const char *details,
    const char *help,
    bool has_span,
    uint32_t file_id,
    uint64_t source_id,
    const char *source_name,
    size_t start_offset,
    size_t end_offset,
    size_t start_line,
    size_t start_column,
    size_t end_line,
    size_t end_column,
    const char *expected,
    const char *found)
{
    vitte_cli_diagnostic_t *diagnostics;
    vitte_cli_diagnostic_t *diagnostic;
    vitte_cli_diagnostic_transport_header_t *header;
    const char *owned_code;
    const char *owned_message;
    const char *owned_help;
    const char *owned_expected;
    const char *owned_found;
    const char *owned_source_name;

    if (batch == NULL || index >= count || code == NULL || message == NULL ||
        severity > (uint32_t)VITTE_CLI_DIAGNOSTIC_FATAL ||
        origin > (uint32_t)VITTE_CLI_DIAGNOSTIC_ORIGIN_INTERNAL ||
        (has_span && end_offset < start_offset)) {
        return false;
    }

    if (!vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_CODE,
            code, &owned_code) ||
        !vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_MESSAGE,
            message, &owned_message) ||
        !vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_HELP,
            help, &owned_help) ||
        !vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_EXPECTED,
            expected, &owned_expected) ||
        !vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_FOUND,
            found, &owned_found) ||
        !vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_SOURCE_NAME,
            source_name, &owned_source_name)) {
        return false;
    }

    diagnostics = (vitte_cli_diagnostic_t *)batch;
    diagnostic = &diagnostics[index];
    vitte_cli_diagnostic_init(diagnostic);
    diagnostic->diagnostic_id = diagnostic_id;
    diagnostic->code = owned_code;
    diagnostic->severity = (vitte_cli_diagnostic_severity_t)severity;
    diagnostic->origin = (vitte_cli_diagnostic_origin_t)origin;
    diagnostic->message = owned_message;
    diagnostic->help = owned_help;
    diagnostic->expected = owned_expected;
    diagnostic->found = owned_found;
    diagnostic->is_primary = true;
    diagnostic->poisons_analysis =
        diagnostic->severity == VITTE_CLI_DIAGNOSTIC_ERROR ||
        diagnostic->severity == VITTE_CLI_DIAGNOSTIC_FATAL;

    if (details != NULL && details[0] != '\0') {
        size_t length = 0u;

        while (length + 1u < sizeof(diagnostic->details_storage) &&
               details[length] != '\0') {
            ++length;
        }
        if (length != 0u) {
            memcpy(diagnostic->details_storage, details, length);
        }
        diagnostic->details_storage[length] = '\0';
        diagnostic->details = diagnostic->details_storage;
    }

    if (has_span) {
        diagnostic->has_span = true;
        diagnostic->file_id = file_id;
        diagnostic->source_id = source_id;
        diagnostic->source_name = owned_source_name;
        diagnostic->start_offset = start_offset;
        diagnostic->end_offset = end_offset;
        diagnostic->start_line = start_line;
        diagnostic->start_column = start_column;
        diagnostic->end_line = end_line;
        diagnostic->end_column = end_column;
    }
    header = ((vitte_cli_diagnostic_transport_header_t *)batch) - 1;
    vitte_cli_diagnostic_transport_add_parser_suggestion(
        diagnostic,
        &header->owned_suggestions[index],
        diagnostic_id);
    return true;
}

bool
vitte_cli_diagnostic_transport_set_semantic(
    void *batch,
    size_t count,
    size_t index,
    const char *symbol,
    const char *probable_cause,
    const char *context,
    bool has_related,
    uint32_t related_file_id,
    uint64_t related_source_id,
    const char *related_source_name,
    size_t related_start_offset,
    size_t related_end_offset,
    size_t related_start_line,
    size_t related_start_column,
    size_t related_end_line,
    size_t related_end_column,
    const char *related_label)
{
    vitte_cli_diagnostic_t *diagnostics;
    vitte_cli_diagnostic_t *diagnostic;
    vitte_cli_diagnostic_transport_header_t *header;
    vitte_cli_diagnostic_location_t *location;
    const char *owned_symbol;
    const char *owned_probable_cause;
    const char *owned_context;
    const char *owned_related_source_name;
    const char *owned_related_label;

    if (batch == NULL || index >= count ||
        (has_related && (related_end_offset < related_start_offset ||
                         related_source_name == NULL || related_label == NULL))) {
        return false;
    }

    if (!vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_SYMBOL,
            symbol, &owned_symbol) ||
        !vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_PROBABLE_CAUSE,
            probable_cause, &owned_probable_cause) ||
        !vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_CONTEXT,
            context, &owned_context) ||
        !vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_RELATED_SOURCE_NAME,
            related_source_name, &owned_related_source_name) ||
        !vitte_cli_diagnostic_transport_copy_text(
            batch, index, VITTE_CLI_DIAGNOSTIC_TRANSPORT_RELATED_LABEL,
            related_label, &owned_related_label)) {
        return false;
    }

    diagnostics = (vitte_cli_diagnostic_t *)batch;
    diagnostic = &diagnostics[index];
    header = ((vitte_cli_diagnostic_transport_header_t *)batch) - 1;
    diagnostic->symbol = owned_symbol;
    diagnostic->probable_cause = owned_probable_cause;
    diagnostic->context = owned_context;
    if (has_related) {
        location = (vitte_cli_diagnostic_location_t *)calloc(1u, sizeof(*location));
        if (location == NULL) {
            return false;
        }
        location->kind = VITTE_CLI_DIAGNOSTIC_LOCATION_RELATED;
        location->label = owned_related_label;
        location->span.valid = true;
        location->span.file_id = related_file_id;
        location->span.source_id = related_source_id;
        location->span.source_name = owned_related_source_name;
        location->span.start_offset = related_start_offset;
        location->span.end_offset = related_end_offset;
        location->span.start_line = related_start_line;
        location->span.start_column = related_start_column;
        location->span.end_line = related_end_line;
        location->span.end_column = related_end_column;
        diagnostic->related = location;
        diagnostic->related_count = 1u;
    }
    if (diagnostic->origin == VITTE_CLI_DIAGNOSTIC_ORIGIN_CONTRACT ||
        diagnostic->origin == VITTE_CLI_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL) {
        vitte_cli_diagnostic_cause_t *cause;
        cause = &header->owned_causes[index];
        memset(cause, 0, sizeof(*cause));
        cause->kind = diagnostic->origin == VITTE_CLI_DIAGNOSTIC_ORIGIN_CONTRACT
            ? VITTE_CLI_DIAGNOSTIC_CAUSE_CONTRACT
            : VITTE_CLI_DIAGNOSTIC_CAUSE_CONSTANT_EVAL;
        cause->message = owned_probable_cause != NULL
            ? owned_probable_cause
            : diagnostic->help != NULL
                ? diagnostic->help
                : diagnostic->origin == VITTE_CLI_DIAGNOSTIC_ORIGIN_CONTRACT
                    ? "contract condition is invalid or false"
                    : "compile-time evaluation cannot produce a value";
        cause->confidence = VITTE_CLI_DIAGNOSTIC_CONFIDENCE_CERTAIN;
        if (has_related && diagnostic->origin == VITTE_CLI_DIAGNOSTIC_ORIGIN_CONTRACT) {
            cause->has_span = true;
            cause->span = location->span;
        } else if (diagnostic->has_span) {
            cause->has_span = true;
            cause->span.valid = true;
            cause->span.file_id = diagnostic->file_id;
            cause->span.source_id = diagnostic->source_id;
            cause->span.source_name = diagnostic->source_name;
            cause->span.start_offset = diagnostic->start_offset;
            cause->span.end_offset = diagnostic->end_offset;
            cause->span.start_line = diagnostic->start_line;
            cause->span.start_column = diagnostic->start_column;
            cause->span.end_line = diagnostic->end_line;
            cause->span.end_column = diagnostic->end_column;
        }
        diagnostic->causes = cause;
        diagnostic->cause_count = 1u;
    }
    return true;
}

void
vitte_cli_diagnostic_transport_destroy(
    void *batch)
{
    vitte_cli_diagnostic_t *diagnostics = (vitte_cli_diagnostic_t *)batch;
    vitte_cli_diagnostic_transport_header_t *header;
    size_t index;

    if (diagnostics == NULL) {
        return;
    }
    header = ((vitte_cli_diagnostic_transport_header_t *)batch) - 1;
    for (index = 0u; index < header->count; ++index) {
        size_t slot;

        for (slot = 0u;
             slot < VITTE_CLI_DIAGNOSTIC_TRANSPORT_TEXT_SLOTS;
             ++slot) {
            free(header->owned_text[
                index * VITTE_CLI_DIAGNOSTIC_TRANSPORT_TEXT_SLOTS + slot]);
        }
        if (diagnostics[index].related_count != 0u &&
            diagnostics[index].related != NULL) {
            free((void *)diagnostics[index].related);
        }
    }
    free(header->owned_text);
    free(header->owned_suggestions);
    free(header->owned_causes);
    free(header);
}
#endif

void
vitte_cli_diagnostic_render_options_init(
    vitte_cli_diagnostic_render_options_t *options
)
{
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->format = VITTE_CLI_DIAGNOSTIC_TERMINAL;
    options->show_source = true;
    options->show_source_line = true;
    options->show_codes = true;
    options->show_details = true;
    options->show_origin = true;
    options->show_category = true;
    options->show_labels = true;
    options->show_expected_found = true;
    options->show_causes = true;
    options->show_notes = true;
    options->show_help = true;
    options->show_related = true;
    options->show_suggestions = true;
    options->show_trace = true;
    options->show_suppressed = false;
    options->show_cascades = true;
    options->show_internal = false;
    options->show_summary = true;
    options->context_lines_before = 1u;
    options->context_lines_after = 1u;
    options->position_encoding = 1; /* LSP UTF-16 */
    options->pretty = true;
    options->trailing_newline = true;
}

void
vitte_cli_diagnostic_statistics_init(
    vitte_cli_diagnostic_statistics_t *statistics
)
{
    if (statistics != NULL) {
        memset(statistics, 0, sizeof(*statistics));
    }
}
