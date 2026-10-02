/*
 * Vitte Compiler
 * src/backend/c17/writer.c
 *
 * Robust output writer for the ISO C17 backend.
 *
 * Responsibilities:
 *
 *   - callback / FILE / memory sinks
 *   - optional buffering
 *   - exact byte accounting
 *   - line / column tracking
 *   - indentation
 *   - line-start state
 *   - newline normalization
 *   - output limits
 *   - persistent error state
 *   - cancellation
 *   - flush
 *   - memory-writer checkpoints / rollback
 *   - deterministic FNV-1a stream fingerprint
 *
 * This layer does not know anything about C declarations, expressions,
 * modules, programs, translation units, or diagnostics.
 *
 * It is deliberately a low-level byte-output subsystem.
 */

#include "writer.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Internal helpers                                                          */
/* ========================================================================= */

static bool
vitte_c17_writer_size_add(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left > SIZE_MAX - right) {
        return false;
    }

    *result = left + right;
    return true;
}

static bool
vitte_c17_writer_size_mul(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left != 0u &&
        right > SIZE_MAX / left) {
        return false;
    }

    *result = left * right;
    return true;
}

static uint64_t
vitte_c17_writer_hash_update(
    uint64_t hash,
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    size_t index;

    if (data == NULL) {
        return hash;
    }

    bytes =
        (const unsigned char *)data;

    for (index = 0u;
         index < length;
         ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_C17_WRITER_FNV_PRIME;
    }

    return hash;
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_c17_writer_error_name(
    vitte_c17_writer_error_t error)
{
    switch (error) {
        case VITTE_C17_WRITER_ERROR_NONE:
            return "none";

        case VITTE_C17_WRITER_ERROR_INVALID_WRITER:
            return "invalid-writer";

        case VITTE_C17_WRITER_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_C17_WRITER_ERROR_INVALID_STATE:
            return "invalid-state";

        case VITTE_C17_WRITER_ERROR_OUTPUT:
            return "output";

        case VITTE_C17_WRITER_ERROR_OUTPUT_LIMIT:
            return "output-limit";

        case VITTE_C17_WRITER_ERROR_LINE_LIMIT:
            return "line-limit";

        case VITTE_C17_WRITER_ERROR_COLUMN_LIMIT:
            return "column-limit";

        case VITTE_C17_WRITER_ERROR_INDENT_LIMIT:
            return "indent-limit";

        case VITTE_C17_WRITER_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_C17_WRITER_ERROR_OUT_OF_MEMORY:
            return "out-of-memory";

        case VITTE_C17_WRITER_ERROR_CANCELLED:
            return "cancelled";

        case VITTE_C17_WRITER_ERROR_NOT_MEMORY_WRITER:
            return "not-memory-writer";

        case VITTE_C17_WRITER_ERROR_INVALID_CHECKPOINT:
            return "invalid-checkpoint";

        case VITTE_C17_WRITER_ERROR_CORRUPTION:
            return "corruption";

        default:
            return "unknown";
    }
}

const char *
vitte_c17_writer_kind_name(
    vitte_c17_writer_kind_t kind)
{
    switch (kind) {
        case VITTE_C17_WRITER_KIND_INVALID:
            return "invalid";

        case VITTE_C17_WRITER_KIND_CALLBACK:
            return "callback";

        case VITTE_C17_WRITER_KIND_FILE:
            return "file";

        case VITTE_C17_WRITER_KIND_MEMORY:
            return "memory";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_c17_writer_config_t
vitte_c17_writer_config_default(void)
{
    vitte_c17_writer_config_t config;

    memset(
        &config,
        0,
        sizeof(config));

    config.indent_width = 4u;
    config.max_indent = 256u;

    config.max_output_bytes = 0u;
    config.max_lines = 0u;
    config.max_column = 0u;

    config.buffer_capacity =
        VITTE_C17_WRITER_DEFAULT_BUFFER_CAPACITY;

    config.memory_initial_capacity =
        VITTE_C17_WRITER_DEFAULT_MEMORY_CAPACITY;

    config.auto_indent = true;
    config.normalize_newlines = true;
    config.flush_on_newline = false;
    config.track_hash = true;

    return config;
}

static bool
vitte_c17_writer_config_valid(
    const vitte_c17_writer_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->indent_width >
        VITTE_C17_WRITER_MAX_INDENT_WIDTH) {
        return false;
    }

    if (config->max_indent >
        VITTE_C17_WRITER_MAX_INDENT_DEPTH) {
        return false;
    }

    if (config->buffer_capacity >
        VITTE_C17_WRITER_MAX_BUFFER_CAPACITY) {
        return false;
    }

    if (config->memory_initial_capacity >
        VITTE_C17_WRITER_MAX_MEMORY_INITIAL_CAPACITY) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Failure                                                                   */
/* ========================================================================= */

static bool
vitte_c17_writer_fail(
    vitte_c17_writer_t *writer,
    vitte_c17_writer_error_t error)
{
    if (writer != NULL &&
        writer->magic == VITTE_C17_WRITER_MAGIC) {
        if (writer->last_error ==
            VITTE_C17_WRITER_ERROR_NONE) {
            writer->last_error = error;
        }

        writer->failed = true;
    }

    return false;
}

/* ========================================================================= */
/* Structural validity                                                       */
/* ========================================================================= */

bool
vitte_c17_writer_is_valid(
    const vitte_c17_writer_t *writer)
{
    if (writer == NULL) {
        return false;
    }

    if (writer->magic !=
        VITTE_C17_WRITER_MAGIC) {
        return false;
    }

    if (writer->kind <=
            VITTE_C17_WRITER_KIND_INVALID ||
        writer->kind >=
            VITTE_C17_WRITER_KIND_COUNT) {
        return false;
    }

    if (!vitte_c17_writer_config_valid(
            &writer->config)) {
        return false;
    }

    if (writer->buffer_length >
        writer->buffer_capacity) {
        return false;
    }

    if (writer->buffer_capacity != 0u &&
        writer->buffer == NULL) {
        return false;
    }

    switch (writer->kind) {
        case VITTE_C17_WRITER_KIND_CALLBACK:
            if (writer->sink.callback.write == NULL) {
                return false;
            }
            break;

        case VITTE_C17_WRITER_KIND_FILE:
            if (writer->sink.file.stream == NULL) {
                return false;
            }
            break;

        case VITTE_C17_WRITER_KIND_MEMORY:
            if (writer->sink.memory.length >
                writer->sink.memory.capacity) {
                return false;
            }

            if (writer->sink.memory.capacity != 0u &&
                writer->sink.memory.data == NULL) {
                return false;
            }
            break;

        default:
            return false;
    }

    return true;
}

/* ========================================================================= */
/* Base initialization                                                       */
/* ========================================================================= */

static bool
vitte_c17_writer_init_base(
    vitte_c17_writer_t *writer,
    vitte_c17_writer_kind_t kind,
    const vitte_c17_writer_config_t *config)
{
    vitte_c17_writer_config_t effective;

    if (writer == NULL) {
        return false;
    }

    effective =
        config != NULL
            ? *config
            : vitte_c17_writer_config_default();

    if (!vitte_c17_writer_config_valid(
            &effective)) {
        return false;
    }

    memset(
        writer,
        0,
        sizeof(*writer));

    writer->magic =
        VITTE_C17_WRITER_MAGIC;

    writer->kind = kind;
    writer->config = effective;

    writer->last_error =
        VITTE_C17_WRITER_ERROR_NONE;

    writer->line = 1u;
    writer->column = 1u;

    writer->at_line_start = true;
    writer->last_was_cr = false;

    writer->hash =
        VITTE_C17_WRITER_FNV_OFFSET;

    writer->generation =
        UINT64_C(1);

    if (effective.buffer_capacity != 0u &&
        kind != VITTE_C17_WRITER_KIND_MEMORY) {
        writer->buffer =
            (unsigned char *)malloc(
                effective.buffer_capacity);

        if (writer->buffer == NULL) {
            memset(
                writer,
                0,
                sizeof(*writer));

            return false;
        }

        writer->buffer_capacity =
            effective.buffer_capacity;
    }

    return true;
}

/* ========================================================================= */
/* Callback writer                                                           */
/* ========================================================================= */

bool
vitte_c17_writer_init_callback(
    vitte_c17_writer_t *writer,
    vitte_c17_writer_write_fn write_fn,
    vitte_c17_writer_flush_fn flush_fn,
    void *user_data,
    const vitte_c17_writer_config_t *config)
{
    if (write_fn == NULL) {
        return false;
    }

    if (!vitte_c17_writer_init_base(
            writer,
            VITTE_C17_WRITER_KIND_CALLBACK,
            config)) {
        return false;
    }

    writer->sink.callback.write =
        write_fn;

    writer->sink.callback.flush =
        flush_fn;

    writer->sink.callback.user_data =
        user_data;

    return true;
}

/* ========================================================================= */
/* FILE writer                                                               */
/* ========================================================================= */

bool
vitte_c17_writer_init_file(
    vitte_c17_writer_t *writer,
    FILE *stream,
    bool close_on_destroy,
    const vitte_c17_writer_config_t *config)
{
    if (stream == NULL) {
        return false;
    }

    if (!vitte_c17_writer_init_base(
            writer,
            VITTE_C17_WRITER_KIND_FILE,
            config)) {
        return false;
    }

    writer->sink.file.stream =
        stream;

    writer->sink.file.close_on_destroy =
        close_on_destroy;

    return true;
}

/* ========================================================================= */
/* Memory reserve                                                            */
/* ========================================================================= */

static bool
vitte_c17_writer_memory_reserve_internal(
    vitte_c17_writer_t *writer,
    size_t required)
{
    size_t capacity;
    size_t bytes;
    unsigned char *replacement;

    if (writer->kind !=
        VITTE_C17_WRITER_KIND_MEMORY) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_NOT_MEMORY_WRITER);
    }

    /*
     * Always reserve one byte for the trailing NUL.
     */
    if (!vitte_c17_writer_size_add(
            required,
            1u,
            &bytes)) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_OVERFLOW);
    }

    if (bytes <=
        writer->sink.memory.capacity) {
        return true;
    }

    capacity =
        writer->sink.memory.capacity;

    if (capacity == 0u) {
        capacity =
            writer->config.memory_initial_capacity;

        if (capacity == 0u) {
            capacity = 64u;
        }
    }

    while (capacity < bytes) {
        size_t next;

        if (capacity > SIZE_MAX / 2u) {
            capacity = bytes;
            break;
        }

        next = capacity * 2u;

        if (next < capacity) {
            capacity = bytes;
            break;
        }

        capacity = next;
    }

    if (capacity < bytes) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_OVERFLOW);
    }

    replacement =
        (unsigned char *)realloc(
            writer->sink.memory.data,
            capacity);

    if (replacement == NULL) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_OUT_OF_MEMORY);
    }

    writer->sink.memory.data =
        replacement;

    writer->sink.memory.capacity =
        capacity;

    return true;
}

