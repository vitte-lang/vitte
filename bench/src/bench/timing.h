#pragma once

/*
  timing.h

  Advanced timing utilities for benchmarking.
*/

#ifndef VITTE_BENCH_TIMING_H
#define VITTE_BENCH_TIMING_H

#include <stdint.h>
#include <stdbool.h>
#include "bench/platform.h"

/* Forward declaration */
uint64_t bench_now_ns(void);

/* Timing result structure */
typedef struct {
  uint64_t start_ns;
  uint64_t end_ns;
  uint64_t elapsed_ns;
} bench_timing_t;

/* Timer state for scoped measurements */
typedef struct {
  uint64_t start;
  uint64_t accumulated;
  bool running;
} bench_timer_t;

/* Start timing */
BENCH_INLINE bench_timing_t bench_time_start(void) {
  bench_timing_t t = {0};
  t.start_ns = bench_now_ns();
  return t;
}

/* End timing and compute elapsed */
BENCH_INLINE uint64_t bench_time_end(bench_timing_t* t) {
  t->end_ns = bench_now_ns();
  t->elapsed_ns = t->end_ns - t->start_ns;
  return t->elapsed_ns;
}

/* Create a new scoped timer */
bench_timer_t bench_timer_new(void);

/* Start timer */
void bench_timer_start(bench_timer_t* timer);

/* Stop timer and accumulate */
uint64_t bench_timer_stop(bench_timer_t* timer);

/* Get accumulated time */
uint64_t bench_timer_elapsed(const bench_timer_t* timer);

/* Reset timer */
void bench_timer_reset(bench_timer_t* timer);

#endif /* VITTE_BENCH_TIMING_H */
