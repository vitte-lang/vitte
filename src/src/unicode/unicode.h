#ifndef VITTE_UNICODE_UNICODE_H
#define VITTE_UNICODE_UNICODE_H

/*
 * Vitte Compiler
 * src/unicode/unicode.h
 *
 * Canonical public Unicode subsystem contract.
 *
 * Synchronized with unicode.c.
 *
 * Responsibilities:
 *   - strict Unicode scalar validation;
 *   - strict UTF-8 decoding and encoding;
 *   - UTF-8 validation;
 *   - UTF-8 code-point counting;
 *   - forward/backward UTF-8 traversal;
 *   - UTF-16 decoding and encoding;
 *   - ASCII classification and case conversion;
 *   - Unicode whitespace/newline classification;
 *   - combining-mark classification;
 *   - decimal-digit classification;
 *   - Vitte Unicode identifier validation;
 *   - control/printable classification;
 *   - deterministic display-width estimation;
 *   - UTF-8 BOM handling;
 *   - deterministic FNV-1a hashing;
 *   - UTF-8 statistics/analysis.
 *
 * Properties:
 *   - locale-independent;
 *   - allocation-free;
 *   - deterministic;
 *   - no global mutable state;
 *   - no libc locale dependency;
 *   - no null-terminated-string requirement;
 *   - embedded NUL bytes are supported;
 *   - all input strings use explicit byte lengths.
 *
 * UTF-8 decoder policy:
 *   - rejects malformed continuation bytes;
 *   - rejects truncated sequences;
 *   - rejects overlong encodings;
 *   - rejects UTF-16 surrogate values;
 *   - rejects values above U+10FFFF;
 *   - never reads beyond the supplied input length.
 *
 * Identifier policy:
 *   - '_' is explicitly accepted;
 *   - ASCII letters are accepted as identifier starts;
 *   - Unicode script ranges are accepted;
 *   - decimal digits are continuation characters;
 *   - combining marks are continuation characters;
 *   - U+200C/U+200D are continuation characters;
 *   - selected connector punctuation is accepted in continuation.
 *
 * NOTE:
 *   The current identifier implementation is a deterministic compiler policy
 *   and is not yet a generated, Unicode-version-pinned UAX #31 XID_Start /
 *   XID_Continue database. A future generated Unicode table may replace the
 *   classification implementation without changing this public API.
 *
 * Display-width policy:
 *   - invalid scalar: -1;
 *   - control character: -1;
 *   - combining/zero-width character: 0;
 *   - ordinary character: 1;
 *   - East Asian wide/full-width and supported emoji ranges: 2.
 *
 * This width is suitable for compiler diagnostics. It is not a complete
 * grapheme-cluster renderer or terminal emulator.
 *
 * ISO C17 / C++ compatible public header.
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

#define VITTE_UNICODE_API_VERSION_MAJOR 1u
#define VITTE_UNICODE_API_VERSION_MINOR 0u
#define VITTE_UNICODE_API_VERSION_PATCH 0u

#define VITTE_UNICODE_API_VERSION \
    ((VITTE_UNICODE_API_VERSION_MAJOR * 10000u) + \
     (VITTE_UNICODE_API_VERSION_MINOR * 100u) + \
     VITTE_UNICODE_API_VERSION_PATCH)

/* ========================================================================= */
/* Unicode version contract                                                  */
/* ========================================================================= */

/*
 * Classification tables are currently implemented as deterministic explicit
 * ranges rather than a generated complete Unicode Character Database.
 *
 * These values therefore describe the subsystem API's Unicode ceiling rather
 * than claiming complete UCD coverage for every property.
 */
#define VITTE_UNICODE_MAX_CODEPOINT \
    UINT32_C(0x10ffff)

#define VITTE_UNICODE_MAX_BMP_CODEPOINT \
    UINT32_C(0xffff)

#define VITTE_UNICODE_REPLACEMENT_CHARACTER \
    UINT32_C(0xfffd)

#define VITTE_UNICODE_BYTE_ORDER_MARK \
    UINT32_C(0xfeff)