/* ========================================================================= */
/* Memory writer                                                             */
/* ========================================================================= */

bool
vitte_c17_writer_init_memory(
    vitte_c17_writer_t *writer,
    const vitte_c17_writer_config_t *config)
{
    if (!vitte_c17_writer_init_base(
            writer,
            VITTE_C17_WRITER_KIND_MEMORY,
            config)) {
        return false;
    }

    if (!vitte_c17_writer_memory_reserve_internal(
            writer,
            0u)) {
        free(writer->buffer);

        memset(
            writer,
            0,
            sizeof(*writer));

        return false;
    }

    writer->sink.memory.data[0] =
        (unsigned char)'\0';

    return true;
}

/* ========================================================================= */
/* Raw sink                                                                  */
/* ========================================================================= */

static bool
vitte_c17_writer_sink_write(
    vitte_c17_writer_t *writer,
    const unsigned char *data,
    size_t length)
{
    if (length == 0u) {
        return true;
    }

    switch (writer->kind) {
        case VITTE_C17_WRITER_KIND_CALLBACK:
            if (!writer->sink.callback.write(
                    writer->sink.callback.user_data,
                    (const char *)data,
                    length)) {
                return
                    vitte_c17_writer_fail(
                        writer,
                        VITTE_C17_WRITER_ERROR_OUTPUT);
            }

            return true;

        case VITTE_C17_WRITER_KIND_FILE:
        {
            size_t written;

            written =
                fwrite(
                    data,
                    1u,
                    length,
                    writer->sink.file.stream);

            if (written != length) {
                writer->system_error = errno;

                return
                    vitte_c17_writer_fail(
                        writer,
                        VITTE_C17_WRITER_ERROR_OUTPUT);
            }

            return true;
        }

        case VITTE_C17_WRITER_KIND_MEMORY:
        {
            size_t new_length;

            if (!vitte_c17_writer_size_add(
                    writer->sink.memory.length,
                    length,
                    &new_length)) {
                return
                    vitte_c17_writer_fail(
                        writer,
                        VITTE_C17_WRITER_ERROR_OVERFLOW);
            }

            if (!vitte_c17_writer_memory_reserve_internal(
                    writer,
                    new_length)) {
                return false;
            }

            memcpy(
                writer->sink.memory.data +
                    writer->sink.memory.length,
                data,
                length);

            writer->sink.memory.length =
                new_length;

            writer->sink.memory.data[
                new_length] =
                (unsigned char)'\0';

            return true;
        }

        default:
            return
                vitte_c17_writer_fail(
                    writer,
                    VITTE_C17_WRITER_ERROR_CORRUPTION);
    }
}

