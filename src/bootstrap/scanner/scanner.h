#ifndef VITTE_SCANNER_SCANNER_H
#define VITTE_SCANNER_SCANNER_H

/*
 * Vitte Compiler
 * src/scanner/scanner.h
 *
 * Public source scanner contract.
 *
 * Synchronized with scanner.c.
 *
 * The scanner is the low-level source navigation layer. It performs bounded
 * byte traversal, strict UTF-8 decoding, line/column tracking, checkpoints,
 * source slicing and position reconstruction without performing lexical token
 * classification.
 *
 * Design:
 *   - ISO C17;
 *   - C/C++ compatible;
 *   - immutable borrowed source;
 *   - no NUL-termination requirement;
 *   - byte-precise offsets;
 *   - 1-based line/column/display-column values;
 *   - strict UTF-8 support;
 *   - CR, LF and CRLF support;
 *   - configurable tab stops;
 *   - deterministic checkpoints;
 *   - explicit lifecycle;
 *   - bounded source size;
 *   - deterministic fingerprints;
 *   - no lexer/parser dependency;
 *   - no dynamic allocation.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* API version                                                               */
/* ========================================================================= */

#define VITTE_SCANNER_API_VERSION_MAJOR 1u
#define VITTE_SCANNER_API_VERSION_MINOR 0u
#define VITTE_SCANNER_API_VERSION_PATCH 0u

#define VITTE_SCANNER_API_VERSION \
    ((VITTE_SCANNER_API_VERSION_MAJOR * 10000u) + \
     (VITTE_SCANNER_API_VERSION_MINOR * 100u) + \
     VITTE_SCANNER_API_VERSION_PATCH)

/* ========================================================================= */
/* Context magic                                                             */
/* ========================================================================= */

#define VITTE_SCANNER_MAGIC \
    UINT64_C(0x564954545343414e)

#define VITTE_SCANNER_DEAD_MAGIC \
    UINT64_C(0x444541445343414e)

/* ========================================================================= */
/* Fingerprint constants                                                     */
/* ========================================================================= */

#define VITTE_SCANNER_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_SCANNER_FNV_PRIME \
    UINT64_C(1099511628211)

/* ========================================================================= */
/* Defaults / limits                                                         */
/* ========================================================================= */

#define VITTE_SCANNER_DEFAULT_TAB_WIDTH \
    ((size_t)4u)

#define VITTE_SCANNER_MAX_TAB_WIDTH \
    ((size_t)64u)

#define VITTE_SCANNER_DEFAULT_MAX_SOURCE_BYTES \
    ((size_t)(1024u * 1024u * 1024u))

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_scanner_error {
    VITTE_SCANNER_ERROR_NONE = 0,

    VITTE_SCANNER_ERROR_INVALID_ARGUMENT,
    VITTE_SCANNER_ERROR_INVALID_CONTEXT,
    VITTE_SCANNER_ERROR_INVALID_STATE,

    VITTE_SCANNER_ERROR_SOURCE_TOO_LARGE,
    VITTE_SCANNER_ERROR_INVALID_UTF8,
    VITTE_SCANNER_ERROR_END_OF_INPUT,
    VITTE_SCANNER_ERROR_INVALID_OFFSET,
    VITTE_SCANNER_ERROR_INVALID_CHECKPOINT,

    VITTE_SCANNER_ERROR_OVERFLOW,

    VITTE_SCANNER_ERROR_VALIDATION,
    VITTE_SCANNER_ERROR_CORRUPTION,
    VITTE_SCANNER_ERROR_INTERNAL,

    VITTE_SCANNER_ERROR_COUNT
} vitte_scanner_error_t;

/* ========================================================================= */
/* Lifecycle states                                                          */
/* ========================================================================= */

typedef enum vitte_scanner_state {
    VITTE_SCANNER_STATE_INVALID = 0,

    VITTE_SCANNER_STATE_READY,
    VITTE_SCANNER_STATE_SCANNING,
    VITTE_SCANNER_STATE_DONE,
    VITTE_SCANNER_STATE_FAILED,
    VITTE_SCANNER_STATE_DESTROYED,

    VITTE_SCANNER_STATE_COUNT
} vitte_scanner_state_t;

/* ========================================================================= */
/* Position                                                                  */
/* ========================================================================= */

