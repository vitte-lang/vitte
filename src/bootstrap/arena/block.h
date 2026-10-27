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
 * Allocation is monotonic. Individual allocations cannot be freed.
 *
 * A block can be:
 *
 *   - rewound to an earlier physical offset
 *   - reset for reuse
 *   - destroyed by its owning allocator
 *
 * allocator.c owns block-chain topology and arena-wide accounting.
 *
 * ISO C17.
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
 * These options are intentionally disabled by default because poisoning
 * potentially large arena blocks can materially affect compiler performance.
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
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_arena_block
    vitte_arena_block_t;

typedef struct vitte_arena_block_stats
    vitte_arena_block_stats_t;

/* ========================================================================= */
/* Block                                                                     */
/* ========================================================================= */

/*
 * Physical arena block.
 *
 * The structure is visible because allocator.c manages the doubly-linked
 * chain directly.
 *
 * Other compiler subsystems should normally use allocator.h or arena.h.
 *
 * The flexible payload begins at data[] and is guaranteed to be suitably
 * aligned for max_align_t.
 */
struct vitte_arena_block {
    /*
     * Runtime integrity cookie.
     *
     * A live block contains the private VITTE_ARENA_BLOCK_MAGIC value used by
     * block.c. Destroyed blocks are poisoned with a different cookie before
     * being released.
     */
    uint64_t magic;

    /*
     * Allocator-owned doubly-linked chain.
     *
     * block.c performs only immediate link-symmetry validation.
     * allocator.c owns complete chain validation and cycle detection.
     */
    vitte_arena_block_t *next;
    vitte_arena_block_t *previous;

    /*
     * Total payload capacity in bytes.
     *
     * Does not include sizeof(vitte_arena_block_t).
     */
    size_t capacity;

    /*
     * Physical payload consumption.
     *
     * This is the bump-pointer offset from data[].
     *
     * It includes:
     *
     *   - requested bytes
     *   - alignment padding
     *   - the physical byte consumed by a zero-size allocation
     */
    size_t used;

    /*
     * Arena generation this block currently belongs to.
     *
     * allocator.c updates this during reset/reuse.
     */
    uint64_t generation;

    /*
     * Dedicated block classification.
     *
     * Dedicated blocks are normally created for allocations large enough that
     * consuming a normal reusable block would be undesirable.
     */
    bool dedicated;

    /*
     * Number of successful allocations since the most recent full block
     * reset.
     *
     * A zero-size allocation still increments this counter.
     *
     * This counter saturates at SIZE_MAX.
     *
     * IMPORTANT:
     *
     * vitte_arena_block_rewind() cannot reconstruct this value because blocks
     * intentionally contain no per-allocation metadata. Exact transactional
     * accounting is owned by allocator/arena checkpoints.
     */
    size_t allocation_count;

    /*
     * Sum of logical caller-requested bytes since the most recent full reset.
     *
     * Alignment padding is excluded.
     *
     * A zero-size allocation contributes zero even though one physical byte is
     * consumed.
     *
     * This counter saturates at SIZE_MAX.
     *
     * Like allocation_count, this value is not reconstructed by a raw
     * block-level rewind.
     */
    size_t requested_bytes;

    /*
     * Highest physical used offset observed during the lifetime of this
     * physical block.
     *
     * This value survives reset and rewind.
     */
    size_t peak_used;

    /*
     * Force the header and therefore the following flexible payload to have
     * at least max_align_t alignment.
     */
    max_align_t alignment_anchor;

    /*
     * Flexible payload.
     *
     * The allocation performed by vitte_arena_block_create() reserves
     *
     *     sizeof(vitte_arena_block_t) + capacity
     *
     * bytes.
     */
    unsigned char data[];
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

struct vitte_arena_block_stats {
    /*
     * Total payload capacity.
     */
    size_t capacity;

    /*
     * Current physical payload consumption.
     */
    size_t used;

    /*
     * Current immediately unused payload capacity.
     *
     * Equivalent to:
     *
     *     capacity - used
     */
    size_t remaining;

    /*
     * Logical requested-byte accounting.
     *
     * May equal SIZE_MAX if accounting saturated.
     */
    size_t requested_bytes;

    /*
     * Physical overhead represented by:
     *
     *     used - requested_bytes
     *
     * when requested_bytes has not saturated.
     *
     * This includes alignment padding and physical normalization of zero-byte
     * allocations.
     *
     * Zero is reported when exact reconstruction is impossible.
     */
    size_t padding_bytes;

    /*
     * Successful allocations since reset.
     *
     * May equal SIZE_MAX if accounting saturated.
     */
    size_t allocation_count;

    /*
     * Lifetime high-water physical usage for this physical block.
     */
    size_t peak_used;

    /*
     * Current owning arena generation.
     */
    uint64_t generation;

