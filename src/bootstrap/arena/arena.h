#ifndef VITTE_SRC_ARENA_ARENA_H
#define VITTE_SRC_ARENA_ARENA_H

/*
 * Vitte Compiler
 * src/arena/arena.h
 *
 * High-level compiler arena.
 *
 * allocator.h provides the low-level block allocator.
 * This interface adds the compiler-facing policy layer used by:
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
 *   - configurable allocator block size
 *   - logical allocation limits
 *   - bounded string operations
 *   - arbitrary power-of-two alignment
 *   - overflow-safe array allocation
 *   - zero-initialized allocation
 *   - typed allocation helpers
 *   - memory/string duplication
 *   - arena-backed formatted strings
 *   - transactional checkpoints
 *   - rollback
 *   - reset and block reuse
 *   - trimming
 *   - current-generation accounting
 *   - lifetime accounting
 *   - ownership queries
 *   - structural validation
 *
 * Individual arena allocations cannot be freed independently.
 *
 * Allocation lifetime ends when the allocation is invalidated by:
 *
 *   - vitte_arena_context_rollback()
 *   - vitte_arena_context_reset()
 *   - vitte_arena_context_reset_and_trim()
 *   - vitte_arena_context_destroy()
 *
 * ISO C17.
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
#define VITTE_ARENA_DEFAULT_LIMIT \
    SIZE_MAX
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
     * Context is initialized and operational.
     */
    VITTE_ARENA_STATE_READY = 1,

    /*
     * Context encountered an unrecoverable high-level invariant failure.
     *
     * A failed context must not be used for further allocation.
     * Destruction remains permitted.
     */
    VITTE_ARENA_STATE_FAILED = 2,

    /*
     * Context has been destroyed.
     */
    VITTE_ARENA_STATE_DESTROYED = 3
} vitte_arena_state_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_arena_context_error {
    VITTE_ARENA_CONTEXT_ERROR_NONE = 0,

    /*
     * Context pointer or runtime validation cookie is invalid.
     */
    VITTE_ARENA_CONTEXT_ERROR_INVALID_CONTEXT = 1,

    /*
     * Invalid public function argument.
     */
    VITTE_ARENA_CONTEXT_ERROR_INVALID_ARGUMENT = 2,

    /*
     * Requested alignment is not zero and is not a power of two.
     */
    VITTE_ARENA_CONTEXT_ERROR_INVALID_ALIGNMENT = 3,

    /*
     * Integer/address/size arithmetic overflow.
     */
    VITTE_ARENA_CONTEXT_ERROR_OVERFLOW = 4,

    /*
     * Logical memory limit, string limit or formatted-string limit exceeded.
     */
    VITTE_ARENA_CONTEXT_ERROR_LIMIT_EXCEEDED = 5,

    /*
     * Underlying system allocation failed.
     */
    VITTE_ARENA_CONTEXT_ERROR_OUT_OF_MEMORY = 6,

    /*
     * Checkpoint does not belong to this context/current generation or its
     * captured allocator state is no longer valid.
     */
    VITTE_ARENA_CONTEXT_ERROR_INVALID_CHECKPOINT = 7,

    /*
     * Internal allocator, accounting or lifecycle invariant failed.
     */
    VITTE_ARENA_CONTEXT_ERROR_CORRUPTION = 8,

    /*
     * Operation is not valid in the current lifecycle state.
     */
    VITTE_ARENA_CONTEXT_ERROR_INVALID_STATE = 9
} vitte_arena_context_error_t;

/* ========================================================================= */
/* Configuration object                                                      */
/* ========================================================================= */

