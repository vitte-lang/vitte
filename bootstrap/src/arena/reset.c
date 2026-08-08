#include "reset.h"

void vitte_arena_reset_keep_first(vitte_arena_t *arena) {
    vitte_arena_reset_with_policy(arena, VITTE_ARENA_RESET_KEEP_FIRST_BLOCK);
}

void vitte_arena_reset_release_all(vitte_arena_t *arena) {
    vitte_arena_reset_with_policy(arena, VITTE_ARENA_RESET_RELEASE_ALL_BLOCKS);
}

void vitte_arena_reset_with_policy(vitte_arena_t *arena, vitte_arena_reset_policy_t policy) {
    vitte_arena_reset_policy_t previous;

    if (!vitte_arena_is_initialized(arena)) {
        return;
    }

    previous = arena->config.reset_policy;
    arena->config.reset_policy = policy;
    vitte_arena_reset(arena);
    arena->config.reset_policy = previous;
}
