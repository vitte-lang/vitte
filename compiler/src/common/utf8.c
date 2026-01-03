// SPDX-License-Identifier: MIT
// utf8.c
//
// UTF-8 utilities for vitte.
//
// Scope:
//  - Validate UTF-8 sequences
//  - Decode/encode one codepoint
//  - Count codepoints
//  - Advance/rewind by codepoints (best-effort)
//  - Simple ASCII fast paths
//
// Notes:
//  - This module treats invalid sequences conservatively.
//  - It supports decoding to Unicode scalar values (U+0000..U+10FFFF excluding surrogates).
//  - It rejects overlong encodings.
//  - It can optionally accept NUL within buffers when length is provided.
//
// Pairing:
//  - utf8.h should declare the functions here and define types:
//      typedef struct { const uint8_t* ptr; size_t len; } steel_u8_view;   (optional)
//      typedef struct { const char* ptr; size_t len; } steel_str_view;     (optional)
//    If your header differs, adjust accordingly.

#include "utf8.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifndef STEEL_UTF8_REPLACEMENT
  #define STEEL_UTF8_REPLACEMENT 0xFFFDu
#endif

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------

static inline bool steel_u_is_surrogate(uint32_t cp)
{
    return (cp >= 0xD800u && cp <= 0xDFFFu);
}

static inline bool steel_u_is_scalar(uint32_t cp)
{
    return (cp <= 0x10FFFFu) && !steel_u_is_surrogate(cp);
}

static inline uint8_t steel_u8_at(const uint8_t* s, size_t i)
{
    return s[i];
}

