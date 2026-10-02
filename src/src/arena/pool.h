#ifndef VITTE_SRC_ARENA_POOL_H
#define VITTE_SRC_ARENA_POOL_H

/*
 * Vitte Compiler
 * src/arena/pool.h
 *
 * Fixed-size reusable object pool.
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
 * Pool properties:
 *
 *     - fixed object size
 *     - fixed alignment
 *     - paged allocation
 *     - O(1) common allocation
 *     - O(1) release
 *     - intrusive free-list
 *     - allocation bitmap
 *     - double-free detection
 *     - foreign-pointer rejection
 *     - configurable growth
 *     - object limit
 *     - reserved-memory limit
 *     - reset
 *     - page trimming
 *     - optional memory poisoning
 *     - detailed statistics
 *     - deep validation
 *
 * Pool allocations differ from ordinary arena allocations because individual
 * objects may be returned to the pool and reused.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
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
    /*
     * No error.
     */
    VITTE_POOL_ERROR_NONE = 0,

    /*
     * Invalid/destroyed pool object.
     */
    VITTE_POOL_ERROR_INVALID_POOL,

    /*
     * Invalid argument.
     */
    VITTE_POOL_ERROR_INVALID_ARGUMENT,

    /*
     * Alignment is zero where forbidden, not a power of two, or exceeds the
     * alignment supported by this implementation.
     */
    VITTE_POOL_ERROR_INVALID_ALIGNMENT,

    /*
     * Integer arithmetic overflow.
     */
    VITTE_POOL_ERROR_OVERFLOW,

    /*
     * malloc() failed.
     */
    VITTE_POOL_ERROR_OUT_OF_MEMORY,

    /*
     * max_objects or max_reserved_bytes prevented growth.
     */
    VITTE_POOL_ERROR_LIMIT_EXCEEDED,

    /*
     * Pointer does not belong to any page in this pool.
     */
    VITTE_POOL_ERROR_FOREIGN_POINTER,

    /*
     * Pointer lies inside a page but is not exactly at a slot boundary.
     */
    VITTE_POOL_ERROR_MISALIGNED_POINTER,

    /*
     * Object slot is already free.
     */
    VITTE_POOL_ERROR_DOUBLE_FREE,

    /*
     * Internal pool invariant failure.
     */
    VITTE_POOL_ERROR_CORRUPTION
} vitte_pool_error_t;

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

typedef struct vitte_pool_config {
    /*
     * Number of slots requested for the first page.
     *
     * init() treats zero as "use default".
     */
    size_t objects_per_page;

    /*
     * Maximum total slot capacity.
     *
     * SIZE_MAX means unlimited.
     *
     * init() treats zero as "use default/unlimited".
     */
    size_t max_objects;

    /*
     * Maximum number of bytes reserved by pool pages.
     *
     * This includes:
     *
     *     page headers
     *     allocation bitmaps
     *     alignment padding
     *     slot storage
     *
     * SIZE_MAX means unlimited.
     *
     * init() treats zero as "use default/unlimited".
     */
    size_t max_reserved_bytes;

    /*
     * Grow page slot counts geometrically.
     *
     * false:
     *
     *     each page uses approximately objects_per_page slots
     *
     * true:
     *
     *     subsequent pages grow by VITTE_POOL_GROWTH_FACTOR, bounded by
     *     VITTE_POOL_MAX_OBJECTS_PER_PAGE and configured limits.
     */
    bool geometric_growth;

    /*
     * Keep one empty page when trim() is called.
     *
     * reset_and_trim() explicitly overrides this temporarily and releases all
     * empty pages.
     */
    bool retain_empty_pages;

    /*
     * Zero newly allocated slots before returning them.
     */
    bool zero_on_allocate;
} vitte_pool_config_t;

/* ========================================================================= */
/* Pool                                                                      */
/* ========================================================================= */

/*
 * The pool structure is intentionally visible to compiler-internal users.
 *
 * Consumers should nevertheless use the API instead of modifying fields
 * directly.
 */
struct vitte_pool {
    /*
     * Runtime integrity cookie.
     */
    uint64_t magic;

    /*
     * Logical object size requested by the caller.
     */
    size_t object_size;

    /*
     * Physical slot size.
     *
     * slot_size >= object_size
     * slot_size >= sizeof(free-list node)
     * slot_size is a multiple of alignment
     */
    size_t slot_size;

    /*
     * Object alignment.
     */
    size_t alignment;

    /*
     * Effective configuration.
     */
    vitte_pool_config_t config;

