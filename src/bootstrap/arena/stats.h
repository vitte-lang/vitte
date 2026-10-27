#ifndef VITTE_SRC_ARENA_STATS_H
#define VITTE_SRC_ARENA_STATS_H

/*
 * Vitte Compiler
 * src/arena/stats.h
 *
 * Memory statistics and observation layer.
 *
 * ISO C17.
 *
 * Architecture:
 *
 *     block.c/.h
 *         physical arena blocks
 *
 *     allocator.c/.h
 *         low-level monotonic allocator
 *
 *     arena.c/.h
 *         compiler-facing arena context
 *
 *     checkpoint.c/.h
 *         transactional checkpoints
 *
 *     pool.c/.h
 *         reusable fixed-size pools
 *
 *     reset.c/.h
 *         coordinated reset policy
 *
 *     stats.c/.h
 *         observation, aggregation and deltas
 *
 * Properties:
 *
 *     - no allocation
 *     - no mutation of observed allocators
 *     - deterministic snapshots
 *     - overflow-safe aggregation
 *     - saturating aggregate counters
 *     - arena statistics
 *     - pool statistics
 *     - aggregate pool statistics
 *     - global memory statistics
 *     - before/after deltas
 *     - high-water information
 *     - utilization ratios
 *     - padding/overhead information
 *     - structural validation
 *
 * Ratios use basis points:
 *
 *         0 =   0.00%
 *      5000 =  50.00%
 *     10000 = 100.00%
 *
 * Saturation:
 *
 * Aggregate size_t counters saturate at SIZE_MAX instead of wrapping.
 *
 * When a snapshot or aggregate reports saturated == true, one or more
 * quantities no longer represent an exact mathematical value. Such
 * observations remain useful for diagnostics/high-water reporting, but
 * exact arithmetic comparisons should be treated accordingly.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arena.h"
#include "pool.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_STATS_BASIS_POINTS
#define VITTE_STATS_BASIS_POINTS UINT32_C(10000)
#endif

/* ========================================================================= */
/* Arena statistics                                                         */
/* ========================================================================= */

/*
 * Compiler-facing arena observation.
 *
 * requested_bytes:
 *
 *     Current logical bytes requested through the arena context.
 *
 * used_bytes:
 *
 *     Current physical bytes consumed inside arena blocks.
 *
 * padding_bytes:
 *
 *     Physical used bytes not represented by requested payload according to
 *     arena accounting.
 *
 *     This may include alignment padding and physical storage consumed by
 *     zero-byte logical allocations.
 *
 * unused_bytes:
 *
 *     reserved_bytes - used_bytes
 */
typedef struct vitte_stats_arena {
    bool valid;

    size_t block_size;

    size_t block_count;
    size_t peak_block_count;

    size_t reserved_bytes;
    size_t peak_reserved_bytes;

    size_t used_bytes;
    size_t peak_used_bytes;

    size_t requested_bytes;

    size_t unused_bytes;
    size_t padding_bytes;

    size_t allocation_count;

    uint64_t generation;

    /*
     * used_bytes / reserved_bytes
     */
    uint32_t utilization_basis_points;

    /*
     * requested_bytes / reserved_bytes
     */
    uint32_t requested_utilization_basis_points;

    /*
     * padding_bytes / used_bytes
     */
    uint32_t padding_basis_points;
} vitte_stats_arena_t;

/* ========================================================================= */
/* Pool statistics                                                          */
/* ========================================================================= */

/*
 * Observation of one fixed-size object pool.
 *
 * live_object_bytes:
 *
 *     live_count * object_size
 *
 * slot_storage_bytes:
 *
 *     capacity * slot_size
 *
 * slot_padding_bytes:
 *
 *     capacity * (slot_size - object_size)
 *
 * metadata_bytes:
 *
 *     reserved_bytes - slot_storage_bytes
 *
 * provided those values remain exactly representable.
 *
 * metadata_bytes therefore represents page headers, allocation bitmaps,
 * page-level alignment overhead and other storage outside physical slots.
 */
