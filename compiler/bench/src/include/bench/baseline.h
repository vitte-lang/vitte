#ifndef BENCH_BASELINE_H
#define BENCH_BASELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

/* Parsing (format v1)                                                         */

static inline void vitte_baseline__skip_ws(const char** pp)
{
    const char* p = *pp;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    *pp = p;
}

static inline bool vitte_baseline__read_u64_tok(const char** pp, uint64_t* out)
{
    const char* p = *pp;
    vitte_baseline__skip_ws(&p);

    if (*p == '\0' || *p == '\n' || *p == '\r') {
        return false;
    }

    uint64_t v = 0;
    const char* q = p;
    while (*q >= '0' && *q <= '9') {
        uint64_t d = (uint64_t)(*q - '0');
        if (v > (UINT64_MAX - d) / 10u) {
            return false;
        }
        v = v * 10u + d;
        q++;
    }

    if (q == p) {
        return false;
    }

    /* must end token */
    if (*q != '\0' && *q != ' ' && *q != '\t' && *q != '\r' && *q != '\n') {
        return false;
    }

    *pp = q;
    if (out) {
        *out = v;
    }
    return true;
}

static inline bool vitte_baseline__read_f64_tok_simple(const char** pp, double* out)
{
    const char* p = *pp;
    vitte_baseline__skip_ws(&p);

    if (*p == '\0' || *p == '\n' || *p == '\r') {
        return false;
    }

    bool neg = false;
    if (*p == '-') {
        neg = true;
        p++;
    }

    uint64_t ip = 0;
    bool any_int = false;
    while (*p >= '0' && *p <= '9') {
        any_int = true;
        uint64_t d = (uint64_t)(*p - '0');
        if (ip > (UINT64_MAX - d) / 10u) {
            return false;
        }
        ip = ip * 10u + d;
        p++;
    }

    double v = (double)ip;

    if (*p == '.') {
        p++;
        double scale = 1.0;
        bool any_frac = false;
        while (*p >= '0' && *p <= '9') {
            any_frac = true;
            uint64_t d = (uint64_t)(*p - '0');
            scale *= 0.1;
            v += (double)d * scale;
            p++;
        }
        if (!any_int && !any_frac) {
            return false;
        }
    } else {
        if (!any_int) {
            return false;
        }
    }

    if (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        return false;
    }

    *pp = p;
    if (neg) {
        v = -v;
    }
    if (out) {
        *out = v;
    }
    return true;
}

static inline bool vitte_baseline_parse_line(vitte_baseline* b, const char* line)
{
    if (!line) {
        return false;
    }

    const char* p = line;
    uint64_t p50 = 0, p90 = 0, p99 = 0;
    double mean = 0.0;

    /* helper: read next u64 */
    /* helper: read next f64 */

    if (!vitte_baseline__read_u64_tok(&p, &p50)) return false;
    if (!vitte_baseline__read_u64_tok(&p, &p90)) return false;
    if (!vitte_baseline__read_u64_tok(&p, &p99)) return false;
    if (!vitte_baseline__read_f64_tok_simple(&p, &mean)) return false;

    b->p50 = p50;
    b->p90 = p90;
    b->p99 = p99;
    b->mean = mean;

    return true;
}

#endif /* BENCH_BASELINE_H */

/*
  bench/baseline.h

  Baseline management for Vitte benchmarks.

  Problem
  - Bench results drift due to:
      - compiler/toolchain changes
      - OS scheduler / CPU frequency behavior
      - micro-architectural effects
      - host contention
  - To detect regressions, CI often compares current results against a stored baseline.

  This module provides:
  - A compact in-memory representation of baseline entries.
  - Helpers to load/save a simple line-based format.
  - Comparison utilities with configurable thresholds.

  Intended usage
  - Runner executes suite -> produces summary stats.
  - Baseline loader reads previous "golden" values.
  - Comparator decides pass/fail, and emits a report.

  Format (v1)
  - UTF-8 text
  - One entry per line:
      <key> <p50_ns> <p90_ns> <p99_ns> <mean_ns>
  - `key` must not contain spaces.

  Notes
  - I/O is abstracted via callbacks to avoid tying to stdio.
  - A reference file adapter can be implemented in bench/detail/io.* (optional).

  Pairing
  - This header is header-only (no .c required).
  - Depends on bench/detail/format.h and bench/detail/compat.h.
*/

#pragma once
#ifndef VITTE_BENCH_BASELINE_H
#define VITTE_BENCH_BASELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/detail/compat.h"
#include "bench/detail/format.h"
#include "bench/assert.h"
#include "bench/arena.h"

