#include "pool.h"

#include <string.h>

bool vitte_arena_pool_init(
    vitte_arena_pool_t *pool,
    vitte_arena_t *arena,
    size_t object_size,
    size_t object_alignment
) {
    if (pool == NULL || !vitte_arena_is_initialized(arena) || object_size == 0u) {
        return false;
    }

    if (object_size < sizeof(vitte_arena_pool_node_t)) {
        object_size = sizeof(vitte_arena_pool_node_t);
    }
    if (object_alignment == 0u) {
        object_alignment = VITTE_ARENA_DEFAULT_ALIGNMENT;
    }
    if (!vitte_arena_alignment_is_valid(object_alignment)) {
        vitte_arena_set_error(arena, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_ARENA_POOL_E_ALIGNMENT", "invalid pool object alignment", NULL);
        return false;
    }

    memset(pool, 0, sizeof(*pool));
    pool->arena = arena;
    pool->object_size = object_size;
    pool->object_alignment = object_alignment;
    pool->initialized = true;
    return true;
}

void *vitte_arena_pool_alloc(vitte_arena_pool_t *pool) {
    void *object;

    if (!vitte_arena_pool_is_initialized(pool)) {
        return NULL;
    }

    if (pool->free_list != NULL) {
        object = pool->free_list;
        pool->free_list = pool->free_list->next;
    } else {
        object = vitte_arena_alloc(pool->arena, pool->object_size, pool->object_alignment);
    }

    if (object != NULL) {
        pool->allocation_count++;
    }
    return object;
}

void vitte_arena_pool_free(vitte_arena_pool_t *pool, void *object) {
    vitte_arena_pool_node_t *node;

    if (!vitte_arena_pool_is_initialized(pool) || object == NULL) {
        return;
    }

    if (!vitte_arena_contains(pool->arena, object)) {
        vitte_arena_set_error(pool->arena, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_ARENA_POOL_E_POINTER", "pool object does not belong to arena", NULL);
        return;
    }

    node = (vitte_arena_pool_node_t *)object;
    node->next = pool->free_list;
    pool->free_list = node;
    pool->free_count++;
}

void vitte_arena_pool_reset(vitte_arena_pool_t *pool) {
    if (pool == NULL) {
        return;
    }

    pool->free_list = NULL;
    pool->allocation_count = 0u;
    pool->free_count = 0u;
}

bool vitte_arena_pool_is_initialized(const vitte_arena_pool_t *pool) {
    return pool != NULL && pool->initialized && vitte_arena_is_initialized(pool->arena);
}