typedef struct vitte_stats_pool {
    bool valid;

    size_t object_size;
    size_t slot_size;
    size_t alignment;

    size_t page_count;
    size_t peak_page_count;

    size_t capacity;

    size_t live_count;
    size_t peak_live_count;

    size_t free_count;

    size_t reserved_bytes;
    size_t peak_reserved_bytes;

    size_t live_object_bytes;
    size_t slot_storage_bytes;
    size_t slot_padding_bytes;
    size_t metadata_bytes;

    /*
     * Lifetime successful allocation operations.
     */
    size_t allocation_count;

    /*
     * Lifetime successful explicit free operations.
     *
     * Pool reset does not count as individual free operations.
     */
    size_t free_operation_count;

    uint64_t generation;

    size_t max_objects;
    size_t max_reserved_bytes;

    /*
     * live_count / capacity
     */
    uint32_t utilization_basis_points;

    /*
     * live_object_bytes / reserved_bytes
     */
    uint32_t live_bytes_basis_points;

    /*
     * metadata_bytes / reserved_bytes
     */
    uint32_t metadata_basis_points;

    /*
     * At least one derived quantity could not be represented exactly in
     * size_t and was saturated or otherwise conservatively represented.
     */
    bool saturated;
} vitte_stats_pool_t;

/* ========================================================================= */
/* Aggregate pool statistics                                                */
/* ========================================================================= */

/*
 * Aggregate statistics for a caller-supplied set of pools.
 *
 * valid:
 *
 *     true only when every requested pool was successfully captured.
 *
 * saturated:
 *
 *     true when at least one source pool was saturated or an aggregate
 *     addition saturated.
 *
 * A saturated aggregate may still be valid: validity describes source
 * completeness/structural capture, whereas saturation describes numerical
 * precision.
 */
typedef struct vitte_stats_pools {
    bool valid;
    bool saturated;

    size_t pool_count;
    size_t valid_pool_count;
    size_t invalid_pool_count;

    size_t page_count;
    size_t peak_page_count;

    size_t capacity;

    size_t live_count;
    size_t peak_live_count;

    size_t free_count;

    size_t reserved_bytes;
    size_t peak_reserved_bytes;

    size_t live_object_bytes;
    size_t slot_storage_bytes;
    size_t slot_padding_bytes;
    size_t metadata_bytes;

    size_t allocation_count;
    size_t free_operation_count;

    /*
     * live_count / capacity
     */
    uint32_t utilization_basis_points;

    /*
     * live_object_bytes / reserved_bytes
     */
    uint32_t live_bytes_basis_points;

    /*
     * metadata_bytes / reserved_bytes
     */
    uint32_t metadata_basis_points;
} vitte_stats_pools_t;

/* ========================================================================= */
/* Global snapshot                                                          */
/* ========================================================================= */

/*
 * Combined arena/pool observation.
 *
 * total_reserved_bytes:
 *
 *     arena reserved bytes + pool reserved bytes
 *
 * total_live_bytes:
 *
 *     arena physical used bytes + pool logical live-object payload
 *
 * The two components intentionally have slightly different meanings:
 *
 *     arena memory is monotonic and its current physical used bytes describe
 *     memory associated with the current arena generation;
 *
 *     pool slots are reusable, therefore only logical payload belonging to
 *     currently live objects contributes to total_live_bytes.
 *
 * total_requested_bytes:
 *
 *     arena logical requested bytes + pool live-object payload
 *
 * total_unused_bytes:
 *
 *     max(total_reserved_bytes - total_live_bytes, 0)
 *
 * total_overhead_bytes:
 *
 *     arena padding
 *   + pool metadata
 *   + pool per-slot padding
 *
 * Free reusable pool slot payload is not classified as overhead.
 *
 * total_allocation_count:
 *
 *     arena current logical allocation count
 *   + pool lifetime successful allocation-operation count
 *
 * Consequently total_allocation_count is an observation counter, not a
 * current live-object count.
 */
