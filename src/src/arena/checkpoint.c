/*
 * Vitte Compiler
 * src/arena/checkpoint.c
 *
 * Transactional checkpoints for compiler arenas.
 *
 * This module builds on:
 *
 *     block.c/.h
 *         physical blocks
 *
 *     allocator.c/.h
 *         low-level arena allocation + marks
 *
 *     arena.c/.h
 *         compiler-facing arena context
 *
 *     checkpoint.c/.h
 *         transactional allocation checkpoints
 *
 * Typical compiler usage:
 *
 *     vitte_checkpoint_t checkpoint;
 *
 *     if (!vitte_checkpoint_capture(&checkpoint, arena)) {
 *         return false;
 *     }
 *
 *     ...
 *
 *     if (parse_failed) {
 *         vitte_checkpoint_rollback(&checkpoint);
 *         return false;
 *     }
 *
 *     vitte_checkpoint_commit(&checkpoint);
 *
 * Checkpoints provide transactional memory semantics for speculative parser,
 * semantic-analysis and lowering operations.
 *
 * Properties:
 *
 *   - arena ownership validation
 *   - generation validation
 *   - stale-checkpoint detection
 *   - nested checkpoint support
 *   - rollback
 *   - logical commit
 *   - explicit invalidation
 *   - high-level accounting restoration
 *   - low-level allocator rewind
 *   - checkpoint statistics
 *   - sequence identifiers
 *   - corruption detection
 *   - overflow-safe accounting
 *   - deterministic state transitions
 *
 * A commit does not physically change arena memory. It simply accepts all
 * allocations performed after capture and invalidates the checkpoint.
 *
 * A rollback restores the low-level allocator mark and the high-level arena
 * allocation counters captured by the checkpoint.
 */

#include "checkpoint.h"

#include "allocator.h"
#include "arena.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_CHECKPOINT_MAGIC
#define VITTE_CHECKPOINT_MAGIC \
    UINT64_C(0x5649545445434850)
#endif

#ifndef VITTE_CHECKPOINT_DEAD_MAGIC
#define VITTE_CHECKPOINT_DEAD_MAGIC \
    UINT64_C(0x4445414443484B50)
#endif

#ifndef VITTE_CHECKPOINT_MAX_DEPTH
#define VITTE_CHECKPOINT_MAX_DEPTH \
    ((size_t)1024u)
#endif

/* ========================================================================= */
/* Sequence                                                                  */
/* ========================================================================= */

/*
 * Checkpoint identifiers are diagnostic/debug identifiers.
 *
 * They are intentionally process-local and are not serialized.
 *
 * This implementation avoids requiring atomics because arena contexts are not
 * themselves specified as concurrently mutable. A checkpoint manager should
 * be externally synchronized if shared between threads.
 */
static uint64_t vitte_checkpoint_next_sequence = UINT64_C(1);

static uint64_t
vitte_checkpoint_allocate_sequence(void)
{
    uint64_t result;

    result =
        vitte_checkpoint_next_sequence;

    ++vitte_checkpoint_next_sequence;

    /*
     * Zero is reserved as "no sequence".
     */
    if (vitte_checkpoint_next_sequence == 0u) {
        vitte_checkpoint_next_sequence =
            UINT64_C(1);
    }

    if (result == 0u) {
        result =
            vitte_checkpoint_next_sequence;

        ++vitte_checkpoint_next_sequence;

        if (vitte_checkpoint_next_sequence == 0u) {
            vitte_checkpoint_next_sequence =
                UINT64_C(1);
        }
    }

    return result;
}

/* ========================================================================= */
/* Arithmetic                                                                */
/* ========================================================================= */

static bool
vitte_checkpoint_size_sub(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left < right) {
        *result = 0u;
        return false;
    }

    *result = left - right;

    return true;
}

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

