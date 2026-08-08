#ifndef VITTE_BOOTSTRAP_ARENA_ALLOCATOR_H
#define VITTE_BOOTSTRAP_ARENA_ALLOCATOR_H

#include "arena.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_arena_allocator {
    vitte_arena_t *arena;
    vitte_allocator_t interface;
} vitte_arena_allocator_t;

bool vitte_arena_allocator_init(vitte_arena_allocator_t *allocator, vitte_arena_t *arena);
vitte_allocator_t *vitte_arena_allocator_interface(vitte_arena_allocator_t *allocator);
const vitte_allocator_t *vitte_arena_allocator_interface_const(const vitte_arena_allocator_t *allocator);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_ARENA_ALLOCATOR_H */