#define VITTE_UNICODE_MAX_UTF8_BYTES \
    ((size_t)4u)

#define VITTE_UNICODE_MAX_UTF16_UNITS \
    ((size_t)2u)

#define VITTE_UNICODE_UTF8_BOM_LENGTH \
    ((size_t)3u)

/* ========================================================================= */
/* UTF-8 constants                                                           */
/* ========================================================================= */

#define VITTE_UNICODE_UTF8_CONTINUATION_MASK \
    UINT8_C(0xc0)

#define VITTE_UNICODE_UTF8_CONTINUATION_VALUE \
    UINT8_C(0x80)

#define VITTE_UNICODE_UTF8_BOM_BYTE_0 \
    UINT8_C(0xef)

#define VITTE_UNICODE_UTF8_BOM_BYTE_1 \
    UINT8_C(0xbb)

#define VITTE_UNICODE_UTF8_BOM_BYTE_2 \
    UINT8_C(0xbf)

/* ========================================================================= */
/* UTF-16 constants                                                          */
/* ========================================================================= */

#define VITTE_UNICODE_UTF16_HIGH_SURROGATE_FIRST \
    UINT16_C(0xd800)

#define VITTE_UNICODE_UTF16_HIGH_SURROGATE_LAST \
    UINT16_C(0xdbff)

#define VITTE_UNICODE_UTF16_LOW_SURROGATE_FIRST \
    UINT16_C(0xdc00)

#define VITTE_UNICODE_UTF16_LOW_SURROGATE_LAST \
    UINT16_C(0xdfff)

/* ========================================================================= */
/* Diagnostic/display defaults                                               */
/* ========================================================================= */

#define VITTE_UNICODE_DEFAULT_TAB_WIDTH \
    ((size_t)4u)

/* ========================================================================= */
/* Hashing                                                                   */
/* ========================================================================= */

/*
 * 64-bit FNV-1a.
 */
#define VITTE_UNICODE_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_UNICODE_FNV_PRIME \
    UINT64_C(1099511628211)

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_unicode_error {
    VITTE_UNICODE_ERROR_NONE = 0,

    /*
     * Invalid API argument.
     */
    VITTE_UNICODE_ERROR_INVALID_ARGUMENT,

    /*
     * Decoder reached an empty input range.
     */
    VITTE_UNICODE_ERROR_END_OF_INPUT,

    /*
     * Malformed UTF-8 sequence.
     */
    VITTE_UNICODE_ERROR_INVALID_UTF8,

    /*
     * A valid UTF-8 prefix does not contain enough bytes.
     */
    VITTE_UNICODE_ERROR_TRUNCATED_UTF8,

    /*
     * UTF-8 representation is longer than required for the scalar value.
     */
    VITTE_UNICODE_ERROR_OVERLONG_UTF8,

    /*
     * UTF-8/UTF-16 representation addresses U+D800..U+DFFF.
     */
    VITTE_UNICODE_ERROR_SURROGATE,

    /*
     * Scalar value is above U+10FFFF.
     */
    VITTE_UNICODE_ERROR_OUT_OF_RANGE,

    /*
     * Malformed UTF-16 surrogate sequence.
     */
    VITTE_UNICODE_ERROR_INVALID_UTF16,

    /*
     * Reserved for bounded conversion APIs.
     */
    VITTE_UNICODE_ERROR_OUTPUT_TOO_SMALL,

    /*
     * Arithmetic overflow.
     */
    VITTE_UNICODE_ERROR_OVERFLOW,

    VITTE_UNICODE_ERROR_COUNT
} vitte_unicode_error_t;

/* ========================================================================= */
/* Decode result                                                             */
/* ========================================================================= */

/*
 * Result returned by the strict UTF-8 decoder.
 *
 * On success:
 *   error     = VITTE_UNICODE_ERROR_NONE
 *   codepoint = decoded Unicode scalar
 *   length    = 1..4
 *
 * On malformed input:
 *   codepoint = U+FFFD
 *
 * length on malformed UTF-8 currently describes the number of bytes the
 * decoder considers safe for the caller's recovery policy. The compiler
 * normally reports the error at the current byte and decides itself whether
 * to recover or stop.
 */
