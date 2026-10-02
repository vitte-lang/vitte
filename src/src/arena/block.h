#ifndef VITTE_SRC_ARENA_BLOCK_H
#define VITTE_SRC_ARENA_BLOCK_H

/*
 * Vitte Compiler
 * src/arena/block.h
 *
 * Physical arena block abstraction.
 *
 * Architecture:
 *
 *     block.c/.h
 *         physical memory block
 *         aligned bump allocation
 *         block-local accounting
 *
 *     allocator.c/.h
 *         block-chain management
 *         allocation policy
 *         mark/rewind
 *
 *     arena.c/.h
 *         compiler-facing high-level arena
 *         limits/checkpoints/statistics
 *
 * A block owns one contiguous allocation:
 *
 *     +-----------------------------+
 *     | vitte_arena_block_t header  |
 *     +-----------------------------+
 *     | flexible payload            |
 *     |                             |
 *     | used bytes                  |
 *     | --------------------------- |
 *     | unused capacity             |
 *     |                             |
 *     +-----------------------------+
 *
 * Allocation inside a block is monotonic. Individual allocations cannot be
 * freed. A block may instead be rewound or reset.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_ARENA_BLOCK_MIN_CAPACITY
#define VITTE_ARENA_BLOCK_MIN_CAPACITY \
    ((size_t)1024u)
#endif

#ifndef VITTE_ARENA_BLOCK_DEFAULT_CAPACITY
#define VITTE_ARENA_BLOCK_DEFAULT_CAPACITY \
    ((size_t)64u * (size_t)1024u)
#endif

/*
 * Debug memory poisoning.
 *
 * Keep disabled for normal/release builds.
 */
#ifndef VITTE_ARENA_BLOCK_POISON_ALLOC
#define VITTE_ARENA_BLOCK_POISON_ALLOC 0
#endif

#ifndef VITTE_ARENA_BLOCK_POISON_RESET
#define VITTE_ARENA_BLOCK_POISON_RESET 0
#endif

#ifndef VITTE_ARENA_BLOCK_POISON_DESTROY
#define VITTE_ARENA_BLOCK_POISON_DESTROY 0
#endif

#ifndef VITTE_ARENA_BLOCK_ALLOC_PATTERN
#define VITTE_ARENA_BLOCK_ALLOC_PATTERN 0xCD
#endif

#ifndef VITTE_ARENA_BLOCK_RESET_PATTERN
#define VITTE_ARENA_BLOCK_RESET_PATTERN 0xDD
#endif

#ifndef VITTE_ARENA_BLOCK_DESTROY_PATTERN
#define VITTE_ARENA_BLOCK_DESTROY_PATTERN 0xDE
#endif

/* ========================================================================= */
/* Block                                                                     */
/* ========================================================================= */

typedef struct vitte_arena_block
    vitte_arena_block_t;

/*
 * Physical arena block.
 *
 * The structure is intentionally visible to allocator.c because the allocator
 * manages the doubly-linked block chain directly.
 *
 * Other compiler subsystems should normally use allocator.h or arena.h rather
 * than manipulating this structure.
 */
struct vitte_arena_block {
    /*
     * Runtime integrity cookie.
     */
    uint64_t magic;

    /*
     * Allocator-owned doubly-linked chain.
     */
    vitte_arena_block_t *next;
    vitte_arena_block_t *previous;

    /*
     * Payload capacity in bytes.
     */
    size_t capacity;

    /*
     * Number of payload bytes consumed.
     *
     * Includes alignment padding.
     */
    size_t used;

    /*
     * Arena generation this block currently belongs to.
     */
    uint64_t generation;

    /*
     * Dedicated blocks are generally created for unusually large
     * allocations instead of using the normal reusable block size.
     */
    bool dedicated;

    /*
     * Successful allocations performed in this block since its last reset.
     */
    size_t allocation_count;

    /*
     * Logical caller-requested bytes since the last reset.
     *
     * Does not include alignment padding.
     *
     * A zero-sized request contributes zero here even though the allocator may
     * consume one physical byte to provide a usable address.
     */
    size_t requested_bytes;

    /*
     * Highest physical used offset observed during this block's lifetime.
     *
     * This survives reset.
     */
    size_t peak_used;

    /*
     * Force the header/flexible payload boundary to support max_align_t.
     *
     * The flexible array follows this member.
     */
    max_align_t alignment_anchor;