const char *
vitte_checkpoint_state_name(
    vitte_checkpoint_state_t state)
{
    switch (state) {
        case VITTE_CHECKPOINT_STATE_EMPTY:
            return "empty";

        case VITTE_CHECKPOINT_STATE_ACTIVE:
            return "active";

        case VITTE_CHECKPOINT_STATE_COMMITTED:
            return "committed";

        case VITTE_CHECKPOINT_STATE_ROLLED_BACK:
            return "rolled-back";

        case VITTE_CHECKPOINT_STATE_INVALIDATED:
            return "invalidated";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

const char *
vitte_checkpoint_error_name(
    vitte_checkpoint_error_t error)
{
    switch (error) {
        case VITTE_CHECKPOINT_ERROR_NONE:
            return "none";

        case VITTE_CHECKPOINT_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_CHECKPOINT_ERROR_INVALID_ARENA:
            return "invalid-arena";

        case VITTE_CHECKPOINT_ERROR_INVALID_CHECKPOINT:
            return "invalid-checkpoint";

        case VITTE_CHECKPOINT_ERROR_INVALID_STATE:
            return "invalid-state";

        case VITTE_CHECKPOINT_ERROR_STALE:
            return "stale";

        case VITTE_CHECKPOINT_ERROR_FOREIGN_ARENA:
            return "foreign-arena";

        case VITTE_CHECKPOINT_ERROR_GENERATION_MISMATCH:
            return "generation-mismatch";

        case VITTE_CHECKPOINT_ERROR_DEPTH_EXCEEDED:
            return "depth-exceeded";

        case VITTE_CHECKPOINT_ERROR_ROLLBACK_FAILED:
            return "rollback-failed";

        case VITTE_CHECKPOINT_ERROR_CORRUPTION:
            return "corruption";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Internal error handling                                                   */
/* ========================================================================= */

static void
vitte_checkpoint_set_error(
    vitte_checkpoint_t *checkpoint,
    vitte_checkpoint_error_t error)
{
    if (checkpoint == NULL) {
        return;
    }

    checkpoint->last_error = error;
}

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

void
vitte_checkpoint_init(
    vitte_checkpoint_t *checkpoint)
{
    if (checkpoint == NULL) {
        return;
    }

    memset(
        checkpoint,
        0,
        sizeof(*checkpoint));

    checkpoint->magic =
        VITTE_CHECKPOINT_MAGIC;

    checkpoint->state =
        VITTE_CHECKPOINT_STATE_EMPTY;

    checkpoint->last_error =
        VITTE_CHECKPOINT_ERROR_NONE;
}

/* ========================================================================= */
/* Basic structural validation                                               */
/* ========================================================================= */

static bool
vitte_checkpoint_magic_valid(
    const vitte_checkpoint_t *checkpoint)
{
    return
        checkpoint != NULL &&
        checkpoint->magic ==
            VITTE_CHECKPOINT_MAGIC;
}

bool
vitte_checkpoint_is_initialized(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_magic_valid(
            checkpoint);
}

bool
vitte_checkpoint_is_active(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_magic_valid(
            checkpoint) &&
        checkpoint->state ==
            VITTE_CHECKPOINT_STATE_ACTIVE;
}

bool
vitte_checkpoint_is_committed(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_magic_valid(
            checkpoint) &&
        checkpoint->state ==
            VITTE_CHECKPOINT_STATE_COMMITTED;
}

bool
vitte_checkpoint_is_rolled_back(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_magic_valid(
            checkpoint) &&
        checkpoint->state ==
            VITTE_CHECKPOINT_STATE_ROLLED_BACK;
}

bool
vitte_checkpoint_is_invalidated(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_magic_valid(
            checkpoint) &&
        checkpoint->state ==
            VITTE_CHECKPOINT_STATE_INVALIDATED;
}

/* ========================================================================= */
/* Capture                                                                   */
/* ========================================================================= */

bool
vitte_checkpoint_capture(
    vitte_checkpoint_t *checkpoint,
    vitte_arena_context_t *arena)
{
    vitte_arena_checkpoint_t arena_checkpoint;

    if (checkpoint == NULL) {
        return false;
    }

    /*
     * Allow zeroed/uninitialized checkpoint storage as a convenience.
     */
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        vitte_checkpoint_init(
            checkpoint);
    }

    if (!vitte_arena_context_is_valid(
            arena)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_ARENA);

        return false;
    }

    /*
     * Reusing an active checkpoint without explicitly resolving it would lose
     * the original rollback position.
     */
    if (checkpoint->state ==
        VITTE_CHECKPOINT_STATE_ACTIVE) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_STATE);

        return false;
    }

    arena_checkpoint =
        vitte_arena_context_checkpoint(
            arena);

    if (arena_checkpoint.context != arena ||
        arena_checkpoint.generation == 0u) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_CORRUPTION);

        return false;
    }

    checkpoint->magic =
        VITTE_CHECKPOINT_MAGIC;

    checkpoint->state =
        VITTE_CHECKPOINT_STATE_ACTIVE;

    checkpoint->last_error =
        VITTE_CHECKPOINT_ERROR_NONE;

    checkpoint->arena = arena;

    checkpoint->mark =
        arena_checkpoint.mark;

    checkpoint->generation =
        arena_checkpoint.generation;

    checkpoint->allocation_count =
        arena_checkpoint.allocation_count;

    checkpoint->requested_bytes =
        arena_checkpoint.requested_bytes;

    checkpoint->used_bytes =
        vitte_arena_context_used_bytes(
            arena);

    checkpoint->reserved_bytes =
        vitte_arena_context_reserved_bytes(
            arena);

    checkpoint->sequence =
        vitte_checkpoint_allocate_sequence();

    checkpoint->depth = 0u;

    checkpoint->parent = NULL;

    return true;
}

