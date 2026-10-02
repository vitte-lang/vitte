/*
 * Vitte Compiler
 * src/unicode/unicode.c
 *
 * Unicode subsystem.
 *
 * Public contract: unicode.h
 *
 * ISO C17.
 *
 * Provides:
 *   - strict UTF-8 decoding and encoding;
 *   - scalar validation;
 *   - UTF-8 validation;
 *   - code-point counting;
 *   - forward/backward traversal;
 *   - UTF-16 conversion primitives;
 *   - Unicode classification;
 *   - identifier start/continue classification;
 *   - whitespace/newline classification;
 *   - ASCII case conversion;
 *   - display-width estimation;
 *   - deterministic hashing;
 *   - statistics;
 *   - bounded operations.
 */

#include "unicode.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ========================================================================= */
/* Internal                                                                  */
/* ========================================================================= */

static uint64_t
vitte_unicode_hash_byte(
    uint64_t hash,
    unsigned char byte)
{
    hash ^= (uint64_t)byte;
    hash *= VITTE_UNICODE_FNV_PRIME;

    return hash;
}

static bool
vitte_unicode_is_continuation(
    unsigned char byte)
{
    return (byte & 0xc0u) == 0x80u;
}

static vitte_unicode_result_t
vitte_unicode_make_result(
    vitte_unicode_error_t error,
    uint32_t codepoint,
    size_t length)
{
    vitte_unicode_result_t result;

    result.error = error;
    result.codepoint = codepoint;
    result.length = length;

    return result;
}

