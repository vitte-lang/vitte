#ifndef VITTE_PRINTER_PRINTER_H
#define VITTE_PRINTER_PRINTER_H

/*
 * Vitte Compiler
 * src/printer/printer.h
 *
 * Public printer contract.
 *
 * This header is the single public contract for printer.c.
 *
 * Responsibilities:
 *   - AST printing;
 *   - token-stream printing;
 *   - parser diagnostic printing;
 *   - source-span rendering;
 *   - parser summaries;
 *   - FILE sinks;
 *   - dynamically allocated memory-buffer sinks;
 *   - callback sinks;
 *   - configurable formatting;
 *   - bounded output;
 *   - explicit lifecycle;
 *   - statistics;
 *   - validation.
 *
 * Design:
 *   - ISO C17;
 *   - C/C++ compatible API;
 *   - deterministic output;
 *   - parser.h is the AST/parser contract;
 *   - lexer types are inherited through parser.h;
 *   - no duplicated AST/token definitions;
 *   - no ownership of parser/token/source inputs;
 *   - explicit ownership of memory-buffer sink storage.
 */

#include "../parser/parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* API version                                                               */
/* ========================================================================= */

#define VITTE_PRINTER_API_VERSION_MAJOR 1u
#define VITTE_PRINTER_API_VERSION_MINOR 0u
#define VITTE_PRINTER_API_VERSION_PATCH 0u

#define VITTE_PRINTER_API_VERSION \
    ((VITTE_PRINTER_API_VERSION_MAJOR * 10000u) + \
     (VITTE_PRINTER_API_VERSION_MINOR * 100u) + \
     VITTE_PRINTER_API_VERSION_PATCH)

/* ========================================================================= */
/* Magic                                                                     */
/* ========================================================================= */

#define VITTE_PRINTER_MAGIC \
    UINT64_C(0x5649545450524e54)

#define VITTE_PRINTER_DEAD_MAGIC \
    UINT64_C(0x4445414450524e54)

/* ========================================================================= */
/* Defaults                                                                  */
/* ========================================================================= */

#define VITTE_PRINTER_DEFAULT_BUFFER_CAPACITY \
    ((size_t)4096u)

#define VITTE_PRINTER_DEFAULT_INDENT_WIDTH \
    ((size_t)4u)

#define VITTE_PRINTER_DEFAULT_TAB_WIDTH \
    ((size_t)4u)

#define VITTE_PRINTER_DEFAULT_MAX_DEPTH \
    ((size_t)4096u)

#define VITTE_PRINTER_DEFAULT_MAX_OUTPUT_BYTES \
    ((size_t)(256u * 1024u * 1024u))

/* ========================================================================= */
/* Hard configuration limits                                                 */
/* ========================================================================= */

#define VITTE_PRINTER_MAX_INDENT_WIDTH \
    ((size_t)64u)

#define VITTE_PRINTER_MAX_TAB_WIDTH \
    ((size_t)64u)

/* ========================================================================= */
/* Printer errors                                                            */
/* ========================================================================= */

typedef enum vitte_printer_error {
    VITTE_PRINTER_ERROR_NONE = 0,

    VITTE_PRINTER_ERROR_INVALID_ARGUMENT,
    VITTE_PRINTER_ERROR_INVALID_CONTEXT,
    VITTE_PRINTER_ERROR_INVALID_STATE,
    VITTE_PRINTER_ERROR_INVALID_SINK,

    VITTE_PRINTER_ERROR_INVALID_NODE,
    VITTE_PRINTER_ERROR_INVALID_TOKEN,
    VITTE_PRINTER_ERROR_INVALID_SOURCE,

    VITTE_PRINTER_ERROR_DEPTH_LIMIT,
    VITTE_PRINTER_ERROR_OUTPUT_LIMIT,

    VITTE_PRINTER_ERROR_OVERFLOW,
    VITTE_PRINTER_ERROR_OUT_OF_MEMORY,

    VITTE_PRINTER_ERROR_IO,
    VITTE_PRINTER_ERROR_CALLBACK,
    VITTE_PRINTER_ERROR_FORMAT,

    VITTE_PRINTER_ERROR_VALIDATION,
    VITTE_PRINTER_ERROR_INTERNAL,

    VITTE_PRINTER_ERROR_COUNT
} vitte_printer_error_t;

/* ========================================================================= */
/* Printer lifecycle states                                                  */
/* ========================================================================= */

