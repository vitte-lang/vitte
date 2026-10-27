#ifndef VITTE_SRC_ARENA_POOL_H
#define VITTE_SRC_ARENA_POOL_H

/*
 * Vitte Compiler
 * src/arena/pool.h
 *
 * Fixed-size reusable object pool.
 *
 * ISO C17.
 *
 * Architecture:
 *
 *     block.c/.h
 *         physical monotonic arena blocks
 *
 *     allocator.c/.h
 *         low-level arena allocator
 *
 *     arena.c/.h
 *         compiler-facing arena context
 *
 *     checkpoint.c/.h
 *         transactional arena checkpoints
 *
 *     pool.c/.h
 *         reusable fixed-size objects
 *
 * Properties:
 *
 *     - fixed logical object size
 *     - fixed alignment
 *     - aligned physical slots
 *     - paged allocation
 *     - lazy page creation
 *     - O(1) common allocation
 *     - O(1) free-list insertion
 *     - intrusive free-list
 *     - allocation bitmap
 *     - double-free detection
 *     - foreign/interior-pointer rejection
 *     - configurable linear/geometric growth
 *     - object-capacity limit
 *     - reserved-memory limit
 *     - deterministic reset/reuse
 *     - optional page trimming
 *     - optional memory poisoning
 *     - generation tracking
 *     - lifetime/high-water statistics
 *     - deep structural validation
 *
 * Individual pool objects may be returned and reused. This intentionally
 * differs from the monotonic arena allocator.
 *
 * Thread safety:
 *
 *     Pool mutation is not internally synchronized. Concurrent access
 *     involving mutation requires external synchronization.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* C / C++ portability                                                       */
/* ========================================================================= */

#ifdef __cplusplus
#define VITTE_POOL_ALIGNOF(type) alignof(type)
#else
#define VITTE_POOL_ALIGNOF(type) _Alignof(type)
#endif

/* ========================================================================= */
/* Configuration constants                                                   */
/* ========================================================================= */

#ifndef VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE
#define VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE \
    ((size_t)64u)
#endif

#ifndef VITTE_POOL_MIN_OBJECTS_PER_PAGE
#define VITTE_POOL_MIN_OBJECTS_PER_PAGE \
    ((size_t)8u)
#endif

#ifndef VITTE_POOL_MAX_OBJECTS_PER_PAGE
#define VITTE_POOL_MAX_OBJECTS_PER_PAGE \
    ((size_t)65536u)
#endif

#ifndef VITTE_POOL_DEFAULT_MAX_OBJECTS
#define VITTE_POOL_DEFAULT_MAX_OBJECTS \
    SIZE_MAX
#endif

#ifndef VITTE_POOL_DEFAULT_MAX_RESERVED_BYTES
#define VITTE_POOL_DEFAULT_MAX_RESERVED_BYTES \
    SIZE_MAX
#endif

#ifndef VITTE_POOL_GROWTH_FACTOR
#define VITTE_POOL_GROWTH_FACTOR \
    ((size_t)2u)
#endif

/* ========================================================================= */
/* Debug poisoning                                                           */
/* ========================================================================= */

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
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_pool
    vitte_pool_t;

typedef struct vitte_pool_page
    vitte_pool_page_t;

typedef struct vitte_pool_free_node
    vitte_pool_free_node_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_pool_error {
    VITTE_POOL_ERROR_NONE = 0,

    /*
     * Pool pointer does not identify a live initialized pool.
     */
    VITTE_POOL_ERROR_INVALID_POOL,

    /*
     * Invalid non-alignment argument.
     */
    VITTE_POOL_ERROR_INVALID_ARGUMENT,

    /*
     * Requested alignment is unsupported.
     */
    VITTE_POOL_ERROR_INVALID_ALIGNMENT,

    /*
     * Integer arithmetic overflow.
     */
    VITTE_POOL_ERROR_OVERFLOW,

    /*
     * Physical memory allocation failed.
     */
    VITTE_POOL_ERROR_OUT_OF_MEMORY,

    /*
     * max_objects or max_reserved_bytes prevents growth.
     */
    VITTE_POOL_ERROR_LIMIT_EXCEEDED,

    /*
     * Pointer does not lie in the slot-storage region of this pool.
     */
    VITTE_POOL_ERROR_FOREIGN_POINTER,

    /*
     * Pointer lies inside pool storage but is not a slot beginning.
     */
    VITTE_POOL_ERROR_MISALIGNED_POINTER,

    /*
     * Slot is already free.
     */
    VITTE_POOL_ERROR_DOUBLE_FREE,

    /*
     * Internal structural/accounting invariant failure.
     */
    VITTE_POOL_ERROR_CORRUPTION
} vitte_pool_error_t;

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

