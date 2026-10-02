/*
 * Vitte Compiler
 * src/arena/pool.c
 *
 * Fixed-size object pool allocator.
 *
 * Design goals:
 *
 *   - O(1) allocation in the common case
 *   - O(1) release
 *   - fixed object size/alignment
 *   - paged growth
 *   - intrusive free-list
 *   - allocation bitmap
 *   - reliable double-free detection
 *   - foreign-pointer rejection
 *   - overflow-safe arithmetic
 *   - configurable memory/object limits
 *   - deterministic reset
 *   - optional page trimming
 *   - optional memory poisoning
 *   - detailed statistics
 *   - deep structural validation
 *
 * Unlike the monotonic arena allocator, pool objects may be released
 * individually and reused.
 */

#include "pool.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_POOL_MAGIC
#define VITTE_POOL_MAGIC UINT64_C(0x5649545445504F4F)
#endif

#ifndef VITTE_POOL_DEAD_MAGIC
#define VITTE_POOL_DEAD_MAGIC UINT64_C(0x44454144504F4F4C)
#endif

#ifndef VITTE_POOL_PAGE_MAGIC
#define VITTE_POOL_PAGE_MAGIC UINT64_C(0x5649545445504147)
#endif

#ifndef VITTE_POOL_PAGE_DEAD_MAGIC
#define VITTE_POOL_PAGE_DEAD_MAGIC UINT64_C(0x4445414450414745)
#endif

#ifndef VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE
#define VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE ((size_t)64u)
#endif

#ifndef VITTE_POOL_MIN_OBJECTS_PER_PAGE
#define VITTE_POOL_MIN_OBJECTS_PER_PAGE ((size_t)8u)
#endif

#ifndef VITTE_POOL_MAX_OBJECTS_PER_PAGE
#define VITTE_POOL_MAX_OBJECTS_PER_PAGE ((size_t)65536u)
#endif

#ifndef VITTE_POOL_DEFAULT_MAX_OBJECTS
#define VITTE_POOL_DEFAULT_MAX_OBJECTS SIZE_MAX
#endif

#ifndef VITTE_POOL_DEFAULT_MAX_RESERVED_BYTES
#define VITTE_POOL_DEFAULT_MAX_RESERVED_BYTES SIZE_MAX
#endif

#ifndef VITTE_POOL_GROWTH_FACTOR
#define VITTE_POOL_GROWTH_FACTOR ((size_t)2u)
#endif

#ifndef VITTE_POOL_POISON_ALLOC
#define VITTE_POOL_POISON_ALLOC 0
#endif

#ifndef VITTE_POOL_POISON_FREE
#define VITTE_POOL_POISON_FREE 0
#endif

#ifndef VITTE_POOL_POISON_RESET
#define VITTE_POOL_POISON_RESET 0
#endif

#ifndef VITTE_POOL_POISON_DESTROY
#define VITTE_POOL_POISON_DESTROY 0
#endif

#ifndef VITTE_POOL_ALLOC_PATTERN
#define VITTE_POOL_ALLOC_PATTERN 0xCD
#endif

#ifndef VITTE_POOL_FREE_PATTERN
#define VITTE_POOL_FREE_PATTERN 0xDD
#endif

#ifndef VITTE_POOL_RESET_PATTERN
#define VITTE_POOL_RESET_PATTERN 0xDE
#endif

#ifndef VITTE_POOL_DESTROY_PATTERN
#define VITTE_POOL_DESTROY_PATTERN 0xFD
#endif

/* ========================================================================= */
/* Internal structures                                                       */
/* ========================================================================= */

/*
 * Free-list node.
 *
 * The beginning of every free slot is interpreted as this structure.
 */
typedef struct vitte_pool_free_node {
    struct vitte_pool_free_node *next;
} vitte_pool_free_node_t;

/*
 * Physical pool page.
 *
 * Layout:
 *
 *     [ page header ]
 *     [ bitmap ]
 *     [ alignment padding ]
 *     [ slot 0 ]
 *     [ slot 1 ]
 *     ...
 *
 * A single malloc() owns the complete page.
 */
struct vitte_pool_page {
    uint64_t magic;

    vitte_pool_page_t *next;
    vitte_pool_page_t *previous;

    size_t slot_count;
    size_t live_count;

    size_t bitmap_size;
    size_t slots_offset;

    size_t allocation_size;

    uint64_t generation;
};

/* ========================================================================= */
/* Arithmetic                                                                */
/* ========================================================================= */

