#include "diagnostic.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "registry.h"

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_DIAGNOSTIC_INDEX_NONE
#define VITTE_DIAGNOSTIC_INDEX_NONE SIZE_MAX
#endif

#ifndef VITTE_SOURCE_ID_INVALID
#define VITTE_SOURCE_ID_INVALID ((vitte_source_id_t)0)
#endif

#define VITTE_DIAGNOSTIC_FNV_OFFSET UINT64_C(14695981039346656037)
#define VITTE_DIAGNOSTIC_FNV_PRIME  UINT64_C(1099511628211)

/* ========================================================================= */
/* Internal text helpers                                                     */
/* ========================================================================= */

static bool
vitte_diagnostic_text_present(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static size_t
vitte_diagnostic_bounded_length(
    const char *text,
    size_t maximum
)
{
    size_t length;

    if (text == NULL) {
        return 0u;
    }

    length = 0u;

    while (length < maximum && text[length] != '\0') {
        ++length;
    }

    return length;
}

static const char *
vitte_diagnostic_copy_text(
    char *destination,
    size_t capacity,
    const char *source
)
{
    size_t length;

    if (destination == NULL || capacity == 0u) {
        return NULL;
    }

    destination[0] = '\0';

    if (source == NULL) {
        return destination;
    }

    length = vitte_diagnostic_bounded_length(source, capacity - 1u);

    if (length != 0u) {
        memcpy(destination, source, length);
    }

    destination[length] = '\0';

    return destination;
}

static bool
vitte_diagnostic_text_equal(
    const char *left,
    const char *right
)
{
    if (left == right) {
        return true;
    }

    if (left == NULL || right == NULL) {
        return false;
    }

    return strcmp(left, right) == 0;
}

/* ========================================================================= */
/* Source/span helpers                                                       */
/* ========================================================================= */

static bool
vitte_diagnostic_source_id_valid(vitte_source_id_t source_id)
{
    return source_id != VITTE_SOURCE_ID_INVALID;
}

static bool
vitte_diagnostic_span_valid(const vitte_ast_span_t *span)
{
    if (span == NULL) {
        return false;
    }

    if (span->end_offset < span->start_offset) {
        return false;
    }

    if (vitte_diagnostic_source_id_valid(span->source_id)) {
        return true;
    }

    return vitte_diagnostic_text_present(span->source_name);
}

static bool
vitte_diagnostic_same_source(
    vitte_source_id_t left_id,
    const char *left_name,
    vitte_source_id_t right_id,
    const char *right_name
)
{
    if (vitte_diagnostic_source_id_valid(left_id) &&
        vitte_diagnostic_source_id_valid(right_id)) {
        return left_id == right_id;
    }

    if (vitte_diagnostic_text_present(left_name) &&
        vitte_diagnostic_text_present(right_name)) {
        return strcmp(left_name, right_name) == 0;
    }

    return false;
}

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

static uint64_t
vitte_diagnostic_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t size
)
{
    const unsigned char *bytes;
    size_t index;

    bytes = (const unsigned char *)data;

    for (index = 0u; index < size; ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_DIAGNOSTIC_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_diagnostic_hash_text(
    uint64_t hash,
    const char *text
)
{
    if (text == NULL) {
        static const unsigned char zero = 0u;

        return vitte_diagnostic_hash_bytes(
            hash,
            &zero,
            sizeof(zero)
        );
    }

    return vitte_diagnostic_hash_bytes(
        hash,
        text,
        strlen(text)
    );
}

static uint64_t
vitte_diagnostic_hash_size(
    uint64_t hash,
    size_t value
)
{
    return vitte_diagnostic_hash_bytes(
        hash,
        &value,
        sizeof(value)
    );
}

static uint64_t
vitte_diagnostic_hash_u64(
    uint64_t hash,
    uint64_t value
)
{
    return vitte_diagnostic_hash_bytes(
        hash,
        &value,
        sizeof(value)
    );
}

/* ========================================================================= */
/* Severity                                                                  */
/* ========================================================================= */

const char *
vitte_diagnostic_severity_name(
    vitte_diagnostic_severity_t severity
)
{
    switch (severity) {
        case VITTE_DIAGNOSTIC_NOTE:
            return "note";

        case VITTE_DIAGNOSTIC_HELP:
            return "help";

        case VITTE_DIAGNOSTIC_WARNING:
            return "warning";

        case VITTE_DIAGNOSTIC_ERROR:
            return "error";

        case VITTE_DIAGNOSTIC_FATAL:
            return "fatal";

        default:
            return "unknown";
    }
}

bool
vitte_diagnostic_severity_is_error(
    vitte_diagnostic_severity_t severity
)
{
    return severity == VITTE_DIAGNOSTIC_ERROR ||
           severity == VITTE_DIAGNOSTIC_FATAL;
}

/* ========================================================================= */
/* Origin                                                                    */
/* ========================================================================= */

const char *
vitte_diagnostic_origin_name(
    vitte_diagnostic_origin_t origin
)
{
    switch (origin) {
        case VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN:
            return "unknown";

        case VITTE_DIAGNOSTIC_ORIGIN_IO:
            return "io";

        case VITTE_DIAGNOSTIC_ORIGIN_LEXER:
            return "lexer";

        case VITTE_DIAGNOSTIC_ORIGIN_PARSER:
            return "parser";

        case VITTE_DIAGNOSTIC_ORIGIN_IMPORT:
            return "import";

        case VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION:
            return "name-resolution";

        case VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK:
            return "type-check";

        case VITTE_DIAGNOSTIC_ORIGIN_CONTRACT:
            return "contract";

        case VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL:
            return "constant-eval";

        case VITTE_DIAGNOSTIC_ORIGIN_HIR:
            return "hir";

        case VITTE_DIAGNOSTIC_ORIGIN_IR:
            return "ir";

        case VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND:
            return "c17-backend";

        case VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER:
            return "c-compiler";

        case VITTE_DIAGNOSTIC_ORIGIN_LINKER:
            return "linker";

        case VITTE_DIAGNOSTIC_ORIGIN_RUNTIME:
            return "runtime";

        case VITTE_DIAGNOSTIC_ORIGIN_DRIVER:
            return "driver";

        case VITTE_DIAGNOSTIC_ORIGIN_INTERNAL:
            return "internal";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Location kind                                                             */
/* ========================================================================= */

const char *
vitte_diagnostic_location_kind_name(
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
            return "related";
    }
}

/* ========================================================================= */
/* Annotation kind                                                           */
/* ========================================================================= */

const char *
vitte_diagnostic_annotation_kind_name(
    vitte_diagnostic_annotation_kind_t kind
)
{
    switch (kind) {
        case VITTE_DIAGNOSTIC_ANNOTATION_NOTE:
            return "note";

        case VITTE_DIAGNOSTIC_ANNOTATION_HELP:
            return "help";

        default:
            return "note";
    }
}

/* ========================================================================= */
/* Format                                                                    */
/* ========================================================================= */

const char *
vitte_diagnostic_format_name(
    vitte_diagnostic_format_t format
)
{
    switch (format) {
        case VITTE_DIAGNOSTIC_FORMAT_TERMINAL:
            return "terminal";

        case VITTE_DIAGNOSTIC_FORMAT_JSON:
            return "json";

        case VITTE_DIAGNOSTIC_FORMAT_SARIF:
            return "sarif";

        case VITTE_DIAGNOSTIC_FORMAT_LSP:
            return "lsp";

        default:
            return "terminal";
    }
}

/* ========================================================================= */
/* Options                                                                   */
/* ========================================================================= */

void
vitte_diagnostic_options_init(
    vitte_diagnostic_options_t *options
)
{
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));

    options->max_diagnostics = 100u;
    options->warnings_as_errors = false;
    options->color_enabled = true;
    options->show_source_line = true;
    options->show_codes = true;
    options->show_details = true;
    options->show_phase = true;
    options->show_causes = true;
    options->show_suggestions = true;
    options->sources = NULL;
}

/* ========================================================================= */
/* Counts                                                                    */
/* ========================================================================= */

void
vitte_diagnostic_counts_init(
    vitte_diagnostic_counts_t *counts
)
{
    if (counts == NULL) {
        return;
    }

    memset(counts, 0, sizeof(*counts));
}

static void
vitte_diagnostic_counts_increment(
    vitte_diagnostic_counts_t *counts,
    vitte_diagnostic_severity_t severity
)
{
    if (counts == NULL) {
        return;
    }

    switch (severity) {
        case VITTE_DIAGNOSTIC_NOTE:
            ++counts->note_count;
            break;

        case VITTE_DIAGNOSTIC_HELP:
            ++counts->help_count;
            break;

        case VITTE_DIAGNOSTIC_WARNING:
            ++counts->warning_count;
            break;

        case VITTE_DIAGNOSTIC_ERROR:
            ++counts->error_count;
            break;

        case VITTE_DIAGNOSTIC_FATAL:
            ++counts->fatal_count;
            break;

        default:
            break;
    }
}

/* ========================================================================= */
/* Rebind helpers                                                            */
/* ========================================================================= */

static void
vitte_diagnostic_rebind_label(
    vitte_diagnostic_label_t *label
)
{
    if (label == NULL) {
        return;
    }

    label->message = label->message_storage;
}

static void
vitte_diagnostic_rebind_location(
    vitte_diagnostic_location_t *location
)
{
    if (location == NULL) {
        return;
    }

    location->label = location->label_storage;
}

static void
vitte_diagnostic_rebind_cause(
    vitte_diagnostic_cause_t *cause
)
{
    if (cause == NULL) {
        return;
    }

    cause->message = cause->message_storage;
}

static void
vitte_diagnostic_rebind_annotation(
    vitte_diagnostic_annotation_t *annotation
)
{
    if (annotation == NULL) {
        return;
    }

    annotation->message = annotation->message_storage;
}

static void
vitte_diagnostic_rebind_suggestion(
    vitte_diagnostic_suggestion_t *suggestion
)
{
    if (suggestion == NULL) {
        return;
    }

    suggestion->message = suggestion->message_storage;
    suggestion->replacement = suggestion->replacement_storage;
}

static void
vitte_diagnostic_rebind_contract(
    vitte_diagnostic_contract_t *contract
)
{
    if (contract == NULL) {
        return;
    }

    contract->expression = contract->expression_storage;
    contract->procedure = contract->procedure_storage;
    contract->reason = contract->reason_storage;
}

static void
vitte_diagnostic_rebind(
    vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return;
    }

    diagnostic->code = diagnostic->code_storage;
    diagnostic->short_code = diagnostic->short_code_storage;
    diagnostic->category = diagnostic->category_storage;
    diagnostic->message = diagnostic->message_storage;
    diagnostic->details = diagnostic->details_storage;
    diagnostic->symbol = diagnostic->symbol_storage;
    diagnostic->expected_type = diagnostic->expected_type_storage;
    diagnostic->actual_type = diagnostic->actual_type_storage;
    diagnostic->context = diagnostic->context_storage;
    diagnostic->package = diagnostic->package_storage;
    diagnostic->module = diagnostic->module_storage;
    diagnostic->procedure = diagnostic->procedure_storage;
    diagnostic->source_name = diagnostic->source_name_storage;

    for (index = 0u; index < diagnostic->label_count; ++index) {
        vitte_diagnostic_rebind_label(&diagnostic->labels[index]);
    }

    for (index = 0u; index < diagnostic->related_count; ++index) {
        vitte_diagnostic_rebind_location(&diagnostic->related[index]);
    }

    for (index = 0u; index < diagnostic->cause_count; ++index) {
        vitte_diagnostic_rebind_cause(&diagnostic->causes[index]);
    }

    for (index = 0u; index < diagnostic->annotation_count; ++index) {
        vitte_diagnostic_rebind_annotation(
            &diagnostic->annotations[index]
        );
    }

    for (index = 0u; index < diagnostic->suggestion_count; ++index) {
        vitte_diagnostic_rebind_suggestion(
            &diagnostic->suggestions[index]
        );
    }

    if (diagnostic->has_contract) {
        vitte_diagnostic_rebind_contract(&diagnostic->contract);
    }
}