    /*
     * Dedicated/large-block classification.
     */
    bool dedicated;
};

/* ========================================================================= */
/* Capacity                                                                  */
/* ========================================================================= */

/*
 * Normalize a requested block capacity.
 *
 * Rules:
 *
 *     capacity == 0
 *         -> VITTE_ARENA_BLOCK_DEFAULT_CAPACITY
 *
 *     capacity < VITTE_ARENA_BLOCK_MIN_CAPACITY
 *         -> VITTE_ARENA_BLOCK_MIN_CAPACITY
 *
 *     otherwise
 *         -> capacity
 */
size_t
vitte_arena_block_normalize_capacity(
    size_t capacity);

/*
 * Calculate the worst-case payload capacity required for one allocation.
 *
 * For a non-zero request this is conceptually:
 *
 *     size + alignment - 1
 *
 * For a zero-size request the physical allocation size is normalized to one
 * byte, therefore this is conceptually:
 *
 *     1 + alignment - 1
 *
 * alignment == 0 selects _Alignof(max_align_t).
 *
 * Returns false when:
 *
 *   - result == NULL
 *   - alignment is not a power of two
 *   - arithmetic overflows
 *
 * On failure, result is set to zero whenever result itself is non-NULL.
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
 * Allocate and initialize a physical arena block.
 *
 * capacity is normalized using vitte_arena_block_normalize_capacity().
 *
 * generation is copied into the block and identifies its owning arena
 * generation.
 *
 * dedicated classifies the block for allocator policy.
 *
 * Block creation is lazy at the allocator layer; calling this function itself
 * performs one system allocation.
 *
 * Returns NULL when:
 *
 *   - header + payload size overflows size_t
 *   - malloc() fails
 *   - the resulting block fails structural validation
 */
vitte_arena_block_t *
vitte_arena_block_create(
    size_t capacity,
    uint64_t generation,
    bool dedicated);

/*
 * Destroy a physical block.
 *
 * block == NULL is accepted.
 *
 * A pointer whose magic cookie is not recognized as a live block is ignored.
 *
 * After successful destruction the pointer and every pointer into its payload
 * are invalid.
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
 * alignment:
 *
 *     0
 *         -> _Alignof(max_align_t)
 *
 *     power of two
 *         -> requested alignment
 *
 *     otherwise
 *         -> failure
 *
 * A zero-size logical request consumes one physical payload byte. This gives
 * successful zero-size allocations an arena-owned address while preserving
 * zero logical requested-byte accounting.
 *
 * Failed allocations do not modify block state.
 *
 * Successful allocations update:
 *
 *   - used
 *   - allocation_count
 *   - requested_bytes
 *   - peak_used
 *
 * Returns NULL when:
 *
 *   - block validation fails
 *   - alignment is invalid
 *   - address arithmetic overflows
 *   - offset arithmetic overflows
 *   - insufficient capacity remains
 */
void *
vitte_arena_block_alloc(
    vitte_arena_block_t *block,
    size_t size,
    size_t alignment);

/*
 * Test whether vitte_arena_block_alloc() could satisfy an allocation without
 * modifying the block.
 *
 * Zero-size and alignment semantics are identical to
 * vitte_arena_block_alloc().
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
 * All current payload allocations become invalid.
 *
 * The following state is reset:
 *
 *     used              -> 0
 *     generation        -> generation
 *     allocation_count  -> 0
 *     requested_bytes   -> 0
 *
 * peak_used intentionally survives because it describes the lifetime
 * high-water mark of the physical block.
 */
void
vitte_arena_block_reset(
    vitte_arena_block_t *block,
    uint64_t generation);

/*
 * Move the physical bump pointer backwards.
 *
 * used must satisfy:
 *
 *     used <= current block->used
 *
 * This function intentionally restores only the physical bump position.
 *
 * It does NOT reconstruct:
 *
 *   - allocation_count
 *   - requested_bytes
 *
 * Exact transactional accounting requires allocation metadata that blocks do
 * not store. allocator.c and arena.c checkpoints own that responsibility.
 *
 * peak_used is not decreased.
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

/*
 * Return true when the block contains no physically consumed payload.
 *
 * Invalid blocks are treated as empty by this convenience query.
 */
bool
vitte_arena_block_is_empty(
    const vitte_arena_block_t *block);

/*
 * Return true when used == capacity.
 *
 * An invalid block is never considered full.
 *
 * Note that a block may be unable to satisfy a particular aligned allocation
 * before this predicate becomes true. Use vitte_arena_block_can_allocate()
 * when allocation feasibility matters.
 */
bool
vitte_arena_block_is_full(
    const vitte_arena_block_t *block);

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

/*
 * Return true when pointer lies in:
 *
 *     [data, data + capacity)
 *
 * This only proves that the address belongs to the block's payload storage.
 *
 * It does not prove:
 *
 *   - that the byte is currently live
 *   - that pointer is an allocation start
 *   - that pointer has a particular alignment
 */
bool
vitte_arena_block_contains(
    const vitte_arena_block_t *block,
    const void *pointer);

/*
 * Return true when pointer lies in:
 *
 *     [data, data + used)
 *
 * This is stronger than vitte_arena_block_contains(), but still does not
 * prove that pointer is the start of an allocation.
 */
bool
vitte_arena_block_contains_live(
    const vitte_arena_block_t *block,
    const void *pointer);

/* ========================================================================= */
/* Offset conversion                                                         */
/* ========================================================================= */

/*
 * Convert an address belonging to the block payload into a byte offset from
 * data[].
 *
 * The pointer may refer to used or unused payload storage.
 *
 * Returns false if pointer is outside:
 *
 *     [data, data + capacity)
 *
 * On failure, *offset is set to zero when offset is non-NULL.
 */
bool
vitte_arena_block_offset_of(
    const vitte_arena_block_t *block,
    const void *pointer,
    size_t *offset);

/*
 * Convert a block-relative payload offset to an address.
 *
 * Valid offsets satisfy:
 *
 *     offset < capacity
 *
 * The returned pointer is not necessarily inside the currently live region.
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

/*
 * Return a block statistics snapshot.
 *
 * An all-zero structure is returned for an invalid block.
 */
vitte_arena_block_stats_t
vitte_arena_block_stats(
    const vitte_arena_block_t *block);

/*
 * Physical payload utilization in basis points.
 *
 *     0      =   0.00%
 *     5000   =  50.00%
 *     10000  = 100.00%
 *
 * The implementation avoids size_t multiplication overflow.
 */
uint32_t
vitte_arena_block_utilization_basis_points(
    const vitte_arena_block_t *block);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Perform an expensive structural integrity check.
 *
 * Validation includes:
 *
 *   - non-NULL block
 *   - live magic cookie
 *   - minimum/non-zero capacity
 *   - used <= capacity
 *   - peak_used <= capacity
 *   - peak_used >= used
 *   - requested-byte accounting sanity
 *   - allocation-count/used consistency
 *   - max_align_t payload alignment
 *   - representable payload address interval
 *   - no immediate self links
 *   - immediate next/previous link symmetry
 *
 * This function validates one block and its immediate links only.
 *
 * It does NOT replace allocator-level validation for:
 *
 *   - complete list topology
 *   - long-cycle detection
 *   - first/last/current membership
 *   - block-count reconciliation
 *   - reserved-byte reconciliation
 *   - arena-wide generation consistency
 *
 * Intended for:
 *
 *   - unit tests
 *   - fuzzing
 *   - ASan/UBSan builds
 *   - debug builds
 *   - compiler self-checks
 */
bool
vitte_arena_block_validate(
    const vitte_arena_block_t *block);

/* ========================================================================= */
/* Convenience                                                               */
/* ========================================================================= */

/*
 * Raw payload accessors.
 *
 * These deliberately do not perform full structural validation because they
 * are intended for low-level allocator internals. NULL is still handled.
 */
static inline unsigned char *
vitte_arena_block_data(
    vitte_arena_block_t *block)
{
    return
        block != NULL
            ? block->data
            : NULL;
}

static inline const unsigned char *
vitte_arena_block_data_const(
    const vitte_arena_block_t *block)
{
    return
        block != NULL
            ? block->data
            : NULL;
}

/*
 * Return true when at least one physical byte remains.
 *
 * This does not guarantee that a specific aligned allocation can fit.
 */
static inline bool
vitte_arena_block_has_space(
    const vitte_arena_block_t *block)
{
    return
        vitte_arena_block_remaining(
            block) != 0u;
}

/*
 * Return physical overhead currently attributable to alignment and zero-byte
 * allocation normalization.
 *
 * When requested-byte accounting saturated at SIZE_MAX, exact padding cannot
 * be reconstructed and zero is returned.
 */
static inline size_t
vitte_arena_block_padding_bytes(
    const vitte_arena_block_t *block)
{
    size_t used;
    size_t requested;

    used =
        vitte_arena_block_used(
            block);

    requested =
        vitte_arena_block_requested_bytes(
            block);

    if (requested == SIZE_MAX) {
        return 0u;
    }

    return
        used >= requested
            ? used - requested
            : 0u;
}

/*
 * Return true if an allocation with default alignment can fit.
 */
static inline bool
vitte_arena_block_can_allocate_default(
    const vitte_arena_block_t *block,
    size_t size)
{
    return
        vitte_arena_block_can_allocate(
            block,
            size,
            0u);
}

/*
 * Return true when pointer lies inside the currently live region.
 */
static inline bool
vitte_arena_block_owns_live_pointer(
    const vitte_arena_block_t *block,
    const void *pointer)
{
    return
        vitte_arena_block_contains_live(
            block,
            pointer);
}

/* ========================================================================= */
/* Linked-list access                                                        */
/* ========================================================================= */

/*
 * These accessors expose immediate allocator-owned topology.
 *
 * They do not validate the block and do not modify list invariants.
 */
static inline vitte_arena_block_t *
vitte_arena_block_next(
    vitte_arena_block_t *block)
{
    return
        block != NULL
            ? block->next
            : NULL;
}

static inline const vitte_arena_block_t *
vitte_arena_block_next_const(
    const vitte_arena_block_t *block)
{
    return
        block != NULL
            ? block->next
            : NULL;
}

static inline vitte_arena_block_t *
vitte_arena_block_previous(
    vitte_arena_block_t *block)
{
    return
        block != NULL
            ? block->previous
            : NULL;
}

static inline const vitte_arena_block_t *
vitte_arena_block_previous_const(
    const vitte_arena_block_t *block)
{
    return
        block != NULL
            ? block->previous
            : NULL;
}

/* ========================================================================= */
/* Linked-list mutation                                                      */
/* ========================================================================= */

/*
 * Link two adjacent blocks.
 *
 * These helpers intentionally perform only local pointer mutation.
 * allocator.c remains responsible for global topology and accounting.
 */
static inline void
vitte_arena_block_set_next(
    vitte_arena_block_t *block,
    vitte_arena_block_t *next)
{
    if (block != NULL) {
        block->next = next;
    }
}

static inline void
vitte_arena_block_set_previous(
    vitte_arena_block_t *block,
    vitte_arena_block_t *previous)
{
    if (block != NULL) {
        block->previous = previous;
    }
}

/*
 * Connect:
 *
 *     left <-> right
 *
 * Either side may be NULL.
 *
 * Existing opposite links are not repaired automatically. This is a primitive
 * intended for allocator.c, not a complete list-splice operation.
 */
static inline void
vitte_arena_block_link(
    vitte_arena_block_t *left,
    vitte_arena_block_t *right)
{
    if (left != NULL) {
        left->next = right;
    }

    if (right != NULL) {
        right->previous = left;
    }
}

/*
 * Clear this block's immediate links without modifying neighboring blocks.
 */
static inline void
vitte_arena_block_clear_links(
    vitte_arena_block_t *block)
{
    if (block == NULL) {
        return;
    }

    block->next = NULL;
    block->previous = NULL;
}

/* ========================================================================= */
/* Generation helpers                                                        */
/* ========================================================================= */

static inline bool
vitte_arena_block_belongs_to_generation(
    const vitte_arena_block_t *block,
    uint64_t generation)
{
    return
        block != NULL &&
        vitte_arena_block_generation(
            block) == generation;
}

/* ========================================================================= */
/* Utilization helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_arena_block_is_unused(
    const vitte_arena_block_t *block)
{
    return
        vitte_arena_block_used(
            block) == 0u;
}

static inline size_t
vitte_arena_block_live_bytes(
    const vitte_arena_block_t *block)
{
    return
        vitte_arena_block_used(
            block);
}

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

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
    sizeof(uintptr_t) >=
        sizeof(void *),
    "uintptr_t must represent native pointers");

_Static_assert(
    _Alignof(max_align_t) != 0u,
    "max_align_t must provide valid alignment");

_Static_assert(
    (_Alignof(max_align_t) &
     (_Alignof(max_align_t) - 1u)) == 0u,
    "max_align_t alignment must be a power of two");

_Static_assert(
    _Alignof(vitte_arena_block_t) >=
        _Alignof(max_align_t),
    "arena block structure must satisfy max_align_t alignment");

_Static_assert(
    offsetof(
        vitte_arena_block_t,
        data) %
        _Alignof(max_align_t) ==
        0u,
    "arena block payload must be max_align_t aligned");

_Static_assert(
    offsetof(
        vitte_arena_block_t,
        data) ==
        sizeof(vitte_arena_block_t),
    "arena block flexible payload must immediately follow the header");

_Static_assert(
    VITTE_ARENA_BLOCK_ALLOC_PATTERN >= 0x00 &&
    VITTE_ARENA_BLOCK_ALLOC_PATTERN <= 0xFF,
    "arena allocation poison pattern must fit in one byte");

_Static_assert(
    VITTE_ARENA_BLOCK_RESET_PATTERN >= 0x00 &&
    VITTE_ARENA_BLOCK_RESET_PATTERN <= 0xFF,
    "arena reset poison pattern must fit in one byte");

_Static_assert(
    VITTE_ARENA_BLOCK_DESTROY_PATTERN >= 0x00 &&
    VITTE_ARENA_BLOCK_DESTROY_PATTERN <= 0xFF,
    "arena destroy poison pattern must fit in one byte");

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_BLOCK_H */