#ifndef VITTE_SRC_ARENA_ARENA_H
#define VITTE_SRC_ARENA_ARENA_H

/*
 * Vitte Compiler
 * src/arena/arena.h
 *
 * High-level compiler arena.
 *
 * allocator.h provides the low-level block allocator.
 * arena.h provides the compiler-facing abstraction used by:
 *
 *   - lexer
 *   - parser
 *   - AST
 *   - semantic analysis
 *   - HIR
 *   - IR
 *   - diagnostics
 *   - temporary compiler passes
 *
 * Features:
 *
 *   - configurable block size
 *   - allocation limits
 *   - string limits
 *   - aligned allocations
 *   - zeroed allocations
 *   - typed allocation helpers
 *   - memory duplication
 *   - string duplication
 *   - formatted strings
 *   - checkpoints and rollback
 *   - reset and block reuse
 *   - trimming
 *   - lifetime statistics
 *   - ownership queries
 *   - invariant validation
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_ARENA_DEFAULT_LIMIT
#define VITTE_ARENA_DEFAULT_LIMIT SIZE_MAX
#endif

#ifndef VITTE_ARENA_DEFAULT_STRING_LIMIT
#define VITTE_ARENA_DEFAULT_STRING_LIMIT \
    ((size_t)64u * (size_t)1024u * (size_t)1024u)
#endif

#ifndef VITTE_ARENA_MAX_FORMATTED_STRING
#define VITTE_ARENA_MAX_FORMATTED_STRING \
    ((size_t)64u * (size_t)1024u * (size_t)1024u)
#endif

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

typedef enum vitte_arena_state {
    /*
     * Storage exists but initialization has not completed.
     */
    VITTE_ARENA_STATE_UNINITIALIZED = 0,

    /*
     * Arena is initialized and available for allocation.
     */
    VITTE_ARENA_STATE_READY = 1,

    /*
     * Arena encountered a fatal higher-level failure.
     *
     * The current arena.c does not normally transition here after successful
     * initialization, but the state is retained for lifecycle diagnostics and
     * future compiler integration.
     */
    VITTE_ARENA_STATE_FAILED = 2,

    /*
     * Arena has been destroyed.
     */
    VITTE_ARENA_STATE_DESTROYED = 3
} vitte_arena_state_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_arena_context_error {
    VITTE_ARENA_CONTEXT_ERROR_NONE = 0,

    /*
     * Context pointer or magic value is invalid.
     */
    VITTE_ARENA_CONTEXT_ERROR_INVALID_CONTEXT,

    /*
     * Invalid function argument.
     */
    VITTE_ARENA_CONTEXT_ERROR_INVALID_ARGUMENT,

    /*
     * Requested alignment is invalid.
     */
    VITTE_ARENA_CONTEXT_ERROR_INVALID_ALIGNMENT,

    /*
     * Integer arithmetic overflow.
     */
    VITTE_ARENA_CONTEXT_ERROR_OVERFLOW,

    /*
     * Configured memory/string limit exceeded.
     */
    VITTE_ARENA_CONTEXT_ERROR_LIMIT_EXCEEDED,

    /*
     * Underlying system allocator failed.
     */
    VITTE_ARENA_CONTEXT_ERROR_OUT_OF_MEMORY,

    /*
     * Checkpoint belongs to another context/generation or is otherwise
     * invalid.
     */
    VITTE_ARENA_CONTEXT_ERROR_INVALID_CHECKPOINT,

    /*
     * Internal allocator/accounting invariant failed.
     */
    VITTE_ARENA_CONTEXT_ERROR_CORRUPTION,

    /*
     * Operation is not valid in the current lifecycle state.
     */
    VITTE_ARENA_CONTEXT_ERROR_INVALID_STATE
} vitte_arena_context_error_t;

/* ========================================================================= */
/* Configuration object                                                      */
/* ========================================================================= */

