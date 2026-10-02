/*
 * Vitte Compiler
 * src/arena/allocator.c
 *
 * High-performance arena allocator.
 *
 * Design goals:
 *
 *   - O(1) fast-path allocation
 *   - deterministic ownership
 *   - configurable block size
 *   - arbitrary power-of-two alignment
 *   - overflow-safe arithmetic
 *   - dedicated oversized allocations
 *   - block reuse after reset
 *   - marks and rewind
 *   - zero-initialized allocation
 *   - string/memory duplication
 *   - detailed statistics
 *   - structural validation
 *   - C17 portability
 *
 * Individual allocations are never freed. Memory is reclaimed through
 * rewind(), reset(), trim() or destroy().
 */

#include "allocator.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_ARENA_DEFAULT_BLOCK_SIZE
#define VITTE_ARENA_DEFAULT_BLOCK_SIZE ((size_t)64u * 1024u)
#endif

#ifndef VITTE_ARENA_MIN_BLOCK_SIZE
#define VITTE_ARENA_MIN_BLOCK_SIZE ((size_t)1024u)
#endif

#ifndef VITTE_ARENA_LARGE_ALLOCATION_DIVISOR
#define VITTE_ARENA_LARGE_ALLOCATION_DIVISOR ((size_t)2u)
#endif

#ifndef VITTE_ARENA_MAGIC
#define VITTE_ARENA_MAGIC UINT64_C(0x564954544541524E)
#endif

#ifndef VITTE_ARENA_DEAD_MAGIC
#define VITTE_ARENA_DEAD_MAGIC UINT64_C(0x444541444152454E)
#endif

#ifndef VITTE_ARENA_BLOCK_MAGIC
#define VITTE_ARENA_BLOCK_MAGIC UINT64_C(0x5649545445424C4B)
#endif

#ifndef VITTE_ARENA_POISON_ALLOCATED
#define VITTE_ARENA_POISON_ALLOCATED 0
#endif

#ifndef VITTE_ARENA_POISON_RESET
#define VITTE_ARENA_POISON_RESET 0
#endif

#ifndef VITTE_ARENA_ALLOC_PATTERN
#define VITTE_ARENA_ALLOC_PATTERN 0xCD
#endif

#ifndef VITTE_ARENA_RESET_PATTERN
#define VITTE_ARENA_RESET_PATTERN 0xDD
#endif

/* ========================================================================= */
/* Internal block                                                            */
/* ========================================================================= */

struct vitte_arena_block {
    uint64_t magic;

    struct vitte_arena_block *next;
    struct vitte_arena_block *previous;

    size_t capacity;
    size_t used;

    uint64_t generation;

    bool dedicated;

    max_align_t alignment_anchor;

    unsigned char data[];
};

/* ========================================================================= */
/* Arithmetic helpers                                                        */
/* ========================================================================= */

static bool
vitte_arena_add_overflow(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return true;
    }

    if (left > SIZE_MAX - right) {
        return true;
    }

    *result = left + right;
    return false;
}

static bool
vitte_arena_mul_overflow(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return true;
    }

    if (left != 0u &&
        right > SIZE_MAX / left) {
        return true;
    }

    *result = left * right;
    return false;
}

static bool
vitte_arena_is_power_of_two(
    size_t value)
{
    return value != 0u &&
           (value & (value - 1u)) == 0u;
}

static bool
vitte_arena_align_up(
    uintptr_t value,
    size_t alignment,
    uintptr_t *result)
{
    uintptr_t mask;

    if (result == NULL ||
        !vitte_arena_is_power_of_two(alignment)) {
        return false;
    }

    mask = (uintptr_t)(alignment - 1u);

    if (value > UINTPTR_MAX - mask) {
        return false;
    }

    *result = (value + mask) & ~mask;

    return true;
}

static size_t
vitte_arena_normalize_alignment(
    size_t alignment)
{
    if (alignment == 0u) {
        return _Alignof(max_align_t);
    }

    return alignment;
}

