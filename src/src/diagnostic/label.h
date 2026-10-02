#ifndef VITTE_DIAGNOSTIC_LABEL_H
#define VITTE_DIAGNOSTIC_LABEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Vitte diagnostic label subsystem.
 *
 * A diagnostic label associates a source span with a short contextual
 * explanation.
 *
 * Example:
 *
 *     error[E0501]: type mismatch
 *       --> src/main.vit:12:17
 *        |
 *     12 |     let value: i32 = text;
 *        |                ---   ^^^^ expected i32, found string
 *        |                |
 *        |                expected because of this declaration
 *
 * Labels are intentionally independent from terminal rendering. The same
 * structured labels can therefore be consumed by:
 *
 *     - terminal diagnostics;
 *     - JSON output;
 *     - SARIF;
 *     - LSP diagnostics;
 *     - editor integrations;
 *     - tests;
 *     - compiler debugging tools.
 *
 * Source offsets are byte offsets. Conversion to source line, Unicode scalar
 * position, grapheme position or terminal display column belongs to the
 * source/rendering layer.
 */

/* ========================================================================= */
/* Style                                                                     */
/* ========================================================================= */

/*
 * Return true when style is a valid diagnostic-label style.
 */
bool
vitte_diagnostic_label_style_is_valid(
    vitte_diagnostic_label_style_t style
);

/*
 * Return the stable textual representation of a label style.
 *
 * Results:
 *
 *     "primary"
 *     "secondary"
 *     "unknown"
 */
const char *
vitte_diagnostic_label_style_name(
    vitte_diagnostic_label_style_t style
);

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

/*
 * Reset a label to its empty state.
 *
 * The default style is secondary.
 */
void
vitte_diagnostic_label_init(
    vitte_diagnostic_label_t *label
);

/* ========================================================================= */
/* Construction                                                              */
/* ========================================================================= */

/*
 * Initialize/populate a standalone diagnostic label.
 *
 * The span is copied by value.
 *
 * The source name and label message are copied into label-owned storage when
 * present. The resulting label therefore does not depend on the lifetime of
 * the supplied message string.
 */
vitte_status_t
vitte_diagnostic_label_set(
    vitte_diagnostic_label_t *label,
    vitte_diagnostic_label_style_t style,
    const vitte_ast_span_t *span,
    const char *message
);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate a diagnostic label.
 *
 * Validation checks at least:
 *
 *     - non-NULL label;
 *     - valid style;
 *     - valid source span;
 *     - start_offset <= end_offset.
 */
bool
vitte_diagnostic_label_is_valid(
    const vitte_diagnostic_label_t *label
);

/* ========================================================================= */
/* Equality                                                                  */
/* ========================================================================= */

/*
 * Return true when two labels have the same:
 *
 *     - style;
 *     - source identity;
 *     - source span;
 *     - message.
 */
bool
vitte_diagnostic_label_equal(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
);

/*
 * Return true when two labels identify exactly the same source range.
 *
 * Label style and message are ignored.
 */
bool
vitte_diagnostic_label_same_span(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
);

/* ========================================================================= */
/* Span relationships                                                        */
/* ========================================================================= */

/*
 * Return true when outer fully contains inner.
 *
 * Both labels must refer to the same source.
 */
bool
vitte_diagnostic_label_contains(
    const vitte_diagnostic_label_t *outer,
    const vitte_diagnostic_label_t *inner
);

/*
 * Return true when two labels overlap.
 *
 * Source ranges are treated as half-open:
 *
 *     [start_offset, end_offset)
 *
 * Zero-width labels represent insertion points and receive explicit handling.
 */
bool
vitte_diagnostic_label_overlaps(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
);

/*
 * Return the byte distance between two labels.
 *
 * Returns:
 *
 *     0
 *         labels overlap.
 *
 *     > 0
 *         byte distance between non-overlapping ranges.
 *
 *     SIZE_MAX
 *         labels are invalid or belong to different sources.
 */
size_t
vitte_diagnostic_label_distance(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
);

/* ========================================================================= */
/* Ordering                                                                  */
/* ========================================================================= */

/*
 * Deterministically compare two labels.
 *
 * Ordering considers:
 *
 *     1. source identity;
 *     2. source name;
 *     3. start offset;
 *     4. end offset;
 *     5. primary/secondary style;
 *     6. message.
 *
 * Return convention:
 *
 *     < 0   left sorts before right
 *       0   equivalent ordering
 *     > 0   left sorts after right
 */
