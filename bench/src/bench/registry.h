#pragma once

/*
  registry.h

  Benchmark registry interface.

  The registry is a simple, process-local store of benchmark cases.
  It is optimized for benchmark harness usage (single-threaded registration
  during startup, read-only iteration during execution).

  Conventions:
    - Return 0 on success.
    - Return negative on error:
        -1 = invalid arguments
        -2 = out of memory
        -3 = duplicate id
*/

#include <stdint.h>

#include "bench/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bench_case
{
    const char* id;   /* stable, owned by registry (duplicated on add) */
    int kind;         /* runner-defined kind/category */
    bench_fn_t fn;    /* benchmark entry */
    void* ctx;        /* user context */
} bench_case_t;

/* Add a benchmark case to the registry.
   The registry duplicates `id` internally (caller may free its original).
*/
int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);

/* Number of registered benchmark cases. */
int32_t bench_registry_count(void);

/* Get case by index in [0..count). Returns NULL if out of range. */
const bench_case_t* bench_registry_get(int32_t index);

/* Find case by id. Returns index or -1 if not found. */
int32_t bench_registry_find(const char* id);

/* Remove all cases but keep allocated capacity. */
void bench_registry_clear(void);

/* Free all internal storage (including duplicated ids). */
void bench_registry_shutdown(void);

/* Iterate over cases; if `it` returns non-zero, iteration stops and that
   value is returned. Returns 0 if fully iterated.
*/
typedef int (*bench_registry_iter_fn)(const bench_case_t* c, void* user);
int bench_registry_foreach(bench_registry_iter_fn it, void* user);

#ifdef __cplusplus
}
#endif
