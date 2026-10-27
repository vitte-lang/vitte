/*
 * Vitte Compiler
 * src/arena/allocator.c
 *
 * Canonical high-performance arena allocator implementation.
 *
 * Design goals:
 *
 *   - ISO C17
 *   - O(1) allocation fast path
 *   - deterministic ownership
 *   - configurable normal block size
 *   - arbitrary valid power-of-two alignment
 *   - overflow-safe size and address arithmetic
 *   - dedicated oversized allocations
 *   - retained-block reuse after rewind/reset
 *   - transactional marks and rewind
 *   - zero-initialized allocation
 *   - memory/string duplication
 *   - deterministic statistics
 *   - structural corruption detection
 *   - optional allocation/reset poisoning
 *   - no per-allocation heap metadata
 *
 * Individual allocations are never freed. Memory is reclaimed through:
 *
 *     vitte_arena_rewind()
 *     vitte_arena_reset()
 *     vitte_arena_trim()
 *     vitte_arena_reset_and_trim()
 *     vitte_arena_destroy()
 *
 * Dedicated blocks are single-allocation blocks. They are never selected by
 * the normal retained-block reuse path.
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
#define VITTE_ARENA_DEFAULT_BLOCK_SIZE \
    ((size_t)64u * (size_t)1024u)
#endif

#ifndef VITTE_ARENA_MIN_BLOCK_SIZE
#define VITTE_ARENA_MIN_BLOCK_SIZE \
    ((size_t)1024u)
#endif

#ifndef VITTE_ARENA_LARGE_ALLOCATION_DIVISOR
#define VITTE_ARENA_LARGE_ALLOCATION_DIVISOR \
    ((size_t)2u)
#endif

#ifndef VITTE_ARENA_MAGIC
#define VITTE_ARENA_MAGIC \
    UINT64_C(0x564954544541524E)
#endif

#ifndef VITTE_ARENA_DEAD_MAGIC
#define VITTE_ARENA_DEAD_MAGIC \
    UINT64_C(0x444541444152454E)
#endif

#ifndef VITTE_ARENA_BLOCK_MAGIC
#define VITTE_ARENA_BLOCK_MAGIC \
    UINT64_C(0x5649545445424C4B)
#endif

#ifndef VITTE_ARENA_DEAD_BLOCK_MAGIC
#define VITTE_ARENA_DEAD_BLOCK_MAGIC \
    UINT64_C(0x44454144424C4B21)
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
/* Compile-time invariants                                                   */
/* ========================================================================= */

_Static_assert(
    VITTE_ARENA_MIN_BLOCK_SIZE != 0u,
    "VITTE_ARENA_MIN_BLOCK_SIZE must be non-zero"
);

_Static_assert(
    VITTE_ARENA_DEFAULT_BLOCK_SIZE != 0u,
    "VITTE_ARENA_DEFAULT_BLOCK_SIZE must be non-zero"
);

_Static_assert(
    VITTE_ARENA_LARGE_ALLOCATION_DIVISOR != 0u,
    "VITTE_ARENA_LARGE_ALLOCATION_DIVISOR must be non-zero"
);

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte arena requires a 64-bit uint64_t"
);

_Static_assert(
    sizeof(uintptr_t) >= sizeof(void *),
    "uintptr_t must represent object pointers"
);

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

    /*
     * Ensures that the flexible data member begins after storage whose
     * alignment is at least max_align_t.
     *
     * Larger requested alignments are handled by address rounding inside the
     * data region.
     */
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

static size_t
vitte_arena_normalize_alignment(
    size_t alignment)
{
    if (alignment == 0u) {
        return _Alignof(max_align_t);
    }

    return alignment;
}

static bool
vitte_arena_alignment_valid(
    size_t alignment)
{
    if (!vitte_arena_is_power_of_two(
            alignment)) {
        return false;
    }

    /*
     * Alignment participates in uintptr_t address arithmetic.
     */
    if (alignment > (size_t)UINTPTR_MAX) {
        return false;
    }

    return true;
}

static bool
vitte_arena_align_up(
    uintptr_t value,
    size_t alignment,
    uintptr_t *result)
{
    uintptr_t mask;

    if (result == NULL ||
        !vitte_arena_alignment_valid(
            alignment)) {
        return false;
    }

    mask =
        (uintptr_t)(alignment - 1u);

    if (value > UINTPTR_MAX - mask) {
        return false;
    }

    *result =
        (value + mask) & ~mask;

    return true;
}

/* ========================================================================= */
/* Error handling                                                            */
/* ========================================================================= */

static bool
vitte_arena_magic_valid(
    const vitte_arena_t *arena)
{
    return arena != NULL &&
           arena->magic == VITTE_ARENA_MAGIC;
}

