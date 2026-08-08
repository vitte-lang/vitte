#include "allocator.h"

#include <string.h>

static void *vitte_arena_allocator_alloc(void *user, size_t size) {
    vitte_arena_t *arena = (vitte_arena_t *)user;
    return vitte_arena_alloc(arena, size, VITTE_ARENA_DEFAULT_ALIGNMENT);
}

static void *vitte_arena_allocator_realloc(void *user, void *pointer, size_t size) {
    vitte_arena_t *arena = (vitte_arena_t *)user;

    if (pointer != NULL) {
        vitte_arena_set_error(
            arena,
            VITTE_STATUS_ERROR_UNSUPPORTED,
            "VITTE_ARENA_ALLOCATOR_E_REALLOC",
            "arena allocator cannot realloc existing allocations",
            NULL
        );
        return NULL;
    }

    return vitte_arena_alloc(arena, size, VITTE_ARENA_DEFAULT_ALIGNMENT);
}

static void vitte_arena_allocator_free(void *user, void *pointer) {
    (void)user;
    (void)pointer;
}

bool vitte_arena_allocator_init(vitte_arena_allocator_t *allocator, vitte_arena_t *arena) {
    if (allocator == NULL || !vitte_arena_is_initialized(arena)) {
        return false;
    }

    memset(allocator, 0, sizeof(*allocator));
    allocator->arena = arena;
    allocator->interface.user = arena;
    allocator->interface.alloc = vitte_arena_allocator_alloc;
    allocator->interface.realloc = vitte_arena_allocator_realloc;
    allocator->interface.free = vitte_arena_allocator_free;
    return true;
}

vitte_allocator_t *vitte_arena_allocator_interface(vitte_arena_allocator_t *allocator) {
    return allocator != NULL ? &allocator->interface : NULL;
}

const vitte_allocator_t *vitte_arena_allocator_interface_const(const vitte_arena_allocator_t *allocator) {
    return allocator != NULL ? &allocator->interface : NULL;
}
