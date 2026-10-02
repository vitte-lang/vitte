#ifndef VITTE_SRC_ARENA_STATS_H
#define VITTE_SRC_ARENA_STATS_H

/*
 * Vitte Compiler
 * src/arena/stats.h
 *
 * Memory statistics and observation layer.
 *
 * This module observes:
 *
 *     arena contexts
 *     fixed-size pools
 *     groups of pools
 *     complete arena + pools snapshots
 *     differences between snapshots
 *
 * Properties:
 *
 *     - read-only observation
 *     - no allocator mutation
 *     - no dynamic allocation
 *     - overflow-aware aggregation
 *     - deterministic snapshots
 *     - basis-point ratios
 *     - high-water statistics
 *     - overhead/padding accounting
 *     - before/after deltas
 *     - structural validation
 *
 * Ratio convention:
 *
 *         0 =   0.00%
 *      5000 =  50.00%
 *     10000 = 100.00%
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "arena.h"
#include "pool.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#ifndef VITTE_STATS_BASIS_POINTS
#define VITTE_STATS_BASIS_POINTS UINT32_C(10000)
#endif

/* ========================================================================= */
/* Arena statistics                                                          */
/* ========================================================================= */

typedef struct vitte_stats_arena {
    /*
     * Snapshot successfully captured.
     */
    bool valid;

    /*
     * Configured/default block size.
     */
    size_t block_size;

    /*
     * Physical block counts.
     */
    size_t block_count;
    size_t peak_block_count;

    /*
     * Physical bytes reserved by arena blocks.
     */
    size_t reserved_bytes;
    size_t peak_reserved_bytes;

    /*
     * Physical bump space consumed.
     *
     * Includes alignment padding.
     */
    size_t used_bytes;
    size_t peak_used_bytes;

    /*
     * Logical caller-requested bytes.
     */
    size_t requested_bytes;

    /*
     * Physical reserved space not currently consumed.
     *
     * Expected invariant:
     *
     *     unused_bytes =
     *         reserved_bytes - used_bytes
     */
    size_t unused_bytes;

    /*
     * Alignment/allocator padding currently represented by the arena
     * accounting.
     */
    size_t padding_bytes;

    /*
     * Current logical allocation count.
     */
    size_t allocation_count;

    /*
     * Arena generation.
     */
    uint64_t generation;

    /*
     * used_bytes / reserved_bytes.
     */
    uint32_t utilization_basis_points;

    /*
     * requested_bytes / reserved_bytes.
     */
    uint32_t requested_utilization_basis_points;

    /*
     * padding_bytes / used_bytes.
     */
    uint32_t padding_basis_points;
} vitte_stats_arena_t;

/* ========================================================================= */
/* Pool statistics                                                           */
/* ========================================================================= */

typedef struct vitte_stats_pool {
    bool valid;

    /*
     * Logical and physical slot dimensions.
     */
    size_t object_size;
    size_t slot_size;
    size_t alignment;

    /*
     * Physical pages.
     */
    size_t page_count;
    size_t peak_page_count;

    /*
     * Slot counts.
     */
    size_t capacity;

    size_t live_count;
    size_t peak_live_count;

    size_t free_count;

    /*
     * Physical page allocation.
     *
     * Includes page headers, bitmaps, alignment padding and slots.
     */
    size_t reserved_bytes;
    size_t peak_reserved_bytes;

    /*
     * Logical bytes represented by live objects:
     *
     *     live_count * object_size
     */
    size_t live_object_bytes;

    /*
     * Physical slot region:
     *
     *     capacity * slot_size
     */
    size_t slot_storage_bytes;

    /*
     * Internal slot padding:
     *
     *     capacity * (slot_size - object_size)
     */
    size_t slot_padding_bytes;

    /*
     * Page metadata and page-layout overhead:
     *
     *     reserved_bytes - slot_storage_bytes
     */
    size_t metadata_bytes;

    /*
     * Lifetime operation counters.
     */
    size_t allocation_count;
    size_t free_operation_count;

    uint64_t generation;

    /*
     * Effective configured limits.
     */
    size_t max_objects;
    size_t max_reserved_bytes;

    /*
     * live_count / capacity.
     */
    uint32_t utilization_basis_points;

    /*
     * live_object_bytes / reserved_bytes.
     */
    uint32_t live_bytes_basis_points;

    /*
     * metadata_bytes / reserved_bytes.
     */
    uint32_t metadata_basis_points;

    /*
     * At least one derived size_t statistic saturated at SIZE_MAX.
     */
    bool saturated;
} vitte_stats_pool_t;