/* -------------------------------------------------------------------------- */
/* Types                                                                       */
/* -------------------------------------------------------------------------- */

typedef struct vitte_baseline_entry {
    const char* key;     /* stable benchmark id (suite.case.variant) */

    uint64_t p50_ns;
    uint64_t p90_ns;
    uint64_t p99_ns;
    double   mean_ns;

    /* Optional metadata */
    const char* unit;    /* "ns" by default */
} vitte_baseline_entry;

typedef struct vitte_baseline {
    vitte_baseline_entry* entries;
    size_t count;
    size_t cap;

    /* storage for keys (optional) */
    vitte_arena* arena;
} vitte_baseline;

typedef struct vitte_baseline_threshold {
    /*
      Relative threshold: allowed slowdown ratio.
      Example: 0.05 means +5% allowed.
    */
    double rel;

    /*
      Absolute threshold in nanoseconds (useful for tiny benches).
      Example: 50 means allow +50ns.
    */
    uint64_t abs_ns;
} vitte_baseline_threshold;

typedef enum vitte_baseline_metric {
    VITTE_BASELINE_P50 = 0,
    VITTE_BASELINE_P90 = 1,
    VITTE_BASELINE_P99 = 2,
    VITTE_BASELINE_MEAN = 3
} vitte_baseline_metric;

typedef enum vitte_baseline_cmp {
    VITTE_BASELINE_EQUAL = 0,
    VITTE_BASELINE_FASTER = 1,
    VITTE_BASELINE_SLOWER = 2,
    VITTE_BASELINE_MISSING = 3
} vitte_baseline_cmp;

typedef struct vitte_baseline_result {
    vitte_baseline_cmp cmp;
    vitte_baseline_metric metric;

    uint64_t base_ns;
    uint64_t curr_ns;

    double   ratio;      /* curr/base (1.0 == equal) */
    uint64_t delta_ns;   /* curr - base */

    bool     pass;
} vitte_baseline_result;

/* -------------------------------------------------------------------------- */
/* Init                                                                        */
/* -------------------------------------------------------------------------- */

static inline void vitte_baseline_init(vitte_baseline* b, vitte_baseline_entry* entries, size_t cap, vitte_arena* arena)
{
    b->entries = entries;
    b->count = 0;
    b->cap = cap;
    b->arena = arena;
}

/* -------------------------------------------------------------------------- */
/* Utilities                                                                   */
/* -------------------------------------------------------------------------- */

static inline uint64_t vitte_baseline__metric_ns(const vitte_baseline_entry* e, vitte_baseline_metric m)
{
    switch (m) {
        case VITTE_BASELINE_P50:  return e->p50_ns;
        case VITTE_BASELINE_P90:  return e->p90_ns;
        case VITTE_BASELINE_P99:  return e->p99_ns;
        case VITTE_BASELINE_MEAN: return (uint64_t)(e->mean_ns + 0.5);
        default: return e->p50_ns;
    }
}

static inline const vitte_baseline_entry* vitte_baseline_find(const vitte_baseline* b, const char* key)
{
    if (!b || !key) {
        return NULL;
    }
    for (size_t i = 0; i < b->count; i++) {
        const vitte_baseline_entry* e = &b->entries[i];
        /* strcmp minimal */
        const char* a = e->key;
        const char* c = key;
        if (!a || !c) {
            continue;
        }
        while (*a && *c && *a == *c) {
            a++; c++;
        }
        if (*a == '\0' && *c == '\0') {
            return e;
        }
    }
    return NULL;
}

static inline bool vitte_baseline_push(vitte_baseline* b, vitte_baseline_entry e)
{
    if (!b || b->count >= b->cap) {
        return false;
    }
    b->entries[b->count++] = e;
    return true;
}

static inline const char* vitte_baseline__arena_strdup(vitte_arena* a, const char* s)
{
    if (!a || !s) {
        return s;
    }
    size_t n = 0;
    while (s[n] != '\0') n++;
    char* p = (char*)vitte_arena_alloc_aligned(a, n + 1, 1);
    if (!p) {
        return NULL;
    }
    for (size_t i = 0; i < n; i++) {
        p[i] = s[i];
    }
    p[n] = '\0';
    return p;
}

/* -------------------------------------------------------------------------- */
/* Parsing (format v1)                                                         */
/* -------------------------------------------------------------------------- */

