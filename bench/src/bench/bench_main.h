// bench_main.h — benchmark runner API (C17)
//
// This header defines the common ABI between:
//   - bench_main.c (runner)
//   - bench_registry.c (registry)
//   - suite providers (alloc/micro/...): they should be adapted via registry trampolines
//
// If your project already has a central bench.h, you can merge these typedefs there.

#ifndef VITTE_BENCH_MAIN_H
#define VITTE_BENCH_MAIN_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Runner-facing result type
// -----------------------------------------------------------------------------

typedef struct bench_result {
  // Total wall time for the run in nanoseconds.
  uint64_t elapsed_ns;

  // Anti dead-code-elimination checksum (xor/accumulated by runner).
  uint64_t checksum;

  // Iterations actually executed.
  uint64_t iters;

  // Case size parameter.
  size_t size;

  // Optional cycles delta (0 if not supported / not provided).
  uint64_t cycles;
} bench_result;

// Benchmark function type.
typedef bench_result (*bench_fn)(uint64_t iters, size_t size, uint64_t seed);

// A benchmark case in the global registry.
typedef struct bench_case {
  const char* suite;  // e.g. "alloc", "micro"
  const char* name;   // e.g. "malloc_free"
  bench_fn fn;
  uint32_t flags;     // reserved
} bench_case;

// -----------------------------------------------------------------------------
// Registry API (implemented by bench_registry.c)
// -----------------------------------------------------------------------------

// Initialize the registry (idempotent).
void bench_registry_init(void);

// Get the full list of benchmark cases.
// Returns a pointer to a static / process-lifetime array.
const bench_case* bench_registry_list(size_t* out_count);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_BENCH_MAIN_H