/* ========================================================================= */
/* Diagnostic initialization                                                 */
/* ========================================================================= */

void
vitte_diagnostic_init(
    vitte_diagnostic_t *diagnostic
)
{
    if (diagnostic == NULL) {
        return;
    }

    memset(diagnostic, 0, sizeof(*diagnostic));

    diagnostic->severity = VITTE_DIAGNOSTIC_ERROR;
    diagnostic->origin = VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN;

    diagnostic->source_id = VITTE_SOURCE_ID_INVALID;

    diagnostic->root_diagnostic = VITTE_DIAGNOSTIC_INDEX_NONE;
    diagnostic->parent_diagnostic = VITTE_DIAGNOSTIC_INDEX_NONE;

    diagnostic->is_primary = true;
    diagnostic->is_cascade = false;
    diagnostic->is_suppressed = false;

    diagnostic->code = diagnostic->code_storage;
    diagnostic->short_code = diagnostic->short_code_storage;
    diagnostic->category = diagnostic->category_storage;
    diagnostic->message = diagnostic->message_storage;
    diagnostic->details = diagnostic->details_storage;
    diagnostic->symbol = diagnostic->symbol_storage;
    diagnostic->expected_type = diagnostic->expected_type_storage;
    diagnostic->actual_type = diagnostic->actual_type_storage;
    diagnostic->context = diagnostic->context_storage;
    diagnostic->package = diagnostic->package_storage;
    diagnostic->module = diagnostic->module_storage;
    diagnostic->procedure = diagnostic->procedure_storage;
    diagnostic->source_name = diagnostic->source_name_storage;
}

