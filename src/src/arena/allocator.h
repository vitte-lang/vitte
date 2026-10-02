#ifndef VITTE_SRC_ARENA_ALLOCATOR_H
#define VITTE_SRC_ARENA_ALLOCATOR_H

/*
 * Vitte Compiler
 * src/arena/allocator.h
 *
 * High-performance arena allocator.
 *
 * Properties:
 *
 *   - amortized O(1) allocation
 *   - arena ownership
 *   - arbitrary power-of-two alignment
 *   - overflow-safe array allocation
 *   - zero-initialized allocation
 *   - memory/string duplication
 *   - marks and rewind
 *   - reset and block reuse
 *   - optional memory trimming
 *   - detailed statistics
 *   - allocation ownership queries
 *   - structural validation
 *   - deterministic destruction
 *
 * Arena allocations have no individual free operation.
 *
 * Memory remains valid until one of:
 *
 *   - vitte_arena_rewind()
 *   - vitte_arena_reset()
 *   - vitte_arena_reset_and_trim()
 *   - vitte_arena_destroy()
 *
 * invalidates the allocation.
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

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_arena_block
    vitte_arena_block_t;

typedef struct vitte_arena
    vitte_arena_t;

typedef struct vitte_arena_mark
    vitte_arena_mark_t;

typedef struct vitte_arena_stats
    vitte_arena_stats_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

/*
 * Arena-local error classification.
 *
 * Allocation functions return NULL on failure and record one of these values
 * in the arena whenever possible.
 */
typedef enum vitte_arena_error {
    VITTE_ARENA_ERROR_NONE = 0,

    /*
     * Arena pointer/state is invalid.
     */
    VITTE_ARENA_ERROR_INVALID_ARENA,

    /*
     * Invalid input argument.
     */
    VITTE_ARENA_ERROR_INVALID_ARGUMENT,

    /*
     * Alignment is zero after normalization failure or is not a power of two.
     */
    VITTE_ARENA_ERROR_INVALID_ALIGNMENT,

    /*
     * Integer/address arithmetic overflow.
     */
    VITTE_ARENA_ERROR_OVERFLOW,

    /*
     * System allocator could not provide memory.
     */
    VITTE_ARENA_ERROR_OUT_OF_MEMORY,

    /*
     * Mark does not belong to this arena/current generation.
     */
    VITTE_ARENA_ERROR_INVALID_MARK,

    /*
     * Internal block/list invariant failure.
     */
    VITTE_ARENA_ERROR_CORRUPTION
} vitte_arena_error_t;

/* ========================================================================= */
/* Arena                                                                     */
/* ========================================================================= */

/*
 * Arena allocator state.
 *
 * This is an internal compiler structure rather than a stable external ABI.
 *
 * Compiler components may place it directly inside larger subsystem objects.
 */
struct vitte_arena {
    /*
     * Runtime validation cookie.
     */
    uint64_t magic;

    /*
     * Preferred capacity of ordinary blocks.
     */
    size_t block_size;

    /*
     * Doubly-linked block chain.
     */
    vitte_arena_block_t *first_block;
    vitte_arena_block_t *last_block;

    /*
     * Current allocation block.
     */
    vitte_arena_block_t *current_block;

    /*
     * Current and peak block counts.
     */
    size_t block_count;
    size_t peak_block_count;

    /*
     * Sum of block payload capacities.
     *
     * Does not include allocator bookkeeping/header bytes.
     */
    size_t reserved_bytes;
    size_t peak_reserved_bytes;

    /*
     * Bytes consumed from block payloads.
     *
     * Includes alignment padding.
     */
    size_t used_bytes;
    size_t peak_used_bytes;

    /*
     * Sum of caller-requested allocation sizes.
     *
     * Does not include alignment padding.
     */
    size_t requested_bytes;

    /*
     * Number of successful allocations in the current generation.
     */
    size_t allocation_count;

    /*
     * Arena generation.
     *
     * Incremented by reset so marks from previous generations cannot be
     * accidentally reused.
     */
    uint64_t generation;

    /*
     * Most recent arena error.
     */
    vitte_arena_error_t last_error;
};

/* ========================================================================= */
/* Mark                                                                      */
/* ========================================================================= */

/*
 * Arena checkpoint.
 *
 * A mark can be used to discard every allocation performed after the mark.
 *
 * Marks are tied to:
 *
 *   - one specific arena
 *   - one specific arena generation
 *
 * A mark becomes invalid after reset or destroy.
 */
struct vitte_arena_mark {
    /*
     * Arena that created this mark.
     */
    const vitte_arena_t *arena;