/*
  Parse a single baseline line:
    <key> <p50_ns> <p90_ns> <p99_ns> <mean_ns>

  Returns true on success.
*/
static inline bool vitte_baseline_parse_line(vitte_baseline* b, const char* line)
{
    if (!b || !line) {
        return false;
    }

    /* tokenization on spaces/tabs */
    const char* p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0' || *p == '\n' || *p == '#') {
        return true; /* ignore empty/comments */
    }

    /* read key */
    const char* key0 = p;
    size_t key_len = 0;
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        key_len++; p++;
    }
    if (key_len == 0) {
        return false;
    }

    char key_tmp[256];
    if (key_len >= sizeof(key_tmp)) {
        return false;
    }
    for (size_t i = 0; i < key_len; i++) {
        key_tmp[i] = key0[i];
    }
    key_tmp[key_len] = '\0';

    /* helper: read next u64 */
    auto read_u64 = [&](uint64_t* out) -> bool {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '\n' || *p == '\r') return false;
        uint64_t v = 0;
        const char* q = p;
        while (*q >= '0' && *q <= '9') {
            uint64_t d = (uint64_t)(*q - '0');
            if (v > (UINT64_MAX - d) / 10u) return false;
            v = v * 10u + d;
            q++;
        }
        if (q == p) return false;
        /* must end token */
        if (*q != '\0' && *q != ' ' && *q != '\t' && *q != '\r' && *q != '\n') return false;
        p = q;
        *out = v;
        return true;
    };

    /* helper: read next double (simple: digits[.digits]) */
    auto read_f64 = [&](double* out) -> bool {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '\n' || *p == '\r') return false;
        bool neg = false;
        if (*p == '-') { neg = true; p++; }
        uint64_t ip = 0;
        bool any = false;
        while (*p >= '0' && *p <= '9') {
            any = true;
            uint64_t d = (uint64_t)(*p - '0');
            if (ip > (UINT64_MAX - d) / 10u) return false;
            ip = ip * 10u + d;
            p++;
        }
        double v = (double)ip;
        if (*p == '.') {
            p++;
            double scale = 1.0;
            bool anyf = false;
            while (*p >= '0' && *p <= '9') {
                anyf = true;
                uint64_t d = (uint64_t)(*p - '0');
                scale *= 0.1;
                v += (double)d * scale;
                p++;
            }
            if (!any && !anyf) return false;
        } else {
            if (!any) return false;
        }
        if (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') return false;
        if (neg) v = -v;
        *out = v;
        return true;
    };

    uint64_t p50 = 0, p90 = 0, p99 = 0;
    double mean = 0.0;

    if (!read_u64(&p50)) return false;
    if (!read_u64(&p90)) return false;
    if (!read_u64(&p99)) return false;
    if (!read_f64(&mean)) return false;

    vitte_baseline_entry e;
    e.key = vitte_baseline__arena_strdup(b->arena, key_tmp);
    e.p50_ns = p50;
    e.p90_ns = p90;
    e.p99_ns = p99;
    e.mean_ns = mean;
    e.unit = "ns";

    return vitte_baseline_push(b, e);
}

/* -------------------------------------------------------------------------- */
/* I/O adapters                                                                 */
/* -------------------------------------------------------------------------- */

/* Read callback: return number of bytes read into buf (0 = EOF, <0 = error). */
typedef int (*vitte_baseline_read_fn)(void* user, void* buf, size_t cap);

/* Write callback: return number of bytes written (<=0 = error). */
typedef int (*vitte_baseline_write_fn)(void* user, const void* buf, size_t n);

/*
  Load baseline from a stream using read callback.

  - Reads UTF-8 text.
  - Splits on '\n'.
  - Calls vitte_baseline_parse_line per line.

  Returns true on success.
*/
static inline bool vitte_baseline_load(vitte_baseline* b, vitte_baseline_read_fn rd, void* user)
{
    if (!b || !rd) {
        return false;
    }

    char chunk[4096];
    char line[4096];
    size_t line_len = 0;

    for (;;) {
        int r = rd(user, chunk, sizeof(chunk));
        if (r < 0) {
            return false;
        }
        if (r == 0) {
            break;
        }

        for (int i = 0; i < r; i++) {
            char c = chunk[i];
            if (c == '\n') {
                line[line_len] = '\0';
                if (!vitte_baseline_parse_line(b, line)) {
                    return false;
                }
                line_len = 0;
                continue;
            }

            if (c == '\r') {
                continue;
            }

            if (line_len + 1 >= sizeof(line)) {
                return false;
            }
            line[line_len++] = c;
        }
    }

    /* last line */
    if (line_len > 0) {
        line[line_len] = '\0';
        if (!vitte_baseline_parse_line(b, line)) {
            return false;
        }
    }

    return true;
}

