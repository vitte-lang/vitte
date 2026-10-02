/*
 * Vitte Compiler
 * src/arena/stats.c
 *
 * Memory statistics and observation layer.
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
 * Design goals:
 *
 *     - no allocation
 *     - no mutation of observed allocators
 *     - overflow-safe aggregation
 *     - deterministic snapshots
 *     - arena statistics
 *     - pool statistics
 *     - aggregate pool statistics
 *     - global memory statistics
 *     - before/after deltas
 *     - high-water information
 *     - utilization ratios
 *     - padding/overhead information
 *     - validation
 *
 * Ratios are expressed in basis points:
 *
 *         0 =   0.00%
 *      5000 =  50.00%
 *     10000 = 100.00%
 */

#include "stats.h"

#include "allocator.h"
#include "arena.h"
#include "pool.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_STATS_BASIS_POINTS
#define VITTE_STATS_BASIS_POINTS UINT32_C(10000)
#endif

/* ========================================================================= */
/* Arithmetic                                                                */
/* ========================================================================= */

static bool
vitte_stats_add_overflow(
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

static size_t
vitte_stats_saturating_add(
    size_t left,
    size_t right,
    bool *saturated)
{
    size_t result;

    if (vitte_stats_add_overflow(
            left,
            right,
            &result)) {
        if (saturated != NULL) {
            *saturated = true;
        }

        return SIZE_MAX;
    }

    return result;
}

static size_t
vitte_stats_saturating_sub(
    size_t left,
    size_t right)
{
    if (left <= right) {
        return 0u;
    }

    return left - right;
}

static ptrdiff_t
vitte_stats_signed_delta(
    size_t before,
    size_t after,
    bool *saturated)
{
    size_t difference;

    if (after >= before) {
        difference = after - before;

        if (difference >
            (size_t)PTRDIFF_MAX) {
            if (saturated != NULL) {
                *saturated = true;
            }

            return PTRDIFF_MAX;
        }

        return (ptrdiff_t)difference;
    }

    difference = before - after;

    if (difference >
        (size_t)PTRDIFF_MAX) {
        if (saturated != NULL) {
            *saturated = true;
        }

        return PTRDIFF_MIN;
    }

    return -(ptrdiff_t)difference;
}

/* ========================================================================= */
/* Portable mul/div for basis-point ratios                                   */
/* ========================================================================= */

/*
 * Calculate:
 *
 *     floor(value * scale / total)
 *
 * without overflowing size_t.
 *
 * Requirements:
 *
 *     value <= total
 *     scale <= 10000 in current usage
 *
 * This uses quotient/remainder decomposition:
 *
 *     value = q * total + r
 *
 * For ratios value <= total, q is 0 or 1.
 *
 * The difficult part is r * scale. Rather than forming the potentially
 * overflowing product, perform binary accumulation modulo total.
 */
static size_t
vitte_stats_mul_div_floor(
    size_t value,
    size_t scale,
    size_t total)
{
    size_t quotient;
    size_t remainder;
    size_t result;
    size_t add;
    size_t multiplier;

    if (total == 0u ||
        scale == 0u ||
        value == 0u) {
        return 0u;
    }

    quotient = value / total;
    remainder = value % total;

    if (quotient != 0u) {
        if (quotient >
            SIZE_MAX / scale) {
            result = SIZE_MAX;
        } else {
            result = quotient * scale;
        }
    } else {
        result = 0u;
    }

    /*
     * Compute floor(remainder * scale / total).
     *
     * Since remainder < total, the result is < scale.
     *
     * Maintain:
     *
     *     add < total
     *
     * and accumulate quotient contributions separately.
     */
    add = remainder;
    multiplier = scale;

    {
        size_t fractional_result;

        fractional_result = 0u;

        while (multiplier != 0u) {
            if ((multiplier & 1u) != 0u) {
                /*
                 * fractional_result conceptually stores the quotient portion
                 * while add stores a remainder. Since scale is tiny for our
                 * use, use threshold arithmetic to avoid add+remainder
                 * overflow.
                 */
                if (add >= total) {
                    add %= total;
                }

                /*
                 * This implementation intentionally falls back to direct
                 * multiplication when provably safe.
                 */
            }

            multiplier >>= 1u;

            if (multiplier == 0u) {
                break;
            }

            if (add <=
                SIZE_MAX - add) {
                add += add;

                if (add >= total) {
                    add %= total;
                }
            } else {
                /*
                 * 2*add mod total without forming 2*add.
                 */
                if (add >= total - add) {
                    add =
                        add -
                        (total - add);
                } else {
                    add += add;
                }
            }
        }

        (void)fractional_result;
    }

    /*
     * Fast exact path covers every realistic allocator statistic and avoids
     * floating point completely.
     */
    if (remainder <=
        SIZE_MAX / scale) {
        size_t fraction;

        fraction =
            (remainder * scale) /
            total;

        if (result >
            SIZE_MAX - fraction) {
            return SIZE_MAX;
        }

        return result + fraction;
    }

    /*
     * Exact overflow-safe fallback.
     *
     * scale is bounded to basis-point scale, therefore repeated threshold
     * search is small and deterministic.
     */
    {
        size_t low;
        size_t high;

        low = 0u;
        high = scale;

        while (low < high) {
            size_t middle;
            size_t q;
            size_t r;
            size_t threshold;

            middle =
                low +
                (high - low + 1u) / 2u;

            /*
             * Determine:
             *
             *     middle * total <= remainder * scale
             *
             * by comparing:
             *
             *     ceil(middle * total / scale) <= remainder
             *
             * middle <= scale, so decompose total by scale first.
             */
            q = total / scale;
            r = total % scale;

            if (middle != 0u &&
                q > SIZE_MAX / middle) {
                threshold = SIZE_MAX;
            } else {
                size_t qpart;
                size_t rpart;

                qpart = q * middle;

                /*
                 * r < scale and middle <= scale. With basis-point scale this
                 * product is always tiny relative to size_t.
                 */
                rpart = r * middle;

                threshold =
                    qpart +
                    rpart / scale;

                if ((rpart % scale) != 0u) {
                    if (threshold != SIZE_MAX) {
                        ++threshold;
                    }
                }
            }

            if (threshold <= remainder) {
                low = middle;
            } else {
                high = middle - 1u;
            }
        }

        if (result >
            SIZE_MAX - low) {
            return SIZE_MAX;
        }

        return result + low;
    }
}

static uint32_t
vitte_stats_ratio_basis_points(
    size_t value,
    size_t total)
{
    size_t result;

    if (value == 0u ||
        total == 0u) {
        return UINT32_C(0);
    }

    if (value >= total) {
        return VITTE_STATS_BASIS_POINTS;
    }

    result =
        vitte_stats_mul_div_floor(
            value,
            (size_t)VITTE_STATS_BASIS_POINTS,
            total);

    if (result >
        (size_t)VITTE_STATS_BASIS_POINTS) {
        result =
            (size_t)VITTE_STATS_BASIS_POINTS;
    }

    return (uint32_t)result;
}

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

void
vitte_stats_snapshot_init(
    vitte_stats_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return;
    }

    memset(
        snapshot,
        0,
        sizeof(*snapshot));

    snapshot->valid = false;
}

