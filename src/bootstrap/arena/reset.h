#ifndef VITTE_SRC_ARENA_RESET_H
#define VITTE_SRC_ARENA_RESET_H

/*
 * Vitte Compiler
 * src/arena/reset.h
 *
 * Coordinated arena/pool reset engine.
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
 *         reusable fixed-size object pools
 *
 *     reset.c/.h
 *         coordinated reset policy
 *
 * The reset coordinator owns no arena or pool memory.
 *
 * Registered targets are non-owning references. Destroying a coordinator
 * never destroys, resets or otherwise releases its registered targets.
 *
 * Responsibilities:
 *
 *     - reset policy
 *     - coordinated arena + pool reset
 *     - retain / trim / release modes
 *     - optional pre/post deep validation
 *     - deterministic target ordering
 *     - before/after callbacks
 *     - before/after statistics
 *     - released-memory accounting
 *     - generation tracking
 *     - partial-failure reporting
 *     - saturating lifetime execution counters
 *     - coordinator invariant validation
 *
 * Execution is intentionally not transactional across independent targets.
 * If a later target or callback fails, earlier reset operations are not
 * rolled back.
 *
 * Thread safety:
 *
 *     A vitte_reset_t and every target registered with it require external
 *     synchronization when accessed concurrently with mutation.
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
/* Configuration constants                                                   */
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
 * Physical retention policy applied uniformly to selected targets.
 */
typedef enum vitte_reset_mode {
    /*
     * Reset logical allocation state while retaining reusable physical
     * storage.
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
     * Reset logical allocation state, then trim unused physical storage.
     *
     * Arena:
     *
     *     vitte_arena_context_reset()
     *     vitte_arena_context_trim()
     *
     * Pools:
     *
     *     vitte_pool_reset()
     *     vitte_pool_trim()
     *
     * Pool trimming continues to observe each pool's retain_empty_pages
     * configuration.
     */
    VITTE_RESET_MODE_TRIM = 1,

    /*
     * Reset logical allocation state and aggressively release resettable
     * physical storage.
     *
     * Arena:
     *
     *     vitte_arena_context_reset_and_trim()
     *
     * Pools:
     *
     *     vitte_pool_reset_and_trim()
     *
     * For pools, reset_and_trim() overrides retain_empty_pages and releases
     * all resettable empty pages.
     */
    VITTE_RESET_MODE_RELEASE = 2
} vitte_reset_mode_t;

/* ========================================================================= */
/* Coordinator state                                                         */
/* ========================================================================= */

typedef enum vitte_reset_state {
    /*
     * Zero-initialized, uninitialized or otherwise non-live object.
     *
     * A live coordinator never intentionally remains in this state.
     */
    VITTE_RESET_STATE_UNINITIALIZED = 0,

    /*
     * Initialized and ready for its first execution.
     */
    VITTE_RESET_STATE_READY = 1,

    /*
     * An execution is currently in progress.
     *
     * Structural mutation of the coordinator is forbidden while RUNNING.
     */
    VITTE_RESET_STATE_RUNNING = 2,

    /*
     * Most recent execution completed successfully.
     */
    VITTE_RESET_STATE_COMPLETED = 3,

    /*
     * Most recent execution failed.
     *
     * Failure does not imply that no target changed. Reset execution is not
     * transactional and may fail after one or more targets were mutated.
     */
    VITTE_RESET_STATE_FAILED = 4,

    /*
     * Coordinator was explicitly destroyed.
     *
     * Its registered target references have been discarded, but the targets
     * themselves remain untouched.
     */
    VITTE_RESET_STATE_DESTROYED = 5
} vitte_reset_state_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_reset_error {
    /*
     * No reset-engine error.
     */
    VITTE_RESET_ERROR_NONE = 0,

    /*
     * Coordinator is NULL, uninitialized, destroyed or has an invalid
     * integrity cookie.
     */
    VITTE_RESET_ERROR_INVALID_RESET,

    /*
     * Invalid argument or invalid reset configuration.
     */
    VITTE_RESET_ERROR_INVALID_ARGUMENT,

    /*
     * Requested operation is forbidden in the current coordinator state.
     */
    VITTE_RESET_ERROR_INVALID_STATE,

    /*
     * Arena target is invalid.
     */
    VITTE_RESET_ERROR_INVALID_ARENA,

    /*
     * Pool target is invalid.
     */
    VITTE_RESET_ERROR_INVALID_POOL,

    /*
     * VITTE_RESET_MAX_POOLS registrations are already present.
     */
    VITTE_RESET_ERROR_TOO_MANY_POOLS,

    /*
     * The same pool pointer was registered more than once.
     */
    VITTE_RESET_ERROR_DUPLICATE_POOL,

    /*
     * Arena reset operation failed, left an arena-local error, violated the
     * expected generation transition, or failed to produce reset state.
     */
    VITTE_RESET_ERROR_ARENA_RESET_FAILED,

    /*
     * Pool reset operation failed, left a pool-local error, violated the
     * expected generation transition, or failed to produce reset state.
     */
    VITTE_RESET_ERROR_POOL_RESET_FAILED,

    /*
     * Requested pre/post deep validation failed.
     */
    VITTE_RESET_ERROR_VALIDATION_FAILED,

    /*
     * A before/after callback rejected execution by returning false.
     */
    VITTE_RESET_ERROR_CALLBACK_FAILED,

    /*
     * Arithmetic/statistical operation could not be represented exactly.
     *
     * Ordinary report aggregation uses saturation where documented. This
     * error value remains available for reset-engine operations for which
     * saturation would not preserve the required semantics.
     */
    VITTE_RESET_ERROR_OVERFLOW,

    /*
     * Internal reset-coordinator invariant failed.
     */
    VITTE_RESET_ERROR_CORRUPTION
} vitte_reset_error_t;

