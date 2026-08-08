#include "writer.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void vitte_c17_writer_set_error(
    vitte_c17_writer_t *writer,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (writer != NULL) {
        writer->failed = true;
        vitte_error_set_details(&writer->last_error, status, code, message, details);
    }
}

static vitte_status_t vitte_c17_writer_init_common(
    vitte_c17_writer_t *writer,
    const vitte_c17_options_t *options
) {
    vitte_c17_options_t defaults;
    const vitte_c17_options_t *effective_options = options;
    vitte_status_t status;

    if (writer == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (effective_options == NULL) {
        vitte_c17_options_init(&defaults);
        effective_options = &defaults;
    }

    status = vitte_c17_options_validate(effective_options, &writer->last_error);
    if (status != VITTE_STATUS_OK) {
        writer->failed = true;
        return status;
    }

    writer->indent_width = effective_options->indent_width;
    writer->newline = vitte_c17_options_newline_text(effective_options->newline);
    writer->at_line_start = true;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_c17_writer_init_buffer(
    vitte_c17_writer_t *writer,
    char *buffer,
    size_t capacity,
    const vitte_c17_options_t *options
) {
    vitte_status_t status;

    if (writer == NULL || buffer == NULL || capacity == 0u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(writer, 0, sizeof(*writer));
    vitte_error_init(&writer->last_error);
    writer->buffer = buffer;
    writer->capacity = capacity;
    writer->buffer[0] = '\0';
    status = vitte_c17_writer_init_common(writer, options);
    if (status != VITTE_STATUS_OK) {
        writer->buffer[0] = '\0';
    }
    return status;
}

vitte_status_t vitte_c17_writer_init_file(
    vitte_c17_writer_t *writer,
    FILE *stream,
    const vitte_c17_options_t *options
) {
    if (writer == NULL || stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(writer, 0, sizeof(*writer));
    vitte_error_init(&writer->last_error);
    writer->stream = stream;
    return vitte_c17_writer_init_common(writer, options);
}

void vitte_c17_writer_reset(vitte_c17_writer_t *writer) {
    if (writer == NULL) {
        return;
    }

    writer->length = 0u;
    writer->indent_level = 0u;
    writer->line_count = 0u;
    writer->byte_count = 0u;
    writer->at_line_start = true;
    writer->failed = false;
    if (writer->buffer != NULL && writer->capacity > 0u) {
        writer->buffer[0] = '\0';
    }
    vitte_error_reset(&writer->last_error);
}

const vitte_error_t *vitte_c17_writer_last_error(const vitte_c17_writer_t *writer) {
    return writer != NULL ? &writer->last_error : vitte_error_last();
}

static vitte_status_t vitte_c17_writer_put_byte(vitte_c17_writer_t *writer, char value) {
    if (writer == NULL || writer->failed) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (writer->buffer != NULL) {
        if (writer->length + 1u >= writer->capacity) {
            vitte_c17_writer_set_error(writer, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_OVERFLOW", "C17 writer buffer overflow", NULL);
            return VITTE_STATUS_ERROR_BACKEND;
        }
        writer->buffer[writer->length] = value;
        writer->length++;
        writer->buffer[writer->length] = '\0';
    } else if (writer->stream != NULL) {
        if (fputc((unsigned char)value, writer->stream) == EOF) {
            vitte_c17_writer_set_error(writer, VITTE_STATUS_ERROR_IO, "VITTE_C17_E_WRITE", "failed to write C17 output", NULL);
            return VITTE_STATUS_ERROR_IO;
        }
    } else {
        vitte_c17_writer_set_error(writer, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_C17_E_WRITER", "C17 writer has no sink", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    writer->byte_count++;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_c17_write_indent(vitte_c17_writer_t *writer) {
    size_t level;
    size_t space;

    if (writer == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!writer->at_line_start) {
        return VITTE_STATUS_OK;
    }

    for (level = 0u; level < writer->indent_level; level++) {
        for (space = 0u; space < writer->indent_width; space++) {
            vitte_status_t status = vitte_c17_writer_put_byte(writer, ' ');
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
    }
    writer->at_line_start = false;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_c17_write_char(vitte_c17_writer_t *writer, char value) {
    vitte_status_t status;

    if (writer == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_c17_write_indent(writer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    return vitte_c17_writer_put_byte(writer, value);
}

vitte_status_t vitte_c17_write_string(vitte_c17_writer_t *writer, const char *text) {
    const char *cursor;
    vitte_status_t status;

    if (writer == NULL || text == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_c17_write_indent(writer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    for (cursor = text; *cursor != '\0'; cursor++) {
        status = vitte_c17_writer_put_byte(writer, *cursor);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_c17_write_format(vitte_c17_writer_t *writer, const char *format, ...) {
    char stack_buffer[256];
    va_list args;
    int written;

    if (writer == NULL || format == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    va_start(args, format);
    written = vsnprintf(stack_buffer, sizeof(stack_buffer), format, args);
    va_end(args);

    if (written < 0) {
        vitte_c17_writer_set_error(writer, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_FORMAT", "failed to format C17 output", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }
    if ((size_t)written >= sizeof(stack_buffer)) {
        vitte_c17_writer_set_error(writer, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_FORMAT", "C17 formatted output is too large", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    return vitte_c17_write_string(writer, stack_buffer);
}

vitte_status_t vitte_c17_write_newline(vitte_c17_writer_t *writer) {
    const char *cursor;

    if (writer == NULL || writer->newline == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (cursor = writer->newline; *cursor != '\0'; cursor++) {
        vitte_status_t status = vitte_c17_writer_put_byte(writer, *cursor);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    writer->line_count++;
    writer->at_line_start = true;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_c17_write_open_block(vitte_c17_writer_t *writer) {
    vitte_status_t status;

    status = vitte_c17_write_string(writer, "{");
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_newline(writer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    writer->indent_level++;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_c17_write_close_block(vitte_c17_writer_t *writer) {
    if (writer == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (writer->indent_level > 0u) {
        writer->indent_level--;
    }
    return vitte_c17_write_string(writer, "}");
}

vitte_status_t vitte_c17_writer_flush(vitte_c17_writer_t *writer) {
    if (writer == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (writer->stream != NULL && fflush(writer->stream) != 0) {
        vitte_c17_writer_set_error(writer, VITTE_STATUS_ERROR_IO, "VITTE_C17_E_FLUSH", "failed to flush C17 output", NULL);
        return VITTE_STATUS_ERROR_IO;
    }
    return writer->failed ? writer->last_error.status : VITTE_STATUS_OK;
}