typedef struct vitte_unicode_result {
    vitte_unicode_error_t error;
    uint32_t codepoint;
    size_t length;
} vitte_unicode_result_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_unicode_stats {
    /*
     * Number of input bytes.
     */
    size_t bytes;

    /*
     * Number of successfully decoded Unicode scalar values.
     */
    size_t codepoints;

    size_t ascii_codepoints;
    size_t non_ascii_codepoints;

    size_t newlines;
    size_t whitespace_codepoints;
    size_t combining_codepoints;

    /*
     * Number of malformed sequences encountered by analysis.
     *
     * Current vitte_unicode_analyze_utf8() stops at the first malformed
     * sequence, therefore this is currently 0 or 1.
     */
    size_t invalid_sequences;

    /*
     * Byte offset of the first malformed sequence.
     *
     * On valid input this equals bytes.
     */
    size_t first_error_offset;

    bool valid_utf8;
    bool has_bom;
} vitte_unicode_stats_t;

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_unicode_error_name(
    vitte_unicode_error_t error);

/* ========================================================================= */
/* Unicode scalar validation                                                 */
/* ========================================================================= */

/*
 * True for Unicode scalar values:
 *
 *   U+0000..U+D7FF
 *   U+E000..U+10FFFF
 */
bool
vitte_unicode_is_scalar(
    uint32_t codepoint);

/*
 * True for U+0000..U+007F.
 */
bool
vitte_unicode_is_ascii(
    uint32_t codepoint);

/*
 * True for U+D800..U+DFFF.
 */
bool
vitte_unicode_is_surrogate(
    uint32_t codepoint);

/* ========================================================================= */
/* UTF-8 decode                                                              */
/* ========================================================================= */

/*
 * Strictly decode the first UTF-8 sequence from input.
 *
 * input:
 *   pointer to raw bytes.
 *
 * length:
 *   number of available bytes.
 *
 * No NUL terminator is required.
 */
vitte_unicode_result_t
vitte_unicode_decode_utf8(
    const unsigned char *input,
    size_t length);

/* ========================================================================= */
/* UTF-8 encode                                                              */
/* ========================================================================= */

/*
 * Encode one Unicode scalar value into UTF-8.
 *
 * output must provide at least 4 bytes.
 *
 * On success:
 *   *out_length = 1..4
 *
 * On failure:
 *   *out_length = 0
 */
vitte_unicode_error_t
vitte_unicode_encode_utf8(
    uint32_t codepoint,
    unsigned char output[4],
    size_t *out_length);

/* ========================================================================= */
/* UTF-8 validation                                                          */
/* ========================================================================= */

/*
 * Validate exactly length bytes.
 *
 * input == NULL is accepted only when length == 0.
 *
 * On malformed input:
 *   returns false;
 *   *error_offset receives the first malformed byte when non-NULL.
 *
 * On valid input:
 *   *error_offset receives length when non-NULL.
 */
bool
vitte_unicode_validate_utf8(
    const char *input,
    size_t length,
    size_t *error_offset);

/* ========================================================================= */
/* UTF-8 code-point count                                                    */
/* ========================================================================= */

/*
 * Count Unicode scalar values in a strict UTF-8 byte sequence.
 *
 * Returns false for malformed UTF-8.
 */
bool
vitte_unicode_count_utf8(
    const char *input,
    size_t length,
    size_t *out_count,
    size_t *error_offset);

/* ========================================================================= */
/* UTF-8 traversal                                                           */
/* ========================================================================= */

/*
 * Decode the scalar beginning at *offset and move *offset to the first byte
 * following it.
 *
 * Returns false on:
 *   - invalid argument;
 *   - end of input;
 *   - malformed UTF-8.
 *
 * On failure the caller's offset is not advanced.
 */
bool
vitte_unicode_next_utf8(
    const char *input,
    size_t length,
    size_t *offset,
    uint32_t *out_codepoint);

