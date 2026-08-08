#include "checkpoint.h"

#include <string.h>

static bool vitte_arena_checkpoint_block_belongs(
    const vitte_arena_t *arena,
    const vitte_arena_block_t *target
) {
    const vitte_arena_block_t *block;

    if (arena == NULL) {
        return false;
    }

    if (target == NULL) {
        return true;
    }

    for (block = arena->first; block != NULL; block = block->next) {
        if (block == target) {
            return true;
        }
    }

    return false;
}

static void vitte_arena_checkpoint_recompute(vitte_arena_t *arena) {
    vitte_arena_block_t *block;
    size_t reserved = 0u;
    size_t used = 0u;
    size_t blocks = 0u;

    for (block = arena->first; block != NULL; block = block->next) {
        reserved += block->capacity;
        used += block->offset;
        blocks++;
    }

    arena->stats.bytes_reserved = reserved;
    arena->stats.bytes_used = used;
    arena->stats.block_count = blocks;
}

vitte_arena_checkpoint_t vitte_arena_checkpoint(vitte_arena_t *arena) {
    vitte_arena_checkpoint_t checkpoint;

    memset(&checkpoint, 0, sizeof(checkpoint));
    if (!vitte_arena_is_initialized(arena)) {
        return checkpoint;
    }

    checkpoint.arena = arena;
    checkpoint.block = arena->current;
    checkpoint.offset = arena->current != NULL ? arena->current->offset : 0u;
    checkpoint.bytes_used = arena->stats.bytes_used;
    checkpoint.allocation_count = arena->stats.allocation_count;
    checkpoint.valid = true;
    return checkpoint;
}

bool vitte_arena_checkpoint_is_valid(const vitte_arena_checkpoint_t *checkpoint) {
    return checkpoint != NULL &&
        checkpoint->valid &&
        vitte_arena_is_initialized(checkpoint->arena) &&
        vitte_arena_checkpoint_block_belongs(checkpoint->arena, checkpoint->block);
}

bool vitte_arena_rollback(vitte_arena_checkpoint_t *checkpoint) {
    vitte_arena_t *arena;
    vitte_arena_block_t *after;

    if (!vitte_arena_checkpoint_is_valid(checkpoint)) {
        return false;
    }

    arena = checkpoint->arena;
    if (checkpoint->block == NULL) {
        vitte_arena_block_destroy_chain(&arena->config.allocator, arena->first);
        arena->first = NULL;
        arena->current = NULL;
    } else {
        after = checkpoint->block->next;
        checkpoint->block->next = NULL;
        vitte_arena_block_destroy_chain(&arena->config.allocator, after);
        checkpoint->block->offset = checkpoint->offset;
        arena->current = checkpoint->block;
    }

    arena->stats.bytes_used = checkpoint->bytes_used;
    arena->stats.allocation_count = checkpoint->allocation_count;
    vitte_arena_checkpoint_recompute(arena);
    vitte_error_reset(&arena->last_error);
    checkpoint->valid = false;
    return true;
}