/*
 * Scanner positions represent boundaries between source scalars.
 *
 * offset:
 *   zero-based byte offset into the source.
 *
 * line:
 *   one-based logical line.
 *
 * column:
 *   one-based logical codepoint column.
 *
 * display_column:
 *   one-based column after tab-stop expansion.
 *
 * line_start_offset:
 *   byte offset at which the current logical line begins.
 */
typedef struct vitte_scanner_position {
    uint32_t file_id;

    size_t offset;

    size_t line;
    size_t column;
    size_t display_column;

    size_t line_start_offset;

    bool valid;
} vitte_scanner_position_t;

/* ========================================================================= */
/* Span                                                                      */
/* ========================================================================= */

/*
 * Half-open byte span:
 *
 *     [begin, end)
 *
 * line/column/display_column describe the beginning of the span.
 */
typedef struct vitte_scanner_span {
    uint32_t file_id;

    size_t begin;
    size_t end;

    size_t line;
    size_t column;
    size_t display_column;

    bool valid;
} vitte_scanner_span_t;

/* ========================================================================= */
/* Checkpoint                                                                */
/* ========================================================================= */

/*
 * A checkpoint is a complete lightweight navigation snapshot.
 *
 * It can only be restored into the scanner generation that created it.
 * Calling vitte_scanner_reset() invalidates all older checkpoints.
 */
typedef struct vitte_scanner_checkpoint {
    uint64_t generation;

    size_t offset;

    size_t line;
    size_t column;
    size_t display_column;

    size_t line_start_offset;

    bool valid;
} vitte_scanner_checkpoint_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_scanner_stats {
    /*
     * Number of source bytes consumed through advance operations.
     *
     * CRLF contributes two bytes.
     */
    uint64_t bytes_consumed;

    /*
     * Number of logical codepoints consumed.
     *
     * CRLF counts as one logical newline/codepoint operation.
     */
    uint64_t codepoints_consumed;

    /*
     * Logical newline count.
     */
    uint64_t newlines;

    /*
     * Number of CRLF pairs consumed as one logical newline.
     */
    uint64_t crlf_newlines;

    /*
     * Tab codepoints consumed.
     */
    uint64_t tabs;

    /*
     * Invalid UTF-8 sequences encountered by the advancing scanner.
     *
     * In strict mode, encountering one fails the scanner.
     * In permissive mode, one source byte is consumed as U+FFFD.
     */
    uint64_t invalid_utf8_sequences;

    /*
     * UTF-8 BOMs automatically skipped at source start.
     */
    uint64_t boms_skipped;

    /*
     * Successful checkpoint restore operations.
     */
    uint64_t restores;

    /*
     * Explicit full-source UTF-8 validation runs.
     */
    uint64_t utf8_validation_runs;

    /*
     * Failed full-source UTF-8 validations.
     */
    uint64_t utf8_validation_failures;

    /*
     * Explicit context validation runs.
     */
    uint64_t validation_runs;

    /*
     * Failed context validation runs.
     */
    uint64_t validation_failures;

    /*
     * Fingerprint operations.
     */
    uint64_t hash_runs;

    /*
     * Scanner operations that transitioned the context to FAILED.
     */
    uint64_t failures;
} vitte_scanner_stats_t;

/* ========================================================================= */
/* Scanner context                                                           */
/* ========================================================================= */

typedef struct vitte_scanner {
    /*
     * Runtime integrity marker.
     */
    uint64_t magic;

    /*
     * Lifecycle.
     */
    vitte_scanner_state_t state;

    /*
     * Most recent scanner error.
     */
    vitte_scanner_error_t last_error;

    /*
     * Borrowed immutable source bytes.
     *
     * May be NULL only when source_length == 0.
     */
    const unsigned char *source;

    /*
     * Exact byte length.
     */
    size_t source_length;

    /*
     * Source identity supplied by the caller.
     */
    uint32_t file_id;

    /*
     * Current byte offset.
     */
    size_t offset;

    /*
     * Current 1-based logical source position.
     */
    size_t line;
    size_t column;
    size_t display_column;

    /*
     * Byte offset of the current line's first byte.
     */
    size_t line_start_offset;

    /*
     * Tab-stop width.
     */
    size_t tab_width;

    /*
     * Maximum accepted source size.
     */
    size_t max_source_bytes;

    /*
     * true:
     *   malformed UTF-8 causes advance to fail.
     *
     * false:
     *   malformed input consumes one byte and yields U+FFFD.
     */
    bool strict_utf8;

    /*
     * When true, an initial UTF-8 BOM is not exposed as source content.
     */
    bool skip_utf8_bom;

    /*
     * Runtime statistics.
     */
    vitte_scanner_stats_t stats;

    /*
     * Checkpoint generation.
     *
     * Reset increments this value and invalidates old checkpoints.
     */
    uint64_t generation;
} vitte_scanner_t;

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_scanner_error_name(
    vitte_scanner_error_t error);

