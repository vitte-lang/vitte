#ifndef VITTE_BOOTSTRAP_ARENA_BLOCK_H
#define VITTE_BOOTSTRAP_ARENA_BLOCK_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_arena_block {
    unsigned char *memory;
    size_t capacity;
    size_t offset;
    struct vitte_arena_block *next;
} vitte_arena_block_t;

bool vitte_arena_alignment_is_valid(size_t alignment);
bool vitte_arena_align_forward(size_t value, size_t alignment, size_t *aligned);

vitte_arena_block_t *vitte_arena_block_create(
    const vitte_allocator_t *allocator,
    size_t capacity
);

void vitte_arena_block_destroy(
    const vitte_allocator_t *allocator,
    vitte_arena_block_t *block
);

void vitte_arena_block_destroy_chain(
    const vitte_allocator_t *allocator,
    vitte_arena_block_t *block
);

void *vitte_arena_block_alloc(
    vitte_arena_block_t *block,
    size_t size,
    size_t alignment
);

size_t vitte_arena_block_remaining(const vitte_arena_block_t *block);
bool vitte_arena_block_contains(const vitte_arena_block_t *block, const void *pointer);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_ARENA_BLOCK_H */