typedef enum vitte_printer_state {
    VITTE_PRINTER_STATE_INVALID = 0,

    VITTE_PRINTER_STATE_READY,
    VITTE_PRINTER_STATE_PRINTING,
    VITTE_PRINTER_STATE_FAILED,
    VITTE_PRINTER_STATE_DESTROYED,

    VITTE_PRINTER_STATE_COUNT
} vitte_printer_state_t;

/* ========================================================================= */
/* Printer modes                                                             */
/* ========================================================================= */

typedef enum vitte_printer_mode {
    /*
     * Compact S-expression representation.
     *
     * Example:
     *
     *   (translation_unit
     *     (proc_decl value="main"
     *       (block ...)))
     *
     * Actual compact output is emitted on one logical line unless limited
     * by the caller's output sink.
     */
    VITTE_PRINTER_MODE_COMPACT = 0,

    /*
     * Human-readable representation.
     */
    VITTE_PRINTER_MODE_PRETTY,

    /*
     * Explicit AST tree representation.
     */
    VITTE_PRINTER_MODE_TREE,

    /*
     * Verbose representation intended for compiler development.
     */
    VITTE_PRINTER_MODE_DEBUG,

    VITTE_PRINTER_MODE_COUNT
} vitte_printer_mode_t;

/* ========================================================================= */
/* Sink kinds                                                                */
/* ========================================================================= */

typedef enum vitte_printer_sink_kind {
    VITTE_PRINTER_SINK_INVALID = 0,

    VITTE_PRINTER_SINK_FILE,
    VITTE_PRINTER_SINK_BUFFER,
    VITTE_PRINTER_SINK_CALLBACK,

    VITTE_PRINTER_SINK_COUNT
} vitte_printer_sink_kind_t;

/* ========================================================================= */
/* Callback sink                                                             */
/* ========================================================================= */

/*
 * Callback contract:
 *
 *   - `data` points to exactly `length` bytes to write.
 *   - returning exactly `length` means success.
 *   - returning any other value means failure.
 *   - callback must not retain `data`.
 */
typedef size_t
(*vitte_printer_write_fn)(
    void *userdata,
    const char *data,
    size_t length);

/*
 * Optional callback flush operation.
 *
 * Return true on success.
 */
typedef bool
(*vitte_printer_flush_fn)(
    void *userdata);

/* ========================================================================= */
/* Buffer sink                                                               */
/* ========================================================================= */

typedef struct vitte_printer_buffer {
    /*
     * Printer-owned allocation.
     *
     * When non-NULL, data[length] is always '\0'.
     */
    char *data;

    /*
     * Number of meaningful bytes, excluding trailing '\0'.
     */
    size_t length;

    /*
     * Allocated byte capacity, including space available for '\0'.
     */
    size_t capacity;
} vitte_printer_buffer_t;

/* ========================================================================= */
/* Callback sink state                                                       */
/* ========================================================================= */

typedef struct vitte_printer_callback {
    vitte_printer_write_fn write;
    vitte_printer_flush_fn flush;

    void *userdata;
} vitte_printer_callback_t;

/* ========================================================================= */
/* Sink                                                                      */
/* ========================================================================= */

typedef union vitte_printer_sink {
    /*
     * Borrowed FILE*.
     *
     * The printer never fclose()s this stream.
     */
    FILE *file;

    /*
     * Printer-owned dynamic memory sink.
     */
    vitte_printer_buffer_t buffer;

    /*
     * Borrowed callbacks and userdata.
     */
    vitte_printer_callback_t callback;
} vitte_printer_sink_t;

/* ========================================================================= */
/* Options                                                                   */
/* ========================================================================= */

