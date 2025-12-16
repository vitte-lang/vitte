// File: bench/src/bench/bench_time.c
// Vitte benchmark time utilities — "max" (C17/C23).
//
// Fournit :
//   - bench_clock_init() / bench_now_ns(): horloge monotone haute résolution
//   - bench_sleep_ms(): sleep portable
//   - bench_estimate_timer_overhead_ns(): estimation overhead timer
//   - bench_format_duration(): formatage ns/us/ms/s
//   - bench_cpu_logical_count(): nombre de CPU logiques (best-effort)
//   - bench_pin_to_single_cpu(): pinning affinity (Windows/Linux best-effort)
//   - bench_spin_wait_ns(): attente active (spin)
//
// Conçu pour être utilisé par bench_main.c.
//
// Notes :
//   - Sur Windows : QueryPerformanceCounter / SetThreadAffinityMask.
//   - Sur POSIX : clock_gettime(CLOCK_MONOTONIC[_RAW]) sinon timespec_get fallback.
//   - macOS: clock_gettime ok (10.12+). Pinning non exposé via API publique standard -> false.
//   - Sur Linux old glibc: clock_gettime pouvait nécessiter -lrt, mais plus généralement plus nécessaire.
//
// API (bench/bench.h) :
//   typedef struct bench_clock bench_clock_t;
//   bench_clock_t bench_clock_init(void);
//   uint64_t bench_now_ns(const bench_clock_t* c);
//   void bench_sleep_ms(uint32_t ms);
//   uint64_t bench_estimate_timer_overhead_ns(const bench_clock_t* c, int iters);
//   const char* bench_format_duration(char* buf, size_t buf_len, uint64_t ns);
//   uint32_t bench_cpu_logical_count(void);
//   bool bench_pin_to_single_cpu(uint32_t cpu_index);
//   void bench_spin_wait_ns(const bench_clock_t* c, uint64_t ns);

#include "bench/bench.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <time.h>
  #include <errno.h>
  #include <unistd.h>
  #if defined(__linux__)
    #include <sched.h>
  #endif
  #if defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
  #endif
#endif

// ======================================================================================
// Internal helpers
// ======================================================================================

static uint64_t bench_timespec_to_ns(struct timespec ts) {
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint64_t bench_saturating_sub_u64(uint64_t a, uint64_t b) {
  return (a > b) ? (a - b) : 0;
}

// ======================================================================================
// Public: init + now
// ======================================================================================

bench_clock_t bench_clock_init(void) {
  bench_clock_t c;
#if defined(_WIN32)
  QueryPerformanceFrequency(&c.qpc_freq);
#else
  c.has_clock_gettime = 0;
  c.clock_id = (clockid_t)0;

  // Prefer MONOTONIC_RAW when present (Linux), else MONOTONIC.
  #if defined(CLOCK_MONOTONIC_RAW)
    c.clock_id = CLOCK_MONOTONIC_RAW;
    c.has_clock_gettime = 1;
  #elif defined(CLOCK_MONOTONIC)
    c.clock_id = CLOCK_MONOTONIC;
    c.has_clock_gettime = 1;
  #else
    c.has_clock_gettime = 0;
  #endif

  // Runtime validation (defensive)
  #if defined(CLOCK_MONOTONIC_RAW) || defined(CLOCK_MONOTONIC)
    if (c.has_clock_gettime) {
      struct timespec ts;
      if (clock_gettime(c.clock_id, &ts) != 0) {
        c.has_clock_gettime = 0;
      }
    }
  #endif
#endif
  return c;
}

uint64_t bench_now_ns(const bench_clock_t* c) {
#if defined(_WIN32)
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);

  // ns = (t * 1e9) / freq; use long double to reduce overflow risk.
  // (freq is stable; QPC is monotonic)
  long double ns = ((long double)t.QuadPart * 1000000000.0L) / (long double)c->qpc_freq.QuadPart;
  return (uint64_t)ns;
#else
  if (c && c->has_clock_gettime) {
    struct timespec ts;
    if (clock_gettime(c->clock_id, &ts) == 0) {
      return bench_timespec_to_ns(ts);
    }
  }

  // Fallback: C11 timespec_get (not monotonic, but always available)
  struct timespec ts2;
  timespec_get(&ts2, TIME_UTC);
  return bench_timespec_to_ns(ts2);
#endif
}

// ======================================================================================
// Sleep
// ======================================================================================

