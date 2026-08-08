#include "block.h"

#include <stdint.h>
#include <string.h>

bool vitte_arena_alignment_is_valid(size_t alignment) {
    return alignment != 0u && (alignment & (alignment - 1u)) == 0u;
}

bool vitte_arena_align_forward(size_t value, size_t alignment, size_t *aligned) {
    size_t mask;

    if (!vitte_arena_alignment_is_valid(alignment) || aligned == NULL) {
        return false;
    }

    mask = alignment - 1u;
    if (value > SIZE_MAX - mask) {
        return false;
    }

    *aligned = (value + mask) & ~mask;
    return true;
}

vitte_arena_block_t *vitte_arena_block_create(
    const vitte_allocator_t *allocator,
    size_t capacity
) {
    vitte_arena_block_t *block;

    if (!vitte_allocator_is_valid(allocator) || capacity == 0u) {
        return NULL;
    }

    block = (vitte_arena_block_t *)allocator->alloc(allocator->user, sizeof(*block));
    if (block == NULL) {
        return NULL;
    }

    block->memory = (unsigned char *)allocator->alloc(allocator->user, capacity);
    if (block->memory == NULL) {
        allocator->free(allocator->user, block);
        return NULL;
    }

    block->capacity = capacity;
    block->offset = 0u;
    block->next = NULL;
    return block;
}

void vitte_arena_block_destroy(
    const vitte_allocator_t *allocator,
    vitte_arena_block_t *block
) {
    if (!vitte_allocator_is_valid(allocator) || block == NULL) {
        return;
    }

    allocator->free(allocator->user, block->memory);
    memset(block, 0, sizeof(*block));
    allocator->free(allocator->user, block);
}

void vitte_arena_block_destroy_chain(
    const vitte_allocator_t *allocator,
    vitte_arena_block_t *block
) {
    while (block != NULL) {
        vitte_arena_block_t *next = block->next;
        vitte_arena_block_destroy(allocator, block);
        block = next;
    }
}

void *vitte_arena_block_alloc(
    vitte_arena_block_t *block,
    size_t size,
    size_t alignment
) {
    size_t aligned_offset;

    if (block == NULL || size == 0u) {
        return NULL;
    }

    if (!vitte_arena_align_forward(block->offset, alignment, &aligned_offset)) {
        return NULL;
    }

    if (aligned_offset > block->capacity || size > block->capacity - aligned_offset) {
        return NULL;
    }

    block->offset = aligned_offset + size;
    return block->memory + aligned_offset;
}

size_t vitte_arena_block_remaining(const vitte_arena_block_t *block) {
    if (block == NULL || block->offset > block->capacity) {
        return 0u;
    }

    return block->capacity - block->offset;
}

bool vitte_arena_block_contains(const vitte_arena_block_t *block, const void *pointer) {
    const unsigned char *byte_pointer = (const unsigned char *)pointer;

    if (block == NULL || pointer == NULL || block->memory == NULL) {
        return false;
    }

    return byte_pointer >= block->memory && byte_pointer < block->memory + block->capacity;
}
