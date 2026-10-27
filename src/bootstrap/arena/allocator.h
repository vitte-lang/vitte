#ifndef VITTE_SRC_ARENA_ALLOCATOR_H
#define VITTE_SRC_ARENA_ALLOCATOR_H

/*
 * Vitte Compiler
 * src/arena/allocator.h
 *
 * Canonical high-performance arena allocator interface.
 *
 * Properties:
 *
 *   - ISO C17
 *   - amortized O(1) allocation
 *   - deterministic arena ownership
 *   - configurable normal block size
 *   - arbitrary valid power-of-two alignment
 *   - overflow-safe size arithmetic
 *   - dedicated oversized allocations
 *   - retained-block reuse
 *   - zero-initialized allocation
 *   - memory/string duplication
 *   - transactional marks and rewind
 *   - generation-based stale-mark rejection
 *   - reset and trimming
 *   - current and lifetime-peak statistics
 *   - ownership queries
 *   - structural validation
 *   - optional debug poisoning
 *   - deterministic destruction
 *
 * Individual allocations cannot be freed independently.
 *
 * Allocation lifetime ends when invalidated by:
 *
 *   - vitte_arena_rewind()
 *   - vitte_arena_reset()
 *   - vitte_arena_reset_and_trim()
 *   - vitte_arena_destroy()
 *
 * Thread safety:
 *
 *   A single arena is not internally synchronized. Independent arenas may be
 *   used concurrently by independent threads.
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

/*
 * Optional debug poisoning.
 *
 * These switches are consumed by allocator.c and may be overridden by the
 * build system.
 */
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
 * Operations returning pointers use NULL for failure and, whenever the arena
 * itself is valid, record a detailed error here.
 *
 * Mutating void operations likewise update last_error when they can detect a
 * failure.
 */
typedef enum vitte_arena_error {
    /*
     * No error.
     */
    VITTE_ARENA_ERROR_NONE = 0,

    /*
     * Arena pointer or runtime arena state is invalid.
     */
    VITTE_ARENA_ERROR_INVALID_ARENA,

    /*
     * Invalid public API argument.
     */
    VITTE_ARENA_ERROR_INVALID_ARGUMENT,

    /*
     * Explicit alignment is not supported by the arena contract.
     *
     * alignment == 0 is valid and means _Alignof(max_align_t).
     */
    VITTE_ARENA_ERROR_INVALID_ALIGNMENT,

    /*
     * Integer, size, or address arithmetic overflow.
     */
    VITTE_ARENA_ERROR_OVERFLOW,

    /*
     * malloc() could not provide a required block.
     */
    VITTE_ARENA_ERROR_OUT_OF_MEMORY,

    /*
     * A mark does not belong to this arena, is stale, references an invalid
     * block, or contains an inconsistent snapshot.
     */
    VITTE_ARENA_ERROR_INVALID_MARK,

    /*
     * An internal arena/block/list/accounting invariant is broken.
     */
    VITTE_ARENA_ERROR_CORRUPTION
} vitte_arena_error_t;

/* ========================================================================= */
/* Arena state                                                               */
/* ========================================================================= */

/*
 * Arena allocator state.
 *
 * This structure is intentionally visible because compiler subsystems embed
 * arenas directly inside larger structures.
 *
 * It is an internal Vitte compiler ABI and must not be considered a stable
 * external binary interface.
 *
 * Callers must not modify fields directly.
 */
struct vitte_arena {
    /*
     * Runtime validation cookie.
     *
     * Managed exclusively by allocator.c.
     */
    uint64_t magic;

    /*
     * Preferred payload capacity for ordinary blocks.
     */
    size_t block_size;

    /*
     * Doubly-linked owned block chain.
     */
    vitte_arena_block_t *first_block;
    vitte_arena_block_t *last_block;

    /*
     * Current allocation/reuse cursor.
     *
     * May be NULL only when the arena owns no blocks.
     */
    vitte_arena_block_t *current_block;

    /*
     * Current number of owned blocks.
     */
    size_t block_count;

    /*
     * Maximum block_count observed during this arena lifetime.
     *
     * Reset/rewind/trim do not decrease this value.
     */
    size_t peak_block_count;

