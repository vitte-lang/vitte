/*
 * Vitte Compiler
 * src/arena/block.c
 *
 * Low-level arena block implementation.
 *
 * Responsibilities:
 *
 *   - block creation/destruction
 *   - capacity normalization
 *   - overflow-safe size/address arithmetic
 *   - aligned bump allocation
 *   - allocation feasibility queries
 *   - reset/reuse
 *   - rewind
 *   - optional memory poisoning
 *   - block statistics
 *   - ownership queries
 *   - offset/pointer conversion
 *   - utilization accounting
 *   - structural validation
 *
 * allocator.c owns block-list policy and topology. This module only validates
 * the immediate links visible from an individual block.
 *
 * ISO C17.
 */

#include "block.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_ARENA_BLOCK_MAGIC
#define VITTE_ARENA_BLOCK_MAGIC \
    UINT64_C(0x5649545445424C4B)
#endif

#ifndef VITTE_ARENA_BLOCK_DEAD_MAGIC
#define VITTE_ARENA_BLOCK_DEAD_MAGIC \
    UINT64_C(0x44454144424C4F43)
#endif

#ifndef VITTE_ARENA_BLOCK_MIN_CAPACITY
#define VITTE_ARENA_BLOCK_MIN_CAPACITY \
    ((size_t)1024u)
#endif

#ifndef VITTE_ARENA_BLOCK_DEFAULT_CAPACITY
#define VITTE_ARENA_BLOCK_DEFAULT_CAPACITY \
    ((size_t)64u * (size_t)1024u)
#endif

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
/* Arithmetic                                                                */
/* ========================================================================= */

