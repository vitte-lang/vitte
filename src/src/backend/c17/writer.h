#ifndef VITTE_SRC_BACKEND_C17_WRITER_H
#define VITTE_SRC_BACKEND_C17_WRITER_H

/*
 * Vitte Compiler
 * src/backend/c17/writer.h
 *
 * Low-level robust byte writer for the ISO C17 backend.
 *
 * This subsystem is deliberately independent from the C17 semantic emitter.
 * It knows how to write bytes reliably, but does not know how to emit C
 * declarations, expressions, statements, types, modules, programs, or
 * translation units.
 *
 * Responsibilities:
 *
 *   - callback sinks
 *   - FILE sinks
 *   - dynamically growing memory sinks
 *   - optional buffering
 *   - byte accounting
 *   - line/column tracking
 *   - automatic indentation
 *   - newline normalization
 *   - output limits
 *   - persistent error state
 *   - cancellation
 *   - flushing
 *   - memory checkpoints
 *   - memory rollback
 *   - arbitrary memory truncation
 *   - memory-buffer detachment
 *   - deterministic stream hashing
 *   - statistics
 *   - structural/deep validation
 *
 * Architecture:
 *
 *      declaration.c
 *      expression.c
 *      statement.c
 *      translation_unit.c
 *              |
 *              v
 *          backend.c
 *              |
 *              v
 *           writer.c
 *              |
 *      +-------+-------+
 *      |       |       |
 *      v       v       v
 *   callback  FILE   memory
 *
 * Ownership:
 *
 *   CALLBACK
 *     writer does not own user_data.
 *
 *   FILE
 *     writer owns FILE only when close_on_destroy=true.
 *
 *   MEMORY
 *     writer owns the dynamic buffer until memory_detach() or destroy().
 *
 * Thread safety:
 *
 *   A writer object is not internally synchronized.
 *
 *   Independent writer objects may be used concurrently.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#ifndef VITTE_C17_WRITER_MAGIC
#define VITTE_C17_WRITER_MAGIC \
    UINT64_C(0x5649545445575254)
#endif

#ifndef VITTE_C17_WRITER_DEAD_MAGIC
#define VITTE_C17_WRITER_DEAD_MAGIC \
    UINT64_C(0x4445414457525452)
#endif

#ifndef VITTE_C17_WRITER_FNV_OFFSET
#define VITTE_C17_WRITER_FNV_OFFSET \
    UINT64_C(14695981039346656037)
#endif

#ifndef VITTE_C17_WRITER_FNV_PRIME
#define VITTE_C17_WRITER_FNV_PRIME \
    UINT64_C(1099511628211)
#endif

#ifndef VITTE_C17_WRITER_DEFAULT_BUFFER_CAPACITY
#define VITTE_C17_WRITER_DEFAULT_BUFFER_CAPACITY \
    ((size_t)8192u)
#endif

#ifndef VITTE_C17_WRITER_DEFAULT_MEMORY_CAPACITY
#define VITTE_C17_WRITER_DEFAULT_MEMORY_CAPACITY \
    ((size_t)4096u)
#endif

#ifndef VITTE_C17_WRITER_MAX_BUFFER_CAPACITY
#define VITTE_C17_WRITER_MAX_BUFFER_CAPACITY \
    ((size_t)(16u * 1024u * 1024u))
#endif

#ifndef VITTE_C17_WRITER_MAX_MEMORY_INITIAL_CAPACITY
#define VITTE_C17_WRITER_MAX_MEMORY_INITIAL_CAPACITY \
    ((size_t)(64u * 1024u * 1024u))
#endif

#ifndef VITTE_C17_WRITER_MAX_INDENT_WIDTH
#define VITTE_C17_WRITER_MAX_INDENT_WIDTH \
    ((size_t)64u)
#endif

#ifndef VITTE_C17_WRITER_MAX_INDENT_DEPTH
#define VITTE_C17_WRITER_MAX_INDENT_DEPTH \
    ((size_t)(1024u * 1024u))
#endif

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_c17_writer
    vitte_c17_writer_t;

typedef struct vitte_c17_writer_config
    vitte_c17_writer_config_t;

typedef struct vitte_c17_writer_checkpoint
    vitte_c17_writer_checkpoint_t;

typedef struct vitte_c17_writer_stats
    vitte_c17_writer_stats_t;

/* ========================================================================= */
/* Callback API                                                              */
/* ========================================================================= */

