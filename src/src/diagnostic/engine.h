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
 * High-level orchestration layer for compiler diagnostics.
 *
 * Responsibilities:
 *
 *   - emission policy;
 *   - compiler-phase context;
 *   - package/module/procedure context;
 *   - explicit causal relationships;
 *   - automatic cascade classification;
 *   - error/warning limits;
 *   - warnings-as-errors policy;
 *   - checkpoints and rollback;
 *   - deterministic ordering;
 *   - finalization;
 *   - statistics.
 *
 * Lower-level responsibilities remain separated:
 *
 *   diagnostic.c
 *       diagnostic storage and primitive mutation
 *
 *   cascade.c
 *       causal/cascade classification
 *
 *   renderer modules
 *       terminal / JSON / SARIF / LSP output
 *
 * No global mutable diagnostic state is required.
 */

/* ========================================================================= */
/* Policy                                                                    */
/* ========================================================================= */

typedef struct vitte_diagnostic_engine_policy {
    /*
     * Maximum number of visible error/fatal diagnostics before compilation
     * is stopped.
     */
    size_t error_limit;

    /*
     * Maximum number of visible warnings.
     *
     * Additional warnings are ignored after the limit.
     */
    size_t warning_limit;

    /*
     * Promote warnings emitted through the engine to errors.
     */
    bool warnings_as_errors;

    /*
     * Reserved policy flag for deduplication.
     *
     * The current primitive diagnostic layer performs deterministic
     * deduplication itself. Keeping the flag at engine level allows future
     * per-session or per-consumer control without changing the public engine
     * structure.
     */
    bool deduplicate;

    /*
     * Run automatic cascade classification when an explicit causal
     * relationship was not supplied by the compiler phase.
     */
    bool classify_cascades;

    /*
     * Keep cascade diagnostics internally but hide them from normal visible
     * diagnostic output.
     */
    bool suppress_cascades;

    /*
     * Stop accepting normal diagnostics after a fatal diagnostic.
     */
    bool stop_after_fatal;

    /*
     * Stop compilation after error_limit visible errors.
     */
    bool stop_after_error_limit;

    /*
     * Emit an infrastructure diagnostic when the configured error limit is
     * reached.
     */
    bool emit_limit_diagnostic;

    /*
     * Sort diagnostics deterministically before final rendering.
     */
    bool sort_before_render;

    /*
     * Preserve suppressed diagnostics in the diagnostic bag for machine
     * consumers, debugging and analysis.
     *
     * The current implementation always preserves them. This policy field
     * reserves explicit control for a later compaction pass.
     */
    bool preserve_suppressed;
} vitte_diagnostic_engine_policy_t;

/* ========================================================================= */
/* Checkpoint                                                                */
/* ========================================================================= */

/*
 * A checkpoint captures diagnostic-engine state before a speculative compiler
 * operation.
 *
 * Typical users:
 *
 *   - parser recovery;
 *   - speculative parsing;
 *   - overload resolution;
 *   - generic candidate selection;
 *   - type inference;
 *   - contract checking;
 *   - macro expansion;
 *   - compiler passes.
 *
 * A failed speculative operation can roll back every diagnostic emitted after
 * the checkpoint without disturbing earlier diagnostics.
 */
typedef struct vitte_diagnostic_checkpoint {
    size_t count;

    vitte_diagnostic_counts_t counts;

    size_t emitted_count;
    size_t rejected_count;
    size_t cascade_count;
    size_t suppressed_count;

    bool stopped;
    bool limit_reported;

    bool valid;
} vitte_diagnostic_checkpoint_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_diagnostic_engine_statistics {
    /*
     * Number of diagnostics physically stored in the bag.
     */
    size_t stored;

    /*
     * Number of successful engine emissions.
     *
     * Deduplicated emissions that did not create a new diagnostic are not
     * counted here.
     */
    size_t emitted;

    /*
     * Number of diagnostics rejected by stop/limit policies.
     */
    size_t rejected;

    /*
     * Number of stored diagnostics classified as cascades.
     */
    size_t cascade;

    /*
     * Number of visible independent error/fatal diagnostics.
     */
    size_t primary;

    /*
     * Number of suppressed/deduplicated diagnostics recorded by the
     * underlying diagnostic bag.
     */
    size_t suppressed;

    /*
     * Visible error/fatal count after suppression.
     */
    size_t visible_errors;

    /*
     * Visible warning count after suppression.
     */
    size_t visible_warnings;

    /*
     * Whether the engine has entered its stopped state.
     */
    bool stopped;
} vitte_diagnostic_engine_statistics_t;