typedef struct vitte_arena_config {
    /*
     * Preferred size of ordinary allocator blocks.
     *
     * Zero is normalized to VITTE_ARENA_DEFAULT_BLOCK_SIZE by init().
     */
    size_t block_size;

    /*
     * Maximum number of caller-requested bytes allowed in one arena
     * generation.
     *
     * SIZE_MAX means unlimited.
     *
     * This limit concerns logical requested bytes rather than actual malloc()
     * reservation size.
     */
    size_t memory_limit;

    /*
     * Maximum accepted string length.
     *
     * This protects strdup/strndup/formatted-string operations against
     * pathological inputs.
     */
    size_t string_limit;

    /*
     * true:
     *     reset keeps blocks for future reuse.
     *
     * false:
     *     reset releases retained blocks.
     */
    bool retain_blocks_on_reset;
} vitte_arena_config_t;

/* ========================================================================= */
/* High-level arena context                                                  */
/* ========================================================================= */

typedef struct vitte_arena_context {
    /*
     * Runtime validation cookie.
     */
    uint64_t magic;

    /*
     * Lifecycle state.
     */
    vitte_arena_state_t state;

    /*
     * Effective configuration.
     */
    vitte_arena_config_t config;

    /*
     * Low-level allocator.
     */
    vitte_arena_t allocator;

    /*
     * High-level generation.
     *
     * Kept synchronized with allocator.generation.
     */
    uint64_t generation;

    /*
     * Successful allocations in the current generation.
     */
    size_t allocation_count;

    /*
     * Logical caller-requested bytes in the current generation.
     *
     * Unlike the low-level allocator accounting, this intentionally preserves
     * a zero-byte request as zero requested bytes.
     */
    size_t requested_bytes;

    /*
     * Successful allocations over the entire context lifetime.
     *
     * Rollback/reset does not decrement this counter.
     */
    size_t lifetime_allocation_count;

    /*
     * Logical requested bytes accumulated over the entire context lifetime.
     *
     * Rollback/reset does not decrement this counter.
     */
    size_t lifetime_requested_bytes;

    /*
     * Maximum logical requested-byte count observed in one generation.
     */
    size_t peak_requested_bytes;

    /*
     * Last high-level arena error.
     */
    vitte_arena_context_error_t last_error;
} vitte_arena_context_t;

/* ========================================================================= */
/* Checkpoint                                                                */
/* ========================================================================= */

/*
 * High-level transactional checkpoint.
 *
 * A checkpoint captures both:
 *
 *   - low-level allocator position
 *   - high-level accounting state
 *
 * It is only valid for the exact context and generation that created it.
 */
typedef struct vitte_arena_checkpoint {
    const vitte_arena_context_t *context;

    vitte_arena_mark_t mark;

    uint64_t generation;

    size_t allocation_count;
    size_t requested_bytes;
} vitte_arena_checkpoint_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_arena_context_stats {
    /*
     * Low-level allocator configuration/statistics.
     */
    size_t block_size;

    size_t block_count;
    size_t peak_block_count;

    size_t reserved_bytes;
    size_t peak_reserved_bytes;

    size_t used_bytes;
    size_t peak_used_bytes;

    /*
     * Requested bytes reported by allocator.c.
     *
     * This may differ slightly from high-level requested_bytes because the
     * low-level allocator normalizes zero-size allocations internally.
     */
    size_t allocator_requested_bytes;

    size_t unused_bytes;
    size_t padding_bytes;

    /*
     * High-level logical accounting.
     */
    size_t requested_bytes;
    size_t peak_requested_bytes;

    size_t allocation_count;

    size_t lifetime_requested_bytes;
    size_t lifetime_allocation_count;

    uint64_t generation;

    /*
     * Effective limits.
     */
    size_t memory_limit;
    size_t string_limit;
} vitte_arena_context_stats_t;

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

/*
 * Return the standard compiler arena configuration.
 */
vitte_arena_config_t
vitte_arena_config_default(void);

/*
 * Validate an arena configuration.
 */
bool
vitte_arena_config_validate(
    const vitte_arena_config_t *config);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize a high-level arena.
 *
 * config == NULL selects the default configuration.
 *
 * Returns true on success.
 */
bool
vitte_arena_context_init(
    vitte_arena_context_t *context,
    const vitte_arena_config_t *config);

/*
 * Initialize using vitte_arena_config_default().
 */
bool
vitte_arena_context_init_default(
    vitte_arena_context_t *context);

/*
 * Destroy the arena and all memory owned by it.
 *
 * All pointers and checkpoints produced by this context become invalid.
 */
void
vitte_arena_context_destroy(
    vitte_arena_context_t *context);

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