/*
 * Move backward by exactly one valid UTF-8 scalar.
 *
 * *offset initially denotes the byte immediately after the desired scalar.
 *
 * On success:
 *   *offset points to the scalar's leading byte;
 *   *out_codepoint contains the decoded scalar.
 *
 * Returns false if the preceding bytes do not form exactly one valid UTF-8
 * sequence.
 */
bool
vitte_unicode_previous_utf8(
    const char *input,
    size_t length,
    size_t *offset,
    uint32_t *out_codepoint);

/* ========================================================================= */
/* UTF-8 encoded length                                                      */
/* ========================================================================= */

/*
 * Return the number of UTF-8 bytes required for a scalar.
 *
 * Returns zero for invalid Unicode scalar values.
 */
size_t
vitte_unicode_utf8_length(
    uint32_t codepoint);

/* ========================================================================= */
/* UTF-16 decode                                                             */
/* ========================================================================= */

/*
 * Decode one UTF-16 scalar.
 *
 * If first is a high surrogate, has_second must be true and second must be a
 * low surrogate.
 *
 * On success:
 *   *out_units = 1 or 2.
 */
vitte_unicode_error_t
vitte_unicode_decode_utf16(
    uint16_t first,
    uint16_t second,
    bool has_second,
    uint32_t *out_codepoint,
    size_t *out_units);

/* ========================================================================= */
/* UTF-16 encode                                                             */
/* ========================================================================= */

/*
 * Encode one Unicode scalar into one or two UTF-16 code units.
 *
 * output must provide two uint16_t entries.
 */
vitte_unicode_error_t
vitte_unicode_encode_utf16(
    uint32_t codepoint,
    uint16_t output[2],
    size_t *out_units);

/* ========================================================================= */
/* ASCII classification                                                      */
/* ========================================================================= */

bool
vitte_unicode_is_ascii_alpha(
    uint32_t codepoint);

bool
vitte_unicode_is_ascii_digit(
    uint32_t codepoint);

bool
vitte_unicode_is_ascii_alnum(
    uint32_t codepoint);

bool
vitte_unicode_is_ascii_hex_digit(
    uint32_t codepoint);

/* ========================================================================= */
/* ASCII case conversion                                                     */
/* ========================================================================= */

/*
 * ASCII-only, locale-independent case conversion.
 *
 * Non-ASCII scalars are returned unchanged.
 */
uint32_t
vitte_unicode_ascii_to_lower(
    uint32_t codepoint);

uint32_t
vitte_unicode_ascii_to_upper(
    uint32_t codepoint);

/* ========================================================================= */
/* Unicode whitespace                                                        */
/* ========================================================================= */

/*
 * Recognizes:
 *   LF
 *   CR
 *   NEL
 *   LINE SEPARATOR
 *   PARAGRAPH SEPARATOR
 */
bool
vitte_unicode_is_newline(
    uint32_t codepoint);

/*
 * Deterministic Unicode whitespace classification suitable for source text.
 */
bool
vitte_unicode_is_whitespace(
    uint32_t codepoint);

/* ========================================================================= */
/* Unicode category helpers                                                  */
/* ========================================================================= */

/*
 * Combining-mark ranges used by identifier and display-width handling.
 */
bool
vitte_unicode_is_combining_mark(
    uint32_t codepoint);

/*
 * Unicode decimal digit ranges used by identifier continuation.
 */
bool
vitte_unicode_is_decimal_digit(
    uint32_t codepoint);

/* ========================================================================= */
/* Identifier classification                                                 */
/* ========================================================================= */

/*
 * Vitte identifier-start policy.
 *
 * Examples accepted:
 *
 *   hello
 *   _value
 *   café
 *   λ
 *   переменная
 *   日本語
 */
bool
vitte_unicode_is_identifier_start(
    uint32_t codepoint);

/*
 * Vitte identifier-continue policy.
 *
 * In addition to identifier-start characters, this accepts decimal digits,
 * combining marks and selected joiner/connector characters.
 */
bool
vitte_unicode_is_identifier_continue(
    uint32_t codepoint);

/*
 * Validate one complete UTF-8 Vitte identifier.
 *
 * Empty identifiers are rejected.
 *
 * error_offset receives the byte offset of:
 *   - malformed UTF-8;
 *   - an invalid first character;
 *   - an invalid continuation character.
 *
 * On success it receives length.
 */