/* ========================================================================= */
/* Flush internal buffer                                                     */
/* ========================================================================= */

static bool
vitte_c17_writer_flush_buffer(
    vitte_c17_writer_t *writer)
{
    size_t length;

    if (writer->buffer_length == 0u) {
        return true;
    }

    length =
        writer->buffer_length;

    /*
     * Do not clear buffer_length until the sink accepted all bytes.
     */
    if (!vitte_c17_writer_sink_write(
            writer,
            writer->buffer,
            length)) {
        return false;
    }

    writer->buffer_length = 0u;

    return true;
}

/* ========================================================================= */
/* Buffered raw output                                                       */
/* ========================================================================= */

static bool
vitte_c17_writer_raw_output(
    vitte_c17_writer_t *writer,
    const unsigned char *data,
    size_t length)
{
    size_t offset;

    if (length == 0u) {
        return true;
    }

    /*
     * Memory writers already provide their own dynamically growing buffer.
     */
    if (writer->kind ==
            VITTE_C17_WRITER_KIND_MEMORY ||
        writer->buffer_capacity == 0u) {
        return
            vitte_c17_writer_sink_write(
                writer,
                data,
                length);
    }

    offset = 0u;

    while (offset < length) {
        size_t available;
        size_t remaining;
        size_t amount;

        available =
            writer->buffer_capacity -
            writer->buffer_length;

        if (available == 0u) {
            if (!vitte_c17_writer_flush_buffer(
                    writer)) {
                return false;
            }

            available =
                writer->buffer_capacity;
        }

        remaining =
            length - offset;

        /*
         * Large writes bypass the internal buffer when it is empty.
         */
        if (writer->buffer_length == 0u &&
            remaining >=
                writer->buffer_capacity) {
            if (!vitte_c17_writer_sink_write(
                    writer,
                    data + offset,
                    remaining)) {
                return false;
            }

            return true;
        }

        amount =
            remaining < available
                ? remaining
                : available;

        memcpy(
            writer->buffer +
                writer->buffer_length,
            data + offset,
            amount);

        writer->buffer_length += amount;
        offset += amount;
    }

    return true;
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

static bool
vitte_c17_writer_check_output_limit(
    vitte_c17_writer_t *writer,
    size_t additional)
{
    size_t total;

    if (writer->config.max_output_bytes == 0u) {
        return true;
    }

    if (!vitte_c17_writer_size_add(
            writer->bytes_written,
            additional,
            &total)) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_OVERFLOW);
    }

    if (total >
        writer->config.max_output_bytes) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_OUTPUT_LIMIT);
    }

    return true;
}

/* ========================================================================= */
/* Position accounting                                                       */
/* ========================================================================= */

static bool
vitte_c17_writer_account_byte(
    vitte_c17_writer_t *writer,
    unsigned char byte)
{
    if (writer->bytes_written == SIZE_MAX) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_OVERFLOW);
    }

    ++writer->bytes_written;

    if (byte == (unsigned char)'\n') {
        if (writer->line == SIZE_MAX) {
            return
                vitte_c17_writer_fail(
                    writer,
                    VITTE_C17_WRITER_ERROR_OVERFLOW);
        }

        ++writer->line;

        writer->column = 1u;
        writer->at_line_start = true;

        writer->maximum_line =
            writer->line >
                writer->maximum_line
                ? writer->line
                : writer->maximum_line;

        if (writer->config.max_lines != 0u &&
            writer->line >
                writer->config.max_lines) {
            return
                vitte_c17_writer_fail(
                    writer,
                    VITTE_C17_WRITER_ERROR_LINE_LIMIT);
        }

        return true;
    }

    writer->at_line_start = false;

    if (writer->column == SIZE_MAX) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_OVERFLOW);
    }

    ++writer->column;

    if (writer->column >
        writer->maximum_column) {
        writer->maximum_column =
            writer->column;
    }

    if (writer->config.max_column != 0u &&
        writer->column >
            writer->config.max_column) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_COLUMN_LIMIT);
    }

    return true;
}