/*
 * Allocate size bytes using the allocator's default alignment.
 */
void *
vitte_arena_context_alloc(
    vitte_arena_context_t *context,
    size_t size);

/*
 * Allocate size bytes with explicit power-of-two alignment.
 *
 * alignment == 0 selects the low-level allocator default.
 */
void *
vitte_arena_context_alloc_aligned(
    vitte_arena_context_t *context,
    size_t size,
    size_t alignment);

/* ========================================================================= */
/* Arrays                                                                    */
/* ========================================================================= */

/*
 * Allocate:
 *
 *     count * element_size
 *
 * bytes with explicit alignment.
 *
 * Multiplication overflow is checked before allocation.
 */
void *
vitte_arena_context_alloc_array(
    vitte_arena_context_t *context,
    size_t count,
    size_t element_size,
    size_t alignment);

/*
 * Allocate zero-initialized array storage.
 */
void *
vitte_arena_context_calloc(
    vitte_arena_context_t *context,
    size_t count,
    size_t element_size);

/*
 * Aligned zero-initialized array storage.
 */
void *
vitte_arena_context_calloc_aligned(
    vitte_arena_context_t *context,
    size_t count,
    size_t element_size,
    size_t alignment);

/* ========================================================================= */
/* Memory duplication                                                        */
/* ========================================================================= */

void *
vitte_arena_context_memdup(
    vitte_arena_context_t *context,
    const void *data,
    size_t size);

/* ========================================================================= */
/* Strings                                                                   */
/* ========================================================================= */

/*
 * Duplicate a NUL-terminated string into arena storage.
 *
 * The configured string_limit is enforced.
 */
char *
vitte_arena_context_strdup(
    vitte_arena_context_t *context,
    const char *text);

/*
 * Duplicate exactly length bytes and append a NUL terminator.
 */
char *
vitte_arena_context_strndup(
    vitte_arena_context_t *context,
    const char *text,
    size_t length);

/* ========================================================================= */
/* Formatted strings                                                         */
/* ========================================================================= */

/*
 * Arena-backed printf.
 *
 * Returned storage belongs to the arena.
 */
char *
vitte_arena_context_printf(
    vitte_arena_context_t *context,
    const char *format,
    ...);

/*
 * va_list variant.
 */
char *
vitte_arena_context_vprintf(
    vitte_arena_context_t *context,
    const char *format,
    va_list arguments)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 0)))
#endif
    ;

/* ========================================================================= */
/* Checkpoints                                                               */
/* ========================================================================= */

/*
 * Capture the current allocation/accounting state.
 */
vitte_arena_checkpoint_t
vitte_arena_context_checkpoint(
    const vitte_arena_context_t *context);

/*
 * Discard allocations performed after checkpoint.
 *
 * Lifetime statistics are intentionally not rolled back.
 */
bool
vitte_arena_context_rollback(
    vitte_arena_context_t *context,
    vitte_arena_checkpoint_t checkpoint);

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

/*
 * Invalidate all current-generation allocations.
 *
 * Whether blocks remain reserved depends on retain_blocks_on_reset.
 *
 * All existing checkpoints become invalid.
 */
void
vitte_arena_context_reset(
    vitte_arena_context_t *context);

/*
 * Release currently unused low-level blocks.
 */
void
vitte_arena_context_trim(
    vitte_arena_context_t *context);

/*
 * Reset and release retained blocks.
 */
void
vitte_arena_context_reset_and_trim(
    vitte_arena_context_t *context);

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

/*
 * Return true if pointer lies inside any block owned by this arena.
 *
 * The pointer does not necessarily reference a live allocation.
 */
bool
vitte_arena_context_contains(
    const vitte_arena_context_t *context,
    const void *pointer);

/*
 * Return true if pointer lies inside a currently used block region.
 */
bool
vitte_arena_context_contains_live(
    const vitte_arena_context_t *context,
    const void *pointer);

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

vitte_arena_context_error_t
vitte_arena_context_last_error(
    const vitte_arena_context_t *context);

void
vitte_arena_context_clear_error(
    vitte_arena_context_t *context);

const char *
vitte_arena_context_error_name(
    vitte_arena_context_error_t error);

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