typedef struct vitte_pool_config {
    /*
     * Initial/base page slot count.
     *
     * vitte_pool_init() normalizes zero to
     * VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE.
     */
    size_t objects_per_page;

    /*
     * Maximum total physical slot capacity.
     *
     * SIZE_MAX:
     *     unlimited.
     *
     * vitte_pool_init() normalizes zero to
     * VITTE_POOL_DEFAULT_MAX_OBJECTS.
     */
    size_t max_objects;

    /*
     * Maximum total bytes reserved by pages.
     *
     * Includes:
     *
     *     page headers
     *     bitmaps
     *     alignment padding
     *     physical slot storage
     *
     * SIZE_MAX:
     *     unlimited.
     *
     * vitte_pool_init() normalizes zero to
     * VITTE_POOL_DEFAULT_MAX_RESERVED_BYTES.
     */
    size_t max_reserved_bytes;

    /*
     * false:
     *
     *     page sizes remain based on objects_per_page.
     *
     * true:
     *
     *     subsequent page sizes grow by VITTE_POOL_GROWTH_FACTOR, subject to
     *     VITTE_POOL_MAX_OBJECTS_PER_PAGE and configured limits.
     */
    bool geometric_growth;

    /*
     * When trim() is called, retain one empty page if one exists.
     *
     * reset_and_trim() temporarily overrides this and releases every empty
     * page.
     */
    bool retain_empty_pages;

    /*
     * Zero the complete physical slot on ordinary allocation.
     */
    bool zero_on_allocate;
} vitte_pool_config_t;

/* ========================================================================= */
/* Pool                                                                      */
/* ========================================================================= */

/*
 * Compiler-internal representation.
 *
 * Fields are visible so compiler subsystems can embed/inspect pools without
 * an additional opaque allocation, but direct mutation is unsupported.
 */
struct vitte_pool {
    /*
     * Runtime integrity cookie.
     */
    uint64_t magic;

    /*
     * Logical caller-visible object size.
     */
    size_t object_size;

    /*
     * Physical reusable slot size.
     *
     * Invariants:
     *
     *     slot_size >= object_size
     *     slot_size >= sizeof(private free-list node)
     *     slot_size % alignment == 0
     */
    size_t slot_size;

    /*
     * Required slot alignment.
     *
     * Current implementation supports alignment <= alignof(max_align_t).
     */
    size_t alignment;

    /*
     * Normalized effective configuration.
     */
    vitte_pool_config_t config;

    /*
     * Doubly-linked physical page chain.
     */
    vitte_pool_page_t *first_page;
    vitte_pool_page_t *last_page;

    /*
     * Intrusive free-list.
     *
     * Free slots themselves store the next pointer.
     */
    vitte_pool_free_node_t *free_list;

    /*
     * Current and lifetime high-water page counts.
     */
    size_t page_count;
    size_t peak_page_count;

    /*
     * Total number of physical slots across all pages.
     */
    size_t capacity;

    /*
     * Current live-object count.
     */
    size_t live_count;

    /*
     * Lifetime high-water live-object count.
     */
    size_t peak_live_count;

    /*
     * Current free-slot count.
     *
     * Valid pool invariant:
     *
     *     live_count + free_count == capacity
     */
    size_t free_count;

    /*
     * Current physical bytes reserved by all pages.
     */
    size_t reserved_bytes;

    /*
     * Lifetime high-water reserved-byte count.
     */
    size_t peak_reserved_bytes;

    /*
     * Lifetime successful allocation-operation count.
     *
     * Reuse of a previously freed slot counts as another allocation.
     *
     * Saturates at SIZE_MAX.
     */
    size_t allocation_count;

    /*
     * Lifetime successful explicit free-operation count.
     *
     * free(NULL) does not increment it.
     * reset() does not increment it.
     *
     * Saturates at SIZE_MAX.
     */
    size_t free_operation_count;