bool
vitte_unicode_validate_identifier(
    const char *input,
    size_t length,
    size_t *error_offset);

/* ========================================================================= */
/* Control / printable                                                       */
/* ========================================================================= */

bool
vitte_unicode_is_control(
    uint32_t codepoint);

/*
 * Returns false for:
 *   - invalid Unicode scalar values;
 *   - C0/C1 controls;
 *   - Unicode noncharacters.
 */
bool
vitte_unicode_is_printable(
    uint32_t codepoint);

/* ========================================================================= */
/* Display width                                                             */
/* ========================================================================= */

/*
 * Approximate terminal/compiler-diagnostic width for one Unicode scalar.
 *
 * Returns:
 *
 *   -1  invalid/control
 *    0  zero-width/combining
 *    1  ordinary width
 *    2  wide/full-width
 */
int
vitte_unicode_display_width(
    uint32_t codepoint);

/*
 * Calculate display width for a strict UTF-8 byte sequence.
 *
 * Tabs advance to the next tab stop:
 *
 *   tab_width = 4
 *
 *   column 0 -> 4
 *   column 1 -> 4
 *   column 4 -> 8
 *
 * If tab_width == 0, VITTE_UNICODE_DEFAULT_TAB_WIDTH is used.
 *
 * initial_column permits diagnostics to calculate width when the text begins
 * after an existing prefix.
 */
bool
vitte_unicode_utf8_display_width(
    const char *input,
    size_t length,
    size_t tab_width,
    size_t initial_column,
    size_t *out_width,
    size_t *error_offset);

/* ========================================================================= */
/* UTF-8 BOM                                                                 */
/* ========================================================================= */

bool
vitte_unicode_has_utf8_bom(
    const char *input,
    size_t length);

/*
 * Returns:
 *
 *   3 if input starts with EF BB BF;
 *   0 otherwise.
 */
size_t
vitte_unicode_skip_utf8_bom(
    const char *input,
    size_t length);

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

/*
 * Hash raw UTF-8 bytes with deterministic 64-bit FNV-1a.
 *
 * This function intentionally hashes bytes, not normalized Unicode scalar
 * values.
 *
 * Therefore canonically equivalent Unicode spellings may produce different
 * hashes until normalization is explicitly introduced into the language
 * contract.
 */
uint64_t
vitte_unicode_hash_utf8(
    const char *input,
    size_t length);

/* ========================================================================= */
/* Analysis                                                                  */
/* ========================================================================= */

/*
 * Analyze a strict UTF-8 byte sequence.
 *
 * On valid input:
 *   returns true;
 *   stats->valid_utf8 = true;
 *   stats->first_error_offset = length.
 *
 * On malformed input:
 *   returns false;
 *   stats contains statistics up to the malformed sequence;
 *   stats->valid_utf8 = false;
 *   stats->first_error_offset identifies the malformed byte.
 */
bool
vitte_unicode_analyze_utf8(
    const char *input,
    size_t length,
    vitte_unicode_stats_t *stats);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_unicode_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline API version                                                        */
/* ========================================================================= */

static inline unsigned int
vitte_unicode_api_version_major(void)
{
    return VITTE_UNICODE_API_VERSION_MAJOR;
}

static inline unsigned int
vitte_unicode_api_version_minor(void)
{
    return VITTE_UNICODE_API_VERSION_MINOR;
}

static inline unsigned int
vitte_unicode_api_version_patch(void)
{
    return VITTE_UNICODE_API_VERSION_PATCH;
}

static inline unsigned int
vitte_unicode_api_version(void)
{
    return VITTE_UNICODE_API_VERSION;
}

/* ========================================================================= */
/* Inline error helpers                                                      */
/* ========================================================================= */

static inline bool
vitte_unicode_error_is_valid(
    vitte_unicode_error_t error)
{
    return error >=
               VITTE_UNICODE_ERROR_NONE &&
           error <
               VITTE_UNICODE_ERROR_COUNT;
}