/* ========================================================================= */
/* Context validation                                                        */
/* ========================================================================= */

static bool
vitte_arena_magic_valid(
    const vitte_arena_t *arena)
{
    return arena != NULL &&
           arena->magic == VITTE_ARENA_MAGIC;
}

static bool
vitte_arena_block_valid(
    const vitte_arena_block_t *block)
{
    if (block == NULL) {
        return false;
    }

    if (block->magic !=
        VITTE_ARENA_BLOCK_MAGIC) {
        return false;
    }

    if (block->used > block->capacity) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Error handling                                                            */
/* ========================================================================= */

static void
vitte_arena_set_error(
    vitte_arena_t *arena,
    vitte_arena_error_t error)
{
    if (arena != NULL &&
        arena->magic == VITTE_ARENA_MAGIC) {
        arena->last_error = error;
    }
}

vitte_arena_error_t
vitte_arena_last_error(
    const vitte_arena_t *arena)
{
    if (!vitte_arena_magic_valid(arena)) {
        return VITTE_ARENA_ERROR_INVALID_ARENA;
    }

    return arena->last_error;
}

void
vitte_arena_clear_error(
    vitte_arena_t *arena)
{
    if (!vitte_arena_magic_valid(arena)) {
        return;
    }

    arena->last_error =
        VITTE_ARENA_ERROR_NONE;
}

const char *
vitte_arena_error_name(
    vitte_arena_error_t error)
{
    switch (error) {
        case VITTE_ARENA_ERROR_NONE:
            return "none";

        case VITTE_ARENA_ERROR_INVALID_ARENA:
            return "invalid-arena";

        case VITTE_ARENA_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_ARENA_ERROR_INVALID_ALIGNMENT:
            return "invalid-alignment";

        case VITTE_ARENA_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_ARENA_ERROR_OUT_OF_MEMORY:
            return "out-of-memory";

        case VITTE_ARENA_ERROR_INVALID_MARK:
            return "invalid-mark";

        case VITTE_ARENA_ERROR_CORRUPTION:
            return "corruption";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Block allocation                                                          */
/* ========================================================================= */

static vitte_arena_block_t *
vitte_arena_block_create(
    vitte_arena_t *arena,
    size_t capacity,
    bool dedicated)
{
    vitte_arena_block_t *block;
    size_t total_size;

    if (vitte_arena_add_overflow(
            sizeof(*block),
            capacity,
            &total_size)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    block = (vitte_arena_block_t *)
        malloc(total_size);

    if (block == NULL) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OUT_OF_MEMORY);

        return NULL;
    }

    block->magic =
        VITTE_ARENA_BLOCK_MAGIC;

    block->next = NULL;
    block->previous = NULL;

    block->capacity = capacity;
    block->used = 0u;

    block->generation =
        arena != NULL
            ? arena->generation
            : 0u;

    block->dedicated = dedicated;

#if VITTE_ARENA_POISON_RESET
    if (capacity != 0u) {
        memset(
            block->data,
            VITTE_ARENA_RESET_PATTERN,
            capacity);
    }
#endif

    return block;
}

static void
vitte_arena_block_destroy(
    vitte_arena_block_t *block)
{
    if (block == NULL) {
        return;
    }

    if (block->magic !=
        VITTE_ARENA_BLOCK_MAGIC) {
        return;
    }

#if VITTE_ARENA_POISON_RESET
    if (block->capacity != 0u) {
        memset(
            block->data,
            VITTE_ARENA_RESET_PATTERN,
            block->capacity);
    }
#endif

    block->magic = 0u;
    block->next = NULL;
    block->previous = NULL;
    block->capacity = 0u;
    block->used = 0u;
    block->generation = 0u;
    block->dedicated = false;

    free(block);
}

/* ========================================================================= */
/* Linked-list helpers                                                       */
/* ========================================================================= */

static void
vitte_arena_append_block(
    vitte_arena_t *arena,
    vitte_arena_block_t *block)
{
    if (arena == NULL || block == NULL) {
        return;
    }

    block->previous = arena->last_block;
    block->next = NULL;

    if (arena->last_block != NULL) {
        arena->last_block->next = block;
    } else {
        arena->first_block = block;
    }

    arena->last_block = block;
    arena->current_block = block;

    ++arena->block_count;

    if (arena->block_count >
        arena->peak_block_count) {
        arena->peak_block_count =
            arena->block_count;
    }

    arena->reserved_bytes +=
        block->capacity;

    if (arena->reserved_bytes >
        arena->peak_reserved_bytes) {
        arena->peak_reserved_bytes =
            arena->reserved_bytes;
    }
}

static void
vitte_arena_unlink_block(
    vitte_arena_t *arena,
    vitte_arena_block_t *block)
{
    if (arena == NULL || block == NULL) {
        return;
    }

    if (block->previous != NULL) {
        block->previous->next =
            block->next;
    } else {
        arena->first_block =
            block->next;
    }

    if (block->next != NULL) {
        block->next->previous =
            block->previous;
    } else {
        arena->last_block =
            block->previous;
    }

    if (arena->current_block == block) {
        arena->current_block =
            block->previous != NULL
                ? block->previous
                : block->next;
    }

    if (arena->block_count != 0u) {
        --arena->block_count;
    }

    if (arena->reserved_bytes >=
        block->capacity) {
        arena->reserved_bytes -=
            block->capacity;
    } else {
        arena->reserved_bytes = 0u;
    }

    block->next = NULL;
    block->previous = NULL;
}

/* ========================================================================= */
/* Allocation inside a block                                                 */
/* ========================================================================= */

static void *
vitte_arena_try_allocate_from_block(
    vitte_arena_t *arena,
    vitte_arena_block_t *block,
    size_t size,
    size_t alignment)
{
    uintptr_t base;
    uintptr_t current;
    uintptr_t aligned;
    size_t offset;
    size_t end;

    if (!vitte_arena_block_valid(block)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_CORRUPTION);

        return NULL;
    }

    base = (uintptr_t)block->data;

    if ((uintptr_t)block->used >
        UINTPTR_MAX - base) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    current =
        base + (uintptr_t)block->used;

    if (!vitte_arena_align_up(
            current,
            alignment,
            &aligned)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    if (aligned < base) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    if (aligned - base >
        (uintptr_t)SIZE_MAX) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    offset =
        (size_t)(aligned - base);

    if (vitte_arena_add_overflow(
            offset,
            size,
            &end)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    if (end > block->capacity) {
        return NULL;
    }

    block->used = end;

    return (void *)aligned;
}

/* ========================================================================= */
/* Reusable block lookup                                                     */
/* ========================================================================= */

static vitte_arena_block_t *
vitte_arena_find_reusable_block(
    vitte_arena_t *arena,
    size_t required)
{
    vitte_arena_block_t *block;

    if (arena == NULL) {
        return NULL;
    }

    block =
        arena->current_block != NULL
            ? arena->current_block->next
            : arena->first_block;

    while (block != NULL) {
        if (!block->dedicated &&
            block->capacity >= required &&
            block->used == 0u) {
            return block;
        }

        block = block->next;
    }

    return NULL;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_arena_init(
    vitte_arena_t *arena,
    size_t block_size)
{
    if (arena == NULL) {
        return false;
    }

    memset(arena, 0, sizeof(*arena));

    if (block_size == 0u) {
        block_size =
            VITTE_ARENA_DEFAULT_BLOCK_SIZE;
    }

    if (block_size <
        VITTE_ARENA_MIN_BLOCK_SIZE) {
        block_size =
            VITTE_ARENA_MIN_BLOCK_SIZE;
    }

    arena->magic = VITTE_ARENA_MAGIC;

    arena->block_size = block_size;

    arena->first_block = NULL;
    arena->last_block = NULL;
    arena->current_block = NULL;

    arena->block_count = 0u;
    arena->peak_block_count = 0u;

    arena->reserved_bytes = 0u;
    arena->peak_reserved_bytes = 0u;

    arena->used_bytes = 0u;
    arena->peak_used_bytes = 0u;

    arena->requested_bytes = 0u;
    arena->allocation_count = 0u;

    arena->generation = 1u;

    arena->last_error =
        VITTE_ARENA_ERROR_NONE;

    return true;
}

bool
vitte_arena_init_default(
    vitte_arena_t *arena)
{
    return vitte_arena_init(
        arena,
        VITTE_ARENA_DEFAULT_BLOCK_SIZE);
}

void
vitte_arena_destroy(
    vitte_arena_t *arena)
{
    vitte_arena_block_t *block;

    if (!vitte_arena_magic_valid(arena)) {
        return;
    }

    block = arena->first_block;

    while (block != NULL) {
        vitte_arena_block_t *next;

        next = block->next;

        vitte_arena_block_destroy(block);

        block = next;
    }

    arena->first_block = NULL;
    arena->last_block = NULL;
    arena->current_block = NULL;

    arena->block_count = 0u;
    arena->reserved_bytes = 0u;
    arena->used_bytes = 0u;

    arena->magic =
        VITTE_ARENA_DEAD_MAGIC;

    arena->block_size = 0u;
    arena->generation = 0u;

    arena->last_error =
        VITTE_ARENA_ERROR_INVALID_ARENA;
}

/* ========================================================================= */
/* Core allocation                                                           */
/* ========================================================================= */

void *
vitte_arena_alloc_aligned(
    vitte_arena_t *arena,
    size_t size,
    size_t alignment)
{
    vitte_arena_block_t *block;
    void *memory;
    size_t minimum_capacity;
    bool dedicated;

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    arena->last_error =
        VITTE_ARENA_ERROR_NONE;

    alignment =
        vitte_arena_normalize_alignment(
            alignment);

    if (!vitte_arena_is_power_of_two(
            alignment)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_ALIGNMENT);

        return NULL;
    }

    /*
     * Arena allocation of zero bytes still returns a unique usable location
     * whenever possible.
     */
    if (size == 0u) {
        size = 1u;
    }

    if (vitte_arena_add_overflow(
            size,
            alignment - 1u,
            &minimum_capacity)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    block = arena->current_block;

    if (block != NULL &&
        !block->dedicated) {
        size_t old_used;

        old_used = block->used;

        memory =
            vitte_arena_try_allocate_from_block(
                arena,
                block,
                size,
                alignment);

        if (memory != NULL) {
            arena->used_bytes +=
                block->used - old_used;

            goto success;
        }

        /*
         * Capacity miss is expected. Clear only that transient state.
         */
        if (arena->last_error ==
            VITTE_ARENA_ERROR_NONE) {
            /* nothing */
        } else if (
            arena->last_error !=
                VITTE_ARENA_ERROR_CORRUPTION &&
            arena->last_error !=
                VITTE_ARENA_ERROR_OVERFLOW) {
            arena->last_error =
                VITTE_ARENA_ERROR_NONE;
        } else {
            return NULL;
        }
    }

    /*
     * Prefer an unused retained block after reset/rewind.
     */
    block =
        vitte_arena_find_reusable_block(
            arena,
            minimum_capacity);

    if (block != NULL) {
        size_t old_used;

        arena->current_block = block;

        old_used = block->used;

        memory =
            vitte_arena_try_allocate_from_block(
                arena,
                block,
                size,
                alignment);

        if (memory == NULL) {
            return NULL;
        }

        arena->used_bytes +=
            block->used - old_used;

        goto success;
    }

    dedicated =
        minimum_capacity >
            arena->block_size /
                VITTE_ARENA_LARGE_ALLOCATION_DIVISOR;

    if (!dedicated) {
        if (minimum_capacity <
            arena->block_size) {
            minimum_capacity =
                arena->block_size;
        }
    }

    block =
        vitte_arena_block_create(
            arena,
            minimum_capacity,
            dedicated);

    if (block == NULL) {
        return NULL;
    }

    vitte_arena_append_block(
        arena,
        block);

    {
        size_t old_used;

        old_used = block->used;

        memory =
            vitte_arena_try_allocate_from_block(
                arena,
                block,
                size,
                alignment);

        if (memory == NULL) {
            vitte_arena_unlink_block(
                arena,
                block);

            vitte_arena_block_destroy(block);

            return NULL;
        }

        arena->used_bytes +=
            block->used - old_used;
    }

success:

    if (arena->used_bytes >
        arena->peak_used_bytes) {
        arena->peak_used_bytes =
            arena->used_bytes;
    }

    if (arena->requested_bytes <=
        SIZE_MAX - size) {
        arena->requested_bytes += size;
    } else {
        arena->requested_bytes =
            SIZE_MAX;
    }

    if (arena->allocation_count !=
        SIZE_MAX) {
        ++arena->allocation_count;
    }

#if VITTE_ARENA_POISON_ALLOCATED
    memset(
        memory,
        VITTE_ARENA_ALLOC_PATTERN,
        size);
#endif

    return memory;
}

void *
vitte_arena_alloc(
    vitte_arena_t *arena,
    size_t size)
{
    return vitte_arena_alloc_aligned(
        arena,
        size,
        _Alignof(max_align_t));
}

/* ========================================================================= */
/* Typed/count allocation                                                    */
/* ========================================================================= */

void *
vitte_arena_alloc_array(
    vitte_arena_t *arena,
    size_t count,
    size_t element_size,
    size_t alignment)
{
    size_t size;

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    if (vitte_arena_mul_overflow(
            count,
            element_size,
            &size)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    return vitte_arena_alloc_aligned(
        arena,
        size,
        alignment);
}

void *
vitte_arena_calloc(
    vitte_arena_t *arena,
    size_t count,
    size_t element_size)
{
    void *memory;
    size_t size;

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    if (vitte_arena_mul_overflow(
            count,
            element_size,
            &size)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    memory =
        vitte_arena_alloc(
            arena,
            size);

    if (memory == NULL) {
        return NULL;
    }

    if (size != 0u) {
        memset(memory, 0, size);
    }

    return memory;
}

void *
vitte_arena_calloc_aligned(
    vitte_arena_t *arena,
    size_t count,
    size_t element_size,
    size_t alignment)
{
    void *memory;
    size_t size;

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    if (vitte_arena_mul_overflow(
            count,
            element_size,
            &size)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    memory =
        vitte_arena_alloc_aligned(
            arena,
            size,
            alignment);

    if (memory == NULL) {
        return NULL;
    }

    if (size != 0u) {
        memset(memory, 0, size);
    }

    return memory;
}

/* ========================================================================= */
/* Duplication                                                               */
/* ========================================================================= */

void *
vitte_arena_memdup(
    vitte_arena_t *arena,
    const void *data,
    size_t size)
{
    void *copy;

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    if (data == NULL && size != 0u) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_ARGUMENT);

        return NULL;
    }

    copy =
        vitte_arena_alloc(
            arena,
            size);

    if (copy == NULL) {
        return NULL;
    }

    if (size != 0u) {
        memcpy(copy, data, size);
    }

    return copy;
}

char *
vitte_arena_strndup(
    vitte_arena_t *arena,
    const char *text,
    size_t length)
{
    char *copy;
    size_t allocation_size;

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    if (text == NULL && length != 0u) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_ARGUMENT);

        return NULL;
    }

    if (vitte_arena_add_overflow(
            length,
            1u,
            &allocation_size)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    copy = (char *)
        vitte_arena_alloc_aligned(
            arena,
            allocation_size,
            _Alignof(char));

    if (copy == NULL) {
        return NULL;
    }

    if (length != 0u) {
        memcpy(copy, text, length);
    }

    copy[length] = '\0';

    return copy;
}

char *
vitte_arena_strdup(
    vitte_arena_t *arena,
    const char *text)
{
    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    if (text == NULL) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_ARGUMENT);

        return NULL;
    }

    return vitte_arena_strndup(
        arena,
        text,
        strlen(text));
}

/* ========================================================================= */
/* Marks                                                                     */
/* ========================================================================= */

vitte_arena_mark_t
vitte_arena_mark(
    const vitte_arena_t *arena)
{
    vitte_arena_mark_t mark;

    memset(&mark, 0, sizeof(mark));

    if (!vitte_arena_magic_valid(arena)) {
        return mark;
    }

    mark.arena = arena;
    mark.block = arena->current_block;

    mark.used =
        arena->current_block != NULL
            ? arena->current_block->used
            : 0u;

    mark.used_bytes =
        arena->used_bytes;

    mark.requested_bytes =
        arena->requested_bytes;

    mark.allocation_count =
        arena->allocation_count;

    mark.generation =
        arena->generation;

    return mark;
}

bool
vitte_arena_rewind(
    vitte_arena_t *arena,
    vitte_arena_mark_t mark)
{
    vitte_arena_block_t *block;
    bool found;

    if (!vitte_arena_magic_valid(arena)) {
        return false;
    }

    if (mark.arena != arena ||
        mark.generation != arena->generation) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_MARK);

        return false;
    }

    if (mark.block == NULL) {
        block = arena->first_block;

        while (block != NULL) {
#if VITTE_ARENA_POISON_RESET
            if (block->used != 0u) {
                memset(
                    block->data,
                    VITTE_ARENA_RESET_PATTERN,
                    block->used);
            }
#endif

            block->used = 0u;
            block = block->next;
        }

        arena->current_block =
            arena->first_block;

        arena->used_bytes = 0u;
        arena->requested_bytes =
            mark.requested_bytes;

        arena->allocation_count =
            mark.allocation_count;

        arena->last_error =
            VITTE_ARENA_ERROR_NONE;

        return true;
    }

    found = false;
    block = arena->first_block;

    while (block != NULL) {
        if (block == mark.block) {
            found = true;
            break;
        }

        block = block->next;
    }

    if (!found ||
        !vitte_arena_block_valid(mark.block) ||
        mark.used > mark.block->capacity) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_MARK);

        return false;
    }

