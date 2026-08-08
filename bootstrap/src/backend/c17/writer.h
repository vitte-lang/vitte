#ifndef VITTE_BOOTSTRAP_BACKEND_C17_WRITER_H
#define VITTE_BOOTSTRAP_BACKEND_C17_WRITER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "../../api/error.h"
#include "options.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_c17_writer {
    char *buffer;
    size_t capacity;
    size_t length;
    FILE *stream;
    size_t indent_level;
    size_t indent_width;
    size_t line_count;
    size_t byte_count;
    const char *newline;
    bool at_line_start;
    bool failed;
    vitte_error_t last_error;
} vitte_c17_writer_t;

vitte_status_t vitte_c17_writer_init_buffer(
    vitte_c17_writer_t *writer,
    char *buffer,
    size_t capacity,
    const vitte_c17_options_t *options
);

vitte_status_t vitte_c17_writer_init_file(
    vitte_c17_writer_t *writer,
    FILE *stream,
    const vitte_c17_options_t *options
);

void vitte_c17_writer_reset(vitte_c17_writer_t *writer);
const vitte_error_t *vitte_c17_writer_last_error(const vitte_c17_writer_t *writer);

vitte_status_t vitte_c17_write_char(vitte_c17_writer_t *writer, char value);
vitte_status_t vitte_c17_write_string(vitte_c17_writer_t *writer, const char *text);
vitte_status_t vitte_c17_write_format(vitte_c17_writer_t *writer, const char *format, ...);
vitte_status_t vitte_c17_write_newline(vitte_c17_writer_t *writer);
vitte_status_t vitte_c17_write_indent(vitte_c17_writer_t *writer);
vitte_status_t vitte_c17_write_open_block(vitte_c17_writer_t *writer);
vitte_status_t vitte_c17_write_close_block(vitte_c17_writer_t *writer);
vitte_status_t vitte_c17_writer_flush(vitte_c17_writer_t *writer);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_BACKEND_C17_WRITER_H */
