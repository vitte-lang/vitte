#ifndef VITTE_SRC_BACKEND_C17_BACKEND_H
#define VITTE_SRC_BACKEND_C17_BACKEND_H

/*
 * Vitte Compiler
 * src/backend/c17/backend.h
 *
 * Core ISO C17 backend orchestration API.
 *
 * Responsibilities:
 *
 *   - backend lifecycle
 *   - C17 generation state
 *   - target description
 *   - output abstraction
 *   - deterministic section ordering
 *   - indentation
 *   - source locations
 *   - source-map integration
 *   - backend diagnostics
 *   - output limits
 *   - cancellation
 *   - generation statistics
 *   - FILE output
 *   - memory output
 *   - translation-unit driver
 *
 * This layer does not define the lowering rules for every Vitte construct.
 * Dedicated backend modules consume this API.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Configuration constants                                                   */
/* ========================================================================= */

#ifndef VITTE_C17_DEFAULT_INDENT_WIDTH
#define VITTE_C17_DEFAULT_INDENT_WIDTH ((size_t)4u)
#endif

#ifndef VITTE_C17_DEFAULT_MAX_OUTPUT_BYTES
#define VITTE_C17_DEFAULT_MAX_OUTPUT_BYTES SIZE_MAX
#endif

#ifndef VITTE_C17_DEFAULT_MAX_IDENTIFIER_BYTES
#define VITTE_C17_DEFAULT_MAX_IDENTIFIER_BYTES ((size_t)4096u)
#endif

#ifndef VITTE_C17_DEFAULT_MAX_INDENT_DEPTH
#define VITTE_C17_DEFAULT_MAX_INDENT_DEPTH ((size_t)4096u)
#endif

#ifndef VITTE_C17_DEFAULT_MAX_ERRORS
#define VITTE_C17_DEFAULT_MAX_ERRORS ((size_t)100u)
#endif

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_c17_backend
    vitte_c17_backend_t;

typedef struct vitte_c17_backend_config
    vitte_c17_backend_config_t;

typedef struct vitte_c17_target
    vitte_c17_target_t;

typedef struct vitte_c17_writer
    vitte_c17_writer_t;

typedef struct vitte_c17_source_location
    vitte_c17_source_location_t;

typedef struct vitte_c17_backend_stats
    vitte_c17_backend_stats_t;

typedef struct vitte_c17_memory_writer
    vitte_c17_memory_writer_t;

/* ========================================================================= */
/* Backend errors                                                            */
/* ========================================================================= */

typedef enum vitte_c17_backend_error {
    VITTE_C17_BACKEND_ERROR_NONE = 0,

    VITTE_C17_BACKEND_ERROR_INVALID_BACKEND,
    VITTE_C17_BACKEND_ERROR_INVALID_ARGUMENT,
    VITTE_C17_BACKEND_ERROR_INVALID_STATE,

    VITTE_C17_BACKEND_ERROR_INVALID_CONFIG,
    VITTE_C17_BACKEND_ERROR_INVALID_TARGET,

    VITTE_C17_BACKEND_ERROR_OUTPUT,
    VITTE_C17_BACKEND_ERROR_OUTPUT_LIMIT,

    VITTE_C17_BACKEND_ERROR_OVERFLOW,
    VITTE_C17_BACKEND_ERROR_OUT_OF_MEMORY,

    VITTE_C17_BACKEND_ERROR_UNSUPPORTED,

    VITTE_C17_BACKEND_ERROR_CANCELLED,

    VITTE_C17_BACKEND_ERROR_TOO_MANY_ERRORS,

    VITTE_C17_BACKEND_ERROR_INTERNAL,

    VITTE_C17_BACKEND_ERROR_COUNT
} vitte_c17_backend_error_t;

/* ========================================================================= */
/* Backend state                                                             */
/* ========================================================================= */

