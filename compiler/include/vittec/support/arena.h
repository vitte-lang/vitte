#ifndef VITTEC_INCLUDE_VITTEC_SUPPORT_ARENA_H
    #define VITTEC_INCLUDE_VITTEC_SUPPORT_ARENA_H

    #include <stdint.h>

    typedef struct vittec_arena {
  unsigned char* base;
  uint64_t cap;
  uint64_t off;
} vittec_arena_t;

void vittec_arena_init(vittec_arena_t* a, uint64_t cap);
void vittec_arena_free(vittec_arena_t* a);

void* vittec_arena_alloc(vittec_arena_t* a, uint64_t size, uint64_t align);

    #endif /* VITTEC_INCLUDE_VITTEC_SUPPORT_ARENA_H */
