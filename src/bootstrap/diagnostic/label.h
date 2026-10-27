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
 * A label associates a source span with a concise contextual explanation.
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
 * Labels are structured diagnostic data. Rendering is deliberately kept
 * outside this subsystem so the same labels can be consumed by:
 *
 *     - terminal diagnostics;
 *     - JSON;
 *     - SARIF;
 *     - LSP;
 *     - editor integrations;
 *     - documentation/debug tools;
 *     - golden tests;
 *     - diagnostic analysis passes.
 *
 * Coordinates
 * -----------
 *
 * Source spans use byte offsets and half-open ranges:
 *
 *     [start_offset, end_offset)
 *
 * A zero-width span represents an insertion point.
 *
 * Byte offsets must not be interpreted directly as UTF-8 character indices,
 * Unicode scalar positions, grapheme positions, terminal display columns or
 * LSP UTF-16 positions. Those conversions belong to the source/source-map and
 * renderer layers.
 *
 * Ownership
 * ---------
 *
 * Labels stored in vitte_diagnostic_t are owned by that diagnostic.
 *
 * Label messages populated through this API are copied into inline
 * label-owned storage. A label can therefore survive the lifetime of the
 * caller's message buffer.
 *
 * vitte_diagnostic_label_t contains pointers into its own inline storage.
 * Moving a structure by assignment, memmove() or qsort() can therefore make
 * those pointers refer to the old object's storage. Functions in this module
 * automatically rebind pointers after structural movement.
 *
 * External code should avoid manually moving labels.
 */

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

/*
 * Sentinel returned by vitte_diagnostic_label_distance() when two labels
 * cannot be spatially compared.
 */
#define VITTE_DIAGNOSTIC_LABEL_DISTANCE_INVALID SIZE_MAX

/* ========================================================================= */
/* Style                                                                     */
/* ========================================================================= */

/*
 * Return true if style is a recognized label style.
 */
bool
vitte_diagnostic_label_style_is_valid(
    vitte_diagnostic_label_style_t style
);

/*
 * Return the stable textual representation of style.
 *
 * Results:
 *
 *     VITTE_DIAGNOSTIC_LABEL_PRIMARY
 *         "primary"
 *
 *     VITTE_DIAGNOSTIC_LABEL_SECONDARY
 *         "secondary"
 *
 *     otherwise
 *         "unknown"
 *
 * The returned string has static storage duration.
 */
const char *
vitte_diagnostic_label_style_name(
    vitte_diagnostic_label_style_t style
);

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

/*
 * Reset label to its canonical empty state.
 *
 * The default style is VITTE_DIAGNOSTIC_LABEL_SECONDARY.
 *
 * Passing NULL is permitted and has no effect.
 */
void
vitte_diagnostic_label_init(
    vitte_diagnostic_label_t *label
);

/* ========================================================================= */
/* Construction                                                              */
/* ========================================================================= */

/*
 * Populate a standalone label.
 *
 * Requirements:
 *
 *     - label must be non-NULL;
 *     - style must be valid;
 *     - span must be valid;
 *     - span->start_offset <= span->end_offset;
 *     - message must fit in label-owned storage when present.
 *
 * The span is copied by value.
 *
 * The message is copied into label-owned storage. Overlong messages are
 * rejected rather than silently truncated because label text participates in
 * equality, deduplication and fingerprints.
 *
 * Returns:
 *
 *     VITTE_STATUS_OK
 *         label constructed successfully.
 *
 *     VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *         invalid label/style/span.
 *
 *     VITTE_STATUS_ERROR_UNSUPPORTED
 *         message exceeds label storage capacity.
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
 * Validate a label.
 *
 * Validation covers:
 *
 *     - label pointer;
 *     - style;
 *     - AST span validity;
 *     - offset ordering;
 *     - owned-message consistency.
 *
 * This function does not verify that source_id/source_name currently resolves
 * through a source manager. Source existence belongs to the source-map layer.
 */
bool
vitte_diagnostic_label_is_valid(
    const vitte_diagnostic_label_t *label
);

/* ========================================================================= */
/* Equality                                                                  */
/* ========================================================================= */

/*
 * Return true if two labels are exactly semantically equal.
 *
 * Equality includes:
 *
 *     - style;
 *     - source identity;
 *     - start offset;
 *     - end offset;
 *     - message.
 *
 * Two NULL pointers compare equal.
 */
bool
vitte_diagnostic_label_equal(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
);

/*
 * Return true if two labels identify exactly the same source range.
 *
 * Style and message are ignored.
 */
bool
vitte_diagnostic_label_same_span(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
);

/* ========================================================================= */
/* Spatial relationships                                                     */
/* ========================================================================= */

