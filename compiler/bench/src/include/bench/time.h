/*
  bench/time.h

  Time utilities for the Vitte benchmark harness.

  Goals
  - Provide a single, monotonic time source in nanoseconds.
  - Offer helpers for duration conversions, arithmetic, and formatting.
  - Avoid libc locale; keep deterministic behavior.

  Source of truth
  - `vitte_time_now_ns()` delegates to bench/os.h `vitte_os_monotonic_ns()`.

  Notes
  - Wall-clock timestamps (UTC) used in reports are out-of-scope here
    (see bench/detail/time_utc.h).
  - This module is not a scheduler. Sleep/backoff live in bench/os.h.
*/

#pragma once
#ifndef VITTE_BENCH_TIME_H
#define VITTE_BENCH_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/os.h"
#include "bench/diag.h"
#include "bench/detail/format.h" /* vitte_buf */

/* -------------------------------------------------------------------------- */
/* Constants                                                                    */
/* -------------------------------------------------------------------------- */

#define VITTE_NS_PER_US 1000ull
#define VITTE_NS_PER_MS (1000ull * 1000ull)
#define VITTE_NS_PER_S  (1000ull * 1000ull * 1000ull)

/* -------------------------------------------------------------------------- */
/* Monotonic clock                                                             */
/* -------------------------------------------------------------------------- */

/* Monotonic time in nanoseconds. */
static inline uint64_t vitte_time_now_ns(void)
{
    return vitte_os_monotonic_ns();
}

/* -------------------------------------------------------------------------- */
/* Conversions                                                                  */
/* -------------------------------------------------------------------------- */

static inline uint64_t vitte_time_ns_from_us(uint64_t us) { return us * VITTE_NS_PER_US; }
static inline uint64_t vitte_time_ns_from_ms(uint64_t ms) { return ms * VITTE_NS_PER_MS; }
static inline uint64_t vitte_time_ns_from_s(uint64_t s)   { return s * VITTE_NS_PER_S; }

static inline double vitte_time_us_from_ns(uint64_t ns) { return (double)ns / (double)VITTE_NS_PER_US; }
static inline double vitte_time_ms_from_ns(uint64_t ns) { return (double)ns / (double)VITTE_NS_PER_MS; }
static inline double vitte_time_s_from_ns(uint64_t ns)  { return (double)ns / (double)VITTE_NS_PER_S; }

/* -------------------------------------------------------------------------- */
/* Saturating arithmetic                                                        */
/* -------------------------------------------------------------------------- */

/* Saturating add/sub helpers (avoid overflow/underflow). */
static inline uint64_t vitte_time_ns_add_sat(uint64_t a, uint64_t b)
{
    uint64_t r = a + b;
    if (r < a) return UINT64_MAX;
    return r;
}

static inline uint64_t vitte_time_ns_sub_sat(uint64_t a, uint64_t b)
{
    if (a < b) return 0ull;
    return a - b;
}

static inline uint64_t vitte_time_ns_mul_sat(uint64_t a, uint64_t b)
{
    if (a == 0ull || b == 0ull) return 0ull;
    if (a > UINT64_MAX / b) return UINT64_MAX;
    return a * b;
}

/* Clamp ns into [lo, hi]. */
static inline uint64_t vitte_time_ns_clamp(uint64_t ns, uint64_t lo, uint64_t hi)
{
    if (ns < lo) return lo;
    if (ns > hi) return hi;
    return ns;
}

/* -------------------------------------------------------------------------- */
/* Strong-ish types (optional convenience)                                     */
/* -------------------------------------------------------------------------- */

typedef struct vitte_duration_ns {
    uint64_t ns;
} vitte_duration_ns;

typedef struct vitte_timepoint_ns {
    uint64_t ns;
} vitte_timepoint_ns;

static inline vitte_duration_ns vitte_duration_from_ns(uint64_t ns) { vitte_duration_ns d = { ns }; return d; }
static inline vitte_duration_ns vitte_duration_from_us(uint64_t us) { return vitte_duration_from_ns(vitte_time_ns_from_us(us)); }
static inline vitte_duration_ns vitte_duration_from_ms(uint64_t ms) { return vitte_duration_from_ns(vitte_time_ns_from_ms(ms)); }
static inline vitte_duration_ns vitte_duration_from_s(uint64_t s)   { return vitte_duration_from_ns(vitte_time_ns_from_s(s)); }

static inline uint64_t vitte_duration_as_ns(vitte_duration_ns d) { return d.ns; }
static inline double vitte_duration_as_us(vitte_duration_ns d) { return vitte_time_us_from_ns(d.ns); }
static inline double vitte_duration_as_ms(vitte_duration_ns d) { return vitte_time_ms_from_ns(d.ns); }
static inline double vitte_duration_as_s(vitte_duration_ns d)  { return vitte_time_s_from_ns(d.ns); }

