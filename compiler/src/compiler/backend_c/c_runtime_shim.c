// SPDX-License-Identifier: MIT
// c_runtime_shim.c
//
// C backend runtime shim.
//
// This file provides a minimal ABI layer between generated C code and the
// vitte runtime (or "steel" core). It is designed to be linked into all
// C backend outputs.
//
// Goals:
//  - Stable, small API surface for generated code
//  - Basic panic/assert
//  - Memory allocation wrappers
//  - Byte slice and string views
//  - Minimal printing (debug)
//  - Optional checked arithmetic helpers
//  - Platform-independent defines
//
// Pairing:
//  - c_runtime_shim.h should declare exported symbols used by generated code.
//  - If the project already has runtime equivalents, keep the names stable.

#include "c_runtime_shim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <unistd.h>
#endif

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#ifndef VITTE_SHIM_ABORT_ON_PANIC
  #define VITTE_SHIM_ABORT_ON_PANIC 1
#endif

#ifndef VITTE_SHIM_USE_STDIO
  #define VITTE_SHIM_USE_STDIO 1
#endif

#ifndef VITTE_SHIM_MEMSET_ON_ALLOC
  #define VITTE_SHIM_MEMSET_ON_ALLOC 0
#endif

#ifndef VITTE_SHIM_PRINTF_MAX
  #define VITTE_SHIM_PRINTF_MAX 4096
#endif

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

static void vitte_shim_write_stderr(const char* s)
{
#if VITTE_SHIM_USE_STDIO
    fputs(s ? s : "", stderr);
    fflush(stderr);
#else
    if (!s) return;
  #if defined(_WIN32)
    (void)WriteFile(GetStdHandle(STD_ERROR_HANDLE), s, (DWORD)strlen(s), NULL, NULL);
  #else
    (void)write(2, s, strlen(s));
  #endif
#endif
}

static void vitte_shim_write_stdout(const char* s)
{
#if VITTE_SHIM_USE_STDIO
    fputs(s ? s : "", stdout);
    fflush(stdout);
#else
    if (!s) return;
  #if defined(_WIN32)
    (void)WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s, (DWORD)strlen(s), NULL, NULL);
  #else
    (void)write(1, s, strlen(s));
  #endif
#endif
}

static void vitte_shim_vsnprintf(char* out, size_t cap, const char* fmt, va_list ap)
{
    if (!out || cap == 0) return;
    if (!fmt) fmt = "";

#if VITTE_SHIM_USE_STDIO
    (void)vsnprintf(out, cap, fmt, ap);
    out[cap - 1] = 0;
#else
    // Minimal fallback: no formatting.
    (void)ap;
    strncpy(out, fmt, cap - 1);
    out[cap - 1] = 0;
#endif
}

//------------------------------------------------------------------------------
// Panic / assert
//------------------------------------------------------------------------------

void vitte_rt_panic(const char* msg)
{
    vitte_shim_write_stderr("[panic] ");
    vitte_shim_write_stderr(msg ? msg : "(null)");
    vitte_shim_write_stderr("\n");

#if VITTE_SHIM_ABORT_ON_PANIC
    abort();
#else
    // If configured not to abort, still exit with failure.
    exit(1);
#endif
}

void vitte_rt_panicf(const char* fmt, ...)
{
    char buf[VITTE_SHIM_PRINTF_MAX];

    va_list ap;
    va_start(ap, fmt);
    vitte_shim_vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    vitte_rt_panic(buf);
}

void vitte_rt_assert(bool cond, const char* msg)
{
    if (cond) return;
    vitte_rt_panic(msg ? msg : "assertion failed");
}

//------------------------------------------------------------------------------
// Allocation
//------------------------------------------------------------------------------

void* vitte_rt_alloc(size_t n)
{
    if (n == 0) n = 1;
    void* p = malloc(n);
    if (!p)
        vitte_rt_panic("out of memory");

#if VITTE_SHIM_MEMSET_ON_ALLOC
    memset(p, 0, n);
#endif

    return p;
}

void* vitte_rt_calloc(size_t n, size_t sz)
{
    if (n == 0) n = 1;
    if (sz == 0) sz = 1;

    void* p = calloc(n, sz);
    if (!p)
        vitte_rt_panic("out of memory");
    return p;
}

void* vitte_rt_realloc(void* p, size_t n)
{
    if (n == 0) n = 1;
    void* q = realloc(p, n);
    if (!q)
        vitte_rt_panic("out of memory");
    return q;
}

void vitte_rt_free(void* p)
{
    free(p);
}

//------------------------------------------------------------------------------
// Byte slice / string view (ABI-friendly)
//------------------------------------------------------------------------------

vitte_bytes vitte_bytes_make(const void* data, size_t len)
{
    vitte_bytes b;
    b.data = (const uint8_t*)data;
    b.len = len;
    return b;
}

vitte_str vitte_str_make(const char* data, size_t len)
{
    vitte_str s;
    s.data = data;
    s.len = len;
    return s;
}

vitte_str vitte_str_from_cstr(const char* cstr)
{
    vitte_str s;
    s.data = cstr ? cstr : "";
    s.len = cstr ? strlen(cstr) : 0;
    return s;
}

bool vitte_str_eq(vitte_str a, vitte_str b)
{
    if (a.len != b.len) return false;
    if (a.data == b.data) return true;
    if (a.len == 0) return true;
    return memcmp(a.data, b.data, a.len) == 0;
}

//------------------------------------------------------------------------------
// Debug printing
//------------------------------------------------------------------------------

void vitte_rt_print_str(vitte_str s)
{
    if (!s.data || s.len == 0) return;

#if VITTE_SHIM_USE_STDIO
    fwrite(s.data, 1, s.len, stdout);
    fflush(stdout);
#else
  #if defined(_WIN32)
    DWORD written = 0;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s.data, (DWORD)s.len, &written, NULL);
  #else
    (void)write(1, s.data, s.len);
  #endif
#endif
}

void vitte_rt_println_str(vitte_str s)
{
    vitte_rt_print_str(s);
    vitte_shim_write_stdout("\n");
}

void vitte_rt_eprint_str(vitte_str s)
{
    if (!s.data || s.len == 0) return;

#if VITTE_SHIM_USE_STDIO
    fwrite(s.data, 1, s.len, stderr);
    fflush(stderr);
#else
  #if defined(_WIN32)
    DWORD written = 0;
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), s.data, (DWORD)s.len, &written, NULL);
  #else
    (void)write(2, s.data, s.len);
  #endif
#endif
}

void vitte_rt_eprintln_str(vitte_str s)
{
    vitte_rt_eprint_str(s);
    vitte_shim_write_stderr("\n");
}

void vitte_rt_print_u64(uint64_t v)
{
    char buf[64];
#if VITTE_SHIM_USE_STDIO
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
#else
    // minimal u64 -> string
    char tmp[64];
    size_t n = 0;
    if (v == 0) tmp[n++] = '0';
    while (v && n < sizeof(tmp))
    {
        tmp[n++] = (char)('0' + (v % 10));
        v /= 10;
    }
    // reverse
    for (size_t i = 0; i < n; i++) buf[i] = tmp[n - 1 - i];
    buf[n] = 0;
#endif
    vitte_shim_write_stdout(buf);
}

void vitte_rt_print_i64(int64_t v)
{
    if (v < 0)
    {
        vitte_shim_write_stdout("-");
        // careful with INT64_MIN
        uint64_t u = (uint64_t)(-(v + 1)) + 1;
        vitte_rt_print_u64(u);
        return;
    }
    vitte_rt_print_u64((uint64_t)v);
}

//------------------------------------------------------------------------------
// Checked arithmetic helpers (optional)
//------------------------------------------------------------------------------

bool vitte_rt_add_u64_checked(uint64_t a, uint64_t b, uint64_t* out)
{
#if defined(__has_builtin)
  #if __has_builtin(__builtin_add_overflow)
    return !__builtin_add_overflow(a, b, out);
  #endif
#endif
    uint64_t r = a + b;
    if (r < a) return false;
    if (out) *out = r;
    return true;
}

bool vitte_rt_sub_u64_checked(uint64_t a, uint64_t b, uint64_t* out)
{
    if (b > a) return false;
    if (out) *out = a - b;
    return true;
}

bool vitte_rt_mul_u64_checked(uint64_t a, uint64_t b, uint64_t* out)
{
#if defined(__has_builtin)
  #if __has_builtin(__builtin_mul_overflow)
    return !__builtin_mul_overflow(a, b, out);
  #endif
#endif
    if (a == 0 || b == 0)
    {
        if (out) *out = 0;
        return true;
    }
    if (UINT64_MAX / a < b) return false;
    if (out) *out = a * b;
    return true;
}

//------------------------------------------------------------------------------
// Error helpers
//------------------------------------------------------------------------------

int vitte_rt_errno(void)
{
    return errno;
}

const char* vitte_rt_strerror(int err)
{
#if VITTE_SHIM_USE_STDIO
    return strerror(err);
#else
    (void)err;
    return "error";
#endif
}