typedef enum vitte_c17_backend_state {
    VITTE_C17_BACKEND_STATE_UNINITIALIZED = 0,

    VITTE_C17_BACKEND_STATE_READY,

    VITTE_C17_BACKEND_STATE_GENERATING,

    VITTE_C17_BACKEND_STATE_FINISHED,

    VITTE_C17_BACKEND_STATE_FAILED,

    VITTE_C17_BACKEND_STATE_CANCELLED,

    VITTE_C17_BACKEND_STATE_DESTROYED,

    VITTE_C17_BACKEND_STATE_COUNT
} vitte_c17_backend_state_t;

/* ========================================================================= */
/* Target architecture                                                       */
/* ========================================================================= */

typedef enum vitte_c17_architecture {
    VITTE_C17_ARCH_UNKNOWN = 0,

    VITTE_C17_ARCH_X86,
    VITTE_C17_ARCH_X86_64,

    VITTE_C17_ARCH_ARM,
    VITTE_C17_ARCH_AARCH64,

    VITTE_C17_ARCH_RISCV,

    VITTE_C17_ARCH_COUNT
} vitte_c17_architecture_t;

/* ========================================================================= */
/* Target environment                                                        */
/* ========================================================================= */

typedef enum vitte_c17_environment {
    VITTE_C17_ENVIRONMENT_UNKNOWN = 0,

    VITTE_C17_ENVIRONMENT_LINUX,
    VITTE_C17_ENVIRONMENT_DARWIN,
    VITTE_C17_ENVIRONMENT_WINDOWS,
    VITTE_C17_ENVIRONMENT_FREEBSD,

    VITTE_C17_ENVIRONMENT_COUNT
} vitte_c17_environment_t;

/* ========================================================================= */
/* Translation-unit sections                                                 */
/* ========================================================================= */

/*
 * Values intentionally encode canonical output ordering.
 *
 * With deterministic generation enabled, a later section cannot be emitted
 * before an earlier section.
 */
typedef enum vitte_c17_section {
    VITTE_C17_SECTION_NONE = 0,

    VITTE_C17_SECTION_BANNER,

    VITTE_C17_SECTION_FEATURE_MACROS,

    VITTE_C17_SECTION_INCLUDES,

    VITTE_C17_SECTION_FORWARD_DECLARATIONS,

    VITTE_C17_SECTION_TYPE_DECLARATIONS,

    VITTE_C17_SECTION_GLOBAL_DECLARATIONS,

    VITTE_C17_SECTION_RUNTIME_DECLARATIONS,

    VITTE_C17_SECTION_FUNCTION_DECLARATIONS,

    VITTE_C17_SECTION_RUNTIME_DEFINITIONS,

    VITTE_C17_SECTION_GLOBAL_DEFINITIONS,

    VITTE_C17_SECTION_FUNCTION_DEFINITIONS,

    VITTE_C17_SECTION_ENTRY,

    VITTE_C17_SECTION_EPILOGUE,

    VITTE_C17_SECTION_COUNT
} vitte_c17_section_t;

/* ========================================================================= */
/* Diagnostic level                                                          */
/* ========================================================================= */

typedef enum vitte_c17_backend_diagnostic_level {
    VITTE_C17_DIAGNOSTIC_NOTE = 0,
    VITTE_C17_DIAGNOSTIC_WARNING,
    VITTE_C17_DIAGNOSTIC_ERROR,

    VITTE_C17_DIAGNOSTIC_LEVEL_COUNT
} vitte_c17_backend_diagnostic_level_t;

/* ========================================================================= */
/* Source location                                                           */
/* ========================================================================= */

struct vitte_c17_source_location {
    uint32_t file_id;

    size_t begin;
    size_t end;

    bool valid;
};

/* ========================================================================= */
/* Writer abstraction                                                        */
/* ========================================================================= */

/*
 * Writer contract:
 *
 *   - return true when every requested byte was accepted
 *   - return false on output failure
 *   - data is not required to be NUL terminated
 *   - length may be zero
 */