void
vitte_diagnostic_reset(
    vitte_diagnostic_t *diagnostic
)
{
    vitte_diagnostic_init(diagnostic);
}

/* ========================================================================= */
/* Registry metadata                                                         */
/* ========================================================================= */

static void
vitte_diagnostic_apply_registry_metadata(
    vitte_diagnostic_t *diagnostic
)
{
    const vitte_diagnostic_registry_entry_t *entry;
    const char *fallback;

    if (diagnostic == NULL) {
        return;
    }

    entry = NULL;

    if (vitte_diagnostic_text_present(diagnostic->code)) {
        entry = vitte_diagnostic_registry_find_internal(
            diagnostic->code
        );
    }

    if (entry == NULL &&
        vitte_diagnostic_text_present(diagnostic->short_code)) {
        entry = vitte_diagnostic_registry_find_public(
            diagnostic->short_code
        );
    }

    if (entry != NULL) {
        diagnostic->code =
            vitte_diagnostic_copy_text(
                diagnostic->code_storage,
                sizeof(diagnostic->code_storage),
                entry->internal_code
            );

        diagnostic->short_code =
            vitte_diagnostic_copy_text(
                diagnostic->short_code_storage,
                sizeof(diagnostic->short_code_storage),
                entry->public_code
            );

        diagnostic->category =
            vitte_diagnostic_copy_text(
                diagnostic->category_storage,
                sizeof(diagnostic->category_storage),
                entry->category
            );

        diagnostic->origin = entry->origin;

        return;
    }

    switch (diagnostic->severity) {
        case VITTE_DIAGNOSTIC_ERROR:
        case VITTE_DIAGNOSTIC_FATAL:
            fallback = "E????";
            break;

        case VITTE_DIAGNOSTIC_WARNING:
            fallback = "W????";
            break;

        case VITTE_DIAGNOSTIC_NOTE:
            fallback = "N????";
            break;

        case VITTE_DIAGNOSTIC_HELP:
            fallback = "H????";
            break;

        default:
            fallback = "E????";
            break;
    }

    if (!vitte_diagnostic_text_present(diagnostic->short_code)) {
        diagnostic->short_code =
            vitte_diagnostic_copy_text(
                diagnostic->short_code_storage,
                sizeof(diagnostic->short_code_storage),
                fallback
            );
    }
}

/* ========================================================================= */
/* Subject/context                                                           */
/* ========================================================================= */