typedef struct vitte_stats_snapshot {
    bool valid;

    bool has_arena;
    bool has_pools;

    vitte_stats_arena_t arena;
    vitte_stats_pools_t pools;

    size_t total_reserved_bytes;
    size_t total_live_bytes;
    size_t total_requested_bytes;
    size_t total_unused_bytes;
    size_t total_overhead_bytes;

    size_t total_allocation_count;

    /*
     * total_live_bytes / total_reserved_bytes
     */
    uint32_t utilization_basis_points;

    /*
     * total_overhead_bytes / total_reserved_bytes
     */
    uint32_t overhead_basis_points;

    /*
     * At least one aggregate quantity lost exact precision because it
     * saturated.
     */
    bool saturated;
} vitte_stats_snapshot_t;

/* ========================================================================= */
/* Delta                                                                     */
/* ========================================================================= */

/*
 * Signed before/after delta.
 *
 * Sign convention:
 *
 *     positive
 *         value increased from before to after
 *
 *     zero
 *         unchanged
 *
 *     negative
 *         value decreased from before to after
 *
 * Each signed quantity saturates to PTRDIFF_MIN/PTRDIFF_MAX when the exact
 * mathematical difference cannot be represented by ptrdiff_t.
 *
 * saturated is also propagated from either endpoint snapshot.
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

/*
 * Initialize an empty invalid snapshot.
 */
void
vitte_stats_snapshot_init(
    vitte_stats_snapshot_t *snapshot);

/*
 * Initialize an empty invalid delta.
 */
void
vitte_stats_delta_init(
    vitte_stats_delta_t *delta);

/* ========================================================================= */
/* Arena capture                                                             */
/* ========================================================================= */

/*
 * Capture one compiler-facing arena context.
 *
 * This is observational:
 *
 *     - no allocation
 *     - no reset
 *     - no mutation of arena state
 *
 * Returns false for NULL/invalid arena or NULL output.
 *
 * This operation does not automatically invoke the expensive deep arena
 * validator.
 */
bool
vitte_stats_capture_arena(
    const vitte_arena_context_t *arena,
    vitte_stats_arena_t *stats);

/* ========================================================================= */
/* Pool capture                                                              */
/* ========================================================================= */

/*
 * Capture one pool.
 *
 * Derived byte quantities use overflow-safe arithmetic. If an exact derived
 * value cannot fit in size_t, saturated is set.
 */
bool
vitte_stats_capture_pool(
    const vitte_pool_t *pool,
    vitte_stats_pool_t *stats);

/* ========================================================================= */
/* Pool aggregation                                                          */
/* ========================================================================= */

/*
 * Aggregate pool statistics.
 *
 * pools:
 *
 *     Array of pool pointers.
 *
 * pool_count == 0:
 *
 *     pools may be NULL. The resulting empty aggregate is valid.
 *
 * Invalid pools are counted in invalid_pool_count. Valid pools are still
 * observed and accumulated, but the final aggregate valid flag and function
 * return value are false when any requested pool is invalid.
 */
bool
vitte_stats_capture_pools(
    vitte_pool_t *const *pools,
    size_t pool_count,
    vitte_stats_pools_t *stats);

/* ========================================================================= */
/* Snapshot capture                                                          */
/* ========================================================================= */

/*
 * Capture a combined memory snapshot.
 *
 * At least one source must be requested:
 *
 *     arena != NULL
 *
 * or:
 *
 *     pool_count != 0
 *
 * When pool_count != 0, pools must be non-NULL.
 *
 * A snapshot is valid only if every requested source was successfully
 * captured.
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
 * Compute after - before.
 *
 * Both snapshots must:
 *
 *     - be valid
 *     - have the same source shape
 *
 * Source shape means has_arena and has_pools must match.
 */
