#ifndef VITTE_SRC_ARENA_RESET_H
#define VITTE_SRC_ARENA_RESET_H

/*
 * Vitte Compiler
 * src/arena/reset.h
 *
 * Coordinated arena/pool reset engine.
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
 *         reusable fixed-size object pools
 *
 *     reset.c/.h
 *         coordinated reset policy
 *
 * reset.c does not own arena or pool memory.
 *
 * It orchestrates the primitive reset operations exposed by arena.c and
 * pool.c and provides:
 *
 *     - reset policies
 *     - coordinated arena + pool reset
 *     - validation
 *     - callbacks
 *     - execution reports
 *     - before/after statistics
 *     - released-memory accounting
 *     - execution counters
 *
 * Registered arena/pool pointers are non-owning.
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

#ifndef VITTE_RESET_MAX_POOLS
#define VITTE_RESET_MAX_POOLS \
    ((size_t)1024u)
#endif

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_reset
    vitte_reset_t;

typedef struct vitte_reset_config
    vitte_reset_config_t;

typedef struct vitte_reset_report
    vitte_reset_report_t;

/* ========================================================================= */
/* Reset mode                                                                */
/* ========================================================================= */

/*
 * Reset policy applied to the arena and registered pools.
 */
typedef enum vitte_reset_mode {
    /*
     * Reset logical allocation state but retain physical memory for reuse.
     *
     * Arena:
     *
     *     vitte_arena_context_reset()
     *
     * Pools:
     *
     *     vitte_pool_reset()
     */
    VITTE_RESET_MODE_RETAIN = 0,

    /*
     * Reset logical state, then trim unused physical storage.
     *
     * Arena:
     *
     *     reset()
     *     trim()
     *
     * Pools:
     *
     *     reset()
     *     trim()
     *
     * Pool trim behavior still observes retain_empty_pages.
     */
    VITTE_RESET_MODE_TRIM = 1,

    /*
     * Reset logical state and aggressively release resettable physical
     * storage.
     *
     * Arena:
     *
     *     vitte_arena_context_reset_and_trim()
     *
     * Pools:
     *
     *     vitte_pool_reset_and_trim()
     */
    VITTE_RESET_MODE_RELEASE = 2
} vitte_reset_mode_t;

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

typedef enum vitte_reset_state {
    /*
     * Zero/uninitialized or invalid reset object.
     */
    VITTE_RESET_STATE_UNINITIALIZED = 0,

    /*
     * Initialized and available for execution.
     */
    VITTE_RESET_STATE_READY = 1,

    /*
     * Reset operation is currently executing.
     *
     * Structural configuration changes are forbidden in this state.
     */
    VITTE_RESET_STATE_RUNNING = 2,

    /*
     * Most recent execution completed successfully.
     */
    VITTE_RESET_STATE_COMPLETED = 3,

    /*
     * Most recent execution failed.
     *
     * Important:
     *
     * A reset operation is not generally transactional. Some targets may
     * already have been reset before a later target/callback fails.
     */
    VITTE_RESET_STATE_FAILED = 4,

    /*
     * Object has been explicitly destroyed.
     */
    VITTE_RESET_STATE_DESTROYED = 5
} vitte_reset_state_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_reset_error {
    VITTE_RESET_ERROR_NONE = 0,

    /*
     * Reset object is invalid, uninitialized or destroyed.
     */
    VITTE_RESET_ERROR_INVALID_RESET,

    /*
     * Invalid function argument or configuration value.
     */
    VITTE_RESET_ERROR_INVALID_ARGUMENT,

    /*
     * Operation is forbidden in the current reset state.
     */
    VITTE_RESET_ERROR_INVALID_STATE,

    /*
     * Arena target is invalid.
     */
    VITTE_RESET_ERROR_INVALID_ARENA,

    /*
     * Registered pool target is invalid.
     */
    VITTE_RESET_ERROR_INVALID_POOL,

    /*
     * VITTE_RESET_MAX_POOLS was reached.
     */
    VITTE_RESET_ERROR_TOO_MANY_POOLS,

    /*
     * Same pool was registered more than once.
     */
    VITTE_RESET_ERROR_DUPLICATE_POOL,

    /*
     * Arena reset operation left the arena invalid or otherwise failed.
     */
    VITTE_RESET_ERROR_ARENA_RESET_FAILED,

    /*
     * Pool reset operation failed.
     */
    VITTE_RESET_ERROR_POOL_RESET_FAILED,

    /*
     * Requested pre/post deep validation failed.
     */
    VITTE_RESET_ERROR_VALIDATION_FAILED,

    /*
     * A before/after callback returned false.
     */
    VITTE_RESET_ERROR_CALLBACK_FAILED,

    /*
     * Arithmetic/statistical overflow.
     */
    VITTE_RESET_ERROR_OVERFLOW,

    /*
     * Internal reset invariant failure.
     */
    VITTE_RESET_ERROR_CORRUPTION
} vitte_reset_error_t;

