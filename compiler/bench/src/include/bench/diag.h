/*
  bench/diag.h

  Diagnostics and error propagation for the Vitte benchmark harness.

  Goals
  - Provide a small, deterministic diagnostic object.
  - Enable structured error reporting without requiring stdio.
  - Support both "status-return" and "set diag + return false" styles.

  Principles
  - No dynamic allocation.
  - Messages are capped and always NUL-terminated.
  - Caller owns the diag object lifetime.

  Integration
  - Can be embedded in larger result objects.
  - Works with bench/detail/format.h (vitte_buf) for formatting.
*/

#pragma once
#ifndef VITTE_BENCH_DIAG_H
#define VITTE_BENCH_DIAG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/detail/format.h"

/* -------------------------------------------------------------------------- */
/* Error codes                                                                 */
/* -------------------------------------------------------------------------- */

typedef enum vitte_diag_code {
    VITTE_DIAG_OK = 0,

    /* generic */
    VITTE_DIAG_ERR_UNKNOWN = 1,
    VITTE_DIAG_ERR_OOM = 2,
    VITTE_DIAG_ERR_INVALID_ARG = 3,
    VITTE_DIAG_ERR_IO = 4,
    VITTE_DIAG_ERR_PARSE = 5,
    VITTE_DIAG_ERR_NOT_FOUND = 6,
    VITTE_DIAG_ERR_UNSUPPORTED = 7,
    VITTE_DIAG_ERR_TIMEOUT = 8,
    VITTE_DIAG_ERR_INTERNAL = 9,

    /* bench-specific */
    VITTE_DIAG_ERR_BENCH_FAILED = 100,
    VITTE_DIAG_ERR_BASELINE_FAIL = 101,
    VITTE_DIAG_ERR_BASELINE_MISSING = 102
} vitte_diag_code;

/* -------------------------------------------------------------------------- */
/* Diagnostic object                                                          */
/* -------------------------------------------------------------------------- */

#ifndef VITTE_DIAG_MSG_CAP
  #define VITTE_DIAG_MSG_CAP 256u
#endif

typedef struct vitte_diag {
    vitte_diag_code code;
    uint32_t flags;

    /* optional numeric payload */
    int64_t i64;
    uint64_t u64;

    /* location */
    const char* file;
    uint32_t line;

    char msg[VITTE_DIAG_MSG_CAP];
} vitte_diag;

/* flags */
#define VITTE_DIAG_F_NONE        0u
#define VITTE_DIAG_F_TRANSIENT   (1u << 0) /* likely intermittent */
#define VITTE_DIAG_F_PERM        (1u << 1) /* persistent (config/code) */
#define VITTE_DIAG_F_IO          (1u << 2)
#define VITTE_DIAG_F_PARSE       (1u << 3)
#define VITTE_DIAG_F_SYS         (1u << 4)

/* -------------------------------------------------------------------------- */
/* Init / reset                                                                */
/* -------------------------------------------------------------------------- */

static inline void vitte_diag_reset(vitte_diag* d)
{
    if (!d) {
        return;
    }
    d->code = VITTE_DIAG_OK;
    d->flags = 0;
    d->i64 = 0;
    d->u64 = 0;
    d->file = NULL;
    d->line = 0;
    d->msg[0] = '\0';
}

static inline bool vitte_diag_ok(const vitte_diag* d)
{
    return !d || d->code == VITTE_DIAG_OK;
}

/* -------------------------------------------------------------------------- */
/* Safe message copy                                                           */
/* -------------------------------------------------------------------------- */

