

#include "bench/bench.h"

#include <stdint.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#elif defined(__APPLE__) && defined(__MACH__)
  #include <mach/mach_time.h>
#else
  #include <time.h>
#endif

/*
  bench_time.c (max)

  Goal:
  - Provide a fast, monotonic nanosecond clock suitable for benchmarking.

  Strategy:
  - Windows: QueryPerformanceCounter / QueryPerformanceFrequency
  - macOS:  mach_absolute_time + timebase
  - POSIX:  clock_gettime(CLOCK_MONOTONIC)

  Fallbacks:
  - POSIX without clock_gettime: timespec_get (C11) or clock()
*/

uint64_t bench_now_ns(void) {
#if defined(_WIN32)
  static LARGE_INTEGER freq = {0};
  if(freq.QuadPart == 0) {
    QueryPerformanceFrequency(&freq);
    if(freq.QuadPart == 0) {
      /* Should never happen, but avoid divide-by-zero */
      return 0;
    }
  }

  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);

  /* Convert ticks -> ns with minimal precision loss:
     ns = counter * 1e9 / freq
  */
  const uint64_t c = (uint64_t)counter.QuadPart;
  const uint64_t f = (uint64_t)freq.QuadPart;
  return (c * 1000000000ull) / f;

#elif defined(__APPLE__) && defined(__MACH__)
  static mach_timebase_info_data_t tb = {0, 0};
  if(tb.denom == 0) {
    (void)mach_timebase_info(&tb);
    if(tb.denom == 0) return 0;
  }

  const uint64_t t = mach_absolute_time();
  /* ns = t * numer / denom */
  return (t * (uint64_t)tb.numer) / (uint64_t)tb.denom;

#else
  /* Prefer CLOCK_MONOTONIC */
  #if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
      return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    }
  #endif

  /* C11 fallback */
  #if defined(TIME_UTC)
    struct timespec ts2;
    if(timespec_get(&ts2, TIME_UTC) == TIME_UTC) {
      return (uint64_t)ts2.tv_sec * 1000000000ull + (uint64_t)ts2.tv_nsec;
    }
  #endif

  /* Last resort: clock() (CPU time, not ideal for wall-clock bench) */
  return (uint64_t)clock() * (1000000000ull / (uint64_t)CLOCKS_PER_SEC);
#endif
}