bool
vitte_stats_delta(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after,
    vitte_stats_delta_t *delta);

/* ========================================================================= */
/* Released / increased helpers                                              */
/* ========================================================================= */

/*
 * Return max(before.reserved - after.reserved, 0).
 *
 * Returns zero for invalid snapshots or incompatible snapshot shapes.
 */
size_t
vitte_stats_reserved_released(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after);

/*
 * Return max(after.reserved - before.reserved, 0).
 */
size_t
vitte_stats_reserved_increased(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after);

/*
 * Return max(before.live - after.live, 0).
 */
size_t
vitte_stats_live_released(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after);

/* ========================================================================= */
/* Comparison                                                                */
/* ========================================================================= */

/*
 * Compare total reserved bytes.
 *
 * Ordering:
 *
 *     NULL < non-NULL
 *
 * Returns:
 *
 *     < 0
 *         left < right
 *
 *       0
 *         equal
 *
 *     > 0
 *         left > right
 *
 * The comparison does not require valid snapshots.
 */
int
vitte_stats_compare_reserved(
    const vitte_stats_snapshot_t *left,
    const vitte_stats_snapshot_t *right);

/*
 * Compare total live bytes.
 *
 * Uses the same NULL ordering as vitte_stats_compare_reserved().
 */
int
vitte_stats_compare_live(
    const vitte_stats_snapshot_t *left,
    const vitte_stats_snapshot_t *right);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate an arena observation.
 *
 * Checks include:
 *
 *     - valid flag
 *     - non-zero generation
 *     - used <= reserved
 *     - requested <= used
 *     - unused == reserved - used
 *     - padding <= used
 *     - peak counters
 *     - empty-state consistency
 *     - basis-point ranges
 *     - basis-point recomputation
 */
bool
vitte_stats_validate_arena(
    const vitte_stats_arena_t *stats);

/*
 * Validate one pool observation.
 *
 * Checks include:
 *
 *     - valid flag
 *     - object/slot/alignment invariants
 *     - power-of-two alignment
 *     - slot-size alignment
 *     - live/free/capacity accounting
 *     - peak counters
 *     - configured limits
 *     - derived byte accounting when unsaturated
 *     - ratio ranges
 *     - ratio recomputation when unsaturated
 */
bool
vitte_stats_validate_pool(
    const vitte_stats_pool_t *stats);

/*
 * Validate aggregate pool statistics.
 */
bool
vitte_stats_validate_pools(
    const vitte_stats_pools_t *stats);

/*
 * Validate a complete snapshot and recompute its aggregate quantities.
 */
bool
vitte_stats_validate_snapshot(
    const vitte_stats_snapshot_t *snapshot);

/*
 * Validate delta metadata and ratio ranges.
 *
 * A delta cannot reconstruct its endpoint snapshots, therefore structural
 * validation is intentionally limited to invariants representable inside the
 * delta itself.
 */
bool
vitte_stats_validate_delta(
    const vitte_stats_delta_t *delta);

/* ========================================================================= */
/* Snapshot predicates                                                       */
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
        vitte_stats_snapshot_is_valid(snapshot) &&
        snapshot->has_arena;
}