static bool
vitte_arena_block_add_overflow(
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
vitte_arena_block_uintptr_add_overflow(
    uintptr_t left,
    uintptr_t right,
    uintptr_t *result)
{
    if (result == NULL) {
        return true;
    }

    if (left > UINTPTR_MAX - right) {
        return true;
    }

    *result = left + right;
    return false;
}

static bool
vitte_arena_block_is_power_of_two(
    size_t value)
{
    return
        value != 0u &&
        (value & (value - 1u)) == 0u;
}

static size_t
vitte_arena_block_normalize_alignment(
    size_t alignment)
{
    if (alignment == 0u) {
        return _Alignof(max_align_t);
    }

    return alignment;
}

/*
 * Align an integer representation of an address upward.
 *
 * This helper performs only integer arithmetic. It does not dereference the
 * resulting address.
 */
static bool
vitte_arena_block_align_address(
    uintptr_t address,
    size_t alignment,
    uintptr_t *result)
{
    uintptr_t mask;
    uintptr_t aligned;

    if (result == NULL) {
        return false;
    }

    if (!vitte_arena_block_is_power_of_two(
            alignment)) {
        return false;
    }

    /*
     * size_t and uintptr_t are not required by C to have identical widths.
     * Reject an alignment that cannot be represented by uintptr_t.
     */
    if (alignment - 1u >
        (size_t)UINTPTR_MAX) {
        return false;
    }

    mask =
        (uintptr_t)(alignment - 1u);

    if (vitte_arena_block_uintptr_add_overflow(
            address,
            mask,
            &aligned)) {
        return false;
    }

    aligned &= ~mask;

    if (aligned < address) {
        return false;
    }

    *result = aligned;
    return true;
}

/*
 * Compute an allocation position without modifying the block.
 */
static bool
vitte_arena_block_compute_allocation(
    const vitte_arena_block_t *block,
    size_t size,
    size_t alignment,
    size_t *offset,
    size_t *end,
    uintptr_t *address)
{
    uintptr_t base;
    uintptr_t current;
    uintptr_t aligned;
    uintptr_t difference;
    size_t physical_size;
    size_t local_offset;
    size_t local_end;

    if (block == NULL) {
        return false;
    }

    if (offset == NULL ||
        end == NULL ||
        address == NULL) {
        return false;
    }

    alignment =
        vitte_arena_block_normalize_alignment(
            alignment);

    if (!vitte_arena_block_is_power_of_two(
            alignment)) {
        return false;
    }

    physical_size =
        size == 0u
            ? (size_t)1u
            : size;

    base =
        (uintptr_t)(const void *)block->data;

    if ((uintmax_t)block->used >
        (uintmax_t)UINTPTR_MAX) {
        return false;
    }

    if (vitte_arena_block_uintptr_add_overflow(
            base,
            (uintptr_t)block->used,
            &current)) {
        return false;
    }

    if (!vitte_arena_block_align_address(
            current,
            alignment,
            &aligned)) {
        return false;
    }

    if (aligned < base) {
        return false;
    }

    difference =
        aligned - base;

    if ((uintmax_t)difference >
        (uintmax_t)SIZE_MAX) {
        return false;
    }

    local_offset =
        (size_t)difference;

    if (local_offset >
        block->capacity) {
        return false;
    }

    if (vitte_arena_block_add_overflow(
            local_offset,
            physical_size,
            &local_end)) {
        return false;
    }

    if (local_end >
        block->capacity) {
        return false;
    }

    *offset = local_offset;
    *end = local_end;
    *address = aligned;

    return true;
}

/* ========================================================================= */
/* Overflow-safe ratio                                                       */
/* ========================================================================= */

/*
 * Compute:
 *
 *     floor(numerator * scale / denominator)
 *
 * without overflowing size_t.
 *
 * Requirements:
 *
 *     denominator != 0
 *     numerator <= denominator
 *
 * The implementation uses quotient/remainder binary long division. scale is
 * intentionally uint32_t because current users only need basis points.
 */
static uint32_t
vitte_arena_block_scaled_ratio(
    size_t numerator,
    size_t denominator,
    uint32_t scale)
{
    size_t remainder;
    uint32_t result;
    uint32_t bit;

    if (denominator == 0u ||
        numerator == 0u ||
        scale == UINT32_C(0)) {
        return UINT32_C(0);
    }

    if (numerator >= denominator) {
        return scale;
    }

    /*
     * Evaluate:
     *
     *     numerator * scale / denominator
     *
     * by decomposing scale into powers of two.
     *
     * For each bit, calculate the corresponding fractional contribution
     * without ever forming numerator * scale.
     */
    result = UINT32_C(0);

    bit = UINT32_C(1);

    while (bit <= scale / UINT32_C(2)) {
        bit <<= 1u;
    }

    for (;;) {
        if ((scale & bit) != UINT32_C(0)) {
            size_t fraction;
            uint32_t contribution;
            uint32_t step;

            fraction = numerator;
            contribution = UINT32_C(0);
            step = UINT32_C(1);

            while (step < bit) {
                /*
                 * Compute:
                 *
                 *     fraction = (fraction * 2) % denominator
                 *
                 * and propagate the quotient bit without overflowing.
                 */
                if (fraction >=
                    denominator - fraction) {
                    fraction =
                        fraction -
                        (denominator - fraction);

                    if (contribution >
                        UINT32_MAX / UINT32_C(2)) {
                        return scale;
                    }

                    contribution =
                        contribution *
                            UINT32_C(2) +
                        UINT32_C(1);
                } else {
                    fraction += fraction;

                    if (contribution >
                        UINT32_MAX / UINT32_C(2)) {
                        return scale;
                    }

                    contribution *=
                        UINT32_C(2);
                }

                step <<= 1u;
            }

            /*
             * The loop above computes floor(numerator * bit / denominator).
             */
            if (result >
                scale - contribution) {
                return scale;
            }

            result += contribution;
        }

        if (bit == UINT32_C(1)) {
            break;
        }

        bit >>= 1u;
    }

    /*
     * The decomposition above independently floors each bit contribution,
     * which can lose fractional carry between terms. For the fixed small
     * basis-point scale, use exact repeated remainder accumulation instead.
     *
     * This loop executes at most 10000 iterations and is only used for
     * statistics/debug queries, never the allocation fast path.
     */
    result = UINT32_C(0);
    remainder = numerator;

    {
        uint32_t index;
        size_t accumulated;

        accumulated = 0u;

        for (index = UINT32_C(0);
             index < scale;
             ++index) {
            /*
             * Add numerator / denominator to the accumulated fraction.
             *
             * Because numerator < denominator, each iteration contributes
             * either zero or one to the integer quotient.
             *
             * Test:
             *
             *     accumulated + numerator >= denominator
             *
             * without overflowing.
             */
            if (accumulated >=
                denominator - remainder) {
                accumulated =
                    accumulated -
                    (denominator - remainder);

                ++result;
            } else {
                accumulated += remainder;
            }
        }
    }

    if (result > scale) {
        result = scale;
    }

    return result;
}

/* ========================================================================= */
/* Capacity                                                                  */
/* ========================================================================= */

size_t
vitte_arena_block_normalize_capacity(
    size_t capacity)
{
    if (capacity == 0u) {
        return
            VITTE_ARENA_BLOCK_DEFAULT_CAPACITY;
    }

    if (capacity <
        VITTE_ARENA_BLOCK_MIN_CAPACITY) {
        return
            VITTE_ARENA_BLOCK_MIN_CAPACITY;
    }

    return capacity;
}

bool
vitte_arena_block_required_capacity(
    size_t size,
    size_t alignment,
    size_t *result)
{
    size_t physical_size;

    if (result == NULL) {
        return false;
    }

    *result = 0u;

    alignment =
        vitte_arena_block_normalize_alignment(
            alignment);

    if (!vitte_arena_block_is_power_of_two(
            alignment)) {
        return false;
    }

    /*
     * Allocation itself normalizes zero bytes to one physical byte. Required
     * capacity must use the same semantics.
     */
    physical_size =
        size == 0u
            ? (size_t)1u
            : size;

    /*
     * Worst-case alignment padding is alignment - 1.
     */
    if (vitte_arena_block_add_overflow(
            physical_size,
            alignment - 1u,
            result)) {
        *result = 0u;
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Creation                                                                  */
/* ========================================================================= */

vitte_arena_block_t *
vitte_arena_block_create(
    size_t capacity,
    uint64_t generation,
    bool dedicated)
{
    vitte_arena_block_t *block;
    size_t normalized_capacity;
    size_t total_size;

    normalized_capacity =
        vitte_arena_block_normalize_capacity(
            capacity);

    if (normalized_capacity == 0u) {
        return NULL;
    }

    if (vitte_arena_block_add_overflow(
            sizeof(vitte_arena_block_t),
            normalized_capacity,
            &total_size)) {
        return NULL;
    }

    block =
        (vitte_arena_block_t *)
            malloc(total_size);

    if (block == NULL) {
        return NULL;
    }

    /*
     * Initialize the complete fixed header deterministically.
     */
    memset(
        block,
        0,
        sizeof(*block));

    block->magic =
        VITTE_ARENA_BLOCK_MAGIC;

    block->next = NULL;
    block->previous = NULL;

    block->capacity =
        normalized_capacity;

    block->used = 0u;

    block->generation =
        generation;

    block->dedicated =
        dedicated;

    block->allocation_count = 0u;
    block->requested_bytes = 0u;
    block->peak_used = 0u;

    /*
     * alignment_anchor exists in block.h to ensure the block header itself
     * carries max_align_t alignment. Zero it explicitly for deterministic
     * state even though the fixed header has already been cleared.
     */
    memset(
        &block->alignment_anchor,
        0,
        sizeof(block->alignment_anchor));

#if VITTE_ARENA_BLOCK_POISON_RESET
    memset(
        block->data,
        VITTE_ARENA_BLOCK_RESET_PATTERN,
        normalized_capacity);
#endif

    /*
     * Creation must never return a structurally malformed block.
     */
    if (!vitte_arena_block_validate(block)) {
        block->magic =
            VITTE_ARENA_BLOCK_DEAD_MAGIC;

        free(block);
        return NULL;
    }

    return block;
}

/* ========================================================================= */
/* Destruction                                                               */
/* ========================================================================= */

void
vitte_arena_block_destroy(
    vitte_arena_block_t *block)
{
    if (block == NULL) {
        return;
    }

    /*
     * Do not dereference topology or attempt aggressive recovery when the
     * cookie is invalid. The caller may have supplied arbitrary storage.
     */
    if (block->magic !=
        VITTE_ARENA_BLOCK_MAGIC) {
        return;
    }

#if VITTE_ARENA_BLOCK_POISON_DESTROY
    if (block->capacity != 0u) {
        memset(
            block->data,
            VITTE_ARENA_BLOCK_DESTROY_PATTERN,
            block->capacity);
    }
#endif

    /*
     * Poison the fixed metadata before free as well. This is useful under
     * allocators/debuggers that preserve freed memory for inspection.
     */
    block->magic =
        VITTE_ARENA_BLOCK_DEAD_MAGIC;

    block->next = NULL;
    block->previous = NULL;

    block->capacity = 0u;
    block->used = 0u;

    block->generation = 0u;

    block->dedicated = false;

    block->allocation_count = 0u;
    block->requested_bytes = 0u;
    block->peak_used = 0u;

    memset(
        &block->alignment_anchor,
        0,
        sizeof(block->alignment_anchor));

    free(block);
}

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

void *
vitte_arena_block_alloc(
    vitte_arena_block_t *block,
    size_t size,
    size_t alignment)
{
    size_t offset;
    size_t end;
    uintptr_t address;
    size_t physical_size;

    if (!vitte_arena_block_validate(block)) {
        return NULL;
    }

    if (!vitte_arena_block_compute_allocation(
            block,
            size,
            alignment,
            &offset,
            &end,
            &address)) {
        return NULL;
    }

    physical_size =
        size == 0u
            ? (size_t)1u
            : size;

    /*
     * No state mutation occurs before all feasibility checks have completed.
     * This keeps failed allocations transactional.
     */
    block->used = end;

    if (block->allocation_count !=
        SIZE_MAX) {
        ++block->allocation_count;
    }

    if (block->requested_bytes !=
        SIZE_MAX) {
        if (size >
            SIZE_MAX -
                block->requested_bytes) {
            block->requested_bytes =
                SIZE_MAX;
        } else {
            block->requested_bytes +=
                size;
        }
    }

    if (block->used >
        block->peak_used) {
        block->peak_used =
            block->used;
    }

#if VITTE_ARENA_BLOCK_POISON_ALLOC
    memset(
        (void *)address,
        VITTE_ARENA_BLOCK_ALLOC_PATTERN,
        physical_size);
#else
    (void)physical_size;
#endif

    /*
     * offset is deliberately calculated even though address is returned:
     * compute_allocation() verifies that the aligned address is representable
     * as a payload offset.
     */
    (void)offset;

    return (void *)address;
}

/* ========================================================================= */
/* Allocation feasibility                                                    */
/* ========================================================================= */

bool
vitte_arena_block_can_allocate(
    const vitte_arena_block_t *block,
    size_t size,
    size_t alignment)
{
    size_t offset;
    size_t end;
    uintptr_t address;

    if (!vitte_arena_block_validate(block)) {
        return false;
    }

    return
        vitte_arena_block_compute_allocation(
            block,
            size,
            alignment,
            &offset,
            &end,
            &address);
}

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

void
vitte_arena_block_reset(
    vitte_arena_block_t *block,
    uint64_t generation)
{
    if (!vitte_arena_block_validate(block)) {
        return;
    }

#if VITTE_ARENA_BLOCK_POISON_RESET
    if (block->used != 0u) {
        memset(
            block->data,
            VITTE_ARENA_BLOCK_RESET_PATTERN,
            block->used);
    }
#endif

    block->used = 0u;

    block->generation =
        generation;

    block->allocation_count = 0u;
    block->requested_bytes = 0u;

    /*
     * peak_used is a physical-block lifetime high-water mark and therefore
     * intentionally survives reset.
     */
}

/* ========================================================================= */
/* Rewind                                                                    */
/* ========================================================================= */

bool
vitte_arena_block_rewind(
    vitte_arena_block_t *block,
    size_t used)
{
    size_t old_used;

    if (!vitte_arena_block_validate(block)) {
        return false;
    }

    if (used > block->used) {
        return false;
    }

    old_used =
        block->used;

#if VITTE_ARENA_BLOCK_POISON_RESET
    if (old_used > used) {
        memset(
            block->data + used,
            VITTE_ARENA_BLOCK_RESET_PATTERN,
            old_used - used);
    }
#else
    (void)old_used;
#endif

    block->used =
        used;

    /*
     * allocation_count and requested_bytes cannot be reconstructed from a
     * byte offset without per-allocation metadata.
     *
     * allocator.c / the high-level arena checkpoint layer must restore exact
     * transactional accounting when a mark is rewound.
     *
     * peak_used intentionally survives rewind.
     */

    return true;
}

/* ========================================================================= */
/* Capacity queries                                                          */
/* ========================================================================= */

size_t
vitte_arena_block_capacity(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return 0u;
    }

    return block->capacity;
}

size_t
vitte_arena_block_used(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return 0u;
    }

    return block->used;
}

size_t
vitte_arena_block_remaining(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return 0u;
    }

    return
        block->capacity -
        block->used;
}