static bool
vitte_unicode_in_range(
    uint32_t codepoint,
    uint32_t first,
    uint32_t last)
{
    return codepoint >= first &&
           codepoint <= last;
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_unicode_error_name(
    vitte_unicode_error_t error)
{
    switch (error) {
        case VITTE_UNICODE_ERROR_NONE:
            return "none";

        case VITTE_UNICODE_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case VITTE_UNICODE_ERROR_END_OF_INPUT:
            return "end_of_input";

        case VITTE_UNICODE_ERROR_INVALID_UTF8:
            return "invalid_utf8";

        case VITTE_UNICODE_ERROR_TRUNCATED_UTF8:
            return "truncated_utf8";

        case VITTE_UNICODE_ERROR_OVERLONG_UTF8:
            return "overlong_utf8";

        case VITTE_UNICODE_ERROR_SURROGATE:
            return "surrogate";

        case VITTE_UNICODE_ERROR_OUT_OF_RANGE:
            return "out_of_range";

        case VITTE_UNICODE_ERROR_INVALID_UTF16:
            return "invalid_utf16";

        case VITTE_UNICODE_ERROR_OUTPUT_TOO_SMALL:
            return "output_too_small";

        case VITTE_UNICODE_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_UNICODE_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Scalar validation                                                         */
/* ========================================================================= */

bool
vitte_unicode_is_scalar(
    uint32_t codepoint)
{
    if (codepoint > UINT32_C(0x10ffff)) {
        return false;
    }

    if (codepoint >= UINT32_C(0xd800) &&
        codepoint <= UINT32_C(0xdfff)) {
        return false;
    }

    return true;
}

bool
vitte_unicode_is_ascii(
    uint32_t codepoint)
{
    return codepoint <= UINT32_C(0x7f);
}

bool
vitte_unicode_is_surrogate(
    uint32_t codepoint)
{
    return codepoint >= UINT32_C(0xd800) &&
           codepoint <= UINT32_C(0xdfff);
}

/* ========================================================================= */
/* UTF-8 decode                                                              */
/* ========================================================================= */

vitte_unicode_result_t
vitte_unicode_decode_utf8(
    const unsigned char *input,
    size_t length)
{
    unsigned char b0;
    unsigned char b1;
    unsigned char b2;
    unsigned char b3;
    uint32_t codepoint;

    if (input == NULL) {
        return vitte_unicode_make_result(
            VITTE_UNICODE_ERROR_INVALID_ARGUMENT,
            VITTE_UNICODE_REPLACEMENT_CHARACTER,
            0u);
    }

    if (length == 0u) {
        return vitte_unicode_make_result(
            VITTE_UNICODE_ERROR_END_OF_INPUT,
            VITTE_UNICODE_REPLACEMENT_CHARACTER,
            0u);
    }

    b0 = input[0];

    /*
     * ASCII.
     */
    if (b0 <= 0x7fu) {
        return vitte_unicode_make_result(
            VITTE_UNICODE_ERROR_NONE,
            (uint32_t)b0,
            1u);
    }

    /*
     * 2-byte sequence:
     *
     * C2..DF 80..BF
     *
     * C0/C1 are excluded because they can only produce overlong encodings.
     */
    if (b0 >= 0xc2u &&
        b0 <= 0xdfu) {
        if (length < 2u) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_TRUNCATED_UTF8,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        b1 = input[1];

        if (!vitte_unicode_is_continuation(b1)) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_INVALID_UTF8,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        codepoint =
            ((uint32_t)(b0 & 0x1fu) << 6u) |
            (uint32_t)(b1 & 0x3fu);

        return vitte_unicode_make_result(
            VITTE_UNICODE_ERROR_NONE,
            codepoint,
            2u);
    }

    /*
     * 3-byte sequence.
     */
    if (b0 >= 0xe0u &&
        b0 <= 0xefu) {
        if (length < 3u) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_TRUNCATED_UTF8,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        b1 = input[1];
        b2 = input[2];

        if (!vitte_unicode_is_continuation(b1) ||
            !vitte_unicode_is_continuation(b2)) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_INVALID_UTF8,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        /*
         * E0 A0..BF prevents overlong U+0000..U+07FF.
         */
        if (b0 == 0xe0u &&
            b1 < 0xa0u) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_OVERLONG_UTF8,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        /*
         * ED 80..9F only.
         * ED A0..BF would encode UTF-16 surrogates.
         */
        if (b0 == 0xedu &&
            b1 >= 0xa0u) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_SURROGATE,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        codepoint =
            ((uint32_t)(b0 & 0x0fu) << 12u) |
            ((uint32_t)(b1 & 0x3fu) << 6u) |
            (uint32_t)(b2 & 0x3fu);

        if (!vitte_unicode_is_scalar(codepoint)) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_SURROGATE,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        return vitte_unicode_make_result(
            VITTE_UNICODE_ERROR_NONE,
            codepoint,
            3u);
    }

    /*
     * 4-byte sequence.
     */
    if (b0 >= 0xf0u &&
        b0 <= 0xf4u) {
        if (length < 4u) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_TRUNCATED_UTF8,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        b1 = input[1];
        b2 = input[2];
        b3 = input[3];

        if (!vitte_unicode_is_continuation(b1) ||
            !vitte_unicode_is_continuation(b2) ||
            !vitte_unicode_is_continuation(b3)) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_INVALID_UTF8,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        /*
         * F0 90..BF prevents overlong values below U+10000.
         */
        if (b0 == 0xf0u &&
            b1 < 0x90u) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_OVERLONG_UTF8,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        /*
         * F4 80..8F only, because Unicode ends at U+10FFFF.
         */
        if (b0 == 0xf4u &&
            b1 > 0x8fu) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_OUT_OF_RANGE,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        codepoint =
            ((uint32_t)(b0 & 0x07u) << 18u) |
            ((uint32_t)(b1 & 0x3fu) << 12u) |
            ((uint32_t)(b2 & 0x3fu) << 6u) |
            (uint32_t)(b3 & 0x3fu);

        if (codepoint > UINT32_C(0x10ffff)) {
            return vitte_unicode_make_result(
                VITTE_UNICODE_ERROR_OUT_OF_RANGE,
                VITTE_UNICODE_REPLACEMENT_CHARACTER,
                1u);
        }

        return vitte_unicode_make_result(
            VITTE_UNICODE_ERROR_NONE,
            codepoint,
            4u);
    }

    /*
     * C0/C1: explicitly overlong prefix.
     */
    if (b0 == 0xc0u ||
        b0 == 0xc1u) {
        return vitte_unicode_make_result(
            VITTE_UNICODE_ERROR_OVERLONG_UTF8,
            VITTE_UNICODE_REPLACEMENT_CHARACTER,
            1u);
    }

    /*
     * F5..FF can never start valid Unicode UTF-8.
     */
    if (b0 >= 0xf5u) {
        return vitte_unicode_make_result(
            VITTE_UNICODE_ERROR_OUT_OF_RANGE,
            VITTE_UNICODE_REPLACEMENT_CHARACTER,
            1u);
    }

    /*
     * Bare continuation byte or otherwise invalid leading byte.
     */
    return vitte_unicode_make_result(
        VITTE_UNICODE_ERROR_INVALID_UTF8,
        VITTE_UNICODE_REPLACEMENT_CHARACTER,
        1u);
}

/* ========================================================================= */
/* UTF-8 encode                                                              */
/* ========================================================================= */

vitte_unicode_error_t
vitte_unicode_encode_utf8(
    uint32_t codepoint,
    unsigned char output[4],
    size_t *out_length)
{
    if (output == NULL ||
        out_length == NULL) {
        return VITTE_UNICODE_ERROR_INVALID_ARGUMENT;
    }

    *out_length = 0u;

    if (!vitte_unicode_is_scalar(codepoint)) {
        if (vitte_unicode_is_surrogate(codepoint)) {
            return VITTE_UNICODE_ERROR_SURROGATE;
        }

        return VITTE_UNICODE_ERROR_OUT_OF_RANGE;
    }

    if (codepoint <= UINT32_C(0x7f)) {
        output[0] =
            (unsigned char)codepoint;

        *out_length = 1u;

        return VITTE_UNICODE_ERROR_NONE;
    }

    if (codepoint <= UINT32_C(0x7ff)) {
        output[0] =
            (unsigned char)(
                0xc0u |
                ((codepoint >> 6u) & 0x1fu));

        output[1] =
            (unsigned char)(
                0x80u |
                (codepoint & 0x3fu));

        *out_length = 2u;

        return VITTE_UNICODE_ERROR_NONE;
    }

    if (codepoint <= UINT32_C(0xffff)) {
        output[0] =
            (unsigned char)(
                0xe0u |
                ((codepoint >> 12u) & 0x0fu));

        output[1] =
            (unsigned char)(
                0x80u |
                ((codepoint >> 6u) & 0x3fu));

        output[2] =
            (unsigned char)(
                0x80u |
                (codepoint & 0x3fu));

        *out_length = 3u;

        return VITTE_UNICODE_ERROR_NONE;
    }

    output[0] =
        (unsigned char)(
            0xf0u |
            ((codepoint >> 18u) & 0x07u));

    output[1] =
        (unsigned char)(
            0x80u |
            ((codepoint >> 12u) & 0x3fu));

    output[2] =
        (unsigned char)(
            0x80u |
            ((codepoint >> 6u) & 0x3fu));

    output[3] =
        (unsigned char)(
            0x80u |
            (codepoint & 0x3fu));

    *out_length = 4u;

    return VITTE_UNICODE_ERROR_NONE;
}

/* ========================================================================= */
/* UTF-8 validation                                                          */
/* ========================================================================= */

bool
vitte_unicode_validate_utf8(
    const char *input,
    size_t length,
    size_t *error_offset)
{
    size_t offset;

    if (error_offset != NULL) {
        *error_offset = 0u;
    }

    if (input == NULL) {
        return length == 0u;
    }

    offset = 0u;

    while (offset < length) {
        const vitte_unicode_result_t result =
            vitte_unicode_decode_utf8(
                (const unsigned char *)input +
                    offset,
                length - offset);

        if (result.error !=
            VITTE_UNICODE_ERROR_NONE) {
            if (error_offset != NULL) {
                *error_offset = offset;
            }

            return false;
        }

        if (result.length == 0u ||
            result.length > length - offset) {
            if (error_offset != NULL) {
                *error_offset = offset;
            }

            return false;
        }

        offset += result.length;
    }

    if (error_offset != NULL) {
        *error_offset = length;
    }

    return true;
}

/* ========================================================================= */
/* UTF-8 count                                                               */
/* ========================================================================= */

bool
vitte_unicode_count_utf8(
    const char *input,
    size_t length,
    size_t *out_count,
    size_t *error_offset)
{
    size_t offset;
    size_t count;

    if (out_count == NULL) {
        return false;
    }

    *out_count = 0u;

    if (error_offset != NULL) {
        *error_offset = 0u;
    }

    if (input == NULL) {
        return length == 0u;
    }

    offset = 0u;
    count = 0u;

    while (offset < length) {
        const vitte_unicode_result_t result =
            vitte_unicode_decode_utf8(
                (const unsigned char *)input +
                    offset,
                length - offset);

        if (result.error !=
            VITTE_UNICODE_ERROR_NONE) {
            if (error_offset != NULL) {
                *error_offset = offset;
            }

            return false;
        }

        if (count == SIZE_MAX) {
            if (error_offset != NULL) {
                *error_offset = offset;
            }

            return false;
        }

        count++;
        offset += result.length;
    }

    *out_count = count;

    if (error_offset != NULL) {
        *error_offset = length;
    }

    return true;
}

/* ========================================================================= */
/* UTF-8 traversal                                                           */
/* ========================================================================= */

bool
vitte_unicode_next_utf8(
    const char *input,
    size_t length,
    size_t *offset,
    uint32_t *out_codepoint)
{
    vitte_unicode_result_t result;

    if (input == NULL ||
        offset == NULL ||
        out_codepoint == NULL) {
        return false;
    }

    if (*offset >= length) {
        return false;
    }

    result =
        vitte_unicode_decode_utf8(
            (const unsigned char *)input +
                *offset,
            length - *offset);

    if (result.error !=
        VITTE_UNICODE_ERROR_NONE) {
        return false;
    }

    *out_codepoint = result.codepoint;
    *offset += result.length;

    return true;
}

bool
vitte_unicode_previous_utf8(
    const char *input,
    size_t length,
    size_t *offset,
    uint32_t *out_codepoint)
{
    size_t start;
    size_t continuation_count;
    vitte_unicode_result_t result;

    if (input == NULL ||
        offset == NULL ||
        out_codepoint == NULL) {
        return false;
    }

    if (*offset == 0u ||
        *offset > length) {
        return false;
    }

    start = *offset - 1u;
    continuation_count = 0u;

    while (start > 0u &&
           vitte_unicode_is_continuation(
               (unsigned char)input[start])) {
        continuation_count++;

        /*
         * Valid UTF-8 has at most three continuation bytes.
         */
        if (continuation_count > 3u) {
            return false;
        }

        start--;
    }

    /*
     * If the final byte is a continuation and start reached zero, zero may be
     * the leading byte. Decode from there and require the sequence to end
     * exactly at the original offset.
     */
    result =
        vitte_unicode_decode_utf8(
            (const unsigned char *)input +
                start,
            *offset - start);

    if (result.error !=
            VITTE_UNICODE_ERROR_NONE ||
        result.length != *offset - start) {
        return false;
    }

    *offset = start;
    *out_codepoint = result.codepoint;

    return true;
}

/* ========================================================================= */
/* UTF-8 length                                                              */
/* ========================================================================= */

size_t
vitte_unicode_utf8_length(
    uint32_t codepoint)
{
    if (!vitte_unicode_is_scalar(codepoint)) {
        return 0u;
    }

    if (codepoint <= UINT32_C(0x7f)) {
        return 1u;
    }

    if (codepoint <= UINT32_C(0x7ff)) {
        return 2u;
    }

    if (codepoint <= UINT32_C(0xffff)) {
        return 3u;
    }

    return 4u;
}

/* ========================================================================= */
/* UTF-16                                                                    */
/* ========================================================================= */

vitte_unicode_error_t
vitte_unicode_decode_utf16(
    uint16_t first,
    uint16_t second,
    bool has_second,
    uint32_t *out_codepoint,
    size_t *out_units)
{
    uint32_t high;
    uint32_t low;

    if (out_codepoint == NULL ||
        out_units == NULL) {
        return VITTE_UNICODE_ERROR_INVALID_ARGUMENT;
    }

    *out_codepoint =
        VITTE_UNICODE_REPLACEMENT_CHARACTER;
    *out_units = 0u;

    if (first >= UINT16_C(0xd800) &&
        first <= UINT16_C(0xdbff)) {
        if (!has_second) {
            return VITTE_UNICODE_ERROR_INVALID_UTF16;
        }

        if (second < UINT16_C(0xdc00) ||
            second > UINT16_C(0xdfff)) {
            return VITTE_UNICODE_ERROR_INVALID_UTF16;
        }

        high =
            (uint32_t)first -
            UINT32_C(0xd800);

        low =
            (uint32_t)second -
            UINT32_C(0xdc00);

        *out_codepoint =
            UINT32_C(0x10000) +
            (high << 10u) +
            low;

        *out_units = 2u;

        return VITTE_UNICODE_ERROR_NONE;
    }

    if (first >= UINT16_C(0xdc00) &&
        first <= UINT16_C(0xdfff)) {
        return VITTE_UNICODE_ERROR_INVALID_UTF16;
    }

    *out_codepoint = (uint32_t)first;
    *out_units = 1u;

    return VITTE_UNICODE_ERROR_NONE;
}

vitte_unicode_error_t
vitte_unicode_encode_utf16(
    uint32_t codepoint,
    uint16_t output[2],
    size_t *out_units)
{
    uint32_t value;

    if (output == NULL ||
        out_units == NULL) {
        return VITTE_UNICODE_ERROR_INVALID_ARGUMENT;
    }

    *out_units = 0u;

    if (!vitte_unicode_is_scalar(codepoint)) {
        if (vitte_unicode_is_surrogate(codepoint)) {
            return VITTE_UNICODE_ERROR_SURROGATE;
        }

        return VITTE_UNICODE_ERROR_OUT_OF_RANGE;
    }

    if (codepoint <= UINT32_C(0xffff)) {
        output[0] = (uint16_t)codepoint;
        *out_units = 1u;

        return VITTE_UNICODE_ERROR_NONE;
    }

    value = codepoint - UINT32_C(0x10000);

    output[0] =
        (uint16_t)(
            UINT32_C(0xd800) +
            (value >> 10u));

    output[1] =
        (uint16_t)(
            UINT32_C(0xdc00) +
            (value & UINT32_C(0x3ff)));

    *out_units = 2u;

    return VITTE_UNICODE_ERROR_NONE;
}

/* ========================================================================= */
/* ASCII                                                                     */
/* ========================================================================= */

bool
vitte_unicode_is_ascii_alpha(
    uint32_t codepoint)
{
    return (codepoint >= (uint32_t)'A' &&
            codepoint <= (uint32_t)'Z') ||
           (codepoint >= (uint32_t)'a' &&
            codepoint <= (uint32_t)'z');
}

bool
vitte_unicode_is_ascii_digit(
    uint32_t codepoint)
{
    return codepoint >= (uint32_t)'0' &&
           codepoint <= (uint32_t)'9';
}

bool
vitte_unicode_is_ascii_alnum(
    uint32_t codepoint)
{
    return vitte_unicode_is_ascii_alpha(
               codepoint) ||
           vitte_unicode_is_ascii_digit(
               codepoint);
}

bool
vitte_unicode_is_ascii_hex_digit(
    uint32_t codepoint)
{
    return vitte_unicode_is_ascii_digit(
               codepoint) ||
           (codepoint >= (uint32_t)'A' &&
            codepoint <= (uint32_t)'F') ||
           (codepoint >= (uint32_t)'a' &&
            codepoint <= (uint32_t)'f');
}

uint32_t
vitte_unicode_ascii_to_lower(
    uint32_t codepoint)
{
    if (codepoint >= (uint32_t)'A' &&
        codepoint <= (uint32_t)'Z') {
        return codepoint +
               ((uint32_t)'a' -
                (uint32_t)'A');
    }

    return codepoint;
}

uint32_t
vitte_unicode_ascii_to_upper(
    uint32_t codepoint)
{
    if (codepoint >= (uint32_t)'a' &&
        codepoint <= (uint32_t)'z') {
        return codepoint -
               ((uint32_t)'a' -
                (uint32_t)'A');
    }

    return codepoint;
}

/* ========================================================================= */
/* Unicode whitespace                                                        */
/* ========================================================================= */

bool
vitte_unicode_is_newline(
    uint32_t codepoint)
{
    return codepoint == UINT32_C(0x000a) ||
           codepoint == UINT32_C(0x000d) ||
           codepoint == UINT32_C(0x0085) ||
           codepoint == UINT32_C(0x2028) ||
           codepoint == UINT32_C(0x2029);
}

bool
vitte_unicode_is_whitespace(
    uint32_t codepoint)
{
    switch (codepoint) {
        case UINT32_C(0x0009):
        case UINT32_C(0x000a):
        case UINT32_C(0x000b):
        case UINT32_C(0x000c):
        case UINT32_C(0x000d):
        case UINT32_C(0x0020):
        case UINT32_C(0x0085):
        case UINT32_C(0x00a0):
        case UINT32_C(0x1680):
        case UINT32_C(0x2000):
        case UINT32_C(0x2001):
        case UINT32_C(0x2002):
        case UINT32_C(0x2003):
        case UINT32_C(0x2004):
        case UINT32_C(0x2005):
        case UINT32_C(0x2006):
        case UINT32_C(0x2007):
        case UINT32_C(0x2008):
        case UINT32_C(0x2009):
        case UINT32_C(0x200a):
        case UINT32_C(0x2028):
        case UINT32_C(0x2029):
        case UINT32_C(0x202f):
        case UINT32_C(0x205f):
        case UINT32_C(0x3000):
            return true;

        default:
            return false;
    }
}

/* ========================================================================= */
/* Unicode categories useful to identifiers                                  */
/* ========================================================================= */

bool
vitte_unicode_is_combining_mark(
    uint32_t codepoint)
{
    /*
     * Major combining-mark ranges required by compiler identifiers and
     * display handling. This remains deterministic and locale-independent.
     */
    return
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0300),
            UINT32_C(0x036f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0483),
            UINT32_C(0x0489)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0591),
            UINT32_C(0x05bd)) ||
        codepoint == UINT32_C(0x05bf) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x05c1),
            UINT32_C(0x05c2)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x05c4),
            UINT32_C(0x05c5)) ||
        codepoint == UINT32_C(0x05c7) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0610),
            UINT32_C(0x061a)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x064b),
            UINT32_C(0x065f)) ||
        codepoint == UINT32_C(0x0670) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x06d6),
            UINT32_C(0x06ed)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0711),
            UINT32_C(0x0711)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0730),
            UINT32_C(0x074a)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x07a6),
            UINT32_C(0x07b0)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x07eb),
            UINT32_C(0x07f3)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0816),
            UINT32_C(0x082d)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0859),
            UINT32_C(0x085b)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x08d3),
            UINT32_C(0x0903)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x093a),
            UINT32_C(0x094f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0951),
            UINT32_C(0x0957)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0962),
            UINT32_C(0x0963)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x1ab0),
            UINT32_C(0x1aff)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x1dc0),
            UINT32_C(0x1dff)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x20d0),
            UINT32_C(0x20ff)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xfe00),
            UINT32_C(0xfe0f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xfe20),
            UINT32_C(0xfe2f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xe0100),
            UINT32_C(0xe01ef));
}

