/*
 * Vitte Compiler
 * src/scanner/scanner.c
 *
 * Source scanner implementation.
 *
 * Public contract: scanner.h
 *
 * The scanner is the low-level, byte-precise source navigation layer used
 * before and by the lexer. It deliberately does not classify Vitte tokens.
 *
 * Responsibilities:
 *   - bounded byte traversal;
 *   - UTF-8 decoding and validation;
 *   - BOM handling;
 *   - CR/LF/CRLF-aware line tracking;
 *   - byte/codepoint/display-column tracking;
 *   - configurable tab stops;
 *   - lookahead without mutation;
 *   - checkpoints and rollback;
 *   - source spans/slices;
 *   - line lookup;
 *   - deterministic fingerprinting;
 *   - statistics;
 *   - explicit lifecycle and error handling.
 *
 * Design:
 *   - ISO C17;
 *   - scanner.h is the single public contract;
 *   - source memory is borrowed and immutable;
 *   - no lexer/parser dependency in this implementation;
 *   - no unbounded reads;
 *   - no NUL-termination requirement;
 *   - no locale-dependent character classification;
 *   - no global mutable state;
 *   - deterministic behavior.
 */

#include "scanner.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ========================================================================= */
/* Private constants                                                         */
/* ========================================================================= */

#define VITTE_SCANNER_UTF8_REPLACEMENT UINT32_C(0xfffd)

#define VITTE_SCANNER_UTF8_BOM_0 ((unsigned char)0xefu)
#define VITTE_SCANNER_UTF8_BOM_1 ((unsigned char)0xbbu)
#define VITTE_SCANNER_UTF8_BOM_2 ((unsigned char)0xbfu)

#define VITTE_SCANNER_ASCII_TAB ((unsigned char)0x09u)
#define VITTE_SCANNER_ASCII_LF  ((unsigned char)0x0au)
#define VITTE_SCANNER_ASCII_CR  ((unsigned char)0x0du)

/* ========================================================================= */
/* Private arithmetic                                                        */
/* ========================================================================= */

static bool
vitte_scanner_size_add(
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

static uint64_t
vitte_scanner_u64_add_sat(
    uint64_t left,
    uint64_t right)
{
    if (UINT64_MAX - left < right) {
        return UINT64_MAX;
    }

    return left + right;
}

/* ========================================================================= */
/* Private hashing                                                           */
/* ========================================================================= */

static uint64_t
vitte_scanner_hash_byte(
    uint64_t hash,
    unsigned char value)
{
    hash ^= (uint64_t)value;
    hash *= VITTE_SCANNER_FNV_PRIME;

    return hash;
}

static uint64_t
vitte_scanner_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned int shift;

    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        hash =
            vitte_scanner_hash_byte(
                hash,
                (unsigned char)(
                    (value >> shift) &
                    UINT64_C(0xff)));
    }

    return hash;
}

static uint64_t
vitte_scanner_hash_bytes(
    uint64_t hash,
    const unsigned char *data,
    size_t length)
{
    size_t index;

    if (data == NULL && length != 0u) {
        return hash;
    }

    for (index = 0u;
         index < length;
         ++index) {
        hash =
            vitte_scanner_hash_byte(
                hash,
                data[index]);
    }

    return hash;
}

/* ========================================================================= */
/* Private UTF-8 helpers                                                     */
/* ========================================================================= */

static bool
vitte_scanner_utf8_is_continuation(
    unsigned char byte)
{
    return (byte & 0xc0u) == 0x80u;
}

/*
 * Decode exactly one UTF-8 scalar at `offset`.
 *
 * This routine performs strict RFC 3629 style validation:
 *   - rejects overlong encodings;
 *   - rejects UTF-16 surrogate values;
 *   - rejects values above U+10FFFF;
 *   - rejects truncated sequences;
 *   - rejects invalid continuation bytes.
 *
 * It does not mutate scanner state.
 */
static bool
vitte_scanner_decode_at_private(
    const unsigned char *source,
    size_t source_length,
    size_t offset,
    uint32_t *codepoint,
    size_t *width)
{
    unsigned char first;

    if (source == NULL ||
        codepoint == NULL ||
        width == NULL ||
        offset >= source_length) {
        return false;
    }

    first = source[offset];

    if (first <= 0x7fu) {
        *codepoint = (uint32_t)first;
        *width = 1u;
        return true;
    }

    if (first >= 0xc2u &&
        first <= 0xdfu) {
        unsigned char second;

        if (source_length - offset < 2u) {
            return false;
        }

        second = source[offset + 1u];

        if (!vitte_scanner_utf8_is_continuation(second)) {
            return false;
        }

        *codepoint =
            ((uint32_t)(first & 0x1fu) << 6u) |
            (uint32_t)(second & 0x3fu);

        *width = 2u;

        return true;
    }

    if (first >= 0xe0u &&
        first <= 0xefu) {
        unsigned char second;
        unsigned char third;
        uint32_t value;

        if (source_length - offset < 3u) {
            return false;
        }

        second = source[offset + 1u];
        third = source[offset + 2u];

        if (!vitte_scanner_utf8_is_continuation(second) ||
            !vitte_scanner_utf8_is_continuation(third)) {
            return false;
        }

        /*
         * E0 A0..BF prevents overlong U+0000..U+07FF.
         * ED 80..9F prevents UTF-16 surrogate range.
         */
        if (first == 0xe0u && second < 0xa0u) {
            return false;
        }

        if (first == 0xedu && second > 0x9fu) {
            return false;
        }

        value =
            ((uint32_t)(first & 0x0fu) << 12u) |
            ((uint32_t)(second & 0x3fu) << 6u) |
            (uint32_t)(third & 0x3fu);

        if (!vitte_scanner_unicode_is_scalar(value)) {
            return false;
        }

        *codepoint = value;
        *width = 3u;

        return true;
    }

    if (first >= 0xf0u &&
        first <= 0xf4u) {
        unsigned char second;
        unsigned char third;
        unsigned char fourth;
        uint32_t value;

        if (source_length - offset < 4u) {
            return false;
        }

        second = source[offset + 1u];
        third = source[offset + 2u];
        fourth = source[offset + 3u];

        if (!vitte_scanner_utf8_is_continuation(second) ||
            !vitte_scanner_utf8_is_continuation(third) ||
            !vitte_scanner_utf8_is_continuation(fourth)) {
            return false;
        }

        /*
         * F0 90..BF prevents overlong encodings.
         * F4 80..8F caps Unicode at U+10FFFF.
         */
        if (first == 0xf0u && second < 0x90u) {
            return false;
        }

        if (first == 0xf4u && second > 0x8fu) {
            return false;
        }

        value =
            ((uint32_t)(first & 0x07u) << 18u) |
            ((uint32_t)(second & 0x3fu) << 12u) |
            ((uint32_t)(third & 0x3fu) << 6u) |
            (uint32_t)(fourth & 0x3fu);

        if (!vitte_scanner_unicode_is_scalar(value)) {
            return false;
        }

        *codepoint = value;
        *width = 4u;

        return true;
    }

    return false;
}

