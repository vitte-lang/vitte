#ifndef VITTE_SRC_ARENA_CHECKPOINT_H
#define VITTE_SRC_ARENA_CHECKPOINT_H

/*
 * Vitte Compiler
 * src/arena/checkpoint.h
 *
 * Transactional checkpoints for compiler arenas.
 *
 * Architecture:
 *
 *     block.c/.h
 *         physical arena blocks
 *
 *     allocator.c/.h
 *         block management and low-level marks
 *
 *     arena.c/.h
 *         compiler-facing arena context
 *
 *     checkpoint.c/.h
 *         transactional allocation checkpoints
 *
 * Checkpoints are intended for speculative compiler operations:
 *
 *     - parser branches
 *     - speculative syntax recognition
 *     - semantic probes
 *     - temporary type inference
 *     - overload resolution
 *     - generic instantiation
 *     - macro expansion
 *     - HIR lowering
 *     - IR construction
 *     - optimization passes
 *
 * A checkpoint captures the arena allocation state.
 *
 * The transaction may then be:
 *
 *     committed
 *
 *         allocations remain valid;
 *         checkpoint becomes resolved.
 *
 *     rolled back
 *
 *         allocations performed after capture become invalid;
 *         arena accounting is restored.
 *
 * Checkpoints are tied to:
 *
 *     - one arena context
 *     - one arena generation
 *     - one capture position
 *
 * Arena reset invalidates active checkpoints from the previous generation.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "allocator.h"
#include "arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_CHECKPOINT_MAX_DEPTH
#define VITTE_CHECKPOINT_MAX_DEPTH \
    ((size_t)1024u)
#endif

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

typedef enum vitte_checkpoint_state {
    /*
     * Initialized checkpoint with no active transaction.
     */
    VITTE_CHECKPOINT_STATE_EMPTY = 0,

    /*
     * Transaction is active and may be committed or rolled back.
     */
    VITTE_CHECKPOINT_STATE_ACTIVE = 1,

    /*
     * Transaction was accepted.
     */
    VITTE_CHECKPOINT_STATE_COMMITTED = 2,

    /*
     * Transaction was reverted.
     */
    VITTE_CHECKPOINT_STATE_ROLLED_BACK = 3,

    /*
     * Transaction/checkpoint was explicitly invalidated.
     */
    VITTE_CHECKPOINT_STATE_INVALIDATED = 4
} vitte_checkpoint_state_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_checkpoint_error {
    VITTE_CHECKPOINT_ERROR_NONE = 0,

    /*
     * NULL or otherwise invalid function argument.
     */
    VITTE_CHECKPOINT_ERROR_INVALID_ARGUMENT,

    /*
     * Arena context is invalid or unavailable.
     */
    VITTE_CHECKPOINT_ERROR_INVALID_ARENA,

    /*
     * Checkpoint object is invalid.
     */
    VITTE_CHECKPOINT_ERROR_INVALID_CHECKPOINT,

    /*
     * Operation is incompatible with checkpoint state.
     *
     * Example:
     *
     *     commit() on an already committed checkpoint.
     */
    VITTE_CHECKPOINT_ERROR_INVALID_STATE,

    /*
     * Checkpoint can no longer safely represent its original transaction.
     */
    VITTE_CHECKPOINT_ERROR_STALE,

    /*
     * Checkpoint belongs to another arena.
     */
    VITTE_CHECKPOINT_ERROR_FOREIGN_ARENA,

    /*
     * Arena generation changed after capture.
     */
    VITTE_CHECKPOINT_ERROR_GENERATION_MISMATCH,

    /*
     * Nested checkpoint depth exceeds VITTE_CHECKPOINT_MAX_DEPTH.
     */
    VITTE_CHECKPOINT_ERROR_DEPTH_EXCEEDED,

    /*
     * Underlying arena rollback failed.
     */
    VITTE_CHECKPOINT_ERROR_ROLLBACK_FAILED,

    /*
     * Internal checkpoint/arena invariant failure.
     */
    VITTE_CHECKPOINT_ERROR_CORRUPTION
} vitte_checkpoint_error_t;

/* ========================================================================= */
/* Forward declaration                                                       */
/* ========================================================================= */

typedef struct vitte_checkpoint
    vitte_checkpoint_t;

/* ========================================================================= */
/* Checkpoint                                                                */
/* ========================================================================= */

struct vitte_checkpoint {
    /*
     * Runtime integrity cookie.
     */
    uint64_t magic;

