// SPDX-License-Identifier: MIT
// rt_panic.c
//
// Runtime panic/assert subsystem (max).
//
// Goals:
//  - Centralize fatal error handling for the runtime/VM/toolchain.
//  - Provide consistent formatting, optional backtrace, and hooks.
//  - Support "panic" (fatal) + "assert" (fatal in debug, optional in release).
//
// Design:
//  - Thread-local last panic message for diagnostics.
//  - Configurable panic handler (defaults to printing + abort).
//  - Optional backtrace:
//      * glibc: backtrace()/backtrace_symbols_fd()
//      * macOS: backtrace() is available via execinfo.
//    If unavailable, silently skips.
//
// Build toggles:
//  - Define RT_ENABLE_BACKTRACE=1 to attempt printing backtrace.
//  - Define RT_ASSERTS=0 to compile out rt_assert (except for side-effect-free checks).
//  - Define RT_PANIC_TRAP=1 to use __builtin_trap instead of abort.
//
// Integration:
//  - If `rt_panic.h` exists, include it.
//  - Otherwise provide a fallback API that can later be moved to `rt_panic.h`.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__has_include)
  #if __has_include("rt_panic.h")
    #include "rt_panic.h"
    #define RT_HAVE_RT_PANIC_H 1
  #elif __has_include("../runtime/rt_panic.h")
    #include "../runtime/rt_panic.h"
    #define RT_HAVE_RT_PANIC_H 1
  #endif
#endif

#ifndef RT_HAVE_RT_PANIC_H

typedef void (*rt_panic_handler_fn)(void* user, const char* msg);

typedef struct rt_panic_cfg
{
    rt_panic_handler_fn handler;
    void* user;
    bool print_to_stderr;
} rt_panic_cfg;

void rt_panic_cfg_init(rt_panic_cfg* cfg);
void rt_panic_set_handler(rt_panic_handler_fn fn, void* user);

void rt_panic(const char* file, int line, const char* func, const char* fmt, ...);
void rt_panic_v(const char* file, int line, const char* func, const char* fmt, va_list ap);

void rt_fail_assert(const char* file, int line, const char* func, const char* expr, const char* fmt, ...);

const char* rt_panic_last_message(void);

#endif // !RT_HAVE_RT_PANIC_H

//------------------------------------------------------------------------------
// Defaults / toggles
//------------------------------------------------------------------------------

#ifndef RT_ENABLE_BACKTRACE
  #define RT_ENABLE_BACKTRACE 1
#endif

#ifndef RT_ASSERTS
  #if defined(NDEBUG)
    #define RT_ASSERTS 0
  #else
    #define RT_ASSERTS 1
  #endif
#endif

#ifndef RT_PANIC_TRAP
  #define RT_PANIC_TRAP 0
#endif

//------------------------------------------------------------------------------
// Backtrace support (best-effort)
//------------------------------------------------------------------------------

#if RT_ENABLE_BACKTRACE
  #if defined(__has_include)
    #if __has_include(<execinfo.h>)
      #include <execinfo.h>
      #define RT_HAS_EXECINFO 1
    #else
      #define RT_HAS_EXECINFO 0
    #endif
  #else
    #define RT_HAS_EXECINFO 0
  #endif
#else
  #define RT_HAS_EXECINFO 0
#endif

static void rt_print_backtrace_(FILE* f)
{
#if RT_HAS_EXECINFO
    void* frames[64];
    int n = backtrace(frames, (int)(sizeof(frames) / sizeof(frames[0])));
    if (n <= 0)
        return;

    // backtrace_symbols_fd prints directly to fd.
    // On some platforms it prints without a header.
    fprintf(f, "Backtrace (%d frames):\n", n);

    // If backtrace_symbols_fd exists, use it.
    // It's declared in execinfo.h on glibc and many systems.
    backtrace_symbols_fd(frames, n, fileno(f));
    fprintf(f, "\n");
#else
    (void)f;
#endif
}

//------------------------------------------------------------------------------
// Global config
//------------------------------------------------------------------------------

static rt_panic_cfg g_cfg_;
static bool g_cfg_inited_ = false;

static _Thread_local char g_last_msg_[1024];

static void rt_cfg_ensure_(void)
{
    if (g_cfg_inited_)
        return;

    // default
    g_cfg_.handler = NULL;
    g_cfg_.user = NULL;
    g_cfg_.print_to_stderr = true;
    g_cfg_inited_ = true;

    g_last_msg_[0] = 0;
}