int
vitte_diagnostic_label_compare(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
);

/* ========================================================================= */
/* Diagnostic mutation                                                       */
/* ========================================================================= */

/*
 * Add a label to a diagnostic.
 *
 * Exact duplicates are ignored.
 *
 * If the same source/span/message already exists as a secondary label and the
 * new label is primary, the existing label is promoted rather than duplicated.
 */
vitte_status_t
vitte_diagnostic_label_add(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_label_style_t style,
    const vitte_ast_span_t *span,
    const char *message
);

/*
 * Convenience wrapper for adding a primary label.
 */
vitte_status_t
vitte_diagnostic_label_add_primary(
    vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span,
    const char *message
);

/*
 * Convenience wrapper for adding a secondary label.
 */
vitte_status_t
vitte_diagnostic_label_add_secondary(
    vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span,
    const char *message
);

/* ========================================================================= */
/* Access                                                                    */
/* ========================================================================= */

/*
 * Return an immutable label by index.
 *
 * Returns NULL when diagnostic is NULL or index is out of range.
 */
const vitte_diagnostic_label_t *
vitte_diagnostic_label_at(
    const vitte_diagnostic_t *diagnostic,
    size_t index
);

/*
 * Return a mutable label by index.
 *
 * Direct mutation should be used carefully because source-name/message
 * pointers may refer to label-owned storage.
 */
vitte_diagnostic_label_t *
vitte_diagnostic_label_at_mut(
    vitte_diagnostic_t *diagnostic,
    size_t index
);

/* ========================================================================= */
/* Counts                                                                    */
/* ========================================================================= */

/*
 * Count labels of a specific style.
 */
size_t
vitte_diagnostic_label_count_style(
    const vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_label_style_t style
);

/*
 * Count primary labels.
 */
size_t
vitte_diagnostic_label_primary_count(
    const vitte_diagnostic_t *diagnostic
);

/*
 * Count secondary labels.
 */