typedef struct vitte_arena_config {
    /*
     * Preferred payload capacity of ordinary low-level blocks.
     *
     * Zero means VITTE_ARENA_DEFAULT_BLOCK_SIZE.
     *
     * A non-zero value below VITTE_ARENA_MIN_BLOCK_SIZE is invalid.
     */
    size_t block_size;

    /*
     * Maximum logical caller-requested bytes that may remain live in the
     * current arena generation.
     *
     * SIZE_MAX means unlimited.
     *
     * Zero supplied during initialization is normalized to
     * VITTE_ARENA_DEFAULT_LIMIT.
     *
     * This is a logical allocation limit. It does not include:
     *
     *   - allocator block headers
     *   - unused block capacity
     *   - malloc implementation overhead
     *   - alignment padding
     *
     * String allocations do include their terminating NUL because that byte
     * is explicitly requested from the low-level allocator by the high-level
     * string operation.
     */
    size_t memory_limit;

    /*
     * Maximum string payload length, excluding the terminating NUL.
     *
     * Zero supplied during initialization selects
     * VITTE_ARENA_DEFAULT_STRING_LIMIT.
     *
     * After initialization, set_string_limit(0) is rejected.
     */
    size_t string_limit;

    /*
     * true:
     *     reset invalidates allocations but retains low-level blocks for
     *     subsequent reuse.
     *
     * false:
     *     reset invalidates allocations and releases retained blocks.
     */
    bool retain_blocks_on_reset;
} vitte_arena_config_t;

/* ========================================================================= */
/* High-level arena context                                                  */
/* ========================================================================= */

/*
 * Compiler-facing arena state.
 *
 * This is an internal compiler structure, not a stable external ABI.
 *
 * It may be embedded directly in parser, AST, semantic-analysis, HIR, IR and
 * other compiler subsystem structures.
 */
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
     * Effective normalized configuration.
     */
    vitte_arena_config_t config;

    /*
     * Low-level block allocator.
     */
    vitte_arena_t allocator;

    /*
     * High-level generation.
     *
     * Synchronized with allocator.generation.
     *
     * Reset operations increment the generation and invalidate all existing
     * checkpoints.
     */
    uint64_t generation;

    /*
     * Number of successful high-level allocations currently represented by
     * the current generation.
     *
     * Rollback restores this counter.
     * Reset sets it to zero.
     */
    size_t allocation_count;

    /*
     * Logical caller-requested bytes currently represented by the current
     * generation.
     *
     * Important:
     *
     * A high-level zero-byte request contributes zero here even though the
     * low-level allocator intentionally converts a zero-byte allocation into
     * a one-byte physical allocation.
     *
     * String operations account for their NUL terminator because they request
     * length + 1 bytes from this high-level allocation interface.
     */
    size_t requested_bytes;

    /*
     * Number of successful high-level allocations performed during the
     * context lifetime.
     *
     * Rollback and reset do not decrement this value.
     *
     * Saturates at SIZE_MAX.
     */
    size_t lifetime_allocation_count;

    /*
     * Sum of logical high-level requested bytes accumulated over the context
     * lifetime.
     *
     * Rollback and reset do not decrement this value.
     *
     * Saturates at SIZE_MAX.
     */
    size_t lifetime_requested_bytes;

    /*
     * Maximum current-generation logical requested-byte count observed during
     * the context lifetime.
     *
     * Rollback/reset do not reduce the peak.
     */
    size_t peak_requested_bytes;

    /*
     * Most recent high-level arena error.
     */
    vitte_arena_context_error_t last_error;
} vitte_arena_context_t;

/* ========================================================================= */
/* Checkpoint                                                                */
/* ========================================================================= */

