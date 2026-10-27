/*
 * Vitte Compiler
 * src/arena/checkpoint.c
 *
 * Transactional checkpoints for compiler arenas.
 *
 * A checkpoint is a logical transaction boundary over a
 * vitte_arena_context_t.
 *
 * Architecture:
 *
 *     block.c/.h
 *         physical arena blocks
 *
 *     allocator.c/.h
 *         low-level block-chain allocator and marks
 *
 *     arena.c/.h
 *         compiler-facing arena context and accounting
 *
 *     checkpoint.c/.h
 *         transactional capture / commit / rollback
 *
 * Checkpoints are intentionally lightweight:
 *
 *   - no heap allocation
 *   - no per-allocation metadata
 *   - no mutable child list
 *   - commit is purely logical
 *   - rollback delegates physical restoration to arena.c
 *
 * Nested checkpoints are represented by parent pointers.
 *
 * IMPORTANT:
 *
 * Nested transactions must be resolved in LIFO order. Since checkpoints do
 * not maintain child lists, checkpoint.c can validate the child -> parent
 * direction but cannot discover arbitrary live children from a parent.
 *
 * ISO C17.
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
/* Compile-time invariants                                                   */
/* ========================================================================= */

_Static_assert(
    VITTE_CHECKPOINT_MAGIC != UINT64_C(0),
    "checkpoint live magic must be non-zero");

_Static_assert(
    VITTE_CHECKPOINT_DEAD_MAGIC != UINT64_C(0),
    "checkpoint dead magic must be non-zero");

_Static_assert(
    VITTE_CHECKPOINT_MAGIC != VITTE_CHECKPOINT_DEAD_MAGIC,
    "checkpoint live/dead magic values must differ");

_Static_assert(
    VITTE_CHECKPOINT_MAX_DEPTH != 0u,
    "checkpoint maximum depth must be non-zero");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "checkpoint implementation requires 64-bit uint64_t");

/* ========================================================================= */
/* Sequence identifiers                                                      */
/* ========================================================================= */

/*
 * Sequence numbers are diagnostic/debug identifiers.
 *
 * They are:
 *
 *   - thread-local
 *   - monotonically advancing modulo uint64_t
 *   - never zero
 *   - not serialized
 *   - not used as memory addresses
 *   - not used as generation identifiers
 *
 * The counter is thread-local so independent compiler contexts can create
 * checkpoints concurrently without sharing mutable process state. Arena
 * mutation itself remains caller-synchronized.
 */
static _Thread_local uint64_t vitte_checkpoint_next_sequence =
    UINT64_C(1);

static uint64_t
vitte_checkpoint_allocate_sequence(void)
{
    uint64_t result;

    result =
        vitte_checkpoint_next_sequence;

    if (result == UINT64_C(0)) {
        result = UINT64_C(1);
    }

    if (result == UINT64_MAX) {
        vitte_checkpoint_next_sequence =
            UINT64_C(1);
    } else {
        vitte_checkpoint_next_sequence =
            result + UINT64_C(1);

        if (vitte_checkpoint_next_sequence ==
            UINT64_C(0)) {
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

    *result = 0u;

    if (left < right) {
        return false;
    }

    *result =
        left - right;

    return true;
}

/* ========================================================================= */
/* Internal structural helpers                                               */
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

static bool
vitte_checkpoint_state_valid(
    vitte_checkpoint_state_t state)
{
    switch (state) {
        case VITTE_CHECKPOINT_STATE_EMPTY:
        case VITTE_CHECKPOINT_STATE_ACTIVE:
        case VITTE_CHECKPOINT_STATE_COMMITTED:
        case VITTE_CHECKPOINT_STATE_ROLLED_BACK:
        case VITTE_CHECKPOINT_STATE_INVALIDATED:
            return true;

        default:
            return false;
    }
}

static bool
vitte_checkpoint_error_valid(
    vitte_checkpoint_error_t error)
{
    switch (error) {
        case VITTE_CHECKPOINT_ERROR_NONE:
        case VITTE_CHECKPOINT_ERROR_INVALID_ARGUMENT:
        case VITTE_CHECKPOINT_ERROR_INVALID_ARENA:
        case VITTE_CHECKPOINT_ERROR_INVALID_CHECKPOINT:
        case VITTE_CHECKPOINT_ERROR_INVALID_STATE:
        case VITTE_CHECKPOINT_ERROR_STALE:
        case VITTE_CHECKPOINT_ERROR_FOREIGN_ARENA:
        case VITTE_CHECKPOINT_ERROR_GENERATION_MISMATCH:
        case VITTE_CHECKPOINT_ERROR_DEPTH_EXCEEDED:
        case VITTE_CHECKPOINT_ERROR_ROLLBACK_FAILED:
        case VITTE_CHECKPOINT_ERROR_CORRUPTION:
            return true;

        default:
            return false;
    }
}

static void
vitte_checkpoint_set_error(
    vitte_checkpoint_t *checkpoint,
    vitte_checkpoint_error_t error)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return;
    }

    checkpoint->last_error =
        error;
}

