/*
 * Vitte Compiler
 * src/arena/reset.c
 *
 * Coordinated arena/pool reset engine.
 *
 * ISO C17.
 *
 * This module centralizes reset policy while leaving primitive allocator,
 * arena and pool operations in their respective subsystems.
 *
 * Properties:
 *
 *     - deterministic target ordering
 *     - coordinated arena/pool reset
 *     - retain / trim / release policies
 *     - optional pre/post deep validation
 *     - callbacks
 *     - generation verification
 *     - deterministic reports
 *     - saturating lifetime counters
 *     - partial-failure reporting
 *     - defensive structural validation
 *
 * The reset coordinator owns no arena or pool memory.
 *
 * Thread safety:
 *
 *     A reset coordinator and all registered targets require external
 *     synchronization when accessed concurrently with mutation.
 */

#include "reset.h"

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

#ifndef VITTE_RESET_MAGIC
#define VITTE_RESET_MAGIC \
    UINT64_C(0x5649545445525354)
#endif

#ifndef VITTE_RESET_DEAD_MAGIC
#define VITTE_RESET_DEAD_MAGIC \
    UINT64_C(0x4445414452534554)
#endif

#ifndef VITTE_RESET_MAX_POOLS
#define VITTE_RESET_MAX_POOLS \
    ((size_t)1024u)
#endif

/* ========================================================================= */
/* Arithmetic                                                                */
/* ========================================================================= */

static bool
vitte_reset_add_overflow(
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
vitte_reset_saturating_add(
    size_t left,
    size_t right)
{
    size_t result;

    if (vitte_reset_add_overflow(
            left,
            right,
            &result)) {
        return SIZE_MAX;
    }

    return result;
}

static void
vitte_reset_saturating_increment(
    size_t *value)
{
    if (value == NULL) {
        return;
    }

    if (*value != SIZE_MAX) {
        ++(*value);
    }
}

static size_t
vitte_reset_nonnegative_difference(
    size_t before,
    size_t after)
{
    if (before <= after) {
        return 0u;
    }

    return before - after;
}

static uint64_t
vitte_reset_expected_next_generation(
    uint64_t generation)
{
    if (generation == UINT64_MAX ||
        generation == UINT64_C(0)) {
        return UINT64_C(1);
    }

    return generation + UINT64_C(1);
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_reset_mode_name(
    vitte_reset_mode_t mode)
{
    switch (mode) {
        case VITTE_RESET_MODE_RETAIN:
            return "retain";

        case VITTE_RESET_MODE_TRIM:
            return "trim";

        case VITTE_RESET_MODE_RELEASE:
            return "release";

        default:
            return "unknown";
    }
}

const char *
vitte_reset_state_name(
    vitte_reset_state_t state)
{
    switch (state) {
        case VITTE_RESET_STATE_UNINITIALIZED:
            return "uninitialized";

        case VITTE_RESET_STATE_READY:
            return "ready";

        case VITTE_RESET_STATE_RUNNING:
            return "running";

        case VITTE_RESET_STATE_COMPLETED:
            return "completed";

        case VITTE_RESET_STATE_FAILED:
            return "failed";

        case VITTE_RESET_STATE_DESTROYED:
            return "destroyed";

        default:
            return "unknown";
    }
}

const char *
vitte_reset_error_name(
    vitte_reset_error_t error)
{
    switch (error) {
        case VITTE_RESET_ERROR_NONE:
            return "none";

        case VITTE_RESET_ERROR_INVALID_RESET:
            return "invalid-reset";

        case VITTE_RESET_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_RESET_ERROR_INVALID_STATE:
            return "invalid-state";

        case VITTE_RESET_ERROR_INVALID_ARENA:
            return "invalid-arena";

        case VITTE_RESET_ERROR_INVALID_POOL:
            return "invalid-pool";

        case VITTE_RESET_ERROR_TOO_MANY_POOLS:
            return "too-many-pools";

        case VITTE_RESET_ERROR_DUPLICATE_POOL:
            return "duplicate-pool";

        case VITTE_RESET_ERROR_ARENA_RESET_FAILED:
            return "arena-reset-failed";

        case VITTE_RESET_ERROR_POOL_RESET_FAILED:
            return "pool-reset-failed";

        case VITTE_RESET_ERROR_VALIDATION_FAILED:
            return "validation-failed";

        case VITTE_RESET_ERROR_CALLBACK_FAILED:
            return "callback-failed";

        case VITTE_RESET_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_RESET_ERROR_CORRUPTION:
            return "corruption";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Internal validity                                                         */
/* ========================================================================= */

static bool
vitte_reset_magic_valid(
    const vitte_reset_t *reset)
{
    return
        reset != NULL &&
        reset->magic == VITTE_RESET_MAGIC;
}

static bool
vitte_reset_state_operable(
    const vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return false;
    }

    switch (reset->state) {
        case VITTE_RESET_STATE_READY:
        case VITTE_RESET_STATE_COMPLETED:
        case VITTE_RESET_STATE_FAILED:
            return true;

        case VITTE_RESET_STATE_RUNNING:
        case VITTE_RESET_STATE_UNINITIALIZED:
        case VITTE_RESET_STATE_DESTROYED:
        default:
            return false;
    }
}

static bool
vitte_reset_pool_registered(
    const vitte_reset_t *reset,
    const vitte_pool_t *pool)
{
    size_t index;

    if (!vitte_reset_magic_valid(reset) ||
        pool == NULL) {
        return false;
    }

    if (reset->pool_count >
        VITTE_RESET_MAX_POOLS) {
        return false;
    }

    for (index = 0u;
         index < reset->pool_count;
         ++index) {
        if (reset->pools[index] == pool) {
            return true;
        }
    }

    return false;
}

/* ========================================================================= */
/* Error handling                                                            */
/* ========================================================================= */

static void
vitte_reset_set_error(
    vitte_reset_t *reset,
    vitte_reset_error_t error)
{
    if (!vitte_reset_magic_valid(reset)) {
        return;
    }

    reset->last_error = error;
}

vitte_reset_error_t
vitte_reset_last_error(
    const vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return VITTE_RESET_ERROR_INVALID_RESET;
    }

    return reset->last_error;
}

void
vitte_reset_clear_error(
    vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return;
    }

    reset->last_error =
        VITTE_RESET_ERROR_NONE;
}

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_reset_config_t
vitte_reset_config_default(void)
{
    vitte_reset_config_t config;

    memset(
        &config,
        0,
        sizeof(config));

    config.mode =
        VITTE_RESET_MODE_RETAIN;

    config.reset_arena = true;
    config.reset_pools = true;

    config.validate_before = false;
    config.validate_after = false;

    config.stop_on_pool_error = true;

    config.invoke_callbacks = true;

    return config;
}

bool
vitte_reset_config_validate(
    const vitte_reset_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    switch (config->mode) {
        case VITTE_RESET_MODE_RETAIN:
        case VITTE_RESET_MODE_TRIM:
        case VITTE_RESET_MODE_RELEASE:
            return true;

        default:
            return false;
    }
}

/* ========================================================================= */
/* Report                                                                    */
/* ========================================================================= */

void
vitte_reset_report_init(
    vitte_reset_report_t *report)
{
    if (report == NULL) {
        return;
    }

    memset(
        report,
        0,
        sizeof(*report));

    report->success = false;
    report->error =
        VITTE_RESET_ERROR_NONE;
}

static void
vitte_reset_capture_arena_before(
    vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset) ||
        reset->arena == NULL ||
        !vitte_arena_context_is_valid(
            reset->arena)) {
        return;
    }

    reset->report.arena_generation_before =
        vitte_arena_context_generation(
            reset->arena);

    reset->report.arena_reserved_before =
        vitte_arena_context_reserved_bytes(
            reset->arena);

    reset->report.arena_used_before =
        vitte_arena_context_used_bytes(
            reset->arena);

    reset->report.arena_requested_before =
        vitte_arena_context_requested_bytes(
            reset->arena);

    reset->report.arena_allocations_before =
        vitte_arena_context_allocation_count(
            reset->arena);
}