/* ========================================================================= */
/* Private context helpers                                                   */
/* ========================================================================= */

static bool
vitte_scanner_context_valid_private(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL) {
        return false;
    }

    if (scanner->magic != VITTE_SCANNER_MAGIC) {
        return false;
    }

    if (scanner->state <=
            VITTE_SCANNER_STATE_INVALID ||
        scanner->state >=
            VITTE_SCANNER_STATE_DESTROYED) {
        return false;
    }

    if (scanner->source == NULL &&
        scanner->source_length != 0u) {
        return false;
    }

    if (scanner->offset >
        scanner->source_length) {
        return false;
    }

    if (scanner->line == 0u ||
        scanner->column == 0u ||
        scanner->display_column == 0u) {
        return false;
    }

    if (scanner->line_start_offset >
        scanner->offset) {
        return false;
    }

    if (scanner->line_start_offset >
        scanner->source_length) {
        return false;
    }

    if (scanner->tab_width == 0u ||
        scanner->tab_width >
            VITTE_SCANNER_MAX_TAB_WIDTH) {
        return false;
    }

    if (scanner->source_length >
        scanner->max_source_bytes) {
        return false;
    }

    return true;
}

static bool
vitte_scanner_fail(
    vitte_scanner_t *scanner,
    vitte_scanner_error_t error)
{
    if (scanner != NULL &&
        scanner->magic == VITTE_SCANNER_MAGIC) {
        scanner->last_error = error;

        if (scanner->state !=
            VITTE_SCANNER_STATE_DESTROYED) {
            scanner->state =
                VITTE_SCANNER_STATE_FAILED;
        }

        scanner->stats.failures =
            vitte_scanner_u64_add_sat(
                scanner->stats.failures,
                UINT64_C(1));
    }

    return false;
}

static bool
vitte_scanner_source_has_bom_private(
    const unsigned char *source,
    size_t source_length)
{
    return source != NULL &&
           source_length >= 3u &&
           source[0] == VITTE_SCANNER_UTF8_BOM_0 &&
           source[1] == VITTE_SCANNER_UTF8_BOM_1 &&
           source[2] == VITTE_SCANNER_UTF8_BOM_2;
}