static void
vitte_checkpoint_clear_transaction(
    vitte_checkpoint_t *checkpoint)
{
    if (checkpoint == NULL) {
        return;
    }

    checkpoint->arena = NULL;
    checkpoint->parent = NULL;

    memset(
        &checkpoint->mark,
        0,
        sizeof(checkpoint->mark));

    checkpoint->generation = UINT64_C(0);

    checkpoint->allocation_count = 0u;
    checkpoint->requested_bytes = 0u;

    checkpoint->used_bytes = 0u;
    checkpoint->reserved_bytes = 0u;

    checkpoint->sequence = UINT64_C(0);
    checkpoint->depth = 0u;
}

/*
 * Detect a parent-pointer cycle using Floyd's algorithm.
 *
 * Only live checkpoint objects are followed. Encountering malformed metadata
 * is considered invalid.
 */
static bool
vitte_checkpoint_parent_chain_acyclic(
    const vitte_checkpoint_t *checkpoint)
{
    const vitte_checkpoint_t *slow;
    const vitte_checkpoint_t *fast;
    size_t guard;

    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return false;
    }

    slow = checkpoint;
    fast = checkpoint;
    guard = 0u;

    for (;;) {
        if (slow == NULL ||
            fast == NULL) {
            return true;
        }

        if (!vitte_checkpoint_magic_valid(
                slow) ||
            !vitte_checkpoint_magic_valid(
                fast)) {
            return false;
        }

        slow =
            slow->parent;

        fast =
            fast->parent;

        if (fast == NULL) {
            return true;
        }

        if (!vitte_checkpoint_magic_valid(
                fast)) {
            return false;
        }

        fast =
            fast->parent;

        if (slow == NULL ||
            fast == NULL) {
            return true;
        }

        if (slow == fast) {
            return false;
        }

        ++guard;

        if (guard >
            VITTE_CHECKPOINT_MAX_DEPTH) {
            return false;
        }
    }
}

/*
 * Validate an active checkpoint against its current arena without modifying
 * either object.
 */