/* ========================================================================= */
/* Nested capture                                                            */
/* ========================================================================= */

bool
vitte_checkpoint_capture_nested(
    vitte_checkpoint_t *checkpoint,
    vitte_checkpoint_t *parent)
{
    vitte_arena_context_t *arena;

    if (checkpoint == NULL ||
        parent == NULL) {
        if (checkpoint != NULL) {
            if (!vitte_checkpoint_magic_valid(
                    checkpoint)) {
                vitte_checkpoint_init(
                    checkpoint);
            }

            vitte_checkpoint_set_error(
                checkpoint,
                VITTE_CHECKPOINT_ERROR_INVALID_ARGUMENT);
        }

        return false;
    }

    if (!vitte_checkpoint_is_active(
            parent)) {
        if (!vitte_checkpoint_magic_valid(
                checkpoint)) {
            vitte_checkpoint_init(
                checkpoint);
        }

        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_STATE);

        return false;
    }

    arena = parent->arena;

    if (!vitte_arena_context_is_valid(
            arena)) {
        if (!vitte_checkpoint_magic_valid(
                checkpoint)) {
            vitte_checkpoint_init(
                checkpoint);
        }

        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_ARENA);

        return false;
    }

    if (parent->depth >=
        VITTE_CHECKPOINT_MAX_DEPTH) {
        if (!vitte_checkpoint_magic_valid(
                checkpoint)) {
            vitte_checkpoint_init(
                checkpoint);
        }

        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_DEPTH_EXCEEDED);

        return false;
    }

    if (!vitte_checkpoint_capture(
            checkpoint,
            arena)) {
        return false;
    }

    checkpoint->parent = parent;
    checkpoint->depth =
        parent->depth + 1u;

    return true;
}

/* ========================================================================= */
/* Staleness                                                                 */
/* ========================================================================= */

