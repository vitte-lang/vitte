#ifndef VITTE_DIAGNOSTIC_CASCADE_H
#define VITTE_DIAGNOSTIC_CASCADE_H

#include <stdbool.h>
#include <stddef.h>

#include "diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Vitte diagnostic cascade engine.
 *
 * The cascade engine distinguishes independent/root diagnostics from
 * diagnostics that are consequences of an earlier compiler failure.
 *
 * Example:
 *
 *   E0401 unknown symbol
 *       |
 *       +-- E0502 cannot infer type
 *               |
 *               +-- E1001 cannot lower expression
 *
 * E0401 is the root diagnostic.
 * E0502 and E1001 are cascade diagnostics.
 *
 * Derived diagnostics may remain stored for debugging, JSON/SARIF/LSP
 * consumers and compiler analysis while being suppressed from normal
 * terminal output.
 *
 * Compiler phases should establish explicit parent/root relationships
 * whenever causal provenance is known. Automatic classification is a
 * conservative fallback.
 */

/* ------------------------------------------------------------------------- */
/* Explicit relationships                                                    */
/* ------------------------------------------------------------------------- */

/*
 * Make the most recently added diagnostic a child of parent_index.
 *
 * parent_index must refer to an earlier diagnostic.
 *
 * The root is resolved automatically from the parent's causal chain.
 *
 * Returns:
 *   VITTE_STATUS_OK
 *   VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *   VITTE_STATUS_ERROR_INVALID_STATE
 */
vitte_status_t
vitte_diagnostic_set_last_parent(
    vitte_diagnostic_bag_t *bag,
    size_t parent_index
);

/*
 * Mark the most recently added diagnostic as a cascade of root_index.
 *
 * root_index must refer to an earlier diagnostic. If root_index itself is a
 * cascade, its ultimate root is resolved automatically.
 */
vitte_status_t
vitte_diagnostic_mark_last_cascade(
    vitte_diagnostic_bag_t *bag,
    size_t root_index
);

/*
 * Mark the most recently added diagnostic as suppressed.
 *
 * Suppression does not remove the diagnostic from the bag. The diagnostic
 * remains available to machine-readable renderers and debugging facilities.
 *
 * Calling this function more than once for the same diagnostic does not
 * increment suppressed_count more than once.
 */
vitte_status_t
vitte_diagnostic_mark_last_suppressed(
    vitte_diagnostic_bag_t *bag
);

/* ------------------------------------------------------------------------- */
/* Pre-emission suppression                                                  */
/* ------------------------------------------------------------------------- */

/*
 * Determine whether a diagnostic about to be emitted is likely to be a
 * cascade of an existing root diagnostic.
 *
 * This is intentionally conservative. A compatible compiler phase alone is
 * insufficient: the candidate must also have a relevant source relationship.
 *
 * Prefer post-emission classification when symbol/context/provenance
 * information is available.
 */
bool
vitte_diagnostic_should_suppress_cascade(
    const vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_origin_t phase,
    const vitte_ast_span_t *span
);

/* ------------------------------------------------------------------------- */
/* Automatic classification                                                  */
/* ------------------------------------------------------------------------- */

/*
 * Classify the most recently added diagnostic.
 *
 * Explicit relationships previously established by a compiler phase are
 * preserved.
 *
 * Otherwise the cascade engine searches earlier diagnostics and evaluates:
 *
 *   - compiler phase dependency;
 *   - source identity;
 *   - identical/contained/overlapping spans;
 *   - symbol identity;
 *   - semantic context;
 *   - diagnostic code;
 *   - phase distance.
 *
 * If suppress is true, a diagnostic classified as derived is also marked as
 * suppressed.
 */
vitte_status_t
vitte_diagnostic_classify_last_cascade(
    vitte_diagnostic_bag_t *bag,
    bool suppress
);

/*
 * Classify all diagnostics currently stored in the bag.
 *
 * The pass is deterministic and processes diagnostics in emission order.
 * Earlier diagnostics can become parents of later diagnostics, never the
 * reverse.
 *
 * Explicit causal relationships take priority over heuristic classification.
 */
vitte_status_t
vitte_diagnostic_classify_cascades(
    vitte_diagnostic_bag_t *bag,
    bool suppress
);

/* ------------------------------------------------------------------------- */
/* Root queries                                                              */
/* ------------------------------------------------------------------------- */

/*
 * Resolve the ultimate root of diagnostic_index.
 *
 * Returns VITTE_DIAGNOSTIC_INDEX_NONE when:
 *
 *   - diagnostic_index is invalid;
 *   - the causal metadata contains an invalid forward reference;
 *   - the causal graph is malformed or cyclic.
 *
 * A primary diagnostic normally resolves to itself.
 */
size_t
vitte_diagnostic_root_index(
    const vitte_diagnostic_bag_t *bag,
    size_t diagnostic_index
);

/*
 * Return true when diagnostic_index ultimately descends from root_index.
 *
 * A diagnostic is not considered a cascade of itself.
 */
bool
vitte_diagnostic_is_cascade_of(
    const vitte_diagnostic_bag_t *bag,
    size_t diagnostic_index,
    size_t root_index
);

/* ------------------------------------------------------------------------- */
/* Statistics                                                                */
/* ------------------------------------------------------------------------- */

/*
 * Return the number of diagnostics marked as cascade diagnostics.
 *
 * Suppressed cascade diagnostics are included.
 */
size_t
vitte_diagnostic_cascade_count(
    const vitte_diagnostic_bag_t *bag
);

/*
 * Return the number of visible independent error/fatal diagnostics.
 *
 * Notes, help diagnostics, warnings, cascades and suppressed diagnostics are
 * excluded.
 */
size_t
vitte_diagnostic_primary_count(
    const vitte_diagnostic_bag_t *bag
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_CASCADE_H */