void
vitte_stats_delta_init(
    vitte_stats_delta_t *delta)
{
    if (delta == NULL) {
        return;
    }

    memset(
        delta,
        0,
        sizeof(*delta));

    delta->valid = false;
}

/* ========================================================================= */
/* Arena capture                                                             */
/* ========================================================================= */

bool
vitte_stats_capture_arena(
    const vitte_arena_context_t *arena,
    vitte_stats_arena_t *stats)
{
    vitte_arena_context_stats_t source;

    if (stats == NULL) {
        return false;
    }

    memset(
        stats,
        0,
        sizeof(*stats));

    if (arena == NULL ||
        !vitte_arena_context_is_valid(
            arena)) {
        return false;
    }

    source =
        vitte_arena_context_stats(
            arena);

    stats->valid = true;

    stats->block_size =
        source.block_size;

    stats->block_count =
        source.block_count;

    stats->peak_block_count =
        source.peak_block_count;

    stats->reserved_bytes =
        source.reserved_bytes;

    stats->peak_reserved_bytes =
        source.peak_reserved_bytes;

    stats->used_bytes =
        source.used_bytes;

    stats->peak_used_bytes =
        source.peak_used_bytes;

    stats->requested_bytes =
        source.requested_bytes;

    stats->unused_bytes =
        source.unused_bytes;

    stats->padding_bytes =
        source.padding_bytes;

    stats->allocation_count =
        source.allocation_count;

    stats->generation =
        source.generation;

    stats->utilization_basis_points =
        vitte_stats_ratio_basis_points(
            stats->used_bytes,
            stats->reserved_bytes);

    stats->requested_utilization_basis_points =
        vitte_stats_ratio_basis_points(
            stats->requested_bytes,
            stats->reserved_bytes);

    stats->padding_basis_points =
        vitte_stats_ratio_basis_points(
            stats->padding_bytes,
            stats->used_bytes);

    return true;
}

