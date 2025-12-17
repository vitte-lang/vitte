// alloc.h — allocation benchmarks API (C17)
//
// Public, project-agnostic interface for alloc.c.
// Include this from your bench runner to register / invoke allocation cases.
//
// Standalone mode is implemented in alloc.c when compiled with:
//   -DVITTE_BENCH_STANDALONE

#ifndef VITTE_BENCH_ALLOC_H
#define VITTE_BENCH_ALLOC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_bm_result {
  uint64_t elapsed_ns; // wall time, nanoseconds
  uint64_t checksum;   // anti-DCE checksum
  uint64_t iters;      // iterations requested
  size_t   size;       // size parameter passed to the case
} vitte_bm_result;

typedef vitte_bm_result (*vitte_bm_fn)(uint64_t iters, size_t size, uint64_t seed);

typedef struct vitte_bm_case {
  const char* name;
  vitte_bm_fn fn;
} vitte_bm_case;

const vitte_bm_case* vitte_bm_alloc_list_cases(size_t* out_count);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_BENCH_ALLOC_H