const char *
vitte_scanner_state_name(
    vitte_scanner_state_t state);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_scanner_init(
    vitte_scanner_t *scanner,
    const char *source,
    size_t source_length,
    uint32_t file_id);

bool
vitte_scanner_reset(
    vitte_scanner_t *scanner,
    const char *source,
    size_t source_length,
    uint32_t file_id);

void
vitte_scanner_destroy(
    vitte_scanner_t *scanner);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

bool
vitte_scanner_set_tab_width(
    vitte_scanner_t *scanner,
    size_t tab_width);

bool
vitte_scanner_set_max_source_bytes(
    vitte_scanner_t *scanner,
    size_t max_source_bytes);

bool
vitte_scanner_set_strict_utf8(
    vitte_scanner_t *scanner,
    bool strict_utf8);

/* ========================================================================= */
/* Context queries                                                           */
/* ========================================================================= */

bool
vitte_scanner_is_valid(
    const vitte_scanner_t *scanner);

vitte_scanner_error_t
vitte_scanner_last_error(
    const vitte_scanner_t *scanner);

uint64_t
vitte_scanner_generation(
    const vitte_scanner_t *scanner);

/* ========================================================================= */
/* Navigation queries                                                        */
/* ========================================================================= */

bool
vitte_scanner_is_eof(
    const vitte_scanner_t *scanner);

size_t
vitte_scanner_remaining(
    const vitte_scanner_t *scanner);

vitte_scanner_position_t
vitte_scanner_position(
    const vitte_scanner_t *scanner);

/* ========================================================================= */
/* Byte access                                                               */
/* ========================================================================= */

/*
 * Read a byte relative to the current byte offset without mutation.
 */
bool
vitte_scanner_peek_byte(
    const vitte_scanner_t *scanner,
    size_t lookahead,
    unsigned char *byte);

bool
vitte_scanner_current_byte(
    const vitte_scanner_t *scanner,
    unsigned char *byte);

/* ========================================================================= */
/* UTF-8 lookahead                                                           */
/* ========================================================================= */

/*
 * Look ahead by Unicode scalar count, not byte count.
 *
 * width receives the encoded byte width of the selected scalar.
 *
 * In permissive mode an invalid sequence is exposed as U+FFFD with width 1.
 */
bool
vitte_scanner_peek_codepoint(
    const vitte_scanner_t *scanner,
    size_t codepoint_lookahead,
    uint32_t *codepoint,
    size_t *width);

bool
vitte_scanner_current_codepoint(
    const vitte_scanner_t *scanner,
    uint32_t *codepoint,
    size_t *width);

/* ========================================================================= */
/* Advancement                                                               */
/* ========================================================================= */

/*
 * Consume one logical source element.
 *
 * UTF-8:
 *   consumes one Unicode scalar.
 *
 * LF:
 *   consumes one logical newline.
 *
 * CR:
 *   consumes one logical newline.
 *
 * CRLF:
 *   consumes both bytes as one logical newline.
 *
 * If codepoint is non-NULL, it receives the consumed scalar.
 *
 * For CRLF the returned codepoint is U+000D because CR begins the logical
 * newline sequence. Newline classification should use scanner position or
 * byte predicates rather than relying on that distinction.
 */
bool
vitte_scanner_advance(
    vitte_scanner_t *scanner,
    uint32_t *codepoint);

bool
vitte_scanner_advance_n(
    vitte_scanner_t *scanner,
    size_t count);

/* ========================================================================= */
/* Conditional consumption                                                   */
/* ========================================================================= */

/*
 * Consume one byte if it matches expected.
 *
 * Consumption still goes through UTF-8-aware scanner advancement, therefore
 * this function is intended for ASCII syntax bytes.
 */