/* ========================================================================= */
/* Callback phase                                                            */
/* ========================================================================= */

typedef enum vitte_reset_callback_phase {
    /*
     * Called after pre-validation/statistics capture and before mutating any
     * registered target.
     */
    VITTE_RESET_CALLBACK_BEFORE = 0,

    /*
     * Called after reset, post-validation and post-statistics capture.
     */
    VITTE_RESET_CALLBACK_AFTER = 1
} vitte_reset_callback_phase_t;

/* ========================================================================= */
/* Callback                                                                  */
/* ========================================================================= */

/*
 * Reset callback.
 *
 * Return:
 *
 *     true
 *         continue / accept
 *
 *     false
 *         fail the reset execution with
 *         VITTE_RESET_ERROR_CALLBACK_FAILED
 *
 * user_data is the opaque pointer supplied to vitte_reset_set_callbacks().
 *
 * Callbacks must not recursively execute or structurally mutate the same
 * vitte_reset_t while it is RUNNING.
 */
typedef bool
(*vitte_reset_callback_t)(
    vitte_reset_t *reset,
    vitte_reset_callback_phase_t phase,
    void *user_data);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

struct vitte_reset_config {
    /*
     * Physical retention policy.
     */
    vitte_reset_mode_t mode;

    /*
     * Reset the registered arena when non-NULL.
     */
    bool reset_arena;

    /*
     * Reset registered pools.
     */
    bool reset_pools;

    /*
     * Run deep validation before mutation.
     *
     * Arena:
     *
     *     vitte_arena_context_validate()
     *
     * Pools:
     *
     *     vitte_pool_validate()
     */
    bool validate_before;

    /*
     * Run deep validation after mutation.
     */
    bool validate_after;

    /*
     * Stop immediately when a pool operation fails.
     *
     * false permits subsequent pools/arena processing to continue where the
     * implementation can safely do so, but the overall execution still
     * reports failure.
     */
    bool stop_on_pool_error;

    /*
     * Enable registered before/after callbacks.
     */
    bool invoke_callbacks;
};

/* ========================================================================= */
/* Execution report                                                          */
/* ========================================================================= */

/*
 * Snapshot/report for the most recent execution.
 *
 * "released" fields are non-negative:
 *
 *     before > after
 *         released = before - after
 *
 *     otherwise
 *         released = 0
 *
 * This matters because a target can theoretically reserve additional
 * metadata/storage while processing a reset.
 */
struct vitte_reset_report {
    /*
     * Overall execution result.
     */
    bool success;

    /*
     * Error associated with the execution.
     */
    vitte_reset_error_t error;

    /* --------------------------------------------------------------------- */
    /* Arena                                                                 */
    /* --------------------------------------------------------------------- */

    uint64_t arena_generation_before;
    uint64_t arena_generation_after;

    size_t arena_reserved_before;
    size_t arena_reserved_after;
    size_t arena_reserved_released;

    size_t arena_used_before;
    size_t arena_used_after;
    size_t arena_used_released;

    size_t arena_requested_before;
    size_t arena_requested_after;
    size_t arena_requested_released;

    size_t arena_allocations_before;
    size_t arena_allocations_after;

    /* --------------------------------------------------------------------- */
    /* Pools — aggregate across all registered valid pools                   */
    /* --------------------------------------------------------------------- */