static inline bool
vitte_stats_snapshot_has_pools(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        vitte_stats_snapshot_is_valid(snapshot) &&
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
vitte_stats_snapshot_is_empty(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        vitte_stats_snapshot_is_valid(snapshot) &&
        snapshot->total_live_bytes == 0u;
}

static inline bool
vitte_stats_snapshot_has_live_memory(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        vitte_stats_snapshot_is_valid(snapshot) &&
        snapshot->total_live_bytes != 0u;
}

static inline bool
vitte_stats_snapshot_has_reserved_memory(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        vitte_stats_snapshot_is_valid(snapshot) &&
        snapshot->total_reserved_bytes != 0u;
}

static inline bool
vitte_stats_snapshot_has_overhead(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        vitte_stats_snapshot_is_valid(snapshot) &&
        snapshot->total_overhead_bytes != 0u;
}

/* ========================================================================= */
/* Arena predicates                                                          */
/* ========================================================================= */

static inline bool
vitte_stats_arena_is_valid(
    const vitte_stats_arena_t *stats)
{
    return
        stats != NULL &&
        stats->valid;
}

static inline bool
vitte_stats_arena_is_empty(
    const vitte_stats_arena_t *stats)
{
    return
        vitte_stats_arena_is_valid(stats) &&
        stats->allocation_count == 0u;
}

static inline bool
vitte_stats_arena_has_blocks(
    const vitte_stats_arena_t *stats)
{
    return
        vitte_stats_arena_is_valid(stats) &&
        stats->block_count != 0u;
}

/* ========================================================================= */
/* Pool predicates                                                           */
/* ========================================================================= */

static inline bool
vitte_stats_pool_is_valid(
    const vitte_stats_pool_t *stats)
{
    return
        stats != NULL &&
        stats->valid;
}

static inline bool
vitte_stats_pool_is_empty(
    const vitte_stats_pool_t *stats)
{
    return
        vitte_stats_pool_is_valid(stats) &&
        stats->live_count == 0u;
}

static inline bool
vitte_stats_pool_is_full(
    const vitte_stats_pool_t *stats)
{
    return
        vitte_stats_pool_is_valid(stats) &&
        stats->capacity != 0u &&
        stats->live_count ==
            stats->capacity;
}

static inline bool
vitte_stats_pool_has_pages(
    const vitte_stats_pool_t *stats)
{
    return
        vitte_stats_pool_is_valid(stats) &&
        stats->page_count != 0u;
}

static inline bool
vitte_stats_pool_is_saturated(
    const vitte_stats_pool_t *stats)
{
    return
        stats != NULL &&
        stats->saturated;
}

/* ========================================================================= */
/* Aggregate pool predicates                                                 */
/* ========================================================================= */

static inline bool
vitte_stats_pools_are_valid(
    const vitte_stats_pools_t *stats)
{
    return
        stats != NULL &&
        stats->valid;
}

static inline bool
vitte_stats_pools_are_saturated(
    const vitte_stats_pools_t *stats)
{
    return
        stats != NULL &&
        stats->saturated;
}

static inline bool
vitte_stats_pools_have_invalid(
    const vitte_stats_pools_t *stats)
{
    return
        stats != NULL &&
        stats->invalid_pool_count != 0u;
}

static inline bool
vitte_stats_pools_are_empty(
    const vitte_stats_pools_t *stats)
{
    return
        vitte_stats_pools_are_valid(stats) &&
        stats->live_count == 0u;
}

/* ========================================================================= */
/* Delta predicates                                                          */
/* ========================================================================= */

static inline bool
vitte_stats_delta_is_valid(
    const vitte_stats_delta_t *delta)
{
    return
        delta != NULL &&
        delta->valid;
}

static inline bool
vitte_stats_delta_is_saturated(
    const vitte_stats_delta_t *delta)
{
    return
        delta != NULL &&
        delta->saturated;
}

static inline bool
vitte_stats_reserved_decreased(
    const vitte_stats_delta_t *delta)
{
    return
        vitte_stats_delta_is_valid(delta) &&
        delta->total_reserved_bytes < 0;
}

static inline bool
vitte_stats_reserved_grew(
    const vitte_stats_delta_t *delta)
{
    return
        vitte_stats_delta_is_valid(delta) &&
        delta->total_reserved_bytes > 0;
}

static inline bool
vitte_stats_live_decreased(
    const vitte_stats_delta_t *delta)
{
    return
        vitte_stats_delta_is_valid(delta) &&
        delta->total_live_bytes < 0;
}

static inline bool
vitte_stats_live_grew(
    const vitte_stats_delta_t *delta)
{
    return
        vitte_stats_delta_is_valid(delta) &&
        delta->total_live_bytes > 0;
}

/* ========================================================================= */
/* Ratio helpers                                                             */
/* ========================================================================= */

static inline uint32_t
vitte_stats_utilization_basis_points(
    const vitte_stats_snapshot_t *snapshot)
{
    if (!vitte_stats_snapshot_is_valid(
            snapshot)) {
        return UINT32_C(0);
    }

    return
        snapshot->utilization_basis_points;
}

static inline uint32_t
vitte_stats_overhead_basis_points(
    const vitte_stats_snapshot_t *snapshot)
{
    if (!vitte_stats_snapshot_is_valid(
            snapshot)) {
        return UINT32_C(0);
    }

    return
        snapshot->overhead_basis_points;
}

/*
 * Return whole utilization percentage in [0, 100].
 *
 * Fractional basis-point precision is intentionally discarded.
 */
static inline uint32_t
vitte_stats_utilization_percent(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        vitte_stats_utilization_basis_points(
            snapshot) /
        UINT32_C(100);
}

static inline uint32_t
vitte_stats_overhead_percent(
    const vitte_stats_snapshot_t *snapshot)
{
    return
        vitte_stats_overhead_basis_points(
            snapshot) /
        UINT32_C(100);
}

/* ========================================================================= */
/* Snapshot accessors                                                        */
/* ========================================================================= */

static inline size_t
vitte_stats_reserved_bytes(
    const vitte_stats_snapshot_t *snapshot)
{
    if (!vitte_stats_snapshot_is_valid(
            snapshot)) {
        return 0u;
    }

    return
        snapshot->total_reserved_bytes;
}

static inline size_t
vitte_stats_live_bytes(
    const vitte_stats_snapshot_t *snapshot)
{
    if (!vitte_stats_snapshot_is_valid(
            snapshot)) {
        return 0u;
    }

    return
        snapshot->total_live_bytes;
}

static inline size_t
vitte_stats_requested_bytes(
    const vitte_stats_snapshot_t *snapshot)
{
    if (!vitte_stats_snapshot_is_valid(
            snapshot)) {
        return 0u;
    }

    return
        snapshot->total_requested_bytes;
}

static inline size_t
vitte_stats_unused_bytes(
    const vitte_stats_snapshot_t *snapshot)
{
    if (!vitte_stats_snapshot_is_valid(
            snapshot)) {
        return 0u;
    }

    return
        snapshot->total_unused_bytes;
}

static inline size_t
vitte_stats_overhead_bytes(
    const vitte_stats_snapshot_t *snapshot)
{
    if (!vitte_stats_snapshot_is_valid(
            snapshot)) {
        return 0u;
    }

    return
        snapshot->total_overhead_bytes;
}

static inline size_t
vitte_stats_allocation_count(
    const vitte_stats_snapshot_t *snapshot)
{
    if (!vitte_stats_snapshot_is_valid(
            snapshot)) {
        return 0u;
    }

    return
        snapshot->total_allocation_count;
}

/* ========================================================================= */
/* Saturating public helpers                                                 */
/* ========================================================================= */

static inline size_t
vitte_stats_saturating_sum(
    size_t left,
    size_t right)
{
    if (left > SIZE_MAX - right) {
        return SIZE_MAX;
    }

    return left + right;
}

static inline size_t
vitte_stats_saturating_difference(
    size_t left,
    size_t right)
{
    if (left <= right) {
        return 0u;
    }

    return left - right;
}

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_STATS_BASIS_POINTS == UINT32_C(10000),
    "Vitte statistics basis-point scale must remain 10000");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte statistics require 64-bit uint64_t");

_Static_assert(
    PTRDIFF_MAX > 0,
    "Vitte statistics require signed ptrdiff_t");

_Static_assert(
    sizeof(ptrdiff_t) <= sizeof(size_t),
    "ptrdiff_t must not exceed size_t width");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_STATS_H */