/* ========================================================================= */
/* Callback phase                                                            */
/* ========================================================================= */

typedef enum vitte_reset_callback_phase {
    /*
     * Called after target checks, optional pre-validation and before-statistic
     * capture, but before coordinator-driven target mutation.
     */
    VITTE_RESET_CALLBACK_BEFORE = 0,

    /*
     * Called after successful target reset, optional post-validation and
     * after-statistic capture.
     *
     * Returning false here marks the execution failed even though target
     * mutation has already completed.
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
 *         accept the current phase and continue
 *
 *     false
 *         fail execution with VITTE_RESET_ERROR_CALLBACK_FAILED
 *
 * user_data is the opaque value supplied to vitte_reset_set_callbacks().
 *
 * During a callback the coordinator is RUNNING.
 *
 * A callback must not:
 *
 *     - recursively execute the same coordinator
 *     - add/remove/clear registered pools
 *     - replace the arena target
 *     - replace callbacks
 *     - change reset mode
 *
 * Such operations are rejected by the public mutation APIs where those APIs
 * can report an error.
 *
 * A callback may inspect the coordinator and its report. Target mutation
 * performed directly by a callback is outside reset.c's transactional model
 * and remains the callback's responsibility.
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
     * Physical storage retention policy.
     */
    vitte_reset_mode_t mode;

    /*
     * Reset the registered arena when an arena is present.
     *
     * A NULL arena remains legal even when this field is true.
     */
    bool reset_arena;

    /*
     * Reset all registered pools.
     */
    bool reset_pools;

    /*
     * Perform expensive deep target validation before coordinator-driven
     * mutation.
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
     * Perform expensive deep target validation after coordinator-driven
     * reset operations and before the after callback.
     */
    bool validate_after;

    /*
     * Stop pool processing immediately after the first pool failure.
     *
     * true:
     *
     *     execution stops at the first failed pool and the arena is not reset
     *     afterward.
     *
     * false:
     *
     *     subsequent pools and, when safe, the arena are still processed.
     *     Overall execution nevertheless remains failed.
     */
    bool stop_on_pool_error;

    /*
     * Enable registered before/after callbacks.
     *
     * When false, callback pointers remain registered but are not invoked.
     */
    bool invoke_callbacks;
};

/* ========================================================================= */
/* Execution report                                                          */
/* ========================================================================= */