static void
vitte_reset_capture_arena_after(
    vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset) ||
        reset->arena == NULL ||
        !vitte_arena_context_is_valid(
            reset->arena)) {
        return;
    }

    reset->report.arena_generation_after =
        vitte_arena_context_generation(
            reset->arena);

    reset->report.arena_reserved_after =
        vitte_arena_context_reserved_bytes(
            reset->arena);

    reset->report.arena_used_after =
        vitte_arena_context_used_bytes(
            reset->arena);

    reset->report.arena_requested_after =
        vitte_arena_context_requested_bytes(
            reset->arena);

    reset->report.arena_allocations_after =
        vitte_arena_context_allocation_count(
            reset->arena);

    reset->report.arena_reserved_released =
        vitte_reset_nonnegative_difference(
            reset->report.arena_reserved_before,
            reset->report.arena_reserved_after);

    reset->report.arena_used_released =
        vitte_reset_nonnegative_difference(
            reset->report.arena_used_before,
            reset->report.arena_used_after);

    reset->report.arena_requested_released =
        vitte_reset_nonnegative_difference(
            reset->report.arena_requested_before,
            reset->report.arena_requested_after);
}

/* ========================================================================= */
/* Pool report                                                               */
/* ========================================================================= */

static void
vitte_reset_capture_pools_before(
    vitte_reset_t *reset)
{
    size_t index;

    if (!vitte_reset_magic_valid(reset) ||
        reset->pool_count >
            VITTE_RESET_MAX_POOLS) {
        return;
    }

    for (index = 0u;
         index < reset->pool_count;
         ++index) {
        const vitte_pool_t *pool;
        vitte_pool_stats_t stats;

        pool = reset->pools[index];

        if (pool == NULL ||
            !vitte_pool_is_valid(pool)) {
            continue;
        }

        stats =
            vitte_pool_stats(pool);

        reset->report.pool_capacity_before =
            vitte_reset_saturating_add(
                reset->report.pool_capacity_before,
                stats.capacity);

        reset->report.pool_live_before =
            vitte_reset_saturating_add(
                reset->report.pool_live_before,
                stats.live_count);

        reset->report.pool_reserved_before =
            vitte_reset_saturating_add(
                reset->report.pool_reserved_before,
                stats.reserved_bytes);

        reset->report.pool_pages_before =
            vitte_reset_saturating_add(
                reset->report.pool_pages_before,
                stats.page_count);
    }
}