/* ========================================================================= */
/* Aggregated pool statistics                                                */
/* ========================================================================= */

typedef struct vitte_stats_pools {
    /*
     * True only when every requested pool was valid.
     */
    bool valid;

    /*
     * Number of entries supplied to capture_pools().
     */
    size_t pool_count;

    /*
     * Valid/invalid input pool counts.
     */
    size_t valid_pool_count;
    size_t invalid_pool_count;

    /*
     * Aggregated physical page counts.
     */
    size_t page_count;
    size_t peak_page_count;

    /*
     * Aggregated slots.
     */
    size_t capacity;

    size_t live_count;
    size_t peak_live_count;

    size_t free_count;

    /*
     * Aggregated memory.
     */
    size_t reserved_bytes;
    size_t peak_reserved_bytes;

    size_t live_object_bytes;
    size_t slot_storage_bytes;
    size_t slot_padding_bytes;
    size_t metadata_bytes;

    /*
     * Aggregated lifetime operations.
     */
    size_t allocation_count;
    size_t free_operation_count;

    /*
     * live_count / capacity.
     */
    uint32_t utilization_basis_points;

    /*
     * live_object_bytes / reserved_bytes.
     */
    uint32_t live_bytes_basis_points;

    /*
     * metadata_bytes / reserved_bytes.
     */
    uint32_t metadata_basis_points;

    /*
     * One or more aggregated counters saturated at SIZE_MAX.
     */
    bool saturated;
} vitte_stats_pools_t;

/* ========================================================================= */
/* Complete snapshot                                                         */
/* ========================================================================= */

typedef struct vitte_stats_snapshot {
    /*
     * Complete snapshot validity.
     */
    bool valid;

    /*
     * Components included in this snapshot.
     */
    bool has_arena;
    bool has_pools;

    vitte_stats_arena_t arena;
    vitte_stats_pools_t pools;

    /*
     * Total physical memory:
     *
     *     arena.reserved_bytes +
     *     pools.reserved_bytes
     */
    size_t total_reserved_bytes;

    /*
     * Memory currently considered live:
     *
     *     arena.used_bytes +
     *     pools.live_object_bytes
     *
     * Note:
     *
     * arena.used_bytes includes arena alignment/padding while pool live bytes
     * intentionally represent logical object payload.
     */
    size_t total_live_bytes;

    /*
     * Logical requested payload:
     *
     *     arena.requested_bytes +
     *     pools.live_object_bytes
     */
    size_t total_requested_bytes;

    /*
     * Reserved bytes not represented by total_live_bytes.
     */
    size_t total_unused_bytes;

    /*
     * Explicitly identified overhead:
     *
     *     arena.padding_bytes
     *     + pools.metadata_bytes
     *     + pools.slot_padding_bytes
     */
    size_t total_overhead_bytes;

    /*
     * Arena allocation count plus pool lifetime allocation operations.
     *
     * Note that these components do not have identical lifetime semantics;
     * this is an operational aggregate, not a count of currently live
     * objects.
     */
    size_t total_allocation_count;

    /*
     * total_live_bytes / total_reserved_bytes.
     */
    uint32_t utilization_basis_points;

    /*
     * total_overhead_bytes / total_reserved_bytes.
     */
    uint32_t overhead_basis_points;

    /*
     * One or more aggregate fields saturated at SIZE_MAX.
     */
    bool saturated;
} vitte_stats_snapshot_t;

/* ========================================================================= */
/* Snapshot delta                                                            */
/* ========================================================================= */

/*
 * Signed delta convention:
 *
 *     after - before
 *
 * Positive:
 *
 *     value increased
 *
 * Negative:
 *
 *     value decreased
 *
 * If a size_t difference cannot be represented by ptrdiff_t, the field
 * saturates at PTRDIFF_MAX/PTRDIFF_MIN and saturated is set.
 */