static bool
vitte_pool_add_overflow(
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
vitte_pool_mul_overflow(
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
vitte_pool_is_power_of_two(
    size_t value)
{
    return
        value != 0u &&
        (value & (value - 1u)) == 0u;
}

static bool
vitte_pool_align_up(
    size_t value,
    size_t alignment,
    size_t *result)
{
    size_t mask;
    size_t temporary;

    if (result == NULL ||
        !vitte_pool_is_power_of_two(alignment)) {
        return false;
    }

    mask = alignment - 1u;

    if (vitte_pool_add_overflow(
            value,
            mask,
            &temporary)) {
        return false;
    }

    *result = temporary & ~mask;

    return true;
}

/* ========================================================================= */
/* Pointer arithmetic                                                        */
/* ========================================================================= */

static unsigned char *
vitte_pool_page_bytes(
    vitte_pool_page_t *page)
{
    return (unsigned char *)page;
}

static const unsigned char *
vitte_pool_page_bytes_const(
    const vitte_pool_page_t *page)
{
    return (const unsigned char *)page;
}

static unsigned char *
vitte_pool_page_bitmap(
    vitte_pool_page_t *page)
{
    return
        vitte_pool_page_bytes(page) +
        sizeof(vitte_pool_page_t);
}

static const unsigned char *
vitte_pool_page_bitmap_const(
    const vitte_pool_page_t *page)
{
    return
        vitte_pool_page_bytes_const(page) +
        sizeof(vitte_pool_page_t);
}

static unsigned char *
vitte_pool_page_slots(
    vitte_pool_page_t *page)
{
    return
        vitte_pool_page_bytes(page) +
        page->slots_offset;
}

static const unsigned char *
vitte_pool_page_slots_const(
    const vitte_pool_page_t *page)
{
    return
        vitte_pool_page_bytes_const(page) +
        page->slots_offset;
}

/* ========================================================================= */
/* Bitmap                                                                    */
/* ========================================================================= */

static size_t
vitte_pool_bitmap_size(
    size_t slot_count)
{
    size_t value;

    if (slot_count == 0u) {
        return 0u;
    }

    if (slot_count >
        SIZE_MAX - 7u) {
        return 0u;
    }

    value = slot_count + 7u;

    return value / 8u;
}

static bool
vitte_pool_bitmap_get(
    const vitte_pool_page_t *page,
    size_t index)
{
    const unsigned char *bitmap;
    unsigned char mask;

    if (page == NULL ||
        index >= page->slot_count) {
        return false;
    }

    bitmap =
        vitte_pool_page_bitmap_const(page);

    mask =
        (unsigned char)(
            1u << (index & 7u));

    return
        (bitmap[index >> 3u] & mask) != 0u;
}

static void
vitte_pool_bitmap_set(
    vitte_pool_page_t *page,
    size_t index)
{
    unsigned char *bitmap;
    unsigned char mask;

    if (page == NULL ||
        index >= page->slot_count) {
        return;
    }

    bitmap =
        vitte_pool_page_bitmap(page);

    mask =
        (unsigned char)(
            1u << (index & 7u));

    bitmap[index >> 3u] |= mask;
}

static void
vitte_pool_bitmap_clear(
    vitte_pool_page_t *page,
    size_t index)
{
    unsigned char *bitmap;
    unsigned char mask;

    if (page == NULL ||
        index >= page->slot_count) {
        return;
    }

    bitmap =
        vitte_pool_page_bitmap(page);

    mask =
        (unsigned char)(
            1u << (index & 7u));

    bitmap[index >> 3u] &=
        (unsigned char)~mask;
}

static void
vitte_pool_bitmap_clear_all(
    vitte_pool_page_t *page)
{
    if (page == NULL ||
        page->bitmap_size == 0u) {
        return;
    }

    memset(
        vitte_pool_page_bitmap(page),
        0,
        page->bitmap_size);
}

static size_t
vitte_pool_bitmap_count_live(
    const vitte_pool_page_t *page)
{
    size_t index;
    size_t count;

    if (page == NULL) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < page->slot_count;
         ++index) {
        if (vitte_pool_bitmap_get(
                page,
                index)) {
            ++count;
        }
    }

    return count;
}

/* ========================================================================= */
/* Pool validity                                                             */
/* ========================================================================= */

static bool
vitte_pool_magic_valid(
    const vitte_pool_t *pool)
{
    return
        pool != NULL &&
        pool->magic == VITTE_POOL_MAGIC;
}

static bool
vitte_pool_page_magic_valid(
    const vitte_pool_page_t *page)
{
    return
        page != NULL &&
        page->magic == VITTE_POOL_PAGE_MAGIC;
}

/* ========================================================================= */
/* Error handling                                                            */
/* ========================================================================= */

static void
vitte_pool_set_error(
    vitte_pool_t *pool,
    vitte_pool_error_t error)
{
    if (!vitte_pool_magic_valid(pool)) {
        return;
    }

    pool->last_error = error;
}

vitte_pool_error_t
vitte_pool_last_error(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return VITTE_POOL_ERROR_INVALID_POOL;
    }

    return pool->last_error;
}

void
vitte_pool_clear_error(
    vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return;
    }

    pool->last_error =
        VITTE_POOL_ERROR_NONE;
}