static void
vitte_reset_capture_pools_after(
    vitte_reset_t *reset)
{
    size_t index;

    if (!vitte_reset_magic_valid(reset) ||
        reset->pool_count >
            VITTE_RESET_MAX_POOLS) {
        return;
    }

    for (index = 0u;
         index < reset->pool_count;
         ++index) {
        const vitte_pool_t *pool;
        vitte_pool_stats_t stats;

        pool = reset->pools[index];

        if (pool == NULL ||
            !vitte_pool_is_valid(pool)) {
            continue;
        }

        stats =
            vitte_pool_stats(pool);

        reset->report.pool_capacity_after =
            vitte_reset_saturating_add(
                reset->report.pool_capacity_after,
                stats.capacity);

        reset->report.pool_live_after =
            vitte_reset_saturating_add(
                reset->report.pool_live_after,
                stats.live_count);

        reset->report.pool_reserved_after =
            vitte_reset_saturating_add(
                reset->report.pool_reserved_after,
                stats.reserved_bytes);

        reset->report.pool_pages_after =
            vitte_reset_saturating_add(
                reset->report.pool_pages_after,
                stats.page_count);
    }

    reset->report.pool_capacity_released =
        vitte_reset_nonnegative_difference(
            reset->report.pool_capacity_before,
            reset->report.pool_capacity_after);

    reset->report.pool_live_released =
        vitte_reset_nonnegative_difference(
            reset->report.pool_live_before,
            reset->report.pool_live_after);

    reset->report.pool_reserved_released =
        vitte_reset_nonnegative_difference(
            reset->report.pool_reserved_before,
            reset->report.pool_reserved_after);

    reset->report.pool_pages_released =
        vitte_reset_nonnegative_difference(
            reset->report.pool_pages_before,
            reset->report.pool_pages_after);
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_reset_init(
    vitte_reset_t *reset,
    vitte_arena_context_t *arena,
    const vitte_reset_config_t *config)
{
    vitte_reset_config_t effective;

    if (reset == NULL) {
        return false;
    }

    /*
     * A live reset coordinator owns no external memory, but silently
     * reinitializing it would discard its registration/configuration state.
     * Callers are expected to destroy/reinitialize deliberately.
     *
     * We cannot safely inspect arbitrary uninitialized storage before
     * initializing it, therefore initialization starts by clearing storage.
     */
    memset(
        reset,
        0,
        sizeof(*reset));

    if (config == NULL) {
        effective =
            vitte_reset_config_default();
    } else {
        effective = *config;
    }

    if (!vitte_reset_config_validate(
            &effective)) {
        return false;
    }

    if (arena != NULL &&
        !vitte_arena_context_is_valid(
            arena)) {
        return false;
    }

    reset->magic =
        VITTE_RESET_MAGIC;

    reset->state =
        VITTE_RESET_STATE_READY;

    reset->config =
        effective;

    reset->arena = arena;

    reset->pool_count = 0u;

    reset->before_callback = NULL;
    reset->after_callback = NULL;
    reset->callback_user_data = NULL;

    reset->execution_count = 0u;
    reset->successful_execution_count = 0u;
    reset->failed_execution_count = 0u;

    reset->last_error =
        VITTE_RESET_ERROR_NONE;

    vitte_reset_report_init(
        &reset->report);

    return true;
}

bool
vitte_reset_init_default(
    vitte_reset_t *reset,
    vitte_arena_context_t *arena)
{
    vitte_reset_config_t config;

    config =
        vitte_reset_config_default();

    return
        vitte_reset_init(
            reset,
            arena,
            &config);
}

void
vitte_reset_destroy(
    vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return;
    }

    /*
     * No registered target is owned by the coordinator.
     */
    reset->arena = NULL;

    memset(
        reset->pools,
        0,
        sizeof(reset->pools));

    reset->pool_count = 0u;

    reset->before_callback = NULL;
    reset->after_callback = NULL;
    reset->callback_user_data = NULL;

    reset->state =
        VITTE_RESET_STATE_DESTROYED;

    reset->last_error =
        VITTE_RESET_ERROR_INVALID_RESET;

    reset->magic =
        VITTE_RESET_DEAD_MAGIC;
}

/* ========================================================================= */
/* Arena registration                                                        */
/* ========================================================================= */

bool
vitte_reset_set_arena(
    vitte_reset_t *reset,
    vitte_arena_context_t *arena)
{
    if (!vitte_reset_magic_valid(reset)) {
        return false;
    }

    if (reset->state ==
        VITTE_RESET_STATE_RUNNING) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_STATE);

        return false;
    }

    if (arena != NULL &&
        !vitte_arena_context_is_valid(
            arena)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_ARENA);

        return false;
    }

    reset->arena = arena;

    reset->last_error =
        VITTE_RESET_ERROR_NONE;

    return true;
}

vitte_arena_context_t *
vitte_reset_arena(
    vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return NULL;
    }

    return reset->arena;
}

const vitte_arena_context_t *
vitte_reset_arena_const(
    const vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return NULL;
    }

    return reset->arena;
}

/* ========================================================================= */
/* Pool registration                                                         */
/* ========================================================================= */

bool
vitte_reset_add_pool(
    vitte_reset_t *reset,
    vitte_pool_t *pool)
{
    if (!vitte_reset_magic_valid(reset)) {
        return false;
    }

    if (reset->state ==
        VITTE_RESET_STATE_RUNNING) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_STATE);

        return false;
    }

    if (pool == NULL ||
        !vitte_pool_is_valid(pool)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_POOL);

        return false;
    }

    if (reset->pool_count >
        VITTE_RESET_MAX_POOLS) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_CORRUPTION);

        return false;
    }

    if (vitte_reset_pool_registered(
            reset,
            pool)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_DUPLICATE_POOL);

        return false;
    }

    if (reset->pool_count ==
        VITTE_RESET_MAX_POOLS) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_TOO_MANY_POOLS);

        return false;
    }

    reset->pools[reset->pool_count] =
        pool;

    ++reset->pool_count;

    reset->last_error =
        VITTE_RESET_ERROR_NONE;

    return true;
}