bool
vitte_scanner_consume_byte(
    vitte_scanner_t *scanner,
    unsigned char expected);

/*
 * Match and consume a NUL-terminated ASCII sequence.
 *
 * On mismatch the scanner is unchanged.
 *
 * If consumption unexpectedly fails after the match was established, the
 * implementation restores the original checkpoint.
 */
bool
vitte_scanner_consume_ascii(
    vitte_scanner_t *scanner,
    const char *text);

/* ========================================================================= */
/* ASCII classification                                                      */
/* ========================================================================= */

/*
 * Locale-independent ASCII predicates.
 */

bool
vitte_scanner_ascii_is_space(
    unsigned char byte);

bool
vitte_scanner_ascii_is_newline(
    unsigned char byte);

bool
vitte_scanner_ascii_is_whitespace(
    unsigned char byte);

bool
vitte_scanner_ascii_is_digit(
    unsigned char byte);

bool
vitte_scanner_ascii_is_hex_digit(
    unsigned char byte);

bool
vitte_scanner_ascii_is_binary_digit(
    unsigned char byte);

bool
vitte_scanner_ascii_is_octal_digit(
    unsigned char byte);

bool
vitte_scanner_ascii_is_alpha(
    unsigned char byte);

bool
vitte_scanner_ascii_is_identifier_start(
    unsigned char byte);

bool
vitte_scanner_ascii_is_identifier_continue(
    unsigned char byte);

/* ========================================================================= */
/* Checkpoints                                                               */
/* ========================================================================= */

vitte_scanner_checkpoint_t
vitte_scanner_checkpoint(
    const vitte_scanner_t *scanner);

bool
vitte_scanner_restore(
    vitte_scanner_t *scanner,
    vitte_scanner_checkpoint_t checkpoint);

/* ========================================================================= */
/* Spans                                                                     */
/* ========================================================================= */

vitte_scanner_span_t
vitte_scanner_span_from_positions(
    vitte_scanner_position_t begin,
    vitte_scanner_position_t end);

vitte_scanner_span_t
vitte_scanner_span_from_checkpoint(
    const vitte_scanner_t *scanner,
    vitte_scanner_checkpoint_t begin);

/*
 * Obtain a borrowed source view for a span.
 *
 * The returned data is not NUL-terminated unless the original source happens
 * to contain a terminator at span.end.
 */
bool
vitte_scanner_slice(
    const vitte_scanner_t *scanner,
    vitte_scanner_span_t span,
    const char **data,
    size_t *length);

/* ========================================================================= */
/* Source line lookup                                                        */
/* ========================================================================= */

/*
 * Return the half-open byte bounds of the line containing offset.
 *
 * Newline bytes themselves are excluded.
 */
bool
vitte_scanner_line_bounds(
    const vitte_scanner_t *scanner,
    size_t offset,
    size_t *line_begin,
    size_t *line_end);

/*
 * Return a borrowed view of the current logical line.
 */
bool
vitte_scanner_current_line(
    const vitte_scanner_t *scanner,
    const char **data,
    size_t *length);

/* ========================================================================= */
/* Position reconstruction                                                   */
/* ========================================================================= */

/*
 * Reconstruct exact scanner position metadata at an arbitrary source byte
 * boundary.
 *
 * Returns false when target_offset:
 *   - exceeds the source;
 *   - points inside a UTF-8 scalar;
 *   - points inside a CRLF pair;
 *   - points inside a skipped UTF-8 BOM.
 */
bool
vitte_scanner_position_at(
    const vitte_scanner_t *scanner,
    size_t target_offset,
    vitte_scanner_position_t *position);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate the entire source as strict UTF-8 regardless of whether scanner
 * traversal has reached that portion yet.
 *
 * An initial UTF-8 BOM is skipped when skip_utf8_bom is enabled.
 */
bool
vitte_scanner_validate_utf8(
    vitte_scanner_t *scanner);

/*
 * Validate internal navigation invariants by reconstructing the current
 * position from the source.
 */
bool
vitte_scanner_validate(
    vitte_scanner_t *scanner);

/* ========================================================================= */
/* Fingerprints                                                              */
/* ========================================================================= */

/*
 * Fingerprint source bytes and source length only.
 *
 * This is independent of scanner cursor position.
 */
uint64_t
vitte_scanner_source_fingerprint(
    vitte_scanner_t *scanner);