/*
 * Transactional high-level arena checkpoint.
 *
 * A checkpoint captures:
 *
 *   - the owning context
 *   - the low-level allocator mark
 *   - the high-level generation
 *   - current high-level allocation count
 *   - current high-level requested-byte count
 *
 * Lifetime counters are intentionally not captured because rollback does not
 * erase historical allocation activity.
 *
 * Checkpoints are invalidated by:
 *
 *   - reset
 *   - reset_and_trim
 *   - destroy
 *
 * A checkpoint must never be used with another arena context.
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
     * Effective low-level preferred block size.
     */
    size_t block_size;

    /*
     * Current and lifetime-peak low-level block counts.
     */
    size_t block_count;
    size_t peak_block_count;

    /*
     * Current and peak low-level payload capacity.
     *
     * Does not include block headers or malloc implementation overhead.
     */
    size_t reserved_bytes;
    size_t peak_reserved_bytes;

    /*
     * Current and peak consumed low-level payload bytes.
     *
     * Includes alignment padding and the allocator's physical normalization
     * of zero-size allocations.
     */
    size_t used_bytes;
    size_t peak_used_bytes;

    /*
     * Requested bytes recorded by allocator.c.
     *
     * This is intentionally distinct from requested_bytes below.
     *
     * In particular, allocator.c normalizes a zero-byte allocation to one
     * physical byte before updating its own requested-byte accounting, while
     * the high-level context preserves the caller's logical zero-byte request.
     *
     * Therefore allocator_requested_bytes is not required to equal the
     * high-level requested_bytes field.
     */
    size_t allocator_requested_bytes;

    /*
     * Low-level payload capacity currently unused.
     */
    size_t unused_bytes;

    /*
     * Low-level consumed bytes not represented by allocator-requested bytes.
     *
     * This primarily reflects alignment padding.
     */
    size_t padding_bytes;

    /*
     * High-level current-generation logical accounting.
     */
    size_t requested_bytes;
    size_t peak_requested_bytes;

    size_t allocation_count;

    /*
     * High-level lifetime accounting.
     */
    size_t lifetime_requested_bytes;
    size_t lifetime_allocation_count;

    /*
     * Current high-level generation.
     */
    uint64_t generation;

    /*
     * Effective logical limits.
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
 * Validate the externally supplied configuration shape.
 *
 * Zero-valued fields that have documented initialization-time default
 * semantics are accepted here and normalized by vitte_arena_context_init().
 */
bool
vitte_arena_config_validate(
    const vitte_arena_config_t *config);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize a high-level arena context.
 *
 * config == NULL selects vitte_arena_config_default().
 *
 * Initialization is lazy with respect to low-level blocks: no allocation
 * block is required until the first successful allocation request.
 *
 * The storage referenced by context must not contain a live arena context.
 * Destroy a live context before reusing the same object for initialization.
 *
 * Returns true on success.
 */
bool
vitte_arena_context_init(
    vitte_arena_context_t *context,
    const vitte_arena_config_t *config);

/*
 * Initialize using the standard configuration.
 */
bool
vitte_arena_context_init_default(
    vitte_arena_context_t *context);

/*
 * Destroy all low-level blocks and invalidate the context.
 *
 * All allocations and checkpoints produced by this context become invalid.
 *
 * Passing NULL or a non-live context is harmless.
 */
void
vitte_arena_context_destroy(
    vitte_arena_context_t *context);

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

/*
 * Allocate size bytes using the low-level allocator's default alignment.
 *
 * size == 0 is permitted.
 *
 * The high-level logical accounting records zero requested bytes for such a
 * request, while allocator.c may physically reserve one byte to provide a
 * usable arena-owned address.
 *
 * Returns NULL on failure.
 */
void *
vitte_arena_context_alloc(
    vitte_arena_context_t *context,
    size_t size);

/*
 * Allocate size bytes with explicit alignment.
 *
 * alignment == 0 selects the low-level default alignment.
 *
 * A non-zero alignment must be a power of two.
 *
 * Returns NULL on failure.
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
 * Multiplication overflow is detected before allocation.
 *
 * Zero count and/or zero element size are permitted and produce the same
 * zero-size allocation semantics as vitte_arena_context_alloc().
 */
void *
vitte_arena_context_alloc_array(
    vitte_arena_context_t *context,
    size_t count,
    size_t element_size,
    size_t alignment);

/*
 * Allocate count * element_size bytes using the default alignment and
 * initialize the logical requested byte range to zero.
 */
void *
vitte_arena_context_calloc(
    vitte_arena_context_t *context,
    size_t count,
    size_t element_size);

/*
 * Aligned variant of vitte_arena_context_calloc().
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

/*
 * Duplicate size bytes into arena-owned storage.
 *
 * data may be NULL only when size == 0.
 */
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
 * The source payload length must not exceed string_limit.
 *
 * The returned allocation contains:
 *
 *     payload length + 1 terminating NUL byte
 *
 * The complete allocation size is subject to memory_limit.
 */