/* ========================================================================= */
/* Pool capture                                                              */
/* ========================================================================= */

bool
vitte_stats_capture_pool(
    const vitte_pool_t *pool,
    vitte_stats_pool_t *stats)
{
    vitte_pool_stats_t source;
    size_t live_bytes;
    size_t slot_bytes;
    bool saturated;

    if (stats == NULL) {
        return false;
    }

    memset(
        stats,
        0,
        sizeof(*stats));

    if (pool == NULL ||
        !vitte_pool_is_valid(pool)) {
        return false;
    }

    source =
        vitte_pool_stats(pool);

    stats->valid = true;

    stats->object_size =
        source.object_size;

    stats->slot_size =
        source.slot_size;

    stats->alignment =
        source.alignment;

    stats->page_count =
        source.page_count;

    stats->peak_page_count =
        source.peak_page_count;

    stats->capacity =
        source.capacity;

    stats->live_count =
        source.live_count;

    stats->peak_live_count =
        source.peak_live_count;

    stats->free_count =
        source.free_count;

    stats->reserved_bytes =
        source.reserved_bytes;

    stats->peak_reserved_bytes =
        source.peak_reserved_bytes;

    stats->allocation_count =
        source.allocation_count;

    stats->free_operation_count =
        source.free_operation_count;

    stats->generation =
        source.generation;

    stats->max_objects =
        source.max_objects;

    stats->max_reserved_bytes =
        source.max_reserved_bytes;

    saturated = false;

    if (stats->live_count != 0u &&
        stats->object_size >
            SIZE_MAX / stats->live_count) {
        live_bytes = SIZE_MAX;
        saturated = true;
    } else {
        live_bytes =
            stats->live_count *
            stats->object_size;
    }

    if (stats->capacity != 0u &&
        stats->slot_size >
            SIZE_MAX / stats->capacity) {
        slot_bytes = SIZE_MAX;
        saturated = true;
    } else {
        slot_bytes =
            stats->capacity *
            stats->slot_size;
    }

    stats->live_object_bytes =
        live_bytes;

    stats->slot_storage_bytes =
        slot_bytes;

    stats->metadata_bytes =
        vitte_stats_saturating_sub(
            stats->reserved_bytes,
            stats->slot_storage_bytes);

    if (stats->slot_size >=
        stats->object_size) {
        size_t per_slot_padding;

        per_slot_padding =
            stats->slot_size -
            stats->object_size;

        if (stats->capacity != 0u &&
            per_slot_padding >
                SIZE_MAX /
                stats->capacity) {
            stats->slot_padding_bytes =
                SIZE_MAX;

            saturated = true;
        } else {
            stats->slot_padding_bytes =
                per_slot_padding *
                stats->capacity;
        }
    }

    stats->utilization_basis_points =
        vitte_stats_ratio_basis_points(
            stats->live_count,
            stats->capacity);

    stats->live_bytes_basis_points =
        vitte_stats_ratio_basis_points(
            stats->live_object_bytes,
            stats->reserved_bytes);

    stats->metadata_basis_points =
        vitte_stats_ratio_basis_points(
            stats->metadata_bytes,
            stats->reserved_bytes);

    stats->saturated = saturated;

    return true;
}

/* ========================================================================= */
/* Pool aggregation                                                          */
/* ========================================================================= */