/* ========================================================================= */
/* Engine                                                                    */
/* ========================================================================= */

typedef struct vitte_diagnostic_engine {
    bool initialized;

    /*
     * Diagnostic storage owned externally.
     *
     * The engine does not allocate or free the bag.
     */
    vitte_diagnostic_bag_t *bag;

    vitte_diagnostic_engine_policy_t policy;

    /* --------------------------------------------------------------------- */
    /* Current compiler phase                                                */
    /* --------------------------------------------------------------------- */

    vitte_diagnostic_origin_t current_origin;

    /* --------------------------------------------------------------------- */
    /* Current compilation context                                           */
    /* --------------------------------------------------------------------- */

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
     * Immediate causal parent to assign to the next emitted diagnostic.
     */
    size_t current_parent;

    /*
     * Resolved root associated with current_parent.
     *
     * Cached primarily for compiler/debugging context.
     */
    size_t current_root;

    /* --------------------------------------------------------------------- */
    /* State                                                                 */
    /* --------------------------------------------------------------------- */

    bool stopped;
    bool limit_reported;

    /* --------------------------------------------------------------------- */
    /* Engine-level statistics                                               */
    /* --------------------------------------------------------------------- */

    size_t emitted_count;
    size_t rejected_count;
    size_t cascade_count;
    size_t suppressed_count;
} vitte_diagnostic_engine_t;

/* ========================================================================= */
/* Policy initialization                                                     */
/* ========================================================================= */

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
 * The engine borrows the bag and does not take ownership of its storage.
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
 * This function does not clear the underlying diagnostic bag.
 *
 * Use vitte_diagnostic_bag_reset() separately when diagnostics themselves
 * should also be discarded.
 */
void
vitte_diagnostic_engine_reset(
    vitte_diagnostic_engine_t *engine
);