/*
  Save baseline to a stream.
  Writes format v1.
*/
static inline bool vitte_baseline_save(const vitte_baseline* b, vitte_baseline_write_fn wr, void* user)
{
    if (!b || !wr) {
        return false;
    }

    char buf[512];

    for (size_t i = 0; i < b->count; i++) {
        const vitte_baseline_entry* e = &b->entries[i];
        size_t n = (size_t)vitte_bench_snprintf(
            buf,
            sizeof(buf),
            "%s %llu %llu %llu %.6f\n",
            e->key ? e->key : "",
            (unsigned long long)e->p50_ns,
            (unsigned long long)e->p90_ns,
            (unsigned long long)e->p99_ns,
            e->mean_ns
        );
        if (n == 0 || n >= sizeof(buf)) {
            return false;
        }
        if (wr(user, buf, n) <= 0) {
            return false;
        }
    }

    return true;
}

/* -------------------------------------------------------------------------- */
/* Comparison                                                                  */
/* -------------------------------------------------------------------------- */

/*
  Compare current metric against baseline with thresholds.

  pass condition:
    curr <= base + max(abs_ns, base*rel)
*/
static inline vitte_baseline_result vitte_baseline_compare(
    const vitte_baseline_entry* base,
    uint64_t curr_ns,
    vitte_baseline_metric metric,
    vitte_baseline_threshold th
)
{
    vitte_baseline_result r;
    r.metric = metric;

    if (!base) {
        r.cmp = VITTE_BASELINE_MISSING;
        r.base_ns = 0;
        r.curr_ns = curr_ns;
        r.ratio = 0.0;
        r.delta_ns = 0;
        r.pass = false;
        return r;
    }

    uint64_t base_ns = vitte_baseline__metric_ns(base, metric);
    r.base_ns = base_ns;
    r.curr_ns = curr_ns;

    if (base_ns == 0) {
        r.ratio = 0.0;
    } else {
        r.ratio = (double)curr_ns / (double)base_ns;
    }

    if (curr_ns >= base_ns) {
        r.delta_ns = (curr_ns - base_ns);
        r.cmp = (curr_ns == base_ns) ? VITTE_BASELINE_EQUAL : VITTE_BASELINE_SLOWER;
    } else {
        r.delta_ns = (base_ns - curr_ns);
        r.cmp = VITTE_BASELINE_FASTER;
    }

    double rel_allow = (double)base_ns * th.rel;
    uint64_t rel_allow_ns = (uint64_t)(rel_allow + 0.5);
    uint64_t allow = (th.abs_ns > rel_allow_ns) ? th.abs_ns : rel_allow_ns;

    /* pass if not slower beyond allow */
    if (curr_ns <= base_ns + allow) {
        r.pass = true;
    } else {
        r.pass = false;
    }

    return r;
}

/* -------------------------------------------------------------------------- */
/* Reporting helpers                                                           */
/* -------------------------------------------------------------------------- */

static inline const char* vitte_baseline_metric_name(vitte_baseline_metric m)
{
    switch (m) {
        case VITTE_BASELINE_P50: return "p50";
        case VITTE_BASELINE_P90: return "p90";
        case VITTE_BASELINE_P99: return "p99";
        case VITTE_BASELINE_MEAN: return "mean";
        default: return "p50";
    }
}

static inline const char* vitte_baseline_cmp_name(vitte_baseline_cmp c)
{
    switch (c) {
        case VITTE_BASELINE_EQUAL: return "equal";
        case VITTE_BASELINE_FASTER: return "faster";
        case VITTE_BASELINE_SLOWER: return "slower";
        case VITTE_BASELINE_MISSING: return "missing";
        default: return "equal";
    }
}

/*
  Append a human-readable comparison line into `out`.

  Example:
    key p50: base=123ns curr=130ns (+5.7%) PASS
*/
static inline bool vitte_baseline_format_line(
    vitte_buf* out,
    const char* key,
    vitte_baseline_result r
)
{
    if (!out) {
        return false;
    }

    char bdur[64];
    char cdur[64];

    (void)vitte_bench_snprintf(bdur, sizeof(bdur), "%lluns", (unsigned long long)r.base_ns);
    (void)vitte_bench_snprintf(cdur, sizeof(cdur), "%lluns", (unsigned long long)r.curr_ns);

    const double pct = (r.base_ns == 0) ? 0.0 : ((double)r.curr_ns * 100.0 / (double)r.base_ns - 100.0);

    return vitte_buf_appendf(
        out,
        "%s %s: base=%s curr=%s (%+.2f%%) %s\n",
        key ? key : "",
        vitte_baseline_metric_name(r.metric),
        bdur,
        cdur,
        pct,
        r.pass ? "PASS" : "FAIL"
    );
}