    size_t pool_capacity_before;
    size_t pool_capacity_after;
    size_t pool_capacity_released;

    size_t pool_live_before;
    size_t pool_live_after;
    size_t pool_live_released;

    size_t pool_reserved_before;
    size_t pool_reserved_after;
    size_t pool_reserved_released;

    size_t pool_pages_before;
    size_t pool_pages_after;
    size_t pool_pages_released;
};

/* ========================================================================= */
/* Reset object                                                              */
/* ========================================================================= */

/*
 * Internal compiler reset coordinator.
 *
 * The arena and pool pointers are non-owning.
 *
 * Destroying this structure never destroys the registered arena/pools.
 */
struct vitte_reset {
    /*
     * Runtime integrity cookie.
     */
    uint64_t magic;

    /*
     * Coordinator state.
     */
    vitte_reset_state_t state;

    /*
     * Effective policy.
     */
    vitte_reset_config_t config;

    /*
     * Optional arena target.
     *
     * Non-owning.
     */
    vitte_arena_context_t *arena;

    /*
     * Registered pool targets.
     *
     * Non-owning.
     */
    vitte_pool_t *pools[VITTE_RESET_MAX_POOLS];

    /*
     * Number of valid entries at the beginning of pools[].
     */
    size_t pool_count;

    /*
     * Optional execution hooks.
     */
    vitte_reset_callback_t before_callback;
    vitte_reset_callback_t after_callback;

    void *callback_user_data;

    /*
     * Lifetime execution counters.
     *
     * Counters saturate at SIZE_MAX in reset.c.
     */
    size_t execution_count;
    size_t successful_execution_count;
    size_t failed_execution_count;

    /*
     * Last reset-engine error.
     */
    vitte_reset_error_t last_error;

    /*
     * Most recent execution report.
     */
    vitte_reset_report_t report;
};

/* ========================================================================= */
/* Configuration API                                                         */
/* ========================================================================= */

/*
 * Default:
 *
 *     mode               = RETAIN
 *     reset_arena        = true
 *     reset_pools        = true
 *     validate_before    = false
 *     validate_after     = false
 *     stop_on_pool_error = true
 *     invoke_callbacks   = true
 */
vitte_reset_config_t
vitte_reset_config_default(void);

bool
vitte_reset_config_validate(
    const vitte_reset_config_t *config);

/* ========================================================================= */
/* Report initialization                                                     */
/* ========================================================================= */

void
vitte_reset_report_init(
    vitte_reset_report_t *report);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize reset coordinator.
 *
 * arena:
 *
 *     NULL is allowed.
 *
 * config:
 *
 *     NULL selects vitte_reset_config_default().
 *
 * No target memory is modified during initialization.
 */
bool
vitte_reset_init(
    vitte_reset_t *reset,
    vitte_arena_context_t *arena,
    const vitte_reset_config_t *config);

bool
vitte_reset_init_default(
    vitte_reset_t *reset,
    vitte_arena_context_t *arena);

/*
 * Destroy only coordinator metadata.
 *
 * Does NOT destroy or reset the arena/pools.
 */
void
vitte_reset_destroy(
    vitte_reset_t *reset);

/* ========================================================================= */
/* Arena target                                                              */
/* ========================================================================= */

bool
vitte_reset_set_arena(
    vitte_reset_t *reset,
    vitte_arena_context_t *arena);

vitte_arena_context_t *
vitte_reset_arena(
    vitte_reset_t *reset);

const vitte_arena_context_t *
vitte_reset_arena_const(
    const vitte_reset_t *reset);

/* ========================================================================= */
/* Pool registration                                                         */
/* ========================================================================= */

/*
 * Register one pool.
 *
 * Duplicate registration is rejected.
 */
bool
vitte_reset_add_pool(
    vitte_reset_t *reset,
    vitte_pool_t *pool);

/*
 * Remove one registered pool.
 *
 * Remaining entries are compacted.
 */
bool
vitte_reset_remove_pool(
    vitte_reset_t *reset,
    vitte_pool_t *pool);

/*
 * Remove every pool registration without modifying the pools themselves.
 */
void
vitte_reset_clear_pools(
    vitte_reset_t *reset);