bool
vitte_reset_remove_pool(
    vitte_reset_t *reset,
    vitte_pool_t *pool)
{
    size_t index;

    if (!vitte_reset_magic_valid(reset)) {
        return false;
    }

    if (reset->state ==
        VITTE_RESET_STATE_RUNNING) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_STATE);

        return false;
    }

    if (pool == NULL) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_ARGUMENT);

        return false;
    }

    if (reset->pool_count >
        VITTE_RESET_MAX_POOLS) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_CORRUPTION);

        return false;
    }

    for (index = 0u;
         index < reset->pool_count;
         ++index) {
        if (reset->pools[index] == pool) {
            size_t remaining;

            remaining =
                reset->pool_count -
                index -
                1u;

            if (remaining != 0u) {
                memmove(
                    &reset->pools[index],
                    &reset->pools[index + 1u],
                    remaining *
                        sizeof(reset->pools[0]));
            }

            --reset->pool_count;

            reset->pools[reset->pool_count] =
                NULL;

            reset->last_error =
                VITTE_RESET_ERROR_NONE;

            return true;
        }
    }

    vitte_reset_set_error(
        reset,
        VITTE_RESET_ERROR_INVALID_ARGUMENT);

    return false;
}

void
vitte_reset_clear_pools(
    vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return;
    }

    if (reset->state ==
        VITTE_RESET_STATE_RUNNING) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_STATE);

        return;
    }

    memset(
        reset->pools,
        0,
        sizeof(reset->pools));

    reset->pool_count = 0u;

    reset->last_error =
        VITTE_RESET_ERROR_NONE;
}

size_t
vitte_reset_pool_count(
    const vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset) ||
        reset->pool_count >
            VITTE_RESET_MAX_POOLS) {
        return 0u;
    }

    return reset->pool_count;
}

vitte_pool_t *
vitte_reset_pool_at(
    vitte_reset_t *reset,
    size_t index)
{
    if (!vitte_reset_magic_valid(reset) ||
        reset->pool_count >
            VITTE_RESET_MAX_POOLS ||
        index >= reset->pool_count) {
        return NULL;
    }

    return reset->pools[index];
}

const vitte_pool_t *
vitte_reset_pool_at_const(
    const vitte_reset_t *reset,
    size_t index)
{
    if (!vitte_reset_magic_valid(reset) ||
        reset->pool_count >
            VITTE_RESET_MAX_POOLS ||
        index >= reset->pool_count) {
        return NULL;
    }

    return reset->pools[index];
}

/* ========================================================================= */
/* Callbacks                                                                 */
/* ========================================================================= */

void
vitte_reset_set_callbacks(
    vitte_reset_t *reset,
    vitte_reset_callback_t before_callback,
    vitte_reset_callback_t after_callback,
    void *user_data)
{
    if (!vitte_reset_magic_valid(reset)) {
        return;
    }

    if (reset->state ==
        VITTE_RESET_STATE_RUNNING) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_STATE);

        return;
    }

    reset->before_callback =
        before_callback;

    reset->after_callback =
        after_callback;

    reset->callback_user_data =
        user_data;

    reset->last_error =
        VITTE_RESET_ERROR_NONE;
}

/* ========================================================================= */
/* Validation helpers                                                        */
/* ========================================================================= */

static bool
vitte_reset_validate_target_registration(
    vitte_reset_t *reset)
{
    size_t left;
    size_t right;

    if (!vitte_reset_magic_valid(reset)) {
        return false;
    }

    if (reset->pool_count >
        VITTE_RESET_MAX_POOLS) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_CORRUPTION);

        return false;
    }

    for (left = 0u;
         left < reset->pool_count;
         ++left) {
        if (reset->pools[left] == NULL) {
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_INVALID_POOL);

            return false;
        }

        for (right = left + 1u;
             right < reset->pool_count;
             ++right) {
            if (reset->pools[left] ==
                reset->pools[right]) {
                vitte_reset_set_error(
                    reset,
                    VITTE_RESET_ERROR_DUPLICATE_POOL);

                return false;
            }
        }
    }

    return true;
}

static bool
vitte_reset_validate_targets(
    vitte_reset_t *reset)
{
    size_t index;

    if (!vitte_reset_validate_target_registration(
            reset)) {
        return false;
    }

    if (reset->config.reset_arena &&
        reset->arena != NULL) {
        if (!vitte_arena_context_is_valid(
                reset->arena)) {
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_INVALID_ARENA);

            return false;
        }

        if (reset->config.validate_before &&
            !vitte_arena_context_validate(
                reset->arena)) {
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_VALIDATION_FAILED);

            return false;
        }
    }

    if (!reset->config.reset_pools) {
        return true;
    }

    for (index = 0u;
         index < reset->pool_count;
         ++index) {
        vitte_pool_t *pool;

        pool = reset->pools[index];

        if (pool == NULL ||
            !vitte_pool_is_valid(pool)) {
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_INVALID_POOL);

            return false;
        }

        if (reset->config.validate_before &&
            !vitte_pool_validate(pool)) {
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_VALIDATION_FAILED);

            return false;
        }
    }

    return true;
}

