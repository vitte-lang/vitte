

// log.c
// -----------------------------------------------------------------------------
// Benchmark logging facility (C17, no external deps).
//
// Goals:
//   - minimal overhead, predictable output
//   - usable from benches/tools without pulling large frameworks
//   - optional ANSI colors when output is a TTY
//   - simple level filtering
//
// Environment variables:
//   - BENCH_LOG_LEVEL : trace|debug|info|warn|error|fatal|off (case-insensitive)
//   - BENCH_LOG_COLOR : auto|0|1|false|true
//   - BENCH_LOG_QUIET : 0|1
//
// Integration:
//   - If a matching "log.h" exists, it will be included.
//   - Otherwise, this file provides a small fallback API declaration.
// -----------------------------------------------------------------------------

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__has_include)
#  if __has_include("log.h")
#    include "log.h"
#    define BENCH_LOG_HAS_HEADER 1
#  else
#    define BENCH_LOG_HAS_HEADER 0
#  endif
#else
#  define BENCH_LOG_HAS_HEADER 0
#endif

#if !BENCH_LOG_HAS_HEADER

typedef enum bench_log_level {
    BENCH_LOG_TRACE = 0,
    BENCH_LOG_DEBUG,
    BENCH_LOG_INFO,
    BENCH_LOG_WARN,
    BENCH_LOG_ERROR,
    BENCH_LOG_FATAL,
    BENCH_LOG_OFF,
} bench_log_level_t;

void bench_log_init(FILE* out);
void bench_log_set_level(bench_log_level_t lvl);
bench_log_level_t bench_log_get_level(void);
void bench_log_set_color(int enabled /* -1=auto, 0=off, 1=on */);
void bench_log_set_quiet(int quiet);
const char* bench_log_level_name(bench_log_level_t lvl);
bench_log_level_t bench_log_level_parse(const char* s, bench_log_level_t deflt);
void bench_vlogf(bench_log_level_t lvl, const char* file, int line, const char* func,
                 const char* fmt, va_list ap);
void bench_logf(bench_log_level_t lvl, const char* file, int line, const char* func,
                const char* fmt, ...);

#endif // !BENCH_LOG_HAS_HEADER

// -----------------------------------------------------------------------------
// Platform helpers
// -----------------------------------------------------------------------------

#if defined(_WIN32)
#  include <io.h>
#  define bench_isatty(fd) _isatty(fd)
#  define bench_fileno(f)  _fileno(f)
#else
#  include <unistd.h>
#  define bench_isatty(fd) isatty(fd)
#  define bench_fileno(f)  fileno(f)
#endif

static uint64_t bench_now_ns(void)
{
#if defined(TIME_UTC) && !defined(__APPLE__)
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#else
    // Fallback: clock() has lower resolution but is portable.
    clock_t c = clock();
    if (c == (clock_t)-1) return 0;
    double sec = (double)c / (double)CLOCKS_PER_SEC;
    return (uint64_t)(sec * 1000000000.0);
#endif
}

static void bench_local_time(time_t t, struct tm* out)
{
#if defined(_WIN32)
    localtime_s(out, &t);
#else
    localtime_r(&t, out);
#endif
}

static int bench_str_ieq(const char* a, const char* b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        char ca = *a++;
        char cb = *b++;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
    }
    return (*a == 0 && *b == 0);
}

static int bench_parse_boolish(const char* s, int deflt)
{
    if (!s || !*s) return deflt;
    if (bench_str_ieq(s, "1") || bench_str_ieq(s, "true") || bench_str_ieq(s, "yes") || bench_str_ieq(s, "on")) return 1;
    if (bench_str_ieq(s, "0") || bench_str_ieq(s, "false") || bench_str_ieq(s, "no") || bench_str_ieq(s, "off")) return 0;
    if (bench_str_ieq(s, "auto")) return -1;
    return deflt;
}

// -----------------------------------------------------------------------------
// Logger state
// -----------------------------------------------------------------------------

typedef struct bench_logger {
    FILE* out;
    bench_log_level_t level;
    int use_color; // -1 auto
    int quiet;
    uint64_t t0_ns;
} bench_logger_t;

static bench_logger_t g_log = {
    .out = NULL,
    .level = BENCH_LOG_INFO,
    .use_color = -1,
    .quiet = 0,
    .t0_ns = 0,
};

static const char* bench_level_name(bench_log_level_t lvl)
{
    switch (lvl) {
        case BENCH_LOG_TRACE: return "TRACE";
        case BENCH_LOG_DEBUG: return "DEBUG";
        case BENCH_LOG_INFO:  return "INFO";
        case BENCH_LOG_WARN:  return "WARN";
        case BENCH_LOG_ERROR: return "ERROR";
        case BENCH_LOG_FATAL: return "FATAL";
        case BENCH_LOG_OFF:   return "OFF";
        default:              return "?";
    }
}

const char* bench_log_level_name(bench_log_level_t lvl)
{
    return bench_level_name(lvl);
}

bench_log_level_t bench_log_level_parse(const char* s, bench_log_level_t deflt)
{
    if (!s || !*s) return deflt;
    if (bench_str_ieq(s, "trace")) return BENCH_LOG_TRACE;
    if (bench_str_ieq(s, "debug")) return BENCH_LOG_DEBUG;
    if (bench_str_ieq(s, "info"))  return BENCH_LOG_INFO;
    if (bench_str_ieq(s, "warn") || bench_str_ieq(s, "warning")) return BENCH_LOG_WARN;
    if (bench_str_ieq(s, "error")) return BENCH_LOG_ERROR;
    if (bench_str_ieq(s, "fatal")) return BENCH_LOG_FATAL;
    if (bench_str_ieq(s, "off") || bench_str_ieq(s, "none")) return BENCH_LOG_OFF;
    return deflt;
}

