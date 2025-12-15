#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  const char* ptr;
  size_t len;
} steel_str;

static inline steel_str steel_str_from_c(const char* c) {
  steel_str s; s.ptr = c; s.len = 0;
  while (c && c[s.len]) s.len++;
  return s;
}

static inline int steel_str_eq(steel_str a, steel_str b) {
  if (a.len != b.len) return 0;
  for (size_t i = 0; i < a.len; i++) if (a.ptr[i] != b.ptr[i]) return 0;
  return 1;
}