const char *
vitte_pool_error_name(
    vitte_pool_error_t error)
{
    switch (error) {
        case VITTE_POOL_ERROR_NONE:
            return "none";

        case VITTE_POOL_ERROR_INVALID_POOL:
            return "invalid-pool";

        case VITTE_POOL_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_POOL_ERROR_INVALID_ALIGNMENT:
            return "invalid-alignment";

        case VITTE_POOL_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_POOL_ERROR_OUT_OF_MEMORY:
            return "out-of-memory";

        case VITTE_POOL_ERROR_LIMIT_EXCEEDED:
            return "limit-exceeded";

        case VITTE_POOL_ERROR_FOREIGN_POINTER:
            return "foreign-pointer";

        case VITTE_POOL_ERROR_MISALIGNED_POINTER:
            return "misaligned-pointer";

        case VITTE_POOL_ERROR_DOUBLE_FREE:
            return "double-free";

        case VITTE_POOL_ERROR_CORRUPTION:
            return "corruption";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_pool_config_t
vitte_pool_config_default(void)
{
    vitte_pool_config_t config;

    memset(&config, 0, sizeof(config));

    config.objects_per_page =
        VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE;

    config.max_objects =
        VITTE_POOL_DEFAULT_MAX_OBJECTS;

    config.max_reserved_bytes =
        VITTE_POOL_DEFAULT_MAX_RESERVED_BYTES;

    config.geometric_growth = true;
    config.retain_empty_pages = true;
    config.zero_on_allocate = false;

    return config;
}

bool
vitte_pool_config_validate(
    const vitte_pool_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->objects_per_page == 0u) {
        return false;
    }

    if (config->objects_per_page >
        VITTE_POOL_MAX_OBJECTS_PER_PAGE) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Slot size                                                                 */
/* ========================================================================= */

static bool
vitte_pool_compute_slot_size(
    size_t object_size,
    size_t alignment,
    size_t *slot_size)
{
    size_t minimum;

    if (slot_size == NULL ||
        object_size == 0u ||
        !vitte_pool_is_power_of_two(alignment)) {
        return false;
    }

    minimum = object_size;

    if (minimum <
        sizeof(vitte_pool_free_node_t)) {
        minimum =
            sizeof(vitte_pool_free_node_t);
    }

    return
        vitte_pool_align_up(
            minimum,
            alignment,
            slot_size);
}

/* ========================================================================= */
/* Page layout                                                               */
/* ========================================================================= */

static bool
vitte_pool_compute_page_layout(
    const vitte_pool_t *pool,
    size_t slot_count,
    size_t *bitmap_size,
    size_t *slots_offset,
    size_t *allocation_size)
{
    size_t bitmap;
    size_t offset;
    size_t slots_bytes;
    size_t total;

    if (pool == NULL ||
        bitmap_size == NULL ||
        slots_offset == NULL ||
        allocation_size == NULL ||
        slot_count == 0u) {
        return false;
    }

    bitmap =
        vitte_pool_bitmap_size(
            slot_count);

    if (bitmap == 0u) {
        return false;
    }

    if (vitte_pool_add_overflow(
            sizeof(vitte_pool_page_t),
            bitmap,
            &offset)) {
        return false;
    }

    if (!vitte_pool_align_up(
            offset,
            pool->alignment,
            &offset)) {
        return false;
    }

    if (vitte_pool_mul_overflow(
            slot_count,
            pool->slot_size,
            &slots_bytes)) {
        return false;
    }

    if (vitte_pool_add_overflow(
            offset,
            slots_bytes,
            &total)) {
        return false;
    }

    *bitmap_size = bitmap;
    *slots_offset = offset;
    *allocation_size = total;

    return true;
}

/* ========================================================================= */
/* Page ownership                                                            */
/* ========================================================================= */

static bool
vitte_pool_page_contains(
    const vitte_pool_t *pool,
    const vitte_pool_page_t *page,
    const void *pointer)
{
    const unsigned char *slots;
    uintptr_t address;
    uintptr_t begin;
    uintptr_t end;
    size_t slots_bytes;

    if (pool == NULL ||
        !vitte_pool_page_magic_valid(page) ||
        pointer == NULL) {
        return false;
    }

    if (vitte_pool_mul_overflow(
            page->slot_count,
            pool->slot_size,
            &slots_bytes)) {
        return false;
    }

    slots =
        vitte_pool_page_slots_const(page);
    begin = (uintptr_t)slots;

    if ((uintptr_t)slots_bytes >
        UINTPTR_MAX - begin) {
        return false;
    }

    end =
        begin +
        (uintptr_t)slots_bytes;

    address =
        (uintptr_t)pointer;

    return
        address >= begin &&
        address < end;
}

static bool
vitte_pool_page_index_of(
    const vitte_pool_t *pool,
    const vitte_pool_page_t *page,
    const void *pointer,
    size_t *index)
{
    const unsigned char *slots;
    uintptr_t address;
    uintptr_t begin;
    uintptr_t difference;
    size_t offset;

    if (index == NULL) {
        return false;
    }

    *index = 0u;

    if (!vitte_pool_page_contains(
            pool,
            page,
            pointer)) {
        return false;
    }

    address =
        (uintptr_t)pointer;

    slots =
        vitte_pool_page_slots_const(page);
    begin = (uintptr_t)slots;

    difference =
        address - begin;

    if (difference >
        (uintptr_t)SIZE_MAX) {
        return false;
    }

    offset = (size_t)difference;

    if ((offset % pool->slot_size) != 0u) {
        return false;
    }

    *index =
        offset / pool->slot_size;

    return
        *index < page->slot_count;
}

static unsigned char *
vitte_pool_page_slot(
    const vitte_pool_t *pool,
    vitte_pool_page_t *page,
    size_t index)
{
    size_t offset;

    if (pool == NULL ||
        page == NULL ||
        index >= page->slot_count) {
        return NULL;
    }

    if (vitte_pool_mul_overflow(
            index,
            pool->slot_size,
            &offset)) {
        return NULL;
    }

    return
        vitte_pool_page_slots(page) +
        offset;
}

/* ========================================================================= */
/* Find page                                                                 */
/* ========================================================================= */

static vitte_pool_page_t *
vitte_pool_find_page(
    const vitte_pool_t *pool,
    const void *pointer)
{
    vitte_pool_page_t *page;

    if (!vitte_pool_magic_valid(pool) ||
        pointer == NULL) {
        return NULL;
    }

    page = pool->first_page;

    while (page != NULL) {
        if (vitte_pool_page_contains(
                pool,
                page,
                pointer)) {
            return page;
        }

        page = page->next;
    }

    return NULL;
}

/* ========================================================================= */
/* Page linking                                                              */
/* ========================================================================= */

static void
vitte_pool_append_page(
    vitte_pool_t *pool,
    vitte_pool_page_t *page)
{
    page->previous =
        pool->last_page;

    page->next = NULL;

    if (pool->last_page != NULL) {
        pool->last_page->next = page;
    } else {
        pool->first_page = page;
    }

    pool->last_page = page;

    ++pool->page_count;

    if (pool->page_count >
        pool->peak_page_count) {
        pool->peak_page_count =
            pool->page_count;
    }
}

static void
vitte_pool_unlink_page(
    vitte_pool_t *pool,
    vitte_pool_page_t *page)
{
    if (page->previous != NULL) {
        page->previous->next =
            page->next;
    } else {
        pool->first_page =
            page->next;
    }

    if (page->next != NULL) {
        page->next->previous =
            page->previous;
    } else {
        pool->last_page =
            page->previous;
    }

    page->next = NULL;
    page->previous = NULL;

    if (pool->page_count != 0u) {
        --pool->page_count;
    }
}

/* ========================================================================= */
/* Free-list                                                                 */
/* ========================================================================= */

static void
vitte_pool_push_free(
    vitte_pool_t *pool,
    void *slot)
{
    vitte_pool_free_node_t *node;

    node =
        (vitte_pool_free_node_t *)slot;

    node->next =
        pool->free_list;

    pool->free_list = node;
}

static void *
vitte_pool_pop_free(
    vitte_pool_t *pool)
{
    vitte_pool_free_node_t *node;

    node = pool->free_list;

    if (node == NULL) {
        return NULL;
    }

    pool->free_list =
        node->next;

    node->next = NULL;

    return node;
}

/* ========================================================================= */
/* Rebuild free-list                                                         */
/* ========================================================================= */

static bool
vitte_pool_rebuild_free_list(
    vitte_pool_t *pool)
{
    vitte_pool_page_t *page;
    size_t index;

    if (!vitte_pool_magic_valid(pool)) {
        return false;
    }

    pool->free_list = NULL;
    pool->free_count = 0u;

    page = pool->last_page;

    while (page != NULL) {
        if (!vitte_pool_page_magic_valid(
                page)) {
            vitte_pool_set_error(
                pool,
                VITTE_POOL_ERROR_CORRUPTION);

            return false;
        }

        index = page->slot_count;

        while (index != 0u) {
            void *slot;

            --index;

            if (vitte_pool_bitmap_get(
                    page,
                    index)) {
                continue;
            }

            slot =
                vitte_pool_page_slot(
                    pool,
                    page,
                    index);

            if (slot == NULL) {
                vitte_pool_set_error(
                    pool,
                    VITTE_POOL_ERROR_CORRUPTION);

                return false;
            }

            vitte_pool_push_free(
                pool,
                slot);

            if (pool->free_count != SIZE_MAX) {
                ++pool->free_count;
            }
        }

        page = page->previous;
    }

    return true;
}

/* ========================================================================= */
/* Page creation                                                             */
/* ========================================================================= */

static vitte_pool_page_t *
vitte_pool_create_page(
    vitte_pool_t *pool,
    size_t slot_count)
{
    vitte_pool_page_t *page;

    size_t bitmap_size;
    size_t slots_offset;
    size_t allocation_size;

    size_t new_capacity;
    size_t new_reserved;

    size_t index;

    if (!vitte_pool_magic_valid(pool) ||
        slot_count == 0u) {
        return NULL;
    }

    if (slot_count >
        VITTE_POOL_MAX_OBJECTS_PER_PAGE) {
        slot_count =
            VITTE_POOL_MAX_OBJECTS_PER_PAGE;
    }

    if (pool->config.max_objects != SIZE_MAX) {
        if (pool->capacity >
            pool->config.max_objects) {
            vitte_pool_set_error(
                pool,
                VITTE_POOL_ERROR_CORRUPTION);

            return NULL;
        }

        if (slot_count >
            pool->config.max_objects -
            pool->capacity) {
            slot_count =
                pool->config.max_objects -
                pool->capacity;
        }

        if (slot_count == 0u) {
            vitte_pool_set_error(
                pool,
                VITTE_POOL_ERROR_LIMIT_EXCEEDED);

            return NULL;
        }
    }

    if (!vitte_pool_compute_page_layout(
            pool,
            slot_count,
            &bitmap_size,
            &slots_offset,
            &allocation_size)) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_OVERFLOW);

        return NULL;
    }

    if (vitte_pool_add_overflow(
            pool->capacity,
            slot_count,
            &new_capacity)) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_OVERFLOW);

        return NULL;
    }

    if (vitte_pool_add_overflow(
            pool->reserved_bytes,
            allocation_size,
            &new_reserved)) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_OVERFLOW);

        return NULL;
    }

    if (pool->config.max_reserved_bytes !=
            SIZE_MAX &&
        new_reserved >
            pool->config.max_reserved_bytes) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_LIMIT_EXCEEDED);

        return NULL;
    }

    page =
        (vitte_pool_page_t *)
            malloc(allocation_size);

    if (page == NULL) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_OUT_OF_MEMORY);

        return NULL;
    }

    memset(
        page,
        0,
        sizeof(*page));

    page->magic =
        VITTE_POOL_PAGE_MAGIC;

    page->next = NULL;
    page->previous = NULL;

    page->slot_count =
        slot_count;

    page->live_count = 0u;

    page->bitmap_size =
        bitmap_size;

    page->slots_offset =
        slots_offset;

    page->allocation_size =
        allocation_size;

    page->generation =
        pool->generation;

    vitte_pool_bitmap_clear_all(page);