/*
 * Snapshot of the most recent execution attempt.
 *
 * Aggregate pool fields cover every registered pool that was valid and
 * observable at the corresponding snapshot point.
 *
 * Aggregate arithmetic saturates at SIZE_MAX.
 *
 * "released" fields use a non-negative difference:
 *
 *     before > after
 *         released = before - after
 *
 *     before <= after
 *         released = 0
 *
 * Therefore "released" means net reduction between snapshots, not a count of
 * all transient deallocations that may have occurred during execution.
 *
 * On a partial failure, before/after fields describe the observable state
 * around that failed execution where reset.c was able to obtain the
 * corresponding snapshots.
 */
struct vitte_reset_report {
    /*
     * True only when the complete execution, including the after callback,
     * completed successfully.
     */
    bool success;

    /*
     * Final reset-engine error associated with this execution.
     */
    vitte_reset_error_t error;

    /* --------------------------------------------------------------------- */
    /* Arena                                                                 */
    /* --------------------------------------------------------------------- */

    /*
     * Arena generation snapshots.
     *
     * Zero means no valid arena generation was captured for that side.
     */
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
    /* Pools — aggregate                                                     */
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
/* Reset coordinator                                                         */
/* ========================================================================= */

/*
 * Compiler-internal reset coordinator.
 *
 * Fields are visible because this subsystem is compiler-internal, but callers
 * should use the public API rather than mutating fields directly.
 *
 * Arena and pool pointers are non-owning.
 */
struct vitte_reset {
    /*
     * Runtime integrity cookie.
     */
    uint64_t magic;

    /*
     * Current coordinator state.
     */
    vitte_reset_state_t state;

    /*
     * Effective execution configuration.
     */
    vitte_reset_config_t config;

    /*
     * Optional non-owning arena target.
     */
    vitte_arena_context_t *arena;

    /*
     * Non-owning registered pool targets.
     *
     * In a valid coordinator:
     *
     *     pools[0 .. pool_count) != NULL
     *     pools[pool_count .. VITTE_RESET_MAX_POOLS) == NULL
     *
     * Registered entries are unique.
     */
    vitte_pool_t *pools[VITTE_RESET_MAX_POOLS];

    /*
     * Number of registered pool entries.
     */
    size_t pool_count;

    /*
     * Optional execution callbacks.
     */
    vitte_reset_callback_t before_callback;
    vitte_reset_callback_t after_callback;

    /*
     * Opaque callback value.
     */
    void *callback_user_data;

    /*
     * Lifetime number of execute() attempts accepted for execution.
     *
     * Saturates at SIZE_MAX.
     */
    size_t execution_count;

    /*
     * Lifetime number of accepted execute() attempts that completed
     * successfully.
     *
     * Saturates at SIZE_MAX.
     */
    size_t successful_execution_count;

    /*
     * Lifetime number of accepted execute() attempts that completed with
     * failure.
     *
     * Saturates at SIZE_MAX.
     */
    size_t failed_execution_count;