bool
vitte_unicode_is_decimal_digit(
    uint32_t codepoint)
{
    /*
     * Unicode Nd blocks commonly used by source text.
     */
    return
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0030),
            UINT32_C(0x0039)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0660),
            UINT32_C(0x0669)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x06f0),
            UINT32_C(0x06f9)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x07c0),
            UINT32_C(0x07c9)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0966),
            UINT32_C(0x096f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x09e6),
            UINT32_C(0x09ef)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0a66),
            UINT32_C(0x0a6f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0ae6),
            UINT32_C(0x0aef)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0b66),
            UINT32_C(0x0b6f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0be6),
            UINT32_C(0x0bef)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0c66),
            UINT32_C(0x0c6f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0ce6),
            UINT32_C(0x0cef)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0d66),
            UINT32_C(0x0d6f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0de6),
            UINT32_C(0x0def)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0e50),
            UINT32_C(0x0e59)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0ed0),
            UINT32_C(0x0ed9)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0f20),
            UINT32_C(0x0f29)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x1040),
            UINT32_C(0x1049)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x17e0),
            UINT32_C(0x17e9)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x1810),
            UINT32_C(0x1819)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xff10),
            UINT32_C(0xff19));
}

/* ========================================================================= */
/* Identifier classification                                                 */
/* ========================================================================= */