static size_t
vitte_scanner_next_tab_column_private(
    size_t column,
    size_t tab_width)
{
    size_t zero_based;
    size_t remainder;
    size_t advance;

    /*
     * Public columns are 1-based.
     */
    if (column == 0u ||
        tab_width == 0u) {
        return column;
    }

    zero_based = column - 1u;
    remainder = zero_based % tab_width;
    advance = tab_width - remainder;

    if (column > SIZE_MAX - advance) {
        return SIZE_MAX;
    }

    return column + advance;
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_scanner_error_name(
    vitte_scanner_error_t error)
{
    switch (error) {
        case VITTE_SCANNER_ERROR_NONE:
            return "none";

        case VITTE_SCANNER_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case VITTE_SCANNER_ERROR_INVALID_CONTEXT:
            return "invalid_context";

        case VITTE_SCANNER_ERROR_INVALID_STATE:
            return "invalid_state";

        case VITTE_SCANNER_ERROR_SOURCE_TOO_LARGE:
            return "source_too_large";

        case VITTE_SCANNER_ERROR_INVALID_UTF8:
            return "invalid_utf8";

        case VITTE_SCANNER_ERROR_END_OF_INPUT:
            return "end_of_input";

        case VITTE_SCANNER_ERROR_INVALID_OFFSET:
            return "invalid_offset";

        case VITTE_SCANNER_ERROR_INVALID_CHECKPOINT:
            return "invalid_checkpoint";

        case VITTE_SCANNER_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_SCANNER_ERROR_VALIDATION:
            return "validation";

        case VITTE_SCANNER_ERROR_CORRUPTION:
            return "corruption";

        case VITTE_SCANNER_ERROR_INTERNAL:
            return "internal";

        case VITTE_SCANNER_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_scanner_state_name(
    vitte_scanner_state_t state)
{
    switch (state) {
        case VITTE_SCANNER_STATE_INVALID:
            return "invalid";

        case VITTE_SCANNER_STATE_READY:
            return "ready";

        case VITTE_SCANNER_STATE_SCANNING:
            return "scanning";

        case VITTE_SCANNER_STATE_DONE:
            return "done";

        case VITTE_SCANNER_STATE_FAILED:
            return "failed";

        case VITTE_SCANNER_STATE_DESTROYED:
            return "destroyed";

        case VITTE_SCANNER_STATE_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Position                                                                  */
/* ========================================================================= */

static vitte_scanner_position_t
vitte_scanner_position_private(
    const vitte_scanner_t *scanner)
{
    vitte_scanner_position_t position;

    memset(&position, 0, sizeof(position));

    if (scanner == NULL) {
        return position;
    }

    position.file_id =
        scanner->file_id;

    position.offset =
        scanner->offset;

    position.line =
        scanner->line;

    position.column =
        scanner->column;

    position.display_column =
        scanner->display_column;

    position.line_start_offset =
        scanner->line_start_offset;

    position.valid = true;

    return position;
}

vitte_scanner_position_t
vitte_scanner_position(
    const vitte_scanner_t *scanner)
{
    vitte_scanner_position_t position;

    memset(&position, 0, sizeof(position));

    if (!vitte_scanner_context_valid_private(scanner)) {
        return position;
    }

    return vitte_scanner_position_private(scanner);
}

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

bool
vitte_scanner_init(
    vitte_scanner_t *scanner,
    const char *source,
    size_t source_length,
    uint32_t file_id)
{
    size_t initial_offset;

    if (scanner == NULL) {
        return false;
    }

    memset(scanner, 0, sizeof(*scanner));

    if (source == NULL &&
        source_length != 0u) {
        return false;
    }

    scanner->magic =
        VITTE_SCANNER_MAGIC;

    scanner->state =
        VITTE_SCANNER_STATE_READY;

    scanner->last_error =
        VITTE_SCANNER_ERROR_NONE;

    scanner->source =
        (const unsigned char *)source;

    scanner->source_length =
        source_length;

    scanner->file_id =
        file_id;

    scanner->tab_width =
        VITTE_SCANNER_DEFAULT_TAB_WIDTH;

    scanner->max_source_bytes =
        VITTE_SCANNER_DEFAULT_MAX_SOURCE_BYTES;

    scanner->strict_utf8 = true;
    scanner->skip_utf8_bom = true;

    scanner->generation =
        UINT64_C(1);

    if (source_length >
        scanner->max_source_bytes) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_SOURCE_TOO_LARGE);
    }

    initial_offset = 0u;

    if (scanner->skip_utf8_bom &&
        vitte_scanner_source_has_bom_private(
            scanner->source,
            scanner->source_length)) {
        initial_offset = 3u;

        scanner->stats.boms_skipped =
            UINT64_C(1);
    }

    scanner->offset =
        initial_offset;

    scanner->line = 1u;
    scanner->column = 1u;
    scanner->display_column = 1u;

    scanner->line_start_offset =
        initial_offset;

    if (scanner->offset >=
        scanner->source_length) {
        scanner->state =
            VITTE_SCANNER_STATE_DONE;
    }

    return true;
}

bool
vitte_scanner_reset(
    vitte_scanner_t *scanner,
    const char *source,
    size_t source_length,
    uint32_t file_id)
{
    uint64_t generation;
    size_t tab_width;
    size_t max_source_bytes;
    bool strict_utf8;
    bool skip_utf8_bom;
    size_t initial_offset;

    if (!vitte_scanner_context_valid_private(scanner)) {
        return false;
    }

    if (source == NULL &&
        source_length != 0u) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_INVALID_ARGUMENT);
    }

    if (scanner->state ==
        VITTE_SCANNER_STATE_SCANNING) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_INVALID_STATE);
    }

    generation =
        scanner->generation;

    tab_width =
        scanner->tab_width;

    max_source_bytes =
        scanner->max_source_bytes;

    strict_utf8 =
        scanner->strict_utf8;

    skip_utf8_bom =
        scanner->skip_utf8_bom;

    memset(
        &scanner->stats,
        0,
        sizeof(scanner->stats));

    scanner->state =
        VITTE_SCANNER_STATE_READY;

    scanner->last_error =
        VITTE_SCANNER_ERROR_NONE;

    scanner->source =
        (const unsigned char *)source;

    scanner->source_length =
        source_length;

    scanner->file_id =
        file_id;

    scanner->tab_width =
        tab_width;

    scanner->max_source_bytes =
        max_source_bytes;

    scanner->strict_utf8 =
        strict_utf8;

    scanner->skip_utf8_bom =
        skip_utf8_bom;

    scanner->generation =
        generation == UINT64_MAX
            ? UINT64_MAX
            : generation + UINT64_C(1);

    if (source_length >
        scanner->max_source_bytes) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_SOURCE_TOO_LARGE);
    }

    initial_offset = 0u;

    if (scanner->skip_utf8_bom &&
        vitte_scanner_source_has_bom_private(
            scanner->source,
            scanner->source_length)) {
        initial_offset = 3u;

        scanner->stats.boms_skipped =
            UINT64_C(1);
    }

    scanner->offset =
        initial_offset;

    scanner->line = 1u;
    scanner->column = 1u;
    scanner->display_column = 1u;
    scanner->line_start_offset =
        initial_offset;

    if (scanner->offset >=
        scanner->source_length) {
        scanner->state =
            VITTE_SCANNER_STATE_DONE;
    }

    return true;
}

void
vitte_scanner_destroy(
    vitte_scanner_t *scanner)
{
    if (scanner == NULL) {
        return;
    }

    memset(scanner, 0, sizeof(*scanner));

    scanner->magic =
        VITTE_SCANNER_DEAD_MAGIC;

    scanner->state =
        VITTE_SCANNER_STATE_DESTROYED;
}

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

bool
vitte_scanner_set_tab_width(
    vitte_scanner_t *scanner,
    size_t tab_width)
{
    if (!vitte_scanner_context_valid_private(scanner)) {
        return false;
    }

    if (scanner->state ==
        VITTE_SCANNER_STATE_SCANNING) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_INVALID_STATE);
    }

    if (tab_width == 0u ||
        tab_width >
            VITTE_SCANNER_MAX_TAB_WIDTH) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_INVALID_ARGUMENT);
    }

    scanner->tab_width =
        tab_width;

    scanner->last_error =
        VITTE_SCANNER_ERROR_NONE;

    return true;
}