    /*
     * Allocation block active at mark creation.
     */
    vitte_arena_block_t *block;

    /*
     * Used offset inside block.
     */
    size_t used;

    /*
     * Arena accounting snapshot.
     */
    size_t used_bytes;
    size_t requested_bytes;
    size_t allocation_count;

    /*
     * Generation used to reject stale marks.
     */
    uint64_t generation;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

struct vitte_arena_stats {
    /*
     * Preferred normal block size.
     */
    size_t block_size;

    /*
     * Current block count.
     */
    size_t block_count;

    /*
     * Maximum block count observed.
     */
    size_t peak_block_count;

    /*
     * Current payload capacity reserved from malloc().
     */
    size_t reserved_bytes;

    /*
     * Maximum reserved payload capacity observed.
     */
    size_t peak_reserved_bytes;

    /*
     * Current payload bytes consumed.
     *
     * Includes alignment padding.
     */
    size_t used_bytes;

    /*
     * Maximum used payload bytes observed.
     */
    size_t peak_used_bytes;

    /*
     * Caller-requested bytes in the current generation.
     */
    size_t requested_bytes;

    /*
     * Current unused payload capacity.
     *
     * Equivalent to:
     *
     *     reserved_bytes - used_bytes
     */
    size_t unused_bytes;

    /*
     * Alignment/internal padding currently accounted for.
     *
     * Equivalent to:
     *
     *     used_bytes - requested_bytes
     *
     * when accounting has not saturated.
     */
    size_t padding_bytes;

    /*
     * Successful allocations in the current generation.
     */
    size_t allocation_count;