static inline bool
vitte_unicode_error_is_success(
    vitte_unicode_error_t error)
{
    return error ==
           VITTE_UNICODE_ERROR_NONE;
}

static inline bool
vitte_unicode_error_is_utf8(
    vitte_unicode_error_t error)
{
    switch (error) {
        case VITTE_UNICODE_ERROR_INVALID_UTF8:
        case VITTE_UNICODE_ERROR_TRUNCATED_UTF8:
        case VITTE_UNICODE_ERROR_OVERLONG_UTF8:
        case VITTE_UNICODE_ERROR_SURROGATE:
        case VITTE_UNICODE_ERROR_OUT_OF_RANGE:
            return true;

        default:
            return false;
    }
}

/* ========================================================================= */
/* Inline decode-result helpers                                              */
/* ========================================================================= */

static inline bool
vitte_unicode_result_is_valid(
    const vitte_unicode_result_t *result)
{
    return result != NULL &&
           result->error ==
               VITTE_UNICODE_ERROR_NONE &&
           vitte_unicode_is_scalar(
               result->codepoint) &&
           result->length >= 1u &&
           result->length <=
               VITTE_UNICODE_MAX_UTF8_BYTES;
}

static inline bool
vitte_unicode_result_is_error(
    const vitte_unicode_result_t *result)
{
    return result == NULL ||
           result->error !=
               VITTE_UNICODE_ERROR_NONE;
}

/* ========================================================================= */
/* Inline scalar helpers                                                     */
/* ========================================================================= */

static inline bool
vitte_unicode_is_bmp(
    uint32_t codepoint)
{
    return vitte_unicode_is_scalar(
               codepoint) &&
           codepoint <=
               VITTE_UNICODE_MAX_BMP_CODEPOINT;
}

static inline bool
vitte_unicode_is_supplementary(
    uint32_t codepoint)
{
    return vitte_unicode_is_scalar(
               codepoint) &&
           codepoint >
               VITTE_UNICODE_MAX_BMP_CODEPOINT;
}

static inline bool
vitte_unicode_is_replacement_character(
    uint32_t codepoint)
{
    return codepoint ==
           VITTE_UNICODE_REPLACEMENT_CHARACTER;
}

static inline bool
vitte_unicode_is_bom(
    uint32_t codepoint)
{
    return codepoint ==
           VITTE_UNICODE_BYTE_ORDER_MARK;
}

/* ========================================================================= */
/* Inline ASCII helpers                                                      */
/* ========================================================================= */

static inline bool
vitte_unicode_is_ascii_lower(
    uint32_t codepoint)
{
    return codepoint >= (uint32_t)'a' &&
           codepoint <= (uint32_t)'z';
}

static inline bool
vitte_unicode_is_ascii_upper(
    uint32_t codepoint)
{
    return codepoint >= (uint32_t)'A' &&
           codepoint <= (uint32_t)'Z';
}

static inline bool
vitte_unicode_is_ascii_binary_digit(
    uint32_t codepoint)
{
    return codepoint == (uint32_t)'0' ||
           codepoint == (uint32_t)'1';
}

static inline bool
vitte_unicode_is_ascii_octal_digit(
    uint32_t codepoint)
{
    return codepoint >= (uint32_t)'0' &&
           codepoint <= (uint32_t)'7';
}

static inline bool
vitte_unicode_is_ascii_space(
    uint32_t codepoint)
{
    return codepoint == (uint32_t)' ' ||
           codepoint == (uint32_t)'\t' ||
           codepoint == (uint32_t)'\n' ||
           codepoint == (uint32_t)'\r' ||
           codepoint == (uint32_t)'\v' ||
           codepoint == (uint32_t)'\f';
}

static inline bool
vitte_unicode_is_ascii_horizontal_space(
    uint32_t codepoint)
{
    return codepoint == (uint32_t)' ' ||
           codepoint == (uint32_t)'\t';
}

static inline bool
vitte_unicode_is_ascii_newline(
    uint32_t codepoint)
{
    return codepoint == (uint32_t)'\n' ||
           codepoint == (uint32_t)'\r';
}