size_t
vitte_reset_pool_count(
    const vitte_reset_t *reset);

vitte_pool_t *
vitte_reset_pool_at(
    vitte_reset_t *reset,
    size_t index);

const vitte_pool_t *
vitte_reset_pool_at_const(
    const vitte_reset_t *reset,
    size_t index);

/* ========================================================================= */
/* Callbacks                                                                 */
/* ========================================================================= */

void
vitte_reset_set_callbacks(
    vitte_reset_t *reset,
    vitte_reset_callback_t before_callback,
    vitte_reset_callback_t after_callback,
    void *user_data);

/* ========================================================================= */
/* Execution                                                                 */
/* ========================================================================= */

/*
 * Execute configured reset policy.
 *
 * Sequence:
 *
 *     1. configuration/state checks
 *     2. optional pre-validation
 *     3. capture before statistics
 *     4. before callback
 *     5. reset pools
 *     6. reset arena
 *     7. optional post-validation
 *     8. capture after statistics
 *     9. after callback
 *    10. publish report/state
 *
 * IMPORTANT:
 *
 * This operation is NOT transactional.
 *
 * If a failure occurs after one or more targets were mutated, those mutations
 * are not rolled back.
 */
bool
vitte_reset_execute(
    vitte_reset_t *reset);

/* ========================================================================= */
/* Direct helpers                                                            */
/* ========================================================================= */

/*
 * Execute reset policy against one arena only.
 */
bool
vitte_reset_arena_only(
    vitte_arena_context_t *arena,
    vitte_reset_mode_t mode);

/*
 * Execute reset policy against one pool only.
 */
bool
vitte_reset_pool_only(
    vitte_pool_t *pool,
    vitte_reset_mode_t mode);

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

bool
vitte_reset_is_valid(
    const vitte_reset_t *reset);

bool
vitte_reset_is_running(
    const vitte_reset_t *reset);

bool
vitte_reset_succeeded(
    const vitte_reset_t *reset);

bool
vitte_reset_failed(
    const vitte_reset_t *reset);

vitte_reset_state_t
vitte_reset_state(
    const vitte_reset_t *reset);

/* ========================================================================= */
/* Mode                                                                      */
/* ========================================================================= */

bool
vitte_reset_set_mode(
    vitte_reset_t *reset,
    vitte_reset_mode_t mode);

vitte_reset_mode_t
vitte_reset_mode(
    const vitte_reset_t *reset);

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

vitte_reset_error_t
vitte_reset_last_error(
    const vitte_reset_t *reset);

void
vitte_reset_clear_error(
    vitte_reset_t *reset);

const char *
vitte_reset_error_name(
    vitte_reset_error_t error);

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_reset_mode_name(
    vitte_reset_mode_t mode);

const char *
vitte_reset_state_name(
    vitte_reset_state_t state);

/* ========================================================================= */
/* Report                                                                    */
/* ========================================================================= */

const vitte_reset_report_t *
vitte_reset_report(
    const vitte_reset_t *reset);

/* ========================================================================= */
/* Execution statistics                                                      */
/* ========================================================================= */

size_t
vitte_reset_execution_count(
    const vitte_reset_t *reset);

size_t
vitte_reset_successful_execution_count(
    const vitte_reset_t *reset);

size_t
vitte_reset_failed_execution_count(
    const vitte_reset_t *reset);

/* ========================================================================= */
/* Deep validation                                                          */
/* ========================================================================= */

/*
 * Validate coordinator structure.
 *
 * Checks include:
 *
 *     - integrity cookie
 *     - configuration
 *     - pool_count bounds
 *     - arena validity
 *     - pool validity
 *     - duplicate pool registrations
 *     - unused pool[] entries are NULL
 *     - execution counters
 *     - coordinator state
 *
 * This validates the reset coordinator itself.
 *
 * It does not automatically perform the expensive deep arena/pool validation
 * unless execution is configured with validate_before/validate_after.
 */
bool
vitte_reset_validate(
    const vitte_reset_t *reset);

/* ========================================================================= */
/* Convenience predicates                                                    */
/* ========================================================================= */

static inline bool
vitte_reset_has_error(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_last_error(reset) !=
        VITTE_RESET_ERROR_NONE;
}