    /*
     * Logical pool generation.
     *
     * Starts at 1.
     * reset() advances it.
     * Zero is reserved as invalid/uninitialized.
     */
    uint64_t generation;

    /*
     * Last pool-local error.
     */
    vitte_pool_error_t last_error;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_pool_stats {
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

    size_t allocation_count;
    size_t free_operation_count;

    uint64_t generation;

    size_t max_objects;
    size_t max_reserved_bytes;
} vitte_pool_stats_t;

/* ========================================================================= */
/* Configuration API                                                         */
/* ========================================================================= */

vitte_pool_config_t
vitte_pool_config_default(void);

/*
 * Validate a configuration object.
 *
 * This function requires:
 *
 *     objects_per_page != 0
 *     objects_per_page <= VITTE_POOL_MAX_OBJECTS_PER_PAGE
 *
 * Zero max_objects/max_reserved_bytes are accepted because init() normalizes
 * those values before storing the effective configuration.
 */
bool
vitte_pool_config_validate(
    const vitte_pool_config_t *config);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize pool storage.
 *
 * object_size:
 *
 *     must be non-zero.
 *
 * alignment:
 *
 *     zero selects alignof(max_align_t).
 *
 *     otherwise it must be a power of two and must not exceed
 *     alignof(max_align_t).
 *
 * config:
 *
 *     NULL selects vitte_pool_config_default().
 *
 * No physical page is allocated here.
 *
 * Pages are created lazily by reserve()/alloc().
 *
 * Reinitialization contract:
 *
 *     A live initialized pool must be destroyed before being initialized
 *     again. Reinitializing a live pool without destroy() would discard page
 *     ownership and leak memory.
 */
bool
vitte_pool_init(
    vitte_pool_t *pool,
    size_t object_size,
    size_t alignment,
    const vitte_pool_config_t *config);

bool
vitte_pool_init_default(
    vitte_pool_t *pool,
    size_t object_size,
    size_t alignment);

/*
 * Release all physical pages and invalidate the pool.
 *
 * Every outstanding pool pointer becomes invalid.
 *
 * Calling destroy() on an invalid/non-live pool is a no-op.
 */
void
vitte_pool_destroy(
    vitte_pool_t *pool);

/* ========================================================================= */
/* Reservation                                                               */
/* ========================================================================= */

/*
 * Ensure:
 *
 *     capacity >= minimum_capacity
 *
 * Existing live objects are never moved.
 *
 * This operation may allocate multiple pages.
 *
 * A failure can leave capacity increased if earlier page allocations from the
 * same reserve operation succeeded before a later page failed. Existing pool
 * state remains usable.
 */
bool
vitte_pool_reserve(
    vitte_pool_t *pool,
    size_t minimum_capacity);

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

/*
 * Allocate one slot.
 *
 * Common free-list path is O(1), excluding defensive page ownership lookup
 * performed by the current implementation.
 *
 * The returned pointer:
 *
 *     - begins a physical slot
 *     - satisfies the pool alignment
 *     - remains live until free(), reset(), trim-related destruction after
 *       becoming free, or destroy()
 *
 * When zero_on_allocate is enabled, the complete physical slot is zeroed.
 */
void *
vitte_pool_alloc(
    vitte_pool_t *pool);

/*
 * Allocate one slot and zero at least object_size bytes.
 *
 * When zero_on_allocate is already enabled, alloc() has already zeroed the
 * complete slot; the additional logical-object memset remains harmless.
 */
void *
vitte_pool_calloc(
    vitte_pool_t *pool);

/* ========================================================================= */
/* Release                                                                   */
/* ========================================================================= */

/*
 * Return one live slot to the pool.
 *
 * pointer == NULL:
 *
 *     succeeds as a no-op, analogous to free(NULL).
 *
 * Detects:
 *
 *     foreign pointers
 *     interior pointers
 *     non-slot-aligned pointers
 *     double frees
 *     inconsistent live accounting
 *
 * After successful release, the pointer must no longer be used as an object
 * pointer. Its storage is immediately eligible for reuse and may contain an
 * intrusive free-list link.
 */
bool
vitte_pool_free(
    vitte_pool_t *pool,
    void *pointer);

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

/*
 * Logically release every live object while retaining pages.
 *
 * Effects:
 *
 *     - generation advances
 *     - every bitmap is cleared
 *     - every page live_count becomes zero
 *     - pool live_count becomes zero
 *     - free-list is rebuilt deterministically
 *
 * All previously live object pointers become logically invalid.
 *
 * allocation_count/free_operation_count and high-water statistics remain
 * cumulative.
 */
void
vitte_pool_reset(
    vitte_pool_t *pool);

/* ========================================================================= */
/* Trim                                                                      */
/* ========================================================================= */

/*
 * Release empty physical pages.
 *
 * A page is empty when:
 *
 *     page->live_count == 0
 *
 * If retain_empty_pages is true, trim() retains one deterministic empty page.
 *
 * Live objects are never moved.
 *
 * Returns the number of pages physically released.
 */
size_t
vitte_pool_trim(
    vitte_pool_t *pool);

/*
 * Equivalent conceptually to:
 *
 *     reset(pool);
 *     trim all empty pages;
 *
 * retain_empty_pages is temporarily ignored so all now-empty pages are
 * eligible for destruction.
 */
void
vitte_pool_reset_and_trim(
    vitte_pool_t *pool);

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

/*
 * Return true if pointer lies anywhere inside a page's physical slot-storage
 * region.
 *
 * Therefore an interior pointer can satisfy contains().
 */
bool
vitte_pool_contains(
    const vitte_pool_t *pool,
    const void *pointer);

/*
 * Return true only when pointer equals the beginning of a physical slot.
 *
 * The slot may currently be live or free.
 */
bool
vitte_pool_contains_object(
    const vitte_pool_t *pool,
    const void *pointer);

/*
 * Return true only when pointer:
 *
 *     - is a physical slot beginning
 *     - belongs to this pool
 *     - is marked allocated by the page bitmap
 */
bool
vitte_pool_is_live(
    const vitte_pool_t *pool,
    const void *pointer);

/* ========================================================================= */
/* Validity                                                                  */
/* ========================================================================= */

/*
 * Lightweight cookie check.
 *
 * This does not prove structural integrity.
 *
 * Use vitte_pool_validate() for deep validation.
 */
bool
vitte_pool_is_valid(
    const vitte_pool_t *pool);

/* ========================================================================= */
/* Accessors                                                                 */
/* ========================================================================= */

size_t
vitte_pool_object_size(
    const vitte_pool_t *pool);

size_t
vitte_pool_slot_size(
    const vitte_pool_t *pool);

size_t
vitte_pool_alignment(
    const vitte_pool_t *pool);

size_t
vitte_pool_capacity(
    const vitte_pool_t *pool);

size_t
vitte_pool_live_count(
    const vitte_pool_t *pool);

size_t
vitte_pool_free_count(
    const vitte_pool_t *pool);

size_t
vitte_pool_page_count(
    const vitte_pool_t *pool);

size_t
vitte_pool_reserved_bytes(
    const vitte_pool_t *pool);

uint64_t
vitte_pool_generation(
    const vitte_pool_t *pool);

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

vitte_pool_error_t
vitte_pool_last_error(
    const vitte_pool_t *pool);

void
vitte_pool_clear_error(
    vitte_pool_t *pool);

const char *
vitte_pool_error_name(
    vitte_pool_error_t error);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_pool_stats_t
vitte_pool_stats(
    const vitte_pool_t *pool);

/* ========================================================================= */
/* Deep validation                                                          */
/* ========================================================================= */

/*
 * Perform expensive structural validation.
 *
 * Intended for:
 *
 *     debug builds
 *     invariant assertions
 *     unit tests
 *     fuzzing
 *     ASan
 *     UBSan
 *
 * Validation includes:
 *
 *     pool integrity cookie
 *     non-zero generation
 *     object-size invariants
 *     slot-size invariants
 *     alignment invariants
 *     normalized configuration
 *
 *     page-chain endpoint consistency
 *     page-chain cycle detection
 *     forward/backward linkage
 *     page cookies
 *     page generation
 *     page slot counts
 *     page layout recomputation
 *     bitmap size
 *     slots offset
 *     allocation size
 *     slot alignment
 *     unused bitmap bits
 *     bitmap/live-count agreement
 *
 *     total page count
 *     total capacity
 *     total live count
 *     total reserved bytes
 *     live + free == capacity
 *
 *     page-count high-water invariant
 *     live-count high-water invariant
 *     reserved-byte high-water invariant
 *     configured object limit
 *     configured memory limit
 *
 *     free-list null/count agreement
 *     free-list ownership
 *     free-list slot-boundary validation
 *     free-list/bitmap agreement
 *     free-list cycle detection
 *     duplicate free-list entries
 *     missing free-list entries
 *     live slots absent from free-list
 *
 * Complexity is deliberately much greater than ordinary allocation paths.
 */
bool
vitte_pool_validate(
    const vitte_pool_t *pool);

/* ========================================================================= */
/* Convenience predicates                                                    */
/* ========================================================================= */

static inline bool
vitte_pool_is_empty(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_is_valid(pool) &&
        pool->live_count == 0u;
}

static inline bool
vitte_pool_has_live_objects(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_is_valid(pool) &&
        pool->live_count != 0u;
}

static inline bool
vitte_pool_has_free_objects(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_is_valid(pool) &&
        pool->free_count != 0u;
}

static inline bool
vitte_pool_has_pages(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_is_valid(pool) &&
        pool->page_count != 0u;
}

static inline bool
vitte_pool_has_error(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_is_valid(pool) &&
        pool->last_error !=
            VITTE_POOL_ERROR_NONE;
}

static inline bool
vitte_pool_out_of_memory(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_is_valid(pool) &&
        pool->last_error ==
            VITTE_POOL_ERROR_OUT_OF_MEMORY;
}

static inline bool
vitte_pool_limit_exceeded(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_is_valid(pool) &&
        pool->last_error ==
            VITTE_POOL_ERROR_LIMIT_EXCEEDED;
}

/*
 * Physical pool fullness only.
 *
 * A full current capacity does not necessarily mean that the pool cannot
 * grow.
 */
static inline bool
vitte_pool_is_full(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_is_valid(pool) &&
        pool->capacity != 0u &&
        pool->live_count ==
            pool->capacity;
}

/*
 * Number of immediately reusable slots.
 *
 * Does not include slots that could be added by future page growth.
 */
static inline size_t
vitte_pool_available_objects(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_is_valid(pool)) {
        return 0u;
    }