#if VITTE_POOL_POISON_RESET
    {
        size_t slots_bytes;

        if (!vitte_pool_mul_overflow(
                slot_count,
                pool->slot_size,
                &slots_bytes)) {
            memset(
                vitte_pool_page_slots(page),
                VITTE_POOL_RESET_PATTERN,
                slots_bytes);
        }
    }
#endif

    vitte_pool_append_page(
        pool,
        page);

    pool->capacity =
        new_capacity;

    pool->reserved_bytes =
        new_reserved;

    if (pool->reserved_bytes >
        pool->peak_reserved_bytes) {
        pool->peak_reserved_bytes =
            pool->reserved_bytes;
    }

    /*
     * Add all page slots to the free-list.
     *
     * Reverse order keeps the first physical slot at the head.
     */
    index = slot_count;

    while (index != 0u) {
        void *slot;

        --index;

        slot =
            vitte_pool_page_slot(
                pool,
                page,
                index);

        if (slot == NULL) {
            vitte_pool_set_error(
                pool,
                VITTE_POOL_ERROR_CORRUPTION);

            /*
             * Page is already linked. Rebuild the list to leave the pool in
             * a deterministic state; deep validation will expose any further
             * invariant failure.
             */
            (void)vitte_pool_rebuild_free_list(
                pool);

            return page;
        }

        vitte_pool_push_free(
            pool,
            slot);

        if (pool->free_count != SIZE_MAX) {
            ++pool->free_count;
        }
    }

    return page;
}

/* ========================================================================= */
/* Page destruction                                                          */
/* ========================================================================= */

static void
vitte_pool_destroy_page_storage(
    vitte_pool_page_t *page)
{
    if (!vitte_pool_page_magic_valid(page)) {
        return;
    }

#if VITTE_POOL_POISON_DESTROY
    if (page->allocation_size >
        sizeof(vitte_pool_page_t)) {
        memset(
            vitte_pool_page_bytes(page) +
                sizeof(vitte_pool_page_t),
            VITTE_POOL_DESTROY_PATTERN,
            page->allocation_size -
                sizeof(vitte_pool_page_t));
    }
#endif

    page->magic =
        VITTE_POOL_PAGE_DEAD_MAGIC;

    free(page);
}