static inline vitte_duration_ns vitte_duration_add(vitte_duration_ns a, vitte_duration_ns b)
{
    return vitte_duration_from_ns(vitte_time_ns_add_sat(a.ns, b.ns));
}

static inline vitte_duration_ns vitte_duration_sub(vitte_duration_ns a, vitte_duration_ns b)
{
    return vitte_duration_from_ns(vitte_time_ns_sub_sat(a.ns, b.ns));
}

static inline vitte_timepoint_ns vitte_timepoint_now(void)
{
    vitte_timepoint_ns t = { vitte_time_now_ns() };
    return t;
}

static inline vitte_timepoint_ns vitte_timepoint_add(vitte_timepoint_ns t, vitte_duration_ns d)
{
    vitte_timepoint_ns r = { vitte_time_ns_add_sat(t.ns, d.ns) };
    return r;
}

static inline vitte_duration_ns vitte_timepoint_since(vitte_timepoint_ns now, vitte_timepoint_ns then)
{
    return vitte_duration_from_ns(vitte_time_ns_sub_sat(now.ns, then.ns));
}

/* -------------------------------------------------------------------------- */
/* Duration formatting (deterministic)                                         */
/* -------------------------------------------------------------------------- */

/* Formatting flags for *_ex helpers. */
#define VITTE_TIME_FMT_NONE           0u
#define VITTE_TIME_FMT_ALWAYS_3DP     (1u << 0) /* force 3 decimals even for ns */
#define VITTE_TIME_FMT_SPACE_BEFORE_UNIT (1u << 1) /* e.g. "1.234 ms" */

/*
  Default formatter (keeps legacy signature).

  Rules
  - Select largest unit that yields value >= 1.0 (s/ms/us/ns).
  - Render with 3 decimals for s/ms/us, integer for ns.
  - No thousands separators.

  Output examples
    950ns
    12.345us
    1.234ms
    0.123s
*/
static inline bool vitte_time_format_duration(vitte_buf* out, uint64_t ns, vitte_diag* d);

/* Extended formatter with flags (deterministic). */
static inline bool vitte_time_format_duration_ex(vitte_buf* out, uint64_t ns, uint32_t flags, vitte_diag* d);

/* Format in a fixed unit (ns/us/ms/s). `unit_ns` must be 1, 1e3, 1e6 or 1e9. */
static inline bool vitte_time_format_duration_fixed(vitte_buf* out, uint64_t ns, uint64_t unit_ns, int decimals, uint32_t flags, vitte_diag* d);

/* -------------------------------------------------------------------------- */
/* Stopwatch                                                                    */
/* -------------------------------------------------------------------------- */

typedef struct vitte_stopwatch {
    uint64_t start_ns;
} vitte_stopwatch;

static inline void vitte_stopwatch_start(vitte_stopwatch* sw)
{
    if (!sw) return;
    sw->start_ns = vitte_time_now_ns();
}

static inline uint64_t vitte_stopwatch_lap_ns(const vitte_stopwatch* sw)
{
    if (!sw) return 0ull;
    return vitte_time_ns_sub_sat(vitte_time_now_ns(), sw->start_ns);
}

static inline uint64_t vitte_stopwatch_restart_ns(vitte_stopwatch* sw)
{
    if (!sw) return 0ull;
    uint64_t now = vitte_time_now_ns();
    uint64_t dt = vitte_time_ns_sub_sat(now, sw->start_ns);
    sw->start_ns = now;
    return dt;
}

/* Extended stopwatch: start/stop/resume accumulation (useful for multi-phase). */

typedef struct vitte_stopwatch_ex {
    uint64_t t0_ns;
    uint64_t acc_ns;
    bool running;
} vitte_stopwatch_ex;

static inline void vitte_stopwatch_ex_reset(vitte_stopwatch_ex* sw)
{
    if (!sw) return;
    sw->t0_ns = 0ull;
    sw->acc_ns = 0ull;
    sw->running = false;
}

static inline void vitte_stopwatch_ex_start(vitte_stopwatch_ex* sw)
{
    if (!sw) return;
    sw->acc_ns = 0ull;
    sw->t0_ns = vitte_time_now_ns();
    sw->running = true;
}

static inline void vitte_stopwatch_ex_resume(vitte_stopwatch_ex* sw)
{
    if (!sw || sw->running) return;
    sw->t0_ns = vitte_time_now_ns();
    sw->running = true;
}

static inline void vitte_stopwatch_ex_stop(vitte_stopwatch_ex* sw)
{
    if (!sw || !sw->running) return;
    uint64_t now = vitte_time_now_ns();
    sw->acc_ns = vitte_time_ns_add_sat(sw->acc_ns, vitte_time_ns_sub_sat(now, sw->t0_ns));
    sw->running = false;
}

static inline uint64_t vitte_stopwatch_ex_elapsed_ns(const vitte_stopwatch_ex* sw)
{
    if (!sw) return 0ull;
    if (!sw->running) return sw->acc_ns;
    uint64_t now = vitte_time_now_ns();
    return vitte_time_ns_add_sat(sw->acc_ns, vitte_time_ns_sub_sat(now, sw->t0_ns));
}

/* -------------------------------------------------------------------------- */
/* Measurement helpers                                                          */
/* -------------------------------------------------------------------------- */

/*
  Estimate timer-call overhead by measuring `iters` back-to-back calls.
  Returns the minimum observed delta (ns) as a conservative overhead.

  Notes
  - This is best-effort; on some platforms the minimum may be 0 due to
    coarse timers or timing aliasing.
*/
static inline uint64_t vitte_time_estimate_overhead_ns(uint32_t iters)
{
    if (iters < 2u) iters = 2u;

    uint64_t best = UINT64_MAX;
    uint64_t prev = vitte_time_now_ns();

    for (uint32_t i = 0; i < iters; ++i) {
        uint64_t now = vitte_time_now_ns();
        uint64_t dt = vitte_time_ns_sub_sat(now, prev);
        if (dt < best) best = dt;
        prev = now;
    }

    if (best == UINT64_MAX) best = 0ull;
    return best;
}

/* -------------------------------------------------------------------------- */
/* Implementation (header-only)                                                */
/* -------------------------------------------------------------------------- */

static inline bool vitte_time_format_duration_fixed(vitte_buf* out, uint64_t ns, uint64_t unit_ns, int decimals, uint32_t flags, vitte_diag* d)
{
    if (!out) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_time_format_duration_fixed: out is NULL");
        return false;
    }

    if (unit_ns == 0ull) unit_ns = 1ull;
    if (decimals < 0) decimals = 0;
    if (decimals > 9) decimals = 9;

    const bool sp = (flags & VITTE_TIME_FMT_SPACE_BEFORE_UNIT) != 0u;

    const char* unit = "ns";
    if (unit_ns == VITTE_NS_PER_US) unit = "us";
    else if (unit_ns == VITTE_NS_PER_MS) unit = "ms";
    else if (unit_ns == VITTE_NS_PER_S) unit = "s";

    if (unit_ns == 1ull && decimals == 0 && (flags & VITTE_TIME_FMT_ALWAYS_3DP) == 0u) {
        if (!vitte_buf_appendf(out, "%llu%s%s", (unsigned long long)ns, sp ? " " : "", unit)) {
            VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_time_format_duration_fixed: oom");
            return false;
        }
        return true;
    }

    const double v = (double)ns / (double)unit_ns;

    /* We can't portably build a dynamic precision format without extra helpers.
       Provide fixed common paths: 0,3,6,9 decimals; otherwise fall back to 3.
    */
    int dp = decimals;
    if (dp != 0 && dp != 3 && dp != 6 && dp != 9) dp = 3;

    if (dp == 0) {
        if (!vitte_buf_appendf(out, "%.0f%s%s", v, sp ? " " : "", unit)) {
            VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_time_format_duration_fixed: oom");
            return false;
        }
        return true;
    }

    if (dp == 6) {
        if (!vitte_buf_appendf(out, "%.6f%s%s", v, sp ? " " : "", unit)) {
            VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_time_format_duration_fixed: oom");
            return false;
        }
        return true;
    }

    if (dp == 9) {
        if (!vitte_buf_appendf(out, "%.9f%s%s", v, sp ? " " : "", unit)) {
            VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_time_format_duration_fixed: oom");
            return false;
        }
        return true;
    }

    /* dp == 3 */
    if (!vitte_buf_appendf(out, "%.3f%s%s", v, sp ? " " : "", unit)) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_time_format_duration_fixed: oom");
        return false;
    }
    return true;
}

static inline bool vitte_time_format_duration_ex(vitte_buf* out, uint64_t ns, uint32_t flags, vitte_diag* d)
{
    /* Choose unit */
    if (ns >= VITTE_NS_PER_S) {
        return vitte_time_format_duration_fixed(out, ns, VITTE_NS_PER_S, 3, flags, d);
    }
    if (ns >= VITTE_NS_PER_MS) {
        return vitte_time_format_duration_fixed(out, ns, VITTE_NS_PER_MS, 3, flags, d);
    }
    if (ns >= VITTE_NS_PER_US) {
        return vitte_time_format_duration_fixed(out, ns, VITTE_NS_PER_US, 3, flags, d);
    }

    /* ns */
    if ((flags & VITTE_TIME_FMT_ALWAYS_3DP) != 0u) {
        /* show ns as fractional us for consistency */
        return vitte_time_format_duration_fixed(out, ns, VITTE_NS_PER_US, 3, flags, d);
    }

    return vitte_time_format_duration_fixed(out, ns, 1ull, 0, flags, d);
}

static inline bool vitte_time_format_duration(vitte_buf* out, uint64_t ns, vitte_diag* d)
{
    return vitte_time_format_duration_ex(out, ns, VITTE_TIME_FMT_NONE, d);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_TIME_H */