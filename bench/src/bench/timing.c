

// timing.c - timing/stat helpers for vitte/bench (C17)
//
// Provides:
//   - High-resolution monotonic timing wrappers
//   - Basic stats on samples (min/max/mean/stddev/median/percentiles)
//   - Simple unit formatting helpers
//
// Integration:
//   - If "bench/timing.h" exists, it will be included.
//   - Otherwise, this file provides a fallback public surface.
//
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "bench/platform.h"

#if defined(__has_include)
  #if __has_include("bench/timing.h")
    #include "bench/timing.h"
  #elif __has_include("timing.h")
    #include "timing.h"
  #endif
#endif

// -----------------------------------------------------------------------------
// Fallback public surface (only if timing.h was not included)
// -----------------------------------------------------------------------------

#ifndef VITTE_BENCH_TIMING_H
#define VITTE_BENCH_TIMING_H

typedef struct bench_timer
{
    uint64_t t0_ns;
} bench_timer;

typedef struct bench_stats
{
    int32_t count;
    double min;
    double max;
    double mean;
    double stddev;
    double median;
    double p90;
    double p99;
} bench_stats;

// Timer
void bench_timer_start(bench_timer* t);
uint64_t bench_timer_elapsed_ns(const bench_timer* t);

double bench_ns_to_ms(uint64_t ns);
double bench_ns_to_s(uint64_t ns);

// Stats
bench_stats bench_stats_compute(const double* samples, int32_t count);

// Format a duration in a compact human form (ns/us/ms/s).
// Writes to `out` and returns out.
char* bench_format_duration(double ns, char* out, size_t cap);

#endif // VITTE_BENCH_TIMING_H

// -----------------------------------------------------------------------------
// Timer
// -----------------------------------------------------------------------------

void bench_timer_start(bench_timer* t)
{
    if (!t) return;
    t->t0_ns = bench_time_now_ns();
}

uint64_t bench_timer_elapsed_ns(const bench_timer* t)
{
    if (!t) return 0;
    uint64_t now = bench_time_now_ns();
    return (now >= t->t0_ns) ? (now - t->t0_ns) : 0;
}

double bench_ns_to_ms(uint64_t ns)
{
    return (double)ns / 1000000.0;
}

double bench_ns_to_s(uint64_t ns)
{
    return (double)ns / 1000000000.0;
}

// -----------------------------------------------------------------------------
// Stats
// -----------------------------------------------------------------------------

static int cmp_double_(const void* a, const void* b)
{
    double da = *(const double*)a;
    double db = *(const double*)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

static double percentile_sorted_(const double* sorted, int32_t n, double q)
{
    if (!sorted || n <= 0) return 0.0;
    if (q <= 0.0) return sorted[0];
    if (q >= 1.0) return sorted[n - 1];

    // linear interpolation between closest ranks
    double pos = q * (double)(n - 1);
    int32_t i = (int32_t)pos;
    double f = pos - (double)i;
    if (i < 0) i = 0;
    if (i >= n - 1) return sorted[n - 1];
    return sorted[i] * (1.0 - f) + sorted[i + 1] * f;
}

bench_stats bench_stats_compute(const double* samples, int32_t count)
{
    bench_stats st;
    memset(&st, 0, sizeof(st));
    st.count = count;

    if (!samples || count <= 0)
        return st;

    double minv = samples[0];
    double maxv = samples[0];
    double sum = 0.0;

    for (int32_t i = 0; i < count; ++i)
    {
        double x = samples[i];
        if (x < minv) minv = x;
        if (x > maxv) maxv = x;
        sum += x;
    }

    double mean = sum / (double)count;

    // sample stddev (unbiased for n>1)
    double var = 0.0;
    if (count > 1)
    {
        for (int32_t i = 0; i < count; ++i)
        {
            double d = samples[i] - mean;
            var += d * d;
        }
        var /= (double)(count - 1);
    }

    // median/p90/p99 need sort copy
    double* tmp = (double*)malloc((size_t)count * sizeof(double));
    if (tmp)
    {
        memcpy(tmp, samples, (size_t)count * sizeof(double));
        qsort(tmp, (size_t)count, sizeof(double), cmp_double_);

        st.median = percentile_sorted_(tmp, count, 0.50);
        st.p90 = percentile_sorted_(tmp, count, 0.90);
        st.p99 = percentile_sorted_(tmp, count, 0.99);

        free(tmp);
    }
    else
    {
        // fallback: approximate median with mean
        st.median = mean;
        st.p90 = mean;
        st.p99 = mean;
    }

    st.min = minv;
    st.max = maxv;
    st.mean = mean;
    st.stddev = (var > 0.0) ? sqrt(var) : 0.0;

    return st;
}

// -----------------------------------------------------------------------------
// Formatting
// -----------------------------------------------------------------------------

char* bench_format_duration(double ns, char* out, size_t cap)
{
    if (!out || cap == 0) return out;

    const char* unit = "ns";
    double v = ns;

    if (v >= 1000.0)
    {
        v /= 1000.0;
        unit = "us";
    }
    if (v >= 1000.0)
    {
        v /= 1000.0;
        unit = "ms";
    }
    if (v >= 1000.0)
    {
        v /= 1000.0;
        unit = "s";
    }

    // precision heuristic
    if (v < 10.0)
        (void)snprintf(out, cap, "%.3f %s", v, unit);
    else if (v < 100.0)
        (void)snprintf(out, cap, "%.2f %s", v, unit);
    else
        (void)snprintf(out, cap, "%.1f %s", v, unit);

    return out;
}