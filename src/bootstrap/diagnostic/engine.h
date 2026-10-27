#ifndef VITTE_DIAGNOSTIC_ENGINE_H
#define VITTE_DIAGNOSTIC_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

#include "diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Vitte diagnostic engine.
 *
 * High-level orchestration layer above the primitive diagnostic storage API.
 *
 * The engine centralizes compiler-facing diagnostic policy while keeping
 * diagnostic ownership, cascade analysis and rendering separated.
 *
 * Responsibilities:
 *
 *   - diagnostic emission;
 *   - compiler-phase provenance;
 *   - package/module/procedure context propagation;
 *   - explicit parent/root causal context;
 *   - automatic cascade classification;
 *   - cascade suppression policy;
 *   - error and warning limits;
 *   - warnings-as-errors;
 *   - fatal-stop policy;
 *   - speculative checkpoints and rollback;
 *   - deterministic diagnostic ordering;
 *   - causal-index preservation during sorting;
 *   - finalization;
 *   - engine statistics.
 *
 * Ownership:
 *
 *   - the engine owns no diagnostic bag storage;
 *   - the caller owns the vitte_diagnostic_bag_t;
 *   - context strings passed to the engine are copied into engine-owned
 *     fixed-size buffers;
 *   - diagnostics remain owned by the bag;
 *   - pointers returned by vitte_diagnostic_engine_last() are invalidated by
 *     operations that physically reorder or reset the bag.
 *
 * Threading:
 *
 *   An engine instance is intentionally free of global mutable state.
 *   Distinct engines may be used independently. Concurrent mutation of the
 *   same engine or diagnostic bag requires external synchronization.
 *
 * Layering:
 *
 *   diagnostic.c
 *       diagnostic representation, ownership and primitive mutation
 *
 *   cascade.c
 *       parent/root relationships and cascade classification
 *
 *   engine.c
 *       compiler-facing orchestration and policy
 *
 *   render_*.c
 *       terminal / JSON / SARIF / LSP serialization
 */

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

/*
 * Sentinel used by engine causal-context fields.
 *
 * Kept as an alias so compiler-facing code does not need to depend on the
 * internal spelling of the diagnostic index sentinel.
 */
#define VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE \
    VITTE_DIAGNOSTIC_INDEX_NONE

/* ========================================================================= */
/* Policy                                                                    */
/* ========================================================================= */

typedef struct vitte_diagnostic_engine_policy {
    /*
     * Maximum number of visible error/fatal diagnostics.
     *
     * A value of zero supplied to engine_init() is normalized to the engine
     * default rather than interpreted as "unlimited".
     */
    size_t error_limit;

    /*
     * Maximum number of visible warning diagnostics.
     *
     * Additional warnings are rejected once the limit is reached.
     *
     * A value of zero supplied to engine_init() is normalized to the engine
     * default.
     */
    size_t warning_limit;

    /*
     * Promote warnings emitted through this engine to errors before ordinary
     * diagnostic processing.
     *
     * Compiler integration should avoid independently applying another
     * warnings-as-errors transformation to the same diagnostic.
     */
    bool warnings_as_errors;

    /*
     * Deduplication policy reservation.
     *
     * The primitive diagnostic layer currently owns deduplication semantics.
     * This field is retained in the public policy so deduplication can later
     * become session-configurable without another policy ABI redesign.
     *
     * Until the primitive bag exposes a deduplication mode, changing this
     * field alone does not bypass bag-level deduplication.
     */
    bool deduplicate;

    /*
     * Run automatic conservative cascade classification when no explicit
     * causal parent was supplied.
     */
    bool classify_cascades;

    /*
     * Mark automatically or explicitly derived cascade diagnostics as
     * suppressed.
     *
     * Suppression preserves the diagnostic in the bag.
     */
    bool suppress_cascades;

    /*
     * Enter stopped state after a fatal diagnostic.
     */
    bool stop_after_fatal;

    /*
     * Enter stopped state after the configured visible error limit.
     */
    bool stop_after_error_limit;

    /*
     * Emit VITTE_INFRA_E_LIMIT when the visible error limit is reached.
     *
     * The infrastructure diagnostic is emitted directly into the underlying
     * bag so it can still be reported after the ordinary limit has fired.
     */
    bool emit_limit_diagnostic;

    /*
     * Deterministically reorder diagnostics during finalization.
     */
    bool sort_before_render;

    /*
     * Suppressed-diagnostic retention policy reservation.
     *
     * Suppressed diagnostics are currently always retained because they are
     * required by machine-readable consumers, causal analysis and debugging.
     * A future compaction layer may make this flag operational.
     */
    bool preserve_suppressed;
} vitte_diagnostic_engine_policy_t;

