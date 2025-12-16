#pragma once

/*
  alloc.h

  Memory allocation utilities with error tracking.
*/

#ifndef VITTE_BENCH_ALLOC_H
#define VITTE_BENCH_ALLOC_H

#include <stddef.h>
#include <stdlib.h>

/* Allocate memory (with tracking) */
void* bench_malloc(size_t size);

/* Allocate and zero memory */
void* bench_calloc(size_t count, size_t size);

/* Free memory */
void bench_free(void* ptr);

/* Reallocate memory */
void* bench_realloc(void* ptr, size_t size);

/* Get allocation statistics */
void bench_alloc_stats(size_t* total_allocated, size_t* num_allocations);

/* Reset tracking */
void bench_alloc_reset_stats(void);

#endif /* VITTE_BENCH_ALLOC_H */
