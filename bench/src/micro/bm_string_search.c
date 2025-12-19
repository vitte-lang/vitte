#include "bench/bench.h"
#include <stdint.h>
#include <string.h>

/*
  bm_string_search.c (micro)

  Goal:
  - Micro-benchmark for string search algorithms.
  - Tests linear search, memchr, strstr-like patterns.
  - Measures character-by-character processing.

  Design:
  - Character finding in strings.
  - Substring matching.
  - Pattern searching with varying patterns.
*/

static const char* haystack =
  "The quick brown fox jumps over the lazy dog. "
  "The quick brown fox jumps over the lazy dog. "
  "The quick brown fox jumps over the lazy dog. "
  "The quick brown fox jumps over the lazy dog. ";

static const char* find_char(const char* s, char c) {
  while (*s) {
    if (*s == c) return s;
    s++;
  }
  return NULL;
}

static const char* find_substr(const char* haystack, const char* needle) {
  if (!*needle) return haystack;
  
  while (*haystack) {
    if (*haystack == *needle) {
      const char* h = haystack;
      const char* n = needle;
      while (*n && *h == *n) {
        h++;
        n++;
      }
      if (!*n) return haystack;
    }
    haystack++;
  }
  return NULL;
}

static int bm_string_findchar_common(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  for (int iter = 0; iter < 10000; iter++) {
    for (char c = 'a'; c <= 'z'; c++) {
      const char* found = find_char(haystack, c);
      (void)found;
    }
  }
  return 0;
}

static int bm_string_findchar_rare(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  for (int iter = 0; iter < 10000; iter++) {
    const char* found = find_char(haystack, '$');
    (void)found;
  }
  return 0;
}

static int bm_string_findsubstr_short(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  for (int iter = 0; iter < 1000; iter++) {
    const char* found = find_substr(haystack, "fox");
    (void)found;
  }
  return 0;
}

static int bm_string_findsubstr_long(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  for (int iter = 0; iter < 1000; iter++) {
    const char* found = find_substr(haystack, "brown fox jumps");
    (void)found;
  }
  return 0;
}

static int bm_string_findsubstr_notfound(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  for (int iter = 0; iter < 1000; iter++) {
    const char* found = find_substr(haystack, "xyz123");
    (void)found;
  }
  return 0;
}

void bench_register_micro_string_search(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:str_findchar_common", BENCH_MICRO, bm_string_findchar_common, NULL);
  bench_registry_add("micro:str_findchar_rare", BENCH_MICRO, bm_string_findchar_rare, NULL);
  bench_registry_add("micro:str_findsubstr_short", BENCH_MICRO, bm_string_findsubstr_short, NULL);
  bench_registry_add("micro:str_findsubstr_long", BENCH_MICRO, bm_string_findsubstr_long, NULL);
  bench_registry_add("micro:str_findsubstr_notfound", BENCH_MICRO, bm_string_findsubstr_notfound, NULL);
}