bool
vitte_unicode_is_identifier_start(
    uint32_t codepoint)
{
    /*
     * Vitte accepts '_' explicitly.
     */
    if (codepoint == (uint32_t)'_') {
        return true;
    }

    if (vitte_unicode_is_ascii_alpha(codepoint)) {
        return true;
    }

    if (!vitte_unicode_is_scalar(codepoint)) {
        return false;
    }

    /*
     * Exclude obvious non-letter classes before accepting the major Unicode
     * letter/script ranges.
     */
    if (vitte_unicode_is_whitespace(codepoint) ||
        vitte_unicode_is_decimal_digit(codepoint) ||
        vitte_unicode_is_combining_mark(codepoint)) {
        return false;
    }

    /*
     * Latin / IPA / Greek / Cyrillic.
     */
    if (vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x00c0),
            UINT32_C(0x02ff)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0370),
            UINT32_C(0x052f))) {
        return true;
    }

    /*
     * Armenian, Hebrew, Arabic, Syriac, Thaana, NKo and neighboring scripts.
     */
    if (vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0531),
            UINT32_C(0x08ff))) {
        return true;
    }

    /*
     * Indic scripts.
     */
    if (vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x0900),
            UINT32_C(0x1cff))) {
        return true;
    }

    /*
     * Latin/Greek extensions and alphabetic presentation.
     */
    if (vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x1e00),
            UINT32_C(0x2cff))) {
        return true;
    }

    /*
     * CJK, Hiragana, Katakana, Hangul and compatibility ideographs.
     */
    if (vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x3040),
            UINT32_C(0x30ff)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x3100),
            UINT32_C(0x312f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x3130),
            UINT32_C(0x318f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x31a0),
            UINT32_C(0x31bf)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x3400),
            UINT32_C(0x4dbf)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x4e00),
            UINT32_C(0x9fff)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xac00),
            UINT32_C(0xd7a3)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xf900),
            UINT32_C(0xfaff))) {
        return true;
    }

    /*
     * Full-width Latin letters.
     */
    if (vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xff21),
            UINT32_C(0xff3a)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xff41),
            UINT32_C(0xff5a))) {
        return true;
    }

    /*
     * Supplementary-plane historic/modern scripts and CJK extensions.
     *
     * This intentionally rejects emoji/symbol planes rather than accepting
     * every non-ASCII scalar indiscriminately.
     */
    if (vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x10000),
            UINT32_C(0x1d7ff)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x20000),
            UINT32_C(0x3134f))) {
        return true;
    }

    return false;
}