    /*
     * Sum of block payload capacities.
     *
     * malloc bookkeeping and vitte_arena_block_t headers are excluded.
     */
    size_t reserved_bytes;

    /*
     * Maximum reserved_bytes observed during this arena lifetime.
     */
    size_t peak_reserved_bytes;

    /*
     * Sum of block->used.
     *
     * Includes alignment padding and the internal byte consumed by successful
     * logical zero-size allocations.
     */
    size_t used_bytes;

    /*
     * Maximum used_bytes observed during this arena lifetime.
     */
    size_t peak_used_bytes;

    /*
     * Sum of logical caller-requested allocation sizes in the current
     * generation.
     *
     * Alignment padding is excluded.
     *
     * Logical zero-size allocations add zero.
     *
     * This counter saturates at SIZE_MAX instead of wrapping.
     */
    size_t requested_bytes;

    /*
     * Number of successful public allocations in the current generation.
     *
     * This counter saturates at SIZE_MAX instead of wrapping.
     */
    size_t allocation_count;

    /*
     * Arena generation.
     *
     * Initialized to a non-zero value and changed by reset().
     *
     * Marks carry this value so marks from older generations are rejected.
     */
    uint64_t generation;

    /*
     * Most recent arena-local error.
     */
    vitte_arena_error_t last_error;
};

/* ========================================================================= */
/* Marks                                                                     */
/* ========================================================================= */

/*
 * Arena checkpoint.
 *
 * A mark captures enough state to discard allocations performed after the
 * checkpoint without freeing retained blocks.
 *
 * Marks are bound to:
 *
 *   - exactly one arena
 *   - exactly one generation
 *   - one allocation cursor
 *   - one accounting snapshot
 *
 * Marks are value objects and may be copied.
 *
 * reset() invalidates every mark from the previous generation.
 * destroy() invalidates every mark associated with the arena.
 *
 * rewind() itself does not change generation.
 */
struct vitte_arena_mark {
    /*
     * Arena that created the mark.
     */
    const vitte_arena_t *arena;

    /*
     * Allocation block active at mark creation.
     *
     * NULL represents a mark created while the arena had no current block.
     */
    vitte_arena_block_t *block;

    /*
     * block->used at mark creation.
     *
     * Must be zero when block == NULL.
     */
    size_t used;

    /*
     * Aggregate accounting snapshot.
     */
    size_t used_bytes;
    size_t requested_bytes;
    size_t allocation_count;

    /*
     * Arena generation at mark creation.
     */
    uint64_t generation;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

/*
 * Immutable-by-value arena statistics snapshot.
 *
 * Statistics are split into:
 *
 *   current-generation/current-state:
 *       block_count
 *       reserved_bytes
 *       used_bytes
 *       requested_bytes
 *       unused_bytes
 *       padding_bytes
 *       allocation_count
 *
 *   lifetime peaks:
 *       peak_block_count
 *       peak_reserved_bytes
 *       peak_used_bytes
 */
struct vitte_arena_stats {
    /*
     * Preferred ordinary block payload capacity.
     */
    size_t block_size;

    /*
     * Current number of blocks.
     */
    size_t block_count;

    /*
     * Lifetime maximum block count.
     */
    size_t peak_block_count;

    /*
     * Current block payload capacity.
     */
    size_t reserved_bytes;

    /*
     * Lifetime maximum reserved payload capacity.
     */
    size_t peak_reserved_bytes;

    /*
     * Current payload bytes consumed.
     *
     * Includes alignment padding.
     */
    size_t used_bytes;

    /*
     * Lifetime maximum payload bytes consumed.
     */
    size_t peak_used_bytes;

    /*
     * Logical caller-requested bytes in the current generation.
     *
     * May be SIZE_MAX when saturated.
     */
    size_t requested_bytes;

    /*
     * Current unconsumed block payload:
     *
     *     reserved_bytes - used_bytes
     */
    size_t unused_bytes;