bool
vitte_checkpoint_is_stale(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_is_active(
            checkpoint)) {
        return true;
    }

    if (checkpoint->arena == NULL) {
        return true;
    }

    if (!vitte_arena_context_is_valid(
            checkpoint->arena)) {
        return true;
    }

    if (checkpoint->generation !=
        vitte_arena_context_generation(
            checkpoint->arena)) {
        return true;
    }

    return false;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_checkpoint_validate(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return false;
    }

    switch (checkpoint->state) {
        case VITTE_CHECKPOINT_STATE_EMPTY:
            if (checkpoint->arena != NULL) {
                return false;
            }

            return true;

        case VITTE_CHECKPOINT_STATE_ACTIVE:
            break;

        case VITTE_CHECKPOINT_STATE_COMMITTED:
        case VITTE_CHECKPOINT_STATE_ROLLED_BACK:
        case VITTE_CHECKPOINT_STATE_INVALIDATED:
            /*
             * Resolved checkpoints retain metadata for diagnostics.
             */
            return true;

        default:
            return false;
    }

    if (checkpoint->arena == NULL) {
        return false;
    }

    if (!vitte_arena_context_is_valid(
            checkpoint->arena)) {
        return false;
    }

    if (checkpoint->generation == 0u) {
        return false;
    }

    if (checkpoint->sequence == 0u) {
        return false;
    }

    if (checkpoint->generation !=
        vitte_arena_context_generation(
            checkpoint->arena)) {
        return false;
    }

    if (checkpoint->allocation_count >
        vitte_arena_context_allocation_count(
            checkpoint->arena)) {
        return false;
    }

    if (checkpoint->requested_bytes >
        vitte_arena_context_requested_bytes(
            checkpoint->arena)) {
        return false;
    }

    if (checkpoint->used_bytes >
        vitte_arena_context_used_bytes(
            checkpoint->arena)) {
        return false;
    }

    if (checkpoint->reserved_bytes >
        vitte_arena_context_reserved_bytes(
            checkpoint->arena)) {
        /*
         * Trimming while a checkpoint is active can legitimately reduce
         * reserved capacity only if no live allocation is affected. The
         * current high-level API does not model that as checkpoint-safe, so
         * treat it as invalid transactional state.
         */
        return false;
    }

    if (checkpoint->depth >
        VITTE_CHECKPOINT_MAX_DEPTH) {
        return false;
    }

    if (checkpoint->parent != NULL) {
        if (!vitte_checkpoint_magic_valid(
                checkpoint->parent)) {
            return false;
        }

        if (!vitte_checkpoint_is_active(
                checkpoint->parent)) {
            return false;
        }

        if (checkpoint->parent->arena !=
            checkpoint->arena) {
            return false;
        }

        if (checkpoint->parent->generation !=
            checkpoint->generation) {
            return false;
        }

        if (checkpoint->depth !=
            checkpoint->parent->depth + 1u) {
            return false;
        }
    } else if (checkpoint->depth != 0u) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Rollback                                                                  */
/* ========================================================================= */

bool
vitte_checkpoint_rollback(
    vitte_checkpoint_t *checkpoint)
{
    vitte_arena_checkpoint_t arena_checkpoint;

    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return false;
    }

    if (checkpoint->state !=
        VITTE_CHECKPOINT_STATE_ACTIVE) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_STATE);

        return false;
    }

    if (checkpoint->arena == NULL) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_CHECKPOINT);

        return false;
    }

    if (!vitte_arena_context_is_valid(
            checkpoint->arena)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_ARENA);

        return false;
    }

    if (checkpoint->generation !=
        vitte_arena_context_generation(
            checkpoint->arena)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_GENERATION_MISMATCH);

        return false;
    }

    /*
     * If a child checkpoint is still active, rolling back its parent would
     * silently invalidate the child's rollback position.
     *
     * Since checkpoints intentionally do not maintain a mutable child list,
     * callers must resolve nested checkpoints in LIFO order. The parent
     * relationship lets us enforce the inverse direction: a child cannot
     * remain valid once its parent has already been resolved.
     */
    if (checkpoint->parent != NULL &&
        !vitte_checkpoint_is_active(
            checkpoint->parent)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_STALE);

        return false;
    }

    memset(
        &arena_checkpoint,
        0,
        sizeof(arena_checkpoint));

    arena_checkpoint.context =
        checkpoint->arena;

    arena_checkpoint.mark =
        checkpoint->mark;

    arena_checkpoint.generation =
        checkpoint->generation;

    arena_checkpoint.allocation_count =
        checkpoint->allocation_count;

    arena_checkpoint.requested_bytes =
        checkpoint->requested_bytes;

    if (!vitte_arena_context_rollback(
            checkpoint->arena,
            arena_checkpoint)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_ROLLBACK_FAILED);

        return false;
    }

    checkpoint->state =
        VITTE_CHECKPOINT_STATE_ROLLED_BACK;

    checkpoint->last_error =
        VITTE_CHECKPOINT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Commit                                                                    */
