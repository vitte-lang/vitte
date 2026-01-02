/*
  bench/detail/format.h

  Lightweight formatting helpers for the Vitte benchmark harness.

  Goals
  - Small, predictable surface for formatting bench reports and logs.
  - C17-compatible; safe wrappers around snprintf/vsnprintf.
  - Avoid pulling in heavy iostream or platform headers.

  This header pairs with:
    - bench/detail/compat.h (platform + attributes)
    - bench/detail/format.c (implementation)
*/

#pragma once
#ifndef VITTE_BENCH_DETAIL_FORMAT_H
#define VITTE_BENCH_DETAIL_FORMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "bench/detail/compat.h"

/* -------------------------------------------------------------------------- */
/* String view                                                                 */
/* -------------------------------------------------------------------------- */

typedef struct vitte_sv {
    const char* ptr;
    size_t      len;
} vitte_sv;

static inline vitte_sv vitte_sv_make(const char* p, size_t n)
{
    vitte_sv v;
    v.ptr = p;
    v.len = n;
    return v;
}

static inline vitte_sv vitte_sv_cstr(const char* s)
{
    vitte_sv v;
    v.ptr = s;
    v.len = 0;
    if (s) {
        while (s[v.len] != '\0') {
            v.len++;
        }
    }
    return v;
}

/* -------------------------------------------------------------------------- */
/* Append buffer                                                               */
/* -------------------------------------------------------------------------- */

typedef struct vitte_buf {
    char*  data;
    size_t cap;
    size_t len;
} vitte_buf;

static inline void vitte_buf_init(vitte_buf* b, char* mem, size_t cap)
{
    b->data = mem;
    b->cap  = cap;
    b->len  = 0;
    if (b->cap > 0) {
        b->data[0] = '\0';
    }
}

static inline size_t vitte_buf_avail(const vitte_buf* b)
{
    if (b->cap == 0) {
        return 0;
    }
    if (b->len >= b->cap) {
        return 0;
    }
    return (b->cap - b->len);
}

static inline bool vitte_buf_ok(const vitte_buf* b)
{
    return b && b->data && (b->len < b->cap || b->cap == 0);
}

/*
  Append raw bytes; always maintains NUL termination when cap > 0.
  Returns true if fully appended, false if truncated.
*/
bool vitte_buf_append(vitte_buf* b, const void* bytes, size_t n);

/*
  Append a string view.
*/
static inline bool vitte_buf_append_sv(vitte_buf* b, vitte_sv s)
{
    return vitte_buf_append(b, s.ptr, s.len);
}

/*
  Append a C string (null-terminated).
*/
bool vitte_buf_append_cstr(vitte_buf* b, const char* s);

/*
  Append a single char.
*/
bool vitte_buf_append_ch(vitte_buf* b, char ch);

/* -------------------------------------------------------------------------- */
/* printf-style appends                                                        */
/* -------------------------------------------------------------------------- */

/*
  Append formatted text.
  Returns true if fully appended, false if truncated.
*/
bool vitte_buf_appendf(vitte_buf* b, const char* fmt, ...) VITTE_PRINTF_FMT(2, 3);

bool vitte_buf_vappendf(vitte_buf* b, const char* fmt, va_list ap) VITTE_PRINTF_FMT(2, 0);

/* -------------------------------------------------------------------------- */
/* Numeric helpers                                                             */
/* -------------------------------------------------------------------------- */

/*
  Append unsigned integer as decimal.
*/
bool vitte_buf_append_u64(vitte_buf* b, uint64_t v);

/*
  Append signed integer as decimal.
*/
bool vitte_buf_append_i64(vitte_buf* b, int64_t v);

/*
  Append bytes as hex string (lowercase).
*/
bool vitte_buf_append_hex(vitte_buf* b, const void* bytes, size_t n);

/* -------------------------------------------------------------------------- */
/* JSON-safe helpers                                                           */
/* -------------------------------------------------------------------------- */

/*
  Append a JSON string literal (including quotes).

  Escapes:
    - backslash, quote
    - control chars (\n, \r, \t)
    - other < 0x20 as \u00XX

  NOTE: This does not validate UTF-8; it treats bytes as-is.
*/
bool vitte_buf_append_json_string(vitte_buf* b, const char* s);

/*
  Append a JSON key/value pair with string value: "key": "value"
  If `comma` is true, prefix with ",".
*/
bool vitte_buf_append_json_kv_str(vitte_buf* b, const char* key, const char* value, bool comma);

/*
  Append a JSON key/value pair with u64 value: "key": 123
  If `comma` is true, prefix with ",".
*/
bool vitte_buf_append_json_kv_u64(vitte_buf* b, const char* key, uint64_t value, bool comma);

/* -------------------------------------------------------------------------- */
/* Convenience: build small JSON objects                                       */
/* -------------------------------------------------------------------------- */

/*
  Start a JSON object: '{'
*/
bool vitte_buf_json_begin(vitte_buf* b);

/*
  End a JSON object: '}'
*/
bool vitte_buf_json_end(vitte_buf* b);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_DETAIL_FORMAT_H */