typedef struct vitte_stats_delta {
    bool valid;
    bool saturated;

    /* --------------------------------------------------------------------- */
    /* Global                                                                */
    /* --------------------------------------------------------------------- */

    ptrdiff_t total_reserved_bytes;
    ptrdiff_t total_live_bytes;
    ptrdiff_t total_requested_bytes;
    ptrdiff_t total_unused_bytes;
    ptrdiff_t total_overhead_bytes;
    ptrdiff_t total_allocation_count;

    /* --------------------------------------------------------------------- */
    /* Arena                                                                 */
    /* --------------------------------------------------------------------- */

    ptrdiff_t arena_block_count;

    ptrdiff_t arena_reserved_bytes;
    ptrdiff_t arena_used_bytes;
    ptrdiff_t arena_requested_bytes;
    ptrdiff_t arena_unused_bytes;
    ptrdiff_t arena_padding_bytes;

    ptrdiff_t arena_allocation_count;

    /* --------------------------------------------------------------------- */
    /* Pools                                                                 */
    /* --------------------------------------------------------------------- */

    ptrdiff_t pools_page_count;

    ptrdiff_t pools_capacity;

    ptrdiff_t pools_live_count;
    ptrdiff_t pools_free_count;

    ptrdiff_t pools_reserved_bytes;
    ptrdiff_t pools_live_object_bytes;
    ptrdiff_t pools_metadata_bytes;

    ptrdiff_t pools_allocation_count;
    ptrdiff_t pools_free_operation_count;

    /* --------------------------------------------------------------------- */
    /* Ratios                                                                */
    /* --------------------------------------------------------------------- */

    uint32_t utilization_before_basis_points;
    uint32_t utilization_after_basis_points;

    uint32_t overhead_before_basis_points;
    uint32_t overhead_after_basis_points;
} vitte_stats_delta_t;

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

void
vitte_stats_snapshot_init(
    vitte_stats_snapshot_t *snapshot);

void
vitte_stats_delta_init(
    vitte_stats_delta_t *delta);

/* ========================================================================= */
/* Arena capture                                                             */
/* ========================================================================= */

/*
 * Capture a read-only arena snapshot.
 *
 * No allocator state is modified.
 */
bool
vitte_stats_capture_arena(
    const vitte_arena_context_t *arena,
    vitte_stats_arena_t *stats);

/* ========================================================================= */
/* Pool capture                                                              */
/* ========================================================================= */

bool
vitte_stats_capture_pool(
    const vitte_pool_t *pool,
    vitte_stats_pool_t *stats);

/* ========================================================================= */
/* Pool aggregation                                                          */
/* ========================================================================= */

/*
 * Aggregate pool_count pool pointers.
 *
 * Invalid pools are counted in invalid_pool_count.
 *
 * Return:
 *
 *     true
 *         every supplied pool was valid
 *
 *     false
 *         invalid arguments or at least one invalid pool
 *
 * Even when false because individual pools were invalid, stats contains the
 * aggregate of the valid pools that could be observed.
 */
bool
vitte_stats_capture_pools(
    vitte_pool_t *const *pools,
    size_t pool_count,
    vitte_stats_pools_t *stats);

/* ========================================================================= */
/* Complete capture                                                          */
/* ========================================================================= */

/*
 * Capture arena + pool state.
 *
 * arena may be NULL when at least one pool is supplied.
 *
 * pools may be NULL only when pool_count == 0.
 *
 * At least one observed component must be present.
 */
bool
vitte_stats_capture(
    const vitte_arena_context_t *arena,
    vitte_pool_t *const *pools,
    size_t pool_count,
    vitte_stats_snapshot_t *snapshot);

/* ========================================================================= */
/* Delta                                                                     */
/* ========================================================================= */

/*
 * Calculate after - before.
 *
 * Both snapshots must:
 *
 *     - be valid
 *     - contain the same component categories
 *
 * i.e. has_arena/has_pools must match.
 */
bool
vitte_stats_delta(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after,
    vitte_stats_delta_t *delta);

/* ========================================================================= */
/* Difference helpers                                                        */
/* ========================================================================= */

/*
 * Return physical bytes released:
 *
 *     max(before.reserved - after.reserved, 0)
 */
size_t
vitte_stats_reserved_released(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after);

/*
 * Return physical bytes newly reserved:
 *
 *     max(after.reserved - before.reserved, 0)
 */