/* ========================================================================= */
/* Commit bytes                                                              */
/* ========================================================================= */

static bool
vitte_c17_writer_commit_bytes(
    vitte_c17_writer_t *writer,
    const unsigned char *data,
    size_t length)
{
    size_t index;

    if (length == 0u) {
        return true;
    }

    if (!vitte_c17_writer_check_output_limit(
            writer,
            length)) {
        return false;
    }

    /*
     * Preflight line/column constraints before mutating the external sink.
     */
    if (writer->config.max_lines != 0u ||
        writer->config.max_column != 0u) {
        size_t line;
        size_t column;

        line = writer->line;
        column = writer->column;

        for (index = 0u;
             index < length;
             ++index) {
            if (data[index] ==
                (unsigned char)'\n') {
                if (line == SIZE_MAX) {
                    return
                        vitte_c17_writer_fail(
                            writer,
                            VITTE_C17_WRITER_ERROR_OVERFLOW);
                }

                ++line;
                column = 1u;

                if (writer->config.max_lines != 0u &&
                    line > writer->config.max_lines) {
                    return
                        vitte_c17_writer_fail(
                            writer,
                            VITTE_C17_WRITER_ERROR_LINE_LIMIT);
                }
            } else {
                if (column == SIZE_MAX) {
                    return
                        vitte_c17_writer_fail(
                            writer,
                            VITTE_C17_WRITER_ERROR_OVERFLOW);
                }

                ++column;

                if (writer->config.max_column != 0u &&
                    column >
                        writer->config.max_column) {
                    return
                        vitte_c17_writer_fail(
                            writer,
                            VITTE_C17_WRITER_ERROR_COLUMN_LIMIT);
                }
            }
        }
    }

    if (!vitte_c17_writer_raw_output(
            writer,
            data,
            length)) {
        return false;
    }

    if (writer->config.track_hash) {
        writer->hash =
            vitte_c17_writer_hash_update(
                writer->hash,
                data,
                length);
    }

    for (index = 0u;
         index < length;
         ++index) {
        if (!vitte_c17_writer_account_byte(
                writer,
                data[index])) {
            /*
             * All limit checks were preflighted. Reaching this path now means
             * an internal accounting overflow/corruption.
             */
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Indentation                                                               */
/* ========================================================================= */

static bool
vitte_c17_writer_emit_indent(
    vitte_c17_writer_t *writer)
{
    static const unsigned char spaces[] =
        "                                                                ";
    size_t count;

    if (!writer->config.auto_indent ||
        !writer->at_line_start ||
        writer->indent_level == 0u ||
        writer->config.indent_width == 0u) {
        return true;
    }

    if (!vitte_c17_writer_size_mul(
            writer->indent_level,
            writer->config.indent_width,
            &count)) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_OVERFLOW);
    }

    while (count != 0u) {
        size_t amount;

        amount =
            count < sizeof(spaces) - 1u
                ? count
                : sizeof(spaces) - 1u;

        if (!vitte_c17_writer_commit_bytes(
                writer,
                spaces,
                amount)) {
            return false;
        }

        count -= amount;
    }

    return true;
}

/* ========================================================================= */
/* Normalized byte emission                                                  */
/* ========================================================================= */

static bool
vitte_c17_writer_emit_normalized(
    vitte_c17_writer_t *writer,
    const unsigned char *data,
    size_t length)
{
    size_t index;

    for (index = 0u;
         index < length;
         ++index) {
        unsigned char byte;

        byte = data[index];

        if (writer->config.normalize_newlines) {
            if (byte == (unsigned char)'\r') {
                static const unsigned char newline =
                    (unsigned char)'\n';

                if (!vitte_c17_writer_emit_indent(
                        writer)) {
                    return false;
                }

                if (!vitte_c17_writer_commit_bytes(
                        writer,
                        &newline,
                        1u)) {
                    return false;
                }

                writer->last_was_cr = true;

                if (writer->config.flush_on_newline &&
                    !vitte_c17_writer_flush(writer)) {
                    return false;
                }

                continue;
            }

            if (byte == (unsigned char)'\n' &&
                writer->last_was_cr) {
                writer->last_was_cr = false;
                continue;
            }

            writer->last_was_cr = false;
        }

        if (byte == (unsigned char)'\n') {
            if (!vitte_c17_writer_emit_indent(
                    writer)) {
                return false;
            }

            if (!vitte_c17_writer_commit_bytes(
                    writer,
                    &byte,
                    1u)) {
                return false;
            }

            if (writer->config.flush_on_newline &&
                !vitte_c17_writer_flush(writer)) {
                return false;
            }

            continue;
        }

        if (!vitte_c17_writer_emit_indent(
                writer)) {
            return false;
        }

        if (!vitte_c17_writer_commit_bytes(
                writer,
                &byte,
                1u)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Public write                                                              */
/* ========================================================================= */

bool
vitte_c17_writer_write_n(
    vitte_c17_writer_t *writer,
    const char *data,
    size_t length)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    if (writer->failed) {
        return false;
    }

    if (writer->cancelled) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_CANCELLED);
    }

    if (length == 0u) {
        return true;
    }

    if (data == NULL) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_INVALID_ARGUMENT);
    }

    return
        vitte_c17_writer_emit_normalized(
            writer,
            (const unsigned char *)data,
            length);
}