bool
vitte_scanner_set_max_source_bytes(
    vitte_scanner_t *scanner,
    size_t max_source_bytes)
{
    if (!vitte_scanner_context_valid_private(scanner)) {
        return false;
    }

    if (scanner->state ==
        VITTE_SCANNER_STATE_SCANNING) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_INVALID_STATE);
    }

    if (max_source_bytes == 0u ||
        max_source_bytes <
            scanner->source_length) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_INVALID_ARGUMENT);
    }

    scanner->max_source_bytes =
        max_source_bytes;

    scanner->last_error =
        VITTE_SCANNER_ERROR_NONE;

    return true;
}

bool
vitte_scanner_set_strict_utf8(
    vitte_scanner_t *scanner,
    bool strict_utf8)
{
    if (!vitte_scanner_context_valid_private(scanner)) {
        return false;
    }

    if (scanner->state ==
        VITTE_SCANNER_STATE_SCANNING) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_INVALID_STATE);
    }

    scanner->strict_utf8 =
        strict_utf8;

    scanner->last_error =
        VITTE_SCANNER_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* EOF / remaining                                                           */
/* ========================================================================= */

bool
vitte_scanner_is_eof(
    const vitte_scanner_t *scanner)
{
    if (!vitte_scanner_context_valid_private(scanner)) {
        return true;
    }

    return scanner->offset >=
           scanner->source_length;
}

size_t
vitte_scanner_remaining(
    const vitte_scanner_t *scanner)
{
    if (!vitte_scanner_context_valid_private(scanner) ||
        scanner->offset >=
            scanner->source_length) {
        return 0u;
    }

    return scanner->source_length -
           scanner->offset;
}

/* ========================================================================= */
/* Raw byte access                                                           */
/* ========================================================================= */

bool
vitte_scanner_peek_byte(
    const vitte_scanner_t *scanner,
    size_t lookahead,
    unsigned char *byte)
{
    size_t target;

    if (!vitte_scanner_context_valid_private(scanner) ||
        byte == NULL) {
        return false;
    }

    if (!vitte_scanner_size_add(
            scanner->offset,
            lookahead,
            &target)) {
        return false;
    }

    if (target >=
        scanner->source_length) {
        return false;
    }

    *byte = scanner->source[target];

    return true;
}

bool
vitte_scanner_current_byte(
    const vitte_scanner_t *scanner,
    unsigned char *byte)
{
    return vitte_scanner_peek_byte(
        scanner,
        0u,
        byte);
}

/* ========================================================================= */
/* UTF-8 lookahead                                                           */
/* ========================================================================= */

bool
vitte_scanner_peek_codepoint(
    const vitte_scanner_t *scanner,
    size_t codepoint_lookahead,
    uint32_t *codepoint,
    size_t *width)
{
    size_t offset;
    size_t index;

    if (!vitte_scanner_context_valid_private(scanner) ||
        codepoint == NULL ||
        width == NULL) {
        return false;
    }

    offset = scanner->offset;

    for (index = 0u;
         index <= codepoint_lookahead;
         ++index) {
        uint32_t current;
        size_t current_width;

        if (offset >=
            scanner->source_length) {
            return false;
        }

        if (!vitte_scanner_decode_at_private(
                scanner->source,
                scanner->source_length,
                offset,
                &current,
                &current_width)) {
            if (scanner->strict_utf8) {
                return false;
            }

            current =
                VITTE_SCANNER_UTF8_REPLACEMENT;

            current_width = 1u;
        }

        if (index ==
            codepoint_lookahead) {
            *codepoint = current;
            *width = current_width;

            return true;
        }

        if (!vitte_scanner_size_add(
                offset,
                current_width,
                &offset)) {
            return false;
        }
    }

    return false;
}

bool
vitte_scanner_current_codepoint(
    const vitte_scanner_t *scanner,
    uint32_t *codepoint,
    size_t *width)
{
    return vitte_scanner_peek_codepoint(
        scanner,
        0u,
        codepoint,
        width);
}

/* ========================================================================= */
/* Newline recognition                                                       */
/* ========================================================================= */

static bool
vitte_scanner_newline_width_private(
    const vitte_scanner_t *scanner,
    size_t offset,
    size_t *width)
{
    unsigned char byte;

    if (scanner == NULL ||
        width == NULL ||
        offset >= scanner->source_length) {
        return false;
    }

    byte = scanner->source[offset];

    if (byte == VITTE_SCANNER_ASCII_LF) {
        *width = 1u;
        return true;
    }

    if (byte == VITTE_SCANNER_ASCII_CR) {
        if (offset + 1u <
                scanner->source_length &&
            scanner->source[offset + 1u] ==
                VITTE_SCANNER_ASCII_LF) {
            *width = 2u;
        } else {
            *width = 1u;
        }

        return true;
    }

    return false;
}

/* ========================================================================= */
/* Advance                                                                   */
/* ========================================================================= */

