#ifndef VITTE_DIAGNOSTIC_CLI_BRIDGE_H
#define VITTE_DIAGNOSTIC_CLI_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../source/source.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vitte_cli_diagnostic_severity {
    VITTE_CLI_DIAGNOSTIC_NOTE = 0,
    VITTE_CLI_DIAGNOSTIC_HELP,
    VITTE_CLI_DIAGNOSTIC_WARNING,
    VITTE_CLI_DIAGNOSTIC_ERROR,
    VITTE_CLI_DIAGNOSTIC_FATAL
} vitte_cli_diagnostic_severity_t;

typedef enum vitte_cli_diagnostic_format {
    VITTE_CLI_DIAGNOSTIC_TERMINAL = 0,
    VITTE_CLI_DIAGNOSTIC_JSON,
    VITTE_CLI_DIAGNOSTIC_SARIF
} vitte_cli_diagnostic_format_t;

typedef struct vitte_cli_diagnostic {
    vitte_cli_diagnostic_severity_t severity;
    const char *code;
    const char *message;
    const char *details;
    char details_storage[256];
    bool has_span;
    uint32_t file_id;
    size_t start_offset;
    size_t end_offset;
    size_t start_line;
    size_t start_column;
    size_t end_line;
    size_t end_column;
    const char *procedure;
    const char *context;
} vitte_cli_diagnostic_t;

bool
vitte_diagnostic_render_cli_batch(
    FILE *stream,
    const vitte_source_context_t *sources,
    vitte_source_id_t source_id,
    const char *source_name,
    const vitte_cli_diagnostic_t *diagnostics,
    size_t diagnostic_count,
    bool color,
    vitte_cli_diagnostic_format_t format);

bool
vitte_diagnostic_render_cli(
    FILE *stream,
    const vitte_source_context_t *sources,
    vitte_source_id_t source_id,
    const char *source_name,
    vitte_cli_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    bool has_span,
    uint32_t file_id,
    size_t start_offset,
    size_t end_offset,
    size_t start_line,
    size_t start_column,
    size_t end_line,
    size_t end_column,
    const char *procedure,
    const char *context,
    bool color,
    vitte_cli_diagnostic_format_t format);

#ifdef __cplusplus
}
#endif

#endif