    /*
     * Last coordinator-local error.
     *
     * A successful execution clears this to VITTE_RESET_ERROR_NONE.
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
 * Return default reset configuration:
 *
 *     mode               = VITTE_RESET_MODE_RETAIN
 *     reset_arena        = true
 *     reset_pools        = true
 *     validate_before    = false
 *     validate_after     = false
 *     stop_on_pool_error = true
 *     invoke_callbacks   = true
 */
vitte_reset_config_t
vitte_reset_config_default(void);

/*
 * Validate reset configuration.
 *
 * Currently the mode enumeration is the only value requiring normalization-
 * independent validation. Boolean members are inherently valid.
 */
bool
vitte_reset_config_validate(
    const vitte_reset_config_t *config);

/* ========================================================================= */
/* Report initialization                                                     */
/* ========================================================================= */

/*
 * Initialize a report to deterministic empty state.
 *
 * The resulting report has:
 *
 *     success = false
 *     error   = VITTE_RESET_ERROR_NONE
 *
 * and all numeric counters set to zero.
 */
void
vitte_reset_report_init(
    vitte_reset_report_t *report);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize a reset coordinator.
 *
 * reset:
 *
 *     must be non-NULL.
 *
 * arena:
 *
 *     may be NULL.
 *
 *     when non-NULL, it must satisfy
 *     vitte_arena_context_is_valid().
 *
 * config:
 *
 *     NULL selects vitte_reset_config_default().
 *
 * Initialization does not mutate arena/pool target memory.
 *
 * The coordinator initially contains no registered pools.
 */
bool
vitte_reset_init(
    vitte_reset_t *reset,
    vitte_arena_context_t *arena,
    const vitte_reset_config_t *config);

/*
 * Initialize using vitte_reset_config_default().
 */
bool
vitte_reset_init_default(
    vitte_reset_t *reset,
    vitte_arena_context_t *arena);

/*
 * Destroy coordinator metadata.
 *
 * Does NOT:
 *
 *     - destroy the arena
 *     - destroy pools
 *     - reset targets
 *     - trim targets
 *
 * Registered references and callbacks are discarded.
 */
void
vitte_reset_destroy(
    vitte_reset_t *reset);

/* ========================================================================= */
/* Arena target                                                              */
/* ========================================================================= */

/*
 * Replace the optional arena target.
 *
 * arena == NULL removes the current arena registration.
 *
 * Rejected while RUNNING.
 */
bool
vitte_reset_set_arena(
    vitte_reset_t *reset,
    vitte_arena_context_t *arena);

/*
 * Return the registered arena or NULL.
 */
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
 * Register one non-owning pool target.
 *
 * Rejected when:
 *
 *     - coordinator is invalid
 *     - coordinator is RUNNING
 *     - pool is invalid
 *     - pool is already registered
 *     - VITTE_RESET_MAX_POOLS is reached
 */
bool
vitte_reset_add_pool(
    vitte_reset_t *reset,
    vitte_pool_t *pool);

/*
 * Remove one registered pool.
 *
 * Remaining entries are compacted while preserving registration order.
 *
 * Does not mutate the removed pool.
 *
 * Rejected while RUNNING.
 */
bool
vitte_reset_remove_pool(
    vitte_reset_t *reset,
    vitte_pool_t *pool);

/*
 * Remove all pool registrations.
 *
 * Does not mutate any pool.
 *
 * Rejected while RUNNING.
 */
void
vitte_reset_clear_pools(
    vitte_reset_t *reset);

/*
 * Return number of registered pools.
 */
size_t
vitte_reset_pool_count(
    const vitte_reset_t *reset);

/*
 * Return registered pool at index or NULL when unavailable/out of range.
 */
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

/*
 * Replace execution callbacks and callback user data.
 *
 * Passing NULL for either callback disables that phase.
 *
 * Rejected while RUNNING.
 */
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
 * Execute the configured reset policy.
 *
 * Normal successful sequence:
 *
 *      1. validate coordinator state/configuration
 *      2. validate target registration
 *      3. optional target pre-validation
 *      4. capture before statistics
 *      5. invoke before callback
 *      6. reset registered pools in registration order
 *      7. reset arena
 *      8. optional target post-validation
 *      9. capture after statistics
 *     10. invoke after callback
 *     11. publish successful report/state
 *
 * Pool ordering:
 *
 *     Pools are reset before the arena. This invalidates pooled object
 *     liveness before arena-backed data referenced by those objects becomes
 *     logically invalid.
 *
 * Failure:
 *
 *     Execution is NOT transactional.
 *
 *     If failure occurs after mutation begins, already-completed target
 *     mutations remain in effect.
 *
 *     With stop_on_pool_error == false, subsequent pools and the arena may
 *     still be processed after a pool failure, but the overall result remains
 *     failure.
 *
 * Reentrancy:
 *
 *     Calling execute() recursively on the same coordinator while RUNNING is
 *     rejected.
 */
bool
vitte_reset_execute(
    vitte_reset_t *reset);

/* ========================================================================= */
/* Direct helpers                                                            */
/* ========================================================================= */

/*
 * Execute one reset mode against a single arena.
 *
 * Uses a temporary coordinator with:
 *
 *     reset_arena = true
 *     reset_pools = false
 */
bool
vitte_reset_arena_only(
    vitte_arena_context_t *arena,
    vitte_reset_mode_t mode);

/*
 * Execute one reset mode against a single pool.
 *
 * Uses a temporary coordinator with:
 *
 *     reset_arena = false
 *     reset_pools = true
 */
bool
vitte_reset_pool_only(
    vitte_pool_t *pool,
    vitte_reset_mode_t mode);

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

/*
 * Lightweight coordinator validity check.
 *
 * This checks the reset integrity cookie.
 *
 * Use vitte_reset_validate() for structural validation.
 */
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

/*
 * Change reset mode.
 *
 * Rejected while RUNNING.
 */
bool
vitte_reset_set_mode(
    vitte_reset_t *reset,
    vitte_reset_mode_t mode);

/*
 * Return current reset mode.
 *
 * Invalid coordinators return VITTE_RESET_MODE_RETAIN as a deterministic
 * fallback.
 */
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

/*
 * Return the most recent report.
 *
 * The returned pointer refers to storage owned by the coordinator and remains
 * valid until the coordinator is destroyed.
 *
 * A subsequent execute() overwrites the report.
 */
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
 * Perform structural coordinator validation.
 *
 * Checks include:
 *
 *     - integrity cookie
 *     - configuration validity
 *     - coordinator state
 *     - pool_count bounds
 *     - arena lightweight validity
 *     - pool lightweight validity
 *     - duplicate pool registrations
 *     - unused pools[] entries are NULL
 *     - execution-counter consistency
 *     - report/state consistency
 *     - released-field arithmetic
 *     - successful reset-state invariants
 *     - successful arena generation transition
 *
 * Target deep validation:
 *
 *     This function validates coordinator relationships and selected
 *     postconditions, but does not unconditionally perform the full expensive
 *     vitte_arena_context_validate()/vitte_pool_validate() traversal.
 *
 *     Full target validation during execution is controlled by
 *     validate_before and validate_after.
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
        reset->pool_count != 0u &&
        reset->pool_count <=
            VITTE_RESET_MAX_POOLS;
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

static inline bool
vitte_reset_targets_arena(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_is_valid(reset) &&
        reset->config.reset_arena &&
        reset->arena != NULL;
}

static inline bool
vitte_reset_targets_pools(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_is_valid(reset) &&
        reset->config.reset_pools &&
        reset->pool_count != 0u &&
        reset->pool_count <=
            VITTE_RESET_MAX_POOLS;
}

static inline bool
vitte_reset_has_targets(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_targets_arena(reset) ||
        vitte_reset_targets_pools(reset);
}

/* ========================================================================= */
/* Report arithmetic helpers                                                 */
/* ========================================================================= */

static inline size_t
vitte_reset_report_saturating_add(
    size_t left,
    size_t right)
{
    if (left > SIZE_MAX - right) {
        return SIZE_MAX;
    }

    return left + right;
}

static inline size_t
vitte_reset_report_nonnegative_difference(
    size_t before,
    size_t after)
{
    if (before <= after) {
        return 0u;
    }

    return before - after;
}

/* ========================================================================= */
/* Report aggregate helpers                                                  */
/* ========================================================================= */

static inline size_t
vitte_reset_total_reserved_before(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    return
        vitte_reset_report_saturating_add(
            report->arena_reserved_before,
            report->pool_reserved_before);
}

static inline size_t
vitte_reset_total_reserved_after(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    return
        vitte_reset_report_saturating_add(
            report->arena_reserved_after,
            report->pool_reserved_after);
}

static inline size_t
vitte_reset_total_reserved_released(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    /*
     * Sum the independently measured target reductions.
     *
     * This is deliberately different from:
     *
     *     total_before - total_after
     *
     * because one target may grow while another shrinks.
     */
    return
        vitte_reset_report_saturating_add(
            report->arena_reserved_released,
            report->pool_reserved_released);
}

static inline size_t
vitte_reset_total_used_before(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    /*
     * Pool live count is an object count rather than a byte count and cannot
     * be added meaningfully to arena_used_before.
     */
    return report->arena_used_before;
}

static inline size_t
vitte_reset_total_used_after(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    return report->arena_used_after;
}

static inline size_t
vitte_reset_total_used_released(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    return report->arena_used_released;
}

static inline size_t
vitte_reset_total_live_objects_before(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    return report->pool_live_before;
}

static inline size_t
vitte_reset_total_live_objects_after(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    return report->pool_live_after;
}

static inline size_t
vitte_reset_total_live_objects_released(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return 0u;
    }

