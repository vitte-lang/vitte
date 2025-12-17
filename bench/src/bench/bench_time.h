// bench_time.h — timing utilities for benchmarking (C17)

#ifndef VITTE_BENCH_TIME_H
#define VITTE_BENCH_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t bench_time_now_ns(void);

uint64_t bench_time_cycles_now(void);
uint64_t bench_time_cycles_begin(void);
uint64_t bench_time_cycles_end(void);

uint64_t bench_time_cycles_freq_hz(void);

void bench_time_sleep_ms(uint32_t ms);
void bench_time_busy_wait_ns(uint64_t ns);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_BENCH_TIME_H