/*
 * Fingerprint source content, scanner configuration and current navigation
 * state.
 */
uint64_t
vitte_scanner_fingerprint(
    vitte_scanner_t *scanner);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_scanner_stats_t
vitte_scanner_stats(
    const vitte_scanner_t *scanner);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_scanner_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline version helpers                                                    */
/* ========================================================================= */

static inline unsigned int
vitte_scanner_api_version_major(void)
{
    return VITTE_SCANNER_API_VERSION_MAJOR;
}

static inline unsigned int
vitte_scanner_api_version_minor(void)
{
    return VITTE_SCANNER_API_VERSION_MINOR;
}

static inline unsigned int
vitte_scanner_api_version_patch(void)
{
    return VITTE_SCANNER_API_VERSION_PATCH;
}

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_scanner_error_is_valid(
    vitte_scanner_error_t error)
{
    return error >= VITTE_SCANNER_ERROR_NONE &&
           error < VITTE_SCANNER_ERROR_COUNT;
}

static inline bool
vitte_scanner_state_is_valid(
    vitte_scanner_state_t state)
{
    return state > VITTE_SCANNER_STATE_INVALID &&
           state < VITTE_SCANNER_STATE_COUNT;
}

/* ========================================================================= */
/* Inline lifecycle helpers                                                  */
/* ========================================================================= */

static inline bool
vitte_scanner_is_ready(
    const vitte_scanner_t *scanner)
{
    return scanner != NULL &&
           scanner->magic == VITTE_SCANNER_MAGIC &&
           scanner->state == VITTE_SCANNER_STATE_READY;
}

static inline bool
vitte_scanner_is_scanning(
    const vitte_scanner_t *scanner)
{
    return scanner != NULL &&
           scanner->magic == VITTE_SCANNER_MAGIC &&
           scanner->state == VITTE_SCANNER_STATE_SCANNING;
}

static inline bool
vitte_scanner_is_done(
    const vitte_scanner_t *scanner)
{
    return scanner != NULL &&
           scanner->magic == VITTE_SCANNER_MAGIC &&
           scanner->state == VITTE_SCANNER_STATE_DONE;
}

static inline bool
vitte_scanner_has_failed(
    const vitte_scanner_t *scanner)
{
    return scanner != NULL &&
           scanner->magic == VITTE_SCANNER_MAGIC &&
           scanner->state == VITTE_SCANNER_STATE_FAILED;
}

static inline bool
vitte_scanner_is_destroyed(
    const vitte_scanner_t *scanner)
{
    return scanner != NULL &&
           scanner->magic == VITTE_SCANNER_DEAD_MAGIC &&
           scanner->state == VITTE_SCANNER_STATE_DESTROYED;
}

/* ========================================================================= */
/* Inline source queries                                                     */
/* ========================================================================= */

static inline const unsigned char *
vitte_scanner_source(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return NULL;
    }

    return scanner->source;
}

static inline size_t
vitte_scanner_source_length(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return 0u;
    }

    return scanner->source_length;
}

static inline uint32_t
vitte_scanner_file_id(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return UINT32_C(0);
    }

    return scanner->file_id;
}

static inline size_t
vitte_scanner_offset(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return 0u;
    }

    return scanner->offset;
}

static inline size_t
vitte_scanner_line(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return 0u;
    }

    return scanner->line;
}

static inline size_t
vitte_scanner_column(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return 0u;
    }

    return scanner->column;
}

static inline size_t
vitte_scanner_display_column(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return 0u;
    }

    return scanner->display_column;
}

static inline size_t
vitte_scanner_line_start_offset(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return 0u;
    }

    return scanner->line_start_offset;
}

/* ========================================================================= */
/* Inline configuration queries                                              */
/* ========================================================================= */

static inline size_t
vitte_scanner_tab_width(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return 0u;
    }

    return scanner->tab_width;
}

static inline size_t
vitte_scanner_max_source_bytes(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return 0u;
    }

    return scanner->max_source_bytes;
}

static inline bool
vitte_scanner_is_strict_utf8(
    const vitte_scanner_t *scanner)
{
    return scanner != NULL &&
           scanner->magic == VITTE_SCANNER_MAGIC &&
           scanner->strict_utf8;
}