static void
vitte_arena_set_error(
    vitte_arena_t *arena,
    vitte_arena_error_t error)
{
    if (vitte_arena_magic_valid(arena)) {
        arena->last_error = error;
    }
}

static void
vitte_arena_set_success(
    vitte_arena_t *arena)
{
    if (vitte_arena_magic_valid(arena)) {
        arena->last_error =
            VITTE_ARENA_ERROR_NONE;
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
    vitte_arena_set_success(arena);
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
/* Block validation                                                          */
/* ========================================================================= */

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

    if (block->used >
        block->capacity) {
        return false;
    }

    return true;
}

static bool
vitte_arena_block_can_fit(
    const vitte_arena_block_t *block,
    size_t size,
    size_t alignment)
{
    uintptr_t base;
    uintptr_t current;
    uintptr_t aligned;
    uintptr_t offset_address;
    size_t offset;
    size_t end;

    if (!vitte_arena_block_valid(block) ||
        !vitte_arena_alignment_valid(
            alignment)) {
        return false;
    }

    base =
        (uintptr_t)block->data;

    if ((uintptr_t)block->used >
        UINTPTR_MAX - base) {
        return false;
    }

    current =
        base + (uintptr_t)block->used;

    if (!vitte_arena_align_up(
            current,
            alignment,
            &aligned)) {
        return false;
    }

    if (aligned < base) {
        return false;
    }

    offset_address =
        aligned - base;

    if (offset_address >
        (uintptr_t)SIZE_MAX) {
        return false;
    }

    offset =
        (size_t)offset_address;

    if (vitte_arena_add_overflow(
            offset,
            size,
            &end)) {
        return false;
    }

    return end <= block->capacity;
}

/* ========================================================================= */
/* Poisoning                                                                 */
/* ========================================================================= */

static void
vitte_arena_poison_allocated(
    void *memory,
    size_t size)
{
#if VITTE_ARENA_POISON_ALLOCATED
    if (memory != NULL &&
        size != 0u) {
        memset(
            memory,
            VITTE_ARENA_ALLOC_PATTERN,
            size);
    }
#else
    (void)memory;
    (void)size;
#endif
}

static void
vitte_arena_poison_reset_region(
    unsigned char *memory,
    size_t size)
{
#if VITTE_ARENA_POISON_RESET
    if (memory != NULL &&
        size != 0u) {
        memset(
            memory,
            VITTE_ARENA_RESET_PATTERN,
            size);
    }
#else
    (void)memory;
    (void)size;
#endif
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

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    if (capacity == 0u) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_ARGUMENT);

        return NULL;
    }

    if (vitte_arena_add_overflow(
            sizeof(vitte_arena_block_t),
            capacity,
            &total_size)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    block =
        (vitte_arena_block_t *)
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
        arena->generation;

    block->dedicated =
        dedicated;

    vitte_arena_poison_reset_region(
        block->data,
        block->capacity);

    return block;
}

static void
vitte_arena_block_destroy(
    vitte_arena_block_t *block)
{
    size_t capacity;

    if (block == NULL) {
        return;
    }

    if (block->magic !=
        VITTE_ARENA_BLOCK_MAGIC) {
        return;
    }

    capacity =
        block->capacity;

    vitte_arena_poison_reset_region(
        block->data,
        capacity);

    block->magic =
        VITTE_ARENA_DEAD_BLOCK_MAGIC;

    block->next = NULL;
    block->previous = NULL;

    block->capacity = 0u;
    block->used = 0u;

    block->generation = 0u;
    block->dedicated = false;

    free(block);
}

/* ========================================================================= */
/* Statistics helpers                                                        */
/* ========================================================================= */

static void
vitte_arena_update_peak_blocks(
    vitte_arena_t *arena)
{
    if (arena->block_count >
        arena->peak_block_count) {
        arena->peak_block_count =
            arena->block_count;
    }
}

static void
vitte_arena_update_peak_reserved(
    vitte_arena_t *arena)
{
    if (arena->reserved_bytes >
        arena->peak_reserved_bytes) {
        arena->peak_reserved_bytes =
            arena->reserved_bytes;
    }
}

static void
vitte_arena_update_peak_used(
    vitte_arena_t *arena)
{
    if (arena->used_bytes >
        arena->peak_used_bytes) {
        arena->peak_used_bytes =
            arena->used_bytes;
    }
}