static bool
vitte_reset_validate_after(
    vitte_reset_t *reset)
{
    size_t index;

    if (!reset->config.validate_after) {
        return true;
    }

    if (reset->config.reset_arena &&
        reset->arena != NULL) {
        if (!vitte_arena_context_is_valid(
                reset->arena) ||
            !vitte_arena_context_validate(
                reset->arena)) {
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_VALIDATION_FAILED);

            return false;
        }
    }

    if (!reset->config.reset_pools) {
        return true;
    }

    for (index = 0u;
         index < reset->pool_count;
         ++index) {
        vitte_pool_t *pool;

        pool = reset->pools[index];

        if (pool == NULL ||
            !vitte_pool_is_valid(pool) ||
            !vitte_pool_validate(pool)) {
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_VALIDATION_FAILED);

            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Arena execution                                                           */
/* ========================================================================= */

static bool
vitte_reset_execute_arena(
    vitte_reset_t *reset)
{
    uint64_t generation_before;
    uint64_t generation_after;
    uint64_t expected_generation;

    if (!reset->config.reset_arena ||
        reset->arena == NULL) {
        return true;
    }

    if (!vitte_arena_context_is_valid(
            reset->arena)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_ARENA);

        return false;
    }

    generation_before =
        vitte_arena_context_generation(
            reset->arena);

    if (generation_before == UINT64_C(0)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_CORRUPTION);

        return false;
    }

    expected_generation =
        vitte_reset_expected_next_generation(
            generation_before);

    switch (reset->config.mode) {
        case VITTE_RESET_MODE_RETAIN:
            vitte_arena_context_reset(
                reset->arena);
            break;

        case VITTE_RESET_MODE_TRIM:
            vitte_arena_context_reset(
                reset->arena);

            if (!vitte_arena_context_is_valid(
                    reset->arena) ||
                vitte_arena_context_last_error(
                    reset->arena) !=
                    VITTE_ARENA_CONTEXT_ERROR_NONE) {
                vitte_reset_set_error(
                    reset,
                    VITTE_RESET_ERROR_ARENA_RESET_FAILED);

                return false;
            }

            (void)vitte_arena_context_trim(
                reset->arena);
            break;

        case VITTE_RESET_MODE_RELEASE:
            vitte_arena_context_reset_and_trim(
                reset->arena);
            break;

        default:
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_CORRUPTION);

            return false;
    }

    if (!vitte_arena_context_is_valid(
            reset->arena)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_ARENA_RESET_FAILED);

        return false;
    }

    if (vitte_arena_context_last_error(
            reset->arena) !=
        VITTE_ARENA_CONTEXT_ERROR_NONE) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_ARENA_RESET_FAILED);

        return false;
    }

    generation_after =
        vitte_arena_context_generation(
            reset->arena);

    if (generation_after !=
        expected_generation) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_ARENA_RESET_FAILED);

        return false;
    }

    if (vitte_arena_context_allocation_count(
            reset->arena) != 0u ||
        vitte_arena_context_requested_bytes(
            reset->arena) != 0u ||
        vitte_arena_context_used_bytes(
            reset->arena) != 0u) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_ARENA_RESET_FAILED);

        return false;
    }

    return true;
}

/* ========================================================================= */
/* Pool execution                                                            */
/* ========================================================================= */

static bool
vitte_reset_execute_one_pool(
    vitte_reset_t *reset,
    vitte_pool_t *pool)
{
    uint64_t generation_before;
    uint64_t generation_after;
    uint64_t expected_generation;

    if (pool == NULL ||
        !vitte_pool_is_valid(pool)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_POOL);

        return false;
    }

    generation_before =
        vitte_pool_generation(pool);

    if (generation_before == UINT64_C(0)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_CORRUPTION);

        return false;
    }

    expected_generation =
        vitte_reset_expected_next_generation(
            generation_before);

    switch (reset->config.mode) {
        case VITTE_RESET_MODE_RETAIN:
            vitte_pool_reset(pool);
            break;

        case VITTE_RESET_MODE_TRIM:
            vitte_pool_reset(pool);

            if (!vitte_pool_is_valid(pool) ||
                vitte_pool_last_error(pool) !=
                    VITTE_POOL_ERROR_NONE) {
                vitte_reset_set_error(
                    reset,
                    VITTE_RESET_ERROR_POOL_RESET_FAILED);

                return false;
            }

            (void)vitte_pool_trim(pool);
            break;

        case VITTE_RESET_MODE_RELEASE:
            vitte_pool_reset_and_trim(pool);
            break;

        default:
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_CORRUPTION);

            return false;
    }

    if (!vitte_pool_is_valid(pool) ||
        vitte_pool_last_error(pool) !=
            VITTE_POOL_ERROR_NONE) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_POOL_RESET_FAILED);

        return false;
    }

    generation_after =
        vitte_pool_generation(pool);

    if (generation_after !=
        expected_generation) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_POOL_RESET_FAILED);

        return false;
    }

    if (vitte_pool_live_count(pool) != 0u) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_POOL_RESET_FAILED);

        return false;
    }

    if (vitte_pool_free_count(pool) !=
        vitte_pool_capacity(pool)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_POOL_RESET_FAILED);

        return false;
    }

    if (reset->config.mode ==
            VITTE_RESET_MODE_RELEASE &&
        (vitte_pool_capacity(pool) != 0u ||
         vitte_pool_page_count(pool) != 0u ||
         vitte_pool_reserved_bytes(pool) != 0u)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_POOL_RESET_FAILED);

        return false;
    }

    return true;
}