/*
 * Return true if outer fully covers inner.
 *
 * Both labels must identify the same source.
 *
 * Zero-width inner spans are insertion points. An insertion point at the end
 * boundary of outer is considered covered. This is useful for diagnostics
 * describing missing delimiters/tokens immediately after a construct.
 */
bool
vitte_diagnostic_label_contains(
    const vitte_diagnostic_label_t *outer,
    const vitte_diagnostic_label_t *inner
);

/*
 * Return true if two labels spatially overlap or touch through a zero-width
 * insertion point.
 *
 * Non-empty ranges use half-open semantics:
 *
 *     [start_offset, end_offset)
 *
 * Zero-width spans receive explicit insertion-point semantics.
 *
 * Labels from different sources never overlap.
 */
bool
vitte_diagnostic_label_overlaps(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
);

/*
 * Return the byte distance between two labels.
 *
 * Results:
 *
 *     0
 *         labels overlap/touch according to label overlap semantics.
 *
 *     > 0
 *         number of bytes separating the ranges.
 *
 *     VITTE_DIAGNOSTIC_LABEL_DISTANCE_INVALID
 *         labels are invalid, NULL or belong to different sources.
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
 * Ordering is stable with respect to:
 *
 *     1. source name/identity;
 *     2. source ID;
 *     3. start offset;
 *     4. end offset;
 *     5. label style;
 *     6. message.
 *
 * Primary labels sort before secondary labels when all preceding keys match.
 *
 * NULL labels sort after non-NULL labels.
 *
 * Return convention:
 *
 *     < 0
 *         left sorts before right.
 *
 *       0
 *         equivalent ordering.
 *
 *     > 0
 *         left sorts after right.
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
 * Add a label to diagnostic.
 *
 * Behavior:
 *
 *     - exact duplicates are ignored;
 *     - semantic duplicates with the same source/span/message are merged;
 *     - PRIMARY dominates SECONDARY for semantic duplicates;
 *     - message text is copied into owned storage;
 *     - label capacity is never exceeded.
 *
 * Returns:
 *
 *     VITTE_STATUS_OK
 *         label added, merged or already present.
 *
 *     VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *         invalid diagnostic/style/span.
 *
 *     VITTE_STATUS_ERROR_UNSUPPORTED
 *         label capacity or message capacity exceeded.
 */
vitte_status_t
vitte_diagnostic_label_add(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_label_style_t style,
    const vitte_ast_span_t *span,
    const char *message
);

/*
 * Add a primary label.
 */
vitte_status_t
vitte_diagnostic_label_add_primary(
    vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span,
    const char *message
);

/*
 * Add a secondary label.
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
 * Return immutable label index.
 *
 * Returns NULL if:
 *
 *     - diagnostic is NULL;
 *     - index >= diagnostic->label_count.
 *
 * The returned pointer is invalidated by operations that remove, sort,
 * normalize or otherwise structurally modify the diagnostic's labels.
 */
const vitte_diagnostic_label_t *
vitte_diagnostic_label_at(
    const vitte_diagnostic_t *diagnostic,
    size_t index
);

/*
 * Return mutable label index.
 *
 * Returns NULL for an invalid diagnostic/index.
 *
 * Direct mutation is intentionally available for compiler internals, but
 * callers must preserve all label invariants. In particular, manually
 * replacing message pointers with externally owned storage is unsupported.
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
 * Count labels matching style.
 *
 * Returns zero for NULL diagnostics or invalid styles.
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
 * Return the first primary label in current collection order.
 *
 * Returns NULL when no primary label exists.
 *
 * For deterministic selection independent of insertion order, normalize or
 * sort the diagnostic first.
 */