static bool
vitte_checkpoint_active_snapshot_valid(
    const vitte_checkpoint_t *checkpoint)
{
    const vitte_arena_context_t *arena;

    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return false;
    }

    if (checkpoint->state !=
        VITTE_CHECKPOINT_STATE_ACTIVE) {
        return false;
    }

    arena =
        checkpoint->arena;

    if (arena == NULL ||
        !vitte_arena_context_is_valid(
            arena)) {
        return false;
    }

    if (checkpoint->generation ==
        UINT64_C(0)) {
        return false;
    }

    if (checkpoint->sequence ==
        UINT64_C(0)) {
        return false;
    }

    if (checkpoint->generation !=
        vitte_arena_context_generation(
            arena)) {
        return false;
    }

    if (checkpoint->allocation_count >
        vitte_arena_context_allocation_count(
            arena)) {
        return false;
    }

    if (checkpoint->requested_bytes >
        vitte_arena_context_requested_bytes(
            arena)) {
        return false;
    }

    if (checkpoint->used_bytes >
        vitte_arena_context_used_bytes(
            arena)) {
        return false;
    }

    /*
     * A trim that decreases reserved capacity while a transaction is active
     * makes the physical snapshot incomparable with the capture point.
     *
     * Treat that as stale/invalid transactional state.
     */
    if (checkpoint->reserved_bytes >
        vitte_arena_context_reserved_bytes(
            arena)) {
        return false;
    }

    if (checkpoint->depth >
        VITTE_CHECKPOINT_MAX_DEPTH) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* State names                                                               */
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
/* Error names                                                               */
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
/* Basic state queries                                                       */
/* ========================================================================= */

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
    vitte_arena_checkpoint_t snapshot;

    if (checkpoint == NULL) {
        return false;
    }

    /*
     * Permit fresh zeroed storage and previously destroyed storage.
     *
     * An active valid checkpoint is never silently overwritten.
     */
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        vitte_checkpoint_init(
            checkpoint);
    }

    if (checkpoint->state ==
        VITTE_CHECKPOINT_STATE_ACTIVE) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_STATE);

        return false;
    }

    if (!vitte_arena_context_is_valid(
            arena)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_ARENA);

        return false;
    }

    snapshot =
        vitte_arena_context_checkpoint(
            arena);

    if (snapshot.context != arena ||
        snapshot.generation ==
            UINT64_C(0)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_CORRUPTION);

        return false;
    }

    /*
     * Build the complete new transaction only after capture succeeded.
     */
    checkpoint->state =
        VITTE_CHECKPOINT_STATE_ACTIVE;

    checkpoint->last_error =
        VITTE_CHECKPOINT_ERROR_NONE;

    checkpoint->arena =
        arena;

    checkpoint->mark =
        snapshot.mark;

    checkpoint->generation =
        snapshot.generation;

    checkpoint->allocation_count =
        snapshot.allocation_count;

    checkpoint->requested_bytes =
        snapshot.requested_bytes;

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

    /*
     * Reject an internally inconsistent capture rather than exposing a
     * transaction that can never be safely rolled back.
     */
    if (!vitte_checkpoint_active_snapshot_valid(
            checkpoint)) {
        vitte_checkpoint_clear_transaction(
            checkpoint);

        checkpoint->state =
            VITTE_CHECKPOINT_STATE_INVALIDATED;

        checkpoint->last_error =
            VITTE_CHECKPOINT_ERROR_CORRUPTION;

        return false;
    }

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

    if (checkpoint == NULL) {
        return false;
    }

    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        vitte_checkpoint_init(
            checkpoint);
    }

    if (parent == NULL ||
        checkpoint == parent) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_ARGUMENT);

        return false;
    }

    if (checkpoint->state ==
        VITTE_CHECKPOINT_STATE_ACTIVE) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_STATE);

        return false;
    }

    if (!vitte_checkpoint_is_active(
            parent)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_STATE);

        return false;
    }

    if (!vitte_checkpoint_active_snapshot_valid(
            parent) ||
        !vitte_checkpoint_parent_chain_acyclic(
            parent)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_STALE);

        return false;
    }

    if (parent->depth >=
        VITTE_CHECKPOINT_MAX_DEPTH) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_DEPTH_EXCEEDED);

        return false;
    }

    arena =
        parent->arena;

    if (arena == NULL ||
        !vitte_arena_context_is_valid(
            arena)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_INVALID_ARENA);

        return false;
    }

    if (parent->generation !=
        vitte_arena_context_generation(
            arena)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_GENERATION_MISMATCH);

        return false;
    }

    if (!vitte_checkpoint_capture(
            checkpoint,
            arena)) {
        return false;
    }

    checkpoint->parent =
        parent;

    checkpoint->depth =
        parent->depth + 1u;

    if (checkpoint->depth >
        VITTE_CHECKPOINT_MAX_DEPTH ||
        !vitte_checkpoint_parent_chain_acyclic(
            checkpoint)) {
        vitte_checkpoint_clear_transaction(
            checkpoint);

        checkpoint->state =
            VITTE_CHECKPOINT_STATE_INVALIDATED;

        checkpoint->last_error =
            VITTE_CHECKPOINT_ERROR_CORRUPTION;

        return false;
    }

    return true;
}

