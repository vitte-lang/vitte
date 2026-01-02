/*
  bench_time.c - portable high-resolution timer
*/
#include "bench/bench.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
double bench_now_s(void) {
  static LARGE_INTEGER freq;
  static int init = 0;
  if (!init) {
    QueryPerformanceFrequency(&freq);
    init = 1;
  }
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  return (double)t.QuadPart / (double)freq.QuadPart;
}
#else
#include <time.h>
double bench_now_s(void) {
#if defined(CLOCK_MONOTONIC)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#else
  return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}
#endif