static inline bool
vitte_scanner_skips_utf8_bom(
    const vitte_scanner_t *scanner)
{
    return scanner != NULL &&
           scanner->magic == VITTE_SCANNER_MAGIC &&
           scanner->skip_utf8_bom;
}

/* ========================================================================= */
/* Inline position helpers                                                   */
/* ========================================================================= */

static inline vitte_scanner_position_t
vitte_scanner_position_invalid(void)
{
    vitte_scanner_position_t position;

    position.file_id = UINT32_C(0);
    position.offset = 0u;
    position.line = 0u;
    position.column = 0u;
    position.display_column = 0u;
    position.line_start_offset = 0u;
    position.valid = false;

    return position;
}

static inline bool
vitte_scanner_position_is_valid(
    vitte_scanner_position_t position)
{
    return position.valid &&
           position.line != 0u &&
           position.column != 0u &&
           position.display_column != 0u &&
           position.line_start_offset <=
               position.offset;
}

static inline bool
vitte_scanner_position_equal(
    vitte_scanner_position_t left,
    vitte_scanner_position_t right)
{
    if (!left.valid ||
        !right.valid) {
        return false;
    }

    return left.file_id == right.file_id &&
           left.offset == right.offset &&
           left.line == right.line &&
           left.column == right.column &&
           left.display_column ==
               right.display_column &&
           left.line_start_offset ==
               right.line_start_offset;
}

/* ========================================================================= */
/* Inline span helpers                                                       */
/* ========================================================================= */

static inline vitte_scanner_span_t
vitte_scanner_span_invalid(void)
{
    vitte_scanner_span_t span;

    span.file_id = UINT32_C(0);
    span.begin = 0u;
    span.end = 0u;
    span.line = 0u;
    span.column = 0u;
    span.display_column = 0u;
    span.valid = false;

    return span;
}

static inline bool
vitte_scanner_span_is_valid(
    vitte_scanner_span_t span)
{
    return span.valid &&
           span.begin <= span.end &&
           span.line != 0u &&
           span.column != 0u &&
           span.display_column != 0u;
}

static inline size_t
vitte_scanner_span_length(
    vitte_scanner_span_t span)
{
    if (!vitte_scanner_span_is_valid(span)) {
        return 0u;
    }

    return span.end - span.begin;
}

static inline bool
vitte_scanner_span_is_empty(
    vitte_scanner_span_t span)
{
    return !vitte_scanner_span_is_valid(span) ||
           span.begin == span.end;
}

static inline bool
vitte_scanner_span_contains_offset(
    vitte_scanner_span_t span,
    size_t offset)
{
    return vitte_scanner_span_is_valid(span) &&
           offset >= span.begin &&
           offset < span.end;
}

static inline bool
vitte_scanner_span_contains_span(
    vitte_scanner_span_t outer,
    vitte_scanner_span_t inner)
{
    return vitte_scanner_span_is_valid(outer) &&
           vitte_scanner_span_is_valid(inner) &&
           outer.file_id == inner.file_id &&
           inner.begin >= outer.begin &&
           inner.end <= outer.end;
}

static inline bool
vitte_scanner_spans_overlap(
    vitte_scanner_span_t left,
    vitte_scanner_span_t right)
{
    return vitte_scanner_span_is_valid(left) &&
           vitte_scanner_span_is_valid(right) &&
           left.file_id == right.file_id &&
           left.begin < right.end &&
           right.begin < left.end;
}

/* ========================================================================= */
/* Inline checkpoint helpers                                                 */
/* ========================================================================= */

static inline vitte_scanner_checkpoint_t
vitte_scanner_checkpoint_invalid(void)
{
    vitte_scanner_checkpoint_t checkpoint;

    checkpoint.generation = UINT64_C(0);
    checkpoint.offset = 0u;
    checkpoint.line = 0u;
    checkpoint.column = 0u;
    checkpoint.display_column = 0u;
    checkpoint.line_start_offset = 0u;
    checkpoint.valid = false;

    return checkpoint;
}

static inline bool
vitte_scanner_checkpoint_is_valid(
    vitte_scanner_checkpoint_t checkpoint)
{
    return checkpoint.valid &&
           checkpoint.generation != UINT64_C(0) &&
           checkpoint.line != 0u &&
           checkpoint.column != 0u &&
           checkpoint.display_column != 0u &&
           checkpoint.line_start_offset <=
               checkpoint.offset;
}