bool
vitte_c17_writer_write(
    vitte_c17_writer_t *writer,
    const char *text)
{
    if (text == NULL) {
        if (writer != NULL &&
            writer->magic ==
                VITTE_C17_WRITER_MAGIC) {
            return
                vitte_c17_writer_fail(
                    writer,
                    VITTE_C17_WRITER_ERROR_INVALID_ARGUMENT);
        }

        return false;
    }

    return
        vitte_c17_writer_write_n(
            writer,
            text,
            strlen(text));
}

bool
vitte_c17_writer_write_char(
    vitte_c17_writer_t *writer,
    char character)
{
    return
        vitte_c17_writer_write_n(
            writer,
            &character,
            1u);
}

/* ========================================================================= */
/* Repeat                                                                    */
/* ========================================================================= */

bool
vitte_c17_writer_repeat(
    vitte_c17_writer_t *writer,
    char character,
    size_t count)
{
    char buffer[64];
    size_t amount;

    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    if (count == 0u) {
        return true;
    }

    memset(
        buffer,
        (unsigned char)character,
        sizeof(buffer));

    while (count != 0u) {
        amount =
            count < sizeof(buffer)
                ? count
                : sizeof(buffer);

        if (!vitte_c17_writer_write_n(
                writer,
                buffer,
                amount)) {
            return false;
        }

        count -= amount;
    }

    return true;
}

/* ========================================================================= */
/* Newline                                                                   */
/* ========================================================================= */

bool
vitte_c17_writer_newline(
    vitte_c17_writer_t *writer)
{
    static const char newline = '\n';

    return
        vitte_c17_writer_write_n(
            writer,
            &newline,
            1u);
}

bool
vitte_c17_writer_blank_line(
    vitte_c17_writer_t *writer)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    /*
     * If currently at the beginning of a line, one newline creates one empty
     * line. Otherwise terminate the current line first, then emit one empty
     * line.
     */
    if (!writer->at_line_start) {
        if (!vitte_c17_writer_newline(
                writer)) {
            return false;
        }
    }

    return
        vitte_c17_writer_newline(
            writer);
}

/* ========================================================================= */
/* Indent controls                                                           */
/* ========================================================================= */

bool
vitte_c17_writer_indent(
    vitte_c17_writer_t *writer)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    if (writer->failed) {
        return false;
    }

    if (writer->indent_level >=
        writer->config.max_indent) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_INDENT_LIMIT);
    }

    if (writer->indent_level ==
        SIZE_MAX) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_OVERFLOW);
    }

    ++writer->indent_level;

    if (writer->indent_level >
        writer->maximum_indent) {
        writer->maximum_indent =
            writer->indent_level;
    }

    return true;
}

bool
vitte_c17_writer_dedent(
    vitte_c17_writer_t *writer)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    if (writer->failed) {
        return false;
    }

    if (writer->indent_level == 0u) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_INVALID_STATE);
    }

    --writer->indent_level;

    return true;
}

bool
vitte_c17_writer_set_indent(
    vitte_c17_writer_t *writer,
    size_t level)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    if (writer->failed) {
        return false;
    }

    if (level >
        writer->config.max_indent) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_INDENT_LIMIT);
    }

    writer->indent_level = level;

    if (level >
        writer->maximum_indent) {
        writer->maximum_indent = level;
    }

    return true;
}

/* ========================================================================= */
/* Flush                                                                     */
/* ========================================================================= */

bool
vitte_c17_writer_flush(
    vitte_c17_writer_t *writer)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    if (writer->failed) {
        return false;
    }

    if (!vitte_c17_writer_flush_buffer(
            writer)) {
        return false;
    }

    switch (writer->kind) {
        case VITTE_C17_WRITER_KIND_CALLBACK:
            if (writer->sink.callback.flush != NULL &&
                !writer->sink.callback.flush(
                    writer->sink.callback.user_data)) {
                return
                    vitte_c17_writer_fail(
                        writer,
                        VITTE_C17_WRITER_ERROR_OUTPUT);
            }
            break;

        case VITTE_C17_WRITER_KIND_FILE:
            if (fflush(
                    writer->sink.file.stream) != 0) {
                writer->system_error = errno;

                return
                    vitte_c17_writer_fail(
                        writer,
                        VITTE_C17_WRITER_ERROR_OUTPUT);
            }
            break;

        case VITTE_C17_WRITER_KIND_MEMORY:
            /*
             * Already committed.
             */
            break;

        default:
            return
                vitte_c17_writer_fail(
                    writer,
                    VITTE_C17_WRITER_ERROR_CORRUPTION);
    }

    return true;
}

/* ========================================================================= */
/* Cancellation                                                              */
/* ========================================================================= */

void
vitte_c17_writer_cancel(
    vitte_c17_writer_t *writer)
{
    if (writer == NULL ||
        writer->magic !=
            VITTE_C17_WRITER_MAGIC) {
        return;
    }

    writer->cancelled = true;

    if (writer->last_error ==
        VITTE_C17_WRITER_ERROR_NONE) {
        writer->last_error =
            VITTE_C17_WRITER_ERROR_CANCELLED;
    }
}

/* ========================================================================= */
/* Memory access                                                             */
/* ========================================================================= */

const char *
vitte_c17_writer_memory_data(
    const vitte_c17_writer_t *writer)
{
    if (!vitte_c17_writer_is_valid(
            writer) ||
        writer->kind !=
            VITTE_C17_WRITER_KIND_MEMORY) {
        return NULL;
    }

    return
        (const char *)
            writer->sink.memory.data;
}

size_t
vitte_c17_writer_memory_length(
    const vitte_c17_writer_t *writer)
{
    if (!vitte_c17_writer_is_valid(
            writer) ||
        writer->kind !=
            VITTE_C17_WRITER_KIND_MEMORY) {
        return 0u;
    }

    return
        writer->sink.memory.length;
}