bool
vitte_unicode_is_identifier_continue(
    uint32_t codepoint)
{
    if (vitte_unicode_is_identifier_start(
            codepoint)) {
        return true;
    }

    if (vitte_unicode_is_decimal_digit(
            codepoint)) {
        return true;
    }

    if (vitte_unicode_is_combining_mark(
            codepoint)) {
        return true;
    }

    /*
     * U+200C ZERO WIDTH NON-JOINER and U+200D ZERO WIDTH JOINER are used by
     * several writing systems in identifiers.
     */
    if (codepoint == UINT32_C(0x200c) ||
        codepoint == UINT32_C(0x200d)) {
        return true;
    }

    /*
     * Connector punctuation commonly accepted in identifier continuation.
     */
    if (codepoint == UINT32_C(0x203f) ||
        codepoint == UINT32_C(0x2040) ||
        codepoint == UINT32_C(0x2054) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xfe33),
            UINT32_C(0xfe34)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xfe4d),
            UINT32_C(0xfe4f)) ||
        codepoint == UINT32_C(0xff3f)) {
        return true;
    }

    return false;
}

bool
vitte_unicode_validate_identifier(
    const char *input,
    size_t length,
    size_t *error_offset)
{
    size_t offset;
    bool first;

    if (error_offset != NULL) {
        *error_offset = 0u;
    }

    if (input == NULL ||
        length == 0u) {
        return false;
    }

    offset = 0u;
    first = true;

    while (offset < length) {
        const vitte_unicode_result_t result =
            vitte_unicode_decode_utf8(
                (const unsigned char *)input +
                    offset,
                length - offset);

        if (result.error !=
            VITTE_UNICODE_ERROR_NONE) {
            if (error_offset != NULL) {
                *error_offset = offset;
            }

            return false;
        }

        if (first) {
            if (!vitte_unicode_is_identifier_start(
                    result.codepoint)) {
                if (error_offset != NULL) {
                    *error_offset = offset;
                }

                return false;
            }

            first = false;
        } else if (!vitte_unicode_is_identifier_continue(
                       result.codepoint)) {
            if (error_offset != NULL) {
                *error_offset = offset;
            }

            return false;
        }

        offset += result.length;
    }

    if (error_offset != NULL) {
        *error_offset = length;
    }

    return !first;
}