bool
vitte_stats_capture_pools(
    vitte_pool_t *const *pools,
    size_t pool_count,
    vitte_stats_pools_t *stats)
{
    size_t index;
    bool saturated;

    if (stats == NULL) {
        return false;
    }

    memset(
        stats,
        0,
        sizeof(*stats));

    if (pool_count != 0u &&
        pools == NULL) {
        return false;
    }

    saturated = false;

    for (index = 0u;
         index < pool_count;
         ++index) {
        vitte_stats_pool_t current;

        if (pools[index] == NULL ||
            !vitte_stats_capture_pool(
                pools[index],
                &current)) {
            ++stats->invalid_pool_count;
            continue;
        }

        ++stats->valid_pool_count;

#define VITTE_STATS_ACCUMULATE(field)                                      \
        do {                                                                \
            stats->field =                                                  \
                vitte_stats_saturating_add(                                 \
                    stats->field,                                           \
                    current.field,                                          \
                    &saturated);                                            \
        } while (0)

        VITTE_STATS_ACCUMULATE(page_count);
        VITTE_STATS_ACCUMULATE(peak_page_count);

        VITTE_STATS_ACCUMULATE(capacity);

        VITTE_STATS_ACCUMULATE(live_count);
        VITTE_STATS_ACCUMULATE(peak_live_count);
        VITTE_STATS_ACCUMULATE(free_count);

        VITTE_STATS_ACCUMULATE(reserved_bytes);
        VITTE_STATS_ACCUMULATE(peak_reserved_bytes);

        VITTE_STATS_ACCUMULATE(live_object_bytes);
        VITTE_STATS_ACCUMULATE(slot_storage_bytes);
        VITTE_STATS_ACCUMULATE(slot_padding_bytes);
        VITTE_STATS_ACCUMULATE(metadata_bytes);

        VITTE_STATS_ACCUMULATE(allocation_count);
        VITTE_STATS_ACCUMULATE(free_operation_count);

#undef VITTE_STATS_ACCUMULATE

        if (current.saturated) {
            saturated = true;
        }
    }

    stats->pool_count =
        pool_count;

    stats->utilization_basis_points =
        vitte_stats_ratio_basis_points(
            stats->live_count,
            stats->capacity);

    stats->live_bytes_basis_points =
        vitte_stats_ratio_basis_points(
            stats->live_object_bytes,
            stats->reserved_bytes);

    stats->metadata_basis_points =
        vitte_stats_ratio_basis_points(
            stats->metadata_bytes,
            stats->reserved_bytes);

    stats->saturated =
        saturated;

    stats->valid =
        stats->invalid_pool_count == 0u;

    return stats->valid;
}

/* ========================================================================= */
/* Snapshot capture                                                          */
/* ========================================================================= */

bool
vitte_stats_capture(
    const vitte_arena_context_t *arena,
    vitte_pool_t *const *pools,
    size_t pool_count,
    vitte_stats_snapshot_t *snapshot)
{
    bool arena_ok;
    bool pools_ok;
    bool saturated;

    if (snapshot == NULL) {
        return false;
    }

    vitte_stats_snapshot_init(
        snapshot);

    if (arena == NULL &&
        pool_count == 0u) {
        return false;
    }

    if (pool_count != 0u &&
        pools == NULL) {
        return false;
    }

    arena_ok = true;
    pools_ok = true;

    if (arena != NULL) {
        arena_ok =
            vitte_stats_capture_arena(
                arena,
                &snapshot->arena);

        snapshot->has_arena =
            arena_ok;
    }

    if (pool_count != 0u) {
        pools_ok =
            vitte_stats_capture_pools(
                pools,
                pool_count,
                &snapshot->pools);

        snapshot->has_pools = true;
    }

    saturated = false;

    snapshot->total_reserved_bytes =
        vitte_stats_saturating_add(
            snapshot->arena.reserved_bytes,
            snapshot->pools.reserved_bytes,
            &saturated);

    snapshot->total_live_bytes =
        vitte_stats_saturating_add(
            snapshot->arena.used_bytes,
            snapshot->pools.live_object_bytes,
            &saturated);

    snapshot->total_requested_bytes =
        vitte_stats_saturating_add(
            snapshot->arena.requested_bytes,
            snapshot->pools.live_object_bytes,
            &saturated);

    snapshot->total_unused_bytes =
        vitte_stats_saturating_sub(
            snapshot->total_reserved_bytes,
            snapshot->total_live_bytes);

    snapshot->total_overhead_bytes =
        vitte_stats_saturating_add(
            snapshot->arena.padding_bytes,
            snapshot->pools.metadata_bytes,
            &saturated);

    snapshot->total_overhead_bytes =
        vitte_stats_saturating_add(
            snapshot->total_overhead_bytes,
            snapshot->pools.slot_padding_bytes,
            &saturated);

    snapshot->total_allocation_count =
        vitte_stats_saturating_add(
            snapshot->arena.allocation_count,
            snapshot->pools.allocation_count,
            &saturated);

    snapshot->utilization_basis_points =
        vitte_stats_ratio_basis_points(
            snapshot->total_live_bytes,
            snapshot->total_reserved_bytes);

    snapshot->overhead_basis_points =
        vitte_stats_ratio_basis_points(
            snapshot->total_overhead_bytes,
            snapshot->total_reserved_bytes);

    snapshot->saturated =
        saturated ||
        snapshot->pools.saturated;

    snapshot->valid =
        arena_ok &&
        pools_ok;

    return snapshot->valid;
}