static bool
vitte_reset_execute_pools(
    vitte_reset_t *reset)
{
    size_t index;
    bool success;
    vitte_reset_error_t first_error;

    if (!reset->config.reset_pools) {
        return true;
    }

    success = true;
    first_error =
        VITTE_RESET_ERROR_NONE;

    for (index = 0u;
         index < reset->pool_count;
         ++index) {
        if (!vitte_reset_execute_one_pool(
                reset,
                reset->pools[index])) {
            success = false;

            if (first_error ==
                VITTE_RESET_ERROR_NONE) {
                first_error =
                    reset->last_error;
            }

            if (reset->config.stop_on_pool_error) {
                return false;
            }
        }
    }

    if (!success) {
        reset->last_error =
            first_error !=
                VITTE_RESET_ERROR_NONE
            ? first_error
            : VITTE_RESET_ERROR_POOL_RESET_FAILED;
    }

    return success;
}

/* ========================================================================= */
/* Execution                                                                 */
/* ========================================================================= */

static void
vitte_reset_mark_failed(
    vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return;
    }

    reset->state =
        VITTE_RESET_STATE_FAILED;

    reset->report.success = false;

    if (reset->last_error ==
        VITTE_RESET_ERROR_NONE) {
        reset->last_error =
            VITTE_RESET_ERROR_CORRUPTION;
    }

    reset->report.error =
        reset->last_error;

    vitte_reset_saturating_increment(
        &reset->failed_execution_count);
}

bool
vitte_reset_execute(
    vitte_reset_t *reset)
{
    bool pools_success;
    bool mutation_started;

    if (!vitte_reset_magic_valid(reset)) {
        return false;
    }

    if (reset->state ==
        VITTE_RESET_STATE_RUNNING) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_STATE);

        return false;
    }

    if (!vitte_reset_state_operable(reset)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_STATE);

        return false;
    }

    if (!vitte_reset_config_validate(
            &reset->config)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_ARGUMENT);

        reset->state =
            VITTE_RESET_STATE_FAILED;

        return false;
    }

    vitte_reset_report_init(
        &reset->report);

    reset->last_error =
        VITTE_RESET_ERROR_NONE;

    reset->state =
        VITTE_RESET_STATE_RUNNING;

    vitte_reset_saturating_increment(
        &reset->execution_count);

    mutation_started = false;

    /*
     * Registration and optional deep pre-validation happen before snapshots
     * and callbacks so a structurally invalid target cannot enter a reset
     * transaction.
     */
    if (!vitte_reset_validate_targets(
            reset)) {
        vitte_reset_mark_failed(reset);
        return false;
    }

    /*
     * Capture the complete observable baseline before user callbacks.
     *
     * Callbacks are part of reset execution but are not allowed to mutate
     * coordinator registration while state == RUNNING.
     */
    vitte_reset_capture_arena_before(
        reset);

    vitte_reset_capture_pools_before(
        reset);

    if (reset->config.invoke_callbacks &&
        reset->before_callback != NULL) {
        if (!reset->before_callback(
                reset,
                VITTE_RESET_CALLBACK_BEFORE,
                reset->callback_user_data)) {
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_CALLBACK_FAILED);

            /*
             * No coordinator reset mutation has occurred yet. Capture after
             * values anyway so the failure report remains complete and also
             * reflects any target mutation performed by the callback itself.
             */
            vitte_reset_capture_arena_after(
                reset);

            vitte_reset_capture_pools_after(
                reset);

            vitte_reset_mark_failed(reset);
            return false;
        }
    }

    mutation_started = true;

    /*
     * Pools are reset before the monotonic arena. This ordering is important
     * when pooled compiler objects contain pointers into arena storage: pool
     * liveness is invalidated first.
     */
    pools_success =
        vitte_reset_execute_pools(
            reset);

    if (!pools_success &&
        reset->config.stop_on_pool_error) {
        goto failure_after_mutation;
    }

    /*
     * Continue to the arena when stop_on_pool_error == false. The overall
     * execution remains failed if any pool failed.
     */
    if (!vitte_reset_execute_arena(
            reset)) {
        goto failure_after_mutation;
    }

    if (!pools_success) {
        goto failure_after_mutation;
    }

    if (!vitte_reset_validate_after(
            reset)) {
        goto failure_after_mutation;
    }

    vitte_reset_capture_arena_after(
        reset);

    vitte_reset_capture_pools_after(
        reset);

    /*
     * The after callback sees a fully populated success candidate report.
     * success remains false until the callback itself succeeds.
     */
    if (reset->config.invoke_callbacks &&
        reset->after_callback != NULL) {
        if (!reset->after_callback(
                reset,
                VITTE_RESET_CALLBACK_AFTER,
                reset->callback_user_data)) {
            vitte_reset_set_error(
                reset,
                VITTE_RESET_ERROR_CALLBACK_FAILED);

            vitte_reset_mark_failed(reset);
            return false;
        }
    }

    reset->report.success = true;
    reset->report.error =
        VITTE_RESET_ERROR_NONE;

    reset->state =
        VITTE_RESET_STATE_COMPLETED;

    reset->last_error =
        VITTE_RESET_ERROR_NONE;

    vitte_reset_saturating_increment(
        &reset->successful_execution_count);

    return true;

failure_after_mutation:

    /*
     * Reset is intentionally not transactional across independent pools and
     * the arena. Earlier targets may already have been reset when a later
     * target fails. Report the resulting state precisely.
     */
    if (mutation_started) {
        vitte_reset_capture_arena_after(
            reset);

        vitte_reset_capture_pools_after(
            reset);
    }

    vitte_reset_mark_failed(reset);

    return false;
}

