

// benchmark_init.h — optional benchmark runtime initialization (C17)

#ifndef VITTE_BENCH_BENCHMARK_INIT_H
#define VITTE_BENCH_BENCHMARK_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

// One-time (idempotent) initialization for the benchmark runtime.
void benchmark_runtime_init(void);

// Optional cleanup hook.
void benchmark_runtime_shutdown(void);

// Best-effort: pin current thread/process to a single CPU index.
// Returns 1 on success, 0 otherwise.
int benchmark_pin_to_single_cpu(int cpu_index);

// Best-effort: raise priority.
// Returns 1 on success, 0 otherwise.
int benchmark_set_high_priority(void);

// Hook placeholder: attempt to disable turbo/boost (non-portable).
// Returns 1 on success, 0 otherwise.
int benchmark_disable_turbo_best_effort(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_BENCH_BENCHMARK_INIT_H