void bench_sleep_ms(uint32_t ms) {
#if defined(_WIN32)
  Sleep((DWORD)ms);
#else
  struct timespec req;
  req.tv_sec = (time_t)(ms / 1000U);
  req.tv_nsec = (long)((ms % 1000U) * 1000000UL);

  while (nanosleep(&req, &req) != 0) {
    if (errno == EINTR) continue;
    break;
  }
#endif
}

// ======================================================================================
// Timer overhead estimation
// ======================================================================================

uint64_t bench_estimate_timer_overhead_ns(const bench_clock_t* c, int iters) {
  if (iters <= 0) iters = 20000;
  const int runs = 7;

  uint64_t best_total = UINT64_MAX;

  for (int r = 0; r < runs; r++) {
    uint64_t t0 = bench_now_ns(c);
    for (int i = 0; i < iters; i++) {
      (void)bench_now_ns(c);
    }
    uint64_t t1 = bench_now_ns(c);
    uint64_t dt = t1 - t0;
    if (dt < best_total) best_total = dt;
  }

  if (best_total == UINT64_MAX) return 0;
  return best_total / (uint64_t)iters;
}

// ======================================================================================
// Duration formatting
// ======================================================================================

const char* bench_format_duration(char* buf, size_t buf_len, uint64_t ns) {
  if (!buf || buf_len == 0) return "";

  if (ns < 1000ULL) {
#if defined(_MSC_VER)
    _snprintf_s(buf, buf_len, _TRUNCATE, "%lluns", (unsigned long long)ns);
#else
    snprintf(buf, buf_len, "%lluns", (unsigned long long)ns);
#endif
    return buf;
  }

  if (ns < 1000ULL * 1000ULL) {
    double us = (double)ns / 1000.0;
#if defined(_MSC_VER)
    _snprintf_s(buf, buf_len, _TRUNCATE, "%.3fus", us);
#else
    snprintf(buf, buf_len, "%.3fus", us);
#endif
    return buf;
  }

  if (ns < 1000ULL * 1000ULL * 1000ULL) {
    double ms = (double)ns / 1000000.0;
#if defined(_MSC_VER)
    _snprintf_s(buf, buf_len, _TRUNCATE, "%.3fms", ms);
#else
    snprintf(buf, buf_len, "%.3fms", ms);
#endif
    return buf;
  }

  double s = (double)ns / 1000000000.0;
#if defined(_MSC_VER)
  _snprintf_s(buf, buf_len, _TRUNCATE, "%.6fs", s);
#else
  snprintf(buf, buf_len, "%.6fs", s);
#endif
  return buf;
}

// ======================================================================================
// CPU info
// ======================================================================================

uint32_t bench_cpu_logical_count(void) {
#if defined(_WIN32)
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  if (si.dwNumberOfProcessors == 0) return 1;
  return (uint32_t)si.dwNumberOfProcessors;
#elif defined(__APPLE__)
  int nm[2] = { CTL_HW, HW_NCPU };
  int ncpu = 0;
  size_t len = sizeof(ncpu);
  if (sysctl(nm, 2, &ncpu, &len, NULL, 0) == 0 && ncpu > 0) return (uint32_t)ncpu;
  return 1;
#else
  long n = sysconf(_SC_NPROCESSORS_ONLN);
  if (n <= 0) return 1;
  return (uint32_t)n;
#endif
}

// ======================================================================================
// Pinning to a single CPU (best-effort)
// ======================================================================================

bool bench_pin_to_single_cpu(uint32_t cpu_index) {
#if defined(_WIN32)
  if (cpu_index >= 64) return false;
  DWORD_PTR mask = ((DWORD_PTR)1) << cpu_index;
  HANDLE th = GetCurrentThread();
  DWORD_PTR prev = SetThreadAffinityMask(th, mask);
  return (prev != 0);

#elif defined(__linux__)
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET((int)cpu_index, &set);
  int rc = sched_setaffinity(0, sizeof(set), &set);
  return rc == 0;

#else
  (void)cpu_index;
  // macOS / others: no stable public C API in standard headers here.
  return false;
#endif
}

// ======================================================================================
// Spin wait (busy loop) for sub-millisecond stabilization
// ======================================================================================

void bench_spin_wait_ns(const bench_clock_t* c, uint64_t ns) {
  if (ns == 0) return;
  uint64_t start = bench_now_ns(c);
  while (bench_saturating_sub_u64(bench_now_ns(c), start) < ns) {
#if defined(_WIN32)
    YieldProcessor();
#else
    __asm__ __volatile__("" ::: "memory");
#endif
  }
}
