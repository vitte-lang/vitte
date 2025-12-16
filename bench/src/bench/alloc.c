#include "bench/alloc.h"
#include <string.h>
#include <stdio.h>

/*
  alloc.c

  Memory allocation utilities with tracking.
*/

static size_t g_total_allocated = 0;
static size_t g_num_allocations = 0;

void* bench_malloc(size_t size) {
  void* ptr = malloc(size);
  if (ptr) {
    g_total_allocated += size;
    g_num_allocations++;
  }
  return ptr;
}

void* bench_calloc(size_t count, size_t size) {
  void* ptr = calloc(count, size);
  if (ptr) {
    size_t total = count * size;
    g_total_allocated += total;
    g_num_allocations++;
  }
  return ptr;
}

void bench_free(void* ptr) {
  if (ptr) {
    free(ptr);
  }
}

void* bench_realloc(void* ptr, size_t size) {
  void* new_ptr = realloc(ptr, size);
  if (new_ptr && !ptr) {
    g_total_allocated += size;
    g_num_allocations++;
  }
  return new_ptr;
}

void bench_alloc_stats(size_t* total_allocated, size_t* num_allocations) {
  if (total_allocated) *total_allocated = g_total_allocated;
  if (num_allocations) *num_allocations = g_num_allocations;
}

void bench_alloc_reset_stats(void) {
  g_total_allocated = 0;
  g_num_allocations = 0;
}
