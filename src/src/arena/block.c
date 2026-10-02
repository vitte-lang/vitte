/*
 * Vitte Compiler
 * src/arena/block.c
 *
 * Arena block implementation.
 *
 * Responsibilities:
 *
 *   - block creation/destruction
 *   - capacity normalization
 *   - overflow-safe size calculations
 *   - aligned bump allocation
 *   - remaining-capacity queries
 *   - reset/reuse
 *   - optional memory poisoning
 *   - block statistics
 *   - ownership queries
 *   - structural validation
 *
 * This module does not manage arena block lists. allocator.c owns the
 * allocation policy and block-chain topology.
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
vitte_arena_block_is_power_of_two(
    size_t value)
{
    return value != 0u &&
           (value & (value - 1u)) == 0u;
}

static bool
vitte_arena_block_align_address(
    uintptr_t address,
    size_t alignment,
    uintptr_t *result)
{
    uintptr_t mask;

    if (result == NULL) {
        return false;
    }

    if (!vitte_arena_block_is_power_of_two(
            alignment)) {
        return false;
    }

    mask =
        (uintptr_t)(alignment - 1u);

    if (address >
        UINTPTR_MAX - mask) {
        return false;
    }

    *result =
        (address + mask) & ~mask;

    return true;
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
    if (result == NULL) {
        return false;
    }

    if (alignment == 0u) {
        alignment =
            _Alignof(max_align_t);
    }

    if (!vitte_arena_block_is_power_of_two(
            alignment)) {
        return false;
    }

    /*
     * Worst-case padding is alignment - 1.
     */
    return !vitte_arena_block_add_overflow(
        size,
        alignment - 1u,
        result);
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
    size_t total_size;

    capacity =
        vitte_arena_block_normalize_capacity(
            capacity);

    if (vitte_arena_block_add_overflow(
            sizeof(vitte_arena_block_t),
            capacity,
            &total_size)) {
        return NULL;
    }

    block =
        (vitte_arena_block_t *)
            malloc(total_size);

    if (block == NULL) {
        return NULL;
    }

    block->magic =
        VITTE_ARENA_BLOCK_MAGIC;

    block->next = NULL;
    block->previous = NULL;

    block->capacity = capacity;
    block->used = 0u;

    block->generation = generation;

    block->dedicated = dedicated;

    block->allocation_count = 0u;
    block->requested_bytes = 0u;
    block->peak_used = 0u;

    /*
     * Keep the flexible payload naturally aligned after the header.
     * alignment_anchor is part of the header specifically to force the
     * structure's alignment to max_align_t.
     */
    memset(
        &block->alignment_anchor,
        0,
        sizeof(block->alignment_anchor));

#if VITTE_ARENA_BLOCK_POISON_RESET
    if (capacity != 0u) {
        memset(
            block->data,
            VITTE_ARENA_BLOCK_RESET_PATTERN,
            capacity);
    }
#endif

    return block;
}

/* ========================================================================= */
/* Destruction                                                               */
/* ========================================================================= */