const vitte_diagnostic_label_t *
vitte_diagnostic_label_primary(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Removal                                                                   */
/* ========================================================================= */

/*
 * Remove one label.
 *
 * Remaining labels are compacted. Self-referential message pointers are
 * rebound after movement.
 *
 * Returns VITTE_STATUS_ERROR_INVALID_ARGUMENT for an invalid diagnostic/index.
 */
vitte_status_t
vitte_diagnostic_label_remove(
    vitte_diagnostic_t *diagnostic,
    size_t index
);

/*
 * Remove every label.
 *
 * Passing NULL has no effect.
 */
void
vitte_diagnostic_label_clear(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Sorting                                                                   */
/* ========================================================================= */

/*
 * Sort labels according to vitte_diagnostic_label_compare().
 *
 * Self-referential storage pointers are automatically rebound after qsort().
 *
 * Passing NULL or a collection containing fewer than two labels has no effect.
 */
void
vitte_diagnostic_label_sort(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Normalization                                                             */
/* ========================================================================= */

/*
 * Canonicalize diagnostic labels.
 *
 * Normalization performs:
 *
 *     1. invalid-label removal;
 *     2. collection compaction;
 *     3. self-storage pointer rebinding;
 *     4. semantic duplicate merging;
 *     5. PRIMARY-over-SECONDARY promotion for equivalent labels;
 *     6. deterministic sorting.
 *
 * Normalization does not automatically enforce a single-primary policy.
 *
 * The operation is idempotent for an already normalized collection.
 */
vitte_status_t
vitte_diagnostic_label_normalize(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Style mutation                                                            */
/* ========================================================================= */

/*
 * Promote label[index] to primary.
 */
vitte_status_t
vitte_diagnostic_label_promote(
    vitte_diagnostic_t *diagnostic,
    size_t index
);

/*
 * Demote label[index] to secondary.
 */
vitte_status_t
vitte_diagnostic_label_demote(
    vitte_diagnostic_t *diagnostic,
    size_t index
);

/*
 * Ensure that a non-empty label collection contains at least one primary
 * label.
 *
 * If none exists, labels are sorted deterministically and the first label is
 * promoted.
 *
 * Empty collections are valid and return VITTE_STATUS_OK.
 */
vitte_status_t
vitte_diagnostic_label_ensure_primary(
    vitte_diagnostic_t *diagnostic
);

/*
 * Enforce exactly one primary label for a non-empty collection.
 *
 * Behavior:
 *
 *     - labels are first deterministically sorted;
 *     - the first primary is retained;
 *     - subsequent primaries are demoted;
 *     - if no primary exists, the first deterministic label is promoted;
 *     - labels are sorted again after style changes.
 *
 * The core model itself allows multiple primary labels. Call this function
 * only where a renderer/protocol requires a single primary location.
 */
vitte_status_t
vitte_diagnostic_label_enforce_single_primary(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Spatial lookup                                                            */
/* ========================================================================= */

/*
 * Find the smallest label that completely covers span.
 *
 * Selection:
 *
 *     1. same source;
 *     2. covering range;
 *     3. smallest covering byte length;
 *     4. primary over secondary;
 *     5. deterministic label ordering.
 *
 * Returns NULL if no label covers span.
 */
const vitte_diagnostic_label_t *
vitte_diagnostic_label_find_covering(
    const vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span
);

/*
 * Find the nearest label belonging to the same source.
 *
 * Selection:
 *
 *     1. minimum byte distance;
 *     2. primary over secondary;
 *     3. deterministic label ordering.
 *
 * Overlapping/touching labels have distance zero.
 *
 * Returns NULL when no comparable label exists.
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
 * Count distinct sources referenced by labels.
 *
 * Source identity follows the same rules used by label equality/spatial
 * operations.
 */
size_t
vitte_diagnostic_label_source_count(
    const vitte_diagnostic_t *diagnostic
);

/*
 * Return true when labels reference more than one source.
 *
 * Multi-source labels are useful for:
 *
 *     - declaration/use diagnostics;
 *     - imports;
 *     - generic instantiation traces;
 *     - trait/implementation relationships;
 *     - contract call/declaration relationships;
 *     - cross-module diagnostics.
 */
bool
vitte_diagnostic_label_is_multisource(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Fingerprints                                                              */
/* ========================================================================= */

/*
 * Compute a deterministic 64-bit label fingerprint.
 *
 * Fingerprinted information includes:
 *
 *     - style;
 *     - source identity metadata;
 *     - source name;
 *     - start offset;
 *     - end offset;
 *     - message.
 *
 * Integer values are encoded explicitly rather than hashed using host-native
 * byte representation, keeping the result independent of host endianness.
 *
 * Returns zero for an invalid label.
 *
 * Fingerprints are internal metadata. They are not diagnostic codes and must
 * never be exposed as replacements for stable E/W/N/H identifiers.
 */
uint64_t
vitte_diagnostic_label_fingerprint(
    const vitte_diagnostic_label_t *label
);

/*
 * Compute a fingerprint for the current ordered label collection.
 *
 * The collection count and each ordered label fingerprint participate.
 *
 * The result is order-sensitive.
 *
 * Call vitte_diagnostic_label_normalize() first when a canonical collection
 * fingerprint is required.
 */
uint64_t
vitte_diagnostic_labels_fingerprint(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Debug output                                                              */
/* ========================================================================= */

/*
 * Write a compact escaped debug representation of one label.
 *
 * Example:
 *
 *     primary src/main.vit:120-125: expected i32
 *
 * Control characters in names/messages are escaped so one logical label
 * remains one debug record.
 *
 * This is debugging output, not the user-facing terminal renderer.
 */
vitte_status_t
vitte_diagnostic_label_dump(
    FILE *stream,
    const vitte_diagnostic_label_t *label
);

/*
 * Dump every label in current collection order.
 *
 * The stream remains owned by the caller and is never closed.
 */
vitte_status_t
vitte_diagnostic_labels_dump(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Convenience queries                                                       */
/* ========================================================================= */

/*
 * Return true if diagnostic contains at least one label.
 */
static inline bool
vitte_diagnostic_has_labels(
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_label_count(
        diagnostic
    ) != 0u;
}

/*
 * Return true if diagnostic contains at least one primary label.
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
 * Return true if diagnostic contains at least one secondary label.
 */
static inline bool
vitte_diagnostic_has_secondary_label(
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_label_secondary_count(
        diagnostic
    ) != 0u;
}

/*
 * Return true if diagnostic has zero or one distinct label source.
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
 * Return true if diagnostic contains exactly one label.
 */
static inline bool
vitte_diagnostic_has_single_label(
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_label_count(
        diagnostic
    ) == 1u;
}

/*
 * Return true if diagnostic has exactly one primary label.
 *
 * Secondary labels may also be present.
 */
static inline bool
vitte_diagnostic_has_single_primary_label(
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_label_primary_count(
        diagnostic
    ) == 1u;
}

/*
 * Return true if diagnostic has multiple primary labels.
 */
static inline bool
vitte_diagnostic_has_multiple_primary_labels(
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_label_primary_count(
        diagnostic
    ) > 1u;
}

/* ========================================================================= */
/* API guarantees                                                            */
/* ========================================================================= */

/*
 * Ownership
 * ---------
 *
 * Labels stored in vitte_diagnostic_t are owned by that diagnostic.
 *
 * Message strings accepted by vitte_diagnostic_label_set() and
 * vitte_diagnostic_label_add() are copied into inline storage.
 *
 * Returned pointers from label_at(), label_at_mut(), label_primary(),
 * find_covering() and find_nearest() refer to diagnostic-owned storage.
 *
 * Those pointers must not be freed.
 *
 *
 * Pointer stability
 * -----------------
 *
 * Label pointers are not stable across:
 *
 *     - add operations that modify the collection;
 *     - remove;
 *     - clear;
 *     - sort;
 *     - normalize.
 *
 * Callers must reacquire label pointers after structural mutation.
 *
 *
 * Self-referential storage
 * ------------------------
 *
 * vitte_diagnostic_label_t may contain pointers into its own inline storage.
 *
 * Raw structure movement can invalidate those pointers. This implementation
 * repairs pointers after every structural move it performs.
 *
 * External code should not qsort(), memmove() or manually compact the label
 * array.
 *
 *
 * Source semantics
 * ----------------
 *
 * Source spans use byte offsets.
 *
 * The label layer does not attempt to convert offsets into:
 *
 *     - line numbers;
 *     - Unicode scalar columns;
 *     - grapheme columns;
 *     - terminal display columns;
 *     - UTF-16/LSP positions.
 *
 * Such conversions require the original source buffer and belong to the
 * source-map/rendering layer.
 *
 *
 * Zero-width spans
 * ----------------
 *
 * Zero-width labels are valid insertion points.
 *
 * They are particularly useful for:
 *
 *     - missing delimiters;
 *     - missing punctuation;
 *     - missing arguments;
 *     - missing return expressions;
 *     - insertion fix-its.
 *
 *
 * Multiple primaries
 * ------------------
 *
 * The core representation permits multiple primary labels.
 *
 * This is useful for diagnostics where several locations jointly form the
 * principal failure.
 *
 * Consumers requiring one primary location should explicitly invoke
 * vitte_diagnostic_label_enforce_single_primary().
 *
 *
 * Determinism
 * -----------
 *
 * vitte_diagnostic_label_sort() and
 * vitte_diagnostic_label_normalize() provide deterministic collection order.
 *
 * Deterministic order is important for:
 *
 *     - golden tests;
 *     - JSON;
 *     - SARIF;
 *     - LSP;
 *     - reproducible compiler output;
 *     - diagnostic fingerprints;
 *     - cache keys.
 *
 *
 * Threading
 * ---------
 *
 * This subsystem contains no global mutable state.
 *
 * Independent diagnostics may therefore be manipulated concurrently provided
 * the caller does not concurrently mutate the same vitte_diagnostic_t.
 *
 *
 * ABI
 * ---
 *
 * Callers should use this interface rather than depending on the physical
 * ordering of fields inside vitte_diagnostic_label_t.
 *
 * Fingerprints are implementation metadata and are not an ABI substitute for
 * stable diagnostic codes.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_LABEL_H */