size_t
vitte_c17_writer_memory_capacity(
    const vitte_c17_writer_t *writer)
{
    if (!vitte_c17_writer_is_valid(
            writer) ||
        writer->kind !=
            VITTE_C17_WRITER_KIND_MEMORY) {
        return 0u;
    }

    return
        writer->sink.memory.capacity;
}

bool
vitte_c17_writer_memory_reserve(
    vitte_c17_writer_t *writer,
    size_t capacity)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    if (writer->kind !=
        VITTE_C17_WRITER_KIND_MEMORY) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_NOT_MEMORY_WRITER);
    }

    return
        vitte_c17_writer_memory_reserve_internal(
            writer,
            capacity);
}

/* ========================================================================= */
/* Position recomputation                                                    */
/* ========================================================================= */

static bool
vitte_c17_writer_recompute_memory_state(
    vitte_c17_writer_t *writer)
{
    size_t index;
    size_t length;
    const unsigned char *data;

    if (writer->kind !=
        VITTE_C17_WRITER_KIND_MEMORY) {
        return false;
    }

    data =
        writer->sink.memory.data;

    length =
        writer->sink.memory.length;

    writer->bytes_written = 0u;
    writer->line = 1u;
    writer->column = 1u;

    writer->maximum_line = 1u;
    writer->maximum_column = 1u;

    writer->at_line_start = true;
    writer->last_was_cr = false;

    writer->hash =
        VITTE_C17_WRITER_FNV_OFFSET;

    for (index = 0u;
         index < length;
         ++index) {
        unsigned char byte;

        byte = data[index];

        if (writer->config.track_hash) {
            writer->hash =
                vitte_c17_writer_hash_update(
                    writer->hash,
                    &byte,
                    1u);
        }

        if (!vitte_c17_writer_account_byte(
                writer,
                byte)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Memory checkpoint                                                         */
/* ========================================================================= */

vitte_c17_writer_checkpoint_t
vitte_c17_writer_checkpoint(
    const vitte_c17_writer_t *writer)
{
    vitte_c17_writer_checkpoint_t checkpoint;

    memset(
        &checkpoint,
        0,
        sizeof(checkpoint));

    if (!vitte_c17_writer_is_valid(
            writer) ||
        writer->kind !=
            VITTE_C17_WRITER_KIND_MEMORY) {
        return checkpoint;
    }

    checkpoint.valid = true;

    checkpoint.generation =
        writer->generation;

    checkpoint.length =
        writer->sink.memory.length;

    checkpoint.bytes_written =
        writer->bytes_written;

    checkpoint.line =
        writer->line;

    checkpoint.column =
        writer->column;

    checkpoint.maximum_line =
        writer->maximum_line;

    checkpoint.maximum_column =
        writer->maximum_column;

    checkpoint.indent_level =
        writer->indent_level;

    checkpoint.maximum_indent =
        writer->maximum_indent;

    checkpoint.at_line_start =
        writer->at_line_start;

    checkpoint.last_was_cr =
        writer->last_was_cr;

    checkpoint.hash =
        writer->hash;

    return checkpoint;
}

/* ========================================================================= */
/* Rollback                                                                  */
/* ========================================================================= */

bool
vitte_c17_writer_rollback(
    vitte_c17_writer_t *writer,
    const vitte_c17_writer_checkpoint_t *checkpoint)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    if (writer->kind !=
        VITTE_C17_WRITER_KIND_MEMORY) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_NOT_MEMORY_WRITER);
    }

    if (checkpoint == NULL ||
        !checkpoint->valid ||
        checkpoint->generation !=
            writer->generation ||
        checkpoint->length >
            writer->sink.memory.length) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_INVALID_CHECKPOINT);
    }

    writer->sink.memory.length =
        checkpoint->length;

    writer->sink.memory.data[
        checkpoint->length] =
        (unsigned char)'\0';

    writer->bytes_written =
        checkpoint->bytes_written;

    writer->line =
        checkpoint->line;

    writer->column =
        checkpoint->column;

    writer->maximum_line =
        checkpoint->maximum_line;

    writer->maximum_column =
        checkpoint->maximum_column;

    writer->indent_level =
        checkpoint->indent_level;

    writer->maximum_indent =
        checkpoint->maximum_indent;

    writer->at_line_start =
        checkpoint->at_line_start;

    writer->last_was_cr =
        checkpoint->last_was_cr;

    writer->hash =
        checkpoint->hash;

    /*
     * A rollback is an explicit recovery operation for memory writers.
     */
    writer->failed = false;
    writer->cancelled = false;

    writer->last_error =
        VITTE_C17_WRITER_ERROR_NONE;

    writer->system_error = 0;

    return true;
}

/* ========================================================================= */
/* Memory truncate                                                           */
/* ========================================================================= */

bool
vitte_c17_writer_memory_truncate(
    vitte_c17_writer_t *writer,
    size_t length)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    if (writer->kind !=
        VITTE_C17_WRITER_KIND_MEMORY) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_NOT_MEMORY_WRITER);
    }

    if (length >
        writer->sink.memory.length) {
        return
            vitte_c17_writer_fail(
                writer,
                VITTE_C17_WRITER_ERROR_INVALID_ARGUMENT);
    }

    writer->sink.memory.length =
        length;

    writer->sink.memory.data[length] =
        (unsigned char)'\0';

    /*
     * Recompute all stream-dependent metadata. This makes arbitrary truncate
     * safe, not only rollback to a captured checkpoint.
     */
    writer->failed = false;
    writer->cancelled = false;
    writer->last_error =
        VITTE_C17_WRITER_ERROR_NONE;

    return
        vitte_c17_writer_recompute_memory_state(
            writer);
}

/* ========================================================================= */
/* Memory detach                                                             */
/* ========================================================================= */