static inline bool
vitte_unicode_is_ascii_identifier_start(
    uint32_t codepoint)
{
    return codepoint == (uint32_t)'_' ||
           vitte_unicode_is_ascii_alpha(
               codepoint);
}

static inline bool
vitte_unicode_is_ascii_identifier_continue(
    uint32_t codepoint)
{
    return vitte_unicode_is_ascii_identifier_start(
               codepoint) ||
           vitte_unicode_is_ascii_digit(
               codepoint);
}

/* ========================================================================= */
/* Inline hexadecimal value                                                  */
/* ========================================================================= */

/*
 * Convert one ASCII hexadecimal digit into 0..15.
 *
 * Returns -1 for non-hexadecimal input.
 */
static inline int
vitte_unicode_ascii_hex_value(
    uint32_t codepoint)
{
    if (codepoint >= (uint32_t)'0' &&
        codepoint <= (uint32_t)'9') {
        return (int)(
            codepoint -
            (uint32_t)'0');
    }

    if (codepoint >= (uint32_t)'a' &&
        codepoint <= (uint32_t)'f') {
        return (int)(
            UINT32_C(10) +
            codepoint -
            (uint32_t)'a');
    }

    if (codepoint >= (uint32_t)'A' &&
        codepoint <= (uint32_t)'F') {
        return (int)(
            UINT32_C(10) +
            codepoint -
            (uint32_t)'A');
    }

    return -1;
}

/* ========================================================================= */
/* Inline UTF-8 byte classification                                          */
/* ========================================================================= */

static inline bool
vitte_unicode_utf8_is_ascii_byte(
    unsigned char byte)
{
    return byte <= 0x7fu;
}

static inline bool
vitte_unicode_utf8_is_continuation_byte(
    unsigned char byte)
{
    return (byte & 0xc0u) == 0x80u;
}

static inline bool
vitte_unicode_utf8_is_two_byte_lead(
    unsigned char byte)
{
    return byte >= 0xc2u &&
           byte <= 0xdfu;
}

static inline bool
vitte_unicode_utf8_is_three_byte_lead(
    unsigned char byte)
{
    return byte >= 0xe0u &&
           byte <= 0xefu;
}

static inline bool
vitte_unicode_utf8_is_four_byte_lead(
    unsigned char byte)
{
    return byte >= 0xf0u &&
           byte <= 0xf4u;
}

static inline bool
vitte_unicode_utf8_is_valid_lead(
    unsigned char byte)
{
    return vitte_unicode_utf8_is_ascii_byte(
               byte) ||
           vitte_unicode_utf8_is_two_byte_lead(
               byte) ||
           vitte_unicode_utf8_is_three_byte_lead(
               byte) ||
           vitte_unicode_utf8_is_four_byte_lead(
               byte);
}

/*
 * Determine the expected encoded byte count from a leading byte.
 *
 * Returns zero for invalid UTF-8 leading bytes.
 */
static inline size_t
vitte_unicode_utf8_expected_length(
    unsigned char byte)
{
    if (vitte_unicode_utf8_is_ascii_byte(
            byte)) {
        return 1u;
    }

    if (vitte_unicode_utf8_is_two_byte_lead(
            byte)) {
        return 2u;
    }

    if (vitte_unicode_utf8_is_three_byte_lead(
            byte)) {
        return 3u;
    }

    if (vitte_unicode_utf8_is_four_byte_lead(
            byte)) {
        return 4u;
    }

    return 0u;
}

/* ========================================================================= */
/* Inline UTF-16 classification                                              */
/* ========================================================================= */

static inline bool
vitte_unicode_utf16_is_high_surrogate(
    uint16_t unit)
{
    return unit >=
               VITTE_UNICODE_UTF16_HIGH_SURROGATE_FIRST &&
           unit <=
               VITTE_UNICODE_UTF16_HIGH_SURROGATE_LAST;
}

static inline bool
vitte_unicode_utf16_is_low_surrogate(
    uint16_t unit)
{
    return unit >=
               VITTE_UNICODE_UTF16_LOW_SURROGATE_FIRST &&
           unit <=
               VITTE_UNICODE_UTF16_LOW_SURROGATE_LAST;
}

