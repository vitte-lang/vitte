#ifndef VITTE_BOOTSTRAP_ARENA_POOL_H
#define VITTE_BOOTSTRAP_ARENA_POOL_H

#include <stdbool.h>
#include <stddef.h>

#include "arena.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_arena_pool_node {
    struct vitte_arena_pool_node *next;
} vitte_arena_pool_node_t;

typedef struct vitte_arena_pool {
    vitte_arena_t *arena;
    size_t object_size;
    size_t object_alignment;
    vitte_arena_pool_node_t *free_list;
    size_t allocation_count;
    size_t free_count;
    bool initialized;
} vitte_arena_pool_t;

bool vitte_arena_pool_init(
    vitte_arena_pool_t *pool,
    vitte_arena_t *arena,
    size_t object_size,
    size_t object_alignment
);

void *vitte_arena_pool_alloc(vitte_arena_pool_t *pool);
void vitte_arena_pool_free(vitte_arena_pool_t *pool, void *object);
void vitte_arena_pool_reset(vitte_arena_pool_t *pool);
bool vitte_arena_pool_is_initialized(const vitte_arena_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_ARENA_POOL_H */