/* ========================================================================= */
/* Delta                                                                     */
/* ========================================================================= */

bool
vitte_stats_delta(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after,
    vitte_stats_delta_t *delta)
{
    bool saturated;

    if (delta == NULL) {
        return false;
    }

    vitte_stats_delta_init(delta);

    if (before == NULL ||
        after == NULL ||
        !before->valid ||
        !after->valid) {
        return false;
    }

    if (before->has_arena !=
            after->has_arena ||
        before->has_pools !=
            after->has_pools) {
        return false;
    }

    saturated = false;

#define VITTE_STATS_DELTA_FIELD(field)                                      \
    delta->field =                                                          \
        vitte_stats_signed_delta(                                           \
            before->field,                                                  \
            after->field,                                                   \
            &saturated)

    VITTE_STATS_DELTA_FIELD(total_reserved_bytes);
    VITTE_STATS_DELTA_FIELD(total_live_bytes);
    VITTE_STATS_DELTA_FIELD(total_requested_bytes);
    VITTE_STATS_DELTA_FIELD(total_unused_bytes);
    VITTE_STATS_DELTA_FIELD(total_overhead_bytes);
    VITTE_STATS_DELTA_FIELD(total_allocation_count);

#undef VITTE_STATS_DELTA_FIELD

#define VITTE_STATS_ARENA_DELTA(field)                                      \
    delta->arena_##field =                                                  \
        vitte_stats_signed_delta(                                           \
            before->arena.field,                                            \
            after->arena.field,                                             \
            &saturated)

    VITTE_STATS_ARENA_DELTA(block_count);
    VITTE_STATS_ARENA_DELTA(reserved_bytes);
    VITTE_STATS_ARENA_DELTA(used_bytes);
    VITTE_STATS_ARENA_DELTA(requested_bytes);
    VITTE_STATS_ARENA_DELTA(unused_bytes);
    VITTE_STATS_ARENA_DELTA(padding_bytes);
    VITTE_STATS_ARENA_DELTA(allocation_count);

#undef VITTE_STATS_ARENA_DELTA

#define VITTE_STATS_POOLS_DELTA(field)                                      \
    delta->pools_##field =                                                  \
        vitte_stats_signed_delta(                                           \
            before->pools.field,                                            \
            after->pools.field,                                             \
            &saturated)

    VITTE_STATS_POOLS_DELTA(page_count);
    VITTE_STATS_POOLS_DELTA(capacity);
    VITTE_STATS_POOLS_DELTA(live_count);
    VITTE_STATS_POOLS_DELTA(free_count);
    VITTE_STATS_POOLS_DELTA(reserved_bytes);
    VITTE_STATS_POOLS_DELTA(live_object_bytes);
    VITTE_STATS_POOLS_DELTA(metadata_bytes);
    VITTE_STATS_POOLS_DELTA(allocation_count);
    VITTE_STATS_POOLS_DELTA(free_operation_count);

#undef VITTE_STATS_POOLS_DELTA

    delta->utilization_before_basis_points =
        before->utilization_basis_points;

    delta->utilization_after_basis_points =
        after->utilization_basis_points;

    delta->overhead_before_basis_points =
        before->overhead_basis_points;

    delta->overhead_after_basis_points =
        after->overhead_basis_points;

    delta->saturated =
        saturated ||
        before->saturated ||
        after->saturated;

    delta->valid = true;

    return true;
}