    return pool->free_count;
}

/* ========================================================================= */
/* Limit helpers                                                             */
/* ========================================================================= */

/*
 * Remaining physical capacity allowed by max_objects.
 *
 * SIZE_MAX means no configured object-capacity bound.
 */
static inline size_t
vitte_pool_remaining_object_limit(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_is_valid(pool)) {
        return 0u;
    }

    if (pool->config.max_objects ==
        SIZE_MAX) {
        return SIZE_MAX;
    }

    if (pool->capacity >=
        pool->config.max_objects) {
        return 0u;
    }

    return
        pool->config.max_objects -
        pool->capacity;
}

/*
 * Remaining page-memory budget.
 *
 * SIZE_MAX means no configured reserved-byte bound.
 */
static inline size_t
vitte_pool_remaining_reserved_limit(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_is_valid(pool)) {
        return 0u;
    }

    if (pool->config.max_reserved_bytes ==
        SIZE_MAX) {
        return SIZE_MAX;
    }

    if (pool->reserved_bytes >=
        pool->config.max_reserved_bytes) {
        return 0u;
    }

    return
        pool->config.max_reserved_bytes -
        pool->reserved_bytes;
}

/* ========================================================================= */
/* Utilization                                                               */
/* ========================================================================= */

/*
 * Return physical slot utilization in basis points.
 *
 *       0 =   0.00%
 *    2500 =  25.00%
 *    5000 =  50.00%
 *    7500 =  75.00%
 *   10000 = 100.00%
 *
 * Exact result:
 *
 *     floor(live_count * 10000 / capacity)
 *
 * No floating point and no overflowing live_count * 10000 multiplication are
 * required.
 */