/*
 * Return true only when all requested bytes were accepted.
 *
 * Partial writes are not represented by this callback interface.
 */
typedef bool
(*vitte_c17_writer_write_fn)(
    void *user_data,
    const char *data,
    size_t length);

/*
 * Optional callback sink flush operation.
 */
typedef bool
(*vitte_c17_writer_flush_fn)(
    void *user_data);

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_c17_writer_error {
    VITTE_C17_WRITER_ERROR_NONE = 0,

    VITTE_C17_WRITER_ERROR_INVALID_WRITER,

    VITTE_C17_WRITER_ERROR_INVALID_ARGUMENT,

    VITTE_C17_WRITER_ERROR_INVALID_STATE,

    VITTE_C17_WRITER_ERROR_OUTPUT,

    VITTE_C17_WRITER_ERROR_OUTPUT_LIMIT,

    VITTE_C17_WRITER_ERROR_LINE_LIMIT,

    VITTE_C17_WRITER_ERROR_COLUMN_LIMIT,

    VITTE_C17_WRITER_ERROR_INDENT_LIMIT,

    VITTE_C17_WRITER_ERROR_OVERFLOW,

    VITTE_C17_WRITER_ERROR_OUT_OF_MEMORY,

    VITTE_C17_WRITER_ERROR_CANCELLED,

    VITTE_C17_WRITER_ERROR_NOT_MEMORY_WRITER,

    VITTE_C17_WRITER_ERROR_INVALID_CHECKPOINT,

    VITTE_C17_WRITER_ERROR_CORRUPTION,

    VITTE_C17_WRITER_ERROR_COUNT
} vitte_c17_writer_error_t;

/* ========================================================================= */
/* Writer kind                                                               */
/* ========================================================================= */

typedef enum vitte_c17_writer_kind {
    VITTE_C17_WRITER_KIND_INVALID = 0,

    /*
     * Arbitrary user-provided sink.
     */
    VITTE_C17_WRITER_KIND_CALLBACK,

    /*
     * stdio FILE sink.
     */
    VITTE_C17_WRITER_KIND_FILE,

    /*
     * Dynamically growing in-memory sink.
     */
    VITTE_C17_WRITER_KIND_MEMORY,

    VITTE_C17_WRITER_KIND_COUNT
} vitte_c17_writer_kind_t;

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

struct vitte_c17_writer_config {
    /*
     * Spaces emitted for one indentation level.
     *
     * Zero disables visible indentation while still allowing the logical
     * indentation depth to be tracked.
     */
    size_t indent_width;

    /*
     * Maximum logical indentation depth.
     */
    size_t max_indent;

    /*
     * Hard maximum generated bytes.
     *
     * Zero means unlimited.
     */
    size_t max_output_bytes;

    /*
     * Maximum logical line number.
     *
     * Zero means unlimited.
     */
    size_t max_lines;

    /*
     * Maximum logical column.
     *
     * Zero means unlimited.
     */
    size_t max_column;

    /*
     * Intermediate buffering for callback/FILE sinks.
     *
     * Zero disables intermediate buffering.
     *
     * Memory writers do not use this buffer because their sink is already a
     * dynamic memory buffer.
     */
    size_t buffer_capacity;

    /*
     * Initial memory-writer allocation.
     *
     * The memory writer grows automatically as required.
     */
    size_t memory_initial_capacity;

    /*
     * Automatically insert indentation when non-empty output begins at the
     * start of a logical line.
     */
    bool auto_indent;

    /*
     * Normalize:
     *
     *   CR
     *   LF
     *   CRLF
     *
     * to LF.
     */
    bool normalize_newlines;

    /*
     * Flush callback/FILE output after every generated newline.
     *
     * Primarily useful for interactive diagnostics/debugging. Normally false
     * for compiler output performance.
     */
    bool flush_on_newline;

    /*
     * Maintain deterministic FNV-1a hash of logical generated output.
     */
    bool track_hash;
};