    /*
     * Transaction state.
     */
    vitte_checkpoint_state_t state;

    /*
     * Last checkpoint-local error.
     */
    vitte_checkpoint_error_t last_error;

    /*
     * Arena context owning this transaction.
     *
     * Non-owning pointer.
     */
    vitte_arena_context_t *arena;

    /*
     * Low-level allocator position.
     */
    vitte_arena_mark_t mark;

    /*
     * Arena generation at capture time.
     */
    uint64_t generation;

    /*
     * High-level accounting snapshot.
     */
    size_t allocation_count;
    size_t requested_bytes;

    /*
     * Physical allocator statistics captured for diagnostics/delta queries.
     */
    size_t used_bytes;
    size_t reserved_bytes;

    /*
     * Process-local checkpoint creation identifier.
     *
     * Zero means no sequence.
     */
    uint64_t sequence;

    /*
     * Nested transaction depth.
     *
     * Root checkpoint:
     *
     *     depth == 0
     *
     * Child:
     *
     *     depth == parent->depth + 1
     */
    size_t depth;

    /*
     * Parent transaction.
     *
     * Non-owning pointer.
     *
     * NULL for a root checkpoint.
     */
    vitte_checkpoint_t *parent;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_checkpoint_stats {
    /*
     * Current checkpoint state/error.
     */
    vitte_checkpoint_state_t state;
    vitte_checkpoint_error_t last_error;

    /*
     * Identity.
     */
    uint64_t sequence;
    uint64_t generation;

    /*
     * Nested transaction depth.
     */
    size_t depth;

    /*
     * Snapshot captured at transaction start.
     */
    size_t captured_allocation_count;
    size_t captured_requested_bytes;
    size_t captured_used_bytes;
    size_t captured_reserved_bytes;

    /*
     * Current arena state when the transaction remains active.
     *
     * For a resolved checkpoint, checkpoint.c reports the captured values.
     */
    size_t current_allocation_count;
    size_t current_requested_bytes;
    size_t current_used_bytes;
    size_t current_reserved_bytes;

    /*
     * Changes since capture.
     */
    size_t allocation_delta;
    size_t requested_delta;
    size_t used_delta;
    size_t reserved_delta;

    /*
     * Relationship/lifetime information.
     */
    bool stale;
    bool has_parent;
} vitte_checkpoint_stats_t;

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

/*
 * Initialize checkpoint storage.
 *
 * Resulting state:
 *
 *     VITTE_CHECKPOINT_STATE_EMPTY
 *
 * The checkpoint may subsequently be passed to capture().
 */
void
vitte_checkpoint_init(
    vitte_checkpoint_t *checkpoint);

/*
 * Destroy checkpoint metadata.
 *
 * This does NOT roll back the arena.
 *
 * Destroying an ACTIVE checkpoint therefore explicitly abandons the ability
 * to roll that transaction back through this object.
 */
void
vitte_checkpoint_destroy(
    vitte_checkpoint_t *checkpoint);

/*
 * Reset checkpoint metadata to EMPTY.
 *
 * Like destroy(), this does not automatically roll back an active
 * transaction.
 */
void
vitte_checkpoint_reset(
    vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Capture                                                                   */
/* ========================================================================= */

/*
 * Begin a transaction by capturing arena state.
 *
 * checkpoint may contain zeroed/uninitialized storage; capture() initializes
 * it automatically when necessary.
 *
 * An already ACTIVE checkpoint cannot be overwritten.
 */
bool
vitte_checkpoint_capture(
    vitte_checkpoint_t *checkpoint,
    vitte_arena_context_t *arena);

/*
 * Convenience synonym for capture().
 */
bool
vitte_checkpoint_begin(
    vitte_checkpoint_t *checkpoint,
    vitte_arena_context_t *arena);

/* ========================================================================= */
/* Nested capture                                                            */
/* ========================================================================= */

/*
 * Begin a nested transaction.
 *
 * parent must:
 *
 *     - be initialized
 *     - be ACTIVE
 *     - reference a valid arena
 *     - remain in the same generation
 *
 * The child uses the same arena.
 *
 * Maximum depth is VITTE_CHECKPOINT_MAX_DEPTH.
 */
bool
vitte_checkpoint_capture_nested(
    vitte_checkpoint_t *checkpoint,
    vitte_checkpoint_t *parent);

/* ========================================================================= */
/* Commit                                                                    */
/* ========================================================================= */

/*
 * Accept allocations made after capture.
 *
 * Commit is logical only:
 *
 *     - arena memory is unchanged
 *     - allocations remain valid
 *     - checkpoint becomes COMMITTED
 *
 * Returns false when the checkpoint is stale, resolved, invalid, or its arena
 * generation no longer matches.
 */
bool
vitte_checkpoint_commit(
    vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Rollback                                                                  */
/* ========================================================================= */

/*
 * Restore arena state captured by this checkpoint.
 *
 * Allocations made after capture become invalid.
 *
 * Restores:
 *
 *     - allocator bump position
 *     - current allocation count
 *     - current requested-byte count
 *
 * Lifetime statistics intentionally remain cumulative.
 */
bool
vitte_checkpoint_rollback(
    vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Finish                                                                    */
/* ========================================================================= */

/*
 * Resolve a transaction.
 *
 * commit == true
 *     -> vitte_checkpoint_commit()
 *
 * commit == false
 *     -> vitte_checkpoint_rollback()
 */
bool
vitte_checkpoint_finish(
    vitte_checkpoint_t *checkpoint,
    bool commit);

/* ========================================================================= */
/* Explicit invalidation                                                     */
/* ========================================================================= */

/*
 * Mark a checkpoint unusable without changing arena memory.
 */
void
vitte_checkpoint_invalidate(
    vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* State queries                                                             */
/* ========================================================================= */

bool
vitte_checkpoint_is_initialized(
    const vitte_checkpoint_t *checkpoint);

bool
vitte_checkpoint_is_active(
    const vitte_checkpoint_t *checkpoint);

bool
vitte_checkpoint_is_committed(
    const vitte_checkpoint_t *checkpoint);

bool
vitte_checkpoint_is_rolled_back(
    const vitte_checkpoint_t *checkpoint);

bool
vitte_checkpoint_is_invalidated(
    const vitte_checkpoint_t *checkpoint);

/*
 * An active checkpoint is stale when its arena is unavailable/invalid or its
 * generation no longer matches.
 *
 * Non-active checkpoints are considered stale by the current implementation.
 */
bool
vitte_checkpoint_is_stale(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

vitte_checkpoint_error_t
vitte_checkpoint_last_error(
    const vitte_checkpoint_t *checkpoint);

void
vitte_checkpoint_clear_error(
    vitte_checkpoint_t *checkpoint);

bool
vitte_checkpoint_has_error(
    const vitte_checkpoint_t *checkpoint);

const char *
vitte_checkpoint_error_name(
    vitte_checkpoint_error_t error);

/* ========================================================================= */
/* State names                                                               */
/* ========================================================================= */

const char *
vitte_checkpoint_state_name(
    vitte_checkpoint_state_t state);

/* ========================================================================= */
/* Arena access                                                              */
/* ========================================================================= */

vitte_arena_context_t *
vitte_checkpoint_arena(
    vitte_checkpoint_t *checkpoint);

const vitte_arena_context_t *
vitte_checkpoint_arena_const(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Parent access                                                             */
/* ========================================================================= */

vitte_checkpoint_t *
vitte_checkpoint_parent(
    vitte_checkpoint_t *checkpoint);

const vitte_checkpoint_t *
vitte_checkpoint_parent_const(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

uint64_t
vitte_checkpoint_sequence(
    const vitte_checkpoint_t *checkpoint);

uint64_t
vitte_checkpoint_generation(
    const vitte_checkpoint_t *checkpoint);

size_t
vitte_checkpoint_depth(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Captured snapshot                                                         */
/* ========================================================================= */

size_t
vitte_checkpoint_captured_allocation_count(
    const vitte_checkpoint_t *checkpoint);

size_t
vitte_checkpoint_captured_requested_bytes(
    const vitte_checkpoint_t *checkpoint);

size_t
vitte_checkpoint_captured_used_bytes(
    const vitte_checkpoint_t *checkpoint);

size_t
vitte_checkpoint_captured_reserved_bytes(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Delta queries                                                             */
/* ========================================================================= */

/*
 * Number of successful arena allocations performed since capture.
 *
 * Returns zero for invalid/resolved checkpoints or inconsistent accounting.
 */
size_t
vitte_checkpoint_allocation_delta(
    const vitte_checkpoint_t *checkpoint);

/*
 * Logical requested-byte increase since capture.
 */
size_t
vitte_checkpoint_requested_delta(
    const vitte_checkpoint_t *checkpoint);

/*
 * Physical allocator used-byte increase since capture.
 */
size_t
vitte_checkpoint_used_delta(
    const vitte_checkpoint_t *checkpoint);

/*
 * Reserved-capacity increase since capture.
 */
size_t
vitte_checkpoint_reserved_delta(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_checkpoint_stats_t
vitte_checkpoint_stats(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Comparison                                                                */
/* ========================================================================= */

/*
 * Return true when both checkpoints belong to the same non-NULL arena.
 */
bool
vitte_checkpoint_same_arena(
    const vitte_checkpoint_t *left,
    const vitte_checkpoint_t *right);

/*
 * Return true when both checkpoints belong to the same arena and generation.
 */
bool
vitte_checkpoint_same_generation(
    const vitte_checkpoint_t *left,
    const vitte_checkpoint_t *right);

/*
 * Compare checkpoint creation order.
 *
 * This is sequence order, not physical memory order.
 */
bool
vitte_checkpoint_precedes(
    const vitte_checkpoint_t *left,
    const vitte_checkpoint_t *right);

/* ========================================================================= */
/* Hierarchy                                                                 */
/* ========================================================================= */

/*
 * Return true when ancestor occurs in checkpoint's parent chain.
 *
 * A checkpoint is not considered its own ancestor.
 */
bool
vitte_checkpoint_is_ancestor_of(
    const vitte_checkpoint_t *ancestor,
    const vitte_checkpoint_t *checkpoint);

/*
 * Return the root of a nested checkpoint chain.
 *
 * Returns NULL if corruption is detected.
 */
vitte_checkpoint_t *
vitte_checkpoint_root(
    vitte_checkpoint_t *checkpoint);

const vitte_checkpoint_t *
vitte_checkpoint_root_const(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate one checkpoint and its direct relationship to the arena/parent.
 */
bool
vitte_checkpoint_validate(
    const vitte_checkpoint_t *checkpoint);

/*
 * Validate the complete parent chain.
 *
 * Checks:
 *
 *     - checkpoint magic
 *     - common arena
 *     - common generation
 *     - monotonically decreasing depth
 *     - root depth == 0
 *     - maximum traversal depth
 */
bool
vitte_checkpoint_validate_chain(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Convenience predicates                                                    */
/* ========================================================================= */

static inline bool
vitte_checkpoint_is_empty(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_is_initialized(
            checkpoint) &&
        checkpoint->state ==
            VITTE_CHECKPOINT_STATE_EMPTY;
}

static inline bool
vitte_checkpoint_is_resolved(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_is_initialized(
            checkpoint)) {
        return false;
    }

    return
        checkpoint->state ==
            VITTE_CHECKPOINT_STATE_COMMITTED ||
        checkpoint->state ==
            VITTE_CHECKPOINT_STATE_ROLLED_BACK ||
        checkpoint->state ==
            VITTE_CHECKPOINT_STATE_INVALIDATED;
}

static inline bool
vitte_checkpoint_has_parent(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_is_initialized(
            checkpoint) &&
        checkpoint->parent != NULL;
}

static inline bool
vitte_checkpoint_is_root(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_is_initialized(
            checkpoint) &&
        checkpoint->parent == NULL &&
        checkpoint->depth == 0u;
}

static inline bool
vitte_checkpoint_has_allocations_since_capture(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_allocation_delta(
            checkpoint) != 0u;
}

static inline bool
vitte_checkpoint_has_memory_since_capture(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_requested_delta(
            checkpoint) != 0u;
}

/* ========================================================================= */
/* Transaction macros                                                        */
/* ========================================================================= */

#define VITTE_CHECKPOINT_BEGIN(checkpoint, arena) \
    vitte_checkpoint_begin( \
        (checkpoint), \
        (arena))

#define VITTE_CHECKPOINT_COMMIT(checkpoint) \
    vitte_checkpoint_commit( \
        (checkpoint))

#define VITTE_CHECKPOINT_ROLLBACK(checkpoint) \
    vitte_checkpoint_rollback( \
        (checkpoint))

#define VITTE_CHECKPOINT_FINISH(checkpoint, commit) \
    vitte_checkpoint_finish( \
        (checkpoint), \
        (commit))

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_CHECKPOINT_MAX_DEPTH != 0u,
    "checkpoint maximum depth must not be zero");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte checkpoints require 64-bit uint64_t");

_Static_assert(
    VITTE_CHECKPOINT_STATE_EMPTY == 0,
    "checkpoint empty state must remain zero");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_NONE == 0,
    "checkpoint no-error value must remain zero");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_CHECKPOINT_H */