/* ========================================================================= */
/* Released / increased helpers                                              */
/* ========================================================================= */

size_t
vitte_stats_reserved_released(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after)
{
    if (before == NULL ||
        after == NULL ||
        !before->valid ||
        !after->valid) {
        return 0u;
    }

    return
        vitte_stats_saturating_sub(
            before->total_reserved_bytes,
            after->total_reserved_bytes);
}

size_t
vitte_stats_reserved_increased(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after)
{
    if (before == NULL ||
        after == NULL ||
        !before->valid ||
        !after->valid) {
        return 0u;
    }

    return
        vitte_stats_saturating_sub(
            after->total_reserved_bytes,
            before->total_reserved_bytes);
}

size_t
vitte_stats_live_released(
    const vitte_stats_snapshot_t *before,
    const vitte_stats_snapshot_t *after)
{
    if (before == NULL ||
        after == NULL ||
        !before->valid ||
        !after->valid) {
        return 0u;
    }

    return
        vitte_stats_saturating_sub(
            before->total_live_bytes,
            after->total_live_bytes);
}

/* ========================================================================= */
/* Comparison                                                                */
/* ========================================================================= */

int
vitte_stats_compare_reserved(
    const vitte_stats_snapshot_t *left,
    const vitte_stats_snapshot_t *right)
{
    if (left == NULL ||
        right == NULL) {
        return 0;
    }

    if (left->total_reserved_bytes <
        right->total_reserved_bytes) {
        return -1;
    }

    if (left->total_reserved_bytes >
        right->total_reserved_bytes) {
        return 1;
    }

    return 0;
}

int
vitte_stats_compare_live(
    const vitte_stats_snapshot_t *left,
    const vitte_stats_snapshot_t *right)
{
    if (left == NULL ||
        right == NULL) {
        return 0;
    }

    if (left->total_live_bytes <
        right->total_live_bytes) {
        return -1;
    }

    if (left->total_live_bytes >
        right->total_live_bytes) {
        return 1;
    }

    return 0;
}

/* ========================================================================= */
/* Arena validation                                                          */
/* ========================================================================= */