    /*
     * Doubly-linked page chain.
     */
    vitte_pool_page_t *first_page;
    vitte_pool_page_t *last_page;

    /*
     * Intrusive list of currently free slots.
     *
     * The concrete node structure is private to pool.c.
     */
    vitte_pool_free_node_t *free_list;

    /*
     * Current/peak page counts.
     */
    size_t page_count;
    size_t peak_page_count;

    /*
     * Total number of physical slots.
     */
    size_t capacity;

    /*
     * Current number of allocated/live slots.
     */
    size_t live_count;

    /*
     * Lifetime high-water live-object count.
     */
    size_t peak_live_count;

    /*
     * Current number of free slots.
     *
     * In a valid pool:
     *
     *     live_count + free_count == capacity
     */
    size_t free_count;

    /*
     * Current/peak physical page memory.
     */
    size_t reserved_bytes;
    size_t peak_reserved_bytes;

    /*
     * Lifetime number of successful allocation operations.
     *
     * Saturates at SIZE_MAX.
     */
    size_t allocation_count;

    /*
     * Lifetime number of successful explicit free operations.
     *
     * reset() does not increment this counter.
     *
     * Saturates at SIZE_MAX.
     */
    size_t free_operation_count;

    /*
     * Pool generation.
     *
     * Incremented by reset().
     *
     * Zero is reserved.
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

/*
 * Return the default pool configuration.
 */
vitte_pool_config_t
vitte_pool_config_default(void);

/*
 * Validate an already-normalized configuration.
 *
 * Important:
 *
 *     config_validate() rejects objects_per_page == 0.
 *
 *     vitte_pool_init() treats objects_per_page == 0 as "use default" before
 *     validating the effective configuration.
 */
bool
vitte_pool_config_validate(
    const vitte_pool_config_t *config);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize a fixed-size object pool.
 *
 * object_size:
 *
 *     must be non-zero.
 *
 * alignment:
 *
 *     0 selects _Alignof(max_align_t)
 *
 *     otherwise must be a power of two and no larger than
 *     _Alignof(max_align_t).
 *
 * config:
 *
 *     NULL selects vitte_pool_config_default().
 *
 * No page is allocated during initialization. Physical memory is allocated
 * lazily on reserve()/alloc().
 */
bool
vitte_pool_init(
    vitte_pool_t *pool,
    size_t object_size,
    size_t alignment,
    const vitte_pool_config_t *config);

/*
 * Initialize with default configuration.
 */
bool
vitte_pool_init_default(
    vitte_pool_t *pool,
    size_t object_size,
    size_t alignment);

/*
 * Release every physical page and invalidate the pool object.
 *
 * All outstanding pool pointers become invalid.
 */
void
vitte_pool_destroy(
    vitte_pool_t *pool);

/* ========================================================================= */
/* Reservation                                                               */
/* ========================================================================= */

/*
 * Ensure that total slot capacity is at least minimum_capacity.
 *
 * May allocate multiple pages.
 *
 * Existing objects are not moved.
 */
bool
vitte_pool_reserve(
    vitte_pool_t *pool,
    size_t minimum_capacity);

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

/*
 * Allocate one object.
 *
 * Common case:
 *
 *     O(1)
 *
 * The returned pointer has the alignment supplied to vitte_pool_init().
 *
 * If config.zero_on_allocate is enabled, the slot is zeroed.
 */
void *
vitte_pool_alloc(
    vitte_pool_t *pool);

/*
 * Allocate one object and zero object_size bytes regardless of the
 * zero_on_allocate configuration.
 */
void *
vitte_pool_calloc(
    vitte_pool_t *pool);

/* ========================================================================= */
/* Release                                                                   */
/* ========================================================================= */

/*
 * Return one live object to the pool.
 *
 * pointer == NULL is accepted and succeeds, matching free(NULL)-style
 * semantics.
 *
 * Detects:
 *
 *     - foreign pointers
 *     - interior/misaligned pointers
 *     - double free
 *     - accounting corruption
 */
bool
vitte_pool_free(
    vitte_pool_t *pool,
    void *pointer);

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

/*
 * Mark every object free while retaining pages.
 *
 * All outstanding object pointers become logically invalid.
 *
 * The pool generation is advanced.
 *
 * Lifetime statistics remain available.
 */
void
vitte_pool_reset(
    vitte_pool_t *pool);

/* ========================================================================= */
/* Trim                                                                      */
/* ========================================================================= */

/*
 * Release empty pages.
 *
 * When config.retain_empty_pages is true, trim() retains one page.
 *
 * Returns the number of physical pages released.
 */
size_t
vitte_pool_trim(
    vitte_pool_t *pool);

/*
 * Reset all objects and release all empty pages.
 *
 * Since reset makes every page empty, this normally releases every page.
 */
void
vitte_pool_reset_and_trim(
    vitte_pool_t *pool);

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

/*
 * Return true when pointer lies anywhere inside the slot-storage region of a
 * pool page.
 *
 * An interior pointer can therefore satisfy contains().
 */
bool
vitte_pool_contains(
    const vitte_pool_t *pool,
    const void *pointer);

/*
 * Return true only when pointer is exactly the beginning of a physical slot.
 *
 * The slot may be either live or free.
 */
bool
vitte_pool_contains_object(
    const vitte_pool_t *pool,
    const void *pointer);

/*
 * Return true when pointer is exactly a slot beginning and that slot is
 * currently allocated according to the page bitmap.
 */
bool
vitte_pool_is_live(
    const vitte_pool_t *pool,
    const void *pointer);

/* ========================================================================= */
/* Pool validity                                                             */
/* ========================================================================= */

/*
 * Lightweight pool-object validity check.
 *
 * This verifies the pool integrity cookie only.
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
 *     - debug builds
 *     - unit tests
 *     - fuzzing
 *     - ASan
 *     - UBSan
 *     - compiler invariant checking
 *
 * Validation includes:
 *
 *     pool cookie
 *     object/slot/alignment invariants
 *     configuration
 *     page chain integrity
 *     page metadata
 *     page layout
 *     bitmap/live-count agreement
 *     total page count
 *     total capacity
 *     total live count
 *     total reserved bytes
 *     live + free == capacity
 *     peak counters
 *     configured limits
 *     free-list cycle detection
 *     free-list ownership
 *     free-list slot alignment
 *     free-list/bitmap agreement
 *     duplicate free-list entry detection
 *     missing free-list entry detection
 *
 * This operation can be substantially more expensive than ordinary pool
 * operations and should not be placed on hot production paths.
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
        vitte_pool_live_count(pool) == 0u;
}

static inline bool
vitte_pool_has_live_objects(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_live_count(pool) != 0u;
}

static inline bool
vitte_pool_has_free_objects(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_free_count(pool) != 0u;
}

static inline bool
vitte_pool_has_pages(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_page_count(pool) != 0u;
}

static inline bool
vitte_pool_has_error(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_last_error(pool) !=
        VITTE_POOL_ERROR_NONE;
}

static inline bool
vitte_pool_out_of_memory(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_last_error(pool) ==
        VITTE_POOL_ERROR_OUT_OF_MEMORY;
}

static inline bool
vitte_pool_limit_exceeded(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_last_error(pool) ==
        VITTE_POOL_ERROR_LIMIT_EXCEEDED;
}

static inline bool
vitte_pool_is_full(
    const vitte_pool_t *pool)
{
    if (!vitte_pool_is_valid(pool)) {
        return false;
    }

    return
        pool->capacity != 0u &&
        pool->live_count ==
            pool->capacity;
}

static inline size_t
vitte_pool_available_objects(
    const vitte_pool_t *pool)
{
    return
        vitte_pool_free_count(pool);
}

/* ========================================================================= */
/* Limit helpers                                                             */
/* ========================================================================= */

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
 * Return slot utilization in basis points:
 *
 *       0 =   0.00%
 *    5000 =  50.00%
 *   10000 = 100.00%
 *
 * This implementation deliberately avoids floating point.
 */
static inline uint32_t
vitte_pool_utilization_basis_points(
    const vitte_pool_t *pool)
{
    size_t live;
    size_t capacity;

    if (!vitte_pool_is_valid(pool)) {
        return UINT32_C(0);
    }

    live =
        pool->live_count;

    capacity =
        pool->capacity;

    if (capacity == 0u ||
        live == 0u) {
        return UINT32_C(0);
    }

    if (live >= capacity) {
        return UINT32_C(10000);
    }

    /*
     * Exact fast path.
     */
    if (live <=
        SIZE_MAX / (size_t)10000u) {
        return
            (uint32_t)(
                (live * (size_t)10000u) /
                capacity);
    }

    /*
     * Overflow-safe decomposition.
     *
     * Compute:
     *
     *     floor(live * 10000 / capacity)
     *
     * one decimal digit at a time.
     *
     * Because live < capacity, every generated digit is in [0, 9].
     *
     * rem * 10 may itself overflow, so determine the next decimal digit by
     * repeated subtraction using capacity/10 decomposition.
     *
     * For statistics this branch is relevant only at extreme size_t values.
     */
    {
        uint32_t result;
        size_t rem;
        unsigned digit_index;

        result = UINT32_C(0);
        rem = live;

        for (digit_index = 0u;
             digit_index < 4u;
             ++digit_index) {
            unsigned digit;
            size_t quotient;
            size_t remainder;

            /*
             * Calculate floor(rem * 10 / capacity) without forming rem * 10.
             *
             * capacity = 10*q + r
             *
             * For each possible digit d in [1,9], test whether:
             *
             *     rem >= ceil(d * capacity / 10)
             *
             * Products are bounded because d <= 9.
             */
            quotient =
                capacity / (size_t)10u;

            remainder =
                capacity % (size_t)10u;

            digit = 0u;

            {
                unsigned candidate;

                for (candidate = 1u;
                     candidate <= 9u;
                     ++candidate) {
                    size_t threshold;
                    size_t qpart;
                    size_t rpart;

                    /*
                     * candidate * quotient cannot exceed capacity.
                     */
                    qpart =
                        quotient *
                        (size_t)candidate;

                    rpart =
                        remainder *
                        (size_t)candidate;

                    threshold =
                        qpart +
                        rpart / (size_t)10u;

                    if ((rpart %
                         (size_t)10u) != 0u) {
                        ++threshold;
                    }

                    if (rem >= threshold) {
                        digit = candidate;
                    } else {
                        break;
                    }
                }
            }

            result =
                result * UINT32_C(10) +
                (uint32_t)digit;

            /*
             * Compute:
             *
             *     rem = rem * 10 - digit * capacity
             *
             * without overflowing rem * 10.
             *
             * Since digit == floor(rem*10/capacity), resulting rem is
             * strictly less than capacity.
             */
            {
                size_t ten_q;
                size_t ten_r;
                size_t digit_q;
                size_t digit_r;

                ten_q =
                    rem / capacity;

                ten_r =
                    rem % capacity;

                /*
                 * rem < capacity here, therefore ten_q is normally zero.
                 * Keep the representation explicit for defensive clarity.
                 */
                (void)ten_q;

                digit_q =
                    capacity / (size_t)10u;

                digit_r =
                    capacity % (size_t)10u;

                /*
                 * At extreme size_t values an exact portable generic mul-div
                 * helper would be preferable. For the pool statistics API,
                 * use a reduced safe remainder reconstruction.
                 */
                if (ten_r <=
                    SIZE_MAX / (size_t)10u) {
                    size_t scaled;
                    size_t subtract;

                    scaled =
                        ten_r *
                        (size_t)10u;

                    if ((size_t)digit <=
                        SIZE_MAX / capacity) {
                        subtract =
                            (size_t)digit *
                            capacity;

                        if (scaled >= subtract) {
                            rem =
                                scaled -
                                subtract;
                        } else {
                            rem = 0u;
                        }
                    } else {
                        rem = 0u;
                    }
                } else {
                    /*
                     * Conservative reduced fallback. This affects only the
                     * final basis-point precision at pathological near-SIZE_MAX
                     * capacities, never pool correctness.
                     */
                    size_t threshold;

                    threshold =
                        digit_q *
                        (size_t)digit;

                    if (digit_r != 0u) {
                        size_t extra;

                        extra =
                            (digit_r *
                             (size_t)digit +
                             (size_t)9u) /
                            (size_t)10u;

                        if (threshold <=
                            SIZE_MAX - extra) {
                            threshold += extra;
                        }
                    }

                    if (ten_r >= threshold) {
                        rem =
                            ten_r -
                            threshold;
                    } else {
                        rem = 0u;
                    }
                }
            }
        }

        if (result >
            UINT32_C(10000)) {
            result =
                UINT32_C(10000);
        }

        return result;
    }
}

/* ========================================================================= */
/* Typed allocation helpers                                                  */
/* ========================================================================= */

/*
 * Allocate one typed object.
 *
 * The caller is responsible for ensuring sizeof(type) matches the object size
 * used to initialize the pool.
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
        _Alignof(type))

#define VITTE_POOL_INIT_FOR_TYPE_CONFIG(pool, type, config) \
    vitte_pool_init( \
        (pool), \
        sizeof(type), \
        _Alignof(type), \
        (config))

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
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
    _Alignof(max_align_t) != 0u,
    "max_align_t must have valid alignment");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_POOL_H */
