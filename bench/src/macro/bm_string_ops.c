#include "bench/bench.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/*
  bm_string_ops.c (macro)

  Goal:
  - Macro benchmark for string manipulation operations.
  - Tests concatenation, searching, replacement, case conversion.
  - Realistic text processing workloads.

  Design:
  - Build strings of varying sizes.
  - Perform find/replace operations.
  - Measure throughput of string transformations.
*/

/* String buffer for accumulating results */
typedef struct {
  char buffer[8192];
  size_t len;
} StringBuf;

static StringBuf* strbuf_new(void) {
  StringBuf* sb = (StringBuf*)malloc(sizeof(StringBuf));
  if (sb) {
    sb->len = 0;
    sb->buffer[0] = '\0';
  }
  return sb;
}

static void strbuf_append(StringBuf* sb, const char* s) {
  size_t n = strlen(s);
  if (sb->len + n < sizeof(sb->buffer)) {
    memcpy(sb->buffer + sb->len, s, n);
    sb->len += n;
    sb->buffer[sb->len] = '\0';
  }
}

static void strbuf_reset(StringBuf* sb) {
  sb->len = 0;
  sb->buffer[0] = '\0';
}

static void strbuf_free(StringBuf* sb) {
  free(sb);
}

/* String finding */
static const char* strbuf_find(StringBuf* sb, const char* needle) {
  return strstr(sb->buffer, needle);
}

/* Case conversion */
static void strbuf_toupper_inplace(StringBuf* sb) {
  for (size_t i = 0; i < sb->len; i++) {
    sb->buffer[i] = (char)toupper((unsigned char)sb->buffer[i]);
  }
}

static void strbuf_tolower_inplace(StringBuf* sb) {
  for (size_t i = 0; i < sb->len; i++) {
    sb->buffer[i] = (char)tolower((unsigned char)sb->buffer[i]);
  }
}

/* Test data */
static const char* words[] = {
  "hello", "world", "benchmark", "performance", "measurement",
  "optimization", "algorithm", "computation", "data", "structure",
  "compiler", "runtime", "system", "application", "library",
  "function", "method", "variable", "constant", "parameter",
};

static void bm_string_concat(void* ctx) {
  (void)ctx;
  
  StringBuf* sb = strbuf_new();
  if (!sb) return;
  
  for (int iter = 0; iter < 50; iter++) {
    strbuf_reset(sb);
    for (int i = 0; i < 100; i++) {
      size_t idx = i % (sizeof(words) / sizeof(words[0]));
      strbuf_append(sb, words[idx]);
      if (i < 99) strbuf_append(sb, " ");
    }
  }
  
  strbuf_free(sb);
}

static void bm_string_search(void* ctx) {
  (void)ctx;
  
  StringBuf* sb = strbuf_new();
  if (!sb) return;
  
  for (int iter = 0; iter < 50; iter++) {
    strbuf_reset(sb);
    for (int i = 0; i < 50; i++) {
      size_t idx = i % (sizeof(words) / sizeof(words[0]));
      strbuf_append(sb, words[idx]);
      strbuf_append(sb, " ");
    }
    
    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); i++) {
      const char* found = strbuf_find(sb, words[i]);
      (void)found;
    }
  }
  
  strbuf_free(sb);
}

static void bm_string_case(void* ctx) {
  (void)ctx;
  
  StringBuf* sb = strbuf_new();
  if (!sb) return;
  
  for (int iter = 0; iter < 50; iter++) {
    strbuf_reset(sb);
    for (int i = 0; i < 100; i++) {
      size_t idx = i % (sizeof(words) / sizeof(words[0]));
      strbuf_append(sb, words[idx]);
      strbuf_append(sb, " ");
    }
    
    strbuf_toupper_inplace(sb);
    strbuf_tolower_inplace(sb);
  }
  
  strbuf_free(sb);
}

void bench_register_macro_strings(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("macro:concat", BENCH_MACRO, bm_string_concat, NULL);
  bench_registry_add("macro:search", BENCH_MACRO, bm_string_search, NULL);
  bench_registry_add("macro:case", BENCH_MACRO, bm_string_case, NULL);
}