/* ========================================================================= */

bool
vitte_checkpoint_commit(
    vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return false;
    }

    if (checkpoint->state !=
        VITTE_CHECKPOINT_STATE_ACTIVE) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_STATE);

        return false;
    }

    if (checkpoint->arena == NULL ||
        !vitte_arena_context_is_valid(
            checkpoint->arena)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_ARENA);

        return false;
    }

    if (checkpoint->generation !=
        vitte_arena_context_generation(
            checkpoint->arena)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_GENERATION_MISMATCH);

        return false;
    }

    if (checkpoint->parent != NULL &&
        !vitte_checkpoint_is_active(
            checkpoint->parent)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_STALE);

        return false;
    }

    /*
     * Commit is logical only.
     *
     * Allocations already belong to the arena, therefore accepting the
     * transaction requires no memory operation.
     */
    checkpoint->state =
        VITTE_CHECKPOINT_STATE_COMMITTED;

    checkpoint->last_error =
        VITTE_CHECKPOINT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Invalidation                                                              */
/* ========================================================================= */

void
vitte_checkpoint_invalidate(
    vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return;
    }

    checkpoint->state =
        VITTE_CHECKPOINT_STATE_INVALIDATED;

    checkpoint->last_error =
        VITTE_CHECKPOINT_ERROR_NONE;
}

/* ========================================================================= */
/* Reset object                                                              */
/* ========================================================================= */

void
vitte_checkpoint_reset(
    vitte_checkpoint_t *checkpoint)
{
    if (checkpoint == NULL) {
        return;
    }

    /*
     * Resetting an active checkpoint intentionally abandons rollback
     * capability. This function should therefore only be used when the caller
     * explicitly wants to discard checkpoint metadata.
     */
    vitte_checkpoint_init(
        checkpoint);
}

/* ========================================================================= */
/* Destroy                                                                   */
/* ========================================================================= */

void
vitte_checkpoint_destroy(
    vitte_checkpoint_t *checkpoint)
{
    if (checkpoint == NULL) {
        return;
    }

    if (vitte_checkpoint_magic_valid(
            checkpoint)) {
        checkpoint->state =
            VITTE_CHECKPOINT_STATE_INVALIDATED;
    }

    checkpoint->arena = NULL;
    checkpoint->parent = NULL;

    memset(
        &checkpoint->mark,
        0,
        sizeof(checkpoint->mark));

    checkpoint->generation = 0u;

    checkpoint->allocation_count = 0u;
    checkpoint->requested_bytes = 0u;

    checkpoint->used_bytes = 0u;
    checkpoint->reserved_bytes = 0u;

    checkpoint->sequence = 0u;
    checkpoint->depth = 0u;

    checkpoint->last_error =
        VITTE_CHECKPOINT_ERROR_INVALID_CHECKPOINT;

    checkpoint->magic =
        VITTE_CHECKPOINT_DEAD_MAGIC;
}

/* ========================================================================= */
/* Error access                                                              */
/* ========================================================================= */

vitte_checkpoint_error_t
vitte_checkpoint_last_error(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return
            VITTE_CHECKPOINT_ERROR_INVALID_CHECKPOINT;
    }

    return checkpoint->last_error;
}

void
vitte_checkpoint_clear_error(
    vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return;
    }

    checkpoint->last_error =
        VITTE_CHECKPOINT_ERROR_NONE;
}