size_t
vitte_diagnostic_label_secondary_count(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Primary label                                                             */
/* ========================================================================= */

/*
 * Return the first primary label.
 *
 * Returns NULL if no primary label exists.
 */
const vitte_diagnostic_label_t *
vitte_diagnostic_label_primary(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Removal                                                                   */
/* ========================================================================= */

/*
 * Remove one label by index.
 *
 * Remaining labels are compacted and all internal pointers to owned storage
 * are rebound after movement.
 */
vitte_status_t
vitte_diagnostic_label_remove(
    vitte_diagnostic_t *diagnostic,
    size_t index
);

/*
 * Remove every label from a diagnostic.
 */
void
vitte_diagnostic_label_clear(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Sorting                                                                   */
/* ========================================================================= */

/*
 * Sort labels deterministically.
 *
 * Because labels contain pointers into their own inline storage, the
 * implementation automatically rebinds those pointers after qsort().
 */
void
vitte_diagnostic_label_sort(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Normalization                                                             */
/* ========================================================================= */

/*
 * Normalize a diagnostic's label collection.
 *
 * Current normalization performs:
 *
 *     - invalid-label removal;
 *     - deterministic sorting;
 *     - exact duplicate removal;
 *     - owned-storage pointer rebinding.
 */
vitte_status_t
vitte_diagnostic_label_normalize(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Style mutation                                                            */
/* ========================================================================= */

/*
 * Promote one label to primary.
 */
vitte_status_t
vitte_diagnostic_label_promote(
    vitte_diagnostic_t *diagnostic,
    size_t index
);

/*
 * Demote one label to secondary.
 */
vitte_status_t
vitte_diagnostic_label_demote(
    vitte_diagnostic_t *diagnostic,
    size_t index
);

/*
 * Ensure that a diagnostic with labels has at least one primary label.
 *
 * If no primary label exists, labels are deterministically sorted and the
 * first label becomes primary.
 */
vitte_status_t
vitte_diagnostic_label_ensure_primary(
    vitte_diagnostic_t *diagnostic
);

/*
 * Enforce a renderer-friendly single-primary policy.
 *
 * The first primary label is preserved. Additional primary labels are demoted
 * to secondary.
 *
 * If no primary label exists and at least one label is present, one label is
 * promoted.
 *
 * The core data model itself does not require this policy; callers that want
 * multiple primary labels may simply avoid invoking this function.
 */
vitte_status_t
vitte_diagnostic_label_enforce_single_primary(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Spatial lookup                                                            */
/* ========================================================================= */

/*
 * Find the smallest label that fully covers span.
 *
 * Tie breaking prefers a primary label.
 */
const vitte_diagnostic_label_t *
vitte_diagnostic_label_find_covering(
    const vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span
);

/*
 * Find the nearest label belonging to the same source.
 *
 * Overlapping labels have distance zero.
 *
 * Equal-distance ties prefer primary labels.
 */
const vitte_diagnostic_label_t *
vitte_diagnostic_label_find_nearest(
    const vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span
);

/* ========================================================================= */
/* Source analysis                                                           */
/* ========================================================================= */

/*
 * Count distinct source files referenced by labels.
 */
size_t
vitte_diagnostic_label_source_count(
    const vitte_diagnostic_t *diagnostic
);

/*
 * Return true when labels refer to more than one source file.
 *
 * This is useful for renderers handling diagnostics such as:
 *
 *     - declaration in one file;
 *     - invalid use in another file;
 *     - generic instantiation elsewhere;
 *     - import/module relationships.
 */
bool
vitte_diagnostic_label_is_multisource(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Fingerprints                                                              */
/* ========================================================================= */

/*
 * Compute a deterministic fingerprint for one label.
 *
 * The fingerprint includes:
 *
 *     - style;
 *     - source identity;
 *     - start/end offsets;
 *     - message.
 *
 * This fingerprint is intended for:
 *
 *     - deduplication support;
 *     - deterministic tests;
 *     - caches;
 *     - debugging.
 *
 * It is not a public diagnostic identifier.
 */
uint64_t
vitte_diagnostic_label_fingerprint(
    const vitte_diagnostic_label_t *label
);

/*
 * Compute a fingerprint for the current ordered label collection.
 *
 * The result depends on label order. Call
 * vitte_diagnostic_label_normalize() first when a canonical fingerprint is
 * required.
 */
uint64_t
vitte_diagnostic_labels_fingerprint(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Debug output                                                              */
/* ========================================================================= */

/*
 * Write a compact debug representation of one label.
 *
 * Example:
 *
 *     primary src/main.vit:120-125: expected i32
 */
vitte_status_t
vitte_diagnostic_label_dump(
    FILE *stream,
    const vitte_diagnostic_label_t *label
);

/*
 * Dump every label belonging to a diagnostic.
 */
vitte_status_t
vitte_diagnostic_labels_dump(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Convenience helpers                                                       */
/* ========================================================================= */

/*
 * Return true when diagnostic contains at least one label.
 */
static inline bool
vitte_diagnostic_has_labels(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL &&
           diagnostic->label_count != 0u;
}

/*
 * Return true when diagnostic contains a primary label.
 */
static inline bool
vitte_diagnostic_has_primary_label(
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_label_primary(
        diagnostic
    ) != NULL;
}

/*
 * Return true when all labels belong to one source or there are no labels.
 */
static inline bool
vitte_diagnostic_labels_single_source(
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_label_source_count(
        diagnostic
    ) <= 1u;
}

/*
 * Return the number of labels.
 */
/* ========================================================================= */
/* API guarantees                                                            */
/* ========================================================================= */

/*
 * Ownership
 * ---------
 *
 * Labels are owned by vitte_diagnostic_t.
 *
 * Source-name and message text copied through vitte_diagnostic_label_set()
 * and vitte_diagnostic_label_add() are stored in inline label-owned buffers.
 *
 *
 * Movement
 * --------
 *
 * vitte_diagnostic_label_t contains pointers into its own inline storage.
 *
 * Raw structure assignment, memmove() or qsort() can therefore temporarily
 * invalidate those pointers. The label implementation performs pointer
 * rebinding after its own structural moves.
 *
 * External code should prefer the functions in this interface instead of
 * manually moving labels.
 *
 *
 * Source positions
 * ----------------
 *
 * start_offset/end_offset are source byte offsets.
 *
 * They must not be interpreted directly as:
 *
 *     - UTF-8 character indices;
 *     - Unicode scalar indices;
 *     - grapheme-cluster indices;
 *     - terminal columns;
 *     - LSP UTF-16 positions.
 *
 * Those conversions belong to the source manager and rendering layers.
 *
 *
 * Determinism
 * -----------
 *
 * Normalized labels have deterministic ordering, which is important for:
 *
 *     - golden diagnostics tests;
 *     - JSON output;
 *     - SARIF;
 *     - LSP;
 *     - reproducible builds;
 *     - diagnostic fingerprints.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_LABEL_H */
