#ifndef VITTE_BOOTSTRAP_DIAGNOSTIC_H
#define VITTE_BOOTSTRAP_DIAGNOSTIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "../api/error.h"
#include "../ast/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vitte_diagnostic_severity {
    VITTE_DIAGNOSTIC_NOTE = 0,
    VITTE_DIAGNOSTIC_HELP,
    VITTE_DIAGNOSTIC_WARNING,
    VITTE_DIAGNOSTIC_ERROR,
    VITTE_DIAGNOSTIC_FATAL
} vitte_diagnostic_severity_t;

typedef struct vitte_diagnostic_options {
    size_t max_diagnostics;
    bool warnings_as_errors;
    bool color_enabled;
    bool show_source_line;
    bool show_codes;
    bool show_details;
} vitte_diagnostic_options_t;

typedef struct vitte_diagnostic_counts {
    size_t note_count;
    size_t help_count;
    size_t warning_count;
    size_t error_count;
    size_t fatal_count;
    size_t suppressed_count;
} vitte_diagnostic_counts_t;

typedef struct vitte_diagnostic {
    vitte_diagnostic_severity_t severity;
    const char *code;
    const char *message;
    const char *details;
    const char *source_name;
    size_t start_offset;
    size_t end_offset;
    unsigned start_line;
    unsigned start_column;
    unsigned end_line;
    unsigned end_column;
    bool has_span;
} vitte_diagnostic_t;

typedef struct vitte_diagnostic_bag {
    bool initialized;
    vitte_diagnostic_t *storage;
    size_t capacity;
    size_t count;
    vitte_diagnostic_counts_t counts;
    vitte_diagnostic_options_t options;
    vitte_error_t last_error;
} vitte_diagnostic_bag_t;

void vitte_diagnostic_options_init(vitte_diagnostic_options_t *options);
void vitte_diagnostic_counts_init(vitte_diagnostic_counts_t *counts);
void vitte_diagnostic_init(vitte_diagnostic_t *diagnostic);

vitte_status_t vitte_diagnostic_bag_init(
    vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_t *storage,
    size_t capacity,
    const vitte_diagnostic_options_t *options
);

void vitte_diagnostic_bag_reset(vitte_diagnostic_bag_t *bag);
bool vitte_diagnostic_bag_is_initialized(const vitte_diagnostic_bag_t *bag);
const vitte_error_t *vitte_diagnostic_bag_last_error(const vitte_diagnostic_bag_t *bag);
const vitte_diagnostic_counts_t *vitte_diagnostic_bag_counts(const vitte_diagnostic_bag_t *bag);
const vitte_diagnostic_t *vitte_diagnostic_at(const vitte_diagnostic_bag_t *bag, size_t index);

vitte_status_t vitte_diagnostic_add(
    vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
);

bool vitte_diagnostic_has_errors(const vitte_diagnostic_bag_t *bag);
const char *vitte_diagnostic_severity_name(vitte_diagnostic_severity_t severity);
bool vitte_diagnostic_severity_is_error(vitte_diagnostic_severity_t severity);
bool vitte_diagnostic_severity_is_valid(vitte_diagnostic_severity_t severity);

vitte_status_t vitte_diagnostic_format_one(
    const vitte_diagnostic_t *diagnostic,
    char *buffer,
    size_t capacity,
    size_t *written
);

vitte_status_t vitte_diagnostic_write_one(
    const vitte_diagnostic_t *diagnostic,
    FILE *stream,
    const vitte_diagnostic_options_t *options
);

vitte_status_t vitte_diagnostic_write_all(
    const vitte_diagnostic_bag_t *bag,
    FILE *stream
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_DIAGNOSTIC_H */