typedef struct vitte_printer_options {
    /*
     * AST:
     *
     *     #42 binary_expr
     *
     * instead of:
     *
     *     binary_expr
     */
    bool show_node_ids;

    /*
     * Emit file/line/column metadata when available.
     */
    bool show_locations;

    /*
     * Emit byte source spans.
     */
    bool show_spans;

    /*
     * Emit extra token metadata such as lexeme length.
     */
    bool show_token_metadata;

    /*
     * Emit AST child counts.
     */
    bool show_child_counts;

    /*
     * Reserved formatting policy.
     *
     * Current printer.c always escapes control bytes where required for
     * structural safety. This flag allows future printer implementations to
     * request stricter escaping without changing the public ABI.
     */
    bool escape_non_printable;

    /*
     * Tab expansion width used by source diagnostic rendering.
     *
     * Must be in:
     *
     *   1 .. VITTE_PRINTER_MAX_TAB_WIDTH
     */
    size_t tab_width;
} vitte_printer_options_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_printer_stats {
    /*
     * Total bytes successfully committed to the sink.
     */
    uint64_t bytes_written;

    /*
     * Number of newline characters emitted through the printer's structured
     * newline operation.
     */
    uint64_t lines_written;

    /*
     * AST nodes successfully rendered.
     */
    uint64_t nodes_printed;

    /*
     * Lexer tokens successfully rendered.
     */
    uint64_t tokens_printed;

    /*
     * Parser diagnostics successfully rendered.
     */
    uint64_t diagnostics_printed;

    /*
     * Source spans/snippets successfully rendered.
     */
    uint64_t source_spans_printed;

    /*
     * Maximum indentation/tree depth reached.
     */
    uint64_t max_depth;

    /*
     * Successful/attempted flush operation count.
     *
     * Incremented after a successful flush.
     */
    uint64_t flushes;

    /*
     * Explicit validation attempts.
     */
    uint64_t validation_runs;

    /*
     * Failed explicit validations.
     */
    uint64_t validation_failures;

    /*
     * Dynamic allocations performed internally.
     */
    uint64_t allocations;

    /*
     * Dynamic reallocations performed internally.
     */
    uint64_t reallocations;

    /*
     * Failed allocation attempts.
     */
    uint64_t allocation_failures;
} vitte_printer_stats_t;

/* ========================================================================= */
/* Printer context                                                           */
/* ========================================================================= */

typedef struct vitte_printer {
    /*
     * Runtime context integrity marker.
     */
    uint64_t magic;

    /*
     * Lifecycle state.
     */
    vitte_printer_state_t state;

    /*
     * Most recent printer error.
     */
    vitte_printer_error_t last_error;

    /*
     * AST rendering mode.
     */
    vitte_printer_mode_t mode;

    /*
     * Active sink.
     */
    vitte_printer_sink_kind_t sink_kind;

    /*
     * Active sink payload.
     */
    vitte_printer_sink_t sink;

    /*
     * Formatting configuration.
     */
    vitte_printer_options_t options;

    /*
     * Number of spaces emitted for each tree indentation level.
     */
    size_t indent_width;

    /*
     * Maximum AST/tree recursion depth.
     */
    size_t max_depth;

    /*
     * Maximum number of bytes that may be written between initialization
     * and reset.
     */
    size_t max_output_bytes;

    /*
     * Current AST/tree indentation depth.
     */
    size_t depth;

    /*
     * Total bytes written during the current generation.
     */
    size_t bytes_written;

    /*
     * True when the next structured output should begin at column zero and
     * may therefore need indentation.
     */
    bool at_line_start;

    /*
     * Runtime counters.
     */
    vitte_printer_stats_t stats;

    /*
     * Incremented by reset so clients can invalidate stale views.
     */
    uint64_t generation;
} vitte_printer_t;

/* ========================================================================= */
/* Version helpers                                                           */
/* ========================================================================= */

static inline unsigned int
vitte_printer_api_version_major(void)
{
    return VITTE_PRINTER_API_VERSION_MAJOR;
}

static inline unsigned int
vitte_printer_api_version_minor(void)
{
    return VITTE_PRINTER_API_VERSION_MINOR;
}

static inline unsigned int
vitte_printer_api_version_patch(void)
{
    return VITTE_PRINTER_API_VERSION_PATCH;
}

/* ========================================================================= */
/* Name queries                                                              */
/* ========================================================================= */

const char *
vitte_printer_error_name(
    vitte_printer_error_t error);

const char *
vitte_printer_state_name(
    vitte_printer_state_t state);

const char *
vitte_printer_mode_name(
    vitte_printer_mode_t mode);

const char *
vitte_printer_token_kind_name(
    vitte_token_kind_t kind);

/* ========================================================================= */
/* Default configuration                                                     */
/* ========================================================================= */

vitte_printer_options_t
vitte_printer_default_options(void);

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

/*
 * Initialize a FILE-backed printer.
 *
 * Ownership:
 *   - `file` remains owned by the caller.
 *   - destroy does not fclose(file).
 */
bool
vitte_printer_init_file(
    vitte_printer_t *printer,
    FILE *file);