/* ========================================================================= */
/* Callback sink                                                             */
/* ========================================================================= */

typedef struct vitte_c17_writer_callback_sink {
    vitte_c17_writer_write_fn write;

    vitte_c17_writer_flush_fn flush;

    void *user_data;
} vitte_c17_writer_callback_sink_t;

/* ========================================================================= */
/* FILE sink                                                                 */
/* ========================================================================= */

typedef struct vitte_c17_writer_file_sink {
    FILE *stream;

    /*
     * fclose(stream) during writer_destroy().
     */
    bool close_on_destroy;
} vitte_c17_writer_file_sink_t;

/* ========================================================================= */
/* Memory sink                                                               */
/* ========================================================================= */

typedef struct vitte_c17_writer_memory_sink {
    unsigned char *data;

    /*
     * Number of generated bytes, excluding trailing NUL.
     */
    size_t length;

    /*
     * Allocated bytes including space for the trailing NUL.
     */
    size_t capacity;
} vitte_c17_writer_memory_sink_t;

/* ========================================================================= */
/* Sink union                                                                */
/* ========================================================================= */

typedef union vitte_c17_writer_sink {
    vitte_c17_writer_callback_sink_t callback;

    vitte_c17_writer_file_sink_t file;

    vitte_c17_writer_memory_sink_t memory;
} vitte_c17_writer_sink_t;

/* ========================================================================= */
/* Memory checkpoint                                                         */
/* ========================================================================= */

/*
 * A checkpoint is meaningful only for a memory writer.
 *
 * It represents enough logical state to restore the writer exactly to the
 * captured output position without rescanning the buffer.
 *
 * A checkpoint becomes invalid after:
 *
 *   - reset()
 *   - memory_detach()
 *   - destroy()
 *
 * because those operations change the writer generation.
 */
struct vitte_c17_writer_checkpoint {
    bool valid;

    uint64_t generation;

    size_t length;

    size_t bytes_written;

    size_t line;

    size_t column;

    size_t maximum_line;

    size_t maximum_column;

    size_t indent_level;

    size_t maximum_indent;

    bool at_line_start;

    bool last_was_cr;

    uint64_t hash;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

struct vitte_c17_writer_stats {
    bool valid;

    vitte_c17_writer_kind_t kind;

    /*
     * Logical generated output.
     *
     * For buffered external sinks this may include bytes that have not yet
     * reached the underlying sink.
     */
    size_t bytes_written;

    size_t line;

    size_t column;

    size_t maximum_line;

    size_t maximum_column;

    size_t indent_level;

    size_t maximum_indent;

    /*
     * Bytes waiting in callback/FILE intermediate buffer.
     */
    size_t buffered_bytes;

    /*
     * Memory writer state.
     *
     * Zero for non-memory writers.
     */
    size_t memory_length;

    size_t memory_capacity;

    /*
     * FNV-1a hash of logical output.
     *
     * Zero when track_hash=false.
     */
    uint64_t hash;

    bool failed;

    bool cancelled;

    uint64_t generation;
};

/* ========================================================================= */
/* Writer                                                                    */
/* ========================================================================= */

struct vitte_c17_writer {
    uint64_t magic;

    vitte_c17_writer_kind_t kind;

    vitte_c17_writer_config_t config;

    vitte_c17_writer_error_t last_error;

    /*
     * errno captured from FILE operations where meaningful.
     */
    int system_error;

    vitte_c17_writer_sink_t sink;

    /* --------------------------------------------------------------------- */
    /* Intermediate callback/FILE buffer                                     */
    /* --------------------------------------------------------------------- */

    unsigned char *buffer;

    size_t buffer_length;

    size_t buffer_capacity;

    /* --------------------------------------------------------------------- */
    /* Logical output position                                               */
    /* --------------------------------------------------------------------- */

    size_t bytes_written;

    /*
     * 1-based logical line and column.
     *
     * Empty output starts at:
     *
     *   line   = 1
     *   column = 1
     */
    size_t line;

    size_t column;

    size_t maximum_line;

    size_t maximum_column;

    /* --------------------------------------------------------------------- */
    /* Indentation                                                           */
    /* --------------------------------------------------------------------- */