size_t
vitte_arena_block_requested_bytes(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return 0u;
    }

    return block->requested_bytes;
}

size_t
vitte_arena_block_allocation_count(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return 0u;
    }

    return block->allocation_count;
}

size_t
vitte_arena_block_peak_used(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return 0u;
    }

    return block->peak_used;
}

uint64_t
vitte_arena_block_generation(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return UINT64_C(0);
    }

    return block->generation;
}

/* ========================================================================= */
/* Classification                                                            */
/* ========================================================================= */

bool
vitte_arena_block_is_dedicated(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return false;
    }

    return block->dedicated;
}

bool
vitte_arena_block_is_empty(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return true;
    }

    return block->used == 0u;
}

bool
vitte_arena_block_is_full(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return false;
    }

    return
        block->used ==
        block->capacity;
}

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

bool
vitte_arena_block_contains(
    const vitte_arena_block_t *block,
    const void *pointer)
{
    uintptr_t address;
    uintptr_t begin;
    uintptr_t end;

    if (!vitte_arena_block_validate(block) ||
        pointer == NULL) {
        return false;
    }

    begin =
        (uintptr_t)(const void *)block->data;

    if ((uintmax_t)block->capacity >
        (uintmax_t)UINTPTR_MAX) {
        return false;
    }

    if (vitte_arena_block_uintptr_add_overflow(
            begin,
            (uintptr_t)block->capacity,
            &end)) {
        return false;
    }

    address =
        (uintptr_t)pointer;

    return
        address >= begin &&
        address < end;
}