    /*
     * Current arena generation.
     */
    uint64_t generation;
};

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize an arena.
 *
 * block_size == 0 selects VITTE_ARENA_DEFAULT_BLOCK_SIZE.
 *
 * Values smaller than VITTE_ARENA_MIN_BLOCK_SIZE are raised to the minimum.
 *
 * Returns true on success.
 *
 * Initialization itself does not allocate a block; allocation remains lazy.
 */
bool
vitte_arena_init(
    vitte_arena_t *arena,
    size_t block_size);

/*
 * Initialize with the default block size.
 */
bool
vitte_arena_init_default(
    vitte_arena_t *arena);

/*
 * Destroy every block owned by the arena.
 *
 * Passing NULL or an invalid arena is harmless.
 *
 * All pointers previously returned by this arena become invalid.
 */
void
vitte_arena_destroy(
    vitte_arena_t *arena);

/* ========================================================================= */
/* Core allocation                                                           */
/* ========================================================================= */

/*
 * Allocate size bytes using max_align_t alignment.
 *
 * size == 0 is normalized internally to a one-byte allocation so successful
 * zero-size requests can still return a usable arena-owned address.
 *
 * Returns NULL on failure.
 */
void *
vitte_arena_alloc(
    vitte_arena_t *arena,
    size_t size);

/*
 * Allocate size bytes with explicit alignment.
 *
 * alignment must be a power of two.
 *
 * alignment == 0 selects _Alignof(max_align_t).
 *
 * Returns NULL on failure.
 */
void *
vitte_arena_alloc_aligned(
    vitte_arena_t *arena,
    size_t size,
    size_t alignment);

/* ========================================================================= */
/* Array allocation                                                          */
/* ========================================================================= */

/*
 * Allocate:
 *
 *     count * element_size
 *
 * bytes using explicit alignment.
 *
 * Multiplication overflow is detected before allocation.
 */
void *
vitte_arena_alloc_array(
    vitte_arena_t *arena,
    size_t count,
    size_t element_size,
    size_t alignment);

/*
 * Allocate count * element_size bytes using max_align_t alignment and
 * initialize the requested byte range to zero.
 */
void *
vitte_arena_calloc(
    vitte_arena_t *arena,
    size_t count,
    size_t element_size);

/*
 * Aligned variant of vitte_arena_calloc().
 */
void *
vitte_arena_calloc_aligned(
    vitte_arena_t *arena,
    size_t count,
    size_t element_size,
    size_t alignment);

/* ========================================================================= */
/* Duplication                                                               */
/* ========================================================================= */

/*
 * Copy size bytes into arena-owned storage.
 *
 * data may only be NULL when size == 0.
 */
void *
vitte_arena_memdup(
    vitte_arena_t *arena,
    const void *data,
    size_t size);

/*
 * Duplicate a NUL-terminated string.
 */
char *
vitte_arena_strdup(
    vitte_arena_t *arena,
    const char *text);

/*
 * Duplicate exactly length bytes and append a terminating NUL.
 *
 * This function does not search for an earlier NUL byte.
 */
char *
vitte_arena_strndup(
    vitte_arena_t *arena,
    const char *text,
    size_t length);

/* ========================================================================= */
/* Marks                                                                     */
/* ========================================================================= */

/*
 * Capture the current arena allocation state.
 *
 * An all-zero mark is returned for an invalid arena.
 */
vitte_arena_mark_t
vitte_arena_mark(
    const vitte_arena_t *arena);

/*
 * Rewind the arena to a previously captured mark.
 *
 * Allocations made after the mark become invalid.
 *
 * Blocks are retained for reuse rather than immediately freed.
 *
 * Returns false when the mark:
 *
 *   - belongs to another arena
 *   - belongs to another generation
 *   - references an invalid block
 *   - is otherwise structurally invalid
 */
bool
vitte_arena_rewind(
    vitte_arena_t *arena,
    vitte_arena_mark_t mark);

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

/*
 * Invalidate all current allocations while retaining blocks for reuse.
 *
 * The generation is incremented, invalidating every existing mark.
 *
 * Lifetime peak statistics are retained.
 *
 * Current-generation requested bytes and allocation count are reset.
 */
void
vitte_arena_reset(
    vitte_arena_t *arena);

/* ========================================================================= */
/* Trimming                                                                  */
/* ========================================================================= */

/*
 * Release all currently unused blocks.
 *
 * A block is considered unused when block->used == 0.
 *
 * Live allocations remain untouched.
 */
void
vitte_arena_trim(
    vitte_arena_t *arena);

/*
 * Reset all allocations and release unused blocks.
 *
 * Since reset marks all blocks unused, this currently releases all blocks.
 * The next allocation lazily creates a new block.
 */
void
vitte_arena_reset_and_trim(
    vitte_arena_t *arena);

/* ========================================================================= */
/* Error state                                                               */
/* ========================================================================= */

vitte_arena_error_t
vitte_arena_last_error(
    const vitte_arena_t *arena);

void
vitte_arena_clear_error(
    vitte_arena_t *arena);

/*
 * Stable machine-readable error name.
 */
const char *
vitte_arena_error_name(
    vitte_arena_error_t error);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

/*
 * Return a statistics snapshot.
 *
 * An all-zero structure is returned for an invalid arena.
 */
vitte_arena_stats_t
vitte_arena_stats(
    const vitte_arena_t *arena);

size_t
vitte_arena_block_count(
    const vitte_arena_t *arena);

size_t
vitte_arena_reserved_bytes(
    const vitte_arena_t *arena);

size_t
vitte_arena_used_bytes(
    const vitte_arena_t *arena);

size_t
vitte_arena_requested_bytes(
    const vitte_arena_t *arena);

size_t
vitte_arena_allocation_count(
    const vitte_arena_t *arena);

uint64_t
vitte_arena_generation(
    const vitte_arena_t *arena);

/* ========================================================================= */
/* Ownership queries                                                         */
/* ========================================================================= */

/*
 * Return true when pointer lies anywhere inside the payload capacity of one
 * of the arena's blocks.
 *
 * This does NOT prove that the pointer references a currently live
 * allocation.
 */
bool
vitte_arena_contains(
    const vitte_arena_t *arena,
    const void *pointer);

/*
 * Return true when pointer lies inside the currently used region of one of
 * the arena's blocks.
 *
 * This is stronger than vitte_arena_contains(), but it still does not prove
 * that pointer is exactly the start of an allocation.
 */
bool
vitte_arena_contains_live(
    const vitte_arena_t *arena,
    const void *pointer);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Perform an expensive structural integrity check.
 *
 * Intended for:
 *
 *   - debug builds
 *   - unit tests
 *   - fuzzing
 *   - ASan/UBSan builds
 *   - compiler self-checking
 *
 * Validation checks include:
 *
 *   - arena magic
 *   - block-list consistency
 *   - previous/next relationships
 *   - block magic
 *   - used <= capacity
 *   - generation consistency
 *   - current block membership
 *   - block accounting
 *   - reserved-byte accounting
 *   - used-byte accounting
 *   - peak counters
 */
bool
vitte_arena_validate(
    const vitte_arena_t *arena);

/* ========================================================================= */
/* Convenience queries                                                       */
/* ========================================================================= */

static inline bool
vitte_arena_is_empty(
    const vitte_arena_t *arena)
{
    return vitte_arena_used_bytes(arena) == 0u;
}

static inline bool
vitte_arena_has_allocations(
    const vitte_arena_t *arena)
{
    return vitte_arena_allocation_count(arena) != 0u;
}

static inline size_t
vitte_arena_unused_bytes(
    const vitte_arena_t *arena)
{
    size_t reserved;
    size_t used;

    reserved =
        vitte_arena_reserved_bytes(arena);

    used =
        vitte_arena_used_bytes(arena);

    return reserved >= used
        ? reserved - used
        : 0u;
}

/* ========================================================================= */
/* Typed allocation helpers                                                  */
/* ========================================================================= */

/*
 * Allocate storage for one object.
 *
 * Example:
 *
 *     node = VITTE_ARENA_NEW(&arena, vitte_ast_node_t);
 */
#define VITTE_ARENA_NEW(arena, type) \
    ((type *)vitte_arena_alloc_aligned( \
        (arena), \
        sizeof(type), \
        _Alignof(type)))

/*
 * Allocate zero-initialized storage for one object.
 */
#define VITTE_ARENA_NEW_ZERO(arena, type) \
    ((type *)vitte_arena_calloc_aligned( \
        (arena), \
        (size_t)1u, \
        sizeof(type), \
        _Alignof(type)))

/*
 * Allocate an array of typed objects.
 */
#define VITTE_ARENA_NEW_ARRAY(arena, type, count) \
    ((type *)vitte_arena_alloc_array( \
        (arena), \
        (count), \
        sizeof(type), \
        _Alignof(type)))

/*
 * Allocate a zero-initialized array of typed objects.
 */
#define VITTE_ARENA_NEW_ARRAY_ZERO(arena, type, count) \
    ((type *)vitte_arena_calloc_aligned( \
        (arena), \
        (count), \
        sizeof(type), \
        _Alignof(type)))

/* ========================================================================= */
/* Scope-style marks                                                         */
/* ========================================================================= */

/*
 * Explicit mark helpers intended to make parser/pass code easier to read.
 *
 * Example:
 *
 *     vitte_arena_mark_t mark = VITTE_ARENA_MARK(&arena);
 *
 *     ...
 *
 *     if (!VITTE_ARENA_REWIND(&arena, mark)) {
 *         ...
 *     }
 */
#define VITTE_ARENA_MARK(arena) \
    vitte_arena_mark((arena))

#define VITTE_ARENA_REWIND(arena, mark) \
    vitte_arena_rewind((arena), (mark))

/* ========================================================================= */
/* Error predicates                                                          */
/* ========================================================================= */

static inline bool
vitte_arena_has_error(
    const vitte_arena_t *arena)
{
    return vitte_arena_last_error(arena) !=
           VITTE_ARENA_ERROR_NONE;
}

static inline bool
vitte_arena_out_of_memory(
    const vitte_arena_t *arena)
{
    return vitte_arena_last_error(arena) ==
           VITTE_ARENA_ERROR_OUT_OF_MEMORY;
}

/* ========================================================================= */
/* Statistics helpers                                                        */
/* ========================================================================= */

static inline size_t
vitte_arena_padding_bytes(
    const vitte_arena_t *arena)
{
    size_t used;
    size_t requested;

    used =
        vitte_arena_used_bytes(arena);

    requested =
        vitte_arena_requested_bytes(arena);

    return used >= requested
        ? used - requested
        : 0u;
}

/*
 * Payload utilization in the integer range [0, 10000].
 *
 * 10000 = 100.00%
 *  5000 =  50.00%
 *
 * Integer arithmetic keeps this helper usable without floating-point
 * dependencies.
 */
static inline uint32_t
vitte_arena_utilization_basis_points(
    const vitte_arena_t *arena)
{
    size_t reserved;
    size_t used;

    reserved =
        vitte_arena_reserved_bytes(arena);

    used =
        vitte_arena_used_bytes(arena);

    if (reserved == 0u) {
        return 0u;
    }

    if (used >= reserved) {
        return UINT32_C(10000);
    }

    /*
     * Avoid overflow in used * 10000 by decomposing the calculation.
     */
    return (uint32_t)(
        (used / reserved) * (size_t)10000u +
        ((used % reserved) * (size_t)10000u) /
            reserved);
}

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_ARENA_DEFAULT_BLOCK_SIZE >=
        VITTE_ARENA_MIN_BLOCK_SIZE,
    "default arena block size must not be below minimum");

_Static_assert(
    VITTE_ARENA_LARGE_ALLOCATION_DIVISOR != 0u,
    "large-allocation divisor must not be zero");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte arena requires a 64-bit uint64_t");

_Static_assert(
    sizeof(uintptr_t) >= sizeof(void *),
    "uintptr_t must be capable of representing pointers");

_Static_assert(
    _Alignof(max_align_t) != 0u,
    "max_align_t must provide a valid alignment");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_ALLOCATOR_H */