bool
vitte_arena_context_is_valid(
    const vitte_arena_context_t *context);

const char *
vitte_arena_state_name(
    vitte_arena_state_t state);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_arena_context_stats_t
vitte_arena_context_stats(
    const vitte_arena_context_t *context);

size_t
vitte_arena_context_requested_bytes(
    const vitte_arena_context_t *context);

size_t
vitte_arena_context_allocation_count(
    const vitte_arena_context_t *context);

size_t
vitte_arena_context_reserved_bytes(
    const vitte_arena_context_t *context);

size_t
vitte_arena_context_used_bytes(
    const vitte_arena_context_t *context);

uint64_t
vitte_arena_context_generation(
    const vitte_arena_context_t *context);

/* ========================================================================= */
/* Memory limit                                                              */
/* ========================================================================= */

/*
 * Return the logical requested-byte limit.
 *
 * SIZE_MAX means unlimited.
 */
size_t
vitte_arena_context_memory_limit(
    const vitte_arena_context_t *context);

/*
 * Change the logical requested-byte limit.
 *
 * limit == 0 means unlimited and is normalized to SIZE_MAX.
 *
 * Lowering the limit below current requested_bytes fails.
 */
bool
vitte_arena_context_set_memory_limit(
    vitte_arena_context_t *context,
    size_t limit);

/* ========================================================================= */
/* String limit                                                              */
/* ========================================================================= */

size_t
vitte_arena_context_string_limit(
    const vitte_arena_context_t *context);

/*
 * Change the maximum accepted string size.
 *
 * Zero is invalid.
 */
bool
vitte_arena_context_set_string_limit(
    vitte_arena_context_t *context,
    size_t limit);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Perform expensive structural validation.
 *
 * Checks include:
 *
 *   - context magic
 *   - lifecycle state
 *   - configuration
 *   - low-level allocator invariants
 *   - generation synchronization
 *   - requested-byte accounting
 *   - lifetime accounting
 *   - memory limits
 *   - allocator reserved/used relationship
 *
 * Intended for:
 *
 *   - tests
 *   - fuzzing
 *   - ASan/UBSan
 *   - debug builds
 *   - compiler self-checks
 */
bool
vitte_arena_context_validate(
    const vitte_arena_context_t *context);

/* ========================================================================= */
/* Typed allocation                                                          */
/* ========================================================================= */

/*
 * Allocate one typed object.
 */
#define VITTE_ARENA_CONTEXT_NEW(context, type) \
    ((type *)vitte_arena_context_alloc_aligned( \
        (context), \
        sizeof(type), \
        _Alignof(type)))

/*
 * Allocate one zero-initialized typed object.
 */
#define VITTE_ARENA_CONTEXT_NEW_ZERO(context, type) \
    ((type *)vitte_arena_context_calloc_aligned( \
        (context), \
        (size_t)1u, \
        sizeof(type), \
        _Alignof(type)))

/*
 * Allocate a typed array.
 */
#define VITTE_ARENA_CONTEXT_NEW_ARRAY(context, type, count) \
    ((type *)vitte_arena_context_alloc_array( \
        (context), \
        (count), \
        sizeof(type), \
        _Alignof(type)))

/*
 * Allocate a zero-initialized typed array.
 */
#define VITTE_ARENA_CONTEXT_NEW_ARRAY_ZERO(context, type, count) \
    ((type *)vitte_arena_context_calloc_aligned( \
        (context), \
        (count), \
        sizeof(type), \
        _Alignof(type)))

/* ========================================================================= */
/* Convenience                                                               */
/* ========================================================================= */

static inline bool
vitte_arena_context_is_empty(
    const vitte_arena_context_t *context)
{
    return
        vitte_arena_context_requested_bytes(
            context) == 0u;
}

static inline bool
vitte_arena_context_has_allocations(
    const vitte_arena_context_t *context)
{
    return
        vitte_arena_context_allocation_count(
            context) != 0u;
}

static inline bool
vitte_arena_context_has_error(
    const vitte_arena_context_t *context)
{
    return
        vitte_arena_context_last_error(
            context) !=
        VITTE_ARENA_CONTEXT_ERROR_NONE;
}