#if VITTE_ARENA_POISON_RESET
    if (mark.block->used > mark.used) {
        memset(
            mark.block->data + mark.used,
            VITTE_ARENA_RESET_PATTERN,
            mark.block->used - mark.used);
    }
#endif

    mark.block->used = mark.used;

    block = mark.block->next;

    while (block != NULL) {
#if VITTE_ARENA_POISON_RESET
        if (block->used != 0u) {
            memset(
                block->data,
                VITTE_ARENA_RESET_PATTERN,
                block->used);
        }
#endif

        block->used = 0u;
        block = block->next;
    }

    arena->current_block =
        mark.block;

    arena->used_bytes =
        mark.used_bytes;

    arena->requested_bytes =
        mark.requested_bytes;

    arena->allocation_count =
        mark.allocation_count;

    arena->last_error =
        VITTE_ARENA_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

void
vitte_arena_reset(
    vitte_arena_t *arena)
{
    vitte_arena_block_t *block;

    if (!vitte_arena_magic_valid(arena)) {
        return;
    }

    block = arena->first_block;

    while (block != NULL) {
#if VITTE_ARENA_POISON_RESET
        if (block->used != 0u) {
            memset(
                block->data,
                VITTE_ARENA_RESET_PATTERN,
                block->used);
        }
#endif

        block->used = 0u;
        block->generation =
            arena->generation + 1u;

        block = block->next;
    }

    ++arena->generation;

    if (arena->generation == 0u) {
        ++arena->generation;
    }

    arena->current_block =
        arena->first_block;

    arena->used_bytes = 0u;
    arena->requested_bytes = 0u;
    arena->allocation_count = 0u;

    arena->last_error =
        VITTE_ARENA_ERROR_NONE;
}

