#include "diagnostic.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const size_t VITTE_DIAGNOSTIC_DEFAULT_MAX = 100u;

static void vitte_diagnostic_set_error(
    vitte_diagnostic_bag_t *bag,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (bag != NULL) {
        vitte_error_set_details(&bag->last_error, status, code, message, details);
    }
}

void vitte_diagnostic_options_init(vitte_diagnostic_options_t *options) {
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->max_diagnostics = VITTE_DIAGNOSTIC_DEFAULT_MAX;
    options->show_codes = true;
    options->show_details = true;
}

void vitte_diagnostic_counts_init(vitte_diagnostic_counts_t *counts) {
    if (counts != NULL) {
        memset(counts, 0, sizeof(*counts));
    }
}

void vitte_diagnostic_init(vitte_diagnostic_t *diagnostic) {
    if (diagnostic == NULL) {
        return;
    }

    memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->severity = VITTE_DIAGNOSTIC_ERROR;
}

bool vitte_diagnostic_severity_is_valid(vitte_diagnostic_severity_t severity) {
    return severity == VITTE_DIAGNOSTIC_NOTE ||
        severity == VITTE_DIAGNOSTIC_HELP ||
        severity == VITTE_DIAGNOSTIC_WARNING ||
        severity == VITTE_DIAGNOSTIC_ERROR ||
        severity == VITTE_DIAGNOSTIC_FATAL;
}

bool vitte_diagnostic_severity_is_error(vitte_diagnostic_severity_t severity) {
    return severity == VITTE_DIAGNOSTIC_ERROR || severity == VITTE_DIAGNOSTIC_FATAL;
}