bool
vitte_scanner_advance(
    vitte_scanner_t *scanner,
    uint32_t *codepoint)
{
    uint32_t value;
    size_t width;
    size_t newline_width;
    size_t new_offset;

    if (!vitte_scanner_context_valid_private(scanner)) {
        return false;
    }

    if (scanner->offset >=
        scanner->source_length) {
        scanner->state =
            VITTE_SCANNER_STATE_DONE;

        scanner->last_error =
            VITTE_SCANNER_ERROR_END_OF_INPUT;

        return false;
    }

    scanner->state =
        VITTE_SCANNER_STATE_SCANNING;

    /*
     * Treat CRLF as one logical newline.
     */
    if (vitte_scanner_newline_width_private(
            scanner,
            scanner->offset,
            &newline_width)) {
        value =
            scanner->source[scanner->offset] ==
                    VITTE_SCANNER_ASCII_CR
                ? UINT32_C(0x0d)
                : UINT32_C(0x0a);

        if (!vitte_scanner_size_add(
                scanner->offset,
                newline_width,
                &new_offset)) {
            return vitte_scanner_fail(
                scanner,
                VITTE_SCANNER_ERROR_OVERFLOW);
        }

        scanner->offset =
            new_offset;

        if (scanner->line == SIZE_MAX) {
            return vitte_scanner_fail(
                scanner,
                VITTE_SCANNER_ERROR_OVERFLOW);
        }

        ++scanner->line;

        scanner->column = 1u;
        scanner->display_column = 1u;

        scanner->line_start_offset =
            scanner->offset;

        scanner->stats.newlines =
            vitte_scanner_u64_add_sat(
                scanner->stats.newlines,
                UINT64_C(1));

        scanner->stats.bytes_consumed =
            vitte_scanner_u64_add_sat(
                scanner->stats.bytes_consumed,
                (uint64_t)newline_width);

        scanner->stats.codepoints_consumed =
            vitte_scanner_u64_add_sat(
                scanner->stats.codepoints_consumed,
                UINT64_C(1));

        if (newline_width == 2u) {
            scanner->stats.crlf_newlines =
                vitte_scanner_u64_add_sat(
                    scanner->stats.crlf_newlines,
                    UINT64_C(1));
        }

        if (codepoint != NULL) {
            *codepoint = value;
        }

        if (scanner->offset >=
            scanner->source_length) {
            scanner->state =
                VITTE_SCANNER_STATE_DONE;
        } else {
            scanner->state =
                VITTE_SCANNER_STATE_READY;
        }

        scanner->last_error =
            VITTE_SCANNER_ERROR_NONE;

        return true;
    }

    if (!vitte_scanner_decode_at_private(
            scanner->source,
            scanner->source_length,
            scanner->offset,
            &value,
            &width)) {
        scanner->stats.invalid_utf8_sequences =
            vitte_scanner_u64_add_sat(
                scanner->stats.invalid_utf8_sequences,
                UINT64_C(1));

        if (scanner->strict_utf8) {
            return vitte_scanner_fail(
                scanner,
                VITTE_SCANNER_ERROR_INVALID_UTF8);
        }

        value =
            VITTE_SCANNER_UTF8_REPLACEMENT;

        width = 1u;
    }

    if (!vitte_scanner_size_add(
            scanner->offset,
            width,
            &new_offset)) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_OVERFLOW);
    }

    scanner->offset =
        new_offset;

    if (scanner->column == SIZE_MAX) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_OVERFLOW);
    }

    ++scanner->column;

    if (value ==
        (uint32_t)VITTE_SCANNER_ASCII_TAB) {
        size_t next_column;

        next_column =
            vitte_scanner_next_tab_column_private(
                scanner->display_column,
                scanner->tab_width);

        if (next_column == SIZE_MAX) {
            return vitte_scanner_fail(
                scanner,
                VITTE_SCANNER_ERROR_OVERFLOW);
        }

        scanner->display_column =
            next_column;

        scanner->stats.tabs =
            vitte_scanner_u64_add_sat(
                scanner->stats.tabs,
                UINT64_C(1));
    } else {
        if (scanner->display_column ==
            SIZE_MAX) {
            return vitte_scanner_fail(
                scanner,
                VITTE_SCANNER_ERROR_OVERFLOW);
        }

        ++scanner->display_column;
    }

    scanner->stats.bytes_consumed =
        vitte_scanner_u64_add_sat(
            scanner->stats.bytes_consumed,
            (uint64_t)width);

    scanner->stats.codepoints_consumed =
        vitte_scanner_u64_add_sat(
            scanner->stats.codepoints_consumed,
            UINT64_C(1));

    if (codepoint != NULL) {
        *codepoint = value;
    }

    if (scanner->offset >=
        scanner->source_length) {
        scanner->state =
            VITTE_SCANNER_STATE_DONE;
    } else {
        scanner->state =
            VITTE_SCANNER_STATE_READY;
    }

    scanner->last_error =
        VITTE_SCANNER_ERROR_NONE;

    return true;
}