static inline bool
vitte_scanner_checkpoint_belongs_to(
    const vitte_scanner_t *scanner,
    vitte_scanner_checkpoint_t checkpoint)
{
    return scanner != NULL &&
           scanner->magic == VITTE_SCANNER_MAGIC &&
           vitte_scanner_checkpoint_is_valid(
               checkpoint) &&
           checkpoint.generation ==
               scanner->generation &&
           checkpoint.offset <=
               scanner->source_length;
}

/* ========================================================================= */
/* Inline byte predicates                                                    */
/* ========================================================================= */

static inline bool
vitte_scanner_byte_is_ascii(
    unsigned char byte)
{
    return byte <= 0x7fu;
}

static inline bool
vitte_scanner_byte_is_utf8_continuation(
    unsigned char byte)
{
    return (byte & 0xc0u) == 0x80u;
}

static inline bool
vitte_scanner_byte_is_utf8_lead(
    unsigned char byte)
{
    return (byte >= 0xc2u &&
            byte <= 0xdfu) ||
           (byte >= 0xe0u &&
            byte <= 0xefu) ||
           (byte >= 0xf0u &&
            byte <= 0xf4u);
}

/* ========================================================================= */
/* Inline Unicode predicates                                                 */
/* ========================================================================= */

static inline bool
vitte_scanner_unicode_is_scalar(
    uint32_t codepoint)
{
    return codepoint <= UINT32_C(0x10ffff) &&
           !(codepoint >= UINT32_C(0xd800) &&
             codepoint <= UINT32_C(0xdfff));
}

static inline bool
vitte_scanner_unicode_is_ascii(
    uint32_t codepoint)
{
    return codepoint <= UINT32_C(0x7f);
}

/* ========================================================================= */
/* Inline statistics queries                                                 */
/* ========================================================================= */

static inline uint64_t
vitte_scanner_bytes_consumed(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return UINT64_C(0);
    }

    return scanner->stats.bytes_consumed;
}

static inline uint64_t
vitte_scanner_codepoints_consumed(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return UINT64_C(0);
    }

    return scanner->stats.codepoints_consumed;
}

static inline uint64_t
vitte_scanner_newlines_consumed(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic != VITTE_SCANNER_MAGIC) {
        return UINT64_C(0);
    }

    return scanner->stats.newlines;
}

/* ========================================================================= */
/* Compile-time contract checks                                              */
/* ========================================================================= */

#if defined(__cplusplus)

static_assert(
    sizeof(uint64_t) == 8u,
    "scanner requires 64-bit uint64_t");

static_assert(
    sizeof(uint32_t) == 4u,
    "scanner requires 32-bit uint32_t");

static_assert(
    VITTE_SCANNER_API_VERSION_MAJOR == 1u,
    "unexpected scanner API major version");

static_assert(
    VITTE_SCANNER_DEFAULT_TAB_WIDTH > 0u,
    "default tab width must be non-zero");

static_assert(
    VITTE_SCANNER_DEFAULT_TAB_WIDTH <=
        VITTE_SCANNER_MAX_TAB_WIDTH,
    "default tab width exceeds scanner limit");

static_assert(
    VITTE_SCANNER_DEFAULT_MAX_SOURCE_BYTES > 0u,
    "default source limit must be non-zero");

#else

_Static_assert(
    sizeof(uint64_t) == 8u,
    "scanner requires 64-bit uint64_t");

_Static_assert(
    sizeof(uint32_t) == 4u,
    "scanner requires 32-bit uint32_t");

_Static_assert(
    VITTE_SCANNER_API_VERSION_MAJOR == 1u,
    "unexpected scanner API major version");

_Static_assert(
    VITTE_SCANNER_DEFAULT_TAB_WIDTH > 0u,
    "default tab width must be non-zero");

_Static_assert(
    VITTE_SCANNER_DEFAULT_TAB_WIDTH <=
        VITTE_SCANNER_MAX_TAB_WIDTH,
    "default tab width exceeds scanner limit");

_Static_assert(
    VITTE_SCANNER_DEFAULT_MAX_SOURCE_BYTES > 0u,
    "default source limit must be non-zero");

#endif

#ifdef __cplusplus
}
#endif

#endif /* VITTE_SCANNER_SCANNER_H */