static void
vitte_arena_record_request(
    vitte_arena_t *arena,
    size_t requested_size)
{
    if (arena->requested_bytes >
        SIZE_MAX - requested_size) {
        arena->requested_bytes =
            SIZE_MAX;
    } else {
        arena->requested_bytes +=
            requested_size;
    }

    if (arena->allocation_count !=
        SIZE_MAX) {
        ++arena->allocation_count;
    }
}

/* ========================================================================= */
/* Linked-list helpers                                                       */
/* ========================================================================= */

static bool
vitte_arena_append_block(
    vitte_arena_t *arena,
    vitte_arena_block_t *block)
{
    size_t new_reserved;

    if (!vitte_arena_magic_valid(arena) ||
        !vitte_arena_block_valid(block)) {
        if (vitte_arena_magic_valid(arena)) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);
        }

        return false;
    }

    if (arena->block_count ==
        SIZE_MAX) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return false;
    }

    if (vitte_arena_add_overflow(
            arena->reserved_bytes,
            block->capacity,
            &new_reserved)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return false;
    }

    block->previous =
        arena->last_block;

    block->next = NULL;

    if (arena->last_block != NULL) {
        if (!vitte_arena_block_valid(
                arena->last_block)) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            block->previous = NULL;
            return false;
        }

        arena->last_block->next =
            block;
    } else {
        arena->first_block =
            block;
    }

    arena->last_block =
        block;

    arena->current_block =
        block;

    ++arena->block_count;

    arena->reserved_bytes =
        new_reserved;

    vitte_arena_update_peak_blocks(
        arena);

    vitte_arena_update_peak_reserved(
        arena);

    return true;
}

static bool
vitte_arena_unlink_block(
    vitte_arena_t *arena,
    vitte_arena_block_t *block)
{
    vitte_arena_block_t *replacement;

    if (!vitte_arena_magic_valid(arena) ||
        !vitte_arena_block_valid(block)) {
        if (vitte_arena_magic_valid(arena)) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);
        }

        return false;
    }

    if (block->used >
        arena->used_bytes ||
        block->capacity >
        arena->reserved_bytes ||
        arena->block_count == 0u) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_CORRUPTION);

        return false;
    }

    replacement =
        block->previous != NULL
            ? block->previous
            : block->next;

    if (block->previous != NULL) {
        if (!vitte_arena_block_valid(
                block->previous) ||
            block->previous->next != block) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return false;
        }

        block->previous->next =
            block->next;
    } else {
        if (arena->first_block != block) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return false;
        }

        arena->first_block =
            block->next;
    }

    if (block->next != NULL) {
        if (!vitte_arena_block_valid(
                block->next) ||
            block->next->previous != block) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return false;
        }

        block->next->previous =
            block->previous;
    } else {
        if (arena->last_block != block) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return false;
        }

        arena->last_block =
            block->previous;
    }

    if (arena->current_block == block) {
        arena->current_block =
            replacement;
    }

    --arena->block_count;

    arena->reserved_bytes -=
        block->capacity;

    arena->used_bytes -=
        block->used;

    block->next = NULL;
    block->previous = NULL;

    return true;
}

/* ========================================================================= */
/* Allocation inside a block                                                 */
/* ========================================================================= */

static void *
vitte_arena_try_allocate_from_block(
    vitte_arena_t *arena,
    vitte_arena_block_t *block,
    size_t size,
    size_t alignment,
    size_t *consumed_bytes)
{
    uintptr_t base;
    uintptr_t current;
    uintptr_t aligned;
    uintptr_t offset_address;

    size_t old_used;
    size_t offset;
    size_t end;

    if (consumed_bytes != NULL) {
        *consumed_bytes = 0u;
    }

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    if (!vitte_arena_block_valid(block)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_CORRUPTION);

        return NULL;
    }

    if (!vitte_arena_alignment_valid(
            alignment)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_ALIGNMENT);

        return NULL;
    }

    old_used =
        block->used;

    base =
        (uintptr_t)block->data;

    if ((uintptr_t)old_used >
        UINTPTR_MAX - base) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    current =
        base + (uintptr_t)old_used;

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
            VITTE_ARENA_ERROR_CORRUPTION);

        return NULL;
    }

    offset_address =
        aligned - base;

    if (offset_address >
        (uintptr_t)SIZE_MAX) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    offset =
        (size_t)offset_address;

    if (vitte_arena_add_overflow(
            offset,
            size,
            &end)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    if (end >
        block->capacity) {
        /*
         * A normal capacity miss is not an error.
         */
        return NULL;
    }

    block->used =
        end;

    if (consumed_bytes != NULL) {
        *consumed_bytes =
            end - old_used;
    }

    return (void *)aligned;
}

/* ========================================================================= */
/* Reusable block lookup                                                     */
/* ========================================================================= */