/*
  Append a JSON object describing a baseline comparison.

  Schema:
    {
      "key": "...",
      "metric": "p50",
      "cmp": "slower",
      "base_ns": 123,
      "curr_ns": 130,
      "ratio": 1.0569,
      "pass": true
    }
*/
static inline bool vitte_baseline_format_json(
    vitte_buf* out,
    const char* key,
    vitte_baseline_result r
)
{
    if (!out) {
        return false;
    }

    if (!vitte_buf_json_begin(out)) return false;

    if (!vitte_buf_append_json_kv_str(out, "key", key ? key : "", false)) return false;
    if (!vitte_buf_append_json_kv_str(out, "metric", vitte_baseline_metric_name(r.metric), true)) return false;
    if (!vitte_buf_append_json_kv_str(out, "cmp", vitte_baseline_cmp_name(r.cmp), true)) return false;
    if (!vitte_buf_append_json_kv_u64(out, "base_ns", r.base_ns, true)) return false;
    if (!vitte_buf_append_json_kv_u64(out, "curr_ns", r.curr_ns, true)) return false;

    /* ratio as number */
    {
        char tmp[64];
        (void)vitte_bench_snprintf(tmp, sizeof(tmp), "%.8f", r.ratio);
        if (!vitte_buf_append(out, ",\"ratio\":", 9)) return false;
        if (!vitte_buf_append_cstr(out, tmp)) return false;
    }

    /* pass as bool */
    if (!vitte_buf_append(out, ",\"pass\":", 8)) return false;
    if (!vitte_buf_append_cstr(out, r.pass ? "true" : "false")) return false;

    if (!vitte_buf_json_end(out)) return false;

    return true;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_BASELINE_H */
/*
  bench/baseline.h

  Baseline management for Vitte benchmarks.

  Problem
  - Bench results drift due to:
      - compiler/toolchain changes
      - OS scheduler / CPU frequency behavior
      - micro-architectural effects
      - host contention
  - To detect regressions, CI compares current results against a stored baseline.

  This module provides:
  - Compact in-memory baseline entries
  - Loader/saver for a simple line-based format
  - Comparator with relative/absolute thresholds
  - Text and JSON formatting helpers

  Format (v1)
  - UTF-8 text
  - One entry per line:
      <key> <p50_ns> <p90_ns> <p99_ns> <mean_ns>
  - `key` must not contain spaces.
  - Lines starting with '#' are comments.

  I/O
  - Abstracted via read/write callbacks (no stdio dependency).

  Dependencies
  - bench/detail/compat.h, bench/detail/format.h, bench/arena.h
*/

#pragma once
#ifndef VITTE_BENCH_BASELINE_H
#define VITTE_BENCH_BASELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/detail/compat.h"
#include "bench/detail/format.h"
#include "bench/arena.h"

/* -------------------------------------------------------------------------- */
/* Types                                                                       */
/* -------------------------------------------------------------------------- */

typedef struct vitte_baseline_entry {
    const char* key;     /* stable benchmark id (suite.case.variant) */

    uint64_t p50_ns;
    uint64_t p90_ns;
    uint64_t p99_ns;
    double   mean_ns;

    /* Optional metadata */
    const char* unit;    /* "ns" by default */
} vitte_baseline_entry;

typedef struct vitte_baseline {
    vitte_baseline_entry* entries;
    size_t count;
    size_t cap;

    /* storage for keys (optional) */
    vitte_arena* arena;
} vitte_baseline;

typedef struct vitte_baseline_threshold {
    /* Relative threshold: allowed slowdown ratio. Example: 0.05 means +5% allowed. */
    double rel;

    /* Absolute threshold in nanoseconds (useful for tiny benches). */
    uint64_t abs_ns;
} vitte_baseline_threshold;

typedef enum vitte_baseline_metric {
    VITTE_BASELINE_P50 = 0,
    VITTE_BASELINE_P90 = 1,
    VITTE_BASELINE_P99 = 2,
    VITTE_BASELINE_MEAN = 3
} vitte_baseline_metric;

typedef enum vitte_baseline_cmp {
    VITTE_BASELINE_EQUAL = 0,
    VITTE_BASELINE_FASTER = 1,
    VITTE_BASELINE_SLOWER = 2,
    VITTE_BASELINE_MISSING = 3
} vitte_baseline_cmp;

typedef struct vitte_baseline_result {
    vitte_baseline_cmp cmp;
    vitte_baseline_metric metric;

    uint64_t base_ns;
    uint64_t curr_ns;

    double   ratio;      /* curr/base (1.0 == equal) */
    uint64_t delta_ns;   /* curr - base (absolute value if faster) */

    bool     pass;
} vitte_baseline_result;

/* -------------------------------------------------------------------------- */
/* Init                                                                        */
/* -------------------------------------------------------------------------- */

static inline void vitte_baseline_init(vitte_baseline* b, vitte_baseline_entry* entries, size_t cap, vitte_arena* arena)
{
    b->entries = entries;
    b->count = 0;
    b->cap = cap;
    b->arena = arena;
}

/* -------------------------------------------------------------------------- */
/* Internal parsing helpers (C17, deterministic)                               */
/* -------------------------------------------------------------------------- */

static inline void vitte_baseline__skip_ws(const char** pp)
{
    const char* p = *pp;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    *pp = p;
}

static inline bool vitte_baseline__read_u64_tok(const char** pp, uint64_t* out)
{
    const char* p = *pp;
    vitte_baseline__skip_ws(&p);

    if (*p == '\0' || *p == '\n' || *p == '\r') {
        return false;
    }

    uint64_t v = 0;
    const char* q = p;
    while (*q >= '0' && *q <= '9') {
        uint64_t d = (uint64_t)(*q - '0');
        if (v > (UINT64_MAX - d) / 10u) {
            return false;
        }
        v = v * 10u + d;
        q++;
    }

    if (q == p) {
        return false;
    }

    if (*q != '\0' && *q != ' ' && *q != '\t' && *q != '\r' && *q != '\n') {
        return false;
    }

    *pp = q;
    if (out) {
        *out = v;
    }
    return true;
}

/* Simple float: [-]digits[.digits], no exponent, locale-independent */
static inline bool vitte_baseline__read_f64_tok_simple(const char** pp, double* out)
{
    const char* p = *pp;
    vitte_baseline__skip_ws(&p);

    if (*p == '\0' || *p == '\n' || *p == '\r') {
        return false;
    }

    bool neg = false;
    if (*p == '-') {
        neg = true;
        p++;
    }

    uint64_t ip = 0;
    bool any_int = false;
    while (*p >= '0' && *p <= '9') {
        any_int = true;
        uint64_t d = (uint64_t)(*p - '0');
        if (ip > (UINT64_MAX - d) / 10u) {
            return false;
        }
        ip = ip * 10u + d;
        p++;
    }

    double v = (double)ip;

    if (*p == '.') {
        p++;
        double scale = 1.0;
        bool any_frac = false;
        while (*p >= '0' && *p <= '9') {
            any_frac = true;
            uint64_t d = (uint64_t)(*p - '0');
            scale *= 0.1;
            v += (double)d * scale;
            p++;
        }
        if (!any_int && !any_frac) {
            return false;
        }
    } else {
        if (!any_int) {
            return false;
        }
    }

    if (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        return false;
    }

    *pp = p;
    if (neg) {
        v = -v;
    }
    if (out) {
        *out = v;
    }
    return true;
}

static inline const char* vitte_baseline__arena_strdup(vitte_arena* a, const char* s)
{
    if (!a || !s) {
        return s;
    }
    size_t n = 0;
    while (s[n] != '\0') n++;
    char* p = (char*)vitte_arena_alloc_aligned(a, n + 1, 1);
    if (!p) {
        return NULL;
    }
    for (size_t i = 0; i < n; i++) {
        p[i] = s[i];
    }
    p[n] = '\0';
    return p;
}

/* -------------------------------------------------------------------------- */
/* Utilities                                                                   */
/* -------------------------------------------------------------------------- */

static inline uint64_t vitte_baseline__metric_ns(const vitte_baseline_entry* e, vitte_baseline_metric m)
{
    switch (m) {
        case VITTE_BASELINE_P50:  return e->p50_ns;
        case VITTE_BASELINE_P90:  return e->p90_ns;
        case VITTE_BASELINE_P99:  return e->p99_ns;
        case VITTE_BASELINE_MEAN: return (uint64_t)(e->mean_ns + 0.5);
        default: return e->p50_ns;
    }
}

static inline const vitte_baseline_entry* vitte_baseline_find(const vitte_baseline* b, const char* key)
{
    if (!b || !key) {
        return NULL;
    }
    for (size_t i = 0; i < b->count; i++) {
        const vitte_baseline_entry* e = &b->entries[i];
        const char* a = e->key;
        const char* c = key;
        if (!a || !c) {
            continue;
        }
        while (*a && *c && *a == *c) {
            a++; c++;
        }
        if (*a == '\0' && *c == '\0') {
            return e;
        }
    }
    return NULL;
}

static inline bool vitte_baseline_push(vitte_baseline* b, vitte_baseline_entry e)
{
    if (!b || b->count >= b->cap) {
        return false;
    }
    b->entries[b->count++] = e;
    return true;
}

/* -------------------------------------------------------------------------- */
/* Parsing (format v1)                                                         */
/* -------------------------------------------------------------------------- */

/*
  Parse a single baseline line:
    <key> <p50_ns> <p90_ns> <p99_ns> <mean_ns>

  Returns true on success.
  Ignores empty lines and comment lines.
*/
static inline bool vitte_baseline_parse_line(vitte_baseline* b, const char* line)
{
    if (!b || !line) {
        return false;
    }

    const char* p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0' || *p == '\n' || *p == '#') {
        return true;
    }

    /* read key */
    const char* key0 = p;
    size_t key_len = 0;
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        key_len++; p++;
    }
    if (key_len == 0 || key_len >= 256) {
        return false;
    }

    char key_tmp[256];
    for (size_t i = 0; i < key_len; i++) {
        key_tmp[i] = key0[i];
    }
    key_tmp[key_len] = '\0';

    uint64_t p50 = 0, p90 = 0, p99 = 0;
    double mean = 0.0;

    if (!vitte_baseline__read_u64_tok(&p, &p50)) return false;
    if (!vitte_baseline__read_u64_tok(&p, &p90)) return false;
    if (!vitte_baseline__read_u64_tok(&p, &p99)) return false;
    if (!vitte_baseline__read_f64_tok_simple(&p, &mean)) return false;

    vitte_baseline_entry e;
    e.key = vitte_baseline__arena_strdup(b->arena, key_tmp);
    e.p50_ns = p50;
    e.p90_ns = p90;
    e.p99_ns = p99;
    e.mean_ns = mean;
    e.unit = "ns";

    return vitte_baseline_push(b, e);
}