bool
vitte_scanner_advance_n(
    vitte_scanner_t *scanner,
    size_t count)
{
    size_t index;

    if (!vitte_scanner_context_valid_private(scanner)) {
        return false;
    }

    for (index = 0u;
         index < count;
         ++index) {
        if (!vitte_scanner_advance(
                scanner,
                NULL)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* ASCII predicates                                                          */
/* ========================================================================= */

bool
vitte_scanner_ascii_is_space(
    unsigned char byte)
{
    return byte == (unsigned char)' ' ||
           byte == VITTE_SCANNER_ASCII_TAB ||
           byte == (unsigned char)'\v' ||
           byte == (unsigned char)'\f';
}

bool
vitte_scanner_ascii_is_newline(
    unsigned char byte)
{
    return byte == VITTE_SCANNER_ASCII_LF ||
           byte == VITTE_SCANNER_ASCII_CR;
}

bool
vitte_scanner_ascii_is_whitespace(
    unsigned char byte)
{
    return vitte_scanner_ascii_is_space(byte) ||
           vitte_scanner_ascii_is_newline(byte);
}

bool
vitte_scanner_ascii_is_digit(
    unsigned char byte)
{
    return byte >= (unsigned char)'0' &&
           byte <= (unsigned char)'9';
}

bool
vitte_scanner_ascii_is_hex_digit(
    unsigned char byte)
{
    return vitte_scanner_ascii_is_digit(byte) ||
           (byte >= (unsigned char)'a' &&
            byte <= (unsigned char)'f') ||
           (byte >= (unsigned char)'A' &&
            byte <= (unsigned char)'F');
}

bool
vitte_scanner_ascii_is_binary_digit(
    unsigned char byte)
{
    return byte == (unsigned char)'0' ||
           byte == (unsigned char)'1';
}

bool
vitte_scanner_ascii_is_octal_digit(
    unsigned char byte)
{
    return byte >= (unsigned char)'0' &&
           byte <= (unsigned char)'7';
}

bool
vitte_scanner_ascii_is_alpha(
    unsigned char byte)
{
    return (byte >= (unsigned char)'a' &&
            byte <= (unsigned char)'z') ||
           (byte >= (unsigned char)'A' &&
            byte <= (unsigned char)'Z');
}

bool
vitte_scanner_ascii_is_identifier_start(
    unsigned char byte)
{
    return vitte_scanner_ascii_is_alpha(byte) ||
           byte == (unsigned char)'_';
}

bool
vitte_scanner_ascii_is_identifier_continue(
    unsigned char byte)
{
    return vitte_scanner_ascii_is_identifier_start(byte) ||
           vitte_scanner_ascii_is_digit(byte);
}

/* ========================================================================= */
/* Conditional consumption                                                   */
/* ========================================================================= */

bool
vitte_scanner_consume_byte(
    vitte_scanner_t *scanner,
    unsigned char expected)
{
    unsigned char current;

    if (!vitte_scanner_current_byte(
            scanner,
            &current)) {
        return false;
    }

    if (current != expected) {
        return false;
    }

    return vitte_scanner_advance(
        scanner,
        NULL);
}

bool
vitte_scanner_consume_ascii(
    vitte_scanner_t *scanner,
    const char *text)
{
    size_t length;
    size_t index;
    vitte_scanner_checkpoint_t checkpoint;

    if (!vitte_scanner_context_valid_private(scanner) ||
        text == NULL) {
        return false;
    }

    length = strlen(text);

    if (length >
        vitte_scanner_remaining(scanner)) {
        return false;
    }

    for (index = 0u;
         index < length;
         ++index) {
        if (scanner->source[
                scanner->offset + index] !=
            (unsigned char)text[index]) {
            return false;
        }
    }

    checkpoint =
        vitte_scanner_checkpoint(scanner);

    for (index = 0u;
         index < length;
         ++index) {
        if (!vitte_scanner_advance(
                scanner,
                NULL)) {
            (void)vitte_scanner_restore(
                scanner,
                checkpoint);

            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Checkpoints                                                               */
/* ========================================================================= */

vitte_scanner_checkpoint_t
vitte_scanner_checkpoint(
    const vitte_scanner_t *scanner)
{
    vitte_scanner_checkpoint_t checkpoint;

    memset(&checkpoint, 0, sizeof(checkpoint));

    if (!vitte_scanner_context_valid_private(scanner)) {
        return checkpoint;
    }

    checkpoint.generation =
        scanner->generation;

    checkpoint.offset =
        scanner->offset;

    checkpoint.line =
        scanner->line;

    checkpoint.column =
        scanner->column;

    checkpoint.display_column =
        scanner->display_column;

    checkpoint.line_start_offset =
        scanner->line_start_offset;

    checkpoint.valid = true;

    return checkpoint;
}

bool
vitte_scanner_restore(
    vitte_scanner_t *scanner,
    vitte_scanner_checkpoint_t checkpoint)
{
    if (!vitte_scanner_context_valid_private(scanner)) {
        return false;
    }

    if (!checkpoint.valid ||
        checkpoint.generation !=
            scanner->generation ||
        checkpoint.offset >
            scanner->source_length ||
        checkpoint.line == 0u ||
        checkpoint.column == 0u ||
        checkpoint.display_column == 0u ||
        checkpoint.line_start_offset >
            checkpoint.offset) {
        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_INVALID_CHECKPOINT);
    }

    scanner->offset =
        checkpoint.offset;

    scanner->line =
        checkpoint.line;

    scanner->column =
        checkpoint.column;

    scanner->display_column =
        checkpoint.display_column;

    scanner->line_start_offset =
        checkpoint.line_start_offset;

    scanner->state =
        scanner->offset >=
                scanner->source_length
            ? VITTE_SCANNER_STATE_DONE
            : VITTE_SCANNER_STATE_READY;

    scanner->last_error =
        VITTE_SCANNER_ERROR_NONE;

    scanner->stats.restores =
        vitte_scanner_u64_add_sat(
            scanner->stats.restores,
            UINT64_C(1));

    return true;
}

/* ========================================================================= */
/* Spans                                                                     */
/* ========================================================================= */

vitte_scanner_span_t
vitte_scanner_span_from_positions(
    vitte_scanner_position_t begin,
    vitte_scanner_position_t end)
{
    vitte_scanner_span_t span;

    memset(&span, 0, sizeof(span));

    if (!begin.valid ||
        !end.valid ||
        begin.file_id != end.file_id ||
        begin.offset > end.offset) {
        return span;
    }

    span.file_id =
        begin.file_id;

    span.begin =
        begin.offset;

    span.end =
        end.offset;

    span.line =
        begin.line;

    span.column =
        begin.column;

    span.display_column =
        begin.display_column;

    span.valid = true;

    return span;
}

vitte_scanner_span_t
vitte_scanner_span_from_checkpoint(
    const vitte_scanner_t *scanner,
    vitte_scanner_checkpoint_t begin)
{
    vitte_scanner_position_t begin_position;
    vitte_scanner_position_t end_position;

    memset(
        &begin_position,
        0,
        sizeof(begin_position));

    memset(
        &end_position,
        0,
        sizeof(end_position));

    if (!vitte_scanner_context_valid_private(scanner) ||
        !begin.valid ||
        begin.generation !=
            scanner->generation ||
        begin.offset >
            scanner->offset) {
        return vitte_scanner_span_from_positions(
            begin_position,
            end_position);
    }

    begin_position.file_id =
        scanner->file_id;

    begin_position.offset =
        begin.offset;

    begin_position.line =
        begin.line;

    begin_position.column =
        begin.column;

    begin_position.display_column =
        begin.display_column;

    begin_position.line_start_offset =
        begin.line_start_offset;

    begin_position.valid = true;

    end_position =
        vitte_scanner_position_private(scanner);

    return vitte_scanner_span_from_positions(
        begin_position,
        end_position);
}

bool
vitte_scanner_slice(
    const vitte_scanner_t *scanner,
    vitte_scanner_span_t span,
    const char **data,
    size_t *length)
{
    if (!vitte_scanner_context_valid_private(scanner) ||
        data == NULL ||
        length == NULL ||
        !span.valid ||
        span.file_id != scanner->file_id ||
        span.begin > span.end ||
        span.end > scanner->source_length) {
        return false;
    }

    if (scanner->source == NULL) {
        if (span.begin != 0u ||
            span.end != 0u) {
            return false;
        }

        *data = NULL;
        *length = 0u;

        return true;
    }

    *data =
        (const char *)(scanner->source +
                       span.begin);

    *length =
        span.end - span.begin;

    return true;
}

/* ========================================================================= */
/* Line lookup                                                               */
/* ========================================================================= */

bool
vitte_scanner_line_bounds(
    const vitte_scanner_t *scanner,
    size_t offset,
    size_t *line_begin,
    size_t *line_end)
{
    size_t begin;
    size_t end;

    if (!vitte_scanner_context_valid_private(scanner) ||
        line_begin == NULL ||
        line_end == NULL ||
        offset > scanner->source_length) {
        return false;
    }

    begin = offset;

    while (begin != 0u) {
        unsigned char previous;

        previous =
            scanner->source[begin - 1u];

        if (previous ==
                VITTE_SCANNER_ASCII_LF ||
            previous ==
                VITTE_SCANNER_ASCII_CR) {
            break;
        }

        --begin;
    }

    end = offset;

    while (end <
           scanner->source_length) {
        unsigned char current;

        current =
            scanner->source[end];

        if (current ==
                VITTE_SCANNER_ASCII_LF ||
            current ==
                VITTE_SCANNER_ASCII_CR) {
            break;
        }

        ++end;
    }

    *line_begin = begin;
    *line_end = end;

    return true;
}

bool
vitte_scanner_current_line(
    const vitte_scanner_t *scanner,
    const char **data,
    size_t *length)
{
    size_t begin;
    size_t end;

    if (!vitte_scanner_context_valid_private(scanner) ||
        data == NULL ||
        length == NULL) {
        return false;
    }

    if (!vitte_scanner_line_bounds(
            scanner,
            scanner->offset,
            &begin,
            &end)) {
        return false;
    }

    if (scanner->source == NULL) {
        *data = NULL;
        *length = 0u;
        return true;
    }

    *data =
        (const char *)(scanner->source +
                       begin);

    *length =
        end - begin;

    return true;
}

/* ========================================================================= */
/* Position reconstruction                                                   */
/* ========================================================================= */

/*
 * Reconstruct line/column metadata for an arbitrary byte offset.
 *
 * This intentionally scans from the beginning rather than guessing based on
 * byte offsets. It is used by tooling and validation, not the hot lexer path.
 */
bool
vitte_scanner_position_at(
    const vitte_scanner_t *scanner,
    size_t target_offset,
    vitte_scanner_position_t *position)
{
    size_t offset;
    size_t line;
    size_t column;
    size_t display_column;
    size_t line_start;

    if (!vitte_scanner_context_valid_private(scanner) ||
        position == NULL ||
        target_offset >
            scanner->source_length) {
        return false;
    }

    offset = 0u;
    line = 1u;
    column = 1u;
    display_column = 1u;
    line_start = 0u;

    if (scanner->skip_utf8_bom &&
        vitte_scanner_source_has_bom_private(
            scanner->source,
            scanner->source_length)) {
        if (target_offset < 3u) {
            /*
             * Offsets inside a skipped BOM are deliberately rejected.
             */
            return false;
        }

        offset = 3u;
        line_start = 3u;
    }

    while (offset < target_offset) {
        size_t newline_width;
        uint32_t codepoint;
        size_t width;

        if (vitte_scanner_newline_width_private(
                scanner,
                offset,
                &newline_width)) {
            if (offset + newline_width >
                target_offset) {
                /*
                 * Offset inside CRLF would split one logical newline.
                 */
                return false;
            }

            offset += newline_width;

            if (line == SIZE_MAX) {
                return false;
            }

            ++line;
            column = 1u;
            display_column = 1u;
            line_start = offset;

            continue;
        }

        if (!vitte_scanner_decode_at_private(
                scanner->source,
                scanner->source_length,
                offset,
                &codepoint,
                &width)) {
            if (scanner->strict_utf8) {
                return false;
            }

            codepoint =
                VITTE_SCANNER_UTF8_REPLACEMENT;

            width = 1u;
        }

        if (offset + width >
            target_offset) {
            /*
             * Target points into the middle of a UTF-8 scalar.
             */
            return false;
        }

        offset += width;

        if (column == SIZE_MAX) {
            return false;
        }

        ++column;

        if (codepoint ==
            (uint32_t)VITTE_SCANNER_ASCII_TAB) {
            display_column =
                vitte_scanner_next_tab_column_private(
                    display_column,
                    scanner->tab_width);

            if (display_column ==
                SIZE_MAX) {
                return false;
            }
        } else {
            if (display_column ==
                SIZE_MAX) {
                return false;
            }

            ++display_column;
        }
    }

    memset(position, 0, sizeof(*position));

    position->file_id =
        scanner->file_id;

    position->offset =
        target_offset;

    position->line =
        line;

    position->column =
        column;

    position->display_column =
        display_column;

    position->line_start_offset =
        line_start;

    position->valid = true;

    return true;
}

/* ========================================================================= */
/* UTF-8 validation                                                          */
/* ========================================================================= */

bool
vitte_scanner_validate_utf8(
    vitte_scanner_t *scanner)
{
    size_t offset;

    if (!vitte_scanner_context_valid_private(scanner)) {
        return false;
    }

    scanner->stats.utf8_validation_runs =
        vitte_scanner_u64_add_sat(
            scanner->stats.utf8_validation_runs,
            UINT64_C(1));

    offset = 0u;

    if (scanner->skip_utf8_bom &&
        vitte_scanner_source_has_bom_private(
            scanner->source,
            scanner->source_length)) {
        offset = 3u;
    }

    while (offset <
           scanner->source_length) {
        uint32_t codepoint;
        size_t width;

        if (!vitte_scanner_decode_at_private(
                scanner->source,
                scanner->source_length,
                offset,
                &codepoint,
                &width)) {
            scanner->stats.utf8_validation_failures =
                vitte_scanner_u64_add_sat(
                    scanner->stats.utf8_validation_failures,
                    UINT64_C(1));

            scanner->last_error =
                VITTE_SCANNER_ERROR_INVALID_UTF8;

            return false;
        }

        (void)codepoint;

        if (!vitte_scanner_size_add(
                offset,
                width,
                &offset)) {
            scanner->stats.utf8_validation_failures =
                vitte_scanner_u64_add_sat(
                    scanner->stats.utf8_validation_failures,
                    UINT64_C(1));

            scanner->last_error =
                VITTE_SCANNER_ERROR_OVERFLOW;

            return false;
        }
    }

    scanner->last_error =
        VITTE_SCANNER_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Context validation                                                        */
/* ========================================================================= */

bool
vitte_scanner_validate(
    vitte_scanner_t *scanner)
{
    vitte_scanner_position_t reconstructed;

    if (!vitte_scanner_context_valid_private(scanner)) {
        return false;
    }

    scanner->stats.validation_runs =
        vitte_scanner_u64_add_sat(
            scanner->stats.validation_runs,
            UINT64_C(1));

    if (!vitte_scanner_position_at(
            scanner,
            scanner->offset,
            &reconstructed)) {
        scanner->stats.validation_failures =
            vitte_scanner_u64_add_sat(
                scanner->stats.validation_failures,
                UINT64_C(1));

        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_VALIDATION);
    }

    if (!reconstructed.valid ||
        reconstructed.file_id !=
            scanner->file_id ||
        reconstructed.offset !=
            scanner->offset ||
        reconstructed.line !=
            scanner->line ||
        reconstructed.column !=
            scanner->column ||
        reconstructed.display_column !=
            scanner->display_column ||
        reconstructed.line_start_offset !=
            scanner->line_start_offset) {
        scanner->stats.validation_failures =
            vitte_scanner_u64_add_sat(
                scanner->stats.validation_failures,
                UINT64_C(1));

        return vitte_scanner_fail(
            scanner,
            VITTE_SCANNER_ERROR_CORRUPTION);
    }

    scanner->last_error =
        VITTE_SCANNER_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Fingerprinting                                                            */
/* ========================================================================= */

uint64_t
vitte_scanner_source_fingerprint(
    vitte_scanner_t *scanner)
{
    uint64_t hash;

    if (!vitte_scanner_context_valid_private(scanner)) {
        return UINT64_C(0);
    }

    hash =
        VITTE_SCANNER_FNV_OFFSET;

    hash =
        vitte_scanner_hash_u64(
            hash,
            (uint64_t)scanner->source_length);

    hash =
        vitte_scanner_hash_bytes(
            hash,
            scanner->source,
            scanner->source_length);

    scanner->stats.hash_runs =
        vitte_scanner_u64_add_sat(
            scanner->stats.hash_runs,
            UINT64_C(1));

    return hash;
}

uint64_t
vitte_scanner_fingerprint(
    vitte_scanner_t *scanner)
{
    uint64_t hash;

    if (!vitte_scanner_context_valid_private(scanner)) {
        return UINT64_C(0);
    }

    hash =
        VITTE_SCANNER_FNV_OFFSET;

    /*
     * Source identity/content.
     */
    hash =
        vitte_scanner_hash_u64(
            hash,
            (uint64_t)scanner->source_length);

    hash =
        vitte_scanner_hash_bytes(
            hash,
            scanner->source,
            scanner->source_length);

    /*
     * Scanner configuration.
     */
    hash =
        vitte_scanner_hash_u64(
            hash,
            (uint64_t)scanner->file_id);

    hash =
        vitte_scanner_hash_u64(
            hash,
            (uint64_t)scanner->tab_width);

    hash =
        vitte_scanner_hash_u64(
            hash,
            scanner->strict_utf8
                ? UINT64_C(1)
                : UINT64_C(0));

    hash =
        vitte_scanner_hash_u64(
            hash,
            scanner->skip_utf8_bom
                ? UINT64_C(1)
                : UINT64_C(0));

    /*
     * Current navigation state.
     */
    hash =
        vitte_scanner_hash_u64(
            hash,
            (uint64_t)scanner->offset);

    hash =
        vitte_scanner_hash_u64(
            hash,
            (uint64_t)scanner->line);

    hash =
        vitte_scanner_hash_u64(
            hash,
            (uint64_t)scanner->column);

    hash =
        vitte_scanner_hash_u64(
            hash,
            (uint64_t)scanner->display_column);

    hash =
        vitte_scanner_hash_u64(
            hash,
            (uint64_t)scanner->line_start_offset);

    scanner->stats.hash_runs =
        vitte_scanner_u64_add_sat(
            scanner->stats.hash_runs,
            UINT64_C(1));

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_scanner_stats_t
vitte_scanner_stats(
    const vitte_scanner_t *scanner)
{
    vitte_scanner_stats_t stats;

    memset(&stats, 0, sizeof(stats));

    if (!vitte_scanner_context_valid_private(scanner)) {
        return stats;
    }

    return scanner->stats;
}

/* ========================================================================= */
/* Public queries                                                            */
/* ========================================================================= */

bool
vitte_scanner_is_valid(
    const vitte_scanner_t *scanner)
{
    return vitte_scanner_context_valid_private(scanner);
}

vitte_scanner_error_t
vitte_scanner_last_error(
    const vitte_scanner_t *scanner)
{
    if (scanner == NULL ||
        scanner->magic !=
            VITTE_SCANNER_MAGIC) {
        return VITTE_SCANNER_ERROR_INVALID_CONTEXT;
    }

    return scanner->last_error;
}

uint64_t
vitte_scanner_generation(
    const vitte_scanner_t *scanner)
{
    if (!vitte_scanner_context_valid_private(scanner)) {
        return UINT64_C(0);
    }

    return scanner->generation;
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_scanner_translation_unit_anchor(void)
{
    /*
     * Stable symbol used by static-library linkage tests and build-system
     * probes. Intentionally empty.
     */
}