/* ========================================================================= */
/* Growth                                                                    */
/* ========================================================================= */

static size_t
vitte_pool_next_page_size(
    const vitte_pool_t *pool)
{
    size_t result;

    if (pool->page_count == 0u) {
        result =
            pool->config.objects_per_page;
    } else if (!pool->config.geometric_growth) {
        result =
            pool->config.objects_per_page;
    } else {
        size_t previous;

        if (pool->last_page == NULL) {
            return
                pool->config.objects_per_page;
        }

        previous =
            pool->last_page->slot_count;

        if (vitte_pool_mul_overflow(
                previous,
                VITTE_POOL_GROWTH_FACTOR,
                &result)) {
            result =
                VITTE_POOL_MAX_OBJECTS_PER_PAGE;
        }
    }

    if (result <
        VITTE_POOL_MIN_OBJECTS_PER_PAGE) {
        result =
            VITTE_POOL_MIN_OBJECTS_PER_PAGE;
    }

    if (result >
        VITTE_POOL_MAX_OBJECTS_PER_PAGE) {
        result =
            VITTE_POOL_MAX_OBJECTS_PER_PAGE;
    }

    if (pool->config.max_objects != SIZE_MAX) {
        size_t remaining;

        if (pool->capacity >=
            pool->config.max_objects) {
            return 0u;
        }

        remaining =
            pool->config.max_objects -
            pool->capacity;

        if (result > remaining) {
            result = remaining;
        }
    }

    return result;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_pool_init(
    vitte_pool_t *pool,
    size_t object_size,
    size_t alignment,
    const vitte_pool_config_t *config)
{
    vitte_pool_config_t effective;
    size_t slot_size;

    if (pool == NULL ||
        object_size == 0u) {
        return false;
    }

    memset(pool, 0, sizeof(*pool));

    if (alignment == 0u) {
        alignment =
            _Alignof(max_align_t);
    }

    if (!vitte_pool_is_power_of_two(
            alignment)) {
        return false;
    }

    /*
     * malloc() guarantees max_align_t alignment. Supporting over-aligned
     * objects would require aligned_alloc/posix_memalign or equivalent.
     */
    if (alignment >
        _Alignof(max_align_t)) {
        return false;
    }

    if (!vitte_pool_compute_slot_size(
            object_size,
            alignment,
            &slot_size)) {
        return false;
    }

    if (config == NULL) {
        effective =
            vitte_pool_config_default();
    } else {
        effective = *config;
    }

    if (effective.objects_per_page == 0u) {
        effective.objects_per_page =
            VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE;
    }

    if (effective.max_objects == 0u) {
        effective.max_objects =
            VITTE_POOL_DEFAULT_MAX_OBJECTS;
    }

    if (effective.max_reserved_bytes == 0u) {
        effective.max_reserved_bytes =
            VITTE_POOL_DEFAULT_MAX_RESERVED_BYTES;
    }

    if (!vitte_pool_config_validate(
            &effective)) {
        return false;
    }

    pool->magic =
        VITTE_POOL_MAGIC;

    pool->object_size =
        object_size;

    pool->alignment =
        alignment;

    pool->slot_size =
        slot_size;

    pool->config =
        effective;

    pool->first_page = NULL;
    pool->last_page = NULL;

    pool->free_list = NULL;

    pool->page_count = 0u;
    pool->peak_page_count = 0u;

    pool->capacity = 0u;
    pool->live_count = 0u;
    pool->peak_live_count = 0u;
    pool->free_count = 0u;

    pool->reserved_bytes = 0u;
    pool->peak_reserved_bytes = 0u;

    pool->allocation_count = 0u;
    pool->free_operation_count = 0u;

    pool->generation = UINT64_C(1);

    pool->last_error =
        VITTE_POOL_ERROR_NONE;

    return true;
}

bool
vitte_pool_init_default(
    vitte_pool_t *pool,
    size_t object_size,
    size_t alignment)
{
    vitte_pool_config_t config;

    config =
        vitte_pool_config_default();

    return
        vitte_pool_init(
            pool,
            object_size,
            alignment,
            &config);
}

void
vitte_pool_destroy(
    vitte_pool_t *pool)
{
    vitte_pool_page_t *page;
    vitte_pool_page_t *next;

    if (!vitte_pool_magic_valid(pool)) {
        return;
    }

    page = pool->first_page;

    while (page != NULL) {
        next = page->next;

        vitte_pool_destroy_page_storage(
            page);

        page = next;
    }

    pool->first_page = NULL;
    pool->last_page = NULL;

    pool->free_list = NULL;

    pool->page_count = 0u;
    pool->capacity = 0u;
    pool->live_count = 0u;
    pool->free_count = 0u;
    pool->reserved_bytes = 0u;

    pool->object_size = 0u;
    pool->slot_size = 0u;
    pool->alignment = 0u;

    pool->generation = 0u;

    pool->last_error =
        VITTE_POOL_ERROR_INVALID_POOL;

    pool->magic =
        VITTE_POOL_DEAD_MAGIC;
}

/* ========================================================================= */
/* Reserve                                                                   */
/* ========================================================================= */

bool
vitte_pool_reserve(
    vitte_pool_t *pool,
    size_t minimum_capacity)
{
    if (!vitte_pool_magic_valid(pool)) {
        return false;
    }

    pool->last_error =
        VITTE_POOL_ERROR_NONE;

    while (pool->capacity <
           minimum_capacity) {
        size_t page_size;

        page_size =
            vitte_pool_next_page_size(pool);

        if (page_size == 0u) {
            vitte_pool_set_error(
                pool,
                VITTE_POOL_ERROR_LIMIT_EXCEEDED);

            return false;
        }

        if (vitte_pool_create_page(
                pool,
                page_size) == NULL) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

void *
vitte_pool_alloc(
    vitte_pool_t *pool)
{
    void *slot;
    vitte_pool_page_t *page;
    size_t index;

    if (!vitte_pool_magic_valid(pool)) {
        return NULL;
    }

    pool->last_error =
        VITTE_POOL_ERROR_NONE;

    if (pool->free_list == NULL) {
        size_t page_size;

        page_size =
            vitte_pool_next_page_size(pool);

        if (page_size == 0u) {
            vitte_pool_set_error(
                pool,
                VITTE_POOL_ERROR_LIMIT_EXCEEDED);

            return NULL;
        }

        if (vitte_pool_create_page(
                pool,
                page_size) == NULL) {
            return NULL;
        }
    }

    slot =
        vitte_pool_pop_free(pool);

    if (slot == NULL) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_CORRUPTION);

        return NULL;
    }

    if (pool->free_count == 0u) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_CORRUPTION);

        return NULL;
    }

    --pool->free_count;

    page =
        vitte_pool_find_page(
            pool,
            slot);

    if (page == NULL) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_CORRUPTION);

        (void)vitte_pool_rebuild_free_list(
            pool);

        return NULL;
    }

    if (!vitte_pool_page_index_of(
            pool,
            page,
            slot,
            &index)) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_CORRUPTION);

        (void)vitte_pool_rebuild_free_list(
            pool);

        return NULL;
    }

    if (vitte_pool_bitmap_get(
            page,
            index)) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_CORRUPTION);

        (void)vitte_pool_rebuild_free_list(
            pool);

        return NULL;
    }

    vitte_pool_bitmap_set(
        page,
        index);

    if (page->live_count == SIZE_MAX ||
        pool->live_count == SIZE_MAX) {
        vitte_pool_bitmap_clear(
            page,
            index);

        vitte_pool_push_free(
            pool,
            slot);

        ++pool->free_count;

        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_OVERFLOW);

        return NULL;
    }

    ++page->live_count;
    ++pool->live_count;

    if (pool->live_count >
        pool->peak_live_count) {
        pool->peak_live_count =
            pool->live_count;
    }

    if (pool->allocation_count != SIZE_MAX) {
        ++pool->allocation_count;
    }

    if (pool->config.zero_on_allocate) {
        memset(
            slot,
            0,
            pool->object_size);

        if (pool->slot_size >
            pool->object_size) {
            memset(
                (unsigned char *)slot +
                    pool->object_size,
                0,
                pool->slot_size -
                    pool->object_size);
        }
    }
