

/*
  bench/json.h

  Minimal JSON emission helpers for the Vitte benchmark harness.

  Motivation
  - Reporters and diagnostics need to emit JSON deterministically.
  - We want a small, dependency-free layer over the existing vitte_buf formatter.

  Scope
  - Emission only (no parsing).
  - Safe string escaping.
  - Simple object/array helpers.

  Notes
  - This module assumes UTF-8 input; invalid sequences are passed through.
  - Escaping follows JSON rules for control characters and quotes/backslashes.
  - The implementation is header-only.
*/

#pragma once
#ifndef VITTE_BENCH_JSON_H
#define VITTE_BENCH_JSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/detail/format.h"

/* -------------------------------------------------------------------------- */
/* Escaping                                                                    */
/* -------------------------------------------------------------------------- */

/* Append a JSON-escaped string value WITHOUT surrounding quotes. */
static inline bool vitte_json_escape(vitte_buf* out, const char* s)
{
    if (!out) {
        return false;
    }
    if (!s) {
        return true;
    }

    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        const unsigned char c = *p;
        switch (c) {
            case '"': if (!vitte_buf_append(out, "\\\"", 2)) return false; break;
            case '\\': if (!vitte_buf_append(out, "\\\\", 2)) return false; break;
            case '\b': if (!vitte_buf_append(out, "\\b", 2)) return false; break;
            case '\f': if (!vitte_buf_append(out, "\\f", 2)) return false; break;
            case '\n': if (!vitte_buf_append(out, "\\n", 2)) return false; break;
            case '\r': if (!vitte_buf_append(out, "\\r", 2)) return false; break;
            case '\t': if (!vitte_buf_append(out, "\\t", 2)) return false; break;
            default:
                if (c < 0x20u) {
                    /* control -> \u00XX */
                    static const char hex[] = "0123456789abcdef";
                    char tmp[6];
                    tmp[0] = '\\'; tmp[1] = 'u'; tmp[2] = '0'; tmp[3] = '0';
                    tmp[4] = hex[(c >> 4u) & 0x0fu];
                    tmp[5] = hex[c & 0x0fu];
                    if (!vitte_buf_append(out, tmp, 6)) return false;
                } else {
                    if (!vitte_buf_append(out, (const char*)p, 1)) return false;
                }
                break;
        }
    }

    return true;
}

/* Append a full JSON string including surrounding quotes. */
static inline bool vitte_json_string(vitte_buf* out, const char* s)
{
    if (!out) {
        return false;
    }
    if (!vitte_buf_append(out, "\"", 1)) return false;
    if (!vitte_json_escape(out, s)) return false;
    if (!vitte_buf_append(out, "\"", 1)) return false;
    return true;
}

/* -------------------------------------------------------------------------- */
/* Structural helpers                                                          */
/* -------------------------------------------------------------------------- */

static inline bool vitte_json_begin_obj(vitte_buf* out) { return vitte_buf_append(out, "{", 1); }
static inline bool vitte_json_end_obj(vitte_buf* out)   { return vitte_buf_append(out, "}", 1); }
static inline bool vitte_json_begin_arr(vitte_buf* out) { return vitte_buf_append(out, "[", 1); }
static inline bool vitte_json_end_arr(vitte_buf* out)   { return vitte_buf_append(out, "]", 1); }
static inline bool vitte_json_comma(vitte_buf* out)     { return vitte_buf_append(out, ",", 1); }
static inline bool vitte_json_colon(vitte_buf* out)     { return vitte_buf_append(out, ":", 1); }

/* Append a JSON key (quoted) and ':' */
static inline bool vitte_json_key(vitte_buf* out, const char* key)
{
    if (!vitte_json_string(out, key)) return false;
    return vitte_json_colon(out);
}

/* -------------------------------------------------------------------------- */
/* Key-value convenience                                                        */
/* -------------------------------------------------------------------------- */

/*
  Append key/value pair.
  - If `with_comma` true, prepend a comma.
*/
static inline bool vitte_json_kv_str(vitte_buf* out, const char* key, const char* val, bool with_comma)
{
    if (!out || !key) return false;
    if (with_comma) { if (!vitte_json_comma(out)) return false; }
    if (!vitte_json_key(out, key)) return false;
    if (val) return vitte_json_string(out, val);
    return vitte_buf_append(out, "null", 4);
}

static inline bool vitte_json_kv_bool(vitte_buf* out, const char* key, bool val, bool with_comma)
{
    if (!out || !key) return false;
    if (with_comma) { if (!vitte_json_comma(out)) return false; }
    if (!vitte_json_key(out, key)) return false;
    return vitte_buf_append(out, val ? "true" : "false", val ? 4 : 5);
}

static inline bool vitte_json_kv_u64(vitte_buf* out, const char* key, uint64_t val, bool with_comma)
{
    if (!out || !key) return false;
    if (with_comma) { if (!vitte_json_comma(out)) return false; }
    if (!vitte_json_key(out, key)) return false;
    return vitte_buf_appendf(out, "%llu", (unsigned long long)val);
}

static inline bool vitte_json_kv_i64(vitte_buf* out, const char* key, int64_t val, bool with_comma)
{
    if (!out || !key) return false;
    if (with_comma) { if (!vitte_json_comma(out)) return false; }
    if (!vitte_json_key(out, key)) return false;
    return vitte_buf_appendf(out, "%lld", (long long)val);
}

static inline bool vitte_json_kv_f64(vitte_buf* out, const char* key, double val, bool with_comma)
{
    if (!out || !key) return false;
    if (with_comma) { if (!vitte_json_comma(out)) return false; }
    if (!vitte_json_key(out, key)) return false;
    /* Deterministic enough for bench; reporters may override formatting. */
    return vitte_buf_appendf(out, "%.17g", val);
}

/* -------------------------------------------------------------------------- */
/* Pretty helpers (optional)                                                   */
/* -------------------------------------------------------------------------- */

/* Append '\n' */
static inline bool vitte_json_nl(vitte_buf* out) { return vitte_buf_append(out, "\n", 1); }

/* Append N spaces */
static inline bool vitte_json_indent(vitte_buf* out, uint32_t n)
{
    if (!out) return false;
    while (n--) {
        if (!vitte_buf_append(out, " ", 1)) return false;
    }
    return true;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_JSON_H */