size_t
vitte_stats_reserved_increased(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after);

/*
 * Return live bytes removed:
 *
 *     max(before.live - after.live, 0)
 */
size_t
vitte_stats_live_released(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after);

/* ========================================================================= */
/* Comparison                                                                */
/* ========================================================================= */

/*
 * Compare total_reserved_bytes.
 *
 * Returns:
 *
 *     < 0  left < right
 *       0  left == right
 *     > 0  left > right
 */
int
vitte_stats_compare_reserved(
    const vitte_stats_snapshot_t *left,
    const vitte_stats_snapshot_t *right);

/*
 * Compare total_live_bytes.
 */
int
vitte_stats_compare_live(
    const vitte_stats_snapshot_t *left,
    const vitte_stats_snapshot_t *right);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_stats_validate_arena(
    const vitte_stats_arena_t *stats);

bool
vitte_stats_validate_pool(
    const vitte_stats_pool_t *stats);

bool
vitte_stats_validate_pools(
    const vitte_stats_pools_t *stats);

bool
vitte_stats_validate_snapshot(
    const vitte_stats_snapshot_t *snapshot);

bool
vitte_stats_validate_delta(
    const vitte_stats_delta_t *delta);

/* ========================================================================= */
/* Generic snapshot predicates                                               */
/* ========================================================================= */

static inline bool
vitte_stats_snapshot_is_valid(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        snapshot != NULL &&
        snapshot->valid;
}

static inline bool
vitte_stats_snapshot_has_arena(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        snapshot != NULL &&
        snapshot->valid &&
        snapshot->has_arena;
}

static inline bool
vitte_stats_snapshot_has_pools(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        snapshot != NULL &&
        snapshot->valid &&
        snapshot->has_pools;
}

static inline bool
vitte_stats_snapshot_is_saturated(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        snapshot != NULL &&
        snapshot->saturated;
}

static inline bool
vitte_stats_delta_is_valid(
    const vitte_stats_delta_t *delta)
{
    return
        delta != NULL &&
        delta->valid;
}

/* ========================================================================= */
/* Memory predicates                                                         */
/* ========================================================================= */

static inline bool
vitte_stats_has_reserved_memory(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        snapshot != NULL &&
        snapshot->total_reserved_bytes != 0u;
}

static inline bool
vitte_stats_has_live_memory(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        snapshot != NULL &&
        snapshot->total_live_bytes != 0u;
}

static inline bool
vitte_stats_has_overhead(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        snapshot != NULL &&
        snapshot->total_overhead_bytes != 0u;
}

/* ========================================================================= */
/* Arena helpers                                                             */
/* ========================================================================= */

static inline size_t
vitte_stats_arena_reclaimable_bytes(
    const vitte_stats_arena_t *stats)
{
    if (stats == NULL ||
        !stats->valid) {
        return 0u;
    }

    return stats->unused_bytes;
}

static inline bool
vitte_stats_arena_is_empty(
    const vitte_stats_arena_t *stats)
{
    return
        stats != NULL &&
        stats->valid &&
        stats->used_bytes == 0u;
}

static inline bool
vitte_stats_arena_has_padding(
    const vitte_stats_arena_t *stats)
{
    return
        stats != NULL &&
        stats->valid &&
        stats->padding_bytes != 0u;
}

/* ========================================================================= */
/* Pool helpers                                                              */
/* ========================================================================= */

static inline bool
vitte_stats_pool_is_empty(
    const vitte_stats_pool_t *stats)
{
    return
        stats != NULL &&
        stats->valid &&
        stats->live_count == 0u;
}

static inline bool
vitte_stats_pool_is_full(
    const vitte_stats_pool_t *stats)
{
    return
        stats != NULL &&
        stats->valid &&
        stats->capacity != 0u &&
        stats->live_count ==
            stats->capacity;
}

static inline size_t
vitte_stats_pool_available(
    const vitte_stats_pool_t *stats)
{
    if (stats == NULL ||
        !stats->valid) {
        return 0u;
    }

    return stats->free_count;
}

/* ========================================================================= */
/* Delta predicates                                                          */
/* ========================================================================= */

static inline bool
vitte_stats_memory_grew(
    const vitte_stats_delta_t *delta)
{
    return
        delta != NULL &&
        delta->valid &&
        delta->total_reserved_bytes > 0;
}