static vitte_arena_block_t *
vitte_arena_find_reusable_block(
    vitte_arena_t *arena,
    size_t size,
    size_t alignment)
{
    vitte_arena_block_t *block;

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    /*
     * Prefer blocks after the current allocation cursor. This gives reset and
     * rewind a deterministic forward reuse order.
     */
    block =
        arena->current_block != NULL
            ? arena->current_block->next
            : arena->first_block;

    while (block != NULL) {
        if (!vitte_arena_block_valid(block)) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return NULL;
        }

        if (!block->dedicated &&
            block->used == 0u &&
            vitte_arena_block_can_fit(
                block,
                size,
                alignment)) {
            return block;
        }

        block =
            block->next;
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

    /*
     * init() expects uninitialized or otherwise disposable storage.
     * Calling init() on a live arena without destroy() would lose ownership
     * of its blocks, so callers must respect the lifecycle contract.
     */
    memset(
        arena,
        0,
        sizeof(*arena));

    if (block_size == 0u) {
        block_size =
            VITTE_ARENA_DEFAULT_BLOCK_SIZE;
    }

    if (block_size <
        VITTE_ARENA_MIN_BLOCK_SIZE) {
        block_size =
            VITTE_ARENA_MIN_BLOCK_SIZE;
    }

    arena->magic =
        VITTE_ARENA_MAGIC;

    arena->block_size =
        block_size;

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

    arena->generation =
        UINT64_C(1);

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

    block =
        arena->first_block;

    while (block != NULL) {
        vitte_arena_block_t *next;

        if (!vitte_arena_block_valid(block)) {
            /*
             * At this point following arbitrary corrupted links is unsafe.
             * Stop rather than turning corruption into an invalid free.
             */
            break;
        }

        next =
            block->next;

        vitte_arena_block_destroy(
            block);

        block =
            next;
    }

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

    arena->block_size = 0u;
    arena->generation = 0u;

    arena->last_error =
        VITTE_ARENA_ERROR_INVALID_ARENA;

    arena->magic =
        VITTE_ARENA_DEAD_MAGIC;
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

    size_t requested_size;
    size_t effective_size;
    size_t minimum_capacity;
    size_t consumed;

    bool dedicated;

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    vitte_arena_set_success(
        arena);

    requested_size =
        size;

    /*
     * Preserve a useful zero-size allocation contract while keeping
     * requested_bytes semantically equal to the caller's request.
     */
    effective_size =
        size == 0u
            ? 1u
            : size;

    alignment =
        vitte_arena_normalize_alignment(
            alignment);

    if (!vitte_arena_alignment_valid(
            alignment)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_ALIGNMENT);

        return NULL;
    }

    if (vitte_arena_add_overflow(
            effective_size,
            alignment - 1u,
            &minimum_capacity)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_OVERFLOW);

        return NULL;
    }

    /*
     * Fast path: allocate from current normal block.
     */
    block =
        arena->current_block;

    if (block != NULL) {
        if (!vitte_arena_block_valid(block)) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return NULL;
        }

        if (!block->dedicated) {
            consumed = 0u;

            memory =
                vitte_arena_try_allocate_from_block(
                    arena,
                    block,
                    effective_size,
                    alignment,
                    &consumed);

            if (memory != NULL) {
                if (arena->used_bytes >
                    SIZE_MAX - consumed) {
                    /*
                     * This should be impossible if the arena accounting is
                     * structurally valid, so classify it as corruption.
                     */
                    block->used -= consumed;

                    vitte_arena_set_error(
                        arena,
                        VITTE_ARENA_ERROR_CORRUPTION);

                    return NULL;
                }

                arena->used_bytes +=
                    consumed;

                goto allocation_success;
            }

            if (arena->last_error !=
                VITTE_ARENA_ERROR_NONE) {
                return NULL;
            }
        }
    }

    /*
     * Reuse a retained empty normal block after reset/rewind.
     */
    block =
        vitte_arena_find_reusable_block(
            arena,
            effective_size,
            alignment);

    if (block != NULL) {
        arena->current_block =
            block;

        consumed = 0u;

        memory =
            vitte_arena_try_allocate_from_block(
                arena,
                block,
                effective_size,
                alignment,
                &consumed);

        if (memory == NULL) {
            if (arena->last_error ==
                VITTE_ARENA_ERROR_NONE) {
                vitte_arena_set_error(
                    arena,
                    VITTE_ARENA_ERROR_CORRUPTION);
            }

            return NULL;
        }

        if (arena->used_bytes >
            SIZE_MAX - consumed) {
            block->used -=
                consumed;

            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return NULL;
        }

        arena->used_bytes +=
            consumed;

        goto allocation_success;
    }

    if (arena->last_error !=
        VITTE_ARENA_ERROR_NONE) {
        return NULL;
    }

    /*
     * Large requests receive a dedicated block.
     *
     * Avoid division surprises for pathological configurations by using the
     * configured divisor only after its compile-time non-zero assertion.
     */
    dedicated =
        minimum_capacity >
            arena->block_size /
                VITTE_ARENA_LARGE_ALLOCATION_DIVISOR;

    if (!dedicated &&
        minimum_capacity <
            arena->block_size) {
        minimum_capacity =
            arena->block_size;
    }

    block =
        vitte_arena_block_create(
            arena,
            minimum_capacity,
            dedicated);

    if (block == NULL) {
        return NULL;
    }

    if (!vitte_arena_append_block(
            arena,
            block)) {
        vitte_arena_block_destroy(
            block);

        return NULL;
    }

    consumed = 0u;

    memory =
        vitte_arena_try_allocate_from_block(
            arena,
            block,
            effective_size,
            alignment,
            &consumed);

    if (memory == NULL) {
        vitte_arena_error_t error;

        error =
            arena->last_error;

        /*
         * A newly sized block must fit this allocation. Failure without an
         * explicit arithmetic error means an internal structural invariant
         * was violated.
         */
        if (error ==
            VITTE_ARENA_ERROR_NONE) {
            error =
                VITTE_ARENA_ERROR_CORRUPTION;
        }

        if (!vitte_arena_unlink_block(
                arena,
                block)) {
            /*
             * Preserve the stronger structural error from unlink if present.
             */
            error =
                arena->last_error;
        }

        vitte_arena_block_destroy(
            block);

        vitte_arena_set_error(
            arena,
            error);

        return NULL;
    }

    if (arena->used_bytes >
        SIZE_MAX - consumed) {
        vitte_arena_error_t error;

        block->used -=
            consumed;

        error =
            VITTE_ARENA_ERROR_CORRUPTION;

        if (!vitte_arena_unlink_block(
                arena,
                block)) {
            error =
                arena->last_error;
        }

        vitte_arena_block_destroy(
            block);

        vitte_arena_set_error(
            arena,
            error);

        return NULL;
    }

    arena->used_bytes +=
        consumed;