static inline bool
vitte_reset_has_arena(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_is_valid(reset) &&
        reset->arena != NULL;
}

static inline bool
vitte_reset_has_pools(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_is_valid(reset) &&
        reset->pool_count != 0u;
}

static inline bool
vitte_reset_is_ready(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_is_valid(reset) &&
        reset->state ==
            VITTE_RESET_STATE_READY;
}

static inline bool
vitte_reset_completed(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_is_valid(reset) &&
        reset->state ==
            VITTE_RESET_STATE_COMPLETED;
}

static inline bool
vitte_reset_uses_retain(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_is_valid(reset) &&
        reset->config.mode ==
            VITTE_RESET_MODE_RETAIN;
}

static inline bool
vitte_reset_uses_trim(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_is_valid(reset) &&
        reset->config.mode ==
            VITTE_RESET_MODE_TRIM;
}

static inline bool
vitte_reset_uses_release(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_is_valid(reset) &&
        reset->config.mode ==
            VITTE_RESET_MODE_RELEASE;
}

/* ========================================================================= */
/* Report helpers                                                            */
/* ========================================================================= */

static inline size_t
vitte_reset_total_reserved_before(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    if (report->arena_reserved_before >
        SIZE_MAX -
        report->pool_reserved_before) {
        return SIZE_MAX;
    }

    return
        report->arena_reserved_before +
        report->pool_reserved_before;
}

static inline size_t
vitte_reset_total_reserved_after(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    if (report->arena_reserved_after >
        SIZE_MAX -
        report->pool_reserved_after) {
        return SIZE_MAX;
    }

    return
        report->arena_reserved_after +
        report->pool_reserved_after;
}

static inline size_t
vitte_reset_total_reserved_released(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    if (report->arena_reserved_released >
        SIZE_MAX -
        report->pool_reserved_released) {
        return SIZE_MAX;
    }

    return
        report->arena_reserved_released +
        report->pool_reserved_released;
}

static inline bool
vitte_reset_released_memory(
    const vitte_reset_report_t *report)
{
    return
        vitte_reset_total_reserved_released(
            report) != 0u;
}

/* ========================================================================= */
/* Execution convenience                                                     */
/* ========================================================================= */

static inline bool
vitte_reset_execute_retain(
    vitte_reset_t *reset)
{
    if (!vitte_reset_set_mode(
            reset,
            VITTE_RESET_MODE_RETAIN)) {
        return false;
    }

    return vitte_reset_execute(reset);
}

static inline bool
vitte_reset_execute_trim(
    vitte_reset_t *reset)
{
    if (!vitte_reset_set_mode(
            reset,
            VITTE_RESET_MODE_TRIM)) {
        return false;
    }

    return vitte_reset_execute(reset);
}

static inline bool
vitte_reset_execute_release(
    vitte_reset_t *reset)
{
    if (!vitte_reset_set_mode(
            reset,
            VITTE_RESET_MODE_RELEASE)) {
        return false;
    }

    return vitte_reset_execute(reset);
}

/* ========================================================================= */
/* Macros                                                                    */
/* ========================================================================= */

#define VITTE_RESET_ADD_POOL(reset, pool) \
    vitte_reset_add_pool( \
        (reset), \
        (pool))

#define VITTE_RESET_REMOVE_POOL(reset, pool) \
    vitte_reset_remove_pool( \
        (reset), \
        (pool))

#define VITTE_RESET_EXECUTE(reset) \
    vitte_reset_execute((reset))

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_RESET_MAX_POOLS != 0u,
    "reset coordinator must support at least one pool");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte reset engine requires 64-bit uint64_t");

_Static_assert(
    VITTE_RESET_MODE_RETAIN == 0,
    "retain reset mode must remain zero");

_Static_assert(
    VITTE_RESET_STATE_UNINITIALIZED == 0,
    "uninitialized reset state must remain zero");

_Static_assert(
    VITTE_RESET_ERROR_NONE == 0,
    "reset no-error value must remain zero");

_Static_assert(
    VITTE_RESET_CALLBACK_BEFORE == 0,
    "before callback phase must remain zero");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_RESET_H */
