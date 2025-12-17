// bench_registry.h — benchmark registry API (C17)

#ifndef VITTE_BENCH_REGISTRY_H
#define VITTE_BENCH_REGISTRY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Runner-facing result type.
typedef struct bench_result {
  uint64_t elapsed_ns;
  uint64_t checksum;
  uint64_t iters;
  size_t   size;
  uint64_t cycles; // optional
} bench_result;

// Runner-facing benchmark function signature.
typedef bench_result (*bench_fn)(uint64_t iters, size_t size, uint64_t seed);

// A benchmark case registered in the global registry.
typedef struct bench_case {
  const char* suite;
  const char* name;
  bench_fn     fn;
  uint32_t     flags;
} bench_case;

// Initialize the registry (idempotent).
void bench_registry_init(void);

// Get the full list of benchmark cases.
// Returns a pointer to a process-lifetime array.
const bench_case* bench_registry_list(size_t* out_count);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_BENCH_REGISTRY_H