static inline bool
vitte_arena_context_out_of_memory(
    const vitte_arena_context_t *context)
{
    return
        vitte_arena_context_last_error(
            context) ==
        VITTE_ARENA_CONTEXT_ERROR_OUT_OF_MEMORY;
}

static inline bool
vitte_arena_context_limit_exceeded(
    const vitte_arena_context_t *context)
{
    return
        vitte_arena_context_last_error(
            context) ==
        VITTE_ARENA_CONTEXT_ERROR_LIMIT_EXCEEDED;
}

/* ========================================================================= */
/* Remaining logical capacity                                                */
/* ========================================================================= */

static inline size_t
vitte_arena_context_remaining_limit(
    const vitte_arena_context_t *context)
{
    size_t limit;
    size_t used;

    if (context == NULL) {
        return 0u;
    }

    limit =
        vitte_arena_context_memory_limit(
            context);

    if (limit == SIZE_MAX) {
        return SIZE_MAX;
    }

    used =
        vitte_arena_context_requested_bytes(
            context);

    return limit >= used
        ? limit - used
        : 0u;
}

/* ========================================================================= */
/* Checkpoint convenience                                                    */
/* ========================================================================= */

static inline bool
vitte_arena_checkpoint_is_valid(
    const vitte_arena_checkpoint_t *checkpoint)
{
    return
        checkpoint != NULL &&
        checkpoint->context != NULL &&
        checkpoint->generation != 0u;
}

/* ========================================================================= */
/* Statistics helpers                                                        */
/* ========================================================================= */

static inline size_t
vitte_arena_context_unused_bytes(
    const vitte_arena_context_t *context)
{
    size_t reserved;
    size_t used;

    reserved =
        vitte_arena_context_reserved_bytes(
            context);

    used =
        vitte_arena_context_used_bytes(
            context);

    return reserved >= used
        ? reserved - used
        : 0u;
}

/*
 * Physical payload utilization expressed in basis points.
 *
 *     10000 = 100.00%
 *      5000 =  50.00%
 */
static inline uint32_t
vitte_arena_context_utilization_basis_points(
    const vitte_arena_context_t *context)
{
    size_t reserved;
    size_t used;
    size_t whole;
    size_t remainder;
    size_t scaled;

    reserved =
        vitte_arena_context_reserved_bytes(
            context);

    used =
        vitte_arena_context_used_bytes(
            context);

    if (reserved == 0u) {
        return 0u;
    }

    if (used >= reserved) {
        return UINT32_C(10000);
    }

    whole = used / reserved;
    remainder = used % reserved;

    /*
     * remainder < reserved, but remainder * 10000 can still overflow size_t
     * on extremely large arenas. Decompose when necessary.
     */
    if (remainder <=
        SIZE_MAX / (size_t)10000u) {
        scaled =
            (remainder * (size_t)10000u) /
            reserved;
    } else {
        /*
         * Overflow-safe approximation preserving the [0,10000] contract.
         */
        scaled =
            (remainder / reserved) *
                (size_t)10000u;

        if (scaled == 0u) {
            long double ratio;

            ratio =
                (long double)remainder /
                (long double)reserved;

            scaled =
                (size_t)(
                    ratio *
                    (long double)10000.0L);
        }
    }

    scaled +=
        whole * (size_t)10000u;

    if (scaled > (size_t)10000u) {
        scaled = (size_t)10000u;
    }

    return (uint32_t)scaled;
}

/* ========================================================================= */
/* Checkpoint macros                                                         */
/* ========================================================================= */

#define VITTE_ARENA_CONTEXT_CHECKPOINT(context) \
    vitte_arena_context_checkpoint((context))

#define VITTE_ARENA_CONTEXT_ROLLBACK(context, checkpoint) \
    vitte_arena_context_rollback( \
        (context), \
        (checkpoint))

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_ARENA_DEFAULT_STRING_LIMIT != 0u,
    "default arena string limit must not be zero");

_Static_assert(
    VITTE_ARENA_MAX_FORMATTED_STRING != 0u,
    "formatted-string limit must not be zero");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte arena context requires 64-bit uint64_t");

_Static_assert(
    sizeof(uintptr_t) >= sizeof(void *),
    "uintptr_t must represent native pointers");

_Static_assert(
    _Alignof(max_align_t) != 0u,
    "max_align_t must provide valid alignment");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_ARENA_H */
