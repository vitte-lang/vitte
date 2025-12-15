#include "bench/bench.h"
#include <time.h>

uint64_t bench_now_ns(void) {
#if defined(_WIN32)
  /* Template: for Windows, swap to QueryPerformanceCounter in your core repo */
  return (uint64_t)clock() * (1000000000ull / (uint64_t)CLOCKS_PER_SEC);
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}
