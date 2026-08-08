#ifndef VITTE_BOOTSTRAP_ARENA_CHECKPOINT_H
#define VITTE_BOOTSTRAP_ARENA_CHECKPOINT_H

#include <stdbool.h>
#include <stddef.h>

#include "arena.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_arena_checkpoint {
    vitte_arena_t *arena;
    vitte_arena_block_t *block;
    size_t offset;
    size_t bytes_used;
    size_t allocation_count;
    bool valid;
} vitte_arena_checkpoint_t;

vitte_arena_checkpoint_t vitte_arena_checkpoint(vitte_arena_t *arena);
bool vitte_arena_checkpoint_is_valid(const vitte_arena_checkpoint_t *checkpoint);
bool vitte_arena_rollback(vitte_arena_checkpoint_t *checkpoint);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_ARENA_CHECKPOINT_H */