char *
vitte_c17_writer_memory_detach(
    vitte_c17_writer_t *writer,
    size_t *length)
{
    char *data;

    if (!vitte_c17_writer_is_valid(
            writer) ||
        writer->kind !=
            VITTE_C17_WRITER_KIND_MEMORY) {
        return NULL;
    }

    data =
        (char *)writer->sink.memory.data;

    if (length != NULL) {
        *length =
            writer->sink.memory.length;
    }

    writer->sink.memory.data = NULL;
    writer->sink.memory.length = 0u;
    writer->sink.memory.capacity = 0u;

    /*
     * Keep writer reusable.
     */
    if (!vitte_c17_writer_memory_reserve_internal(
            writer,
            0u)) {
        /*
         * The detached data remains valid and owned by the caller.
         * The writer itself enters a failed state.
         */
        return data;
    }

    writer->sink.memory.data[0] =
        (unsigned char)'\0';

    writer->bytes_written = 0u;
    writer->line = 1u;
    writer->column = 1u;

    writer->maximum_line = 1u;
    writer->maximum_column = 1u;

    writer->indent_level = 0u;
    writer->maximum_indent = 0u;

    writer->at_line_start = true;
    writer->last_was_cr = false;

    writer->hash =
        VITTE_C17_WRITER_FNV_OFFSET;

    writer->failed = false;
    writer->cancelled = false;

    writer->last_error =
        VITTE_C17_WRITER_ERROR_NONE;

    writer->system_error = 0;

    if (writer->generation != UINT64_MAX) {
        ++writer->generation;
    }

    return data;
}

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

bool
vitte_c17_writer_reset(
    vitte_c17_writer_t *writer)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    /*
     * Buffered external output is discarded only if it has not yet reached
     * the sink. Reset is therefore primarily intended for a fresh generation
     * or memory writer. External sink bytes already written cannot be undone.
     */
    writer->buffer_length = 0u;

    if (writer->kind ==
        VITTE_C17_WRITER_KIND_MEMORY) {
        writer->sink.memory.length = 0u;

        if (writer->sink.memory.data != NULL) {
            writer->sink.memory.data[0] =
                (unsigned char)'\0';
        }
    }

    writer->bytes_written = 0u;

    writer->line = 1u;
    writer->column = 1u;

    writer->maximum_line = 1u;
    writer->maximum_column = 1u;

    writer->indent_level = 0u;
    writer->maximum_indent = 0u;

    writer->at_line_start = true;
    writer->last_was_cr = false;

    writer->failed = false;
    writer->cancelled = false;

    writer->last_error =
        VITTE_C17_WRITER_ERROR_NONE;

    writer->system_error = 0;

    writer->hash =
        VITTE_C17_WRITER_FNV_OFFSET;

    if (writer->generation != UINT64_MAX) {
        ++writer->generation;
    }

    return true;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_c17_writer_stats_t