static inline bool
vitte_unicode_utf16_is_surrogate(
    uint16_t unit)
{
    return vitte_unicode_utf16_is_high_surrogate(
               unit) ||
           vitte_unicode_utf16_is_low_surrogate(
               unit);
}

/* ========================================================================= */
/* Inline source-code helpers                                                */
/* ========================================================================= */

static inline bool
vitte_unicode_is_source_line_separator(
    uint32_t codepoint)
{
    return vitte_unicode_is_newline(
        codepoint);
}

static inline bool
vitte_unicode_is_source_horizontal_space(
    uint32_t codepoint)
{
    return vitte_unicode_is_whitespace(
               codepoint) &&
           !vitte_unicode_is_newline(
               codepoint);
}

/* ========================================================================= */
/* Inline diagnostic-width helpers                                           */
/* ========================================================================= */

static inline bool
vitte_unicode_is_zero_width(
    uint32_t codepoint)
{
    return vitte_unicode_display_width(
               codepoint) == 0;
}

static inline bool
vitte_unicode_is_wide(
    uint32_t codepoint)
{
    return vitte_unicode_display_width(
               codepoint) == 2;
}

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__cplusplus)

static_assert(
    VITTE_UNICODE_MAX_CODEPOINT ==
        UINT32_C(0x10ffff),
    "Unicode maximum scalar boundary must be U+10FFFF");

static_assert(
    VITTE_UNICODE_REPLACEMENT_CHARACTER ==
        UINT32_C(0xfffd),
    "Unicode replacement character must be U+FFFD");

static_assert(
    VITTE_UNICODE_BYTE_ORDER_MARK ==
        UINT32_C(0xfeff),
    "Unicode BOM must be U+FEFF");

static_assert(
    VITTE_UNICODE_MAX_UTF8_BYTES == 4u,
    "UTF-8 maximum sequence length must be four bytes");

static_assert(
    VITTE_UNICODE_MAX_UTF16_UNITS == 2u,
    "UTF-16 scalar encoding must use at most two code units");

static_assert(
    VITTE_UNICODE_UTF8_BOM_LENGTH == 3u,
    "UTF-8 BOM must contain three bytes");

static_assert(
    VITTE_UNICODE_ERROR_NONE == 0,
    "Unicode success error value must be zero");

static_assert(
    sizeof(uint32_t) >= 4u,
    "uint32_t must represent all Unicode scalar values");

static_assert(
    sizeof(uint16_t) >= 2u,
    "uint16_t must represent UTF-16 code units");

#else

_Static_assert(
    VITTE_UNICODE_MAX_CODEPOINT ==
        UINT32_C(0x10ffff),
    "Unicode maximum scalar boundary must be U+10FFFF");

_Static_assert(
    VITTE_UNICODE_REPLACEMENT_CHARACTER ==
        UINT32_C(0xfffd),
    "Unicode replacement character must be U+FFFD");

_Static_assert(
    VITTE_UNICODE_BYTE_ORDER_MARK ==
        UINT32_C(0xfeff),
    "Unicode BOM must be U+FEFF");

_Static_assert(
    VITTE_UNICODE_MAX_UTF8_BYTES == 4u,
    "UTF-8 maximum sequence length must be four bytes");

_Static_assert(
    VITTE_UNICODE_MAX_UTF16_UNITS == 2u,
    "UTF-16 scalar encoding must use at most two code units");

_Static_assert(
    VITTE_UNICODE_UTF8_BOM_LENGTH == 3u,
    "UTF-8 BOM must contain three bytes");

_Static_assert(
    VITTE_UNICODE_ERROR_NONE == 0,
    "Unicode success error value must be zero");

_Static_assert(
    sizeof(uint32_t) >= 4u,
    "uint32_t must represent all Unicode scalar values");

_Static_assert(
    sizeof(uint16_t) >= 2u,
    "uint16_t must represent UTF-16 code units");

#endif

#ifdef __cplusplus
}
#endif

#endif /* VITTE_UNICODE_UNICODE_H */