bool
vitte_arena_block_contains_live(
    const vitte_arena_block_t *block,
    const void *pointer)
{
    uintptr_t address;
    uintptr_t begin;
    uintptr_t end;

    if (!vitte_arena_block_validate(block) ||
        pointer == NULL) {
        return false;
    }

    if (block->used == 0u) {
        return false;
    }

    begin =
        (uintptr_t)(const void *)block->data;

    if ((uintmax_t)block->used >
        (uintmax_t)UINTPTR_MAX) {
        return false;
    }

    if (vitte_arena_block_uintptr_add_overflow(
            begin,
            (uintptr_t)block->used,
            &end)) {
        return false;
    }

    address =
        (uintptr_t)pointer;

    return
        address >= begin &&
        address < end;
}

/* ========================================================================= */
/* Pointer offset                                                            */
/* ========================================================================= */

bool
vitte_arena_block_offset_of(
    const vitte_arena_block_t *block,
    const void *pointer,
    size_t *offset)
{
    uintptr_t address;
    uintptr_t begin;
    uintptr_t difference;

    if (offset == NULL) {
        return false;
    }

    *offset = 0u;

    if (!vitte_arena_block_contains(
            block,
            pointer)) {
        return false;
    }

    address =
        (uintptr_t)pointer;

    begin =
        (uintptr_t)(const void *)block->data;

    if (address < begin) {
        return false;
    }

    difference =
        address - begin;

    if ((uintmax_t)difference >
        (uintmax_t)SIZE_MAX) {
        return false;
    }

    *offset =
        (size_t)difference;

    return true;
}