    size_t indent_level;

    size_t maximum_indent;

    /* --------------------------------------------------------------------- */
    /* Lexical stream state                                                  */
    /* --------------------------------------------------------------------- */

    bool at_line_start;

    /*
     * Used only for CRLF normalization across arbitrary write boundaries.
     */
    bool last_was_cr;

    /* --------------------------------------------------------------------- */
    /* Failure/cancellation                                                  */
    /* --------------------------------------------------------------------- */

    bool failed;

    bool cancelled;

    /* --------------------------------------------------------------------- */
    /* Stream fingerprint                                                    */
    /* --------------------------------------------------------------------- */

    uint64_t hash;

    /* --------------------------------------------------------------------- */
    /* Lifecycle                                                             */
    /* --------------------------------------------------------------------- */

    uint64_t generation;
};

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_c17_writer_error_name(
    vitte_c17_writer_error_t error);

const char *
vitte_c17_writer_kind_name(
    vitte_c17_writer_kind_t kind);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_c17_writer_config_t
vitte_c17_writer_config_default(void);

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

bool
vitte_c17_writer_init_callback(
    vitte_c17_writer_t *writer,
    vitte_c17_writer_write_fn write_fn,
    vitte_c17_writer_flush_fn flush_fn,
    void *user_data,
    const vitte_c17_writer_config_t *config);

bool
vitte_c17_writer_init_file(
    vitte_c17_writer_t *writer,
    FILE *stream,
    bool close_on_destroy,
    const vitte_c17_writer_config_t *config);

bool
vitte_c17_writer_init_memory(
    vitte_c17_writer_t *writer,
    const vitte_c17_writer_config_t *config);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Reset logical writer state.
 *
 * MEMORY:
 *   clears the memory output while retaining capacity.
 *
 * CALLBACK / FILE:
 *   clears uncommitted intermediate buffered output and logical counters.
 *   Bytes already delivered to the external sink cannot be rolled back.
 */
bool
vitte_c17_writer_reset(
    vitte_c17_writer_t *writer);

/*
 * Best-effort flush followed by resource destruction.
 *
 * For FILE writers with close_on_destroy=true, fclose() is performed.
 *
 * Returns false if flushing or closing failed.
 */
bool
vitte_c17_writer_destroy(
    vitte_c17_writer_t *writer);

/* ========================================================================= */
/* Structural validity                                                       */
/* ========================================================================= */

bool
vitte_c17_writer_is_valid(
    const vitte_c17_writer_t *writer);

/*
 * Deeper consistency validation.
 */
bool
vitte_c17_writer_validate(
    const vitte_c17_writer_t *writer);

/* ========================================================================= */
/* Output                                                                    */
/* ========================================================================= */

bool
vitte_c17_writer_write_n(
    vitte_c17_writer_t *writer,
    const char *data,
    size_t length);

bool
vitte_c17_writer_write(
    vitte_c17_writer_t *writer,
    const char *text);

bool
vitte_c17_writer_write_char(
    vitte_c17_writer_t *writer,
    char character);

bool
vitte_c17_writer_repeat(
    vitte_c17_writer_t *writer,
    char character,
    size_t count);

bool
vitte_c17_writer_write_space(
    vitte_c17_writer_t *writer);

bool
vitte_c17_writer_write_tab(
    vitte_c17_writer_t *writer);

bool
vitte_c17_writer_write_uint64(
    vitte_c17_writer_t *writer,
    uint64_t value);

bool
vitte_c17_writer_write_size(
    vitte_c17_writer_t *writer,
    size_t value);

/* ========================================================================= */
/* Lines                                                                     */
/* ========================================================================= */

bool
vitte_c17_writer_newline(
    vitte_c17_writer_t *writer);

/*
 * Guarantee one empty line after the current logical output position.
 *
 * If currently in the middle of a line:
 *
 *     current text\n
 *     \n
 *
 * If already at line start:
 *
 *     \n
 */
bool
vitte_c17_writer_blank_line(
    vitte_c17_writer_t *writer);

/* ========================================================================= */
/* Indentation                                                               */
/* ========================================================================= */

bool
vitte_c17_writer_indent(
    vitte_c17_writer_t *writer);

bool
vitte_c17_writer_dedent(
    vitte_c17_writer_t *writer);

bool
vitte_c17_writer_set_indent(
    vitte_c17_writer_t *writer,
    size_t level);

/* ========================================================================= */
/* Flush                                                                     */
/* ========================================================================= */

bool
vitte_c17_writer_flush(
    vitte_c17_writer_t *writer);

/* ========================================================================= */
/* Cancellation                                                              */
/* ========================================================================= */

void
vitte_c17_writer_cancel(
    vitte_c17_writer_t *writer);

/* ========================================================================= */
/* Memory writer                                                             */
/* ========================================================================= */

/*
 * Return NUL-terminated memory output.
 *
 * The trailing NUL is not included in memory_length().
 *
 * Returned pointer remains owned by the writer and may become invalid after
 * any subsequent operation that reallocates, resets, detaches, or destroys
 * the writer.
 */
const char *
vitte_c17_writer_memory_data(
    const vitte_c17_writer_t *writer);

size_t
vitte_c17_writer_memory_length(
    const vitte_c17_writer_t *writer);

size_t
vitte_c17_writer_memory_capacity(
    const vitte_c17_writer_t *writer);

/*
 * Ensure capacity for at least `capacity` output bytes plus the internal
 * trailing NUL.
 */
bool
vitte_c17_writer_memory_reserve(
    vitte_c17_writer_t *writer,
    size_t capacity);

/*
 * Arbitrarily truncate memory output and recompute position/hash metadata.
 */
bool
vitte_c17_writer_memory_truncate(
    vitte_c17_writer_t *writer,
    size_t length);

/*
 * Transfer ownership of the current NUL-terminated memory buffer to caller.
 *
 * Caller must free() the returned pointer.
 *
 * The writer attempts to allocate a fresh empty buffer so it can remain
 * reusable. Even if that allocation fails, the detached returned buffer
 * remains valid and caller-owned.
 */
char *
vitte_c17_writer_memory_detach(
    vitte_c17_writer_t *writer,
    size_t *length);

/* ========================================================================= */
/* Memory transactions                                                       */
/* ========================================================================= */

vitte_c17_writer_checkpoint_t
vitte_c17_writer_checkpoint(
    const vitte_c17_writer_t *writer);

/*
 * Restore a memory writer to an earlier checkpoint.
 *
 * This also explicitly clears failure/cancellation state.
 */
bool
vitte_c17_writer_rollback(
    vitte_c17_writer_t *writer,
    const vitte_c17_writer_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Comments                                                                  */
/* ========================================================================= */

/*
 * Emit:
 *
 *     slash-star text star-slash
 *
 * while preventing embedded comment terminators in caller-provided text.
 *
 * No newline is appended automatically.
 */
bool
vitte_c17_writer_comment(
    vitte_c17_writer_t *writer,
    const char *text);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_c17_writer_stats_t
vitte_c17_writer_stats(
    const vitte_c17_writer_t *writer);

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

vitte_c17_writer_error_t
vitte_c17_writer_last_error(
    const vitte_c17_writer_t *writer);

/*
 * errno captured by a failed FILE operation.
 *
 * Zero when unavailable/not applicable.
 */
int
vitte_c17_writer_system_error(
    const vitte_c17_writer_t *writer);

/*
 * Clear last_error only when the writer is not failed/cancelled.
 *
 * Failed writers require reset() or, for memory writers, rollback().
 */
void
vitte_c17_writer_clear_error(
    vitte_c17_writer_t *writer);

/* ========================================================================= */
/* Inline kind helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_c17_writer_is_callback(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->kind ==
            VITTE_C17_WRITER_KIND_CALLBACK;
}

static inline bool
vitte_c17_writer_is_file(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->kind ==
            VITTE_C17_WRITER_KIND_FILE;
}

static inline bool
vitte_c17_writer_is_memory(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->kind ==
            VITTE_C17_WRITER_KIND_MEMORY;
}

/* ========================================================================= */
/* Inline state helpers                                                      */
/* ========================================================================= */

static inline bool
vitte_c17_writer_has_failed(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->failed;
}

static inline bool
vitte_c17_writer_is_cancelled(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->cancelled;
}

static inline bool
vitte_c17_writer_has_error(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->last_error !=
            VITTE_C17_WRITER_ERROR_NONE;
}

static inline bool
vitte_c17_writer_is_usable(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        !writer->failed &&
        !writer->cancelled;
}

/* ========================================================================= */
/* Inline position helpers                                                   */
/* ========================================================================= */

static inline size_t
vitte_c17_writer_bytes_written(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC
            ? writer->bytes_written
            : 0u;
}

static inline size_t
vitte_c17_writer_line(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC
            ? writer->line
            : 0u;
}

static inline size_t
vitte_c17_writer_column(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC
            ? writer->column
            : 0u;
}

static inline bool
vitte_c17_writer_at_line_start(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->at_line_start;
}

/* ========================================================================= */
/* Inline indentation helpers                                                */
/* ========================================================================= */

static inline size_t
vitte_c17_writer_indent_level(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC
            ? writer->indent_level
            : 0u;
}

static inline size_t
vitte_c17_writer_indent_width(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC
            ? writer->config.indent_width
            : 0u;
}

/* ========================================================================= */
/* Inline buffer helpers                                                     */
/* ========================================================================= */

static inline size_t
vitte_c17_writer_buffered_bytes(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC
            ? writer->buffer_length
            : 0u;
}

static inline bool
vitte_c17_writer_has_buffered_output(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->buffer_length != 0u;
}

/* ========================================================================= */
/* Inline hash helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_c17_writer_tracks_hash(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->config.track_hash;
}

static inline uint64_t
vitte_c17_writer_hash(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->config.track_hash
            ? writer->hash
            : UINT64_C(0);
}

static inline uint64_t
vitte_c17_writer_generation(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC
            ? writer->generation
            : UINT64_C(0);
}

/* ========================================================================= */
/* Inline limit helpers                                                      */
/* ========================================================================= */

static inline bool
vitte_c17_writer_has_output_limit(
    const vitte_c17_writer_t *writer)
{
    return
        writer != NULL &&
        writer->magic ==
            VITTE_C17_WRITER_MAGIC &&
        writer->config.max_output_bytes != 0u;
}

static inline size_t
vitte_c17_writer_output_remaining(
    const vitte_c17_writer_t *writer)
{
    if (writer == NULL ||
        writer->magic !=
            VITTE_C17_WRITER_MAGIC) {
        return 0u;
    }

    if (writer->config.max_output_bytes == 0u) {
        return SIZE_MAX;
    }

    if (writer->bytes_written >=
        writer->config.max_output_bytes) {
        return 0u;
    }

    return
        writer->config.max_output_bytes -
        writer->bytes_written;
}

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte C17 writer requires 64-bit uint64_t");

_Static_assert(
    VITTE_C17_WRITER_ERROR_NONE == 0,
    "writer no-error value must remain zero");

_Static_assert(
    VITTE_C17_WRITER_KIND_INVALID == 0,
    "invalid writer kind must remain zero");

_Static_assert(
    VITTE_C17_WRITER_DEFAULT_BUFFER_CAPACITY <=
        VITTE_C17_WRITER_MAX_BUFFER_CAPACITY,
    "default writer buffer exceeds maximum");

_Static_assert(
    VITTE_C17_WRITER_DEFAULT_MEMORY_CAPACITY <=
        VITTE_C17_WRITER_MAX_MEMORY_INITIAL_CAPACITY,
    "default memory writer capacity exceeds maximum");

_Static_assert(
    VITTE_C17_WRITER_MAX_INDENT_WIDTH != 0u,
    "maximum indentation width must not be zero");

_Static_assert(
    VITTE_C17_WRITER_MAX_INDENT_DEPTH != 0u,
    "maximum indentation depth must not be zero");

_Static_assert(
    VITTE_C17_WRITER_FNV_OFFSET != UINT64_C(0),
    "FNV offset basis must not be zero");

_Static_assert(
    VITTE_C17_WRITER_FNV_PRIME != UINT64_C(0),
    "FNV prime must not be zero");

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_BACKEND_C17_WRITER_H */