/* ========================================================================= */
/* Control / printable                                                       */
/* ========================================================================= */

bool
vitte_unicode_is_control(
    uint32_t codepoint)
{
    return codepoint <= UINT32_C(0x001f) ||
           vitte_unicode_in_range(
               codepoint,
               UINT32_C(0x007f),
               UINT32_C(0x009f));
}

bool
vitte_unicode_is_printable(
    uint32_t codepoint)
{
    if (!vitte_unicode_is_scalar(codepoint)) {
        return false;
    }

    if (vitte_unicode_is_control(codepoint)) {
        return false;
    }

    /*
     * Unicode noncharacters.
     */
    if (vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xfdd0),
            UINT32_C(0xfdef))) {
        return false;
    }

    if ((codepoint & UINT32_C(0xffff)) ==
            UINT32_C(0xfffe) ||
        (codepoint & UINT32_C(0xffff)) ==
            UINT32_C(0xffff)) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Display width                                                             */
/* ========================================================================= */

int
vitte_unicode_display_width(
    uint32_t codepoint)
{
    if (!vitte_unicode_is_scalar(codepoint)) {
        return -1;
    }

    if (codepoint == UINT32_C(0)) {
        return 0;
    }

    if (vitte_unicode_is_control(codepoint)) {
        return -1;
    }

    if (vitte_unicode_is_combining_mark(
            codepoint)) {
        return 0;
    }

    /*
     * Zero-width formatting characters.
     */
    if (codepoint == UINT32_C(0x200b) ||
        codepoint == UINT32_C(0x200c) ||
        codepoint == UINT32_C(0x200d) ||
        codepoint == UINT32_C(0x2060) ||
        codepoint == UINT32_C(0xfeff) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xfe00),
            UINT32_C(0xfe0f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xe0100),
            UINT32_C(0xe01ef))) {
        return 0;
    }

    /*
     * East Asian wide/full-width ranges and emoji ranges.
     *
     * This is deliberately deterministic and locale-independent. Ambiguous
     * East Asian width characters remain width 1.
     */
    if (vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x1100),
            UINT32_C(0x115f)) ||
        codepoint == UINT32_C(0x2329) ||
        codepoint == UINT32_C(0x232a) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x2e80),
            UINT32_C(0xa4cf)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xac00),
            UINT32_C(0xd7a3)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xf900),
            UINT32_C(0xfaff)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xfe10),
            UINT32_C(0xfe19)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xfe30),
            UINT32_C(0xfe6f)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xff00),
            UINT32_C(0xff60)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0xffe0),
            UINT32_C(0xffe6)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x1f300),
            UINT32_C(0x1faff)) ||
        vitte_unicode_in_range(
            codepoint,
            UINT32_C(0x20000),
            UINT32_C(0x3fffd))) {
        return 2;
    }

    return 1;
}