static inline bool
vitte_stats_memory_shrank(
    const vitte_stats_delta_t *delta)
{
    return
        delta != NULL &&
        delta->valid &&
        delta->total_reserved_bytes < 0;
}

static inline bool
vitte_stats_live_memory_grew(
    const vitte_stats_delta_t *delta)
{
    return
        delta != NULL &&
        delta->valid &&
        delta->total_live_bytes > 0;
}

static inline bool
vitte_stats_live_memory_shrank(
    const vitte_stats_delta_t *delta)
{
    return
        delta != NULL &&
        delta->valid &&
        delta->total_live_bytes < 0;
}

static inline bool
vitte_stats_allocation_count_grew(
    const vitte_stats_delta_t *delta)
{
    return
        delta != NULL &&
        delta->valid &&
        delta->total_allocation_count > 0;
}

/* ========================================================================= */
/* Ratio helpers                                                             */
/* ========================================================================= */

static inline uint32_t
vitte_stats_utilization_basis_points(
    const vitte_stats_snapshot_t *snapshot)
{
    if (snapshot == NULL ||
        !snapshot->valid) {
        return UINT32_C(0);
    }

    return
        snapshot->utilization_basis_points;
}

static inline uint32_t
vitte_stats_overhead_basis_points(
    const vitte_stats_snapshot_t *snapshot)
{
    if (snapshot == NULL ||
        !snapshot->valid) {
        return UINT32_C(0);
    }

    return
        snapshot->overhead_basis_points;
}

/*
 * Integer whole-percent convenience.
 *
 * Example:
 *
 *     7534 bp -> 75
 */
static inline uint32_t
vitte_stats_basis_points_to_percent(
    uint32_t basis_points)
{
    if (basis_points >
        VITTE_STATS_BASIS_POINTS) {
        basis_points =
            VITTE_STATS_BASIS_POINTS;
    }

    return
        basis_points /
        UINT32_C(100);
}

/*
 * Hundredths of a percent.
 *
 * Basis points already directly encode this representation:
 *
 *     7534 -> 75.34%
 */
static inline uint32_t
vitte_stats_percent_hundredths(
    uint32_t basis_points)
{
    if (basis_points >
        VITTE_STATS_BASIS_POINTS) {
        return
            VITTE_STATS_BASIS_POINTS;
    }

    return basis_points;
}

/* ========================================================================= */
/* Snapshot equality                                                         */
/* ========================================================================= */

/*
 * Structural metric equality.
 *
 * This intentionally ignores no fields: it is useful for deterministic
 * allocator tests.
 */
static inline bool
vitte_stats_snapshot_equal(
    const vitte_stats_snapshot_t *left,
    const vitte_stats_snapshot_t *right)
{
    if (left == NULL ||
        right == NULL) {
        return false;
    }

    return
        memcmp(
            left,
            right,
            sizeof(*left)) == 0;
}

/* ========================================================================= */
/* Convenience capture macros                                                */
/* ========================================================================= */

#define VITTE_STATS_CAPTURE_ARENA(arena, stats) \
    vitte_stats_capture_arena( \
        (arena), \
        (stats))

#define VITTE_STATS_CAPTURE_POOL(pool, stats) \
    vitte_stats_capture_pool( \
        (pool), \
        (stats))

#define VITTE_STATS_CAPTURE( \
    arena, \
    pools, \
    pool_count, \
    snapshot) \
    vitte_stats_capture( \
        (arena), \
        (pools), \
        (pool_count), \
        (snapshot))

#define VITTE_STATS_DELTA( \
    before, \
    after, \
    delta) \
    vitte_stats_delta( \
        (before), \
        (after), \
        (delta))

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_STATS_BASIS_POINTS ==
        UINT32_C(10000),
    "Vitte statistics ratios use 10000 basis points");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte statistics require 64-bit uint64_t");

_Static_assert(
    sizeof(uint32_t) == 4u,
    "Vitte statistics require 32-bit uint32_t");

_Static_assert(
    PTRDIFF_MAX > 0,
    "ptrdiff_t must represent positive deltas");

_Static_assert(
    PTRDIFF_MIN < 0,
    "ptrdiff_t must represent negative deltas");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_STATS_H */
