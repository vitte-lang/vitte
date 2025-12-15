#include "vittec/support/arena.h"
#include "vittec/support/assert.h"
#include <stdlib.h>
#include <string.h>

static uint64_t align_up_u64(uint64_t x, uint64_t a) {
  return (x + (a - 1)) & ~(a - 1);
}

void vittec_arena_init(vittec_arena_t* a, uint64_t cap) {
  a->base = (unsigned char*)malloc((size_t)cap);
  VITTEC_ASSERT(a->base != NULL);
  a->cap = cap;
  a->off = 0;
}

void vittec_arena_free(vittec_arena_t* a) {
  if (a->base) free(a->base);
  a->base = NULL;
  a->cap = 0;
  a->off = 0;
}

void* vittec_arena_alloc(vittec_arena_t* a, uint64_t size, uint64_t align) {
  uint64_t off = align_up_u64(a->off, align ? align : 1);
  if (off + size > a->cap) return NULL;
  void* p = a->base + off;
  a->off = off + size;
  memset(p, 0, (size_t)size);
  return p;
}