allocation_success:

    vitte_arena_update_peak_used(
        arena);

    vitte_arena_record_request(
        arena,
        requested_size);

    vitte_arena_poison_allocated(
        memory,
        effective_size);

    vitte_arena_set_success(
        arena);

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
/* Count / typed allocation                                                  */
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

    vitte_arena_set_success(
        arena);

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

    vitte_arena_set_success(
        arena);

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

    /*
     * alloc(0) reserves one internal byte, but calloc(0, ...) has zero
     * logical bytes to initialize.
     */
    if (size != 0u) {
        memset(
            memory,
            0,
            size);
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

    vitte_arena_set_success(
        arena);

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
        memset(
            memory,
            0,
            size);
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

    vitte_arena_set_success(
        arena);

    if (data == NULL &&
        size != 0u) {
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
        memcpy(
            copy,
            data,
            size);
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

    vitte_arena_set_success(
        arena);

    if (text == NULL) {
        if (length != 0u) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_INVALID_ARGUMENT);

            return NULL;
        }

        /*
         * NULL + zero length is treated as duplication of an empty string.
         */
        text = "";
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

    copy =
        (char *)
            vitte_arena_alloc_aligned(
                arena,
                allocation_size,
                _Alignof(char));

    if (copy == NULL) {
        return NULL;
    }

    if (length != 0u) {
        memcpy(
            copy,
            text,
            length);
    }

    copy[length] =
        '\0';

    return copy;
}

char *
vitte_arena_strdup(
    vitte_arena_t *arena,
    const char *text)
{
    size_t length;

    if (!vitte_arena_magic_valid(arena)) {
        return NULL;
    }

    vitte_arena_set_success(
        arena);

    if (text == NULL) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_ARGUMENT);

        return NULL;
    }

    length =
        strlen(text);

    return vitte_arena_strndup(
        arena,
        text,
        length);
}

/* ========================================================================= */
/* Marks                                                                     */
/* ========================================================================= */