/* -------------------------------------------------------------------------- */
/* I/O adapters                                                                */
/* -------------------------------------------------------------------------- */

/* Read callback: returns number of bytes read into buf (0 = EOF, <0 = error). */
typedef int (*vitte_baseline_read_fn)(void* user, void* buf, size_t cap);

/* Write callback: returns number of bytes written (<=0 = error). */
typedef int (*vitte_baseline_write_fn)(void* user, const void* buf, size_t n);

/*
  Load baseline from a stream using read callback.
  Splits on '\n'.
*/
static inline bool vitte_baseline_load(vitte_baseline* b, vitte_baseline_read_fn rd, void* user)
{
    if (!b || !rd) {
        return false;
    }

    char chunk[4096];
    char line[4096];
    size_t line_len = 0;

    for (;;) {
        int r = rd(user, chunk, sizeof(chunk));
        if (r < 0) {
            return false;
        }
        if (r == 0) {
            break;
        }

        for (int i = 0; i < r; i++) {
            char c = chunk[i];
            if (c == '\n') {
                line[line_len] = '\0';
                if (!vitte_baseline_parse_line(b, line)) {
                    return false;
                }
                line_len = 0;
                continue;
            }
            if (c == '\r') {
                continue;
            }
            if (line_len + 1 >= sizeof(line)) {
                return false;
            }
            line[line_len++] = c;
        }
    }

    if (line_len > 0) {
        line[line_len] = '\0';
        if (!vitte_baseline_parse_line(b, line)) {
            return false;
        }
    }

    return true;
}