bool
vitte_stats_validate_arena(
    const vitte_stats_arena_t *stats)
{
    if (stats == NULL ||
        !stats->valid) {
        return false;
    }

    if (stats->used_bytes >
        stats->reserved_bytes) {
        return false;
    }

    if (stats->requested_bytes >
        stats->used_bytes) {
        /*
         * With a normal bump allocator requested bytes should not exceed
         * physical used bytes. If the low-level accounting deliberately uses
         * different semantics this invariant must be adjusted there and here
         * together.
         */
        return false;
    }

    if (stats->unused_bytes !=
        stats->reserved_bytes -
        stats->used_bytes) {
        return false;
    }

    if (stats->padding_bytes >
        stats->used_bytes) {
        return false;
    }

    if (stats->peak_block_count <
        stats->block_count) {
        return false;
    }

    if (stats->peak_reserved_bytes <
        stats->reserved_bytes) {
        return false;
    }

    if (stats->peak_used_bytes <
        stats->used_bytes) {
        return false;
    }

    if (stats->utilization_basis_points >
        VITTE_STATS_BASIS_POINTS) {
        return false;
    }

    if (stats->requested_utilization_basis_points >
        VITTE_STATS_BASIS_POINTS) {
        return false;
    }

    if (stats->padding_basis_points >
        VITTE_STATS_BASIS_POINTS) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Pool validation                                                           */
/* ========================================================================= */

bool
vitte_stats_validate_pool(
    const vitte_stats_pool_t *stats)
{
    if (stats == NULL ||
        !stats->valid) {
        return false;
    }

    if (stats->object_size == 0u ||
        stats->slot_size == 0u ||
        stats->alignment == 0u) {
        return false;
    }

    if (stats->slot_size <
        stats->object_size) {
        return false;
    }

    if (stats->live_count >
        stats->capacity ||
        stats->free_count >
        stats->capacity) {
        return false;
    }

    if (stats->capacity -
            stats->live_count !=
        stats->free_count) {
        return false;
    }

    if (stats->peak_live_count <
        stats->live_count) {
        return false;
    }

    if (stats->peak_page_count <
        stats->page_count) {
        return false;
    }

    if (stats->peak_reserved_bytes <
        stats->reserved_bytes) {
        return false;
    }

    if (stats->live_object_bytes >
        stats->reserved_bytes &&
        !stats->saturated) {
        return false;
    }

    if (stats->slot_storage_bytes >
        stats->reserved_bytes &&
        !stats->saturated) {
        return false;
    }

    if (stats->metadata_bytes >
        stats->reserved_bytes) {
        return false;
    }

    if (stats->utilization_basis_points >
        VITTE_STATS_BASIS_POINTS ||
        stats->live_bytes_basis_points >
            VITTE_STATS_BASIS_POINTS ||
        stats->metadata_basis_points >
            VITTE_STATS_BASIS_POINTS) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Aggregate pool validation                                                 */
/* ========================================================================= */

bool
vitte_stats_validate_pools(
    const vitte_stats_pools_t *stats)
{
    if (stats == NULL) {
        return false;
    }

    if (stats->valid &&
        stats->invalid_pool_count != 0u) {
        return false;
    }

    if (stats->valid_pool_count >
        stats->pool_count ||
        stats->invalid_pool_count >
            stats->pool_count) {
        return false;
    }

    if (stats->valid_pool_count >
        SIZE_MAX -
        stats->invalid_pool_count) {
        return false;
    }

    if (stats->valid_pool_count +
            stats->invalid_pool_count !=
        stats->pool_count) {
        return false;
    }

    if (!stats->saturated) {
        if (stats->live_count >
            stats->capacity ||
            stats->free_count >
                stats->capacity) {
            return false;
        }

        if (stats->capacity -
                stats->live_count !=
            stats->free_count) {
            return false;
        }
    }

    if (stats->utilization_basis_points >
        VITTE_STATS_BASIS_POINTS ||
        stats->live_bytes_basis_points >
            VITTE_STATS_BASIS_POINTS ||
        stats->metadata_basis_points >
            VITTE_STATS_BASIS_POINTS) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Snapshot validation                                                       */
/* ========================================================================= */

bool
vitte_stats_validate_snapshot(
    const vitte_stats_snapshot_t *snapshot)
{
    size_t expected;
    bool saturated;

    if (snapshot == NULL ||
        !snapshot->valid) {
        return false;
    }

    if (!snapshot->has_arena &&
        !snapshot->has_pools) {
        return false;
    }

    if (snapshot->has_arena &&
        !vitte_stats_validate_arena(
            &snapshot->arena)) {
        return false;
    }

    if (snapshot->has_pools &&
        !vitte_stats_validate_pools(
            &snapshot->pools)) {
        return false;
    }

    saturated = false;

    expected =
        vitte_stats_saturating_add(
            snapshot->arena.reserved_bytes,
            snapshot->pools.reserved_bytes,
            &saturated);

    if (snapshot->total_reserved_bytes !=
        expected) {
        return false;
    }

    expected =
        vitte_stats_saturating_add(
            snapshot->arena.used_bytes,
            snapshot->pools.live_object_bytes,
            &saturated);

    if (snapshot->total_live_bytes !=
        expected) {
        return false;
    }

    if (snapshot->total_live_bytes >
            snapshot->total_reserved_bytes &&
        !snapshot->saturated) {
        return false;
    }

    if (snapshot->utilization_basis_points >
        VITTE_STATS_BASIS_POINTS ||
        snapshot->overhead_basis_points >
            VITTE_STATS_BASIS_POINTS) {
        return false;
    }

    if (saturated &&
        !snapshot->saturated) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Delta validation                                                          */
/* ========================================================================= */

bool
vitte_stats_validate_delta(
    const vitte_stats_delta_t *delta)
{
    if (delta == NULL ||
        !delta->valid) {
        return false;
    }

    if (delta->utilization_before_basis_points >
            VITTE_STATS_BASIS_POINTS ||
        delta->utilization_after_basis_points >
            VITTE_STATS_BASIS_POINTS ||
        delta->overhead_before_basis_points >
            VITTE_STATS_BASIS_POINTS ||
        delta->overhead_after_basis_points >
            VITTE_STATS_BASIS_POINTS) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