/* ========================================================================= */
/* Trim                                                                      */
/* ========================================================================= */

void
vitte_arena_trim(
    vitte_arena_t *arena)
{
    vitte_arena_block_t *block;

    if (!vitte_arena_magic_valid(arena)) {
        return;
    }

    block = arena->first_block;

    while (block != NULL) {
        vitte_arena_block_t *next;

        next = block->next;

        if (block->used == 0u) {
            vitte_arena_unlink_block(
                arena,
                block);

            vitte_arena_block_destroy(block);
        }

        block = next;
    }

    if (arena->current_block == NULL) {
        arena->current_block =
            arena->last_block;
    }
}

/* ========================================================================= */
/* Reset and trim                                                            */
/* ========================================================================= */

void
vitte_arena_reset_and_trim(
    vitte_arena_t *arena)
{
    if (!vitte_arena_magic_valid(arena)) {
        return;
    }

    vitte_arena_reset(arena);
    vitte_arena_trim(arena);
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_arena_stats_t
vitte_arena_stats(
    const vitte_arena_t *arena)
{
    vitte_arena_stats_t stats;

    memset(&stats, 0, sizeof(stats));

    if (!vitte_arena_magic_valid(arena)) {
        return stats;
    }

    stats.block_size =
        arena->block_size;

    stats.block_count =
        arena->block_count;

    stats.peak_block_count =
        arena->peak_block_count;

    stats.reserved_bytes =
        arena->reserved_bytes;

    stats.peak_reserved_bytes =
        arena->peak_reserved_bytes;

    stats.used_bytes =
        arena->used_bytes;

    stats.peak_used_bytes =
        arena->peak_used_bytes;

    stats.requested_bytes =
        arena->requested_bytes;

    stats.allocation_count =
        arena->allocation_count;

    stats.generation =
        arena->generation;

    if (arena->reserved_bytes >=
        arena->used_bytes) {
        stats.unused_bytes =
            arena->reserved_bytes -
            arena->used_bytes;
    }

    if (arena->used_bytes >=
        arena->requested_bytes) {
        stats.padding_bytes =
            arena->used_bytes -
            arena->requested_bytes;
    }

    return stats;
}

size_t
vitte_arena_block_count(
    const vitte_arena_t *arena)
{
    if (!vitte_arena_magic_valid(arena)) {
        return 0u;
    }

    return arena->block_count;
}

size_t
vitte_arena_reserved_bytes(
    const vitte_arena_t *arena)
{
    if (!vitte_arena_magic_valid(arena)) {
        return 0u;
    }

    return arena->reserved_bytes;
}

size_t
vitte_arena_used_bytes(
    const vitte_arena_t *arena)
{
    if (!vitte_arena_magic_valid(arena)) {
        return 0u;
    }

    return arena->used_bytes;
}

size_t
vitte_arena_requested_bytes(
    const vitte_arena_t *arena)
{
    if (!vitte_arena_magic_valid(arena)) {
        return 0u;
    }

    return arena->requested_bytes;
}

size_t
vitte_arena_allocation_count(
    const vitte_arena_t *arena)
{
    if (!vitte_arena_magic_valid(arena)) {
        return 0u;
    }

    return arena->allocation_count;
}

uint64_t
vitte_arena_generation(
    const vitte_arena_t *arena)
{
    if (!vitte_arena_magic_valid(arena)) {
        return 0u;
    }

    return arena->generation;
}

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

bool
vitte_arena_contains(
    const vitte_arena_t *arena,
    const void *pointer)
{
    const vitte_arena_block_t *block;
    uintptr_t address;

    if (!vitte_arena_magic_valid(arena) ||
        pointer == NULL) {
        return false;
    }

    address = (uintptr_t)pointer;

    block = arena->first_block;

    while (block != NULL) {
        uintptr_t begin;
        uintptr_t end;

        if (!vitte_arena_block_valid(block)) {
            return false;
        }

        begin = (uintptr_t)block->data;

        if ((uintptr_t)block->capacity >
            UINTPTR_MAX - begin) {
            return false;
        }

        end =
            begin +
            (uintptr_t)block->capacity;

        if (address >= begin &&
            address < end) {
            return true;
        }

        block = block->next;
    }

    return false;
}

bool
vitte_arena_contains_live(
    const vitte_arena_t *arena,
    const void *pointer)
{
    const vitte_arena_block_t *block;
    uintptr_t address;

    if (!vitte_arena_magic_valid(arena) ||
        pointer == NULL) {
        return false;
    }

    address = (uintptr_t)pointer;

    block = arena->first_block;

    while (block != NULL) {
        uintptr_t begin;
        uintptr_t end;

        if (!vitte_arena_block_valid(block)) {
            return false;
        }

        begin = (uintptr_t)block->data;

        if ((uintptr_t)block->used >
            UINTPTR_MAX - begin) {
            return false;
        }

        end =
            begin +
            (uintptr_t)block->used;

        if (address >= begin &&
            address < end) {
            return true;
        }

        block = block->next;
    }

    return false;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_arena_validate(
    const vitte_arena_t *arena)
{
    const vitte_arena_block_t *block;
    const vitte_arena_block_t *previous;
    size_t block_count;
    size_t reserved;
    size_t used;
    bool current_found;

    if (!vitte_arena_magic_valid(arena)) {
        return false;
    }

    if (arena->block_size <
        VITTE_ARENA_MIN_BLOCK_SIZE) {
        return false;
    }

    if ((arena->first_block == NULL) !=
        (arena->last_block == NULL)) {
        return false;
    }

    if (arena->block_count == 0u &&
        (arena->first_block != NULL ||
         arena->last_block != NULL)) {
        return false;
    }

    block_count = 0u;
    reserved = 0u;
    used = 0u;

    previous = NULL;
    current_found =
        arena->current_block == NULL;

    block = arena->first_block;

    while (block != NULL) {
        size_t new_value;

        if (!vitte_arena_block_valid(block)) {
            return false;
        }

        if (block->previous != previous) {
            return false;
        }

        if (block->generation >
            arena->generation) {
            return false;
        }

        if (block == arena->current_block) {
            current_found = true;
        }

        if (vitte_arena_add_overflow(
                reserved,
                block->capacity,
                &new_value)) {
            return false;
        }

        reserved = new_value;

        if (vitte_arena_add_overflow(
                used,
                block->used,
                &new_value)) {
            return false;
        }

        used = new_value;

        if (block_count == SIZE_MAX) {
            return false;
        }

        ++block_count;

        previous = block;
        block = block->next;
    }

    if (previous != arena->last_block) {
        return false;
    }

    if (!current_found) {
        return false;
    }

    if (block_count !=
        arena->block_count) {
        return false;
    }

    if (reserved !=
        arena->reserved_bytes) {
        return false;
    }

    if (used !=
        arena->used_bytes) {
        return false;
    }

    if (arena->used_bytes >
        arena->reserved_bytes) {
        return false;
    }

    if (arena->peak_block_count <
        arena->block_count) {
        return false;
    }

    if (arena->peak_reserved_bytes <
        arena->reserved_bytes) {
        return false;
    }

    if (arena->peak_used_bytes <
        arena->used_bytes) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