    /*
     * Block payload.
     */
    unsigned char data[];
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_arena_block_stats {
    /*
     * Total payload capacity.
     */
    size_t capacity;

    /*
     * Current physical payload consumption.
     */
    size_t used;

    /*
     * capacity - used.
     */
    size_t remaining;

    /*
     * Logical requested bytes.
     */
    size_t requested_bytes;

    /*
     * Alignment/internal padding.
     */
    size_t padding_bytes;

    /*
     * Successful allocations since reset.
     */
    size_t allocation_count;

    /*
     * Lifetime high-water physical usage.
     */
    size_t peak_used;

    /*
     * Current block generation.
     */
    uint64_t generation;

    /*
     * Large/dedicated block classification.
     */
    bool dedicated;
} vitte_arena_block_stats_t;

/* ========================================================================= */
/* Capacity                                                                  */
/* ========================================================================= */

/*
 * Normalize a requested block capacity.
 *
 * Rules:
 *
 *     0
 *         -> VITTE_ARENA_BLOCK_DEFAULT_CAPACITY
 *
 *     < VITTE_ARENA_BLOCK_MIN_CAPACITY
 *         -> VITTE_ARENA_BLOCK_MIN_CAPACITY
 *
 *     otherwise
 *         -> unchanged
 */
size_t
vitte_arena_block_normalize_capacity(
    size_t capacity);

/*
 * Calculate the worst-case block payload capacity required for an allocation.
 *
 * Equivalent conceptually to:
 *
 *     size + alignment - 1
 *
 * with overflow checking.
 *
 * alignment == 0 selects _Alignof(max_align_t).
 */
bool
vitte_arena_block_required_capacity(
    size_t size,
    size_t alignment,
    size_t *result);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Allocate a physical block.
 *
 * capacity is normalized before allocation.
 *
 * generation identifies the owning arena generation.
 *
 * dedicated marks a block intended for a large/special allocation.
 *
 * Returns NULL on overflow or malloc failure.
 */
vitte_arena_block_t *
vitte_arena_block_create(
    size_t capacity,
    uint64_t generation,
    bool dedicated);

/*
 * Destroy a block.
 *
 * block == NULL is accepted.
 *
 * The pointer becomes invalid immediately after this call.
 */
void
vitte_arena_block_destroy(
    vitte_arena_block_t *block);

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

/*
 * Perform aligned bump allocation inside a block.
 *
 * alignment must be a power of two.
 *
 * alignment == 0 selects _Alignof(max_align_t).
 *
 * A zero-sized request consumes one physical byte but contributes zero logical
 * requested bytes.
 *
 * Returns NULL when:
 *
 *   - block is invalid
 *   - alignment is invalid
 *   - arithmetic overflows
 *   - insufficient capacity remains
 */
void *
vitte_arena_block_alloc(
    vitte_arena_block_t *block,
    size_t size,
    size_t alignment);

/*
 * Test whether an allocation would fit without modifying the block.
 */
bool
vitte_arena_block_can_allocate(
    const vitte_arena_block_t *block,
    size_t size,
    size_t alignment);

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

/*
 * Reset the block for reuse.
 *
 * Current allocations become invalid.
 *
 * generation is replaced with the supplied generation.
 *
 * Current allocation_count/requested_bytes are cleared.
 *
 * peak_used is retained.
 */
void
vitte_arena_block_reset(
    vitte_arena_block_t *block,
    uint64_t generation);

/*
 * Move the physical bump pointer backwards.
 *
 * This function only restores the physical used offset.
 *
 * Block-local allocation_count and requested_bytes cannot be reconstructed
 * exactly without per-allocation metadata, so transactional accounting belongs
 * to allocator/arena checkpoints.
 */
bool
vitte_arena_block_rewind(
    vitte_arena_block_t *block,
    size_t used);

/* ========================================================================= */
/* Capacity queries                                                          */
/* ========================================================================= */

size_t
vitte_arena_block_capacity(
    const vitte_arena_block_t *block);

size_t
vitte_arena_block_used(
    const vitte_arena_block_t *block);

size_t
vitte_arena_block_remaining(
    const vitte_arena_block_t *block);

size_t
vitte_arena_block_requested_bytes(
    const vitte_arena_block_t *block);

size_t
vitte_arena_block_allocation_count(
    const vitte_arena_block_t *block);

size_t
vitte_arena_block_peak_used(
    const vitte_arena_block_t *block);

uint64_t
vitte_arena_block_generation(
    const vitte_arena_block_t *block);

/* ========================================================================= */
/* Classification                                                            */
/* ========================================================================= */

bool
vitte_arena_block_is_dedicated(
    const vitte_arena_block_t *block);

bool
vitte_arena_block_is_empty(
    const vitte_arena_block_t *block);

bool
vitte_arena_block_is_full(
    const vitte_arena_block_t *block);

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

/*
 * Return true when pointer lies anywhere inside the block's payload capacity.
 *
 * This does not imply that the pointed byte is currently live.
 */
bool
vitte_arena_block_contains(
    const vitte_arena_block_t *block,
    const void *pointer);

/*
 * Return true when pointer lies inside [data, data + used).
 */
bool
vitte_arena_block_contains_live(
    const vitte_arena_block_t *block,
    const void *pointer);

/* ========================================================================= */
/* Offset conversion                                                         */
/* ========================================================================= */

/*
 * Convert an owned payload pointer to a block-relative byte offset.
 */
bool
vitte_arena_block_offset_of(
    const vitte_arena_block_t *block,
    const void *pointer,
    size_t *offset);

/*
 * Convert a block-relative payload offset to a pointer.
 *
 * offset must be strictly less than capacity.
 */
void *
vitte_arena_block_pointer_at(
    vitte_arena_block_t *block,
    size_t offset);

const void *
vitte_arena_block_pointer_at_const(
    const vitte_arena_block_t *block,
    size_t offset);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_arena_block_stats_t
vitte_arena_block_stats(
    const vitte_arena_block_t *block);

/*
 * Physical payload utilization in basis points.
 *
 *     0      =   0.00%
 *     5000   =  50.00%
 *     10000  = 100.00%
 */
uint32_t
vitte_arena_block_utilization_basis_points(
    const vitte_arena_block_t *block);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate structural block invariants.
 *
 * Checks:
 *
 *   - non-NULL block
 *   - magic cookie
 *   - non-zero capacity
 *   - used <= capacity
 *   - peak_used <= capacity
 *   - peak_used >= used
 *   - requested accounting sanity
 *   - payload alignment
 *   - no self-linked next/previous pointers
 *   - local next/previous symmetry
 *
 * Intended for:
 *
 *   - tests
 *   - fuzzing
 *   - ASan/UBSan
 *   - debug compiler builds
 */
bool
vitte_arena_block_validate(
    const vitte_arena_block_t *block);

/* ========================================================================= */
/* Convenience                                                               */
/* ========================================================================= */

static inline unsigned char *
vitte_arena_block_data(
    vitte_arena_block_t *block)
{
    if (block == NULL) {
        return NULL;
    }

    return block->data;
}

static inline const unsigned char *
vitte_arena_block_data_const(
    const vitte_arena_block_t *block)
{
    if (block == NULL) {
        return NULL;
    }

    return block->data;
}

static inline bool
vitte_arena_block_has_space(
    const vitte_arena_block_t *block)
{
    return
        vitte_arena_block_remaining(block) != 0u;
}

static inline size_t
vitte_arena_block_padding_bytes(
    const vitte_arena_block_t *block)
{
    size_t used;
    size_t requested;

    used =
        vitte_arena_block_used(block);

    requested =
        vitte_arena_block_requested_bytes(
            block);

    if (requested == SIZE_MAX) {
        return 0u;
    }

    return used >= requested
        ? used - requested
        : 0u;
}

/* ========================================================================= */
/* Linked-list helpers                                                       */
/* ========================================================================= */

/*
 * These helpers only manipulate block links.
 *
 * Global list invariants and arena counters remain allocator.c's
 * responsibility.
 */

static inline vitte_arena_block_t *
vitte_arena_block_next(
    vitte_arena_block_t *block)
{
    return block != NULL
        ? block->next
        : NULL;
}

static inline const vitte_arena_block_t *
vitte_arena_block_next_const(
    const vitte_arena_block_t *block)
{
    return block != NULL
        ? block->next
        : NULL;
}

static inline vitte_arena_block_t *
vitte_arena_block_previous(
    vitte_arena_block_t *block)
{
    return block != NULL
        ? block->previous
        : NULL;
}

static inline const vitte_arena_block_t *
vitte_arena_block_previous_const(
    const vitte_arena_block_t *block)
{
    return block != NULL
        ? block->previous
        : NULL;
}

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_ARENA_BLOCK_MIN_CAPACITY != 0u,
    "arena block minimum capacity must not be zero");

_Static_assert(
    VITTE_ARENA_BLOCK_DEFAULT_CAPACITY >=
        VITTE_ARENA_BLOCK_MIN_CAPACITY,
    "arena block default capacity must satisfy minimum capacity");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte arena blocks require 64-bit uint64_t");

_Static_assert(
    sizeof(uintptr_t) >= sizeof(void *),
    "uintptr_t must represent native pointers");

_Static_assert(
    _Alignof(max_align_t) != 0u,
    "max_align_t must have valid alignment");

_Static_assert(
    offsetof(
        vitte_arena_block_t,
        data) %
        _Alignof(max_align_t) ==
        0u,
    "arena block payload must be max_align_t aligned");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_BLOCK_H */