typedef bool
(*vitte_c17_write_fn)(
    void *user_data,
    const char *data,
    size_t length);

struct vitte_c17_writer {
    vitte_c17_write_fn write;
    void *user_data;
};

/* ========================================================================= */
/* Diagnostic callback                                                       */
/* ========================================================================= */

typedef void
(*vitte_c17_diagnostic_fn)(
    void *user_data,
    vitte_c17_backend_diagnostic_level_t level,
    const char *code,
    const char *message,
    const vitte_c17_source_location_t *source);

/* ========================================================================= */
/* Source-map callback                                                       */
/* ========================================================================= */

/*
 * Records a Vitte -> generated-C mapping point.
 *
 * generated_line is 1-based.
 * generated_column is 0-based.
 * generated_offset is the byte offset in generated output.
 */
typedef void
(*vitte_c17_source_map_fn)(
    void *user_data,
    uint32_t source_file_id,
    size_t source_begin,
    size_t source_end,
    size_t generated_line,
    size_t generated_column,
    size_t generated_offset);

/* ========================================================================= */
/* Translation-unit emitter                                                  */
/* ========================================================================= */

typedef bool
(*vitte_c17_emit_translation_unit_fn)(
    vitte_c17_backend_t *backend,
    const void *translation_unit,
    void *user_data);

/* ========================================================================= */
/* Backend configuration                                                     */
/* ========================================================================= */

struct vitte_c17_backend_config {
    /*
     * Pretty-printer configuration.
     */
    size_t indent_width;
    size_t max_indent_depth;

    /*
     * Hard output limit.
     */
    size_t max_output_bytes;

    /*
     * Maximum accepted source identifier length.
     */
    size_t max_identifier_bytes;

    /*
     * Diagnostic error limit.
     */
    size_t max_errors;

    /*
     * Generated-file banner.
     */
    bool emit_banner;

    /*
     * Optional #line support.
     *
     * The generic backend stores this policy even if a dedicated source-map
     * module ultimately owns actual #line emission.
     */
    bool emit_line_directives;

    /*
     * Vitte -> C source mapping.
     */
    bool emit_source_map;

    /*
     * User-facing/generated comments.
     */
    bool emit_comments;

    /*
     * Backend section/debug comments.
     */
    bool emit_debug_comments;

    /*
     * Emit generated _Static_assert checks when lowering modules request them.
     */
    bool emit_static_assertions;

    /*
     * Emit required Vitte runtime support.
     */
    bool emit_runtime;

    /*
     * Human-readable output formatting.
     */
    bool pretty;

    /*
     * Require canonical monotonically ordered backend sections.
     */
    bool deterministic;

    /*
     * Require strict ISO C17 output unless a target-specific module
     * explicitly handles an extension.
     */
    bool strict_c17;

    /*
     * Stop generation after first backend error.
     */
    bool fail_fast;

    /*
     * Escape ?? sequences where necessary to avoid historical C trigraph
     * interpretation.
     */
    bool trigraph_safe_strings;

    /*
     * Reserved policy for dedicated string/unicode emitters.
     */
    bool escape_non_ascii;
};

/* ========================================================================= */
/* Target description                                                        */
/* ========================================================================= */

struct vitte_c17_target {
    vitte_c17_architecture_t architecture;
    vitte_c17_environment_t environment;

    uint32_t pointer_bits;
    uint32_t size_bits;
    uint32_t char_bits;

    bool little_endian;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

struct vitte_c17_backend_stats {
    /*
     * Generated output.
     */
    size_t output_bytes;
    size_t output_lines;

    /*
     * Maximum indentation reached.
     */
    size_t maximum_indent_depth;

    /*
     * Number of output sections entered.
     */
    size_t section_count;

    /*
     * Backend semantic/generation counters.
     */
    size_t type_count;
    size_t global_count;
    size_t function_count;

    size_t statement_count;
    size_t expression_count;

    /*
     * Source map.
     */
    size_t source_map_entries;

    /*
     * Diagnostics.
     */
    size_t error_count;
    size_t warning_count;
    size_t note_count;
};

/* ========================================================================= */
/* Backend object                                                            */
/* ========================================================================= */

struct vitte_c17_backend {
    uint64_t magic;

    vitte_c17_backend_state_t state;
    vitte_c17_backend_error_t last_error;

    vitte_c17_backend_config_t config;
    vitte_c17_target_t target;

    /*
     * Output destination.
     */
    vitte_c17_writer_t writer;

    /*
     * Current generated source position.
     *
     * line:
     *     1-based
     *
     * column:
     *     0-based
     */
    size_t line;
    size_t column;

    /*
     * Pretty printer.
     */
    size_t indent_depth;

    bool at_line_start;

    /*
     * Translation-unit section state.
     */
    vitte_c17_section_t current_section;
    vitte_c17_section_t last_section;

    /*
     * Current Vitte source location.
     */
    vitte_c17_source_location_t source;

    /*
     * Diagnostic callback.
     */
    vitte_c17_diagnostic_fn diagnostic;
    void *diagnostic_user_data;

    /*
     * Source-map callback.
     */
    vitte_c17_source_map_fn source_map;
    void *source_map_user_data;

    /*
     * Cooperative cancellation.
     */
    bool cancelled;

    /*
     * Generation statistics.
     */
    vitte_c17_backend_stats_t stats;
};

/* ========================================================================= */
/* Memory writer                                                             */
/* ========================================================================= */

struct vitte_c17_memory_writer {
    char *data;

    size_t length;
    size_t capacity;
};

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_c17_backend_error_name(
    vitte_c17_backend_error_t error);

const char *
vitte_c17_backend_state_name(
    vitte_c17_backend_state_t state);

const char *
vitte_c17_section_name(
    vitte_c17_section_t section);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_c17_backend_config_t
vitte_c17_backend_config_default(void);

bool
vitte_c17_backend_config_validate(
    const vitte_c17_backend_config_t *config);

/* ========================================================================= */
/* Target                                                                    */
/* ========================================================================= */

vitte_c17_target_t
vitte_c17_target_default(void);

bool
vitte_c17_target_validate(
    const vitte_c17_target_t *target);

/* ========================================================================= */
/* Writer                                                                    */
/* ========================================================================= */

vitte_c17_writer_t
vitte_c17_writer_make(
    vitte_c17_write_fn write,
    void *user_data);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_c17_backend_init(
    vitte_c17_backend_t *backend,
    const vitte_c17_backend_config_t *config,
    const vitte_c17_target_t *target,
    vitte_c17_writer_t writer);

/*
 * Enter GENERATING state and initialize one translation-unit generation.
 */
bool
vitte_c17_backend_begin(
    vitte_c17_backend_t *backend);

/*
 * Finish a successful generation.
 *
 * Requires:
 *
 *   - GENERATING state
 *   - no open section
 *   - indentation depth == 0
 *   - not cancelled
 */
bool
vitte_c17_backend_finish(
    vitte_c17_backend_t *backend);

/*
 * Prepare a finished/failed/cancelled backend for another generation.
 *
 * The writer, target, configuration and hooks are retained.
 */
bool
vitte_c17_backend_reset(
    vitte_c17_backend_t *backend);

/*
 * Destroy backend metadata.
 *
 * Does not fclose() FILE writers.
 * Does not destroy user callback state.
 */
void
vitte_c17_backend_destroy(
    vitte_c17_backend_t *backend);

/* ========================================================================= */
/* Validity                                                                  */
/* ========================================================================= */

bool
vitte_c17_backend_is_valid(
    const vitte_c17_backend_t *backend);

bool
vitte_c17_backend_validate(
    const vitte_c17_backend_t *backend);

/* ========================================================================= */
/* Error                                                                     */
/* ========================================================================= */

vitte_c17_backend_error_t
vitte_c17_backend_last_error(
    const vitte_c17_backend_t *backend);

void
vitte_c17_backend_clear_error(
    vitte_c17_backend_t *backend);

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

vitte_c17_backend_state_t
vitte_c17_backend_state(
    const vitte_c17_backend_t *backend);

vitte_c17_section_t
vitte_c17_backend_current_section(
    const vitte_c17_backend_t *backend);

/* ========================================================================= */
/* Hooks                                                                     */
/* ========================================================================= */

void
vitte_c17_backend_set_diagnostic_handler(
    vitte_c17_backend_t *backend,
    vitte_c17_diagnostic_fn handler,
    void *user_data);

void
vitte_c17_backend_set_source_map_handler(
    vitte_c17_backend_t *backend,
    vitte_c17_source_map_fn handler,
    void *user_data);

/* ========================================================================= */
/* Diagnostics                                                               */
/* ========================================================================= */

void
vitte_c17_backend_report(
    vitte_c17_backend_t *backend,
    vitte_c17_backend_diagnostic_level_t level,
    const char *code,
    const char *message);

/* ========================================================================= */
/* Source locations                                                          */
/* ========================================================================= */

void
vitte_c17_backend_set_source_location(
    vitte_c17_backend_t *backend,
    uint32_t file_id,
    size_t begin,
    size_t end);

void
vitte_c17_backend_clear_source_location(
    vitte_c17_backend_t *backend);

/*
 * Emit one source-map mapping point using the current Vitte source location
 * and current generated-C position.
 */
void
vitte_c17_backend_mark_source(
    vitte_c17_backend_t *backend);

/* ========================================================================= */
/* Sections                                                                  */
/* ========================================================================= */

bool
vitte_c17_backend_begin_section(
    vitte_c17_backend_t *backend,
    vitte_c17_section_t section);

bool
vitte_c17_backend_end_section(
    vitte_c17_backend_t *backend);

/* ========================================================================= */
/* Output                                                                    */
/* ========================================================================= */

bool
vitte_c17_backend_write_n(
    vitte_c17_backend_t *backend,
    const char *text,
    size_t length);

bool
vitte_c17_backend_write(
    vitte_c17_backend_t *backend,
    const char *text);

bool
vitte_c17_backend_write_char(
    vitte_c17_backend_t *backend,
    char character);

bool
vitte_c17_backend_newline(
    vitte_c17_backend_t *backend);

bool
vitte_c17_backend_blank_line(
    vitte_c17_backend_t *backend);

/* ========================================================================= */
/* C lexical emission                                                        */
/* ========================================================================= */

/*
 * Emit a valid C string literal.
 *
 * Arbitrary byte data is accepted.
 */
bool
vitte_c17_backend_write_c_string(
    vitte_c17_backend_t *backend,
    const unsigned char *data,
    size_t length);

/*
 * Emit a mangled C identifier.
 *
 * Current canonical prefix:
 *
 *     vitte_
 *
 * Invalid identifier bytes are encoded as:
 *
 *     _xNN
 */
bool
vitte_c17_backend_write_identifier(
    vitte_c17_backend_t *backend,
    const char *identifier,
    size_t length);

/* ========================================================================= */
/* Pretty printing                                                           */
/* ========================================================================= */

bool
vitte_c17_backend_indent(
    vitte_c17_backend_t *backend);

bool
vitte_c17_backend_dedent(
    vitte_c17_backend_t *backend);

bool
vitte_c17_backend_open_block(
    vitte_c17_backend_t *backend);

bool
vitte_c17_backend_close_block(
    vitte_c17_backend_t *backend,
    bool semicolon);

bool
vitte_c17_backend_comment(
    vitte_c17_backend_t *backend,
    const char *text);

/* ========================================================================= */
/* Cancellation                                                              */
/* ========================================================================= */

void
vitte_c17_backend_cancel(
    vitte_c17_backend_t *backend);

bool
vitte_c17_backend_is_cancelled(
    const vitte_c17_backend_t *backend);

/* ========================================================================= */
/* Generated position                                                        */
/* ========================================================================= */

size_t
vitte_c17_backend_line(
    const vitte_c17_backend_t *backend);

size_t
vitte_c17_backend_column(
    const vitte_c17_backend_t *backend);

size_t
vitte_c17_backend_output_offset(
    const vitte_c17_backend_t *backend);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_c17_backend_stats_t
vitte_c17_backend_stats(
    const vitte_c17_backend_t *backend);

void
vitte_c17_backend_record_type(
    vitte_c17_backend_t *backend);

void
vitte_c17_backend_record_global(
    vitte_c17_backend_t *backend);

void
vitte_c17_backend_record_function(
    vitte_c17_backend_t *backend);

void
vitte_c17_backend_record_statement(
    vitte_c17_backend_t *backend);

void
vitte_c17_backend_record_expression(
    vitte_c17_backend_t *backend);

/* ========================================================================= */
/* FILE writer                                                               */
/* ========================================================================= */

bool
vitte_c17_file_writer(
    void *user_data,
    const char *data,
    size_t length);

vitte_c17_writer_t
vitte_c17_writer_from_file(
    FILE *file);

/* ========================================================================= */
/* Memory writer                                                             */
/* ========================================================================= */

void
vitte_c17_memory_writer_init(
    vitte_c17_memory_writer_t *writer);

void
vitte_c17_memory_writer_destroy(
    vitte_c17_memory_writer_t *writer);

bool
vitte_c17_memory_writer_write(
    void *user_data,
    const char *data,
    size_t length);

vitte_c17_writer_t
vitte_c17_writer_from_memory(
    vitte_c17_memory_writer_t *writer);

/* ========================================================================= */
/* Translation-unit driver                                                   */
/* ========================================================================= */

/*
 * Convenience orchestration:
 *
 *     begin
 *       -> emitter
 *     finish
 *
 * The translation_unit pointer is intentionally opaque at this layer.
 * A concrete translation_unit.c/HIR/IR adapter owns its actual type.
 */
bool
vitte_c17_backend_generate(
    vitte_c17_backend_t *backend,
    const void *translation_unit,
    vitte_c17_emit_translation_unit_fn emitter,
    void *user_data);

/* ========================================================================= */
/* Inline state helpers                                                      */
/* ========================================================================= */

static inline bool
vitte_c17_backend_is_ready(
    const vitte_c17_backend_t *backend)
{
    return
        backend != NULL &&
        backend->state ==
            VITTE_C17_BACKEND_STATE_READY;
}

static inline bool
vitte_c17_backend_is_generating(
    const vitte_c17_backend_t *backend)
{
    return
        backend != NULL &&
        backend->state ==
            VITTE_C17_BACKEND_STATE_GENERATING;
}

static inline bool
vitte_c17_backend_is_finished(
    const vitte_c17_backend_t *backend)
{
    return
        backend != NULL &&
        backend->state ==
            VITTE_C17_BACKEND_STATE_FINISHED;
}

static inline bool
vitte_c17_backend_has_failed(
    const vitte_c17_backend_t *backend)
{
    return
        backend != NULL &&
        backend->state ==
            VITTE_C17_BACKEND_STATE_FAILED;
}

static inline bool
vitte_c17_backend_has_error(
    const vitte_c17_backend_t *backend)
{
    return
        backend != NULL &&
        backend->last_error !=
            VITTE_C17_BACKEND_ERROR_NONE;
}

/* ========================================================================= */
/* Inline statistics helpers                                                 */
/* ========================================================================= */

static inline size_t
vitte_c17_backend_error_count(
    const vitte_c17_backend_t *backend)
{
    if (backend == NULL) {
        return 0u;
    }

    return backend->stats.error_count;
}

static inline size_t
vitte_c17_backend_warning_count(
    const vitte_c17_backend_t *backend)
{
    if (backend == NULL) {
        return 0u;
    }

    return backend->stats.warning_count;
}

static inline bool
vitte_c17_backend_has_diagnostics(
    const vitte_c17_backend_t *backend)
{
    if (backend == NULL) {
        return false;
    }

    return
        backend->stats.error_count != 0u ||
        backend->stats.warning_count != 0u ||
        backend->stats.note_count != 0u;
}

/* ========================================================================= */
/* Inline source helpers                                                     */
/* ========================================================================= */

static inline bool
vitte_c17_backend_has_source_location(
    const vitte_c17_backend_t *backend)
{
    return
        backend != NULL &&
        backend->source.valid;
}

/* ========================================================================= */
/* Inline writer helpers                                                     */
/* ========================================================================= */

static inline bool
vitte_c17_writer_is_valid(
    vitte_c17_writer_t writer)
{
    return writer.write != NULL;
}

static inline const char *
vitte_c17_memory_writer_data(
    const vitte_c17_memory_writer_t *writer)
{
    if (writer == NULL ||
        writer->data == NULL) {
        return "";
    }

    return writer->data;
}

static inline size_t
vitte_c17_memory_writer_length(
    const vitte_c17_memory_writer_t *writer)
{
    if (writer == NULL) {
        return 0u;
    }

    return writer->length;
}

/* ========================================================================= */
/* Convenience macros                                                        */
/* ========================================================================= */

#define VITTE_C17_WRITE(backend, text) \
    vitte_c17_backend_write( \
        (backend), \
        (text))

#define VITTE_C17_NEWLINE(backend) \
    vitte_c17_backend_newline( \
        (backend))

#define VITTE_C17_INDENT(backend) \
    vitte_c17_backend_indent( \
        (backend))

#define VITTE_C17_DEDENT(backend) \
    vitte_c17_backend_dedent( \
        (backend))

#define VITTE_C17_BEGIN_SECTION(backend, section) \
    vitte_c17_backend_begin_section( \
        (backend), \
        (section))

#define VITTE_C17_END_SECTION(backend) \
    vitte_c17_backend_end_section( \
        (backend))

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte C17 backend requires 64-bit uint64_t");

_Static_assert(
    sizeof(uint32_t) == 4u,
    "Vitte C17 backend requires 32-bit uint32_t");

_Static_assert(
    VITTE_C17_BACKEND_ERROR_NONE == 0,
    "backend no-error value must remain zero");

_Static_assert(
    VITTE_C17_BACKEND_STATE_UNINITIALIZED == 0,
    "backend uninitialized state must remain zero");

_Static_assert(
    VITTE_C17_SECTION_NONE == 0,
    "backend no-section value must remain zero");

_Static_assert(
    VITTE_C17_ARCH_UNKNOWN == 0,
    "unknown architecture must remain zero");

_Static_assert(
    VITTE_C17_ENVIRONMENT_UNKNOWN == 0,
    "unknown environment must remain zero");

_Static_assert(
    VITTE_C17_DIAGNOSTIC_NOTE == 0,
    "diagnostic note level must remain zero");

_Static_assert(
    VITTE_C17_DEFAULT_INDENT_WIDTH != 0u,
    "C17 indentation width must not be zero");

_Static_assert(
    VITTE_C17_DEFAULT_MAX_INDENT_DEPTH != 0u,
    "C17 maximum indentation depth must not be zero");

_Static_assert(
    VITTE_C17_DEFAULT_MAX_IDENTIFIER_BYTES != 0u,
    "C17 maximum identifier length must not be zero");

_Static_assert(
    VITTE_C17_DEFAULT_MAX_ERRORS != 0u,
    "C17 maximum diagnostic error count must not be zero");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_BACKEND_C17_BACKEND_H */