/* ========================================================================= */
/* Direct helpers                                                            */
/* ========================================================================= */

bool
vitte_reset_arena_only(
    vitte_arena_context_t *arena,
    vitte_reset_mode_t mode)
{
    vitte_reset_t reset;
    vitte_reset_config_t config;
    bool result;

    if (arena == NULL ||
        !vitte_arena_context_is_valid(
            arena)) {
        return false;
    }

    config =
        vitte_reset_config_default();

    config.mode = mode;
    config.reset_arena = true;
    config.reset_pools = false;

    if (!vitte_reset_config_validate(
            &config)) {
        return false;
    }

    if (!vitte_reset_init(
            &reset,
            arena,
            &config)) {
        return false;
    }

    result =
        vitte_reset_execute(
            &reset);

    vitte_reset_destroy(
        &reset);

    return result;
}

bool
vitte_reset_pool_only(
    vitte_pool_t *pool,
    vitte_reset_mode_t mode)
{
    vitte_reset_t reset;
    vitte_reset_config_t config;
    bool result;

    if (pool == NULL ||
        !vitte_pool_is_valid(pool)) {
        return false;
    }

    config =
        vitte_reset_config_default();

    config.mode = mode;
    config.reset_arena = false;
    config.reset_pools = true;

    if (!vitte_reset_config_validate(
            &config)) {
        return false;
    }

    if (!vitte_reset_init(
            &reset,
            NULL,
            &config)) {
        return false;
    }

    if (!vitte_reset_add_pool(
            &reset,
            pool)) {
        vitte_reset_destroy(
            &reset);

        return false;
    }

    result =
        vitte_reset_execute(
            &reset);

    vitte_reset_destroy(
        &reset);

    return result;
}

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

bool
vitte_reset_is_valid(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_magic_valid(reset);
}

bool
vitte_reset_is_running(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_magic_valid(reset) &&
        reset->state ==
            VITTE_RESET_STATE_RUNNING;
}

bool
vitte_reset_succeeded(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_magic_valid(reset) &&
        reset->state ==
            VITTE_RESET_STATE_COMPLETED &&
        reset->report.success &&
        reset->report.error ==
            VITTE_RESET_ERROR_NONE;
}

bool
vitte_reset_failed(
    const vitte_reset_t *reset)
{
    return
        vitte_reset_magic_valid(reset) &&
        reset->state ==
            VITTE_RESET_STATE_FAILED;
}

vitte_reset_state_t
vitte_reset_state(
    const vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return
            VITTE_RESET_STATE_UNINITIALIZED;
    }

    return reset->state;
}

/* ========================================================================= */
/* Configuration mutation                                                    */
/* ========================================================================= */

bool
vitte_reset_set_mode(
    vitte_reset_t *reset,
    vitte_reset_mode_t mode)
{
    vitte_reset_config_t candidate;

    if (!vitte_reset_magic_valid(reset)) {
        return false;
    }

    if (reset->state ==
        VITTE_RESET_STATE_RUNNING) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_STATE);

        return false;
    }

    candidate =
        reset->config;

    candidate.mode = mode;

    if (!vitte_reset_config_validate(
            &candidate)) {
        vitte_reset_set_error(
            reset,
            VITTE_RESET_ERROR_INVALID_ARGUMENT);

        return false;
    }

    reset->config = candidate;

    reset->last_error =
        VITTE_RESET_ERROR_NONE;

    return true;
}

vitte_reset_mode_t
vitte_reset_mode(
    const vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return
            VITTE_RESET_MODE_RETAIN;
    }

    return reset->config.mode;
}

/* ========================================================================= */
/* Report access                                                             */
/* ========================================================================= */

const vitte_reset_report_t *
vitte_reset_report(
    const vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return NULL;
    }

    return &reset->report;
}

/* ========================================================================= */
/* Execution statistics                                                      */
/* ========================================================================= */

size_t
vitte_reset_execution_count(
    const vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return 0u;
    }

    return reset->execution_count;
}

size_t
vitte_reset_successful_execution_count(
    const vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return 0u;
    }

    return
        reset->successful_execution_count;
}

size_t
vitte_reset_failed_execution_count(
    const vitte_reset_t *reset)
{
    if (!vitte_reset_magic_valid(reset)) {
        return 0u;
    }

    return
        reset->failed_execution_count;
}

/* ========================================================================= */
/* Deep validation                                                          */
/* ========================================================================= */