    return report->pool_live_released;
}

static inline bool
vitte_reset_released_memory(
    const vitte_reset_report_t *report)
{
    return
        report != NULL &&
        vitte_reset_total_reserved_released(
            report) != 0u;
}

static inline bool
vitte_reset_released_arena_memory(
    const vitte_reset_report_t *report)
{
    return
        report != NULL &&
        report->arena_reserved_released != 0u;
}

static inline bool
vitte_reset_released_pool_memory(
    const vitte_reset_report_t *report)
{
    return
        report != NULL &&
        report->pool_reserved_released != 0u;
}

static inline bool
vitte_reset_released_pool_pages(
    const vitte_reset_report_t *report)
{
    return
        report != NULL &&
        report->pool_pages_released != 0u;
}

/* ========================================================================= */
/* Report consistency helpers                                                */
/* ========================================================================= */

static inline bool
vitte_reset_report_arena_released_consistent(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return false;
    }

    return
        report->arena_reserved_released ==
            vitte_reset_report_nonnegative_difference(
                report->arena_reserved_before,
                report->arena_reserved_after) &&
        report->arena_used_released ==
            vitte_reset_report_nonnegative_difference(
                report->arena_used_before,
                report->arena_used_after) &&
        report->arena_requested_released ==
            vitte_reset_report_nonnegative_difference(
                report->arena_requested_before,
                report->arena_requested_after);
}