vitte_arena_mark_t
vitte_arena_mark(
    const vitte_arena_t *arena)
{
    vitte_arena_mark_t mark;

    memset(
        &mark,
        0,
        sizeof(mark));

    if (!vitte_arena_magic_valid(arena)) {
        return mark;
    }

    mark.arena =
        arena;

    mark.block =
        arena->current_block;

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

static bool
vitte_arena_mark_block_reachable(
    const vitte_arena_t *arena,
    const vitte_arena_block_t *target)
{
    const vitte_arena_block_t *block;

    if (!vitte_arena_magic_valid(arena) ||
        target == NULL) {
        return false;
    }

    block =
        arena->first_block;

    while (block != NULL) {
        if (!vitte_arena_block_valid(block)) {
            return false;
        }

        if (block == target) {
            return true;
        }

        block =
            block->next;
    }

    return false;
}

bool
vitte_arena_rewind(
    vitte_arena_t *arena,
    vitte_arena_mark_t mark)
{
    vitte_arena_block_t *block;

    size_t recomputed_used;

    if (!vitte_arena_magic_valid(arena)) {
        return false;
    }

    vitte_arena_set_success(
        arena);

    if (mark.arena != arena ||
        mark.generation !=
            arena->generation) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_MARK);

        return false;
    }

    /*
     * A mark cannot describe statistics from the future.
     */
    if (mark.used_bytes >
            arena->used_bytes ||
        mark.requested_bytes >
            arena->requested_bytes ||
        mark.allocation_count >
            arena->allocation_count) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_MARK);

        return false;
    }

    if (mark.block == NULL) {
        if (mark.used != 0u ||
            mark.used_bytes != 0u) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_INVALID_MARK);

            return false;
        }

        block =
            arena->first_block;

        while (block != NULL) {
            if (!vitte_arena_block_valid(block)) {
                vitte_arena_set_error(
                    arena,
                    VITTE_ARENA_ERROR_CORRUPTION);

                return false;
            }

            block =
                block->next;
        }

        block =
            arena->first_block;

        while (block != NULL) {
            vitte_arena_poison_reset_region(
                block->data,
                block->used);

            block->used = 0u;

            block =
                block->next;
        }

        arena->current_block =
            arena->first_block;

        arena->used_bytes =
            mark.used_bytes;

        arena->requested_bytes =
            mark.requested_bytes;

        arena->allocation_count =
            mark.allocation_count;

        vitte_arena_set_success(
            arena);

        return true;
    }

    if (!vitte_arena_mark_block_reachable(
            arena,
            mark.block)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_MARK);

        return false;
    }

    if (!vitte_arena_block_valid(
            mark.block) ||
        mark.used >
            mark.block->used ||
        mark.used >
            mark.block->capacity) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_MARK);

        return false;
    }

    /*
     * Verify the complete tail before mutating anything. This makes rewind
     * transactional with respect to structural corruption detection.
     */
    block =
        mark.block->next;

    while (block != NULL) {
        if (!vitte_arena_block_valid(block)) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return false;
        }

        block =
            block->next;
    }

    /*
     * Recompute what used_bytes will become and verify the mark's stored
     * aggregate before committing.
     */
    recomputed_used = 0u;
    block = arena->first_block;

    while (block != NULL &&
           block != mark.block) {
        if (vitte_arena_add_overflow(
                recomputed_used,
                block->used,
                &recomputed_used)) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return false;
        }

        block =
            block->next;
    }

    if (block != mark.block ||
        vitte_arena_add_overflow(
            recomputed_used,
            mark.used,
            &recomputed_used)) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_CORRUPTION);

        return false;
    }

    if (recomputed_used !=
        mark.used_bytes) {
        vitte_arena_set_error(
            arena,
            VITTE_ARENA_ERROR_INVALID_MARK);

        return false;
    }

    if (mark.block->used >
        mark.used) {
        vitte_arena_poison_reset_region(
            mark.block->data +
                mark.used,
            mark.block->used -
                mark.used);
    }

    mark.block->used =
        mark.used;

    block =
        mark.block->next;

    while (block != NULL) {
        vitte_arena_poison_reset_region(
            block->data,
            block->used);

        block->used = 0u;

        block =
            block->next;
    }

    arena->current_block =
        mark.block;

    arena->used_bytes =
        mark.used_bytes;

    arena->requested_bytes =
        mark.requested_bytes;

    arena->allocation_count =
        mark.allocation_count;

    vitte_arena_set_success(
        arena);

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
    uint64_t next_generation;

    if (!vitte_arena_magic_valid(arena)) {
        return;
    }

    vitte_arena_set_success(
        arena);

    /*
     * Validate first so reset does not partially mutate a corrupted list.
     */
    block =
        arena->first_block;

    while (block != NULL) {
        if (!vitte_arena_block_valid(block)) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return;
        }

        block =
            block->next;
    }

    next_generation =
        arena->generation + UINT64_C(1);

    if (next_generation == 0u) {
        next_generation =
            UINT64_C(1);
    }

    block =
        arena->first_block;

    while (block != NULL) {
        vitte_arena_poison_reset_region(
            block->data,
            block->used);

        block->used = 0u;

        block->generation =
            next_generation;

        block =
            block->next;
    }

    arena->generation =
        next_generation;

    arena->current_block =
        arena->first_block;

    arena->used_bytes = 0u;
    arena->requested_bytes = 0u;
    arena->allocation_count = 0u;

    vitte_arena_set_success(
        arena);
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

    vitte_arena_set_success(
        arena);

    /*
     * Validate first to avoid partially freeing a structurally corrupt list.
     */
    block =
        arena->first_block;

    while (block != NULL) {
        if (!vitte_arena_block_valid(block)) {
            vitte_arena_set_error(
                arena,
                VITTE_ARENA_ERROR_CORRUPTION);

            return;
        }

        block =
            block->next;
    }

    block =
        arena->first_block;

    while (block != NULL) {
        vitte_arena_block_t *next;

        next =
            block->next;

        if (block->used == 0u) {
            if (!vitte_arena_unlink_block(
                    arena,
                    block)) {
                return;
            }

            vitte_arena_block_destroy(
                block);
        }

        block =
            next;
    }

    if (arena->block_count == 0u) {
        arena->first_block = NULL;
        arena->last_block = NULL;
        arena->current_block = NULL;
    } else if (arena->current_block == NULL) {
        arena->current_block =
            arena->last_block;
    }

    vitte_arena_set_success(
        arena);
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

    vitte_arena_reset(
        arena);

    if (arena->last_error !=
        VITTE_ARENA_ERROR_NONE) {
        return;
    }

    vitte_arena_trim(
        arena);
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_arena_stats_t
vitte_arena_stats(
    const vitte_arena_t *arena)
{
    vitte_arena_stats_t stats;

    memset(
        &stats,
        0,
        sizeof(stats));

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
        return UINT64_C(0);
    }

    return arena->generation;
}

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