/* ========================================================================= */
/* Staleness                                                                 */
/* ========================================================================= */

bool
vitte_checkpoint_is_stale(
    const vitte_checkpoint_t *checkpoint)
{
    const vitte_checkpoint_t *parent;

    if (!vitte_checkpoint_is_active(
            checkpoint)) {
        return true;
    }

    if (!vitte_checkpoint_active_snapshot_valid(
            checkpoint)) {
        return true;
    }

    if (!vitte_checkpoint_parent_chain_acyclic(
            checkpoint)) {
        return true;
    }

    parent =
        checkpoint->parent;

    if (parent != NULL) {
        if (!vitte_checkpoint_is_active(
                parent)) {
            return true;
        }

        if (parent->arena !=
            checkpoint->arena) {
            return true;
        }

        if (parent->generation !=
            checkpoint->generation) {
            return true;
        }

        if (parent->depth ==
            SIZE_MAX) {
            return true;
        }

        if (checkpoint->depth !=
            parent->depth + 1u) {
            return true;
        }

        /*
         * A nested checkpoint must have been captured after its parent.
         *
         * Sequence wrap makes raw sequence ordering unsuitable as a permanent
         * correctness invariant, so depth/parent ownership remains the
         * authoritative relationship.
         */
    } else if (checkpoint->depth != 0u) {
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

    if (!vitte_checkpoint_state_valid(
            checkpoint->state)) {
        return false;
    }

    if (!vitte_checkpoint_error_valid(
            checkpoint->last_error)) {
        return false;
    }

    switch (checkpoint->state) {
        case VITTE_CHECKPOINT_STATE_EMPTY:
            return
                checkpoint->arena == NULL &&
                checkpoint->parent == NULL &&
                checkpoint->generation == UINT64_C(0) &&
                checkpoint->sequence == UINT64_C(0) &&
                checkpoint->depth == 0u &&
                checkpoint->allocation_count == 0u &&
                checkpoint->requested_bytes == 0u &&
                checkpoint->used_bytes == 0u &&
                checkpoint->reserved_bytes == 0u;

        case VITTE_CHECKPOINT_STATE_ACTIVE:
            break;

        case VITTE_CHECKPOINT_STATE_COMMITTED:
        case VITTE_CHECKPOINT_STATE_ROLLED_BACK:
        case VITTE_CHECKPOINT_STATE_INVALIDATED:
            /*
             * Resolved checkpoints intentionally retain their capture
             * metadata for diagnostics. Their arena may subsequently reset or
             * be destroyed, so current-arena validation must not be required.
             */
            if (checkpoint->sequence ==
                    UINT64_C(0) &&
                checkpoint->arena != NULL) {
                return false;
            }

            if (checkpoint->depth >
                VITTE_CHECKPOINT_MAX_DEPTH) {
                return false;
            }

            return true;

        default:
            return false;
    }

    if (!vitte_checkpoint_active_snapshot_valid(
            checkpoint)) {
        return false;
    }

    if (!vitte_checkpoint_parent_chain_acyclic(
            checkpoint)) {
        return false;
    }

    if (checkpoint->parent != NULL) {
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

        if (checkpoint->parent->depth ==
            SIZE_MAX) {
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
    vitte_arena_checkpoint_t snapshot;

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

    if (!vitte_checkpoint_parent_chain_acyclic(
            checkpoint)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_CORRUPTION);

        return false;
    }

    if (checkpoint->parent != NULL) {
        if (!vitte_checkpoint_is_active(
                checkpoint->parent)) {
            vitte_checkpoint_set_error(
                checkpoint,
                VITTE_CHECKPOINT_ERROR_STALE);

            return false;
        }

        if (checkpoint->parent->arena !=
            checkpoint->arena) {
            vitte_checkpoint_set_error(
                checkpoint,
                VITTE_CHECKPOINT_ERROR_FOREIGN_ARENA);

            return false;
        }

        if (checkpoint->parent->generation !=
            checkpoint->generation) {
            vitte_checkpoint_set_error(
                checkpoint,
                VITTE_CHECKPOINT_ERROR_GENERATION_MISMATCH);

            return false;
        }
    }

    /*
     * A transaction may only move the arena forward relative to its capture.
     * If current accounting is already behind the snapshot, the checkpoint is
     * stale or arena accounting has been modified outside checkpoint rules.
     */
    if (checkpoint->allocation_count >
            vitte_arena_context_allocation_count(
                checkpoint->arena) ||
        checkpoint->requested_bytes >
            vitte_arena_context_requested_bytes(
                checkpoint->arena) ||
        checkpoint->used_bytes >
            vitte_arena_context_used_bytes(
                checkpoint->arena)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_STALE);

        return false;
    }

    memset(
        &snapshot,
        0,
        sizeof(snapshot));

    snapshot.context =
        checkpoint->arena;

    snapshot.mark =
        checkpoint->mark;

    snapshot.generation =
        checkpoint->generation;

    snapshot.allocation_count =
        checkpoint->allocation_count;

    snapshot.requested_bytes =
        checkpoint->requested_bytes;

    if (!vitte_arena_context_rollback(
            checkpoint->arena,
            snapshot)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_ROLLBACK_FAILED);

        return false;
    }

    /*
     * Verify the logical postcondition. Physical used bytes may differ from
     * simplistic assumptions when allocator internals release dedicated
     * blocks, therefore arena.c remains authoritative for physical topology.
     */
    if (vitte_arena_context_generation(
            checkpoint->arena) !=
            checkpoint->generation ||
        vitte_arena_context_allocation_count(
            checkpoint->arena) !=
            checkpoint->allocation_count ||
        vitte_arena_context_requested_bytes(
            checkpoint->arena) !=
            checkpoint->requested_bytes) {
        checkpoint->state =
            VITTE_CHECKPOINT_STATE_INVALIDATED;

        checkpoint->last_error =
            VITTE_CHECKPOINT_ERROR_CORRUPTION;

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

    if (!vitte_checkpoint_parent_chain_acyclic(
            checkpoint)) {
        vitte_checkpoint_set_error(
            checkpoint,
            VITTE_CHECKPOINT_ERROR_CORRUPTION);

        return false;
    }

    if (checkpoint->parent != NULL) {
        if (!vitte_checkpoint_is_active(
                checkpoint->parent)) {
            vitte_checkpoint_set_error(
                checkpoint,
                VITTE_CHECKPOINT_ERROR_STALE);

            return false;
        }

        if (checkpoint->parent->arena !=
            checkpoint->arena) {
            vitte_checkpoint_set_error(
                checkpoint,
                VITTE_CHECKPOINT_ERROR_FOREIGN_ARENA);

            return false;
        }

        if (checkpoint->parent->generation !=
            checkpoint->generation) {
            vitte_checkpoint_set_error(
                checkpoint,
                VITTE_CHECKPOINT_ERROR_GENERATION_MISMATCH);

            return false;
        }
    }

    /*
     * Commit performs no allocator operation.
     *
     * All post-capture allocations already belong to the arena. Committing
     * merely resolves the rollback capability represented by this object.
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
/* Reset                                                                     */
/* ========================================================================= */

void
vitte_checkpoint_reset(
    vitte_checkpoint_t *checkpoint)
{
    if (checkpoint == NULL) {
        return;
    }

    /*
     * Reset deliberately abandons any active transaction.
     *
     * This is different from rollback.
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

    /*
     * Destroying an active checkpoint intentionally abandons its rollback
     * capability. Arena memory itself is untouched.
     */
    memset(
        checkpoint,
        0,
        sizeof(*checkpoint));

    checkpoint->magic =
        VITTE_CHECKPOINT_DEAD_MAGIC;

    checkpoint->state =
        VITTE_CHECKPOINT_STATE_INVALIDATED;

    checkpoint->last_error =
        VITTE_CHECKPOINT_ERROR_INVALID_CHECKPOINT;
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

    return
        checkpoint->last_error;
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
/* Arena access                                                              */
/* ========================================================================= */

vitte_arena_context_t *
vitte_checkpoint_arena(
    vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return NULL;
    }

    return
        checkpoint->arena;
}

const vitte_arena_context_t *
vitte_checkpoint_arena_const(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return NULL;
    }

    return
        checkpoint->arena;
}

/* ========================================================================= */
/* Parent access                                                             */
/* ========================================================================= */

vitte_checkpoint_t *
vitte_checkpoint_parent(
    vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return NULL;
    }

    return
        checkpoint->parent;
}