static inline void vitte_diag__cpy(char* dst, size_t cap, const char* src)
{
    if (!dst || cap == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t i = 0;
    for (; i + 1 < cap && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/* -------------------------------------------------------------------------- */
/* Set / format                                                                */
/* -------------------------------------------------------------------------- */

static inline void vitte_diag_set(vitte_diag* d, vitte_diag_code code, uint32_t flags, const char* msg)
{
    if (!d) {
        return;
    }
    d->code = code;
    d->flags = flags;
    d->file = NULL;
    d->line = 0;
    vitte_diag__cpy(d->msg, sizeof(d->msg), msg);
}

static inline void vitte_diag_set_loc(vitte_diag* d, vitte_diag_code code, uint32_t flags, const char* file, uint32_t line, const char* msg)
{
    if (!d) {
        return;
    }
    d->code = code;
    d->flags = flags;
    d->file = file;
    d->line = line;
    vitte_diag__cpy(d->msg, sizeof(d->msg), msg);
}

/* printf-style formatting into diag msg (uses vitte_bench_snprintf). */
static inline void vitte_diag_setf(vitte_diag* d, vitte_diag_code code, uint32_t flags, const char* fmt, ...)
{
    if (!d) {
        return;
    }

    d->code = code;
    d->flags = flags;
    d->file = NULL;
    d->line = 0;

    va_list ap;
    va_start(ap, fmt);
    (void)vitte_bench_vsnprintf(d->msg, sizeof(d->msg), fmt, ap);
    va_end(ap);
}

static inline void vitte_diag_setf_loc(vitte_diag* d, vitte_diag_code code, uint32_t flags, const char* file, uint32_t line, const char* fmt, ...)
{
    if (!d) {
        return;
    }

    d->code = code;
    d->flags = flags;
    d->file = file;
    d->line = line;

    va_list ap;
    va_start(ap, fmt);
    (void)vitte_bench_vsnprintf(d->msg, sizeof(d->msg), fmt, ap);
    va_end(ap);
}

/* Attach numeric payloads. */
static inline void vitte_diag_set_i64(vitte_diag* d, int64_t v) { if (d) d->i64 = v; }
static inline void vitte_diag_set_u64(vitte_diag* d, uint64_t v) { if (d) d->u64 = v; }

/* -------------------------------------------------------------------------- */
/* Macros for call-site convenience                                            */
/* -------------------------------------------------------------------------- */

#define VITTE_DIAG_SET(d, code, flags, msg) \
    vitte_diag_set_loc((d), (code), (flags), __FILE__, (uint32_t)__LINE__, (msg))

#define VITTE_DIAG_SETF(d, code, flags, fmt, ...) \
    vitte_diag_setf_loc((d), (code), (flags), __FILE__, (uint32_t)__LINE__, (fmt), __VA_ARGS__)

#define VITTE_DIAG_RETURN_FALSE(d, code, flags, msg) \
    do { VITTE_DIAG_SET((d), (code), (flags), (msg)); return false; } while (0)

#define VITTE_DIAG_RETURNF_FALSE(d, code, flags, fmt, ...) \
    do { VITTE_DIAG_SETF((d), (code), (flags), (fmt), __VA_ARGS__); return false; } while (0)

#define VITTE_DIAG_RETURN_NULL(d, code, flags, msg) \
    do { VITTE_DIAG_SET((d), (code), (flags), (msg)); return NULL; } while (0)

/* -------------------------------------------------------------------------- */
/* Formatting                                                                  */
/* -------------------------------------------------------------------------- */

static inline const char* vitte_diag_code_name(vitte_diag_code c)
{
    switch (c) {
        case VITTE_DIAG_OK: return "ok";
        case VITTE_DIAG_ERR_UNKNOWN: return "unknown";
        case VITTE_DIAG_ERR_OOM: return "oom";
        case VITTE_DIAG_ERR_INVALID_ARG: return "invalid_arg";
        case VITTE_DIAG_ERR_IO: return "io";
        case VITTE_DIAG_ERR_PARSE: return "parse";
        case VITTE_DIAG_ERR_NOT_FOUND: return "not_found";
        case VITTE_DIAG_ERR_UNSUPPORTED: return "unsupported";
        case VITTE_DIAG_ERR_TIMEOUT: return "timeout";
        case VITTE_DIAG_ERR_INTERNAL: return "internal";
        case VITTE_DIAG_ERR_BENCH_FAILED: return "bench_failed";
        case VITTE_DIAG_ERR_BASELINE_FAIL: return "baseline_fail";
        case VITTE_DIAG_ERR_BASELINE_MISSING: return "baseline_missing";
        default: return "unknown";
    }
}

/* Append a human readable line: "code: msg (file:line)" */
static inline bool vitte_diag_format(vitte_buf* out, const vitte_diag* d)
{
    if (!out || !d) {
        return false;
    }
    if (d->file && d->line) {
        return vitte_buf_appendf(out, "%s: %s (%s:%u)", vitte_diag_code_name(d->code), d->msg, d->file, (unsigned)d->line);
    }
    return vitte_buf_appendf(out, "%s: %s", vitte_diag_code_name(d->code), d->msg);
}

/* Append JSON object {code,msg,file,line,i64,u64,flags}. */
static inline bool vitte_diag_format_json(vitte_buf* out, const vitte_diag* d)
{
    if (!out || !d) {
        return false;
    }

    if (!vitte_buf_json_begin(out)) return false;

    if (!vitte_buf_append_json_kv_str(out, "code", vitte_diag_code_name(d->code), false)) return false;
    if (!vitte_buf_append_json_kv_str(out, "msg", d->msg, true)) return false;

    if (d->file) {
        if (!vitte_buf_append_json_kv_str(out, "file", d->file, true)) return false;
    } else {
        if (!vitte_buf_append(out, ",\"file\":null", 12)) return false;
    }

    if (!vitte_buf_append(out, ",\"line\":", 8)) return false;
    if (!vitte_buf_appendf(out, "%u", (unsigned)d->line)) return false;

    if (!vitte_buf_append(out, ",\"flags\":", 9)) return false;
    if (!vitte_buf_appendf(out, "%u", (unsigned)d->flags)) return false;

    if (!vitte_buf_append(out, ",\"i64\":", 7)) return false;
    if (!vitte_buf_appendf(out, "%lld", (long long)d->i64)) return false;

    if (!vitte_buf_append(out, ",\"u64\":", 7)) return false;
    if (!vitte_buf_appendf(out, "%llu", (unsigned long long)d->u64)) return false;

    if (!vitte_buf_json_end(out)) return false;

    return true;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_DIAG_H */