/* ========================================================================= */
/* Offset pointer                                                            */
/* ========================================================================= */

void *
vitte_arena_block_pointer_at(
    vitte_arena_block_t *block,
    size_t offset)
{
    if (!vitte_arena_block_validate(block)) {
        return NULL;
    }

    if (offset >=
        block->capacity) {
        return NULL;
    }

    return
        (void *)(block->data + offset);
}

const void *
vitte_arena_block_pointer_at_const(
    const vitte_arena_block_t *block,
    size_t offset)
{
    if (!vitte_arena_block_validate(block)) {
        return NULL;
    }

    if (offset >=
        block->capacity) {
        return NULL;
    }

    return
        (const void *)(block->data + offset);
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_arena_block_stats_t
vitte_arena_block_stats(
    const vitte_arena_block_t *block)
{
    vitte_arena_block_stats_t stats;

    memset(
        &stats,
        0,
        sizeof(stats));

    if (!vitte_arena_block_validate(block)) {
        return stats;
    }

    stats.capacity =
        block->capacity;

    stats.used =
        block->used;

    stats.remaining =
        block->capacity -
        block->used;

    stats.requested_bytes =
        block->requested_bytes;

    stats.allocation_count =
        block->allocation_count;

    stats.peak_used =
        block->peak_used;

    stats.generation =
        block->generation;

    stats.dedicated =
        block->dedicated;

    /*
     * requested_bytes may saturate at SIZE_MAX. Once saturated, exact padding
     * cannot be reconstructed and zero is the safest non-misleading value.
     */
    if (block->requested_bytes !=
            SIZE_MAX &&
        block->used >=
            block->requested_bytes) {
        stats.padding_bytes =
            block->used -
            block->requested_bytes;
    } else {
        stats.padding_bytes = 0u;
    }

    return stats;
}

/* ========================================================================= */
/* Utilization                                                               */
/* ========================================================================= */

uint32_t
vitte_arena_block_utilization_basis_points(
    const vitte_arena_block_t *block)
{
    if (!vitte_arena_block_validate(block)) {
        return UINT32_C(0);
    }

    if (block->capacity == 0u) {
        return UINT32_C(0);
    }

    if (block->used >=
        block->capacity) {
        return UINT32_C(10000);
    }

    return
        vitte_arena_block_scaled_ratio(
            block->used,
            block->capacity,
            UINT32_C(10000));
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_arena_block_validate(
    const vitte_arena_block_t *block)
{
    uintptr_t begin;
    uintptr_t end;
    size_t alignment;

    if (block == NULL) {
        return false;
    }

    if (block->magic !=
        VITTE_ARENA_BLOCK_MAGIC) {
        return false;
    }

    if (block->capacity == 0u) {
        return false;
    }

    if (block->capacity <
        VITTE_ARENA_BLOCK_MIN_CAPACITY) {
        return false;
    }

    if (block->used >
        block->capacity) {
        return false;
    }

    if (block->peak_used >
        block->capacity) {
        return false;
    }

    if (block->peak_used <
        block->used) {
        return false;
    }

    /*
     * For exact, non-saturated accounting, logical requested bytes cannot
     * exceed physical payload bytes consumed.
     *
     * Alignment padding and physical normalization of zero-byte requests mean
     * used may legitimately be greater than requested_bytes.
     */
    if (block->requested_bytes !=
            SIZE_MAX &&
        block->requested_bytes >
            block->used) {
        return false;
    }

    /*
     * No successful allocation may exist in a physically empty block.
     */
    if (block->used == 0u &&
        block->allocation_count != 0u) {
        return false;
    }

    /*
     * Conversely, used payload implies at least one successful allocation.
     */
    if (block->used != 0u &&
        block->allocation_count == 0u) {
        return false;
    }

    begin =
        (uintptr_t)(const void *)block->data;

    alignment =
        _Alignof(max_align_t);

    if (alignment == 0u ||
        !vitte_arena_block_is_power_of_two(
            alignment)) {
        return false;
    }

    /*
     * The flexible payload must satisfy max_align_t because alignment == 0
     * allocations use max_align_t by default.
     */
    if ((begin &
         (uintptr_t)(alignment - 1u)) !=
        (uintptr_t)0u) {
        return false;
    }

    /*
     * Ensure the complete payload address interval is representable.
     */
    if ((uintmax_t)block->capacity >
        (uintmax_t)UINTPTR_MAX) {
        return false;
    }

    if (vitte_arena_block_uintptr_add_overflow(
            begin,
            (uintptr_t)block->capacity,
            &end)) {
        return false;
    }

    if (end <= begin) {
        return false;
    }

    /*
     * Immediate topology checks only.
     *
     * allocator.c remains responsible for validating the complete list,
     * detecting longer cycles, verifying first/last/current membership and
     * reconciling arena-wide counters.
     */
    if (block->next == block ||
        block->previous == block) {
        return false;
    }

    if (block->next != NULL) {
        if (block->next->magic !=
            VITTE_ARENA_BLOCK_MAGIC) {
            return false;
        }

        if (block->next->previous !=
            block) {
            return false;
        }
    }

    if (block->previous != NULL) {
        if (block->previous->magic !=
            VITTE_ARENA_BLOCK_MAGIC) {
            return false;
        }

        if (block->previous->next !=
            block) {
            return false;
        }
    }

    return true;
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
    "max_align_t must provide a valid alignment");

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */