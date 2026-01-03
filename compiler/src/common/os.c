// SPDX-License-Identifier: MIT
// os.c
//
// Minimal OS/platform helpers for vitte.
//
// Goals:
//  - Cross-platform primitives (POSIX + Windows)
//  - Low dependencies
//  - Small surface area: env, timing, sleep, cpu count, executable path
//
// This file pairs with os.h.
// os.h is expected to declare the functions implemented here.
// If your os.h differs, adjust accordingly.

#include "os.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <processthreadsapi.h>
#else
  #include <unistd.h>
  #include <errno.h>
  #include <time.h>
  #if defined(__APPLE__)
    #include <mach/mach_time.h>
    #include <sys/types.h>
    #include <sys/sysctl.h>
  #endif
#endif

#ifndef STEEL_OS_PATH_MAX
  #define STEEL_OS_PATH_MAX 4096
#endif

//------------------------------------------------------------------------------
// Environment
//------------------------------------------------------------------------------

bool steel_os_getenv(const char* key, char* out, size_t out_cap)
{
    if (!key || !out || out_cap == 0) return false;
    out[0] = 0;

#if defined(_WIN32)
    DWORD n = GetEnvironmentVariableA(key, out, (DWORD)out_cap);
    if (n == 0 || n >= (DWORD)out_cap) return false;
    return true;
#else
    const char* v = getenv(key);
    if (!v) return false;
    size_t n = strlen(v);
    if (n + 1 > out_cap) return false;
    memcpy(out, v, n + 1);
    return true;
#endif
}

bool steel_os_setenv(const char* key, const char* value)
{
    if (!key || !key[0]) return false;

#if defined(_WIN32)
    return SetEnvironmentVariableA(key, value ? value : "") != 0;
#else
    if (!value) value = "";
    return setenv(key, value, 1) == 0;
#endif
}

bool steel_os_unsetenv(const char* key)
{
    if (!key || !key[0]) return false;

#if defined(_WIN32)
    return SetEnvironmentVariableA(key, NULL) != 0;
#else
    return unsetenv(key) == 0;
#endif
}

//------------------------------------------------------------------------------
// Timing
//------------------------------------------------------------------------------

uint64_t steel_os_now_ns(void)
{
#if defined(_WIN32)
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);

    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    // (t / freq) seconds => ns
    // ns = t * 1e9 / freq
    return (uint64_t)((t.QuadPart * 1000000000ull) / (uint64_t)freq.QuadPart);
#else
  #if defined(__APPLE__)
    static mach_timebase_info_data_t tb = {0, 0};
    if (tb.denom == 0) mach_timebase_info(&tb);
    uint64_t t = mach_absolute_time();
    // Convert to ns
    return (t * (uint64_t)tb.numer) / (uint64_t)tb.denom;
  #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
  #endif
#endif
}

uint64_t steel_os_now_ms(void)
{
    return steel_os_now_ns() / 1000000ull;
}

//------------------------------------------------------------------------------
// Sleep
//------------------------------------------------------------------------------

void steel_os_sleep_ms(uint32_t ms)
{
#if defined(_WIN32)
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)((ms % 1000u) * 1000000u);
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {}
#endif
}

//------------------------------------------------------------------------------
// CPU count
//------------------------------------------------------------------------------

uint32_t steel_os_cpu_count(void)
{
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    if (info.dwNumberOfProcessors == 0) return 1;
    return (uint32_t)info.dwNumberOfProcessors;
#else
  #if defined(__APPLE__)
    int n = 0;
    size_t sz = sizeof(n);
    if (sysctlbyname("hw.ncpu", &n, &sz, NULL, 0) == 0 && n > 0)
        return (uint32_t)n;
    return 1;
  #else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n <= 0) return 1;
    return (uint32_t)n;
  #endif
#endif
}

//------------------------------------------------------------------------------
// Executable path
//------------------------------------------------------------------------------

bool steel_os_exe_path(char* out, size_t out_cap)
{
    if (!out || out_cap == 0) return false;
    out[0] = 0;

#if defined(_WIN32)
    DWORD n = GetModuleFileNameA(NULL, out, (DWORD)out_cap);
    if (n == 0 || n >= (DWORD)out_cap) return false;
    return true;
#else
  #if defined(__APPLE__)
    // _NSGetExecutablePath returns size needed if too small.
    #include <mach-o/dyld.h>
    uint32_t sz = (uint32_t)out_cap;
    int r = _NSGetExecutablePath(out, &sz);
    if (r != 0) return false;
    // Best-effort realpath to normalize.
    char real[STEEL_OS_PATH_MAX];
    if (realpath(out, real))
    {
        size_t n = strlen(real);
        if (n + 1 > out_cap) return false;
        memcpy(out, real, n + 1);
    }
    return true;
  #elif defined(__linux__)
    ssize_t n = readlink("/proc/self/exe", out, out_cap - 1);
    if (n <= 0) return false;
    out[n] = 0;
    return true;
  #else
    // Unsupported POSIX platform
    (void)out; (void)out_cap;
    return false;
  #endif
#endif
}

//------------------------------------------------------------------------------
// Path separators
//------------------------------------------------------------------------------

char steel_os_path_sep(void)
{
#if defined(_WIN32)
    return '\\';
#else
    return '/';
#endif
}

bool steel_os_is_windows(void)
{
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
}

bool steel_os_is_posix(void)
{
#if defined(_WIN32)
    return false;
#else
    return true;
#endif
}