bool
vitte_reset_validate(
    const vitte_reset_t *reset)
{
    size_t left;
    size_t right;
    size_t resolved_count;

    if (!vitte_reset_magic_valid(reset)) {
        return false;
    }

    if (!vitte_reset_config_validate(
            &reset->config)) {
        return false;
    }

    if (reset->pool_count >
        VITTE_RESET_MAX_POOLS) {
        return false;
    }

    switch (reset->state) {
        case VITTE_RESET_STATE_READY:
        case VITTE_RESET_STATE_RUNNING:
        case VITTE_RESET_STATE_COMPLETED:
        case VITTE_RESET_STATE_FAILED:
            break;

        case VITTE_RESET_STATE_UNINITIALIZED:
        case VITTE_RESET_STATE_DESTROYED:
        default:
            return false;
    }

    if (reset->arena != NULL &&
        !vitte_arena_context_is_valid(
            reset->arena)) {
        return false;
    }

    for (left = 0u;
         left < reset->pool_count;
         ++left) {
        if (reset->pools[left] == NULL ||
            !vitte_pool_is_valid(
                reset->pools[left])) {
            return false;
        }

        for (right = left + 1u;
             right < reset->pool_count;
             ++right) {
            if (reset->pools[left] ==
                reset->pools[right]) {
                return false;
            }
        }
    }

    for (left = reset->pool_count;
         left < VITTE_RESET_MAX_POOLS;
         ++left) {
        if (reset->pools[left] != NULL) {
            return false;
        }
    }

    if (reset->successful_execution_count >
            reset->execution_count ||
        reset->failed_execution_count >
            reset->execution_count) {
        return false;
    }

    /*
     * Saturating counters require special handling once execution_count
     * reaches SIZE_MAX. Before saturation, every completed execute attempt
     * must be classified exactly once as successful or failed, except while
     * one execution is currently RUNNING.
     */
    if (reset->execution_count != SIZE_MAX &&
        reset->successful_execution_count != SIZE_MAX &&
        reset->failed_execution_count != SIZE_MAX) {
        if (vitte_reset_add_overflow(
                reset->successful_execution_count,
                reset->failed_execution_count,
                &resolved_count)) {
            return false;
        }

        if (reset->state ==
            VITTE_RESET_STATE_RUNNING) {
            if (reset->execution_count == 0u) {
                return false;
            }

            if (resolved_count !=
                reset->execution_count - 1u) {
                return false;
            }
        } else {
            if (resolved_count !=
                reset->execution_count) {
                return false;
            }
        }
    }

    if (reset->state ==
            VITTE_RESET_STATE_COMPLETED &&
        (!reset->report.success ||
         reset->report.error !=
             VITTE_RESET_ERROR_NONE ||
         reset->last_error !=
             VITTE_RESET_ERROR_NONE)) {
        return false;
    }

    if (reset->state ==
            VITTE_RESET_STATE_FAILED &&
        (reset->report.success ||
         reset->last_error ==
             VITTE_RESET_ERROR_NONE)) {
        return false;
    }

    if (reset->state ==
            VITTE_RESET_STATE_READY &&
        reset->report.success) {
        return false;
    }

    /*
     * Validate report arithmetic only for resolved executions.
     */
    if (reset->state ==
            VITTE_RESET_STATE_COMPLETED ||
        reset->state ==
            VITTE_RESET_STATE_FAILED) {
        if (reset->report.arena_reserved_released !=
            vitte_reset_nonnegative_difference(
                reset->report.arena_reserved_before,
                reset->report.arena_reserved_after)) {
            return false;
        }

        if (reset->report.arena_used_released !=
            vitte_reset_nonnegative_difference(
                reset->report.arena_used_before,
                reset->report.arena_used_after)) {
            return false;
        }

        if (reset->report.arena_requested_released !=
            vitte_reset_nonnegative_difference(
                reset->report.arena_requested_before,
                reset->report.arena_requested_after)) {
            return false;
        }

        if (reset->report.pool_capacity_released !=
            vitte_reset_nonnegative_difference(
                reset->report.pool_capacity_before,
                reset->report.pool_capacity_after)) {
            return false;
        }

        if (reset->report.pool_live_released !=
            vitte_reset_nonnegative_difference(
                reset->report.pool_live_before,
                reset->report.pool_live_after)) {
            return false;
        }

        if (reset->report.pool_reserved_released !=
            vitte_reset_nonnegative_difference(
                reset->report.pool_reserved_before,
                reset->report.pool_reserved_after)) {
            return false;
        }

        if (reset->report.pool_pages_released !=
            vitte_reset_nonnegative_difference(
                reset->report.pool_pages_before,
                reset->report.pool_pages_after)) {
            return false;
        }
    }

    /*
     * A successful completed reset must leave all selected targets in their
     * expected logical reset state.
     */
    if (reset->state ==
            VITTE_RESET_STATE_COMPLETED &&
        reset->report.success) {
        if (reset->config.reset_arena &&
            reset->arena != NULL) {
            if (!vitte_arena_context_is_valid(
                    reset->arena)) {
                return false;
            }

            if (vitte_arena_context_allocation_count(
                    reset->arena) != 0u ||
                vitte_arena_context_requested_bytes(
                    reset->arena) != 0u ||
                vitte_arena_context_used_bytes(
                    reset->arena) != 0u) {
                return false;
            }

            if (reset->report.arena_generation_before !=
                    UINT64_C(0) &&
                reset->report.arena_generation_after !=
                    vitte_reset_expected_next_generation(
                        reset->report.arena_generation_before)) {
                return false;
            }
        }

        if (reset->config.reset_pools) {
            for (left = 0u;
                 left < reset->pool_count;
                 ++left) {
                const vitte_pool_t *pool;

                pool =
                    reset->pools[left];

                if (!vitte_pool_is_valid(pool)) {
                    return false;
                }

                if (vitte_pool_live_count(pool) !=
                    0u) {
                    return false;
                }

                if (vitte_pool_free_count(pool) !=
                    vitte_pool_capacity(pool)) {
                    return false;
                }

                if (reset->config.mode ==
                        VITTE_RESET_MODE_RELEASE &&
                    (vitte_pool_page_count(pool) !=
                         0u ||
                     vitte_pool_capacity(pool) !=
                         0u ||
                     vitte_pool_reserved_bytes(pool) !=
                         0u)) {
                    return false;
                }
            }
        }
    }

    return true;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */