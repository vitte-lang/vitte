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
 *         block-chain management and low-level allocator marks
 *
 *     arena.c/.h
 *         compiler-facing arena context and accounting
 *
 *     checkpoint.c/.h
 *         transactional capture / commit / rollback
 *
 * Checkpoints provide lightweight transactional allocation semantics for
 * speculative compiler operations such as:
 *
 *     - parser alternatives
 *     - speculative syntax recognition
 *     - semantic probes
 *     - overload resolution
 *     - temporary type inference
 *     - generic instantiation
 *     - macro expansion
 *     - HIR construction
 *     - IR construction
 *     - optimization passes
 *
 * A checkpoint captures:
 *
 *     - arena ownership
 *     - arena generation
 *     - low-level allocator mark
 *     - allocation count
 *     - logical requested bytes
 *     - physical used bytes
 *     - reserved capacity
 *     - hierarchy metadata
 *     - diagnostic sequence identifier
 *
 * Transactions may then be:
 *
 *     committed
 *
 *         All allocations remain valid.
 *         No allocator operation is required.
 *
 *     rolled back
 *
 *         Allocations performed after capture become invalid.
 *         The allocator mark and logical arena accounting are restored.
 *
 * Checkpoints are bound to exactly one arena generation. Resetting the arena
 * invalidates active checkpoints captured in the previous generation.
 *
 * Nested checkpoints:
 *
 *     root
 *       |
 *       +-- child
 *             |
 *             +-- grandchild
 *
 * Nested transactions must be resolved in LIFO order.
 *
 * checkpoint.c intentionally does not maintain mutable child lists. Therefore
 * the child -> parent relationship is explicit, while discovering arbitrary
 * live descendants from a parent is not supported.
 *
 * Threading:
 *
 * Arena mutation and checkpoint mutation are not internally synchronized.
 * Sharing them between threads requires external synchronization.
 *
 * ISO C17.
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
     * Initialized object with no captured transaction.
     */
    VITTE_CHECKPOINT_STATE_EMPTY = 0,

    /*
     * Transaction is active and may be committed or rolled back.
     */
    VITTE_CHECKPOINT_STATE_ACTIVE = 1,

    /*
     * Transaction was accepted.
     *
     * Allocations remain owned by the arena.
     */
    VITTE_CHECKPOINT_STATE_COMMITTED = 2,

    /*
     * Transaction was reverted to its captured arena position.
     */
    VITTE_CHECKPOINT_STATE_ROLLED_BACK = 3,

    /*
     * Transaction was explicitly abandoned or invalidated.
     *
     * Arena memory is not modified merely by entering this state.
     */
    VITTE_CHECKPOINT_STATE_INVALIDATED = 4
} vitte_checkpoint_state_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_checkpoint_error {
    /*
     * No checkpoint-local error.
     */
    VITTE_CHECKPOINT_ERROR_NONE = 0,

    /*
     * NULL, self-parenting, or otherwise invalid argument.
     */
    VITTE_CHECKPOINT_ERROR_INVALID_ARGUMENT = 1,

    /*
     * Arena context is invalid, unavailable, or destroyed.
     */
    VITTE_CHECKPOINT_ERROR_INVALID_ARENA = 2,

    /*
     * Checkpoint storage/object is invalid.
     */
    VITTE_CHECKPOINT_ERROR_INVALID_CHECKPOINT = 3,

    /*
     * Operation is incompatible with the current checkpoint state.
     *
     * Examples:
     *
     *     commit(committed_checkpoint)
     *     rollback(empty_checkpoint)
     *     capture(active_checkpoint)
     */
    VITTE_CHECKPOINT_ERROR_INVALID_STATE = 4,

    /*
     * Checkpoint can no longer safely represent its captured transaction.
     */
    VITTE_CHECKPOINT_ERROR_STALE = 5,

    /*
     * A related checkpoint unexpectedly belongs to another arena.
     */
    VITTE_CHECKPOINT_ERROR_FOREIGN_ARENA = 6,

    /*
     * Arena generation changed after capture.
     */
    VITTE_CHECKPOINT_ERROR_GENERATION_MISMATCH = 7,

    /*
     * Nested checkpoint depth exceeds VITTE_CHECKPOINT_MAX_DEPTH.
     */
    VITTE_CHECKPOINT_ERROR_DEPTH_EXCEEDED = 8,

    /*
     * Underlying arena rollback operation failed.
     */
    VITTE_CHECKPOINT_ERROR_ROLLBACK_FAILED = 9,

    /*
     * Internal checkpoint, hierarchy, snapshot, or arena invariant failed.
     */
    VITTE_CHECKPOINT_ERROR_CORRUPTION = 10
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
     *
     * Managed exclusively by checkpoint.c.
     */
    uint64_t magic;

    /*
     * Current transaction state.
     */
    vitte_checkpoint_state_t state;

    /*
     * Most recent checkpoint-local error.
     */
    vitte_checkpoint_error_t last_error;

    /*
     * Arena context owning the captured transaction.
     *
     * Non-owning pointer.
     *
     * For ACTIVE checkpoints this must refer to a valid arena context.
     * Resolved checkpoints may retain it only as diagnostic metadata.
     */
    vitte_arena_context_t *arena;

    /*
     * Low-level allocator position captured at transaction start.
     */
    vitte_arena_mark_t mark;

    /*
     * Arena generation captured at transaction start.
     *
     * Zero means no active/captured generation.
     */
    uint64_t generation;

    /*
     * High-level logical arena accounting captured at transaction start.
     */
    size_t allocation_count;
    size_t requested_bytes;

    /*
     * Physical allocator statistics captured at transaction start.
     *
     * These values are primarily used for diagnostics and delta queries.
     *
     * Low-level rollback authority remains the allocator mark.
     */
    size_t used_bytes;
    size_t reserved_bytes;

    /*
     * Process-local capture identifier.
     *
     * Properties:
     *
     *     - non-zero for successfully captured checkpoints
     *     - thread-local
     *     - diagnostic/debug metadata
     *     - may eventually wrap
     *     - not serialized
     *     - not a memory position
     *     - not an arena generation
     */
    uint64_t sequence;

    /*
     * Nested transaction depth.
     *
     * Root:
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
     * NULL denotes a root checkpoint.
     *
     * Nested checkpoints are expected to be resolved in LIFO order.
     */
    vitte_checkpoint_t *parent;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_checkpoint_stats {
    /*
     * Checkpoint state at query time.
     */
    vitte_checkpoint_state_t state;

    /*
     * Checkpoint-local error at query time.
     */
    vitte_checkpoint_error_t last_error;

    /*
     * Capture identity.
     */
    uint64_t sequence;
    uint64_t generation;

    /*
     * Hierarchy depth.
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
     * Current arena accounting.
     *
     * For an ACTIVE checkpoint in its original generation these fields
     * describe the current arena.
     *
     * For resolved/stale checkpoints checkpoint.c reports the captured values
     * instead of exposing unrelated later arena state.
     */
    size_t current_allocation_count;
    size_t current_requested_bytes;
    size_t current_used_bytes;
    size_t current_reserved_bytes;

    /*
     * Unsigned increases since capture.
     *
     * A decrease or inconsistent accounting produces zero because these
     * fields intentionally do not encode signed deltas.
     */
    size_t allocation_delta;
    size_t requested_delta;
    size_t used_delta;
    size_t reserved_delta;

    /*
     * True when an ACTIVE checkpoint can no longer safely represent its
     * captured transaction.
     */
    bool stale;

    /*
     * True when parent != NULL.
     */
    bool has_parent;
} vitte_checkpoint_stats_t;

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

/*
 * Initialize checkpoint storage.
 *
 * Result:
 *
 *     state      = EMPTY
 *     last_error = NONE
 *     arena      = NULL
 *     parent     = NULL
 *     sequence   = 0
 *     generation = 0
 *     depth      = 0
 *
 * This operation does not touch arena memory.
 */
void
vitte_checkpoint_init(
    vitte_checkpoint_t *checkpoint);

/*
 * Destroy checkpoint metadata.
 *
 * This operation NEVER rolls back arena memory.
 *
 * Destroying an ACTIVE checkpoint therefore explicitly abandons the rollback
 * capability represented by this object.
 *
 * The object may later be initialized again.
 */
void
vitte_checkpoint_destroy(
    vitte_checkpoint_t *checkpoint);

/*
 * Reset checkpoint metadata to EMPTY.
 *
 * This operation does not roll back an active transaction.
 *
 * Use rollback() when transactional restoration is required.
 */
void
vitte_checkpoint_reset(
    vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Capture                                                                   */
/* ========================================================================= */

/*
 * Begin a root transaction by capturing arena state.
 *
 * checkpoint:
 *
 *     - must be non-NULL
 *     - may contain zeroed/uninitialized storage
 *     - may be a previously resolved checkpoint
 *     - must not already be ACTIVE
 *
 * arena:
 *
 *     - must be a valid vitte_arena_context_t
 *
 * On success:
 *
 *     state      = ACTIVE
 *     arena      = supplied arena
 *     generation = current arena generation
 *     depth      = 0
 *     parent     = NULL
 *     sequence   != 0
 *
 * On failure, false is returned and last_error is set whenever checkpoint
 * storage can safely represent an initialized checkpoint.
 */
bool
vitte_checkpoint_capture(
    vitte_checkpoint_t *checkpoint,
    vitte_arena_context_t *arena);

/*
 * Convenience synonym for vitte_checkpoint_capture().
 */
bool
vitte_checkpoint_begin(
    vitte_checkpoint_t *checkpoint,
    vitte_arena_context_t *arena);

/* ========================================================================= */
/* Nested capture                                                            */
/* ========================================================================= */

/*
 * Begin a transaction nested below parent.
 *
 * Requirements:
 *
 *     checkpoint != NULL
 *     parent != NULL
 *     checkpoint != parent
 *
 * parent must:
 *
 *     - be initialized
 *     - be ACTIVE
 *     - own a valid arena
 *     - belong to the current arena generation
 *     - have a structurally valid parent chain
 *
 * On success:
 *
 *     checkpoint->arena  == parent->arena
 *     checkpoint->parent == parent
 *     checkpoint->depth  == parent->depth + 1
 *
 * Maximum depth:
 *
 *     VITTE_CHECKPOINT_MAX_DEPTH
 *
 * Nested transactions must be resolved in LIFO order.
 */
bool
vitte_checkpoint_capture_nested(
    vitte_checkpoint_t *checkpoint,
    vitte_checkpoint_t *parent);

/* ========================================================================= */
/* Commit                                                                    */
/* ========================================================================= */

/*
 * Accept allocations performed after capture.
 *
 * Commit is logical only:
 *
 *     - allocator memory is unchanged
 *     - arena accounting is unchanged
 *     - post-capture allocations remain valid
 *     - checkpoint becomes COMMITTED
 *
 * Returns false when:
 *
 *     - checkpoint is invalid
 *     - checkpoint is not ACTIVE
 *     - arena is invalid
 *     - generation changed
 *     - parent relationship is stale/corrupt
 */
bool
vitte_checkpoint_commit(
    vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Rollback                                                                  */
/* ========================================================================= */

/*
 * Restore the arena to the transaction capture position.
 *
 * Allocations performed after capture become invalid.
 *
 * The underlying arena rollback restores:
 *
 *     - low-level allocator position
 *     - current allocation count
 *     - current requested-byte count
 *
 * Lifetime/high-water statistics remain cumulative unless arena.c explicitly
 * documents otherwise.
 *
 * On success:
 *
 *     state = ROLLED_BACK
 *
 * Rollback does not reset the arena generation.
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
 *
 *     equivalent to:
 *
 *         vitte_checkpoint_commit(checkpoint)
 *
 * commit == false
 *
 *     equivalent to:
 *
 *         vitte_checkpoint_rollback(checkpoint)
 */
bool
vitte_checkpoint_finish(
    vitte_checkpoint_t *checkpoint,
    bool commit);

/* ========================================================================= */
/* Explicit invalidation                                                     */
/* ========================================================================= */

/*
 * Mark checkpoint metadata as INVALIDATED.
 *
 * Arena memory is not modified.
 *
 * An ACTIVE transaction invalidated through this operation loses rollback
 * capability through this checkpoint object.
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
 * Determine whether the checkpoint can still safely represent its captured
 * transaction.
 *
 * Non-ACTIVE checkpoints are considered stale by this predicate.
 *
 * An ACTIVE checkpoint is stale when, among other conditions:
 *
 *     - arena is unavailable/invalid
 *     - arena generation changed
 *     - captured accounting is ahead of current accounting
 *     - reserved capacity was reduced incompatibly
 *     - parent relationship is invalid
 *     - parent transaction has already been resolved
 *     - parent chain is corrupt/cyclic
 */
bool
vitte_checkpoint_is_stale(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

/*
 * Return the checkpoint-local error.
 *
 * Invalid/uninitialized checkpoint storage reports:
 *
 *     VITTE_CHECKPOINT_ERROR_INVALID_CHECKPOINT
 */
vitte_checkpoint_error_t
vitte_checkpoint_last_error(
    const vitte_checkpoint_t *checkpoint);

/*
 * Clear checkpoint-local error state.
 *
 * Does nothing for invalid/uninitialized checkpoint storage.
 */
void
vitte_checkpoint_clear_error(
    vitte_checkpoint_t *checkpoint);

/*
 * Return true when last_error != NONE.
 *
 * Invalid checkpoint storage therefore reports an error.
 */
bool
vitte_checkpoint_has_error(
    const vitte_checkpoint_t *checkpoint);

/*
 * Stable textual error name.
 *
 * Unknown enum values return "unknown".
 */
const char *
vitte_checkpoint_error_name(
    vitte_checkpoint_error_t error);

/* ========================================================================= */
/* State names                                                               */
/* ========================================================================= */

/*
 * Stable textual state name.
 *
 * Unknown enum values return "unknown".
 */
const char *
vitte_checkpoint_state_name(
    vitte_checkpoint_state_t state);

/* ========================================================================= */
/* Arena access                                                              */
/* ========================================================================= */

/*
 * Return the arena pointer stored by the checkpoint.
 *
 * The returned pointer is non-owning.
 *
 * A non-NULL pointer retained by a resolved checkpoint is diagnostic metadata
 * and must not by itself be interpreted as proof that the arena remains live.
 */
vitte_arena_context_t *
vitte_checkpoint_arena(
    vitte_checkpoint_t *checkpoint);

const vitte_arena_context_t *
vitte_checkpoint_arena_const(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Parent access                                                             */
/* ========================================================================= */

/*
 * Return the non-owning parent pointer.
 *
 * NULL denotes either:
 *
 *     - a root checkpoint
 *     - an invalid checkpoint
 */
vitte_checkpoint_t *
vitte_checkpoint_parent(
    vitte_checkpoint_t *checkpoint);

const vitte_checkpoint_t *
vitte_checkpoint_parent_const(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

/*
 * Return the capture sequence allocated by the calling thread.
 *
 * Zero means no capture identity.
 */
uint64_t
vitte_checkpoint_sequence(
    const vitte_checkpoint_t *checkpoint);

/*
 * Return arena generation captured by the checkpoint.
 *
 * Zero means no captured generation.
 */
uint64_t
vitte_checkpoint_generation(
    const vitte_checkpoint_t *checkpoint);

/*
 * Return hierarchy depth.
 *
 * Root checkpoints use depth zero.
 */
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
 * Returns zero when:
 *
 *     - checkpoint is not ACTIVE
 *     - arena is invalid
 *     - generation changed
 *     - accounting moved backwards
 */
size_t
vitte_checkpoint_allocation_delta(
    const vitte_checkpoint_t *checkpoint);

/*
 * Increase in logical caller-requested bytes since capture.
 */
size_t
vitte_checkpoint_requested_delta(
    const vitte_checkpoint_t *checkpoint);

/*
 * Increase in physical allocator used bytes since capture.
 *
 * Physical used bytes include alignment/internal allocator consumption.
 */
size_t
vitte_checkpoint_used_delta(
    const vitte_checkpoint_t *checkpoint);

/*
 * Increase in reserved arena capacity since capture.
 *
 * The return type is unsigned. If reserved capacity decreased, zero is
 * returned rather than attempting to encode a negative delta.
 */
size_t
vitte_checkpoint_reserved_delta(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

/*
 * Return a complete checkpoint statistics snapshot.
 *
 * Invalid checkpoints produce a zero-initialized structure.
 *
 * ACTIVE + current-generation checkpoints expose live arena counters and
 * deltas.
 *
 * Resolved or stale checkpoints preserve their captured snapshot rather than
 * exposing unrelated later arena state.
 */
vitte_checkpoint_stats_t
vitte_checkpoint_stats(
    const vitte_checkpoint_t *checkpoint);

/* ========================================================================= */
/* Comparison                                                                */
/* ========================================================================= */

/*
 * Return true when both valid checkpoint objects contain the same non-NULL
 * arena pointer.
 *
 * This is an identity comparison. It does not prove that a resolved
 * checkpoint's retained arena pointer still refers to a live arena.
 */
bool
vitte_checkpoint_same_arena(
    const vitte_checkpoint_t *left,
    const vitte_checkpoint_t *right);

/*
 * Return true when:
 *
 *     - same non-NULL arena
 *     - same non-zero generation
 */
bool
vitte_checkpoint_same_generation(
    const vitte_checkpoint_t *left,
    const vitte_checkpoint_t *right);

/*
 * Compare checkpoint capture sequences.
 *
 * This compares capture order metadata, not physical memory position.
 *
 * Sequence numbers may eventually wrap and therefore must not be used as a
 * permanent correctness primitive across arbitrarily long process lifetimes.
 */
bool
vitte_checkpoint_precedes(
    const vitte_checkpoint_t *left,
    const vitte_checkpoint_t *right);

/* ========================================================================= */
/* Hierarchy                                                                 */
/* ========================================================================= */

/*
 * Return true when ancestor occurs strictly in checkpoint's parent chain.
 *
 * A checkpoint is not its own ancestor.
 *
 * Both checkpoints must belong to the same arena/generation.
 */
bool
vitte_checkpoint_is_ancestor_of(
    const vitte_checkpoint_t *ancestor,
    const vitte_checkpoint_t *checkpoint);

/*
 * Return the root checkpoint of a parent chain.
 *
 * Returns NULL when:
 *
 *     - checkpoint is invalid
 *     - parent metadata is invalid
 *     - arena/generation changes within the chain
 *     - depth metadata is inconsistent
 *     - a parent cycle is detected
 *     - traversal exceeds VITTE_CHECKPOINT_MAX_DEPTH
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
 * Validate checkpoint structural and transactional invariants.
 *
 * EMPTY checkpoints require canonical empty metadata.
 *
 * ACTIVE checkpoints validate:
 *
 *     - runtime cookie
 *     - state/error enum values
 *     - arena validity
 *     - non-zero generation
 *     - non-zero sequence
 *     - generation equality
 *     - captured accounting <= current accounting
 *     - reserved-capacity consistency
 *     - depth bound
 *     - parent relationship
 *     - parent-chain acyclicity
 *
 * Resolved checkpoints retain diagnostic metadata and therefore do not require
 * their arena to remain live.
 */
bool
vitte_checkpoint_validate(
    const vitte_checkpoint_t *checkpoint);

/*
 * Validate the complete parent chain.
 *
 * Checks:
 *
 *     - valid runtime cookies
 *     - acyclic parent chain
 *     - common arena identity
 *     - common generation
 *     - exact depth decrement
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

static inline bool
vitte_checkpoint_generation_matches(
    const vitte_checkpoint_t *checkpoint)
{
    if (!vitte_checkpoint_is_active(
            checkpoint) ||
        checkpoint->arena == NULL) {
        return false;
    }

    if (!vitte_arena_context_is_valid(
            checkpoint->arena)) {
        return false;
    }

    return
        checkpoint->generation != UINT64_C(0) &&
        checkpoint->generation ==
            vitte_arena_context_generation(
                checkpoint->arena);
}

static inline bool
vitte_checkpoint_can_commit(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_is_active(
            checkpoint) &&
        !vitte_checkpoint_is_stale(
            checkpoint);
}

static inline bool
vitte_checkpoint_can_rollback(
    const vitte_checkpoint_t *checkpoint)
{
    return
        vitte_checkpoint_is_active(
            checkpoint) &&
        !vitte_checkpoint_is_stale(
            checkpoint);
}

/* ========================================================================= */
/* Transaction macros                                                        */
/* ========================================================================= */

/*
 * These macros are intentionally thin wrappers.
 *
 * Arguments are evaluated exactly once by the called function.
 */

#define VITTE_CHECKPOINT_BEGIN(checkpoint, arena) \
    vitte_checkpoint_begin(                       \
        (checkpoint),                             \
        (arena))

#define VITTE_CHECKPOINT_COMMIT(checkpoint) \
    vitte_checkpoint_commit(                \
        (checkpoint))

#define VITTE_CHECKPOINT_ROLLBACK(checkpoint) \
    vitte_checkpoint_rollback(                \
        (checkpoint))

#define VITTE_CHECKPOINT_FINISH(checkpoint, commit) \
    vitte_checkpoint_finish(                         \
        (checkpoint),                               \
        (commit))

#define VITTE_CHECKPOINT_NESTED(checkpoint, parent) \
    vitte_checkpoint_capture_nested(                \
        (checkpoint),                               \
        (parent))

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

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
    VITTE_CHECKPOINT_STATE_ACTIVE == 1,
    "checkpoint active state ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_STATE_COMMITTED == 2,
    "checkpoint committed state ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_STATE_ROLLED_BACK == 3,
    "checkpoint rolled-back state ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_STATE_INVALIDATED == 4,
    "checkpoint invalidated state ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_NONE == 0,
    "checkpoint no-error value must remain zero");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_INVALID_ARGUMENT == 1,
    "checkpoint error ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_INVALID_ARENA == 2,
    "checkpoint error ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_INVALID_CHECKPOINT == 3,
    "checkpoint error ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_INVALID_STATE == 4,
    "checkpoint error ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_STALE == 5,
    "checkpoint error ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_FOREIGN_ARENA == 6,
    "checkpoint error ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_GENERATION_MISMATCH == 7,
    "checkpoint error ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_DEPTH_EXCEEDED == 8,
    "checkpoint error ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_ROLLBACK_FAILED == 9,
    "checkpoint error ABI changed");

_Static_assert(
    VITTE_CHECKPOINT_ERROR_CORRUPTION == 10,
    "checkpoint error ABI changed");

_Static_assert(
    sizeof(vitte_checkpoint_state_t) >=
        sizeof(unsigned char),
    "checkpoint state type must be representable");

_Static_assert(
    sizeof(vitte_checkpoint_error_t) >=
        sizeof(unsigned char),
    "checkpoint error type must be representable");

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_ARENA_CHECKPOINT_H */