/* ========================================================================= */
/* Checkpoint                                                                */
/* ========================================================================= */

/*
 * Snapshot of engine/bag state used for speculative compiler work.
 *
 * Checkpoints are value objects. They allocate no memory and do not own the
 * engine or diagnostic bag.
 *
 * Typical use cases:
 *
 *   - speculative parser branches;
 *   - parser recovery;
 *   - overload candidate selection;
 *   - generic candidate selection;
 *   - type inference;
 *   - trait/constraint solving;
 *   - contract checking;
 *   - macro expansion;
 *   - optimization/pass speculation.
 *
 * A checkpoint belongs logically to the engine from which it was obtained.
 * The current representation does not embed an engine identity token, so
 * callers must not apply checkpoints across different engines/bags.
 */
typedef struct vitte_diagnostic_checkpoint {
    /*
     * Number of diagnostics physically stored at checkpoint creation.
     */
    size_t count;

    /*
     * Exact primitive bag counters at checkpoint creation.
     */
    vitte_diagnostic_counts_t counts;

    /*
     * Engine-level counters.
     */
    size_t emitted_count;
    size_t rejected_count;
    size_t cascade_count;
    size_t suppressed_count;

    /*
     * Engine stop/limit state.
     */
    bool stopped;
    bool limit_reported;

    /*
     * False for a checkpoint returned from an invalid/uninitialized engine.
     */
    bool valid;
} vitte_diagnostic_checkpoint_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_diagnostic_engine_statistics {
    /*
     * Diagnostics physically stored in the bag.
     */
    size_t stored;

    /*
     * Successful engine emissions that created a new stored diagnostic.
     *
     * Infrastructure diagnostics emitted internally for limit reporting are
     * not necessarily included in this engine-level counter.
     */
    size_t emitted;

    /*
     * Candidates rejected by stop/limit policy or primitive deduplication.
     */
    size_t rejected;

    /*
     * Stored diagnostics classified as causal cascades.
     */
    size_t cascade;

    /*
     * Visible independent error/fatal diagnostics.
     *
     * This is intentionally not equivalent to all non-cascade diagnostics:
     * notes, help and warnings are excluded.
     */
    size_t primary;

    /*
     * Diagnostics currently marked suppressed in the underlying bag.
     */
    size_t suppressed;

    /*
     * Non-suppressed error/fatal diagnostics.
     */
    size_t visible_errors;

    /*
     * Non-suppressed warning diagnostics.
     */
    size_t visible_warnings;

    /*
     * Whether the engine refuses ordinary future emissions because a stop
     * condition has fired.
     */
    bool stopped;
} vitte_diagnostic_engine_statistics_t;

/* ========================================================================= */
/* Engine                                                                    */
/* ========================================================================= */

typedef struct vitte_diagnostic_engine {
    /*
     * Lifecycle guard.
     */
    bool initialized;

    /*
     * Borrowed diagnostic bag.
     *
     * The bag must remain alive and initialized for the complete lifetime of
     * the engine.
     */
    vitte_diagnostic_bag_t *bag;

    /*
     * Session policy copied at initialization.
     */
    vitte_diagnostic_engine_policy_t policy;

    /* --------------------------------------------------------------------- */
    /* Compiler phase                                                        */
    /* --------------------------------------------------------------------- */

    /*
     * Origin assigned to future diagnostics.
     */
    vitte_diagnostic_origin_t current_origin;

    /* --------------------------------------------------------------------- */
    /* Compilation context                                                   */
    /* --------------------------------------------------------------------- */

    /*
     * Self-referential pointers into the corresponding storage fields below.
     *
     * NULL means that context component is absent.
     */
    const char *package;
    const char *module;
    const char *procedure;

    char package_storage[
        VITTE_DIAGNOSTIC_PACKAGE_CAPACITY
    ];

    char module_storage[
        VITTE_DIAGNOSTIC_MODULE_CAPACITY
    ];

    char procedure_storage[
        VITTE_DIAGNOSTIC_PROCEDURE_CAPACITY
    ];

    /* --------------------------------------------------------------------- */
    /* Explicit causal context                                               */
    /* --------------------------------------------------------------------- */

    /*
     * Immediate causal parent assigned to future diagnostics.
     *
     * VITTE_DIAGNOSTIC_ENGINE_INDEX_NONE means automatic classification.
     */
    size_t current_parent;

    /*
     * Ultimate causal root corresponding to current_parent.
     *
     * This value is cached when the parent is selected and remapped if the
     * diagnostic bag is sorted.
     */
    size_t current_root;

    /* --------------------------------------------------------------------- */
    /* Session state                                                         */
    /* --------------------------------------------------------------------- */

    /*
     * Ordinary diagnostic emission is rejected while stopped is true.
     */
    bool stopped;

    /*
     * Prevent duplicate infrastructure limit diagnostics.
     */
    bool limit_reported;

    /* --------------------------------------------------------------------- */
    /* Engine-level counters                                                 */
    /* --------------------------------------------------------------------- */

    /*
     * Number of ordinary engine emissions that produced stored diagnostics.
     */
    size_t emitted_count;

    /*
     * Number of candidates rejected or absorbed before becoming an ordinary
     * stored engine emission.
     */
    size_t rejected_count;

    /*
     * Cached cascade count.
     *
     * engine.c refreshes this value after operations capable of changing
     * causal classification.
     */
    size_t cascade_count;

    /*
     * Cached suppressed count.
     */
    size_t suppressed_count;
} vitte_diagnostic_engine_t;

/* ========================================================================= */
/* Policy lifecycle                                                          */
/* ========================================================================= */

/*
 * Initialize policy with conservative compiler defaults.
 *
 * Current defaults:
 *
 *   error_limit           = 100
 *   warning_limit         = 100
 *   warnings_as_errors    = false
 *   deduplicate           = true
 *   classify_cascades     = true
 *   suppress_cascades     = true
 *   stop_after_fatal      = true
 *   stop_after_error_limit= true
 *   emit_limit_diagnostic = true
 *   sort_before_render    = true
 *   preserve_suppressed   = true
 */
void
vitte_diagnostic_engine_policy_init(
    vitte_diagnostic_engine_policy_t *policy
);

/* ========================================================================= */
/* Engine lifecycle                                                          */
/* ========================================================================= */

/*
 * Initialize an engine around an already initialized diagnostic bag.
 *
 * policy may be NULL, in which case default policy is used.
 *
 * The policy is copied.
 * The bag is borrowed.
 *
 * Returns:
 *
 *   VITTE_STATUS_OK
 *   VITTE_STATUS_ERROR_INVALID_ARGUMENT
 */
vitte_status_t
vitte_diagnostic_engine_init(
    vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_engine_policy_t *policy
);

/*
 * Reset transient engine state and engine-level counters.
 *
 * IMPORTANT:
 *
 *   This does not clear the underlying diagnostic bag.
 *
 * Existing bag cascade/suppression counts remain reflected by the engine
 * after reset. Use vitte_diagnostic_bag_reset() separately to start a fully
 * empty diagnostic session.
 */
void
vitte_diagnostic_engine_reset(
    vitte_diagnostic_engine_t *engine
);

/*
 * Return true only when the engine and borrowed bag are both initialized.
 */
bool
vitte_diagnostic_engine_is_initialized(
    const vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Compiler phase                                                            */
/* ========================================================================= */

/*
 * Set the compiler subsystem responsible for future diagnostics.
 */
vitte_status_t
vitte_diagnostic_engine_set_origin(
    vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_origin_t origin
);

/*
 * Return the current compiler origin.
 *
 * UNKNOWN is returned for an invalid engine.
 */
vitte_diagnostic_origin_t
vitte_diagnostic_engine_origin(
    const vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Compilation context                                                       */
/* ========================================================================= */

/*
 * Set package/module/procedure context for future diagnostics.
 *
 * Each non-NULL string is copied into engine-owned storage.
 * NULL clears that individual context component.
 *
 * Strings that do not fit their fixed diagnostic capacities are rejected
 * rather than silently truncated.
 *
 * Returns:
 *
 *   VITTE_STATUS_OK
 *   VITTE_STATUS_ERROR_INVALID_STATE
 *   VITTE_STATUS_ERROR_INVALID_ARGUMENT
 */
vitte_status_t
vitte_diagnostic_engine_set_context(
    vitte_diagnostic_engine_t *engine,
    const char *package,
    const char *module,
    const char *procedure
);

/*
 * Clear package/module/procedure context.
 */
void
vitte_diagnostic_engine_clear_context(
    vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Explicit causal context                                                   */
/* ========================================================================= */

/*
 * Set an existing diagnostic as causal parent for future emissions.
 *
 * parent_index must refer to a diagnostic already stored in the bag.
 * Its ultimate root is resolved immediately through cascade.c.
 *
 * The causal context persists until explicitly cleared or until rollback
 * invalidates the selected diagnostic.
 *
 * Returns:
 *
 *   VITTE_STATUS_OK
 *   VITTE_STATUS_ERROR_INVALID_STATE
 *   VITTE_STATUS_ERROR_INVALID_ARGUMENT
 */
vitte_status_t
vitte_diagnostic_engine_set_parent(
    vitte_diagnostic_engine_t *engine,
    size_t parent_index
);

/*
 * Clear explicit causal context.
 *
 * Future diagnostics may again use automatic cascade classification.
 */
void
vitte_diagnostic_engine_clear_parent(
    vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Stop state                                                                */
/* ========================================================================= */

/*
 * Return true when ordinary compiler work should stop because:
 *
 *   - the engine is invalid;
 *   - the engine has explicitly entered stopped state; or
 *   - stop_after_error_limit is active and the visible error limit has been
 *     reached.
 */
bool
vitte_diagnostic_engine_should_stop(
    const vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Last stored diagnostic                                                    */
/* ========================================================================= */

/*
 * Return the last diagnostic physically stored in the bag.
 *
 * NULL is returned when the engine is invalid or the bag is empty.
 *
 * IMPORTANT:
 *
 *   This means "last stored diagnostic", not necessarily "the diagnostic from
 *   the most recent attempted emission".
 *
 *   If an emission is deduplicated/rejected without growing the bag, this
 *   function still returns the previous stored diagnostic.
 *
 *   Therefore decoration helpers should be called only after an emission that
 *   is known to have created a diagnostic.
 *
 * Pointer stability:
 *
 *   The returned pointer must not be retained across sorting, rollback, bag
 *   reset, bag reallocation or other storage-moving operations.
 */
vitte_diagnostic_t *
vitte_diagnostic_engine_last(
    vitte_diagnostic_engine_t *engine
);

const vitte_diagnostic_t *
vitte_diagnostic_engine_last_const(
    const vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Generic emission                                                          */
/* ========================================================================= */

/*
 * Emit one diagnostic using current engine context.
 *
 * Processing may include:
 *
 *   1. input validation;
 *   2. warnings-as-errors promotion;
 *   3. stop/error-limit enforcement;
 *   4. warning-limit enforcement;
 *   5. primitive diagnostic construction;
 *   6. source-span attachment;
 *   7. package/module/procedure propagation;
 *   8. insertion into the diagnostic bag;
 *   9. primitive deduplication;
 *  10. explicit causal-parent attachment;
 *  11. automatic cascade classification;
 *  12. cascade suppression;
 *  13. fatal-stop handling;
 *  14. post-emission error-limit handling.
 *
 * code and message must be non-NULL and non-empty.
 *
 * Input text exceeding fixed diagnostic capacities is rejected rather than
 * silently truncated.
 */
vitte_status_t
vitte_diagnostic_engine_emit(
    vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
);

/* ========================================================================= */
/* Convenience emission                                                      */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_note(
    vitte_diagnostic_engine_t *engine,
    const char *code,
    const char *message,
    const vitte_ast_span_t *span
);

vitte_status_t
vitte_diagnostic_engine_help(
    vitte_diagnostic_engine_t *engine,
    const char *code,
    const char *message,
    const vitte_ast_span_t *span
);

vitte_status_t
vitte_diagnostic_engine_warning(
    vitte_diagnostic_engine_t *engine,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
);

vitte_status_t
vitte_diagnostic_engine_error(
    vitte_diagnostic_engine_t *engine,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
);

vitte_status_t
vitte_diagnostic_engine_fatal(
    vitte_diagnostic_engine_t *engine,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
);

/* ========================================================================= */
/* Diagnostic decoration                                                     */
/* ========================================================================= */

/*
 * The following helpers decorate the diagnostic currently returned by
 * vitte_diagnostic_engine_last().
 *
 * They intentionally delegate ownership/validation rules to diagnostic.c.
 *
 * IMPORTANT:
 *
 *   Because primitive deduplication can make an emission succeed without
 *   growing the bag, callers must not blindly decorate after every successful
 *   emit call unless they know a new diagnostic was stored.
 */

/*
 * Add a primary source label.
 */
vitte_status_t
vitte_diagnostic_engine_primary_label(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
);

/*
 * Add a secondary source label.
 */
vitte_status_t
vitte_diagnostic_engine_secondary_label(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
);

/*
 * Add a note annotation.
 */
vitte_status_t
vitte_diagnostic_engine_note_last(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
);

/*
 * Add a help annotation.
 */
vitte_status_t
vitte_diagnostic_engine_help_last(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
);

/*
 * Add a causal explanation.
 */
vitte_status_t
vitte_diagnostic_engine_cause(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
);

/*
 * Add a fix-it/code-action candidate.
 *
 * replacement may be an empty string.
 *
 * An empty replacement is semantically meaningful and represents deletion.
 * It must not be treated as equivalent to a missing replacement.
 */
vitte_status_t
vitte_diagnostic_engine_suggestion(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability,
    const vitte_ast_span_t *span
);

/*
 * Attach semantic subject information and contextual text.
 *
 * symbol:
 *   relevant symbol/entity name
 *
 * expected_type:
 *   expected semantic type
 *
 * actual_type:
 *   observed semantic type
 *
 * context:
 *   diagnostic-local semantic context
 *
 * Engine package/module/procedure context is also reapplied.
 */
vitte_status_t
vitte_diagnostic_engine_subject(
    vitte_diagnostic_engine_t *engine,
    const char *symbol,
    const char *expected_type,
    const char *actual_type,
    const char *context
);

/* ========================================================================= */
/* Contract diagnostics                                                      */
/* ========================================================================= */

/*
 * Attach contract provenance to the last stored diagnostic.
 *
 * Supported contract information includes:
 *
 *   - contract kind;
 *   - contract expression;
 *   - procedure;
 *   - reason;
 *   - contract definition span;
 *   - call span;
 *   - declaration span.
 */
vitte_status_t
vitte_diagnostic_engine_contract(
    vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_contract_kind_t kind,
    const char *expression,
    const char *procedure,
    const char *reason,
    const vitte_ast_span_t *contract_span,
    const vitte_ast_span_t *call_span,
    const vitte_ast_span_t *declaration_span
);

/* ========================================================================= */
/* Checkpoints                                                               */
/* ========================================================================= */

/*
 * Capture the current engine and bag diagnostic state.
 *
 * An invalid engine returns a checkpoint with valid == false.
 */
vitte_diagnostic_checkpoint_t
vitte_diagnostic_engine_checkpoint(
    const vitte_diagnostic_engine_t *engine
);

/*
 * Roll back diagnostics and engine counters to checkpoint.
 *
 * Diagnostics beyond checkpoint.count are reinitialized before the bag count
 * is reduced so stale self-referential pointers and old diagnostic payloads
 * do not remain observable through reused storage.
 *
 * Explicit causal context is cleared automatically when it references a
 * diagnostic discarded by rollback.
 *
 * The checkpoint must originate from the same logical engine/bag session.
 */
vitte_status_t
vitte_diagnostic_engine_rollback(
    vitte_diagnostic_engine_t *engine,
    const vitte_diagnostic_checkpoint_t *checkpoint
);

/*
 * Return true when the physical diagnostic count differs from the checkpoint.
 *
 * This is deliberately a structural test, not a semantic test.
 */
bool
vitte_diagnostic_engine_changed_since(
    const vitte_diagnostic_engine_t *engine,
    const vitte_diagnostic_checkpoint_t *checkpoint
);

/*
 * Return true when a visible error/fatal diagnostic exists in the portion of
 * the bag added after checkpoint.count.
 */
bool
vitte_diagnostic_engine_has_errors_since(
    const vitte_diagnostic_engine_t *engine,
    const vitte_diagnostic_checkpoint_t *checkpoint
);

/* ========================================================================= */
/* Deterministic ordering                                                    */
/* ========================================================================= */

/*
 * Deterministically reorder diagnostics.
 *
 * Current ordering considers:
 *
 *   - canonical source name;
 *   - source identifier;
 *   - start offset;
 *   - end offset;
 *   - severity;
 *   - compiler origin;
 *   - public short diagnostic code;
 *   - internal diagnostic code;
 *   - symbol;
 *   - expected type;
 *   - actual type;
 *   - message;
 *   - fingerprint;
 *   - original emission index as an absolute final tie-breaker.
 *
 * Causal identity:
 *
 *   Sorting never uses the fingerprint as a unique identity.
 *
 *   Fingerprints may collide. The implementation therefore sorts sidecar
 *   entries containing each diagnostic's unique pre-sort physical index and
 *   constructs an exact old-index -> new-index permutation.
 *
 *   parent_diagnostic and root_diagnostic are remapped through that
 *   permutation.
 *
 *   current_parent and current_root are remapped as well.
 *
 * Self-referential storage:
 *
 *   vitte_diagnostic_t and several nested records contain pointers into their
 *   own inline character buffers. A raw struct move copies those pointers
 *   verbatim. engine_sort() therefore rebinds every supported inline-owned
 *   string pointer after physical movement.
 *
 * Fix-it deletion:
 *
 *   A suggestion with has_replacement == true and replacement == "" remains
 *   a valid deletion fix-it after sorting.
 *
 * Pointer stability:
 *
 *   All pointers previously obtained into bag diagnostic storage should be
 *   considered invalid after this function.
 */
vitte_status_t
vitte_diagnostic_engine_sort(
    vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Finalization                                                              */
/* ========================================================================= */

/*
 * Complete diagnostic processing before rendering.
 *
 * Depending on policy:
 *
 *   1. run final cascade classification;
 *   2. apply cascade suppression;
 *   3. validate causal topology;
 *   4. sort diagnostics deterministically;
 *   5. refresh relation statistics;
 *   6. calculate final compilation status.
 *
 * Finalization may physically reorder diagnostics.
 *
 * Consumers must not retain pointers or physical indices across finalization
 * unless those indices are explicitly remapped by the engine itself.
 */
vitte_status_t
vitte_diagnostic_engine_finalize(
    vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

/*
 * Fill statistics with a coherent snapshot of the current engine/bag state.
 *
 * statistics is always zero-initialized first.
 *
 * Passing NULL is allowed and is a no-op.
 */
void
vitte_diagnostic_engine_get_statistics(
    const vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_engine_statistics_t *statistics
);

/* ========================================================================= */
/* Compilation status                                                        */
/* ========================================================================= */

/*
 * Return the compilation status represented by the underlying diagnostic bag.
 *
 * This does not implicitly finalize or sort the bag.
 */
vitte_status_t
vitte_diagnostic_engine_status(
    const vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Integration invariants                                                    */
/* ========================================================================= */

/*
 * 1. The bag must outlive the engine.
 *
 * 2. The engine does not own bag storage.
 *
 * 3. Compiler phases should prefer explicit causal relationships whenever
 *    provenance is known. Automatic cascade classification is a conservative
 *    fallback.
 *
 * 4. Diagnostics that are merely consequences of an earlier root error should
 *    not be emitted as independent primary errors.
 *
 * 5. Suppression is not deletion. Machine-readable renderers may still inspect
 *    suppressed diagnostics.
 *
 * 6. Fingerprints are hashes, not unique diagnostic identifiers.
 *
 * 7. Physical diagnostic indices are unstable across deterministic sorting.
 *
 * 8. Pointers into diagnostic storage are unstable across sorting, rollback,
 *    reset and storage relocation.
 *
 * 9. Empty replacement strings are valid fix-it replacements and represent
 *    deletion when has_replacement is true.
 *
 * 10. Engine context strings are copied, not borrowed.
 *
 * 11. Engine reset does not reset the bag.
 *
 * 12. Checkpoints must not be transferred between unrelated engine/bag
 *     sessions.
 *
 * 13. Fatal diagnostics may stop future ordinary emission according to policy.
 *
 * 14. Error limits count visible error/fatal diagnostics rather than all
 *     physically stored diagnostics.
 *
 * 15. Warning limits count visible warnings.
 *
 * 16. Cascade diagnostics may remain stored even when hidden from normal
 *     terminal presentation.
 *
 * 17. Finalization should normally occur once all compiler phases capable of
 *     contributing diagnostics have completed.
 *
 * 18. Renderers should treat finalized diagnostics as read-only.
 *
 * 19. Compiler phases that continue after recovery should propagate explicit
 *     error/poison state rather than manufacturing unrelated secondary errors.
 *
 * 20. The engine is orchestration infrastructure. Semantic knowledge such as
 *     expected/found types, declaration provenance, overload candidates,
 *     parser recovery confidence and contract provenance must originate in the
 *     compiler phase that actually possesses that information.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_ENGINE_H */