static inline uint32_t
vitte_pool_utilization_basis_points(
    const vitte_pool_t *pool)
{
    size_t capacity;
    size_t remainder;
    uint32_t result;
    unsigned digit_index;

    if (!vitte_pool_is_valid(pool)) {
        return UINT32_C(0);
    }

    capacity =
        pool->capacity;

    remainder =
        pool->live_count;

    if (capacity == 0u ||
        remainder == 0u) {
        return UINT32_C(0);
    }

    if (remainder >= capacity) {
        return UINT32_C(10000);
    }

    /*
     * Long division of:
     *
     *     remainder / capacity
     *
     * to four decimal places.
     *
     * For each digit compute:
     *
     *     floor(remainder * 10 / capacity)
     *     (remainder * 10) % capacity
     *
     * without ever materializing remainder * 10.
     *
     * Ten modular additions are enough because:
     *
     *     remainder < capacity
     *
     * and the branch:
     *
     *     accumulator >= capacity - remainder
     *
     * computes accumulator + remainder - capacity without overflowing.
     */
    result = UINT32_C(0);

    for (digit_index = 0u;
         digit_index < 4u;
         ++digit_index) {
        size_t accumulator;
        unsigned digit;
        unsigned addition;

        accumulator = 0u;
        digit = 0u;

        for (addition = 0u;
             addition < 10u;
             ++addition) {
            if (accumulator >=
                capacity - remainder) {
                accumulator -=
                    capacity - remainder;

                ++digit;
            } else {
                accumulator +=
                    remainder;
            }
        }

        result =
            result * UINT32_C(10) +
            (uint32_t)digit;

        remainder =
            accumulator;
    }

    return result;
}