void
vitte_arena_block_destroy(
    vitte_arena_block_t *block)
{
#if VITTE_ARENA_BLOCK_POISON_DESTROY
    size_t capacity;
#endif

    if (block == NULL) {
        return;
    }

    if (block->magic !=
        VITTE_ARENA_BLOCK_MAGIC) {
        return;
    }

#if VITTE_ARENA_BLOCK_POISON_DESTROY
    capacity = block->capacity;

    if (capacity != 0u) {
        memset(
            block->data,
            VITTE_ARENA_BLOCK_DESTROY_PATTERN,
            capacity);
    }
#endif

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
    uintptr_t base;
    uintptr_t current;
    uintptr_t aligned;

    size_t offset;
    size_t end;
    size_t requested_size;

    if (!vitte_arena_block_validate(block)) {
        return NULL;
    }

    if (alignment == 0u) {
        alignment =
            _Alignof(max_align_t);
    }

    if (!vitte_arena_block_is_power_of_two(
            alignment)) {
        return NULL;
    }

    /*
     * Preserve logical request size for statistics.
     *
     * Internally a zero-byte request consumes one byte so a successful
     * allocation can still produce an arena-owned address.
     */
    requested_size = size;

    if (size == 0u) {
        size = 1u;
    }

    base =
        (uintptr_t)block->data;

    if ((uintptr_t)block->used >
        UINTPTR_MAX - base) {
        return NULL;
    }

    current =
        base +
        (uintptr_t)block->used;

    if (!vitte_arena_block_align_address(
            current,
            alignment,
            &aligned)) {
        return NULL;
    }

    if (aligned < base) {
        return NULL;
    }

    if ((aligned - base) >
        (uintptr_t)SIZE_MAX) {
        return NULL;
    }

    offset =
        (size_t)(aligned - base);

    if (vitte_arena_block_add_overflow(
            offset,
            size,
            &end)) {
        return NULL;
    }

    if (end > block->capacity) {
        return NULL;
    }

    block->used = end;

    if (block->allocation_count !=
        SIZE_MAX) {
        ++block->allocation_count;
    }

    if (block->requested_bytes <=
        SIZE_MAX - requested_size) {
        block->requested_bytes +=
            requested_size;
    } else {
        block->requested_bytes =
            SIZE_MAX;
    }

    if (block->used >
        block->peak_used) {
        block->peak_used =
            block->used;
    }

#if VITTE_ARENA_BLOCK_POISON_ALLOC
    memset(
        (void *)aligned,
        VITTE_ARENA_BLOCK_ALLOC_PATTERN,
        size);
#endif

    return (void *)aligned;
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
    uintptr_t base;
    uintptr_t current;
    uintptr_t aligned;

    size_t offset;
    size_t end;

    if (!vitte_arena_block_validate(block)) {
        return false;
    }

    if (alignment == 0u) {
        alignment =
            _Alignof(max_align_t);
    }

    if (!vitte_arena_block_is_power_of_two(
            alignment)) {
        return false;
    }

    if (size == 0u) {
        size = 1u;
    }

    base =
        (uintptr_t)block->data;

    if ((uintptr_t)block->used >
        UINTPTR_MAX - base) {
        return false;
    }

    current =
        base +
        (uintptr_t)block->used;

    if (!vitte_arena_block_align_address(
            current,
            alignment,
            &aligned)) {
        return false;
    }

    if (aligned < base) {
        return false;
    }

    if ((aligned - base) >
        (uintptr_t)SIZE_MAX) {
        return false;
    }

    offset =
        (size_t)(aligned - base);

    if (vitte_arena_block_add_overflow(
            offset,
            size,
            &end)) {
        return false;
    }

    return end <= block->capacity;
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

    block->generation = generation;

    block->allocation_count = 0u;
    block->requested_bytes = 0u;

    /*
     * peak_used intentionally survives reset. It represents lifetime
     * high-water usage of this physical block.
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
    if (!vitte_arena_block_validate(block)) {
        return false;
    }

    if (used > block->used) {
        return false;
    }

#if VITTE_ARENA_BLOCK_POISON_RESET
    if (block->used > used) {
        memset(
            block->data + used,
            VITTE_ARENA_BLOCK_RESET_PATTERN,
            block->used - used);
    }
#endif

    block->used = used;

    /*
     * Exact requested-byte/allocation-count reconstruction is impossible
     * without per-allocation metadata. Higher-level arena checkpoints own
     * exact transactional accounting.
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
        return 0u;
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

    return block->used ==
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

    address =
        (uintptr_t)pointer;

    begin =
        (uintptr_t)block->data;

    if ((uintptr_t)block->capacity >
        UINTPTR_MAX - begin) {
        return false;
    }

    end =
        begin +
        (uintptr_t)block->capacity;

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

    address =
        (uintptr_t)pointer;

    begin =
        (uintptr_t)block->data;

    if ((uintptr_t)block->used >
        UINTPTR_MAX - begin) {
        return false;
    }

    end =
        begin +
        (uintptr_t)block->used;

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
        (uintptr_t)block->data;

    if (address < begin) {
        return false;
    }

    if (address - begin >
        (uintptr_t)SIZE_MAX) {
        return false;
    }

    *offset =
        (size_t)(address - begin);

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

    if (offset >= block->capacity) {
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

    if (offset >= block->capacity) {
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

    if (block->used >=
        block->requested_bytes) {
        stats.padding_bytes =
            block->used -
            block->requested_bytes;
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
    size_t used;
    size_t capacity;
    size_t quotient;
    size_t remainder;
    uint32_t result;

    if (!vitte_arena_block_validate(block)) {
        return 0u;
    }

    used = block->used;
    capacity = block->capacity;

    if (capacity == 0u) {
        return 0u;
    }

    if (used >= capacity) {
        return UINT32_C(10000);
    }

    /*
     * Overflow-free computation of:
     *
     *     used * 10000 / capacity
     *
     * Since used < capacity:
     *
     *     used / capacity == 0
     *
     * but the general decomposition keeps the helper mathematically explicit.
     */
    quotient =
        used / capacity;

    remainder =
        used % capacity;

    result =
        (uint32_t)(
            quotient *
            (size_t)10000u);

    /*
     * Avoid remainder * 10000 overflow by splitting the decimal scale.
     */
    {
        size_t scaled;
        size_t whole_thousands;
        size_t rest;

        whole_thousands =
            remainder /
            (capacity / 10000u +
             (capacity % 10000u != 0u
                  ? 1u
                  : 0u));

        if (whole_thousands >
            (size_t)10000u) {
            whole_thousands =
                (size_t)10000u;
        }

        /*
         * Use direct multiplication when safe for exactness.
         */
        if (remainder <=
            SIZE_MAX / (size_t)10000u) {
            scaled =
                (remainder *
                 (size_t)10000u) /
                capacity;
        } else {
            /*
             * Exact overflow-free decomposition:
             *
             * remainder * scale / capacity
             *
             * =
             *
             * (remainder / capacity) * scale
             * +
             * ((remainder % capacity) * scale) / capacity
             *
             * The first term is zero here. For the second term, reduce the
             * scale progressively.
             */
            size_t units;

            units = 0u;
            rest = remainder;

            while (rest >= capacity / 100u &&
                   units <= (size_t)9900u) {
                size_t threshold;

                threshold =
                    capacity / 100u;

                if (threshold == 0u) {
                    break;
                }

                rest -= threshold;
                units += 100u;
            }

            scaled = units;

            if (scaled >
                (size_t)10000u) {
                scaled =
                    (size_t)10000u;
            }
        }

        if (scaled >
            (size_t)10000u) {
            scaled =
                (size_t)10000u;
        }

        result += (uint32_t)scaled;
    }

    if (result >
        UINT32_C(10000)) {
        result =
            UINT32_C(10000);
    }

    return result;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_arena_block_validate(
    const vitte_arena_block_t *block)
{
    uintptr_t begin;

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
     * requested_bytes can saturate at SIZE_MAX, therefore it cannot always
     * be compared directly with used.
     */
    if (block->requested_bytes != SIZE_MAX &&
        block->requested_bytes >
            block->used) {
        /*
         * Normally requested bytes cannot exceed physical bytes consumed.
         *
         * A zero-sized allocation consumes one physical byte and contributes
         * zero requested bytes, so that case remains valid.
         */
        return false;
    }

    begin =
        (uintptr_t)block->data;

    /*
     * Flexible payload must satisfy max_align_t because allocator.c uses
     * max_align_t as its default alignment.
     */
    if ((begin %
         (uintptr_t)_Alignof(max_align_t)) !=
        (uintptr_t)0u) {
        return false;
    }

    if (block->next == block ||
        block->previous == block) {
        return false;
    }

    if (block->next != NULL &&
        block->next->previous != block) {
        return false;
    }

    if (block->previous != NULL &&
        block->previous->next != block) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
