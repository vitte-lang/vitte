// SPDX-License-Identifier: MIT
// pal_posix_time.c
//
// POSIX time backend (max).
//
// Provides:
//  - monotonic time (ns/us/ms)
//  - realtime (unix epoch ms)
//  - sleep helpers
//  - high-resolution stopwatch
//
// Notes:
//  - Uses clock_gettime when available.
//  - On macOS < 10.12 clock_gettime historically missing; modern macOS has it.
//    We keep a mach_absolute_time fallback for extra safety.
//
// If provided, integrates with `pal_time.h`. Otherwise, defines a fallback API.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("../pal_time.h")
    #include "../pal_time.h"
    #define STEEL_HAS_PAL_TIME_H 1
  #elif __has_include("pal_time.h")
    #include "pal_time.h"
    #define STEEL_HAS_PAL_TIME_H 1
  #endif
#endif

#ifndef STEEL_HAS_PAL_TIME_H

typedef struct pal_stopwatch
{
    uint64_t start_ns;
} pal_stopwatch;

uint64_t pal_time_monotonic_ns(void);
uint64_t pal_time_monotonic_us(void);
uint64_t pal_time_monotonic_ms(void);

uint64_t pal_time_unix_ms(void);

void pal_sleep_ms(uint32_t ms);
void pal_sleep_us(uint32_t us);

void pal_stopwatch_start(pal_stopwatch* sw);
uint64_t pal_stopwatch_elapsed_ns(const pal_stopwatch* sw);
uint64_t pal_stopwatch_elapsed_us(const pal_stopwatch* sw);
uint64_t pal_stopwatch_elapsed_ms(const pal_stopwatch* sw);

const char* pal_time_last_error(void);

#endif // !STEEL_HAS_PAL_TIME_H

//------------------------------------------------------------------------------
// Error
//------------------------------------------------------------------------------

static _Thread_local char g_time_err_[256];

static void set_msg_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_time_err_)) n = sizeof(g_time_err_) - 1;
    memcpy(g_time_err_, msg, n);
    g_time_err_[n] = 0;
}

static void set_errno_(const char* prefix, int e)
{
    const char* s = strerror(e);
    if (!prefix) prefix = "";
    if (!s) s = "";
    if (prefix[0])
        snprintf(g_time_err_, sizeof(g_time_err_), "%s: %s", prefix, s);
    else
        snprintf(g_time_err_, sizeof(g_time_err_), "%s", s);
}

const char* pal_time_last_error(void)
{
    return g_time_err_;
}

//------------------------------------------------------------------------------
// clock_gettime wrapper + macOS fallback
//------------------------------------------------------------------------------

#if defined(__APPLE__)

#include <AvailabilityMacros.h>
#include <mach/mach_time.h>

static uint64_t mach_monotonic_ns_(void)
{
    static mach_timebase_info_data_t tb;
    if (tb.denom == 0)
        (void)mach_timebase_info(&tb);

    uint64_t t = mach_absolute_time();
    // ns = t * numer / denom
    __uint128_t v = ( (__uint128_t)t * (__uint128_t)tb.numer );
    v /= (__uint128_t)tb.denom;
    return (uint64_t)v;
}

#endif

static bool gettime_(clockid_t cid, struct timespec* out)
{
    if (!out) return false;

#if defined(__APPLE__)
    // Try clock_gettime first (modern macOS).
    if (clock_gettime(cid, out) == 0)
        return true;

    // Fallback for monotonic only.
    if (cid == CLOCK_MONOTONIC)
    {
        uint64_t ns = mach_monotonic_ns_();
        out->tv_sec = (time_t)(ns / 1000000000ull);
        out->tv_nsec = (long)(ns % 1000000000ull);
        return true;
    }

    set_errno_("clock_gettime", errno);
    return false;
#else
    if (clock_gettime(cid, out) == 0)
        return true;

    set_errno_("clock_gettime", errno);
    return false;
#endif
}

static uint64_t ts_to_ns_(const struct timespec* ts)
{
    if (!ts) return 0;
    return (uint64_t)ts->tv_sec * 1000000000ull + (uint64_t)ts->tv_nsec;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

uint64_t pal_time_monotonic_ns(void)
{
    struct timespec ts;
    if (!gettime_(CLOCK_MONOTONIC, &ts))
        return 0;
    return ts_to_ns_(&ts);
}

uint64_t pal_time_monotonic_us(void)
{
    uint64_t ns = pal_time_monotonic_ns();
    return ns / 1000ull;
}

uint64_t pal_time_monotonic_ms(void)
{
    uint64_t ns = pal_time_monotonic_ns();
    return ns / 1000000ull;
}

uint64_t pal_time_unix_ms(void)
{
    struct timespec ts;
    if (!gettime_(CLOCK_REALTIME, &ts))
        return 0;

    uint64_t ms = (uint64_t)ts.tv_sec * 1000ull;
    ms += (uint64_t)ts.tv_nsec / 1000000ull;
    return ms;
}

void pal_sleep_ms(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)((ms % 1000u) * 1000000ul);

    while (nanosleep(&ts, &ts) != 0)
    {
        if (errno == EINTR)
            continue;
        set_errno_("nanosleep", errno);
        break;
    }
}

void pal_sleep_us(uint32_t us)
{
    // Convert to timespec
    struct timespec ts;
    ts.tv_sec = (time_t)(us / 1000000u);
    ts.tv_nsec = (long)((us % 1000000u) * 1000ul);

    while (nanosleep(&ts, &ts) != 0)
    {
        if (errno == EINTR)
            continue;
        set_errno_("nanosleep", errno);
        break;
    }
}

void pal_stopwatch_start(pal_stopwatch* sw)
{
    if (!sw) return;
    sw->start_ns = pal_time_monotonic_ns();
}

uint64_t pal_stopwatch_elapsed_ns(const pal_stopwatch* sw)
{
    if (!sw) return 0;
    uint64_t now = pal_time_monotonic_ns();
    return (now >= sw->start_ns) ? (now - sw->start_ns) : 0;
}

uint64_t pal_stopwatch_elapsed_us(const pal_stopwatch* sw)
{
    return pal_stopwatch_elapsed_ns(sw) / 1000ull;
}

uint64_t pal_stopwatch_elapsed_ms(const pal_stopwatch* sw)
{
    return pal_stopwatch_elapsed_ns(sw) / 1000000ull;
}