vitte_c17_writer_stats(
    const vitte_c17_writer_t *writer)
{
    vitte_c17_writer_stats_t stats;

    memset(
        &stats,
        0,
        sizeof(stats));

    if (!vitte_c17_writer_is_valid(
            writer)) {
        return stats;
    }

    stats.valid = true;

    stats.kind =
        writer->kind;

    stats.bytes_written =
        writer->bytes_written;

    stats.line =
        writer->line;

    stats.column =
        writer->column;

    stats.maximum_line =
        writer->maximum_line;

    stats.maximum_column =
        writer->maximum_column;

    stats.indent_level =
        writer->indent_level;

    stats.maximum_indent =
        writer->maximum_indent;

    stats.buffered_bytes =
        writer->buffer_length;

    stats.hash =
        writer->config.track_hash
            ? writer->hash
            : UINT64_C(0);

    stats.failed =
        writer->failed;

    stats.cancelled =
        writer->cancelled;

    stats.generation =
        writer->generation;

    if (writer->kind ==
        VITTE_C17_WRITER_KIND_MEMORY) {
        stats.memory_length =
            writer->sink.memory.length;

        stats.memory_capacity =
            writer->sink.memory.capacity;
    }

    return stats;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_c17_writer_validate(
    const vitte_c17_writer_t *writer)
{
    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    if (writer->line == 0u ||
        writer->column == 0u) {
        return false;
    }

    if (writer->maximum_line <
        writer->line) {
        return false;
    }

    if (writer->maximum_column <
        writer->column) {
        return false;
    }

    if (writer->maximum_indent <
        writer->indent_level) {
        return false;
    }

    if (writer->indent_level >
        writer->config.max_indent) {
        return false;
    }

    if (writer->config.max_output_bytes != 0u &&
        writer->bytes_written >
            writer->config.max_output_bytes) {
        return false;
    }

    if (writer->config.max_lines != 0u &&
        writer->line >
            writer->config.max_lines) {
        return false;
    }

    if (writer->config.max_column != 0u &&
        writer->column >
            writer->config.max_column) {
        return false;
    }

    if (writer->failed &&
        writer->last_error ==
            VITTE_C17_WRITER_ERROR_NONE) {
        return false;
    }

    if (writer->cancelled &&
        writer->last_error ==
            VITTE_C17_WRITER_ERROR_NONE) {
        return false;
    }

    if (writer->kind ==
        VITTE_C17_WRITER_KIND_MEMORY) {
        if (writer->sink.memory.data == NULL) {
            return false;
        }

        if (writer->sink.memory.length >=
            writer->sink.memory.capacity) {
            return false;
        }

        if (writer->sink.memory.data[
                writer->sink.memory.length] !=
            (unsigned char)'\0') {
            return false;
        }

        /*
         * Memory output is committed immediately, so the logical stream
         * length must equal bytes_written.
         */
        if (writer->sink.memory.length !=
            writer->bytes_written) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Error access                                                              */
/* ========================================================================= */

vitte_c17_writer_error_t
vitte_c17_writer_last_error(
    const vitte_c17_writer_t *writer)
{
    if (writer == NULL ||
        writer->magic !=
            VITTE_C17_WRITER_MAGIC) {
        return
            VITTE_C17_WRITER_ERROR_INVALID_WRITER;
    }

    return writer->last_error;
}

int
vitte_c17_writer_system_error(
    const vitte_c17_writer_t *writer)
{
    if (writer == NULL ||
        writer->magic !=
            VITTE_C17_WRITER_MAGIC) {
        return 0;
    }

    return writer->system_error;
}

void
vitte_c17_writer_clear_error(
    vitte_c17_writer_t *writer)
{
    if (writer == NULL ||
        writer->magic !=
            VITTE_C17_WRITER_MAGIC) {
        return;
    }

    /*
     * clear_error does not implicitly recover a failed/cancelled writer.
     * reset() or rollback() is required for recovery.
     */
    if (!writer->failed &&
        !writer->cancelled) {
        writer->last_error =
            VITTE_C17_WRITER_ERROR_NONE;

        writer->system_error = 0;
    }
}

/* ========================================================================= */
/* Destroy                                                                   */
/* ========================================================================= */

bool
vitte_c17_writer_destroy(
    vitte_c17_writer_t *writer)
{
    bool success;

    if (!vitte_c17_writer_is_valid(
            writer)) {
        return false;
    }

    success = true;

    /*
     * Best-effort flush. Preserve failure in the return value, but still free
     * all owned resources.
     */
    if (!writer->failed &&
        !writer->cancelled) {
        if (!vitte_c17_writer_flush(
                writer)) {
            success = false;
        }
    } else {
        /*
         * A previously failed writer may still contain buffered bytes.
         * Do not attempt to commit them after failure.
         */
        writer->buffer_length = 0u;
    }

    if (writer->kind ==
            VITTE_C17_WRITER_KIND_FILE &&
        writer->sink.file.close_on_destroy &&
        writer->sink.file.stream != NULL) {
        if (fclose(
                writer->sink.file.stream) != 0) {
            success = false;
        }

        writer->sink.file.stream = NULL;
    }

    if (writer->kind ==
        VITTE_C17_WRITER_KIND_MEMORY) {
        free(
            writer->sink.memory.data);

        writer->sink.memory.data = NULL;
        writer->sink.memory.length = 0u;
        writer->sink.memory.capacity = 0u;
    }

    free(writer->buffer);

    memset(
        writer,
        0,
        sizeof(*writer));

    writer->magic =
        VITTE_C17_WRITER_DEAD_MAGIC;

    writer->kind =
        VITTE_C17_WRITER_KIND_INVALID;

    return success;
}

/* ========================================================================= */
/* Convenience output                                                        */
/* ========================================================================= */

bool
vitte_c17_writer_write_space(
    vitte_c17_writer_t *writer)
{
    return
        vitte_c17_writer_write_char(
            writer,
            ' ');
}

bool
vitte_c17_writer_write_tab(
    vitte_c17_writer_t *writer)
{
    return
        vitte_c17_writer_write_char(
            writer,
            '\t');
}

bool
vitte_c17_writer_write_uint64(
    vitte_c17_writer_t *writer,
    uint64_t value)
{
    char buffer[32];
    size_t length;

    length = 0u;

    do {
        unsigned digit;

        digit =
            (unsigned)(value % UINT64_C(10));

        buffer[length++] =
            (char)('0' + digit);

        value /= UINT64_C(10);
    } while (value != UINT64_C(0));

    /*
     * Reverse in-place.
     */
    {
        size_t left;
        size_t right;

        left = 0u;
        right = length - 1u;

        while (left < right) {
            char temporary;

            temporary =
                buffer[left];

            buffer[left] =
                buffer[right];

            buffer[right] =
                temporary;

            ++left;
            --right;
        }
    }

    return
        vitte_c17_writer_write_n(
            writer,
            buffer,
            length);
}

bool
vitte_c17_writer_write_size(
    vitte_c17_writer_t *writer,
    size_t value)
{
    char buffer[
        (sizeof(size_t) * CHAR_BIT / 3u) + 3u];
    size_t length;

    length = 0u;

    do {
        unsigned digit;

        digit =
            (unsigned)(value % 10u);

        buffer[length++] =
            (char)('0' + digit);

        value /= 10u;
    } while (value != 0u);

    {
        size_t left;
        size_t right;

        left = 0u;
        right = length - 1u;

        while (left < right) {
            char temporary;

            temporary =
                buffer[left];

            buffer[left] =
                buffer[right];

            buffer[right] =
                temporary;

            ++left;
            --right;
        }
    }

    return
        vitte_c17_writer_write_n(
            writer,
            buffer,
            length);
}

/* ========================================================================= */
/* Quoted raw C comment helper                                               */
/* ========================================================================= */

bool
vitte_c17_writer_comment(
    vitte_c17_writer_t *writer,
    const char *text)
{
    size_t index;
    size_t length;

    if (text == NULL) {
        return false;
    }

    length = strlen(text);

    if (!vitte_c17_writer_write(
            writer,
            "/* ")) {
        return false;
    }

    /*
     * Prevent caller text from terminating the generated C comment.
     *
     * Replace the slash in every star-slash sequence with a space:
     *
     *     * /   (without the space in input)
     *
     * becomes:
     *
     *     * _
     *
     * This keeps the helper safe for arbitrary diagnostic/debug text.
     */
    for (index = 0u;
         index < length;
         ++index) {
        if (text[index] == '*' &&
            index + 1u < length &&
            text[index + 1u] == '/') {
            if (!vitte_c17_writer_write(
                    writer,
                    "*_")) {
                return false;
            }

            ++index;
            continue;
        }

        if (!vitte_c17_writer_write_char(
                writer,
                text[index])) {
            return false;
        }
    }

    return
        vitte_c17_writer_write(
            writer,
            " */");
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