/* ========================================================================= */
/* Arena                                                                     */
/* ========================================================================= */

vitte_arena_context_t *
vitte_checkpoint_arena(
    vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return NULL;
    }

    return checkpoint->arena;
}

const vitte_arena_context_t *
vitte_checkpoint_arena_const(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return NULL;
    }

    return checkpoint->arena;
}

/* ========================================================================= */
/* Parent                                                                    */
/* ========================================================================= */

vitte_checkpoint_t *
vitte_checkpoint_parent(
    vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return NULL;
    }

    return checkpoint->parent;
}

const vitte_checkpoint_t *
vitte_checkpoint_parent_const(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return NULL;
    }

    return checkpoint->parent;
}

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

uint64_t
vitte_checkpoint_sequence(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return checkpoint->sequence;
}

uint64_t
vitte_checkpoint_generation(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return checkpoint->generation;
}

size_t
vitte_checkpoint_depth(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return checkpoint->depth;
}

/* ========================================================================= */
/* Allocation delta                                                          */
/* ========================================================================= */

size_t
vitte_checkpoint_allocation_delta(
    const vitte_checkpoint_t *checkpoint)
{
    size_t current;
    size_t result;

    if (!vitte_checkpoint_is_active(
            checkpoint) ||
        checkpoint->arena == NULL) {
        return 0u;
    }

    current =
        vitte_arena_context_allocation_count(
            checkpoint->arena);

    if (!vitte_checkpoint_size_sub(
            current,
            checkpoint->allocation_count,
            &result)) {
        return 0u;
    }

    return result;
}

/* ========================================================================= */
/* Requested-byte delta                                                      */
/* ========================================================================= */

size_t
vitte_checkpoint_requested_delta(
    const vitte_checkpoint_t *checkpoint)
{
    size_t current;
    size_t result;

    if (!vitte_checkpoint_is_active(
            checkpoint) ||
        checkpoint->arena == NULL) {
        return 0u;
    }

    current =
        vitte_arena_context_requested_bytes(
            checkpoint->arena);

    if (!vitte_checkpoint_size_sub(
            current,
            checkpoint->requested_bytes,
            &result)) {
        return 0u;
    }

    return result;
}

/* ========================================================================= */
/* Physical used-byte delta                                                  */
/* ========================================================================= */

size_t
vitte_checkpoint_used_delta(
    const vitte_checkpoint_t *checkpoint)
{
    size_t current;
    size_t result;

    if (!vitte_checkpoint_is_active(
            checkpoint) ||
        checkpoint->arena == NULL) {
        return 0u;
    }

    current =
        vitte_arena_context_used_bytes(
            checkpoint->arena);

    if (!vitte_checkpoint_size_sub(
            current,
            checkpoint->used_bytes,
            &result)) {
        return 0u;
    }

    return result;
}

/* ========================================================================= */
/* Reserved-byte delta                                                       */
/* ========================================================================= */