char *
vitte_arena_context_strdup(
    vitte_arena_context_t *context,
    const char *text);

/*
 * Duplicate exactly length bytes and append a terminating NUL.
 *
 * This function does not stop at an earlier NUL byte in the source range.
 *
 * length must not exceed string_limit.
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
 * Format a string directly into arena-owned storage.
 *
 * The formatted payload must satisfy both:
 *
 *   - context->config.string_limit
 *   - VITTE_ARENA_MAX_FORMATTED_STRING
 *
 * The terminating NUL is additionally included in the logical allocation
 * request.
 *
 * Returns NULL on formatting/allocation failure.
 */
char *
vitte_arena_context_printf(
    vitte_arena_context_t *context,
    const char *format,
    ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

/*
 * va_list variant of vitte_arena_context_printf().
 *
 * The function internally copies the supplied va_list as required; the
 * caller retains responsibility for the lifetime of its own va_list.
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
 * Capture the current allocation and high-level accounting state.
 *
 * An all-zero checkpoint is returned when context is not operational.
 */
vitte_arena_checkpoint_t
vitte_arena_context_checkpoint(
    const vitte_arena_context_t *context);

/*
 * Discard allocations performed after checkpoint.
 *
 * Current-generation allocation_count and requested_bytes are restored.
 *
 * Lifetime statistics and peak statistics are intentionally not rolled back.
 *
 * Returns false if checkpoint:
 *
 *   - belongs to another context
 *   - belongs to another generation
 *   - contains an invalid low-level mark
 *   - represents impossible accounting
 *   - cannot be safely rewound
 */
bool
vitte_arena_context_rollback(
    vitte_arena_context_t *context,
    vitte_arena_checkpoint_t checkpoint);

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

/*
 * Invalidate every current-generation allocation.
 *
 * Existing checkpoints become invalid.
 *
 * If retain_blocks_on_reset is true, blocks are retained for reuse.
 * Otherwise blocks are released.
 *
 * Lifetime and peak statistics remain intact.
 */
void
vitte_arena_context_reset(
    vitte_arena_context_t *context);

/*
 * Release currently unused low-level blocks.
 *
 * Live allocations remain valid.
 */
void
vitte_arena_context_trim(
    vitte_arena_context_t *context);

/*
 * Invalidate all current allocations and release retained blocks.
 *
 * Existing checkpoints become invalid.
 */
void
vitte_arena_context_reset_and_trim(
    vitte_arena_context_t *context);

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

/*
 * Return true if pointer lies anywhere inside the payload capacity of a block
 * currently owned by this operational arena context.
 *
 * This does not prove that pointer references a live allocation or the start
 * of an allocation.
 */
bool
vitte_arena_context_contains(
    const vitte_arena_context_t *context,
    const void *pointer);

/*
 * Return true if pointer lies inside the currently used payload region of a
 * block owned by this operational arena context.
 *
 * This still does not prove that pointer is exactly an allocation start.
 */
bool
vitte_arena_context_contains_live(
    const vitte_arena_context_t *context,
    const void *pointer);

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

/*
 * Return the most recent high-level error.
 *
 * Invalid contexts report VITTE_ARENA_CONTEXT_ERROR_INVALID_CONTEXT.
 */
vitte_arena_context_error_t
vitte_arena_context_last_error(
    const vitte_arena_context_t *context);

/*
 * Clear both the high-level error and the low-level allocator error.
 */
void
vitte_arena_context_clear_error(
    vitte_arena_context_t *context);

/*
 * Return a stable machine-readable error name.
 */
const char *
vitte_arena_context_error_name(
    vitte_arena_context_error_t error);

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

/*
 * Return true only when:
 *
 *   - context magic is valid
 *   - lifecycle state is READY
 *   - the low-level allocator validates
 *   - high-level and low-level generations agree
 */
bool
vitte_arena_context_is_valid(
    const vitte_arena_context_t *context);

/*
 * Return a stable machine-readable lifecycle state name.
 */
const char *
vitte_arena_state_name(
    vitte_arena_state_t state);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

/*
 * Return a combined low-level/high-level statistics snapshot.
 *
 * An all-zero structure is returned for an invalid context.
 */
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
 * Return the logical current-generation requested-byte limit.
 *
 * SIZE_MAX means unlimited.
 *
 * Zero is returned for an invalid context.
 */
size_t
vitte_arena_context_memory_limit(
    const vitte_arena_context_t *context);

/*
 * Change the logical current-generation requested-byte limit.
 *
 * limit == 0 means unlimited and is normalized to SIZE_MAX.
 *
 * Lowering the limit below currently live logical requested bytes fails
 * without modifying the existing limit.
 */
bool
vitte_arena_context_set_memory_limit(
    vitte_arena_context_t *context,
    size_t limit);

/* ========================================================================= */
/* String limit                                                              */
/* ========================================================================= */

/*
 * Return the current maximum accepted string payload length.
 *
 * Zero is returned for an invalid context.
 */
size_t
vitte_arena_context_string_limit(
    const vitte_arena_context_t *context);

/*
 * Change the maximum accepted string payload length.
 *
 * Zero is invalid.
 *
 * Existing strings are unaffected.
 */
bool
vitte_arena_context_set_string_limit(
    vitte_arena_context_t *context,
    size_t limit);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Perform an expensive structural validation of a live context.
 *
 * Validation includes:
 *
 *   - context magic
 *   - READY lifecycle state
 *   - normalized configuration
 *   - low-level allocator invariants
 *   - generation synchronization
 *   - high-level current accounting
 *   - lifetime accounting
 *   - peak accounting
 *   - logical memory-limit compliance
 *   - low-level reserved/used relationship
 *
 * Low-level allocator requested-byte accounting is deliberately not required
 * to equal high-level logical requested-byte accounting because zero-size
 * requests are physically normalized by allocator.c.
 *
 * Intended for:
 *
 *   - unit tests
 *   - fuzzing
 *   - ASan
 *   - UBSan
 *   - debug builds
 *   - compiler self-checking
 */
bool
vitte_arena_context_validate(
    const vitte_arena_context_t *context);

/* ========================================================================= */
/* Typed allocation                                                          */
/* ========================================================================= */

/*
 * Allocate storage for one typed object.
 */
#define VITTE_ARENA_CONTEXT_NEW(context, type) \
    ((type *)vitte_arena_context_alloc_aligned( \
        (context), \
        sizeof(type), \
        _Alignof(type)))

/*
 * Allocate zero-initialized storage for one typed object.
 */
#define VITTE_ARENA_CONTEXT_NEW_ZERO(context, type) \
    ((type *)vitte_arena_context_calloc_aligned( \
        (context), \
        (size_t)1u, \
        sizeof(type), \
        _Alignof(type)))

/*
 * Allocate an array of typed objects.
 */
#define VITTE_ARENA_CONTEXT_NEW_ARRAY(context, type, count) \
    ((type *)vitte_arena_context_alloc_array( \
        (context), \
        (count), \
        sizeof(type), \
        _Alignof(type)))

/*
 * Allocate a zero-initialized array of typed objects.
 */
#define VITTE_ARENA_CONTEXT_NEW_ARRAY_ZERO(context, type, count) \
    ((type *)vitte_arena_context_calloc_aligned( \
        (context), \
        (count), \
        sizeof(type), \
        _Alignof(type)))

/* ========================================================================= */
/* Convenience queries                                                       */
/* ========================================================================= */

static inline bool
vitte_arena_context_is_empty(
    const vitte_arena_context_t *context)
{
    return
        vitte_arena_context_allocation_count(
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

static inline bool
vitte_arena_context_failed(
    const vitte_arena_context_t *context)
{
    return
        context != NULL &&
        context->state ==
            VITTE_ARENA_STATE_FAILED;
}

/* ========================================================================= */
/* Remaining logical capacity                                                */
/* ========================================================================= */

static inline size_t
vitte_arena_context_remaining_limit(
    const vitte_arena_context_t *context)
{
    size_t limit;
    size_t requested;

    if (context == NULL) {
        return 0u;
    }

    limit =
        vitte_arena_context_memory_limit(
            context);

    if (limit == SIZE_MAX) {
        return SIZE_MAX;
    }

    requested =
        vitte_arena_context_requested_bytes(
            context);

    return limit >= requested
        ? limit - requested
        : 0u;
}

/* ========================================================================= */
/* Checkpoint convenience                                                    */
/* ========================================================================= */

/*
 * Lightweight checkpoint-shape predicate.
 *
 * This does NOT prove that the checkpoint is currently rewindable.
 * Full validation occurs inside vitte_arena_context_rollback().
 */
static inline bool
vitte_arena_checkpoint_is_valid(
    const vitte_arena_checkpoint_t *checkpoint)
{
    return
        checkpoint != NULL &&
        checkpoint->context != NULL &&
        checkpoint->generation != 0u &&
        checkpoint->mark.arena != NULL &&
        checkpoint->mark.generation != 0u;
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
 * Return physical payload utilization in basis points.
 *
 *     10000 = 100.00%
 *      5000 =  50.00%
 *         1 =   0.01%
 *
 * This helper uses integer arithmetic only and avoids evaluating
 *
 *     used * 10000
 *
 * directly, so the computation remains safe even when size_t values are near
 * SIZE_MAX.
 */
static inline uint32_t
vitte_arena_context_utilization_basis_points(
    const vitte_arena_context_t *context)
{
    size_t reserved;
    size_t used;
    size_t quotient;
    size_t remainder;
    uint32_t result;
    uint32_t bit;

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

    /*
     * Since used < reserved, quotient is normally zero. Keep this branch for
     * mathematical completeness and defensive robustness.
     */
    quotient =
        used / reserved;

    remainder =
        used % reserved;

    if (quotient != 0u) {
        return UINT32_C(10000);
    }

    /*
     * Compute:
     *
     *     floor(remainder * 10000 / reserved)
     *
     * without overflowing size_t.
     *
     * 10000 requires 14 bits. Build the scaled fraction one binary bit at a
     * time using quotient/remainder arithmetic.
     */
    result = 0u;

    for (bit = UINT32_C(8192);
         bit != 0u;
         bit >>= 1u) {
        size_t doubled;

        if ((UINT32_C(10000) & bit) == 0u) {
            continue;
        }

        /*
         * Add:
         *
         *     floor(remainder * bit / reserved)
         *
         * using repeated doubling of a fraction modulo reserved.
         */
        {
            size_t fraction;
            uint32_t contribution;
            uint32_t step;

            fraction =
                remainder;

            contribution = 0u;

            for (step = UINT32_C(1);
                 step < bit;
                 step <<= 1u) {
                if (fraction >=
                    reserved - fraction) {
                    doubled =
                        fraction -
                        (reserved - fraction);

                    contribution =
                        contribution * UINT32_C(2) +
                        UINT32_C(1);
                } else {
                    doubled =
                        fraction + fraction;

                    contribution *=
                        UINT32_C(2);
                }

                fraction =
                    doubled;
            }

            if (result >
                UINT32_C(10000) -
                    contribution) {
                return UINT32_C(10000);
            }

            result +=
                contribution;
        }
    }

    if (result >
        UINT32_C(10000)) {
        result =
            UINT32_C(10000);
    }

    return result;
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
    VITTE_ARENA_LARGE_ALLOCATION_DIVISOR != 0u,
    "arena large-allocation divisor must not be zero");

_Static_assert(
    VITTE_ARENA_DEFAULT_BLOCK_SIZE >=
        VITTE_ARENA_MIN_BLOCK_SIZE,
    "default arena block size must satisfy allocator minimum");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte arena context requires a 64-bit uint64_t");

_Static_assert(
    sizeof(uintptr_t) >= sizeof(void *),
    "uintptr_t must be capable of representing native pointers");

_Static_assert(
    _Alignof(max_align_t) != 0u,
    "max_align_t must provide a valid alignment");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_ARENA_H */