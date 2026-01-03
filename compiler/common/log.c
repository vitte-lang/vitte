// SPDX-License-Identifier: MIT
// log.c
//
// Minimal logging facility for vitte.
//
// Features:
//  - Log levels (trace/debug/info/warn/error/fatal)
//  - Optional timestamps
//  - Optional ANSI colors (auto/off/on)
//  - Thread-safe best-effort using a lightweight global lock (atomics spin)
//  - Pluggable sink callback
//
// This file pairs with log.h.
// log.h is expected to declare:
//   - enum steel_log_level
//   - enum steel_log_color_mode
//   - typedef void (*steel_log_sink_fn)(void* ctx, steel_log_level lvl, const char* line)
//   - configuration getters/setters used below
//   - the public log functions implemented here
//
// If your log.h differs, adjust one side accordingly.

#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <unistd.h>
#endif

//------------------------------------------------------------------------------
// Defaults / globals
//------------------------------------------------------------------------------

#ifndef STEEL_LOG_LINE_MAX
  #define STEEL_LOG_LINE_MAX 2048
#endif

#ifndef STEEL_LOG_TIME_FMT
  #define STEEL_LOG_TIME_FMT "%Y-%m-%d %H:%M:%S"
#endif

#ifndef STEEL_LOG_SPIN_MAX
  #define STEEL_LOG_SPIN_MAX 100000
#endif

// If atomics are available, use them; otherwise fallback to non-thread-safe.

#if !defined(STEEL_LOG_NO_ATOMICS)
  #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
    #define STEEL_LOG_HAS_STDATOMIC 1
  #endif
#endif

#if defined(STEEL_LOG_HAS_STDATOMIC)
  #include <stdatomic.h>
  static atomic_flag g_log_lock = ATOMIC_FLAG_INIT;
  static void steel_log_lock(void)
  {
      for (int i = 0; i < STEEL_LOG_SPIN_MAX; i++)
      {
          if (!atomic_flag_test_and_set_explicit(&g_log_lock, memory_order_acquire))
              return;
      }
      // If we failed to acquire after spinning, proceed without lock.
  }
  static void steel_log_unlock(void)
  {
      atomic_flag_clear_explicit(&g_log_lock, memory_order_release);
  }
#else
  static void steel_log_lock(void) { }
  static void steel_log_unlock(void) { }
#endif

static steel_log_level g_level = STEEL_LOG_INFO;
static bool g_show_time = false;
static steel_log_color_mode g_color_mode = STEEL_LOG_COLOR_AUTO;

static steel_log_sink_fn g_sink = NULL;
static void* g_sink_ctx = NULL;

//------------------------------------------------------------------------------
// ANSI color
//------------------------------------------------------------------------------

static bool steel_log_is_tty(FILE* f)
{
#if defined(_WIN32)
    (void)f;
    // Conservative: assume console supports colors when auto; Windows 10+ supports VT,
    // but enabling it requires SetConsoleMode. We avoid side effects here.
    return false;
#else
    int fd = (f == stderr) ? 2 : 1;
    return isatty(fd) == 1;
#endif
}

static const char* steel_log_level_name(steel_log_level lvl)
{
    switch (lvl)
    {
        case STEEL_LOG_TRACE: return "TRACE";
        case STEEL_LOG_DEBUG: return "DEBUG";
        case STEEL_LOG_INFO:  return "INFO";
        case STEEL_LOG_WARN:  return "WARN";
        case STEEL_LOG_ERROR: return "ERROR";
        case STEEL_LOG_FATAL: return "FATAL";
        default: return "INFO";
    }
}

static const char* steel_log_level_color(steel_log_level lvl)
{
    // Standard ANSI SGR colors.
    switch (lvl)
    {
        case STEEL_LOG_TRACE: return "\x1b[90m"; // bright black
        case STEEL_LOG_DEBUG: return "\x1b[36m"; // cyan
        case STEEL_LOG_INFO:  return "\x1b[32m"; // green
        case STEEL_LOG_WARN:  return "\x1b[33m"; // yellow
        case STEEL_LOG_ERROR: return "\x1b[31m"; // red
        case STEEL_LOG_FATAL: return "\x1b[35m"; // magenta
        default: return "\x1b[0m";
    }
}

static bool steel_log_use_color(FILE* out)
{
    if (g_color_mode == STEEL_LOG_COLOR_OFF) return false;
    if (g_color_mode == STEEL_LOG_COLOR_ON) return true;
    // AUTO
    return steel_log_is_tty(out);
}