#if VITTE_POOL_POISON_ALLOC
    else {
        memset(
            slot,
            VITTE_POOL_ALLOC_PATTERN,
            pool->slot_size);
    }
#endif

    return slot;
}

void *
vitte_pool_calloc(
    vitte_pool_t *pool)
{
    void *object;

    object =
        vitte_pool_alloc(pool);

    if (object == NULL) {
        return NULL;
    }

    memset(
        object,
        0,
        pool->object_size);

    return object;
}

/* ========================================================================= */
/* Free                                                                      */
/* ========================================================================= */

bool
vitte_pool_free(
    vitte_pool_t *pool,
    void *pointer)
{
    vitte_pool_page_t *page;
    size_t index;

    if (!vitte_pool_magic_valid(pool)) {
        return false;
    }

    pool->last_error =
        VITTE_POOL_ERROR_NONE;

    if (pointer == NULL) {
        /*
         * Match free(NULL) semantics.
         */
        return true;
    }

    page =
        vitte_pool_find_page(
            pool,
            pointer);

    if (page == NULL) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_FOREIGN_POINTER);

        return false;
    }

    if (!vitte_pool_page_index_of(
            pool,
            page,
            pointer,
            &index)) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_MISALIGNED_POINTER);

        return false;
    }

    if (!vitte_pool_bitmap_get(
            page,
            index)) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_DOUBLE_FREE);

        return false;
    }

    if (page->live_count == 0u ||
        pool->live_count == 0u) {
        vitte_pool_set_error(
            pool,
            VITTE_POOL_ERROR_CORRUPTION);

        return false;
    }

    vitte_pool_bitmap_clear(
        page,
        index);

    --page->live_count;
    --pool->live_count;

#if VITTE_POOL_POISON_FREE
    memset(
        pointer,
        VITTE_POOL_FREE_PATTERN,
        pool->slot_size);
#endif

    vitte_pool_push_free(
        pool,
        pointer);

    if (pool->free_count != SIZE_MAX) {
        ++pool->free_count;
    }

    if (pool->free_operation_count !=
        SIZE_MAX) {
        ++pool->free_operation_count;
    }

    return true;
}

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

void
vitte_pool_reset(
    vitte_pool_t *pool)
{
    vitte_pool_page_t *page;

    if (!vitte_pool_magic_valid(pool)) {
        return;
    }

    ++pool->generation;

    if (pool->generation == 0u) {
        pool->generation = UINT64_C(1);
    }

    page = pool->first_page;

    while (page != NULL) {
#if VITTE_POOL_POISON_RESET
        size_t slots_bytes;

        if (!vitte_pool_mul_overflow(
                page->slot_count,
                pool->slot_size,
                &slots_bytes)) {
            memset(
                vitte_pool_page_slots(page),
                VITTE_POOL_RESET_PATTERN,
                slots_bytes);
        }
#endif

        vitte_pool_bitmap_clear_all(
            page);

        page->live_count = 0u;
        page->generation =
            pool->generation;

        page = page->next;
    }

    pool->live_count = 0u;

    if (!vitte_pool_rebuild_free_list(
            pool)) {
        return;
    }

    pool->last_error =
        VITTE_POOL_ERROR_NONE;
}

/* ========================================================================= */
/* Trim                                                                      */
/* ========================================================================= */