/*
 * Initialize a dynamically allocated memory-buffer printer.
 *
 * The buffer is owned by the printer until:
 *
 *   - vitte_printer_destroy(), or
 *   - vitte_printer_take_buffer().
 */
bool
vitte_printer_init_buffer(
    vitte_printer_t *printer);

/*
 * Initialize a callback-backed printer.
 *
 * `write_callback` is mandatory.
 * `flush_callback` may be NULL.
 */
bool
vitte_printer_init_callback(
    vitte_printer_t *printer,
    vitte_printer_write_fn write_callback,
    vitte_printer_flush_fn flush_callback,
    void *userdata);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Reset transient output state while preserving:
 *
 *   - sink;
 *   - mode;
 *   - formatting options;
 *   - configured limits;
 *   - allocated memory-buffer capacity.
 *
 * For buffer sinks, length becomes zero but the allocation is retained.
 */
bool
vitte_printer_reset(
    vitte_printer_t *printer);

/*
 * Destroy the printer.
 *
 * FILE and callback resources remain caller-owned.
 *
 * Buffer storage still owned by the printer is freed.
 */
void
vitte_printer_destroy(
    vitte_printer_t *printer);

/* ========================================================================= */
/* Context queries                                                           */
/* ========================================================================= */

bool
vitte_printer_is_valid(
    const vitte_printer_t *printer);

vitte_printer_error_t
vitte_printer_last_error(
    const vitte_printer_t *printer);

uint64_t
vitte_printer_generation(
    const vitte_printer_t *printer);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

bool
vitte_printer_set_options(
    vitte_printer_t *printer,
    const vitte_printer_options_t *options);

bool
vitte_printer_set_mode(
    vitte_printer_t *printer,
    vitte_printer_mode_t mode);

bool
vitte_printer_set_limits(
    vitte_printer_t *printer,
    size_t max_output_bytes,
    size_t max_depth);

bool
vitte_printer_set_indent_width(
    vitte_printer_t *printer,
    size_t indent_width);

/* ========================================================================= */
/* Token printing                                                            */
/* ========================================================================= */

bool
vitte_printer_print_token(
    vitte_printer_t *printer,
    const vitte_token_t *token);

bool
vitte_printer_print_tokens(
    vitte_printer_t *printer,
    const vitte_token_t *tokens,
    size_t token_count);

/* ========================================================================= */
/* AST printing                                                              */
/* ========================================================================= */

/*
 * Print one AST node and all of its descendants.
 */
bool
vitte_printer_print_ast_node(
    vitte_printer_t *printer,
    const vitte_parser_t *parser,
    vitte_ast_node_id_t id);

/*
 * Print parser->root and all descendants.
 */
bool
vitte_printer_print_ast(
    vitte_printer_t *printer,
    const vitte_parser_t *parser);

/* ========================================================================= */
/* Source rendering                                                          */
/* ========================================================================= */

/*
 * Render the source line containing span.begin and underline the relevant
 * region.
 *
 * `source` is borrowed and does not need to be NUL-terminated.
 *
 * Current implementation renders the first source line touched by the span.
 */
bool
vitte_printer_print_source_span(
    vitte_printer_t *printer,
    const char *source,
    size_t source_length,
    vitte_parser_span_t span,
    const char *label);

/* ========================================================================= */
/* Diagnostic printing                                                       */
/* ========================================================================= */

/*
 * Print one parser diagnostic.
 *
 * `source` may be NULL when source_length == 0. In that case only structured
 * diagnostic metadata is printed.
 */
bool
vitte_printer_print_diagnostic(
    vitte_printer_t *printer,
    const vitte_parser_diagnostic_t *diagnostic,
    const char *source,
    size_t source_length);

/*
 * Print every diagnostic currently owned by `parser`.
 */
bool
vitte_printer_print_diagnostics(
    vitte_printer_t *printer,
    const vitte_parser_t *parser,
    const char *source,
    size_t source_length);

/* ========================================================================= */
/* Parser summary                                                            */
/* ========================================================================= */

bool
vitte_printer_print_parser_summary(
    vitte_printer_t *printer,
    const vitte_parser_t *parser);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_printer_validate(
    vitte_printer_t *printer);

/* ========================================================================= */
/* Flush                                                                     */
/* ========================================================================= */

bool
vitte_printer_flush(
    vitte_printer_t *printer);

/* ========================================================================= */
/* Buffer sink access                                                        */
/* ========================================================================= */