void rt_panic_cfg_init(rt_panic_cfg* cfg)
{
    if (!cfg) return;
    cfg->handler = NULL;
    cfg->user = NULL;
    cfg->print_to_stderr = true;
}

void rt_panic_set_handler(rt_panic_handler_fn fn, void* user)
{
    rt_cfg_ensure_();
    g_cfg_.handler = fn;
    g_cfg_.user = user;
}

const char* rt_panic_last_message(void)
{
    return g_last_msg_;
}

//------------------------------------------------------------------------------
// Formatting
//------------------------------------------------------------------------------

static void rt_store_last_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_last_msg_)) n = sizeof(g_last_msg_) - 1;
    memcpy(g_last_msg_, msg, n);
    g_last_msg_[n] = 0;
}

static void rt_vsnprintf_(char* out, size_t cap, const char* fmt, va_list ap)
{
    if (!out || cap == 0)
        return;

    if (!fmt) fmt = "";

    int n = vsnprintf(out, cap, fmt, ap);
    if (n < 0)
    {
        out[0] = 0;
        return;
    }

    if ((size_t)n >= cap)
        out[cap - 1] = 0;
}

static void rt_emit_(const char* header, const char* file, int line, const char* func, const char* msg)
{
    rt_cfg_ensure_();

    if (g_cfg_.handler)
    {
        // Provide full message string to handler.
        char full[1400];
        if (!header) header = "";
        if (!file) file = "?";
        if (!func) func = "?";
        if (!msg) msg = "";

        snprintf(full, sizeof(full), "%s %s:%d %s: %s", header, file, line, func, msg);
        rt_store_last_(full);
        g_cfg_.handler(g_cfg_.user, full);
        return;
    }

    // Default: stderr
    if (g_cfg_.print_to_stderr)
    {
        FILE* f = stderr;
        if (!header) header = "";
        if (!file) file = "?";
        if (!func) func = "?";
        if (!msg) msg = "";

        fprintf(f, "%s %s:%d %s: %s\n", header, file, line, func, msg);
        rt_store_last_(msg);

        rt_print_backtrace_(f);
        fflush(f);
    }
}

static void rt_terminate_(void)
{
#if RT_PANIC_TRAP
  #if defined(__has_builtin)
    #if __has_builtin(__builtin_trap)
      __builtin_trap();
    #else
      abort();
    #endif
  #else
    abort();
  #endif
#else
    abort();
#endif
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void rt_panic_v(const char* file, int line, const char* func, const char* fmt, va_list ap)
{
    char msg[1024];

    va_list aq;
    va_copy(aq, ap);
    rt_vsnprintf_(msg, sizeof(msg), fmt, aq);
    va_end(aq);

    rt_emit_("PANIC", file, line, func, msg);
    rt_terminate_();
}

void rt_panic(const char* file, int line, const char* func, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rt_panic_v(file, line, func, fmt, ap);
    va_end(ap);
}

void rt_fail_assert(const char* file, int line, const char* func, const char* expr, const char* fmt, ...)
{
#if RT_ASSERTS
    char msg[1024];
    if (!expr) expr = "<expr>";

    if (fmt && fmt[0])
    {
        // format extra details
        char extra[768];
        va_list ap;
        va_start(ap, fmt);
        rt_vsnprintf_(extra, sizeof(extra), fmt, ap);
        va_end(ap);

        snprintf(msg, sizeof(msg), "assertion failed: %s (%s)", expr, extra);
    }
    else
    {
        snprintf(msg, sizeof(msg), "assertion failed: %s", expr);
    }

    rt_emit_("ASSERT", file, line, func, msg);
    rt_terminate_();
#else
    (void)file; (void)line; (void)func; (void)expr; (void)fmt;
#endif
}

//------------------------------------------------------------------------------
// Optional macro helpers when header is missing
//------------------------------------------------------------------------------

#ifndef RT_HAVE_RT_PANIC_H

// Provide lightweight macros if the header doesn't define them.
// These macros are safe to include in C files that include rt_panic.c indirectly.

#ifndef RT_PANIC
  #define RT_PANIC(...) rt_panic(__FILE__, __LINE__, __func__, __VA_ARGS__)
#endif

#ifndef RT_ASSERT
  #if RT_ASSERTS
    #define RT_ASSERT(expr, ...) do { if (!(expr)) rt_fail_assert(__FILE__, __LINE__, __func__, #expr, __VA_ARGS__); } while (0)
  #else
    #define RT_ASSERT(expr, ...) do { (void)sizeof(expr); } while (0)
  #endif
#endif

#endif // !RT_HAVE_RT_PANIC_H