void
vitte_diagnostic_set_subject(
    vitte_diagnostic_t *diagnostic,
    const char *symbol,
    const char *expected_type,
    const char *actual_type
)
{
    if (diagnostic == NULL) {
        return;
    }

    diagnostic->symbol =
        vitte_diagnostic_copy_text(
            diagnostic->symbol_storage,
            sizeof(diagnostic->symbol_storage),
            symbol
        );

    diagnostic->expected_type =
        vitte_diagnostic_copy_text(
            diagnostic->expected_type_storage,
            sizeof(diagnostic->expected_type_storage),
            expected_type
        );

    diagnostic->actual_type =
        vitte_diagnostic_copy_text(
            diagnostic->actual_type_storage,
            sizeof(diagnostic->actual_type_storage),
            actual_type
        );
}

void
vitte_diagnostic_set_context(
    vitte_diagnostic_t *diagnostic,
    const char *package,
    const char *module,
    const char *procedure,
    const char *context
)
{
    if (diagnostic == NULL) {
        return;
    }

    diagnostic->package =
        vitte_diagnostic_copy_text(
            diagnostic->package_storage,
            sizeof(diagnostic->package_storage),
            package
        );

    diagnostic->module =
        vitte_diagnostic_copy_text(
            diagnostic->module_storage,
            sizeof(diagnostic->module_storage),
            module
        );

    diagnostic->procedure =
        vitte_diagnostic_copy_text(
            diagnostic->procedure_storage,
            sizeof(diagnostic->procedure_storage),
            procedure
        );

    diagnostic->context =
        vitte_diagnostic_copy_text(
            diagnostic->context_storage,
            sizeof(diagnostic->context_storage),
            context
        );
}