static inline bool steel_u8_is_cont(uint8_t b)
{
    return (b & 0xC0u) == 0x80u;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

// Returns number of bytes consumed (1..4) on success, 0 on failure.
size_t steel_utf8_decode_one(const uint8_t* s, size_t len, uint32_t* out_cp)
{
    if (out_cp) *out_cp = 0;
    if (!s || len == 0) return 0;

    uint8_t b0 = s[0];

    // ASCII fast path
    if (b0 < 0x80u)
    {
        if (out_cp) *out_cp = (uint32_t)b0;
        return 1;
    }

    // 2-byte sequence: 110xxxxx 10xxxxxx
    if ((b0 & 0xE0u) == 0xC0u)
    {
        if (len < 2) return 0;
        uint8_t b1 = s[1];
        if (!steel_u8_is_cont(b1)) return 0;

        uint32_t cp = ((uint32_t)(b0 & 0x1Fu) << 6) |
                      ((uint32_t)(b1 & 0x3Fu));

        // Reject overlong: must be >= 0x80
        if (cp < 0x80u) return 0;
        if (!steel_u_is_scalar(cp)) return 0;

        if (out_cp) *out_cp = cp;
        return 2;
    }

    // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
    if ((b0 & 0xF0u) == 0xE0u)
    {
        if (len < 3) return 0;
        uint8_t b1 = s[1];
        uint8_t b2 = s[2];
        if (!steel_u8_is_cont(b1) || !steel_u8_is_cont(b2)) return 0;

        // Extra restriction to avoid overlong and surrogate ranges:
        //  - If b0 == 0xE0 then b1 must be >= 0xA0
        //  - If b0 == 0xED then b1 must be <= 0x9F (to exclude surrogates)
        if (b0 == 0xE0u && b1 < 0xA0u) return 0;
        if (b0 == 0xEDu && b1 > 0x9Fu) return 0;

        uint32_t cp = ((uint32_t)(b0 & 0x0Fu) << 12) |
                      ((uint32_t)(b1 & 0x3Fu) << 6) |
                      ((uint32_t)(b2 & 0x3Fu));

        // Reject overlong: must be >= 0x800
        if (cp < 0x800u) return 0;
        if (!steel_u_is_scalar(cp)) return 0;

        if (out_cp) *out_cp = cp;
        return 3;
    }

    // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    if ((b0 & 0xF8u) == 0xF0u)
    {
        if (len < 4) return 0;
        uint8_t b1 = s[1];
        uint8_t b2 = s[2];
        uint8_t b3 = s[3];
        if (!steel_u8_is_cont(b1) || !steel_u8_is_cont(b2) || !steel_u8_is_cont(b3)) return 0;

        // Restrict to valid range U+10000..U+10FFFF
        //  - If b0 == 0xF0 then b1 must be >= 0x90 (avoid overlong)
        //  - If b0 == 0xF4 then b1 must be <= 0x8F (cap at 0x10FFFF)
        if (b0 == 0xF0u && b1 < 0x90u) return 0;
        if (b0 == 0xF4u && b1 > 0x8Fu) return 0;
        // b0 beyond 0xF4 is invalid
        if (b0 > 0xF4u) return 0;

        uint32_t cp = ((uint32_t)(b0 & 0x07u) << 18) |
                      ((uint32_t)(b1 & 0x3Fu) << 12) |
                      ((uint32_t)(b2 & 0x3Fu) << 6) |
                      ((uint32_t)(b3 & 0x3Fu));

        if (cp < 0x10000u) return 0;
        if (!steel_u_is_scalar(cp)) return 0;

        if (out_cp) *out_cp = cp;
        return 4;
    }

    // Invalid leading byte
    return 0;
}

// Encode one codepoint to UTF-8. Returns bytes written (1..4), or 0 if invalid.
size_t steel_utf8_encode_one(uint32_t cp, uint8_t out[4])
{
    if (!out) return 0;

    if (!steel_u_is_scalar(cp)) return 0;

    if (cp <= 0x7Fu)
    {
        out[0] = (uint8_t)cp;
        return 1;
    }
    if (cp <= 0x7FFu)
    {
        out[0] = (uint8_t)(0xC0u | (cp >> 6));
        out[1] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 2;
    }
    if (cp <= 0xFFFFu)
    {
        out[0] = (uint8_t)(0xE0u | (cp >> 12));
        out[1] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 3;
    }

    out[0] = (uint8_t)(0xF0u | (cp >> 18));
    out[1] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
    out[2] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
    out[3] = (uint8_t)(0x80u | (cp & 0x3Fu));
    return 4;
}

// Returns true if [s, s+len) is valid UTF-8.
bool steel_utf8_validate(const uint8_t* s, size_t len)
{
    if (!s) return (len == 0);

    size_t i = 0;
    while (i < len)
    {
        // ASCII run fast path
        while (i < len && s[i] < 0x80u) i++;
        if (i >= len) break;

        uint32_t cp = 0;
        size_t n = steel_utf8_decode_one(s + i, len - i, &cp);
        if (n == 0) return false;
        i += n;
    }

    return true;
}

// Validate a NUL-terminated string.
bool steel_utf8_validate_cstr(const char* s)
{
    if (!s) return true;
    return steel_utf8_validate((const uint8_t*)s, strlen(s));
}

// Count codepoints in a UTF-8 buffer. If invalid sequences occur, it counts
// each invalid leading byte as one replacement character.
size_t steel_utf8_count(const uint8_t* s, size_t len)
{
    if (!s) return 0;

    size_t i = 0;
    size_t count = 0;

    while (i < len)
    {
        // ASCII run
        while (i < len && s[i] < 0x80u)
        {
            i++;
            count++;
        }
        if (i >= len) break;

        uint32_t cp = 0;
        size_t n = steel_utf8_decode_one(s + i, len - i, &cp);
        if (n == 0)
        {
            // Invalid leading byte
            i++;
            count++;
            continue;
        }
        i += n;
        count++;
    }

    return count;
}

// Advance `i` to the start of the next codepoint. Returns new index (<= len).
// If currently at an invalid byte, advances by 1.
size_t steel_utf8_next(const uint8_t* s, size_t len, size_t i)
{
    if (!s) return 0;
    if (i >= len) return len;

    uint8_t b0 = s[i];
    if (b0 < 0x80u) return i + 1;

    uint32_t cp;
    size_t n = steel_utf8_decode_one(s + i, len - i, &cp);
    if (n == 0) return i + 1;
    return i + n;
}

// Rewind `i` to the start of the previous codepoint (best-effort).
// If `i` is 0, returns 0.
// This walks backwards up to 3 continuation bytes.
size_t steel_utf8_prev(const uint8_t* s, size_t len, size_t i)
{
    (void)len;
    if (!s) return 0;
    if (i == 0) return 0;

    size_t j = i;
    if (j > 0) j--;

    // Move back over continuation bytes
    size_t steps = 0;
    while (j > 0 && steps < 3 && steel_u8_is_cont(s[j]))
    {
        j--;
        steps++;
    }

    // Now j might be a leading byte; validate forward from j to i.
    uint32_t cp;
    size_t n = steel_utf8_decode_one(s + j, i - j, &cp);
    if (n == 0 || (j + n) != i)
    {
        // Best effort fallback: return j+1 when we overshot
        return j;
    }

    return j;
}

// Decode one codepoint and advance index.
// Returns true on success; on failure, it advances by 1 and yields replacement.
bool steel_utf8_read(const uint8_t* s, size_t len, size_t* io_i, uint32_t* out_cp)
{
    if (out_cp) *out_cp = 0;
    if (!io_i) return false;

    size_t i = *io_i;
    if (!s || i >= len) return false;

    uint32_t cp = 0;
    size_t n = steel_utf8_decode_one(s + i, len - i, &cp);
    if (n == 0)
    {
        if (out_cp) *out_cp = STEEL_UTF8_REPLACEMENT;
        *io_i = i + 1;
        return false;
    }

    if (out_cp) *out_cp = cp;
    *io_i = i + n;
    return true;
}

// Write a codepoint into a buffer at index. Returns true on success.
bool steel_utf8_write(uint32_t cp, uint8_t* out, size_t out_cap, size_t* io_i)
{
    if (!out || !io_i) return false;

    uint8_t tmp[4];
    size_t n = steel_utf8_encode_one(cp, tmp);
    if (n == 0) return false;

    size_t i = *io_i;
    if (i + n > out_cap) return false;

    memcpy(out + i, tmp, n);
    *io_i = i + n;
    return true;
}

// Convert UTF-8 to a stream of scalar values. This function does not allocate.
// It decodes up to `out_cap` codepoints. Returns number of codepoints written.
// Invalid bytes become U+FFFD.
size_t steel_utf8_decode_all(const uint8_t* s, size_t len, uint32_t* out, size_t out_cap)
{
    if (!s || !out || out_cap == 0) return 0;

    size_t i = 0;
    size_t k = 0;
    while (i < len && k < out_cap)
    {
        uint32_t cp = 0;
        size_t n = steel_utf8_decode_one(s + i, len - i, &cp);
        if (n == 0)
        {
            out[k++] = STEEL_UTF8_REPLACEMENT;
            i++;
            continue;
        }
        out[k++] = cp;
        i += n;
    }

    return k;
}

// ASCII helpers

bool steel_utf8_is_ascii(const uint8_t* s, size_t len)
{
    if (!s) return (len == 0);
    for (size_t i = 0; i < len; i++)
        if (s[i] & 0x80u) return false;
    return true;
}

size_t steel_utf8_ascii_count_bytes_to_cp(const uint8_t* s, size_t len)
{
    // For ASCII, bytes == codepoints.
    (void)s;
    return len;
}