static bool
vitte_arena_pointer_in_block_capacity(
    const vitte_arena_block_t *block,
    const void *pointer)
{
    uintptr_t begin;
    uintptr_t end;
    uintptr_t address;

    if (!vitte_arena_block_valid(block) ||
        pointer == NULL) {
        return false;
    }

    begin =
        (uintptr_t)block->data;

    if ((uintptr_t)block->capacity >
        UINTPTR_MAX - begin) {
        return false;
    }

    end =
        begin +
        (uintptr_t)block->capacity;

    address =
        (uintptr_t)pointer;

    return address >= begin &&
           address < end;
}

static bool
vitte_arena_pointer_in_block_live(
    const vitte_arena_block_t *block,
    const void *pointer)
{
    uintptr_t begin;
    uintptr_t end;
    uintptr_t address;

    if (!vitte_arena_block_valid(block) ||
        pointer == NULL ||
        block->used == 0u) {
        return false;
    }

    begin =
        (uintptr_t)block->data;

    if ((uintptr_t)block->used >
        UINTPTR_MAX - begin) {
        return false;
    }

    end =
        begin +
        (uintptr_t)block->used;

    address =
        (uintptr_t)pointer;

    return address >= begin &&
           address < end;
}

bool
vitte_arena_contains(
    const vitte_arena_t *arena,
    const void *pointer)
{
    const vitte_arena_block_t *block;

    if (!vitte_arena_magic_valid(arena) ||
        pointer == NULL) {
        return false;
    }

    block =
        arena->first_block;

    while (block != NULL) {
        if (!vitte_arena_block_valid(block)) {
            return false;
        }

        if (vitte_arena_pointer_in_block_capacity(
                block,
                pointer)) {
            return true;
        }

        block =
            block->next;
    }

    return false;
}

bool
vitte_arena_contains_live(
    const vitte_arena_t *arena,
    const void *pointer)
{
    const vitte_arena_block_t *block;

    if (!vitte_arena_magic_valid(arena) ||
        pointer == NULL) {
        return false;
    }

    block =
        arena->first_block;

    while (block != NULL) {
        if (!vitte_arena_block_valid(block)) {
            return false;
        }

        if (vitte_arena_pointer_in_block_live(
                block,
                pointer)) {
            return true;
        }

        block =
            block->next;
    }

    return false;
}

/* ========================================================================= */
/* Structural validation                                                     */
/* ========================================================================= */