bool
vitte_diagnostic_engine_is_initialized(
    const vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Compiler phase                                                            */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_set_origin(
    vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_origin_t origin
);

vitte_diagnostic_origin_t
vitte_diagnostic_engine_origin(
    const vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Compilation context                                                       */
/* ========================================================================= */

/*
 * Set package/module/procedure context copied onto future diagnostics.
 *
 * Strings are copied into engine-owned storage.
 */
vitte_status_t
vitte_diagnostic_engine_set_context(
    vitte_diagnostic_engine_t *engine,
    const char *package,
    const char *module,
    const char *procedure
);

void
vitte_diagnostic_engine_clear_context(
    vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Explicit causal context                                                   */
/* ========================================================================= */

/*
 * Set an existing diagnostic as the causal parent for future emissions.
 *
 * The parent must already exist in the diagnostic bag.
 *
 * The root diagnostic is resolved immediately and cached by the engine.
 */
vitte_status_t
vitte_diagnostic_engine_set_parent(
    vitte_diagnostic_engine_t *engine,
    size_t parent_index
);

/*
 * Clear explicit causal context.
 *
 * Future diagnostics return to normal automatic cascade classification.
 */
void
vitte_diagnostic_engine_clear_parent(
    vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Stop state                                                                */
/* ========================================================================= */

/*
 * Return true when the compiler phase should stop producing normal
 * diagnostics/work because a fatal condition or configured error limit has
 * been reached.
 */
bool
vitte_diagnostic_engine_should_stop(
    const vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Last diagnostic                                                          */
/* ========================================================================= */

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
 * Emit one diagnostic using the engine's current phase and compilation
 * context.
 *
 * The operation may:
 *
 *   1. promote a warning to an error;
 *   2. enforce diagnostic limits;
 *   3. add the diagnostic;
 *   4. attach package/module/procedure context;
 *   5. establish an explicit causal relationship;
 *   6. classify a cascade heuristically when no explicit relationship exists;
 *   7. suppress a cascade according to policy;
 *   8. enter stopped state after fatal/limit conditions.
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
 * Add a primary label to the most recently emitted diagnostic.
 */
vitte_status_t
vitte_diagnostic_engine_primary_label(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
);

/*
 * Add a secondary label to the most recently emitted diagnostic.
 */
vitte_status_t
vitte_diagnostic_engine_secondary_label(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
);

/*
 * Add a note annotation to the most recently emitted diagnostic.
 */
vitte_status_t
vitte_diagnostic_engine_note_last(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
);

/*
 * Add a help annotation to the most recently emitted diagnostic.
 */
vitte_status_t
vitte_diagnostic_engine_help_last(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
);

/*
 * Add one causal explanation to the most recently emitted diagnostic.
 */
vitte_status_t
vitte_diagnostic_engine_cause(
    vitte_diagnostic_engine_t *engine,
    const char *message,
    const vitte_ast_span_t *span
);

/*
 * Add a fix-it/code-action candidate.
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
 * Attach semantic information to the most recently emitted diagnostic.
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
 * Attach contract provenance to the most recently emitted diagnostic.
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
/* Checkpoints / speculative diagnostics                                     */
/* ========================================================================= */

/*
 * Capture the current diagnostic state.
 *
 * The returned checkpoint is a value object and owns no resources.
 */
vitte_diagnostic_checkpoint_t
vitte_diagnostic_engine_checkpoint(
    const vitte_diagnostic_engine_t *engine
);

/*
 * Roll the engine and diagnostic bag back to checkpoint.
 *
 * Diagnostics emitted after checkpoint are reinitialized before the bag count
 * is restored.
 */
vitte_status_t
vitte_diagnostic_engine_rollback(
    vitte_diagnostic_engine_t *engine,
    const vitte_diagnostic_checkpoint_t *checkpoint
);

/*
 * Return true if at least one diagnostic has been added since checkpoint.
 */
bool
vitte_diagnostic_engine_changed_since(
    const vitte_diagnostic_engine_t *engine,
    const vitte_diagnostic_checkpoint_t *checkpoint
);

/*
 * Return true if a visible error/fatal diagnostic has been emitted since
 * checkpoint.
 */
bool
vitte_diagnostic_engine_has_errors_since(
    const vitte_diagnostic_engine_t *engine,
    const vitte_diagnostic_checkpoint_t *checkpoint
);

/* ========================================================================= */
/* Ordering                                                                  */
/* ========================================================================= */

/*
 * Deterministically sort diagnostics.
 *
 * Ordering considers:
 *
 *   source
 *   source position
 *   severity
 *   compiler phase
 *   public diagnostic code
 *   internal diagnostic code
 *   fingerprint
 *   message
 *
 * Because diagnostics contain pointers into their own inline string buffers,
 * engine_sort() also rebinds every such pointer after qsort().
 *
 * Causal parent/root indices are reconstructed after sorting.
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
 * Depending on policy this performs:
 *
 *   - final cascade classification;
 *   - cascade suppression;
 *   - deterministic sorting;
 *   - final compilation status calculation.
 *
 * VITTE_STATUS_OK is returned when no error/fatal diagnostic remains.
 * Otherwise the diagnostic compilation status is returned.
 */
vitte_status_t
vitte_diagnostic_engine_finalize(
    vitte_diagnostic_engine_t *engine
);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

void
vitte_diagnostic_engine_get_statistics(
    const vitte_diagnostic_engine_t *engine,
    vitte_diagnostic_engine_statistics_t *statistics
);

/* ========================================================================= */
/* Status                                                                    */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_engine_status(
    const vitte_diagnostic_engine_t *engine
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_ENGINE_H */