bool
vitte_unicode_utf8_display_width(
    const char *input,
    size_t length,
    size_t tab_width,
    size_t initial_column,
    size_t *out_width,
    size_t *error_offset)
{
    size_t offset;
    size_t column;

    if (out_width == NULL) {
        return false;
    }

    *out_width = 0u;

    if (error_offset != NULL) {
        *error_offset = 0u;
    }

    if (input == NULL) {
        return length == 0u;
    }

    if (tab_width == 0u) {
        tab_width = VITTE_UNICODE_DEFAULT_TAB_WIDTH;
    }

    offset = 0u;
    column = initial_column;

    while (offset < length) {
        const vitte_unicode_result_t result =
            vitte_unicode_decode_utf8(
                (const unsigned char *)input +
                    offset,
                length - offset);

        size_t addition;

        if (result.error !=
            VITTE_UNICODE_ERROR_NONE) {
            if (error_offset != NULL) {
                *error_offset = offset;
            }

            return false;
        }

        if (result.codepoint ==
            UINT32_C(0x0009)) {
            const size_t remainder =
                column % tab_width;

            addition =
                remainder == 0u
                    ? tab_width
                    : tab_width - remainder;
        } else {
            const int width =
                vitte_unicode_display_width(
                    result.codepoint);

            if (width < 0) {
                addition = 0u;
            } else {
                addition = (size_t)width;
            }
        }

        if (column > SIZE_MAX - addition) {
            if (error_offset != NULL) {
                *error_offset = offset;
            }

            return false;
        }

        column += addition;
        offset += result.length;
    }

    if (column < initial_column) {
        return false;
    }

    *out_width = column - initial_column;

    if (error_offset != NULL) {
        *error_offset = length;
    }

    return true;
}