bool
vitte_arena_validate(
    const vitte_arena_t *arena)
{
    const vitte_arena_block_t *block;
    const vitte_arena_block_t *previous;

    const vitte_arena_block_t *slow;
    const vitte_arena_block_t *fast;

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

    if (arena->generation == 0u) {
        return false;
    }

    if ((arena->first_block == NULL) !=
        (arena->last_block == NULL)) {
        return false;
    }

    if (arena->first_block == NULL) {
        if (arena->block_count != 0u ||
            arena->reserved_bytes != 0u ||
            arena->used_bytes != 0u ||
            arena->current_block != NULL) {
            return false;
        }

        if (arena->peak_block_count <
                arena->block_count ||
            arena->peak_reserved_bytes <
                arena->reserved_bytes ||
            arena->peak_used_bytes <
                arena->used_bytes) {
            return false;
        }

        return true;
    }

    if (arena->block_count == 0u ||
        arena->first_block->previous != NULL ||
        arena->last_block->next != NULL) {
        return false;
    }

    /*
     * Detect forward-list cycles before the full traversal.
     */
    slow =
        arena->first_block;

    fast =
        arena->first_block;

    while (fast != NULL &&
           fast->next != NULL) {
        if (!vitte_arena_block_valid(fast) ||
            !vitte_arena_block_valid(
                fast->next)) {
            return false;
        }

        slow =
            slow->next;

        fast =
            fast->next->next;

        if (slow != NULL &&
            slow == fast) {
            return false;
        }
    }

    block_count = 0u;
    reserved = 0u;
    used = 0u;

    previous = NULL;

    current_found =
        arena->current_block == NULL;

    block =
        arena->first_block;

    while (block != NULL) {
        size_t next_value;

        if (!vitte_arena_block_valid(block)) {
            return false;
        }

        if (block->previous !=
            previous) {
            return false;
        }

        if (previous != NULL &&
            previous->next != block) {
            return false;
        }

        if (block->generation == 0u ||
            block->generation >
                arena->generation) {
            return false;
        }

        if (block == arena->current_block) {
            current_found = true;
        }

        if (vitte_arena_add_overflow(
                reserved,
                block->capacity,
                &next_value)) {
            return false;
        }

        reserved =
            next_value;

        if (vitte_arena_add_overflow(
                used,
                block->used,
                &next_value)) {
            return false;
        }

        used =
            next_value;

        if (block_count ==
            SIZE_MAX) {
            return false;
        }

        ++block_count;

        previous =
            block;

        block =
            block->next;
    }

    if (previous !=
        arena->last_block) {
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

    /*
     * requested_bytes may be saturated at SIZE_MAX, therefore the normal
     * used >= requested relation cannot be required after saturation.
     */
    if (arena->requested_bytes != SIZE_MAX &&
        arena->requested_bytes >
            arena->used_bytes) {
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
/* Internal design invariants                                                */
/* ========================================================================= */

/*
 * A1 — ownership
 *
 * Every block reachable from first_block is owned exclusively by the arena.
 *
 *
 * A2 — list consistency
 *
 * first_block->previous == NULL
 * last_block->next       == NULL
 *
 * and each adjacent pair is linked in both directions.
 *
 *
 * A3 — accounting
 *
 * reserved_bytes == sum(block.capacity)
 * used_bytes     == sum(block.used)
 *
 * for every structurally valid live arena.
 *
 *
 * A4 — block bounds
 *
 * block.used <= block.capacity
 *
 *
 * A5 — allocation lifetime
 *
 * Individual allocations cannot be freed independently.
 *
 *
 * A6 — alignment
 *
 * Requested alignment must be a non-zero power of two after normalization.
 * Zero means _Alignof(max_align_t).
 *
 *
 * A7 — zero-size allocations
 *
 * A logical zero-byte request consumes one internal byte so a successful call
 * returns a usable arena-owned address. requested_bytes records zero bytes.
 *
 *
 * A8 — dedicated blocks
 *
 * Dedicated blocks are never selected by the normal reuse path.
 *
 *
 * A9 — reset
 *
 * reset invalidates all previous marks by changing generation.
 *
 *
 * A10 — rewind
 *
 * rewind preserves generation so older marks from the same generation remain
 * meaningful only when their block/state is still reachable and compatible.
 *
 *
 * A11 — peaks
 *
 * reset, rewind and trim never decrease peak statistics.
 *
 *
 * A12 — requested bytes
 *
 * requested_bytes records logical caller-requested bytes, not internal
 * zero-size normalization or alignment padding.
 *
 *
 * A13 — no hidden allocation metadata
 *
 * Allocations contain no prefix/header. All ownership is block based.
 *
 *
 * A14 — corruption policy
 *
 * Operations avoid continuing through known-invalid block metadata whenever
 * doing so could cause invalid pointer traversal or invalid free.
 *
 *
 * A15 — thread safety
 *
 * A single vitte_arena_t is not internally synchronized. Independent arenas
 * may be used concurrently by independent threads.
 */

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