const vitte_checkpoint_t *
vitte_checkpoint_parent_const(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return NULL;
    }

    return
        checkpoint->parent;
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
        return UINT64_C(0);
    }

    return
        checkpoint->sequence;
}

uint64_t
vitte_checkpoint_generation(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return UINT64_C(0);
    }

    return
        checkpoint->generation;
}

size_t
vitte_checkpoint_depth(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return
        checkpoint->depth;
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
        checkpoint->arena == NULL ||
        !vitte_arena_context_is_valid(
            checkpoint->arena) ||
        checkpoint->generation !=
            vitte_arena_context_generation(
                checkpoint->arena)) {
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
        checkpoint->arena == NULL ||
        !vitte_arena_context_is_valid(
            checkpoint->arena) ||
        checkpoint->generation !=
            vitte_arena_context_generation(
                checkpoint->arena)) {
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
        checkpoint->arena == NULL ||
        !vitte_arena_context_is_valid(
            checkpoint->arena) ||
        checkpoint->generation !=
            vitte_arena_context_generation(
                checkpoint->arena)) {
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
        checkpoint->arena == NULL ||
        !vitte_arena_context_is_valid(
            checkpoint->arena) ||
        checkpoint->generation !=
            vitte_arena_context_generation(
                checkpoint->arena)) {
        return 0u;
    }

    current =
        vitte_arena_context_reserved_bytes(
            checkpoint->arena);

    if (!vitte_checkpoint_size_sub(
            current,
            checkpoint->reserved_bytes,
            &result)) {
        /*
         * Reserved capacity can decrease if callers perform trim operations.
         * The unsigned delta API cannot represent a negative value.
         */
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
    bool arena_current;

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

    stats.has_parent =
        checkpoint->parent != NULL;

    arena_current =
        checkpoint->state ==
            VITTE_CHECKPOINT_STATE_ACTIVE &&
        checkpoint->arena != NULL &&
        vitte_arena_context_is_valid(
            checkpoint->arena) &&
        checkpoint->generation ==
            vitte_arena_context_generation(
                checkpoint->arena);

    if (arena_current) {
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
        /*
         * For resolved checkpoints, preserve the captured snapshot rather than
         * exposing unrelated later arena state.
         */
        stats.current_allocation_count =
            checkpoint->allocation_count;

        stats.current_requested_bytes =
            checkpoint->requested_bytes;

        stats.current_used_bytes =
            checkpoint->used_bytes;

        stats.current_reserved_bytes =
            checkpoint->reserved_bytes;

        stats.allocation_delta = 0u;
        stats.requested_delta = 0u;
        stats.used_delta = 0u;
        stats.reserved_delta = 0u;

        stats.stale =
            checkpoint->state ==
                VITTE_CHECKPOINT_STATE_ACTIVE;
    }

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
    if (!vitte_checkpoint_magic_valid(
            left) ||
        !vitte_checkpoint_magic_valid(
            right)) {
        return false;
    }

    if (left->arena == NULL ||
        right->arena == NULL) {
        return false;
    }

    return
        left->arena ==
        right->arena;
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

    if (left->generation ==
            UINT64_C(0) ||
        right->generation ==
            UINT64_C(0)) {
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

    if (left == right) {
        return false;
    }

    if (left->sequence ==
            UINT64_C(0) ||
        right->sequence ==
            UINT64_C(0)) {
        return false;
    }

    /*
     * Sequence wrap is extraordinarily rare, but sequence numbers are
     * diagnostic identifiers rather than an eternal total-order primitive.
     *
     * Within a normal non-wrapped generation, direct ordering is exact.
     */
    return
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

    if (ancestor == checkpoint) {
        return false;
    }

    if (ancestor->arena == NULL ||
        checkpoint->arena == NULL ||
        ancestor->arena !=
            checkpoint->arena ||
        ancestor->generation !=
            checkpoint->generation) {
        return false;
    }

    current =
        checkpoint->parent;

    guard = 0u;

    while (current != NULL) {
        if (current == ancestor) {
            return true;
        }

        if (!vitte_checkpoint_magic_valid(
                current)) {
            return false;
        }

        if (current->arena !=
                checkpoint->arena ||
            current->generation !=
                checkpoint->generation) {
            return false;
        }

        current =
            current->parent;

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
    size_t expected_depth;
    size_t guard;

    if (!vitte_checkpoint_magic_valid(
            checkpoint) ||
        !vitte_checkpoint_parent_chain_acyclic(
            checkpoint)) {
        return NULL;
    }

    current =
        checkpoint;

    expected_depth =
        checkpoint->depth;

    guard = 0u;

    for (;;) {
        if (current->depth !=
            expected_depth) {
            return NULL;
        }

        if (current->parent == NULL) {
            return
                current->depth == 0u
                    ? current
                    : NULL;
        }

        if (expected_depth == 0u) {
            return NULL;
        }

        if (!vitte_checkpoint_magic_valid(
                current->parent)) {
            return NULL;
        }

        if (current->arena !=
                current->parent->arena ||
            current->generation !=
                current->parent->generation) {
            return NULL;
        }

        --expected_depth;

        current =
            current->parent;

        ++guard;

        if (guard >
            VITTE_CHECKPOINT_MAX_DEPTH) {
            return NULL;
        }
    }
}

const vitte_checkpoint_t *
vitte_checkpoint_root_const(
    const vitte_checkpoint_t *checkpoint)
{
    const vitte_checkpoint_t *current;
    size_t expected_depth;
    size_t guard;

    if (!vitte_checkpoint_magic_valid(
            checkpoint) ||
        !vitte_checkpoint_parent_chain_acyclic(
            checkpoint)) {
        return NULL;
    }

    current =
        checkpoint;

    expected_depth =
        checkpoint->depth;

    guard = 0u;

    for (;;) {
        if (current->depth !=
            expected_depth) {
            return NULL;
        }

        if (current->parent == NULL) {
            return
                current->depth == 0u
                    ? current
                    : NULL;
        }

        if (expected_depth == 0u) {
            return NULL;
        }

        if (!vitte_checkpoint_magic_valid(
                current->parent)) {
            return NULL;
        }

        if (current->arena !=
                current->parent->arena ||
            current->generation !=
                current->parent->generation) {
            return NULL;
        }

        --expected_depth;

        current =
            current->parent;

        ++guard;

        if (guard >
            VITTE_CHECKPOINT_MAX_DEPTH) {
            return NULL;
        }
    }
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

    if (!vitte_checkpoint_parent_chain_acyclic(
            checkpoint)) {
        return false;
    }

    arena =
        checkpoint->arena;

    generation =
        checkpoint->generation;

    expected_depth =
        checkpoint->depth;

    if (expected_depth >
        VITTE_CHECKPOINT_MAX_DEPTH) {
        return false;
    }

    current =
        checkpoint;

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
            return
                current->depth == 0u &&
                expected_depth == 0u;
        }

        if (expected_depth == 0u) {
            return false;
        }

        if (!vitte_checkpoint_magic_valid(
                current->parent)) {
            return false;
        }

        if (current->parent->arena !=
                arena ||
            current->parent->generation !=
                generation) {
            return false;
        }

        --expected_depth;

        current =
            current->parent;

        ++guard;

        if (guard >
            VITTE_CHECKPOINT_MAX_DEPTH) {
            return false;
        }
    }

    return false;
}

/* ========================================================================= */
/* Commit / rollback helper                                                  */
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

    return
        checkpoint->allocation_count;
}

size_t
vitte_checkpoint_captured_requested_bytes(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return
        checkpoint->requested_bytes;
}

size_t
vitte_checkpoint_captured_used_bytes(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return
        checkpoint->used_bytes;
}

size_t
vitte_checkpoint_captured_reserved_bytes(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_magic_valid(
            checkpoint)) {
        return 0u;
    }

    return
        checkpoint->reserved_bytes;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
