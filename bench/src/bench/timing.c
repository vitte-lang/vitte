#include "bench/timing.h"
#include "bench/bench.h"

/*
  timing.c

  Advanced timing utilities implementation.
*/

bench_timer_t bench_timer_new(void) {
  bench_timer_t t = {
    .start = 0,
    .accumulated = 0,
    .running = false,
  };
  return t;
}

void bench_timer_start(bench_timer_t* timer) {
  if (timer && !timer->running) {
    timer->start = bench_now_ns();
    timer->running = true;
  }
}

uint64_t bench_timer_stop(bench_timer_t* timer) {
  if (!timer || !timer->running) {
    return 0;
  }

  uint64_t end = bench_now_ns();
  uint64_t elapsed = end - timer->start;
  timer->accumulated += elapsed;
  timer->running = false;

  return elapsed;
}

uint64_t bench_timer_elapsed(const bench_timer_t* timer) {
  if (!timer) {
    return 0;
  }

  uint64_t total = timer->accumulated;
  if (timer->running) {
    uint64_t now = bench_now_ns();
    total += (now - timer->start);
  }

  return total;
}

void bench_timer_reset(bench_timer_t* timer) {
  if (timer) {
    timer->start = 0;
    timer->accumulated = 0;
    timer->running = false;
  }
}
