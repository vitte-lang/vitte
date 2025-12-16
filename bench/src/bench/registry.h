#pragma once

/*
  registry.h

  Benchmark registry interface for adding cases dynamically.
*/

#include "bench/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Add a benchmark case to the registry. */
int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);

#ifdef __cplusplus
}
#endif