static inline bool
vitte_reset_report_pool_released_consistent(
    const vitte_reset_report_t *report)
{
    if (report == NULL) {
        return false;
    }

    return
        report->pool_capacity_released ==
            vitte_reset_report_nonnegative_difference(
                report->pool_capacity_before,
                report->pool_capacity_after) &&
        report->pool_live_released ==
            vitte_reset_report_nonnegative_difference(
                report->pool_live_before,
                report->pool_live_after) &&
        report->pool_reserved_released ==
            vitte_reset_report_nonnegative_difference(
                report->pool_reserved_before,
                report->pool_reserved_after) &&
        report->pool_pages_released ==
            vitte_reset_report_nonnegative_difference(
                report->pool_pages_before,
                report->pool_pages_after);
}

static inline bool
vitte_reset_report_released_consistent(
    const vitte_reset_report_t *report)
{
    return
        vitte_reset_report_arena_released_consistent(
            report) &&
        vitte_reset_report_pool_released_consistent(
            report);
}

/* ========================================================================= */
/* Generation helpers                                                        */
/* ========================================================================= */

/*
 * Return the generation expected after one reset operation.
 *
 * Generation zero is reserved. UINT64_MAX wraps to one.
 */
static inline uint64_t
vitte_reset_next_generation(
    uint64_t generation)
{
    if (generation == UINT64_C(0) ||
        generation == UINT64_MAX) {
        return UINT64_C(1);
    }

    return generation + UINT64_C(1);
}

static inline bool
vitte_reset_report_arena_generation_advanced(
    const vitte_reset_report_t *report)
{
    if (report == NULL ||
        report->arena_generation_before ==
            UINT64_C(0) ||
        report->arena_generation_after ==
            UINT64_C(0)) {
        return false;
    }

    return
        report->arena_generation_after ==
        vitte_reset_next_generation(
            report->arena_generation_before);
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

    return
        vitte_reset_execute(reset);
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

    return
        vitte_reset_execute(reset);
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

    return
        vitte_reset_execute(reset);
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

#if defined(__cplusplus)

static_assert(
    VITTE_RESET_MAX_POOLS != 0u,
    "reset coordinator must support at least one pool");

static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte reset engine requires 64-bit uint64_t");

static_assert(
    VITTE_RESET_MODE_RETAIN == 0,
    "retain reset mode must remain zero");

static_assert(
    VITTE_RESET_STATE_UNINITIALIZED == 0,
    "uninitialized reset state must remain zero");

static_assert(
    VITTE_RESET_ERROR_NONE == 0,
    "reset no-error value must remain zero");

static_assert(
    VITTE_RESET_CALLBACK_BEFORE == 0,
    "before callback phase must remain zero");

#elif defined(__STDC_VERSION__) && \
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