static int bench_should_color(FILE* out)
{
    if (g_log.use_color == 0) return 0;
    if (g_log.use_color == 1) return 1;

    // auto
    if (!out) return 0;
    int fd = bench_fileno(out);
    if (fd < 0) return 0;
    if (!bench_isatty(fd)) return 0;

    const char* env = getenv("NO_COLOR");
    if (env && *env) return 0;

    return 1;
}

static const char* bench_color_prefix(bench_log_level_t lvl)
{
    // ANSI SGR codes
    //   - TRACE/DEBUG: dim
    //   - INFO: default
    //   - WARN: yellow
    //   - ERROR/FATAL: red
    switch (lvl) {
        case BENCH_LOG_TRACE: return "\x1b[2m";        // dim
        case BENCH_LOG_DEBUG: return "\x1b[2m";        // dim
        case BENCH_LOG_INFO:  return "\x1b[0m";        // reset
        case BENCH_LOG_WARN:  return "\x1b[33m";       // yellow
        case BENCH_LOG_ERROR: return "\x1b[31m";       // red
        case BENCH_LOG_FATAL: return "\x1b[1;31m";     // bold red
        default:              return "\x1b[0m";
    }
}

static const char* bench_color_reset(void)
{
    return "\x1b[0m";
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void bench_log_init(FILE* out)
{
    if (!out) out = stderr;
    g_log.out = out;
    g_log.t0_ns = bench_now_ns();

    // Apply env overrides only once at init.
    const char* lvl = getenv("BENCH_LOG_LEVEL");
    if (lvl && *lvl) {
        g_log.level = bench_log_level_parse(lvl, g_log.level);
    }

    const char* col = getenv("BENCH_LOG_COLOR");
    if (col && *col) {
        g_log.use_color = bench_parse_boolish(col, g_log.use_color);
    }

    const char* q = getenv("BENCH_LOG_QUIET");
    if (q && *q) {
        g_log.quiet = bench_parse_boolish(q, g_log.quiet) ? 1 : 0;
    }
}

void bench_log_set_level(bench_log_level_t lvl)
{
    g_log.level = lvl;
}

bench_log_level_t bench_log_get_level(void)
{
    return g_log.level;
}

void bench_log_set_color(int enabled)
{
    // -1 auto, 0 off, 1 on
    g_log.use_color = (enabled < 0) ? -1 : (enabled ? 1 : 0);
}

void bench_log_set_quiet(int quiet)
{
    g_log.quiet = quiet ? 1 : 0;
}

static void bench_log_prefix(FILE* out, bench_log_level_t lvl, const char* file, int line)
{
    // Timestamp: wall-clock HH:MM:SS.mmm and elapsed ms since init.
    time_t now_s = time(NULL);
    struct tm tmv;
    bench_local_time(now_s, &tmv);

    uint64_t now_ns = bench_now_ns();
    uint64_t dt_ns = (g_log.t0_ns == 0 || now_ns < g_log.t0_ns) ? 0 : (now_ns - g_log.t0_ns);
    uint64_t dt_ms = dt_ns / 1000000ull;

    // Best effort milliseconds in wall-clock: use dt_ns remainder if timespec unavailable.
    unsigned ms = (unsigned)((dt_ns / 1000000ull) % 1000ull);

    (void)ms;

    // Output format:
    //   12:34:56 +1234ms [INFO ] file.c:123: message
    fprintf(out,
            "%02d:%02d:%02d +%" PRIu64 "ms [% -5s] %s:%d: ",
            tmv.tm_hour, tmv.tm_min, tmv.tm_sec,
            dt_ms,
            bench_level_name(lvl),
            file ? file : "?", line);
}

void bench_vlogf(bench_log_level_t lvl, const char* file, int line, const char* func,
                 const char* fmt, va_list ap)
{
    (void)func;

    if (g_log.quiet) return;
    if (lvl < g_log.level) return;
    if (g_log.level == BENCH_LOG_OFF) return;

    FILE* out = g_log.out ? g_log.out : stderr;

    const int use_color = bench_should_color(out);
    if (use_color) fputs(bench_color_prefix(lvl), out);

    bench_log_prefix(out, lvl, file, line);

    vfprintf(out, fmt, ap);

    if (use_color) fputs(bench_color_reset(), out);

    fputc('\n', out);

    if (lvl >= BENCH_LOG_WARN) {
        fflush(out);
    }

    if (lvl == BENCH_LOG_FATAL) {
        fflush(out);
        abort();
    }
}

void bench_logf(bench_log_level_t lvl, const char* file, int line, const char* func,
                const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    bench_vlogf(lvl, file, line, func, fmt, ap);
    va_end(ap);
}

// -----------------------------------------------------------------------------
// Optional: a tiny printf-like helper for benches that don't carry file/line
// -----------------------------------------------------------------------------

void bench_log_msg(bench_log_level_t lvl, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    bench_vlogf(lvl, NULL, 0, NULL, fmt, ap);
    va_end(ap);
}

// -----------------------------------------------------------------------------
// Debug init (kept for symmetry with other bench modules)
// -----------------------------------------------------------------------------

void bench_log_debug_init(void)
{
#if !defined(NDEBUG)
    if (!g_log.out) {
        bench_log_init(stderr);
    }
#endif
}