    /*
     * Internal consumption beyond logical requested bytes when that
     * relationship can be represented:
     *
     *     used_bytes - requested_bytes
     *
     * This includes alignment padding and internal storage consumed by
     * logical zero-byte allocations.
     *
     * When requested_bytes has saturated beyond the representable accounting
     * relationship, this value is reported conservatively as zero.
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
 * block_size == 0:
 *
 *     selects VITTE_ARENA_DEFAULT_BLOCK_SIZE.
 *
 * block_size < VITTE_ARENA_MIN_BLOCK_SIZE:
 *
 *     is raised to VITTE_ARENA_MIN_BLOCK_SIZE.
 *
 * Initialization is lazy and does not allocate an initial block.
 *
 * The supplied object must not currently own a live arena. Destroy a live
 * arena before reinitializing the same object.
 *
 * Returns true on successful initialization.
 *
 * Returns false only when arena == NULL.
 */
bool
vitte_arena_init(
    vitte_arena_t *arena,
    size_t block_size);

/*
 * Equivalent to:
 *
 *     vitte_arena_init(
 *         arena,
 *         VITTE_ARENA_DEFAULT_BLOCK_SIZE);
 */
bool
vitte_arena_init_default(
    vitte_arena_t *arena);

/*
 * Destroy all blocks owned by the arena.
 *
 * Every pointer returned by the arena becomes invalid.
 *
 * Every mark associated with the arena becomes invalid.
 *
 * Passing NULL or a non-live arena is harmless.
 *
 * Calling destroy() repeatedly is harmless.
 */
void
vitte_arena_destroy(
    vitte_arena_t *arena);

/* ========================================================================= */
/* Core allocation                                                           */
/* ========================================================================= */

/*
 * Allocate size logical bytes using _Alignof(max_align_t).
 *
 * Zero-size contract:
 *
 *     size == 0
 *
 * is represented internally by consuming one payload byte. This allows a
 * successful zero-size allocation to return an arena-owned address.
 *
 * requested_bytes nevertheless records the caller's logical request of zero.
 *
 * Returns NULL on failure.
 */
void *
vitte_arena_alloc(
    vitte_arena_t *arena,
    size_t size);

/*
 * Allocate size logical bytes with explicit alignment.
 *
 * alignment == 0 selects:
 *
 *     _Alignof(max_align_t)
 *
 * Otherwise alignment must be a valid power of two representable by the
 * allocator's uintptr_t address arithmetic.
 *
 * Alignment greater than _Alignof(max_align_t) is supported by reserving
 * enough payload space and aligning within the block.
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
 * logical bytes with explicit alignment.
 *
 * Multiplication overflow is detected before allocation.
 *
 * count == 0 or element_size == 0 therefore forms a logical zero-size
 * allocation and follows the normal zero-size arena contract.
 */
void *
vitte_arena_alloc_array(
    vitte_arena_t *arena,
    size_t count,
    size_t element_size,
    size_t alignment);

/*
 * Allocate:
 *
 *     count * element_size
 *
 * logical bytes using _Alignof(max_align_t), then zero the logical byte range.
 *
 * Multiplication overflow is detected before allocation.
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
 * Duplicate exactly size bytes into arena-owned storage.
 *
 * data == NULL is accepted only when size == 0.
 *
 * A zero-size duplication follows the normal zero-size allocation contract.
 */
void *
vitte_arena_memdup(
    vitte_arena_t *arena,
    const void *data,
    size_t size);

/*
 * Duplicate a NUL-terminated string into arena-owned storage.
 *
 * text must not be NULL.
 *
 * The returned string includes its terminating NUL.
 */
char *
vitte_arena_strdup(
    vitte_arena_t *arena,
    const char *text);

/*
 * Duplicate exactly length bytes and append a terminating NUL.
 *
 * No search for an earlier NUL is performed.
 *
 * text == NULL is accepted only when length == 0, in which case an empty
 * string is produced.
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
 * Capture the current allocation/accounting state.
 *
 * An all-zero mark is returned when arena is invalid.
 *
 * Creating a mark does not mutate the arena.
 */
vitte_arena_mark_t
vitte_arena_mark(
    const vitte_arena_t *arena);

/*
 * Rewind to a mark previously produced by vitte_arena_mark().
 *
 * Every allocation performed after the mark becomes invalid.
 *
 * Blocks are retained and reset for later reuse rather than immediately
 * released.
 *
 * The arena generation does not change.
 *
 * The operation validates the mark and relevant block structure before
 * committing the rewind.
 *
 * Returns false if the mark:
 *
 *   - belongs to another arena
 *   - belongs to another generation
 *   - references a block no longer reachable from the arena
 *   - references an impossible used offset
 *   - contains accounting inconsistent with the current arena
 *   - encounters structural corruption
 *
 * On success:
 *
 *   used_bytes
 *   requested_bytes
 *   allocation_count
 *
 * are restored to the mark's snapshot.
 */
bool
vitte_arena_rewind(
    vitte_arena_t *arena,
    vitte_arena_mark_t mark);

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

/*
 * Invalidate all current allocations while retaining owned blocks.
 *
 * Every retained block becomes empty and available for deterministic reuse.
 *
 * The arena generation changes, invalidating all previously created marks.
 *
 * Current-generation counters reset:
 *
 *     used_bytes       = 0
 *     requested_bytes  = 0
 *     allocation_count = 0
 *
 * Lifetime peak counters are retained.
 *
 * reset() performs structural checks before mutating the block chain. A
 * detected corruption is reported through last_error.
 */
void
vitte_arena_reset(
    vitte_arena_t *arena);

/* ========================================================================= */
/* Trimming                                                                  */
/* ========================================================================= */

/*
 * Release every currently unused block:
 *
 *     block->used == 0
 *
 * Blocks containing live allocations are retained.
 *
 * Peak statistics are not reduced.
 *
 * This operation does not change generation.
 */
void
vitte_arena_trim(
    vitte_arena_t *arena);

/*
 * Reset the arena and then release all unused blocks.
 *
 * Since a successful reset makes every block unused, a successful
 * reset_and_trim() releases all blocks.
 *
 * The next allocation creates a new block lazily.
 *
 * If reset() detects an error, trimming is not attempted.
 */
void
vitte_arena_reset_and_trim(
    vitte_arena_t *arena);

/* ========================================================================= */
/* Error state                                                               */
/* ========================================================================= */

/*
 * Return the most recent arena-local error.
 *
 * Invalid arenas report VITTE_ARENA_ERROR_INVALID_ARENA.
 */
vitte_arena_error_t
vitte_arena_last_error(
    const vitte_arena_t *arena);

/*
 * Clear last_error on a valid arena.
 *
 * Invalid arenas are ignored.
 */
void
vitte_arena_clear_error(
    vitte_arena_t *arena);

/*
 * Return a stable machine-readable error name.
 *
 * Known values:
 *
 *     none
 *     invalid-arena
 *     invalid-argument
 *     invalid-alignment
 *     overflow
 *     out-of-memory
 *     invalid-mark
 *     corruption
 *
 * Unknown enum values return "unknown".
 */
const char *
vitte_arena_error_name(
    vitte_arena_error_t error);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

/*
 * Return a by-value statistics snapshot.
 *
 * Invalid arenas return an all-zero structure.
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
 * Return true when pointer lies anywhere inside the payload capacity of an
 * arena-owned block.
 *
 * This means:
 *
 *     block->data <= pointer < block->data + block->capacity
 *
 * It does not prove that pointer references live storage.
 *
 * It does not prove that pointer is an allocation start.
 */
bool
vitte_arena_contains(
    const vitte_arena_t *arena,
    const void *pointer);

/*
 * Return true when pointer lies inside the currently consumed region of an
 * arena-owned block:
 *
 *     block->data <= pointer < block->data + block->used
 *
 * This is stronger than vitte_arena_contains(), but still does not prove that
 * pointer is exactly the beginning of a public allocation.
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
 *   - unit tests
 *   - debug builds
 *   - fuzzing
 *   - ASan/UBSan builds
 *   - compiler self-checking
 *
 * Validation includes:
 *
 *   - live arena magic
 *   - non-zero generation
 *   - configured minimum block size
 *   - empty/non-empty list consistency
 *   - forward-list cycle detection
 *   - first/last endpoint invariants
 *   - previous/next relationship consistency
 *   - block magic
 *   - block used <= capacity
 *   - block generation consistency
 *   - current-block membership
 *   - exact block count
 *   - exact reserved-byte accounting
 *   - exact used-byte accounting
 *   - used <= reserved
 *   - requested/used relationship when requested accounting is unsaturated
 *   - peak block count
 *   - peak reserved bytes
 *   - peak used bytes
 *
 * Returns true only when all checked invariants hold.
 */
bool
vitte_arena_validate(
    const vitte_arena_t *arena);

/* ========================================================================= */
/* Convenience queries                                                       */
/* ========================================================================= */

/*
 * Return true when the arena currently consumes no payload bytes.
 *
 * Invalid arenas also report empty because their public used-byte query is
 * zero.
 */
static inline bool
vitte_arena_is_empty(
    const vitte_arena_t *arena)
{
    return vitte_arena_used_bytes(
               arena) == 0u;
}

/*
 * Return true when at least one successful allocation is recorded in the
 * current generation.
 */
static inline bool
vitte_arena_has_allocations(
    const vitte_arena_t *arena)
{
    return vitte_arena_allocation_count(
               arena) != 0u;
}

/*
 * Return currently unconsumed payload capacity.
 */
static inline size_t
vitte_arena_unused_bytes(
    const vitte_arena_t *arena)
{
    size_t reserved;
    size_t used;

    reserved =
        vitte_arena_reserved_bytes(
            arena);

    used =
        vitte_arena_used_bytes(
            arena);

    if (reserved < used) {
        return 0u;
    }

    return reserved - used;
}

/*
 * Return current internal payload consumption beyond logical requested bytes.
 *
 * When requested_bytes has saturated such that requested > used, zero is
 * returned conservatively.
 */
static inline size_t
vitte_arena_padding_bytes(
    const vitte_arena_t *arena)
{
    size_t used;
    size_t requested;

    used =
        vitte_arena_used_bytes(
            arena);

    requested =
        vitte_arena_requested_bytes(
            arena);

    if (used < requested) {
        return 0u;
    }

    return used - requested;
}

/* ========================================================================= */
/* Typed allocation helpers                                                  */
/* ========================================================================= */

/*
 * Allocate storage for one object with the object's natural alignment.
 *
 * Example:
 *
 *     vitte_ast_node_t *node =
 *         VITTE_ARENA_NEW(
 *             &arena,
 *             vitte_ast_node_t);
 *
 * type must denote a complete object type accepted by sizeof and _Alignof.
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
 * Allocate count objects with the object's natural alignment.
 *
 * Multiplication overflow is checked by vitte_arena_alloc_array().
 */
#define VITTE_ARENA_NEW_ARRAY(arena, type, count) \
    ((type *)vitte_arena_alloc_array( \
        (arena), \
        (count), \
        sizeof(type), \
        _Alignof(type)))

/*
 * Allocate count zero-initialized objects with the object's natural
 * alignment.
 *
 * Multiplication overflow is checked by vitte_arena_calloc_aligned().
 */
#define VITTE_ARENA_NEW_ARRAY_ZERO(arena, type, count) \
    ((type *)vitte_arena_calloc_aligned( \
        (arena), \
        (count), \
        sizeof(type), \
        _Alignof(type)))

/* ========================================================================= */
/* Mark convenience                                                          */
/* ========================================================================= */

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
    return vitte_arena_last_error(
               arena) !=
           VITTE_ARENA_ERROR_NONE;
}

static inline bool
vitte_arena_out_of_memory(
    const vitte_arena_t *arena)
{
    return vitte_arena_last_error(
               arena) ==
           VITTE_ARENA_ERROR_OUT_OF_MEMORY;
}

static inline bool
vitte_arena_is_corrupted(
    const vitte_arena_t *arena)
{
    return vitte_arena_last_error(
               arena) ==
           VITTE_ARENA_ERROR_CORRUPTION;
}

/* ========================================================================= */
/* Statistics helpers                                                        */
/* ========================================================================= */

/*
 * Return payload utilization in basis points:
 *
 *       0 =   0.00%
 *    5000 =  50.00%
 *   10000 = 100.00%
 *
 * Invalid or blockless arenas return zero.
 *
 * The implementation avoids the overflowing expression:
 *
 *     used * 10000
 */
static inline uint32_t
vitte_arena_utilization_basis_points(
    const vitte_arena_t *arena)
{
    size_t reserved;
    size_t used;
    size_t quotient;
    size_t remainder;
    size_t scaled_remainder;

    reserved =
        vitte_arena_reserved_bytes(
            arena);

    used =
        vitte_arena_used_bytes(
            arena);

    if (reserved == 0u) {
        return UINT32_C(0);
    }

    if (used >= reserved) {
        return UINT32_C(10000);
    }

    quotient =
        used / reserved;

    remainder =
        used % reserved;

    /*
     * Because used < reserved above, quotient is normally zero. Keep the
     * decomposition general and calculate the fractional component without
     * requiring remainder * 10000 to fit in size_t.
     *
     * Compute:
     *
     *     floor(remainder * 10000 / reserved)
     *
     * using 10000 repeated quotient/remainder steps would be exact but
     * needlessly expensive. Instead split 10000 into 100 * 100 while keeping
     * every multiplication bounded by reserved where possible.
     *
     * For the arena's normal size_t domain, use division decomposition:
     *
     *     remainder = q * (reserved / 10000) + ...
     *
     * A simpler exact implementation is possible because the final value is
     * bounded by 9999: repeatedly consume decimal factors 10.
     */
    {
        size_t value;
        size_t fraction;
        unsigned int i;

        value =
            remainder;

        fraction = 0u;

        for (i = 0u; i < 4u; ++i) {
            size_t digit;
            size_t whole;
            size_t rem;

            whole =
                value / reserved;

            rem =
                value % reserved;

            /*
             * rem < reserved. Compute rem * 10 without overflow by deriving
             * the next decimal digit through threshold subtraction when
             * required.
             */
            if (rem <=
                SIZE_MAX / (size_t)10u) {
                value =
                    rem * (size_t)10u;

                digit =
                    value / reserved;

                value %=
                    reserved;
            } else {
                size_t threshold;
                size_t accumulated;

                digit = 0u;
                accumulated = rem;

                /*
                 * At most nine iterations because the next radix is 10.
                 */
                while (digit < 9u) {
                    threshold =
                        reserved - accumulated;

                    if (rem < threshold) {
                        break;
                    }

                    accumulated +=
                        rem - reserved;

                    ++digit;
                }

                value =
                    accumulated;
            }

            (void)whole;

            fraction =
                fraction * (size_t)10u +
                digit;
        }

        scaled_remainder =
            fraction;
    }

    /*
     * used < reserved guarantees quotient == 0, but retain the term to keep
     * the formula explicit.
     */
    return (uint32_t)(
        quotient * (size_t)10000u +
        scaled_remainder);
}

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_ARENA_DEFAULT_BLOCK_SIZE >=
        VITTE_ARENA_MIN_BLOCK_SIZE,
    "default arena block size must not be below minimum");

_Static_assert(
    VITTE_ARENA_MIN_BLOCK_SIZE != 0u,
    "minimum arena block size must not be zero");

_Static_assert(
    VITTE_ARENA_LARGE_ALLOCATION_DIVISOR != 0u,
    "large-allocation divisor must not be zero");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte arena requires a 64-bit uint64_t");

_Static_assert(
    sizeof(uintptr_t) >= sizeof(void *),
    "uintptr_t must be capable of representing object pointers");

_Static_assert(
    _Alignof(max_align_t) != 0u,
    "max_align_t must provide a valid alignment");

_Static_assert(
    (VITTE_ARENA_ALLOC_PATTERN & ~0xFF) == 0,
    "arena allocation poison pattern must fit in one byte");

_Static_assert(
    (VITTE_ARENA_RESET_PATTERN & ~0xFF) == 0,
    "arena reset poison pattern must fit in one byte");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_ALLOCATOR_H */