/* ========================================================================= */
/* BOM                                                                       */
/* ========================================================================= */

bool
vitte_unicode_has_utf8_bom(
    const char *input,
    size_t length)
{
    const unsigned char *bytes;

    if (input == NULL ||
        length < 3u) {
        return false;
    }

    bytes =
        (const unsigned char *)input;

    return bytes[0] == 0xefu &&
           bytes[1] == 0xbbu &&
           bytes[2] == 0xbfu;
}

size_t
vitte_unicode_skip_utf8_bom(
    const char *input,
    size_t length)
{
    return vitte_unicode_has_utf8_bom(
               input,
               length)
        ? 3u
        : 0u;
}

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

uint64_t
vitte_unicode_hash_utf8(
    const char *input,
    size_t length)
{
    uint64_t hash;
    size_t index;

    hash = VITTE_UNICODE_FNV_OFFSET;

    if (input == NULL) {
        return length == 0u
            ? hash
            : UINT64_C(0);
    }

    for (index = 0u;
         index < length;
         ++index) {
        hash =
            vitte_unicode_hash_byte(
                hash,
                (unsigned char)input[index]);
    }

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

bool
vitte_unicode_analyze_utf8(
    const char *input,
    size_t length,
    vitte_unicode_stats_t *stats)
{
    size_t offset;

    if (stats == NULL) {
        return false;
    }

    memset(stats, 0, sizeof(*stats));

    stats->bytes = length;

    if (input == NULL) {
        return length == 0u;
    }

    offset = 0u;

    if (vitte_unicode_has_utf8_bom(
            input,
            length)) {
        stats->has_bom = true;
    }

    while (offset < length) {
        const vitte_unicode_result_t result =
            vitte_unicode_decode_utf8(
                (const unsigned char *)input +
                    offset,
                length - offset);

        if (result.error !=
            VITTE_UNICODE_ERROR_NONE) {
            stats->valid_utf8 = false;
            stats->invalid_sequences++;
            stats->first_error_offset = offset;

            return false;
        }

        stats->codepoints++;

        if (result.codepoint <=
            UINT32_C(0x7f)) {
            stats->ascii_codepoints++;
        } else {
            stats->non_ascii_codepoints++;
        }

        if (vitte_unicode_is_newline(
                result.codepoint)) {
            stats->newlines++;
        }

        if (vitte_unicode_is_whitespace(
                result.codepoint)) {
            stats->whitespace_codepoints++;
        }

        if (vitte_unicode_is_combining_mark(
                result.codepoint)) {
            stats->combining_codepoints++;
        }

        offset += result.length;
    }

    stats->valid_utf8 = true;
    stats->first_error_offset = length;

    return true;
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_unicode_translation_unit_anchor(void)
{
    /*
     * Stable link-time anchor.
     */
}