//------------------------------------------------------------------------------
// Timestamp
//------------------------------------------------------------------------------

static void steel_log_format_time(char* buf, size_t cap)
{
    if (!buf || cap == 0) return;

    time_t t = time(NULL);
    struct tm tmv;

#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif

    strftime(buf, cap, STEEL_LOG_TIME_FMT, &tmv);
}

//------------------------------------------------------------------------------
// Sink
//------------------------------------------------------------------------------

static void steel_log_default_sink(void* ctx, steel_log_level lvl, const char* line)
{
    (void)ctx;
    FILE* out = (lvl >= STEEL_LOG_WARN) ? stderr : stdout;
    fputs(line, out);
    fputc('\n', out);
    fflush(out);
}

//------------------------------------------------------------------------------
// Public configuration API
//------------------------------------------------------------------------------

void steel_log_set_level(steel_log_level lvl)
{
    g_level = lvl;
}

steel_log_level steel_log_get_level(void)
{
    return g_level;
}

void steel_log_set_show_time(bool enabled)
{
    g_show_time = enabled;
}

bool steel_log_get_show_time(void)
{
    return g_show_time;
}

void steel_log_set_color_mode(steel_log_color_mode mode)
{
    g_color_mode = mode;
}

steel_log_color_mode steel_log_get_color_mode(void)
{
    return g_color_mode;
}

void steel_log_set_sink(steel_log_sink_fn fn, void* ctx)
{
    g_sink = fn;
    g_sink_ctx = ctx;
}

void steel_log_get_sink(steel_log_sink_fn* out_fn, void** out_ctx)
{
    if (out_fn) *out_fn = g_sink;
    if (out_ctx) *out_ctx = g_sink_ctx;
}

//------------------------------------------------------------------------------
// Formatting core
//------------------------------------------------------------------------------

static void steel_log_vwrite(steel_log_level lvl, const char* fmt, va_list ap)
{
    if (lvl < g_level) return;

    char msg[STEEL_LOG_LINE_MAX];
    char line[STEEL_LOG_LINE_MAX];

    // Format message
    vsnprintf(msg, sizeof(msg), fmt ? fmt : "", ap);

    // Prefix
    char ts[64];
    ts[0] = 0;
    if (g_show_time)
        steel_log_format_time(ts, sizeof(ts));

    const char* lvl_name = steel_log_level_name(lvl);

    // Build final line (without newline)
    if (g_show_time && ts[0])
        snprintf(line, sizeof(line), "%s [%s] %s", ts, lvl_name, msg);
    else
        snprintf(line, sizeof(line), "[%s] %s", lvl_name, msg);

    // Lock and emit
    steel_log_lock();

    steel_log_sink_fn sink = g_sink ? g_sink : steel_log_default_sink;

    if (sink == steel_log_default_sink)
    {
        FILE* out = (lvl >= STEEL_LOG_WARN) ? stderr : stdout;
        if (steel_log_use_color(out))
        {
            fputs(steel_log_level_color(lvl), out);
            fputs(line, out);
            fputs("\x1b[0m", out);
            fputc('\n', out);
        }
        else
        {
            fputs(line, out);
            fputc('\n', out);
        }
        fflush(out);
    }
    else
    {
        sink(g_sink_ctx, lvl, line);
    }

    steel_log_unlock();

    if (lvl == STEEL_LOG_FATAL)
    {
        // Fatal should terminate.
        abort();
    }
}

//------------------------------------------------------------------------------
// Public logging API
//------------------------------------------------------------------------------

void steel_log_write(steel_log_level lvl, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    steel_log_vwrite(lvl, fmt, ap);
    va_end(ap);
}

void steel_log_trace(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    steel_log_vwrite(STEEL_LOG_TRACE, fmt, ap);
    va_end(ap);
}

void steel_log_debug(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    steel_log_vwrite(STEEL_LOG_DEBUG, fmt, ap);
    va_end(ap);
}

void steel_log_info(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    steel_log_vwrite(STEEL_LOG_INFO, fmt, ap);
    va_end(ap);
}

void steel_log_warn(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    steel_log_vwrite(STEEL_LOG_WARN, fmt, ap);
    va_end(ap);
}

void steel_log_error(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    steel_log_vwrite(STEEL_LOG_ERROR, fmt, ap);
    va_end(ap);
}

void steel_log_fatal(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    steel_log_vwrite(STEEL_LOG_FATAL, fmt, ap);
    va_end(ap);
}