const char *vitte_diagnostic_severity_name(vitte_diagnostic_severity_t severity) {
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

vitte_status_t vitte_diagnostic_bag_init(
    vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_t *storage,
    size_t capacity,
    const vitte_diagnostic_options_t *options
) {
    vitte_diagnostic_options_t defaults;

    if (bag == NULL || storage == NULL || capacity == 0u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(bag, 0, sizeof(*bag));
    bag->storage = storage;
    bag->capacity = capacity;
    bag->options = options != NULL ? *options : (vitte_diagnostic_options_init(&defaults), defaults);
    if (bag->options.max_diagnostics == 0u) {
        bag->options.max_diagnostics = VITTE_DIAGNOSTIC_DEFAULT_MAX;
    }
    vitte_error_init(&bag->last_error);
    bag->initialized = true;
    vitte_diagnostic_bag_reset(bag);
    return VITTE_STATUS_OK;
}

void vitte_diagnostic_bag_reset(vitte_diagnostic_bag_t *bag) {
    size_t index;

    if (bag == NULL || bag->storage == NULL) {
        return;
    }

    for (index = 0u; index < bag->capacity; index++) {
        vitte_diagnostic_init(&bag->storage[index]);
    }
    bag->count = 0u;
    vitte_diagnostic_counts_init(&bag->counts);
    vitte_error_reset(&bag->last_error);
}

bool vitte_diagnostic_bag_is_initialized(const vitte_diagnostic_bag_t *bag) {
    return bag != NULL && bag->initialized && bag->storage != NULL && bag->capacity > 0u;
}

const vitte_error_t *vitte_diagnostic_bag_last_error(const vitte_diagnostic_bag_t *bag) {
    return bag != NULL ? &bag->last_error : vitte_error_last();
}

const vitte_diagnostic_counts_t *vitte_diagnostic_bag_counts(const vitte_diagnostic_bag_t *bag) {
    return bag != NULL ? &bag->counts : NULL;
}

const vitte_diagnostic_t *vitte_diagnostic_at(const vitte_diagnostic_bag_t *bag, size_t index) {
    if (!vitte_diagnostic_bag_is_initialized(bag) || index >= bag->count) {
        return NULL;
    }
    return &bag->storage[index];
}

static void vitte_diagnostic_count(vitte_diagnostic_bag_t *bag, vitte_diagnostic_severity_t severity) {
    switch (severity) {
        case VITTE_DIAGNOSTIC_NOTE:
            bag->counts.note_count++;
            break;
        case VITTE_DIAGNOSTIC_HELP:
            bag->counts.help_count++;
            break;
        case VITTE_DIAGNOSTIC_WARNING:
            bag->counts.warning_count++;
            break;
        case VITTE_DIAGNOSTIC_ERROR:
            bag->counts.error_count++;
            break;
        case VITTE_DIAGNOSTIC_FATAL:
            bag->counts.fatal_count++;
            break;
        default:
            break;
    }
}

static bool vitte_diagnostic_text_is_valid(const char *text) {
    return text != NULL && text[0] != '\0';
}

static void vitte_diagnostic_copy_span(vitte_diagnostic_t *diagnostic, const vitte_ast_span_t *span) {
    if (diagnostic == NULL || span == NULL || !vitte_ast_span_is_valid(span)) {
        return;
    }

    diagnostic->source_name = span->source_name;
    diagnostic->start_offset = span->start_offset;
    diagnostic->end_offset = span->end_offset;
    diagnostic->start_line = span->start_line;
    diagnostic->start_column = span->start_column;
    diagnostic->end_line = span->end_line;
    diagnostic->end_column = span->end_column;
    diagnostic->has_span = true;
}

vitte_status_t vitte_diagnostic_add(
    vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
) {
    vitte_diagnostic_t *diagnostic;

    if (!vitte_diagnostic_bag_is_initialized(bag)) {
        vitte_diagnostic_set_error(bag, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_DIAG_E_STATE", "diagnostic bag is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (!vitte_diagnostic_severity_is_valid(severity) ||
        !vitte_diagnostic_text_is_valid(code) ||
        !vitte_diagnostic_text_is_valid(message)) {
        vitte_diagnostic_set_error(bag, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_DIAG_E_ARGUMENT", "invalid diagnostic arguments", code);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (severity == VITTE_DIAGNOSTIC_WARNING && bag->options.warnings_as_errors) {
        severity = VITTE_DIAGNOSTIC_ERROR;
    }
    if (bag->count >= bag->capacity || bag->count >= bag->options.max_diagnostics) {
        bag->counts.suppressed_count++;
        vitte_diagnostic_set_error(bag, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_DIAG_E_LIMIT", "diagnostic limit reached", code);
        return VITTE_STATUS_ERROR_UNSUPPORTED;
    }

    diagnostic = &bag->storage[bag->count];
    vitte_diagnostic_init(diagnostic);
    diagnostic->severity = severity;
    diagnostic->code = code;
    diagnostic->message = message;
    diagnostic->details = details;
    vitte_diagnostic_copy_span(diagnostic, span);
    bag->count++;
    vitte_diagnostic_count(bag, severity);
    vitte_error_reset(&bag->last_error);
    return VITTE_STATUS_OK;
}

bool vitte_diagnostic_has_errors(const vitte_diagnostic_bag_t *bag) {
    return bag != NULL && (bag->counts.error_count > 0u || bag->counts.fatal_count > 0u);
}

static vitte_status_t vitte_diagnostic_append(
    char *buffer,
    size_t capacity,
    size_t *used,
    const char *format,
    ...
) {
    va_list args;
    int written;

    if (buffer == NULL || used == NULL || format == NULL || *used >= capacity) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    va_start(args, format);
    written = vsnprintf(buffer + *used, capacity - *used, format, args);
    va_end(args);
    if (written < 0) {
        return VITTE_STATUS_ERROR_INTERNAL;
    }
    if ((size_t)written >= capacity - *used) {
        *used = capacity > 0u ? capacity - 1u : 0u;
        if (capacity > 0u) {
            buffer[*used] = '\0';
        }
        return VITTE_STATUS_ERROR_BACKEND;
    }

    *used += (size_t)written;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_diagnostic_format_one(
    const vitte_diagnostic_t *diagnostic,
    char *buffer,
    size_t capacity,
    size_t *written
) {
    size_t used = 0u;
    vitte_status_t status;

    if (written != NULL) {
        *written = 0u;
    }
    if (diagnostic == NULL || buffer == NULL || capacity == 0u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    buffer[0] = '\0';

    status = vitte_diagnostic_append(
        buffer,
        capacity,
        &used,
        "%s[%s]: %s\n",
        vitte_diagnostic_severity_name(diagnostic->severity),
        diagnostic->code != NULL ? diagnostic->code : "VITTE_DIAG",
        diagnostic->message != NULL ? diagnostic->message : ""
    );
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    if (diagnostic->has_span) {
        status = vitte_diagnostic_append(
            buffer,
            capacity,
            &used,
            "  --> %s:%u:%u\n",
            diagnostic->source_name != NULL ? diagnostic->source_name : "<unknown>",
            diagnostic->start_line,
            diagnostic->start_column
        );
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    if (diagnostic->details != NULL && diagnostic->details[0] != '\0') {
        status = vitte_diagnostic_append(buffer, capacity, &used, "  = %s\n", diagnostic->details);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    if (written != NULL) {
        *written = used;
    }
    return VITTE_STATUS_OK;
}

static const char *vitte_diagnostic_color(vitte_diagnostic_severity_t severity) {
    switch (severity) {
        case VITTE_DIAGNOSTIC_WARNING:
            return "\033[33m";
        case VITTE_DIAGNOSTIC_ERROR:
        case VITTE_DIAGNOSTIC_FATAL:
            return "\033[31m";
        case VITTE_DIAGNOSTIC_NOTE:
        case VITTE_DIAGNOSTIC_HELP:
            return "\033[36m";
        default:
            return "";
    }
}

vitte_status_t vitte_diagnostic_write_one(
    const vitte_diagnostic_t *diagnostic,
    FILE *stream,
    const vitte_diagnostic_options_t *options
) {
    char buffer[1024];
    vitte_status_t status;

    if (diagnostic == NULL || stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (options != NULL && options->color_enabled) {
        if (fputs(vitte_diagnostic_color(diagnostic->severity), stream) == EOF) {
            return VITTE_STATUS_ERROR_IO;
        }
    }
    status = vitte_diagnostic_format_one(diagnostic, buffer, sizeof(buffer), NULL);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    if (fputs(buffer, stream) == EOF) {
        return VITTE_STATUS_ERROR_IO;
    }
    if (options != NULL && options->color_enabled) {
        if (fputs("\033[0m", stream) == EOF) {
            return VITTE_STATUS_ERROR_IO;
        }
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_diagnostic_write_all(
    const vitte_diagnostic_bag_t *bag,
    FILE *stream
) {
    size_t index;

    if (!vitte_diagnostic_bag_is_initialized(bag) || stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (index = 0u; index < bag->count; index++) {
        vitte_status_t status = vitte_diagnostic_write_one(&bag->storage[index], stream, &bag->options);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    return VITTE_STATUS_OK;
}