size_t
vitte_checkpoint_reserved_delta(
    const vitte_checkpoint_t *checkpoint)
{
    size_t current;
    size_t result;

    if (!vitte_checkpoint_is_active(
            checkpoint) ||
        checkpoint->arena == NULL) {
        return 0u;
    }

    current =
        vitte_arena_context_reserved_bytes(
            checkpoint->arena);

    if (!vitte_checkpoint_size_sub(
            current,
            checkpoint->reserved_bytes,
            &result)) {
        return 0u;
    }

    return result;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_checkpoint_stats_t
vitte_checkpoint_stats(
    const vitte_checkpoint_t *checkpoint)
{
    vitte_checkpoint_stats_t stats;

    memset(
        &stats,
        0,
        sizeof(stats));

    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return stats;
    }

    stats.state =
        checkpoint->state;

    stats.last_error =
        checkpoint->last_error;

    stats.sequence =
        checkpoint->sequence;

    stats.generation =
        checkpoint->generation;

    stats.depth =
        checkpoint->depth;

    stats.captured_allocation_count =
        checkpoint->allocation_count;

    stats.captured_requested_bytes =
        checkpoint->requested_bytes;

    stats.captured_used_bytes =
        checkpoint->used_bytes;

    stats.captured_reserved_bytes =
        checkpoint->reserved_bytes;

    if (checkpoint->state ==
            VITTE_CHECKPOINT_STATE_ACTIVE &&
        checkpoint->arena != NULL &&
        vitte_arena_context_is_valid(
            checkpoint->arena)) {
        stats.current_allocation_count =
            vitte_arena_context_allocation_count(
                checkpoint->arena);

        stats.current_requested_bytes =
            vitte_arena_context_requested_bytes(
                checkpoint->arena);

        stats.current_used_bytes =
            vitte_arena_context_used_bytes(
                checkpoint->arena);

        stats.current_reserved_bytes =
            vitte_arena_context_reserved_bytes(
                checkpoint->arena);

        stats.allocation_delta =
            vitte_checkpoint_allocation_delta(
                checkpoint);

        stats.requested_delta =
            vitte_checkpoint_requested_delta(
                checkpoint);

        stats.used_delta =
            vitte_checkpoint_used_delta(
                checkpoint);

        stats.reserved_delta =
            vitte_checkpoint_reserved_delta(
                checkpoint);

        stats.stale =
            vitte_checkpoint_is_stale(
                checkpoint);
    } else {
        stats.current_allocation_count =
            checkpoint->allocation_count;

        stats.current_requested_bytes =
            checkpoint->requested_bytes;

        stats.current_used_bytes =
            checkpoint->used_bytes;

        stats.current_reserved_bytes =
            checkpoint->reserved_bytes;

        stats.stale =
            checkpoint->state ==
                VITTE_CHECKPOINT_STATE_ACTIVE;
    }

    stats.has_parent =
        checkpoint->parent != NULL;

    return stats;
}

/* ========================================================================= */
/* Comparison                                                                */
/* ========================================================================= */

bool
vitte_checkpoint_same_arena(
    const vitte_checkpoint_t *left,
    const vitte_checkpoint_t *right)
{
    if (!vitte_checkpoint_magic_valid(left) ||
        !vitte_checkpoint_magic_valid(right)) {
        return false;
    }

    if (left->arena == NULL ||
        right->arena == NULL) {
        return false;
    }

    return left->arena == right->arena;
}

bool
vitte_checkpoint_same_generation(
    const vitte_checkpoint_t *left,
    const vitte_checkpoint_t *right)
{
    if (!vitte_checkpoint_same_arena(
            left,
            right)) {
        return false;
    }

    return
        left->generation ==
        right->generation;
}

bool
vitte_checkpoint_precedes(
    const vitte_checkpoint_t *left,
    const vitte_checkpoint_t *right)
{
    if (!vitte_checkpoint_same_generation(
            left,
            right)) {
        return false;
    }

    /*
     * sequence is creation order, not memory position.
     */
    return
        left->sequence != 0u &&
        right->sequence != 0u &&
        left->sequence <
            right->sequence;
}

/* ========================================================================= */
/* Ancestor                                                                  */
/* ========================================================================= */

bool
vitte_checkpoint_is_ancestor_of(
    const vitte_checkpoint_t *ancestor,
    const vitte_checkpoint_t *checkpoint)
{
    const vitte_checkpoint_t *current;
    size_t guard;

    if (!vitte_checkpoint_magic_valid(
            ancestor) ||
        !vitte_checkpoint_magic_valid(
            checkpoint)) {
        return false;
    }

    current = checkpoint->parent;
    guard = 0u;

    while (current != NULL) {
        if (current == ancestor) {
            return true;
        }

        if (!vitte_checkpoint_magic_valid(
                current)) {
            return false;
        }

        current = current->parent;

        ++guard;

        if (guard >
            VITTE_CHECKPOINT_MAX_DEPTH) {
            return false;
        }
    }

    return false;
}