/*
 * Return a borrowed NUL-terminated view.
 *
 * The pointer remains owned by the printer and may become invalid after:
 *
 *   - additional output causing realloc();
 *   - reset;
 *   - take_buffer;
 *   - destroy.
 */
const char *
vitte_printer_buffer_data(
    const vitte_printer_t *printer);

/*
 * Return buffer payload length excluding the trailing NUL byte.
 */
size_t
vitte_printer_buffer_length(
    const vitte_printer_t *printer);

/*
 * Transfer buffer ownership to the caller.
 *
 * The returned allocation must be released with free().
 *
 * After transfer the printer remains valid and may allocate a fresh buffer
 * on the next write.
 */
char *
vitte_printer_take_buffer(
    vitte_printer_t *printer,
    size_t *length);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_printer_stats_t
vitte_printer_stats(
    const vitte_printer_t *printer);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_printer_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_printer_error_is_valid(
    vitte_printer_error_t error)
{
    return error >= VITTE_PRINTER_ERROR_NONE &&
           error < VITTE_PRINTER_ERROR_COUNT;
}

static inline bool
vitte_printer_state_is_valid(
    vitte_printer_state_t state)
{
    return state > VITTE_PRINTER_STATE_INVALID &&
           state < VITTE_PRINTER_STATE_COUNT;
}

static inline bool
vitte_printer_mode_is_valid(
    vitte_printer_mode_t mode)
{
    return mode >= VITTE_PRINTER_MODE_COMPACT &&
           mode < VITTE_PRINTER_MODE_COUNT;
}

static inline bool
vitte_printer_sink_kind_is_valid(
    vitte_printer_sink_kind_t kind)
{
    return kind > VITTE_PRINTER_SINK_INVALID &&
           kind < VITTE_PRINTER_SINK_COUNT;
}

/* ========================================================================= */
/* Inline state helpers                                                      */
/* ========================================================================= */

static inline bool
vitte_printer_is_ready(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->state == VITTE_PRINTER_STATE_READY;
}

static inline bool
vitte_printer_is_printing(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->state == VITTE_PRINTER_STATE_PRINTING;
}

static inline bool
vitte_printer_has_failed(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->state == VITTE_PRINTER_STATE_FAILED;
}

static inline bool
vitte_printer_is_destroyed(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_DEAD_MAGIC &&
           printer->state == VITTE_PRINTER_STATE_DESTROYED;
}

/* ========================================================================= */
/* Inline sink helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_printer_is_file_sink(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->sink_kind == VITTE_PRINTER_SINK_FILE;
}

static inline bool
vitte_printer_is_buffer_sink(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->sink_kind == VITTE_PRINTER_SINK_BUFFER;
}

static inline bool
vitte_printer_is_callback_sink(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->sink_kind == VITTE_PRINTER_SINK_CALLBACK;
}

/* ========================================================================= */
/* Inline configuration queries                                              */
/* ========================================================================= */

static inline vitte_printer_mode_t
vitte_printer_mode(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return VITTE_PRINTER_MODE_COUNT;
    }

    return printer->mode;
}

static inline vitte_printer_sink_kind_t
vitte_printer_sink_kind(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return VITTE_PRINTER_SINK_INVALID;
    }

    return printer->sink_kind;
}

static inline size_t
vitte_printer_indent_width(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return 0u;
    }

    return printer->indent_width;
}

static inline size_t
vitte_printer_max_depth(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return 0u;
    }

    return printer->max_depth;
}

static inline size_t
vitte_printer_max_output_bytes(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return 0u;
    }

    return printer->max_output_bytes;
}

static inline size_t
vitte_printer_bytes_written(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return 0u;
    }

    return printer->bytes_written;
}

static inline size_t
vitte_printer_depth(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return 0u;
    }

    return printer->depth;
}

/* ========================================================================= */
/* Inline option queries                                                     */
/* ========================================================================= */

static inline bool
vitte_printer_shows_node_ids(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->options.show_node_ids;
}

static inline bool
vitte_printer_shows_locations(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->options.show_locations;
}

static inline bool
vitte_printer_shows_spans(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->options.show_spans;
}

static inline bool
vitte_printer_shows_token_metadata(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->options.show_token_metadata;
}

static inline bool
vitte_printer_shows_child_counts(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->options.show_child_counts;
}

static inline size_t
vitte_printer_tab_width(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return 0u;
    }

    return printer->options.tab_width;
}

/* ========================================================================= */
/* Inline buffer queries                                                     */
/* ========================================================================= */

static inline size_t
vitte_printer_buffer_capacity(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC ||
        printer->sink_kind != VITTE_PRINTER_SINK_BUFFER) {
        return 0u;
    }

    return printer->sink.buffer.capacity;
}

static inline bool
vitte_printer_buffer_is_empty(
    const vitte_printer_t *printer)
{
    return printer != NULL &&
           printer->magic == VITTE_PRINTER_MAGIC &&
           printer->sink_kind == VITTE_PRINTER_SINK_BUFFER &&
           printer->sink.buffer.length == 0u;
}

/* ========================================================================= */
/* Inline limit queries                                                      */
/* ========================================================================= */

static inline size_t
vitte_printer_output_bytes_remaining(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC ||
        printer->bytes_written >= printer->max_output_bytes) {
        return 0u;
    }

    return printer->max_output_bytes -
           printer->bytes_written;
}

static inline size_t
vitte_printer_depth_remaining(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC ||
        printer->depth >= printer->max_depth) {
        return 0u;
    }

    return printer->max_depth -
           printer->depth;
}

/* ========================================================================= */
/* Inline statistics queries                                                 */
/* ========================================================================= */

static inline uint64_t
vitte_printer_nodes_printed(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return UINT64_C(0);
    }

    return printer->stats.nodes_printed;
}

static inline uint64_t
vitte_printer_tokens_printed(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return UINT64_C(0);
    }

    return printer->stats.tokens_printed;
}

static inline uint64_t
vitte_printer_diagnostics_printed(
    const vitte_printer_t *printer)
{
    if (printer == NULL ||
        printer->magic != VITTE_PRINTER_MAGIC) {
        return UINT64_C(0);
    }

    return printer->stats.diagnostics_printed;
}

/* ========================================================================= */
/* Compile-time contract checks                                              */
/* ========================================================================= */

#if defined(__cplusplus)

static_assert(
    sizeof(uint64_t) == 8u,
    "printer requires 64-bit uint64_t");

static_assert(
    VITTE_PRINTER_API_VERSION_MAJOR == 1u,
    "unexpected printer API major version");

static_assert(
    VITTE_PRINTER_DEFAULT_INDENT_WIDTH > 0u,
    "default indentation must be non-zero");

static_assert(
    VITTE_PRINTER_DEFAULT_TAB_WIDTH > 0u,
    "default tab width must be non-zero");

static_assert(
    VITTE_PRINTER_DEFAULT_MAX_DEPTH > 0u,
    "default max depth must be non-zero");

static_assert(
    VITTE_PRINTER_DEFAULT_MAX_OUTPUT_BYTES > 0u,
    "default max output must be non-zero");

static_assert(
    VITTE_PRINTER_DEFAULT_INDENT_WIDTH <=
        VITTE_PRINTER_MAX_INDENT_WIDTH,
    "default indentation exceeds hard limit");

static_assert(
    VITTE_PRINTER_DEFAULT_TAB_WIDTH <=
        VITTE_PRINTER_MAX_TAB_WIDTH,
    "default tab width exceeds hard limit");

#else

_Static_assert(
    sizeof(uint64_t) == 8u,
    "printer requires 64-bit uint64_t");

_Static_assert(
    VITTE_PRINTER_API_VERSION_MAJOR == 1u,
    "unexpected printer API major version");

_Static_assert(
    VITTE_PRINTER_DEFAULT_INDENT_WIDTH > 0u,
    "default indentation must be non-zero");

_Static_assert(
    VITTE_PRINTER_DEFAULT_TAB_WIDTH > 0u,
    "default tab width must be non-zero");

_Static_assert(
    VITTE_PRINTER_DEFAULT_MAX_DEPTH > 0u,
    "default max depth must be non-zero");

_Static_assert(
    VITTE_PRINTER_DEFAULT_MAX_OUTPUT_BYTES > 0u,
    "default max output must be non-zero");

_Static_assert(
    VITTE_PRINTER_DEFAULT_INDENT_WIDTH <=
        VITTE_PRINTER_MAX_INDENT_WIDTH,
    "default indentation exceeds hard limit");

_Static_assert(
    VITTE_PRINTER_DEFAULT_TAB_WIDTH <=
        VITTE_PRINTER_MAX_TAB_WIDTH,
    "default tab width exceeds hard limit");

#endif

#ifdef __cplusplus
}
#endif

#endif /* VITTE_PRINTER_PRINTER_H */