/* ========================================================================= */
/* Primary span                                                              */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_set_span(
    vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span
)
{
    if (diagnostic == NULL || span == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (!vitte_diagnostic_span_valid(span)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    diagnostic->source_id = span->source_id;

    diagnostic->source_name =
        vitte_diagnostic_copy_text(
            diagnostic->source_name_storage,
            sizeof(diagnostic->source_name_storage),
            span->source_name
        );

    diagnostic->start_offset = span->start_offset;
    diagnostic->end_offset = span->end_offset;

    diagnostic->start_line = span->start_line;
    diagnostic->start_column = span->start_column;
    diagnostic->end_line = span->end_line;
    diagnostic->end_column = span->end_column;

    diagnostic->has_span = true;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Labels                                                                    */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_label(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_label_style_t style,
    const vitte_ast_span_t *span,
    const char *message
)
{
    vitte_diagnostic_label_t *label;

    if (diagnostic == NULL || span == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (!vitte_diagnostic_span_valid(span)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (diagnostic->label_count >= VITTE_DIAGNOSTIC_MAX_LABELS) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    label = &diagnostic->labels[diagnostic->label_count];

    memset(label, 0, sizeof(*label));

    label->style = style;
    label->span = *span;

    label->message =
        vitte_diagnostic_copy_text(
            label->message_storage,
            sizeof(label->message_storage),
            message
        );

    ++diagnostic->label_count;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Related locations                                                         */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_location(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_location_kind_t kind,
    const vitte_ast_span_t *span,
    const char *label_text
)
{
    vitte_diagnostic_location_t *location;

    if (diagnostic == NULL || span == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (!vitte_diagnostic_span_valid(span)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (diagnostic->related_count >= VITTE_DIAGNOSTIC_MAX_RELATED) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    location = &diagnostic->related[diagnostic->related_count];

    memset(location, 0, sizeof(*location));

    location->kind = kind;
    location->span = *span;

    location->label =
        vitte_diagnostic_copy_text(
            location->label_storage,
            sizeof(location->label_storage),
            label_text
        );

    ++diagnostic->related_count;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Causes                                                                    */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_cause(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_cause_t *cause;

    if (diagnostic == NULL ||
        !vitte_diagnostic_text_present(message)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (diagnostic->cause_count >= VITTE_DIAGNOSTIC_MAX_CAUSES) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    cause = &diagnostic->causes[diagnostic->cause_count];

    memset(cause, 0, sizeof(*cause));

    cause->message =
        vitte_diagnostic_copy_text(
            cause->message_storage,
            sizeof(cause->message_storage),
            message
        );

    if (span != NULL && vitte_diagnostic_span_valid(span)) {
        cause->span = *span;
        cause->has_span = true;
    }

    ++diagnostic->cause_count;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Notes/help                                                                */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_annotation(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_annotation_kind_t kind,
    const char *message,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_annotation_t *annotation;

    if (diagnostic == NULL ||
        !vitte_diagnostic_text_present(message)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (diagnostic->annotation_count >= VITTE_DIAGNOSTIC_MAX_ANNOTATIONS) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    annotation =
        &diagnostic->annotations[diagnostic->annotation_count];

    memset(annotation, 0, sizeof(*annotation));

    annotation->kind = kind;

    annotation->message =
        vitte_diagnostic_copy_text(
            annotation->message_storage,
            sizeof(annotation->message_storage),
            message
        );

    if (span != NULL && vitte_diagnostic_span_valid(span)) {
        annotation->span = *span;
        annotation->has_span = true;
    }

    ++diagnostic->annotation_count;

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_add_note(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span
)
{
    return vitte_diagnostic_add_annotation(
        diagnostic,
        VITTE_DIAGNOSTIC_ANNOTATION_NOTE,
        message,
        span
    );
}

vitte_status_t
vitte_diagnostic_add_help(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span
)
{
    return vitte_diagnostic_add_annotation(
        diagnostic,
        VITTE_DIAGNOSTIC_ANNOTATION_HELP,
        message,
        span
    );
}

/* ========================================================================= */
/* Suggestions                                                               */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_suggestion(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability
)
{
    vitte_diagnostic_suggestion_t *suggestion;

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (diagnostic->suggestion_count >=
        VITTE_DIAGNOSTIC_MAX_SUGGESTIONS) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    suggestion =
        &diagnostic->suggestions[diagnostic->suggestion_count];

    memset(suggestion, 0, sizeof(*suggestion));

    suggestion->message =
        vitte_diagnostic_copy_text(
            suggestion->message_storage,
            sizeof(suggestion->message_storage),
            message
        );

    suggestion->applicability = applicability;

    if (span != NULL && vitte_diagnostic_span_valid(span)) {
        suggestion->span = *span;
        suggestion->has_span = true;
    }

    if (replacement != NULL) {
        suggestion->replacement =
            vitte_diagnostic_copy_text(
                suggestion->replacement_storage,
                sizeof(suggestion->replacement_storage),
                replacement
            );

        suggestion->has_replacement = true;
    } else {
        suggestion->replacement =
            suggestion->replacement_storage;
        suggestion->replacement_storage[0] = '\0';
        suggestion->has_replacement = false;
    }

    ++diagnostic->suggestion_count;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Contract                                                                  */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_set_contract(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_contract_kind_t kind,
    const char *expression,
    const char *procedure,
    const char *reason,
    const vitte_ast_span_t *contract_span,
    const vitte_ast_span_t *call_span,
    const vitte_ast_span_t *declaration_span
)
{
    vitte_diagnostic_contract_t *contract;

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    contract = &diagnostic->contract;

    memset(contract, 0, sizeof(*contract));

    contract->kind = kind;

    contract->expression =
        vitte_diagnostic_copy_text(
            contract->expression_storage,
            sizeof(contract->expression_storage),
            expression
        );

    contract->procedure =
        vitte_diagnostic_copy_text(
            contract->procedure_storage,
            sizeof(contract->procedure_storage),
            procedure
        );

    contract->reason =
        vitte_diagnostic_copy_text(
            contract->reason_storage,
            sizeof(contract->reason_storage),
            reason
        );

    if (contract_span != NULL &&
        vitte_diagnostic_span_valid(contract_span)) {
        contract->contract_span = *contract_span;
        contract->has_contract_span = true;
    }

    if (call_span != NULL &&
        vitte_diagnostic_span_valid(call_span)) {
        contract->call_span = *call_span;
        contract->has_call_span = true;
    }

    if (declaration_span != NULL &&
        vitte_diagnostic_span_valid(declaration_span)) {
        contract->declaration_span = *declaration_span;
        contract->has_declaration_span = true;
    }

    diagnostic->has_contract = true;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_diagnostic_fingerprint(
    const vitte_diagnostic_t *diagnostic
)
{
    uint64_t hash;

    if (diagnostic == NULL) {
        return UINT64_C(0);
    }

    hash = VITTE_DIAGNOSTIC_FNV_OFFSET;

    hash = vitte_diagnostic_hash_text(hash, diagnostic->code);
    hash = vitte_diagnostic_hash_text(hash, diagnostic->short_code);

    hash = vitte_diagnostic_hash_u64(
        hash,
        (uint64_t)diagnostic->origin
    );

    hash = vitte_diagnostic_hash_u64(
        hash,
        (uint64_t)diagnostic->severity
    );

    if (vitte_diagnostic_source_id_valid(diagnostic->source_id)) {
        hash = vitte_diagnostic_hash_u64(
            hash,
            (uint64_t)diagnostic->source_id
        );
    } else {
        hash = vitte_diagnostic_hash_text(
            hash,
            diagnostic->source_name
        );
    }

    hash = vitte_diagnostic_hash_size(
        hash,
        diagnostic->start_offset
    );

    hash = vitte_diagnostic_hash_size(
        hash,
        diagnostic->end_offset
    );

    hash = vitte_diagnostic_hash_text(
        hash,
        diagnostic->message
    );

    return hash;
}

void
vitte_diagnostic_refresh_fingerprint(
    vitte_diagnostic_t *diagnostic
)
{
    if (diagnostic == NULL) {
        return;
    }

    diagnostic->fingerprint =
        vitte_diagnostic_fingerprint(diagnostic);
}

/* ========================================================================= */
/* Equality for dedup                                                        */
/* ========================================================================= */

static bool
vitte_diagnostic_same_identity(
    const vitte_diagnostic_t *left,
    const vitte_diagnostic_t *right
)
{
    if (left == NULL || right == NULL) {
        return false;
    }

    if (left->severity != right->severity ||
        left->origin != right->origin) {
        return false;
    }

    if (!vitte_diagnostic_text_equal(
            left->short_code,
            right->short_code)) {
        return false;
    }

    if (!vitte_diagnostic_text_equal(
            left->message,
            right->message)) {
        return false;
    }

    if (left->has_span != right->has_span) {
        return false;
    }

    if (!left->has_span) {
        return true;
    }

    if (!vitte_diagnostic_same_source(
            left->source_id,
            left->source_name,
            right->source_id,
            right->source_name)) {
        return false;
    }

    return left->start_offset == right->start_offset &&
           left->end_offset == right->end_offset;
}

/* ========================================================================= */
/* Bag                                                                       */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_bag_init(
    vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_t *storage,
    size_t capacity
)
{
    if (bag == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(bag, 0, sizeof(*bag));

    if (capacity != 0u && storage == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    bag->storage = storage;
    bag->capacity = capacity;
    bag->count = 0u;

    vitte_diagnostic_counts_init(&bag->counts);
    vitte_diagnostic_options_init(&bag->options);

    vitte_error_init(&bag->last_error);
    bag->initialized = true;

    return VITTE_STATUS_OK;
}

void
vitte_diagnostic_bag_reset(
    vitte_diagnostic_bag_t *bag
)
{
    if (bag == NULL) {
        return;
    }

    bag->count = 0u;

    vitte_diagnostic_counts_init(&bag->counts);

    vitte_error_reset(&bag->last_error);
}

bool
vitte_diagnostic_bag_is_initialized(
    const vitte_diagnostic_bag_t *bag
)
{
    return bag != NULL && bag->initialized;
}

size_t
vitte_diagnostic_bag_count(
    const vitte_diagnostic_bag_t *bag
)
{
    if (bag == NULL || !bag->initialized) {
        return 0u;
    }

    return bag->count;
}

bool
vitte_diagnostic_bag_empty(
    const vitte_diagnostic_bag_t *bag
)
{
    return vitte_diagnostic_bag_count(bag) == 0u;
}

const vitte_diagnostic_t *
vitte_diagnostic_at(
    const vitte_diagnostic_bag_t *bag,
    size_t index
)
{
    if (bag == NULL ||
        !bag->initialized ||
        index >= bag->count) {
        return NULL;
    }

    return &bag->storage[index];
}

vitte_diagnostic_t *
vitte_diagnostic_at_mut(
    vitte_diagnostic_bag_t *bag,
    size_t index
)
{
    if (bag == NULL ||
        !bag->initialized ||
        index >= bag->count) {
        return NULL;
    }

    return &bag->storage[index];
}

/* ========================================================================= */
/* Dedup                                                                     */
/* ========================================================================= */

static size_t
vitte_diagnostic_find_duplicate(
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (bag == NULL || diagnostic == NULL) {
        return VITTE_DIAGNOSTIC_INDEX_NONE;
    }

    for (index = 0u; index < bag->count; ++index) {
        const vitte_diagnostic_t *candidate;

        candidate = &bag->storage[index];

        if (candidate->fingerprint != diagnostic->fingerprint) {
            continue;
        }

        if (vitte_diagnostic_same_identity(
                candidate,
                diagnostic)) {
            return index;
        }
    }

    return VITTE_DIAGNOSTIC_INDEX_NONE;
}

/* ========================================================================= */
/* Add                                                                       */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add(
    vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_t *source
)
{
    vitte_diagnostic_t copy;
    size_t duplicate;

    if (bag == NULL ||
        source == NULL ||
        !bag->initialized) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (bag->count >= bag->capacity) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    copy = *source;

    vitte_diagnostic_rebind(&copy);
    vitte_diagnostic_apply_registry_metadata(&copy);
    vitte_diagnostic_refresh_fingerprint(&copy);

    duplicate = vitte_diagnostic_find_duplicate(
        bag,
        &copy
    );

    if (duplicate != VITTE_DIAGNOSTIC_INDEX_NONE) {
        return VITTE_STATUS_OK;
    }

    bag->storage[bag->count] = copy;

    vitte_diagnostic_rebind(
        &bag->storage[bag->count]
    );

    vitte_diagnostic_counts_increment(
        &bag->counts,
        bag->storage[bag->count].severity
    );

    ++bag->count;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Convenience emission                                                      */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_emit(
    vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_severity_t severity,
    vitte_diagnostic_origin_t origin,
    const char *code,
    const char *message,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_t diagnostic;
    vitte_status_t status;

    if (bag == NULL ||
        !vitte_diagnostic_text_present(message)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_init(&diagnostic);

    diagnostic.severity = severity;
    diagnostic.origin = origin;

    diagnostic.code =
        vitte_diagnostic_copy_text(
            diagnostic.code_storage,
            sizeof(diagnostic.code_storage),
            code
        );

    diagnostic.message =
        vitte_diagnostic_copy_text(
            diagnostic.message_storage,
            sizeof(diagnostic.message_storage),
            message
        );

    if (span != NULL) {
        status = vitte_diagnostic_set_span(
            &diagnostic,
            span
        );

        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    vitte_diagnostic_apply_registry_metadata(
        &diagnostic
    );

    vitte_diagnostic_refresh_fingerprint(
        &diagnostic
    );

    return vitte_diagnostic_add(
        bag,
        &diagnostic
    );
}

/* ========================================================================= */
/* Cascade relationships                                                     */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_set_parent(
    vitte_diagnostic_bag_t *bag,
    size_t diagnostic_index,
    size_t parent_index
)
{
    vitte_diagnostic_t *diagnostic;
    size_t root;

    if (bag == NULL ||
        diagnostic_index >= bag->count ||
        parent_index >= bag->count ||
        diagnostic_index == parent_index) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (parent_index >= diagnostic_index) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    diagnostic = &bag->storage[diagnostic_index];

    diagnostic->parent_diagnostic = parent_index;

    root = bag->storage[parent_index].root_diagnostic;

    if (root == VITTE_DIAGNOSTIC_INDEX_NONE) {
        root = parent_index;
    }

    diagnostic->root_diagnostic = root;
    diagnostic->is_primary = false;
    diagnostic->is_cascade = true;

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_mark_cascade(
    vitte_diagnostic_bag_t *bag,
    size_t diagnostic_index,
    size_t root_index
)
{
    vitte_diagnostic_t *diagnostic;

    if (bag == NULL ||
        diagnostic_index >= bag->count ||
        root_index >= bag->count ||
        diagnostic_index == root_index ||
        root_index >= diagnostic_index) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    diagnostic = &bag->storage[diagnostic_index];

    diagnostic->root_diagnostic = root_index;
    diagnostic->parent_diagnostic = root_index;
    diagnostic->is_primary = false;
    diagnostic->is_cascade = true;

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_suppress(
    vitte_diagnostic_bag_t *bag,
    size_t index
)
{
    vitte_diagnostic_t *diagnostic;

    if (bag == NULL || index >= bag->count) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    diagnostic = &bag->storage[index];

    if (!diagnostic->is_suppressed) {
        diagnostic->is_suppressed = true;
        ++bag->counts.suppressed_count;
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Query                                                                     */
/* ========================================================================= */

bool
vitte_diagnostic_has_errors(
    const vitte_diagnostic_bag_t *bag
)
{
    if (bag == NULL) {
        return false;
    }

    return bag->counts.error_count != 0u ||
           bag->counts.fatal_count != 0u;
}

bool
vitte_diagnostic_has_fatal(
    const vitte_diagnostic_bag_t *bag
)
{
    return bag != NULL &&
           bag->counts.fatal_count != 0u;
}

bool
vitte_diagnostic_has_warnings(
    const vitte_diagnostic_bag_t *bag
)
{
    return bag != NULL &&
           bag->counts.warning_count != 0u;
}

/* ========================================================================= */
/* Compilation status                                                        */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_compilation_status(
    const vitte_diagnostic_bag_t *bag
)
{
    if (bag == NULL || !bag->initialized) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (vitte_diagnostic_has_errors(bag)) {
        /*
         * The current Vitte status enum has no dedicated
         * VITTE_STATUS_ERROR_COMPILATION value.
         */
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Merge                                                                     */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_merge(
    vitte_diagnostic_bag_t *destination,
    const vitte_diagnostic_bag_t *source
)
{
    size_t index;
    vitte_status_t status;

    if (destination == NULL ||
        source == NULL ||
        !destination->initialized ||
        !source->initialized) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (index = 0u; index < source->count; ++index) {
        status = vitte_diagnostic_add(
            destination,
            &source->storage[index]
        );

        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Simple terminal formatter                                                 */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_format_one(
    const vitte_diagnostic_t *diagnostic,
    char *buffer,
    size_t capacity
)
{
    int written;
    const char *severity;
    const char *code;
    const char *message;

    if (diagnostic == NULL ||
        buffer == NULL ||
        capacity == 0u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    severity =
        vitte_diagnostic_severity_name(
            diagnostic->severity
        );

    code =
        vitte_diagnostic_text_present(
            diagnostic->short_code
        )
            ? diagnostic->short_code
            : "E????";

    message =
        vitte_diagnostic_text_present(
            diagnostic->message
        )
            ? diagnostic->message
            : "diagnostic";

    if (diagnostic->has_span &&
        vitte_diagnostic_text_present(
            diagnostic->source_name)) {

        written = snprintf(
            buffer,
            capacity,
            "%s:%zu:%zu: %s[%s]: %s",
            diagnostic->source_name,
            diagnostic->start_line + 1u,
            diagnostic->start_column + 1u,
            severity,
            code,
            message
        );
    } else {
        written = snprintf(
            buffer,
            capacity,
            "%s[%s]: %s",
            severity,
            code,
            message
        );
    }

    if (written < 0 ||
        (size_t)written >= capacity) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_format_one_ex(
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_options_t *options,
    char *buffer,
    size_t capacity
)
{
    int written;
    const char *severity;
    const char *code;
    const char *origin;
    const char *category;

    if (diagnostic == NULL ||
        buffer == NULL ||
        capacity == 0u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (options == NULL) {
        return vitte_diagnostic_format_one(
            diagnostic,
            buffer,
            capacity
        );
    }

    severity =
        vitte_diagnostic_severity_name(
            diagnostic->severity
        );

    code =
        vitte_diagnostic_text_present(
            diagnostic->short_code)
            ? diagnostic->short_code
            : "E????";

    origin =
        vitte_diagnostic_origin_name(
            diagnostic->origin
        );

    category =
        vitte_diagnostic_text_present(
            diagnostic->category)
            ? diagnostic->category
            : "general";

    if (options->show_codes &&
        options->show_phase) {
        written = snprintf(
            buffer,
            capacity,
            "%s[%s] (%s/%s): %s",
            severity,
            code,
            origin,
            category,
            diagnostic->message
        );
    } else if (options->show_codes) {
        written = snprintf(
            buffer,
            capacity,
            "%s[%s]: %s",
            severity,
            code,
            diagnostic->message
        );
    } else {
        written = snprintf(
            buffer,
            capacity,
            "%s: %s",
            severity,
            diagnostic->message
        );
    }

    if (written < 0 ||
        (size_t)written >= capacity) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Summary                                                                   */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_format_summary(
    const vitte_diagnostic_bag_t *bag,
    char *buffer,
    size_t capacity
)
{
    int written;

    if (bag == NULL ||
        buffer == NULL ||
        capacity == 0u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    written = snprintf(
        buffer,
        capacity,
        "%zu error(s), %zu fatal error(s), "
        "%zu warning(s), %zu note(s), "
        "%zu help message(s), %zu suppressed",
        bag->counts.error_count,
        bag->counts.fatal_count,
        bag->counts.warning_count,
        bag->counts.note_count,
        bag->counts.help_count,
        bag->counts.suppressed_count
    );

    if (written < 0 ||
        (size_t)written >= capacity) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Stream output                                                             */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_write_one(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
)
{
    char buffer[8192];
    vitte_status_t status;

    if (stream == NULL || diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_diagnostic_format_one(
        diagnostic,
        buffer,
        sizeof(buffer)
    );

    if (status != VITTE_STATUS_OK) {
        return status;
    }

    if (fprintf(stream, "%s\n", buffer) < 0) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_write_all(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag
)
{
    size_t index;
    vitte_status_t status;

    if (stream == NULL || bag == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (index = 0u; index < bag->count; ++index) {
        const vitte_diagnostic_t *diagnostic;

        diagnostic = &bag->storage[index];

        if (diagnostic->is_suppressed) {
            continue;
        }

        status = vitte_diagnostic_write_one(
            stream,
            diagnostic
        );

        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Generic format dispatch                                                   */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_write_all_format(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_format_t format
)
{
    if (stream == NULL || bag == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    /*
     * Rich renderers live in dedicated modules:
     *
     *   render_terminal.c
     *   render_json.c
     *   render_sarif.c
     *   render_lsp.c
     *
     * diagnostic.c deliberately remains independent of their private
     * option structures.
     */
    switch (format) {
        case VITTE_DIAGNOSTIC_FORMAT_TERMINAL:
            return vitte_diagnostic_write_all(
                stream,
                bag
            );

        case VITTE_DIAGNOSTIC_FORMAT_JSON:
        case VITTE_DIAGNOSTIC_FORMAT_SARIF:
        case VITTE_DIAGNOSTIC_FORMAT_LSP:
            /*
             * Dedicated renderer should be selected by the driver.
             * Falling back to structured human output is preferable to
             * silently emitting malformed JSON/SARIF/LSP.
             */
            return vitte_diagnostic_write_all(
                stream,
                bag
            );

        default:
            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
}

/* ========================================================================= */
/* Failure conversion                                                        */
/* ========================================================================= */

vitte_error_t
vitte_diagnostic_last_error(
    const vitte_diagnostic_bag_t *bag
)
{
    if (bag == NULL) {
        vitte_error_t empty;
        vitte_error_init(&empty);
        return empty;
    }

    return bag->last_error;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

size_t
vitte_diagnostic_error_count(
    const vitte_diagnostic_bag_t *bag
)
{
    if (bag == NULL) {
        return 0u;
    }

    return bag->counts.error_count +
           bag->counts.fatal_count;
}

size_t
vitte_diagnostic_warning_count(
    const vitte_diagnostic_bag_t *bag
)
{
    return bag != NULL
        ? bag->counts.warning_count
        : 0u;
}

size_t
vitte_diagnostic_suppressed_count(
    const vitte_diagnostic_bag_t *bag
)
{
    return bag != NULL
        ? bag->counts.suppressed_count
        : 0u;
}

/* ========================================================================= */
/* Component metadata                                                        */
/* ========================================================================= */

const char *
vitte_diagnostic_component_name(void)
{
    return "diagnostic";
}

uint32_t
vitte_diagnostic_component_version(void)
{
    return UINT32_C(1);
}

/* ========================================================================= */
/* Invariant validation                                                      */
/* ========================================================================= */

bool
vitte_diagnostic_validate(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return false;
    }

    if (!vitte_diagnostic_text_present(
            diagnostic->message)) {
        return false;
    }

    if (diagnostic->has_span) {
        if (diagnostic->end_offset <
            diagnostic->start_offset) {
            return false;
        }

        if (!vitte_diagnostic_source_id_valid(
                diagnostic->source_id) &&
            !vitte_diagnostic_text_present(
                diagnostic->source_name)) {
            return false;
        }
    }

    if (diagnostic->label_count >
        VITTE_DIAGNOSTIC_MAX_LABELS) {
        return false;
    }

    if (diagnostic->related_count >
        VITTE_DIAGNOSTIC_MAX_RELATED) {
        return false;
    }

    if (diagnostic->cause_count >
        VITTE_DIAGNOSTIC_MAX_CAUSES) {
        return false;
    }

    if (diagnostic->annotation_count >
        VITTE_DIAGNOSTIC_MAX_ANNOTATIONS) {
        return false;
    }

    if (diagnostic->suggestion_count >
        VITTE_DIAGNOSTIC_MAX_SUGGESTIONS) {
        return false;
    }

    for (index = 0u;
         index < diagnostic->label_count;
         ++index) {

        if (!vitte_diagnostic_span_valid(
                &diagnostic->labels[index].span)) {
            return false;
        }
    }

    for (index = 0u;
         index < diagnostic->related_count;
         ++index) {

        if (!vitte_diagnostic_span_valid(
                &diagnostic->related[index].span)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Bag validation                                                            */
/* ========================================================================= */

bool
vitte_diagnostic_bag_validate(
    const vitte_diagnostic_bag_t *bag
)
{
    size_t index;

    if (bag == NULL || !bag->initialized) {
        return false;
    }

    if (bag->count > bag->capacity) {
        return false;
    }

    if (bag->capacity != 0u &&
        bag->storage == NULL) {
        return false;
    }

    for (index = 0u; index < bag->count; ++index) {
        const vitte_diagnostic_t *diagnostic;

        diagnostic = &bag->storage[index];

        if (!vitte_diagnostic_validate(diagnostic)) {
            return false;
        }

        if (diagnostic->parent_diagnostic !=
                VITTE_DIAGNOSTIC_INDEX_NONE &&
            diagnostic->parent_diagnostic >= index) {
            return false;
        }

        if (diagnostic->root_diagnostic !=
                VITTE_DIAGNOSTIC_INDEX_NONE &&
            diagnostic->root_diagnostic >= index) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