/* ========================================================================= */
/* Root                                                                      */
/* ========================================================================= */

vitte_checkpoint_t *
vitte_checkpoint_root(
    vitte_checkpoint_t *checkpoint)
{
    vitte_checkpoint_t *current;
    size_t guard;

    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return NULL;
    }

    current = checkpoint;
    guard = 0u;

    while (current->parent != NULL) {
        if (!vitte_checkpoint_magic_valid(
                current->parent)) {
            return NULL;
        }

        current = current->parent;

        ++guard;

        if (guard >
            VITTE_CHECKPOINT_MAX_DEPTH) {
            return NULL;
        }
    }

    return current;
}

const vitte_checkpoint_t *
vitte_checkpoint_root_const(
    const vitte_checkpoint_t *checkpoint)
{
    const vitte_checkpoint_t *current;
    size_t guard;

    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return NULL;
    }

    current = checkpoint;
    guard = 0u;

    while (current->parent != NULL) {
        if (!vitte_checkpoint_magic_valid(
                current->parent)) {
            return NULL;
        }

        current = current->parent;

        ++guard;

        if (guard >
            VITTE_CHECKPOINT_MAX_DEPTH) {
            return NULL;
        }
    }

    return current;
}

/* ========================================================================= */
/* Relationship validation                                                   */
/* ========================================================================= */

bool
vitte_checkpoint_validate_chain(
    const vitte_checkpoint_t *checkpoint)
{
    const vitte_checkpoint_t *current;
    const vitte_arena_context_t *arena;
    uint64_t generation;
    size_t expected_depth;
    size_t guard;

    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return false;
    }

    arena = checkpoint->arena;
    generation = checkpoint->generation;
    expected_depth = checkpoint->depth;

    current = checkpoint;
    guard = 0u;

    while (current != NULL) {
        if (!vitte_checkpoint_magic_valid(
                current)) {
            return false;
        }

        if (current->arena != arena) {
            return false;
        }

        if (current->generation !=
            generation) {
            return false;
        }

        if (current->depth !=
            expected_depth) {
            return false;
        }

        if (current->parent == NULL) {
            if (current->depth != 0u) {
                return false;
            }

            return true;
        }

        if (expected_depth == 0u) {
            return false;
        }

        --expected_depth;

        current = current->parent;

        ++guard;

        if (guard >
            VITTE_CHECKPOINT_MAX_DEPTH) {
            return false;
        }
    }

    return false;
}

/* ========================================================================= */
/* Commit or rollback helper                                                 */
/* ========================================================================= */

bool
vitte_checkpoint_finish(
    vitte_checkpoint_t *checkpoint,
    bool commit)
{
    if (commit) {
        return
            vitte_checkpoint_commit(
                checkpoint);
    }

    return
        vitte_checkpoint_rollback(
            checkpoint);
}

/* ========================================================================= */
/* Transaction helper                                                        */
/* ========================================================================= */

bool
vitte_checkpoint_begin(
    vitte_checkpoint_t *checkpoint,
    vitte_arena_context_t *arena)
{
    return
        vitte_checkpoint_capture(
            checkpoint,
            arena);
}

/* ========================================================================= */
/* Error predicates                                                          */
/* ========================================================================= */

bool
vitte_checkpoint_has_error(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_last_error(
            checkpoint) !=
        VITTE_CHECKPOINT_ERROR_NONE;
}

/* ========================================================================= */
/* Snapshot access                                                           */
/* ========================================================================= */

size_t
vitte_checkpoint_captured_allocation_count(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return checkpoint->allocation_count;
}

size_t
vitte_checkpoint_captured_requested_bytes(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return checkpoint->requested_bytes;
}

size_t
vitte_checkpoint_captured_used_bytes(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return checkpoint->used_bytes;
}

size_t
vitte_checkpoint_captured_reserved_bytes(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return checkpoint->reserved_bytes;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