/* ========================================================================= */
/* Typed allocation helpers                                                  */
/* ========================================================================= */

/*
 * These macros do not dynamically verify that sizeof(type) and alignof(type)
 * match the pool's initialization parameters.
 *
 * For compiler-internal use, initialize the pool with the matching typed
 * initialization macro and keep that type invariant for the pool lifetime.
 */

#define VITTE_POOL_NEW(pool, type) \
    ((type *)vitte_pool_alloc((pool)))

#define VITTE_POOL_NEW_ZERO(pool, type) \
    ((type *)vitte_pool_calloc((pool)))

#define VITTE_POOL_DELETE(pool, pointer) \
    vitte_pool_free( \
        (pool), \
        (void *)(pointer))

/* ========================================================================= */
/* Typed initialization helpers                                              */
/* ========================================================================= */

#define VITTE_POOL_INIT_FOR_TYPE(pool, type) \
    vitte_pool_init_default( \
        (pool), \
        sizeof(type), \
        VITTE_POOL_ALIGNOF(type))

#define VITTE_POOL_INIT_FOR_TYPE_CONFIG(pool, type, config) \
    vitte_pool_init( \
        (pool), \
        sizeof(type), \
        VITTE_POOL_ALIGNOF(type), \
        (config))

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__cplusplus)

static_assert(
    VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE != 0u,
    "pool default objects-per-page must not be zero");

static_assert(
    VITTE_POOL_MIN_OBJECTS_PER_PAGE != 0u,
    "pool minimum objects-per-page must not be zero");

static_assert(
    VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE >=
        VITTE_POOL_MIN_OBJECTS_PER_PAGE,
    "pool default page size must satisfy minimum");

static_assert(
    VITTE_POOL_MAX_OBJECTS_PER_PAGE >=
        VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE,
    "pool maximum page size must satisfy default");

static_assert(
    VITTE_POOL_GROWTH_FACTOR >= 1u,
    "pool growth factor must be at least one");

static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte pool requires 64-bit uint64_t");

static_assert(
    sizeof(uintptr_t) >= sizeof(void *),
    "uintptr_t must represent native pointers");

static_assert(
    VITTE_POOL_ALIGNOF(max_align_t) != 0u,
    "max_align_t must have valid alignment");

#elif defined(__STDC_VERSION__) && \
      __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE != 0u,
    "pool default objects-per-page must not be zero");

_Static_assert(
    VITTE_POOL_MIN_OBJECTS_PER_PAGE != 0u,
    "pool minimum objects-per-page must not be zero");

_Static_assert(
    VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE >=
        VITTE_POOL_MIN_OBJECTS_PER_PAGE,
    "pool default page size must satisfy minimum");

_Static_assert(
    VITTE_POOL_MAX_OBJECTS_PER_PAGE >=
        VITTE_POOL_DEFAULT_OBJECTS_PER_PAGE,
    "pool maximum page size must satisfy default");

_Static_assert(
    VITTE_POOL_GROWTH_FACTOR >= 1u,
    "pool growth factor must be at least one");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte pool requires 64-bit uint64_t");

_Static_assert(
    sizeof(uintptr_t) >= sizeof(void *),
    "uintptr_t must represent native pointers");

_Static_assert(
    VITTE_POOL_ALIGNOF(max_align_t) != 0u,
    "max_align_t must have valid alignment");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_POOL_H */