size_t
vitte_pool_trim(
    vitte_pool_t *pool)
{
    vitte_pool_page_t *page;
    vitte_pool_page_t *next;

    size_t removed;

    if (!vitte_pool_magic_valid(pool)) {
        return 0u;
    }

    removed = 0u;

    page = pool->first_page;

    while (page != NULL) {
        next = page->next;

        if (page->live_count == 0u) {
            size_t page_capacity;
            size_t page_reserved;

            /*
             * retain_empty_pages keeps one empty page available for the next
             * allocation, but additional empty pages may still be trimmed.
             */
            if (pool->config.retain_empty_pages &&
                pool->page_count <= 1u) {
                page = next;
                continue;
            }

            page_capacity =
                page->slot_count;

            page_reserved =
                page->allocation_size;

            vitte_pool_unlink_page(
                pool,
                page);

            if (pool->capacity >=
                page_capacity) {
                pool->capacity -=
                    page_capacity;
            } else {
                pool->capacity = 0u;
            }

            if (pool->reserved_bytes >=
                page_reserved) {
                pool->reserved_bytes -=
                    page_reserved;
            } else {
                pool->reserved_bytes = 0u;
            }

            vitte_pool_destroy_page_storage(
                page);

            ++removed;
        }

        page = next;
    }

    if (!vitte_pool_rebuild_free_list(
            pool)) {
        return removed;
    }

    pool->last_error =
        VITTE_POOL_ERROR_NONE;

    return removed;
}

void
vitte_pool_reset_and_trim(
    vitte_pool_t *pool)
{
    bool retain;

    if (!vitte_pool_magic_valid(pool)) {
        return;
    }

    vitte_pool_reset(pool);

    if (pool->last_error !=
        VITTE_POOL_ERROR_NONE) {
        return;
    }

    retain =
        pool->config.retain_empty_pages;

    /*
     * Explicit reset_and_trim means release all empty pages.
     */
    pool->config.retain_empty_pages =
        false;

    (void)vitte_pool_trim(pool);

    pool->config.retain_empty_pages =
        retain;
}

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

bool
vitte_pool_contains(
    const vitte_pool_t *pool,
    const void *pointer)
{
    return
        vitte_pool_find_page(
            pool,
            pointer) != NULL;
}

bool
vitte_pool_contains_object(
    const vitte_pool_t *pool,
    const void *pointer)
{
    vitte_pool_page_t *page;
    size_t index;

    if (!vitte_pool_magic_valid(pool) ||
        pointer == NULL) {
        return false;
    }

    page =
        vitte_pool_find_page(
            pool,
            pointer);

    if (page == NULL) {
        return false;
    }

    return
        vitte_pool_page_index_of(
            pool,
            page,
            pointer,
            &index);
}

bool
vitte_pool_is_live(
    const vitte_pool_t *pool,
    const void *pointer)
{
    vitte_pool_page_t *page;
    size_t index;

    if (!vitte_pool_magic_valid(pool) ||
        pointer == NULL) {
        return false;
    }

    page =
        vitte_pool_find_page(
            pool,
            pointer);

    if (page == NULL) {
        return false;
    }

    if (!vitte_pool_page_index_of(
            pool,
            page,
            pointer,
            &index)) {
        return false;
    }

    return
        vitte_pool_bitmap_get(
            page,
            index);
}

/* ========================================================================= */
/* Accessors                                                                 */
/* ========================================================================= */

bool
vitte_pool_is_valid(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_magic_valid(pool);
}

size_t
vitte_pool_object_size(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return 0u;
    }

    return pool->object_size;
}

size_t
vitte_pool_slot_size(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return 0u;
    }

    return pool->slot_size;
}

size_t
vitte_pool_alignment(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return 0u;
    }

    return pool->alignment;
}

size_t
vitte_pool_capacity(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return 0u;
    }

    return pool->capacity;
}

size_t
vitte_pool_live_count(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return 0u;
    }

    return pool->live_count;
}

size_t
vitte_pool_free_count(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return 0u;
    }

    return pool->free_count;
}

size_t
vitte_pool_page_count(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return 0u;
    }

    return pool->page_count;
}

size_t
vitte_pool_reserved_bytes(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return 0u;
    }

    return pool->reserved_bytes;
}

uint64_t
vitte_pool_generation(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_magic_valid(pool)) {
        return 0u;
    }

    return pool->generation;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_pool_stats_t
vitte_pool_stats(
    const vitte_pool_t *pool)
{
    vitte_pool_stats_t stats;

    memset(
        &stats,
        0,
        sizeof(stats));

    if (!vitte_pool_magic_valid(pool)) {
        return stats;
    }

    stats.object_size =
        pool->object_size;

    stats.slot_size =
        pool->slot_size;

    stats.alignment =
        pool->alignment;

    stats.page_count =
        pool->page_count;

    stats.peak_page_count =
        pool->peak_page_count;

    stats.capacity =
        pool->capacity;

    stats.live_count =
        pool->live_count;

    stats.peak_live_count =
        pool->peak_live_count;

    stats.free_count =
        pool->free_count;

    stats.reserved_bytes =
        pool->reserved_bytes;

    stats.peak_reserved_bytes =
        pool->peak_reserved_bytes;

    stats.allocation_count =
        pool->allocation_count;

    stats.free_operation_count =
        pool->free_operation_count;

    stats.generation =
        pool->generation;

    stats.max_objects =
        pool->config.max_objects;

    stats.max_reserved_bytes =
        pool->config.max_reserved_bytes;

    return stats;
}

/* ========================================================================= */
/* Deep validation                                                          */
/* ========================================================================= */

bool
vitte_pool_validate(
    const vitte_pool_t *pool)
{
    const vitte_pool_page_t *page;
    const vitte_pool_page_t *previous;

    const vitte_pool_free_node_t *slow;
    const vitte_pool_free_node_t *fast;
    const vitte_pool_free_node_t *node;

    size_t page_count;
    size_t capacity;
    size_t live_count;
    size_t reserved_bytes;
    size_t free_count;

    if (!vitte_pool_magic_valid(pool)) {
        return false;
    }

    if (pool->object_size == 0u ||
        pool->slot_size == 0u ||
        pool->alignment == 0u) {
        return false;
    }

    if (!vitte_pool_is_power_of_two(
            pool->alignment)) {
        return false;
    }

    if (pool->alignment >
        _Alignof(max_align_t)) {
        return false;
    }

    if (pool->slot_size <
        pool->object_size ||
        pool->slot_size <
        sizeof(vitte_pool_free_node_t)) {
        return false;
    }

    if ((pool->slot_size %
         pool->alignment) != 0u) {
        return false;
    }

    if (!vitte_pool_config_validate(
            &pool->config)) {
        return false;
    }

    if ((pool->first_page == NULL) !=
        (pool->last_page == NULL)) {
        return false;
    }

    page_count = 0u;
    capacity = 0u;
    live_count = 0u;
    reserved_bytes = 0u;

    previous = NULL;
    page = pool->first_page;

    while (page != NULL) {
        size_t expected_bitmap;
        size_t expected_offset;
        size_t expected_allocation;
        size_t bitmap_live;
        const unsigned char *slots;

        if (!vitte_pool_page_magic_valid(
                page)) {
            return false;
        }

        if (page->previous != previous) {
            return false;
        }

        if (page->next == page ||
            page->previous == page) {
            return false;
        }

        if (page->slot_count == 0u ||
            page->slot_count >
                VITTE_POOL_MAX_OBJECTS_PER_PAGE) {
            return false;
        }

        if (page->live_count >
            page->slot_count) {
            return false;
        }

        if (page->generation !=
            pool->generation) {
            return false;
        }

        if (!vitte_pool_compute_page_layout(
                pool,
                page->slot_count,
                &expected_bitmap,
                &expected_offset,
                &expected_allocation)) {
            return false;
        }

        if (page->bitmap_size !=
                expected_bitmap ||
            page->slots_offset !=
                expected_offset ||
            page->allocation_size !=
                expected_allocation) {
            return false;
        }

        slots =
            vitte_pool_page_slots_const(page);

        if (((uintptr_t)slots %
             (uintptr_t)pool->alignment) !=
            (uintptr_t)0u) {
            return false;
        }

        bitmap_live =
            vitte_pool_bitmap_count_live(
                page);

        if (bitmap_live !=
            page->live_count) {
            return false;
        }

        if (page_count == SIZE_MAX ||
            capacity >
                SIZE_MAX - page->slot_count ||
            live_count >
                SIZE_MAX - page->live_count ||
            reserved_bytes >
                SIZE_MAX -
                page->allocation_size) {
            return false;
        }

        ++page_count;

        capacity +=
            page->slot_count;

        live_count +=
            page->live_count;

        reserved_bytes +=
            page->allocation_size;

        previous = page;
        page = page->next;
    }

    if (previous !=
        pool->last_page) {
        return false;
    }

    if (page_count !=
        pool->page_count) {
        return false;
    }

    if (capacity !=
        pool->capacity) {
        return false;
    }

    if (live_count !=
        pool->live_count) {
        return false;
    }

    if (reserved_bytes !=
        pool->reserved_bytes) {
        return false;
    }

    if (pool->live_count >
        pool->capacity) {
        return false;
    }

    if (pool->free_count >
        pool->capacity) {
        return false;
    }

    if (pool->capacity -
            pool->live_count !=
        pool->free_count) {
        return false;
    }

    if (pool->peak_live_count <
        pool->live_count) {
        return false;
    }

    if (pool->peak_page_count <
        pool->page_count) {
        return false;
    }

    if (pool->peak_reserved_bytes <
        pool->reserved_bytes) {
        return false;
    }

    if (pool->config.max_objects !=
            SIZE_MAX &&
        pool->capacity >
            pool->config.max_objects) {
        return false;
    }

    if (pool->config.max_reserved_bytes !=
            SIZE_MAX &&
        pool->reserved_bytes >
            pool->config.max_reserved_bytes) {
        return false;
    }

    /*
     * Floyd cycle detection on free-list.
     */
    slow = pool->free_list;
    fast = pool->free_list;

    while (fast != NULL &&
           fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;

        if (slow == fast) {
            return false;
        }
    }

    /*
     * Validate every free-list entry.
     */
    free_count = 0u;
    node = pool->free_list;

    while (node != NULL) {
        vitte_pool_page_t *owner;
        size_t index;

        owner =
            vitte_pool_find_page(
                pool,
                node);

        if (owner == NULL) {
            return false;
        }

        if (!vitte_pool_page_index_of(
                pool,
                owner,
                node,
                &index)) {
            return false;
        }

        if (vitte_pool_bitmap_get(
                owner,
                index)) {
            return false;
        }

        if (free_count == SIZE_MAX) {
            return false;
        }

        ++free_count;

        if (free_count >
            pool->capacity) {
            return false;
        }

        node = node->next;
    }

    if (free_count !=
        pool->free_count) {
        return false;
    }

    /*
     * Bitmap says every non-live slot must occur exactly once in free-list.
     *
     * Counting alone plus cycle detection does not prove uniqueness if one
     * free slot is duplicated and another omitted. Perform the expensive
     * exhaustive check here because validate() is deliberately a debug/test
     * operation.
     */
    page = pool->first_page;

    while (page != NULL) {
        size_t index;

        for (index = 0u;
             index < page->slot_count;
             ++index) {
            const void *slot;
            const vitte_pool_free_node_t *cursor;
            size_t occurrences;

            if (vitte_pool_bitmap_get(
                    page,
                    index)) {
                continue;
            }

            slot =
                vitte_pool_page_slots_const(
                    page) +
                index * pool->slot_size;

            occurrences = 0u;
            cursor = pool->free_list;

            while (cursor != NULL) {
                if ((const void *)cursor ==
                    slot) {
                    ++occurrences;

                    if (occurrences > 1u) {
                        return false;
                    }
                }

                cursor = cursor->next;
            }

            if (occurrences != 1u) {
                return false;
            }
        }

        page = page->next;
    }

    return true;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