/* Save baseline to a stream (format v1). */
static inline bool vitte_baseline_save(const vitte_baseline* b, vitte_baseline_write_fn wr, void* user)
{
    if (!b || !wr) {
        return false;
    }

    char buf[512];

    for (size_t i = 0; i < b->count; i++) {
        const vitte_baseline_entry* e = &b->entries[i];
        size_t n = (size_t)vitte_bench_snprintf(
            buf,
            sizeof(buf),
            "%s %llu %llu %llu %.6f\n",
            e->key ? e->key : "",
            (unsigned long long)e->p50_ns,
            (unsigned long long)e->p90_ns,
            (unsigned long long)e->p99_ns,
            e->mean_ns
        );
        if (n == 0 || n >= sizeof(buf)) {
            return false;
        }
        if (wr(user, buf, n) <= 0) {
            return false;
        }
    }

    return true;
}

/* -------------------------------------------------------------------------- */
/* Comparison                                                                  */
/* -------------------------------------------------------------------------- */

/*
  Compare current metric against baseline with thresholds.

  pass condition:
    curr <= base + max(abs_ns, base*rel)
*/
static inline vitte_baseline_result vitte_baseline_compare(
    const vitte_baseline_entry* base,
    uint64_t curr_ns,
    vitte_baseline_metric metric,
    vitte_baseline_threshold th
)
{
    vitte_baseline_result r;
    r.metric = metric;

    if (!base) {
        r.cmp = VITTE_BASELINE_MISSING;
        r.base_ns = 0;
        r.curr_ns = curr_ns;
        r.ratio = 0.0;
        r.delta_ns = 0;
        r.pass = false;
        return r;
    }

    uint64_t base_ns = vitte_baseline__metric_ns(base, metric);
    r.base_ns = base_ns;
    r.curr_ns = curr_ns;

    r.ratio = (base_ns == 0) ? 0.0 : ((double)curr_ns / (double)base_ns);

    if (curr_ns >= base_ns) {
        r.delta_ns = (curr_ns - base_ns);
        r.cmp = (curr_ns == base_ns) ? VITTE_BASELINE_EQUAL : VITTE_BASELINE_SLOWER;
    } else {
        r.delta_ns = (base_ns - curr_ns);
        r.cmp = VITTE_BASELINE_FASTER;
    }

    double rel_allow = (double)base_ns * th.rel;
    uint64_t rel_allow_ns = (uint64_t)(rel_allow + 0.5);
    uint64_t allow = (th.abs_ns > rel_allow_ns) ? th.abs_ns : rel_allow_ns;

    r.pass = (curr_ns <= base_ns + allow);
    return r;
}

/* -------------------------------------------------------------------------- */
/* Reporting helpers                                                           */
/* -------------------------------------------------------------------------- */

static inline const char* vitte_baseline_metric_name(vitte_baseline_metric m)
{
    switch (m) {
        case VITTE_BASELINE_P50: return "p50";
        case VITTE_BASELINE_P90: return "p90";
        case VITTE_BASELINE_P99: return "p99";
        case VITTE_BASELINE_MEAN: return "mean";
        default: return "p50";
    }
}

static inline const char* vitte_baseline_cmp_name(vitte_baseline_cmp c)
{
    switch (c) {
        case VITTE_BASELINE_EQUAL: return "equal";
        case VITTE_BASELINE_FASTER: return "faster";
        case VITTE_BASELINE_SLOWER: return "slower";
        case VITTE_BASELINE_MISSING: return "missing";
        default: return "equal";
    }
}

/*
  Append a human-readable comparison line into `out`.

  Example:
    key p50: base=123ns curr=130ns (+5.7%) PASS
*/
static inline bool vitte_baseline_format_line(vitte_buf* out, const char* key, vitte_baseline_result r)
{
    if (!out) {
        return false;
    }

    char bdur[64];
    char cdur[64];

    (void)vitte_bench_snprintf(bdur, sizeof(bdur), "%lluns", (unsigned long long)r.base_ns);
    (void)vitte_bench_snprintf(cdur, sizeof(cdur), "%lluns", (unsigned long long)r.curr_ns);

    const double pct = (r.base_ns == 0) ? 0.0 : ((double)r.curr_ns * 100.0 / (double)r.base_ns - 100.0);

    return vitte_buf_appendf(
        out,
        "%s %s: base=%s curr=%s (%+.2f%%) %s\n",
        key ? key : "",
        vitte_baseline_metric_name(r.metric),
        bdur,
        cdur,
        pct,
        r.pass ? "PASS" : "FAIL"
    );
}

/*
  Append a JSON object describing a baseline comparison.

  Schema:
    {
      "key": "...",
      "metric": "p50",
      "cmp": "slower",
      "base_ns": 123,
      "curr_ns": 130,
      "ratio": 1.0569,
      "pass": true
    }
*/
static inline bool vitte_baseline_format_json(vitte_buf* out, const char* key, vitte_baseline_result r)
{
    if (!out) {
        return false;
    }

    if (!vitte_buf_json_begin(out)) return false;

    if (!vitte_buf_append_json_kv_str(out, "key", key ? key : "", false)) return false;
    if (!vitte_buf_append_json_kv_str(out, "metric", vitte_baseline_metric_name(r.metric), true)) return false;
    if (!vitte_buf_append_json_kv_str(out, "cmp", vitte_baseline_cmp_name(r.cmp), true)) return false;
    if (!vitte_buf_append_json_kv_u64(out, "base_ns", r.base_ns, true)) return false;
    if (!vitte_buf_append_json_kv_u64(out, "curr_ns", r.curr_ns, true)) return false;

    /* ratio as number */
    {
        char tmp[64];
        (void)vitte_bench_snprintf(tmp, sizeof(tmp), "%.8f", r.ratio);
        if (!vitte_buf_append(out, ",\"ratio\":", 9)) return false;
        if (!vitte_buf_append_cstr(out, tmp)) return false;
    }

    /* pass as bool */
    if (!vitte_buf_append(out, ",\"pass\":", 8)) return false;
    if (!vitte_buf_append_cstr(out, r.pass ? "true" : "false")) return false;

    if (!vitte_buf_json_end(out)) return false;

    return true;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_BASELINE_H */