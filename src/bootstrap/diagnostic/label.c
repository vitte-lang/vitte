#include "label.h"

#include "diagnostic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Vitte diagnostic label subsystem.
 *
 * Labels associate source spans with concise explanations.
 *
 * Core guarantees:
 *
 *   - exact byte-based half-open source spans;
 *   - zero-width insertion-point support;
 *   - primary and secondary labels;
 *   - deterministic ordering;
 *   - exact and semantic deduplication;
 *   - bounded inline message storage;
 *   - safe self-storage pointer rebinding after moves/qsort;
 *   - multi-source diagnostics;
 *   - stable fingerprints;
 *   - no ownership of external source buffers;
 *   - no implicit UTF-8/display-column conversion.
 *
 * Source offsets remain byte offsets. Conversion to Unicode scalar columns,
 * grapheme columns or terminal display columns belongs to source_map/renderers.
 */

#define VITTE_LABEL_INVALID_DISTANCE SIZE_MAX

#define VITTE_LABEL_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_LABEL_FNV_PRIME \
    UINT64_C(1099511628211)

/* ========================================================================= */
/* Text helpers                                                              */
/* ========================================================================= */

static bool
vitte_label_text_present(
    const char *text
)
{
    return text != NULL &&
           text[0] != '\0';
}

static bool
vitte_label_text_equal(
    const char *left,
    const char *right
)
{
    const bool left_present =
        vitte_label_text_present(left);

    const bool right_present =
        vitte_label_text_present(right);

    if (!left_present ||
        !right_present) {
        return left_present ==
               right_present;
    }

    return strcmp(left, right) == 0;
}

static size_t
vitte_label_copy_text(
    char *destination,
    size_t capacity,
    const char *source
)
{
    size_t length;

    if (destination == NULL ||
        capacity == 0u) {
        return 0u;
    }

    destination[0] = '\0';

    if (!vitte_label_text_present(source)) {
        return 0u;
    }

    length = strlen(source);

    if (length >= capacity) {
        length = capacity - 1u;
    }

    if (length != 0u) {
        memcpy(
            destination,
            source,
            length
        );
    }

    destination[length] = '\0';

    return length;
}

/* ========================================================================= */
/* Style                                                                     */
/* ========================================================================= */

bool
vitte_diagnostic_label_style_is_valid(
    vitte_diagnostic_label_style_t style
)
{
    switch (style) {
        case VITTE_DIAGNOSTIC_LABEL_PRIMARY:
        case VITTE_DIAGNOSTIC_LABEL_SECONDARY:
            return true;

        default:
            return false;
    }
}

const char *
vitte_diagnostic_label_style_name(
    vitte_diagnostic_label_style_t style
)
{
    switch (style) {
        case VITTE_DIAGNOSTIC_LABEL_PRIMARY:
            return "primary";

        case VITTE_DIAGNOSTIC_LABEL_SECONDARY:
            return "secondary";

        default:
            return "unknown";
    }
}

static unsigned
vitte_label_style_rank(
    vitte_diagnostic_label_style_t style
)
{
    switch (style) {
        case VITTE_DIAGNOSTIC_LABEL_PRIMARY:
            return 0u;

        case VITTE_DIAGNOSTIC_LABEL_SECONDARY:
            return 1u;

        default:
            return 2u;
    }
}

/* ========================================================================= */
/* Span validity                                                             */
/* ========================================================================= */

static bool
vitte_label_span_valid(
    const vitte_ast_span_t *span
)
{
    if (span == NULL) {
        return false;
    }

    if (!vitte_ast_span_is_valid(*span)) {
        return false;
    }

    return span->end_offset >=
           span->start_offset;
}

static bool
vitte_label_span_empty(
    const vitte_ast_span_t *span
)
{
    return vitte_label_span_valid(span) &&
           span->start_offset ==
           span->end_offset;
}

static size_t
vitte_label_span_length(
    const vitte_ast_span_t *span
)
{
    if (!vitte_label_span_valid(span)) {
        return 0u;
    }

    return span->end_offset -
           span->start_offset;
}

static void
vitte_label_copy_span(
    vitte_ast_span_t *destination,
    const vitte_ast_span_t *source
)
{
    if (destination == NULL) {
        return;
    }

    memset(
        destination,
        0,
        sizeof(*destination)
    );

    if (source != NULL) {
        *destination = *source;
    }
}

/* ========================================================================= */
/* Source identity                                                           */
/* ========================================================================= */

/*
 * Do not infer source identity from source_id == 0.
 *
 * Some source managers legitimately use zero as the first source identifier.
 * When a stable source name exists it is therefore used as an additional
 * discriminator.
 *
 * Two spans are considered to refer to the same source when:
 *
 *   1. both have non-empty source names and those names are identical; or
 *   2. their source IDs are identical and their available names do not
 *      contradict that identity.
 *
 * This deliberately avoids assuming a project-specific invalid-ID sentinel.
 */
static bool
vitte_label_same_source_span(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    bool left_named;
    bool right_named;

    if (!vitte_label_span_valid(left) ||
        !vitte_label_span_valid(right)) {
        return false;
    }

    left_named =
        vitte_label_text_present(
            left->source_name
        );

    right_named =
        vitte_label_text_present(
            right->source_name
        );

    if (left_named &&
        right_named) {

        return strcmp(
            left->source_name,
            right->source_name
        ) == 0;
    }

    /*
     * If one or both source names are unavailable, source_id is the only
     * remaining stable identity carried by the span.
     */
    return left->source_id ==
           right->source_id;
}

static bool
vitte_label_same_source(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
)
{
    if (left == NULL ||
        right == NULL) {
        return false;
    }

    return vitte_label_same_source_span(
        &left->span,
        &right->span
    );
}

/* ========================================================================= */
/* Span relations                                                            */
/* ========================================================================= */

bool
vitte_diagnostic_label_same_span(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
)
{
    if (left == NULL ||
        right == NULL ||
        !vitte_label_same_source(
            left,
            right
        )) {
        return false;
    }

    return left->span.start_offset ==
               right->span.start_offset &&
           left->span.end_offset ==
               right->span.end_offset;
}

bool
vitte_diagnostic_label_contains(
    const vitte_diagnostic_label_t *outer,
    const vitte_diagnostic_label_t *inner
)
{
    if (outer == NULL ||
        inner == NULL ||
        !vitte_label_same_source(
            outer,
            inner
        )) {
        return false;
    }

    /*
     * Half-open ranges:
     *
     *     [start, end)
     *
     * Empty labels are insertion points. A zero-width insertion point at the
     * end boundary is accepted as being covered by the enclosing label. This
     * is useful for diagnostics such as a missing closing delimiter.
     */
    if (vitte_label_span_empty(
            &inner->span
        )) {

        return inner->span.start_offset >=
                   outer->span.start_offset &&
               inner->span.start_offset <=
                   outer->span.end_offset;
    }

    return outer->span.start_offset <=
               inner->span.start_offset &&
           outer->span.end_offset >=
               inner->span.end_offset;
}

bool
vitte_diagnostic_label_overlaps(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
)
{
    bool left_empty;
    bool right_empty;

    if (left == NULL ||
        right == NULL ||
        !vitte_label_same_source(
            left,
            right
        )) {
        return false;
    }

    left_empty =
        vitte_label_span_empty(
            &left->span
        );

    right_empty =
        vitte_label_span_empty(
            &right->span
        );

    if (left_empty &&
        right_empty) {

        return left->span.start_offset ==
               right->span.start_offset;
    }

    /*
     * Insertion points are considered touching a range at either boundary.
     * This is intentionally slightly broader than mathematical half-open
     * intersection because it models diagnostics for missing tokens.
     */
    if (left_empty) {
        return left->span.start_offset >=
                   right->span.start_offset &&
               left->span.start_offset <=
                   right->span.end_offset;
    }

    if (right_empty) {
        return right->span.start_offset >=
                   left->span.start_offset &&
               right->span.start_offset <=
                   left->span.end_offset;
    }

    return left->span.start_offset <
               right->span.end_offset &&
           right->span.start_offset <
               left->span.end_offset;
}

size_t
vitte_diagnostic_label_distance(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
)
{
    if (left == NULL ||
        right == NULL ||
        !vitte_label_same_source(
            left,
            right
        )) {

        return VITTE_LABEL_INVALID_DISTANCE;
    }

    if (vitte_diagnostic_label_overlaps(
            left,
            right
        )) {
        return 0u;
    }

    if (left->span.end_offset <=
        right->span.start_offset) {

        return right->span.start_offset -
               left->span.end_offset;
    }

    if (right->span.end_offset <=
        left->span.start_offset) {

        return left->span.start_offset -
               right->span.end_offset;
    }

    return 0u;
}

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

void
vitte_diagnostic_label_init(
    vitte_diagnostic_label_t *label
)
{
    if (label == NULL) {
        return;
    }

    memset(
        label,
        0,
        sizeof(*label)
    );

    label->style =
        VITTE_DIAGNOSTIC_LABEL_SECONDARY;
}

/* ========================================================================= */
/* Self-storage rebinding                                                    */
/* ========================================================================= */

/*
 * vitte_diagnostic_label_t owns message_storage while message may point into
 * that storage.
 *
 * Assignment, memmove and qsort copy the pointer value verbatim. After a move,
 * that pointer may still reference the previous object's storage. Every
 * structural move must therefore be followed by rebinding.
 */
static void
vitte_label_rebind_storage(
    vitte_diagnostic_label_t *label
)
{
    if (label == NULL) {
        return;
    }

    label->message =
        label->message_storage[0] != '\0'
        ? label->message_storage
        : NULL;
}

static void
vitte_label_rebind_range(
    vitte_diagnostic_label_t *labels,
    size_t count
)
{
    size_t index;

    if (labels == NULL) {
        return;
    }

    for (index = 0u;
         index < count;
         index++) {

        vitte_label_rebind_storage(
            &labels[index]
        );
    }
}

static void
vitte_label_rebind_diagnostic(
    vitte_diagnostic_t *diagnostic
)
{
    if (diagnostic == NULL) {
        return;
    }

    vitte_label_rebind_range(
        diagnostic->labels,
        diagnostic->label_count
    );
}

/* ========================================================================= */
/* Construction                                                              */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_label_set(
    vitte_diagnostic_label_t *label,
    vitte_diagnostic_label_style_t style,
    const vitte_ast_span_t *span,
    const char *message
)
{
    size_t message_length;

    if (label == NULL ||
        !vitte_diagnostic_label_style_is_valid(
            style
        ) ||
        !vitte_label_span_valid(
            span
        )) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    /*
     * Reject overlong text instead of silently changing diagnostic semantics
     * through truncation. Exact diagnostic text participates in equality and
     * fingerprinting, so silent truncation is unsafe.
     */
    if (vitte_label_text_present(message)) {
        message_length =
            strlen(message);

        if (message_length >=
            sizeof(label->message_storage)) {

            return VITTE_STATUS_ERROR_UNSUPPORTED;
        }
    }

    vitte_diagnostic_label_init(
        label
    );

    label->style = style;

    vitte_label_copy_span(
        &label->span,
        span
    );

    if (vitte_label_text_present(message)) {
        (void)vitte_label_copy_text(
            label->message_storage,
            sizeof(label->message_storage),
            message
        );

        label->message =
            label->message_storage;
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_diagnostic_label_is_valid(
    const vitte_diagnostic_label_t *label
)
{
    if (label == NULL) {
        return false;
    }

    if (!vitte_diagnostic_label_style_is_valid(
            label->style
        )) {
        return false;
    }

    if (!vitte_label_span_valid(
            &label->span
        )) {
        return false;
    }

    /*
     * If a message is present, it must be represented by owned storage.
     * This keeps labels safe across bag moves, sorting and serialization.
     */
    if (vitte_label_text_present(
            label->message
        )) {

        if (label->message_storage[0] == '\0') {
            return false;
        }

        if (strcmp(
                label->message,
                label->message_storage
            ) != 0) {
            return false;
        }
    } else if (label->message_storage[0] != '\0') {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Equality                                                                  */
/* ========================================================================= */

bool
vitte_diagnostic_label_equal(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
)
{
    if (left == NULL ||
        right == NULL) {
        return left == right;
    }

    if (left->style !=
        right->style) {
        return false;
    }

    if (!vitte_diagnostic_label_same_span(
            left,
            right
        )) {
        return false;
    }

    return vitte_label_text_equal(
        left->message,
        right->message
    );
}

/* ========================================================================= */
/* Ordering                                                                  */
/* ========================================================================= */

static int
vitte_label_compare_source(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    bool left_named;
    bool right_named;

    left_named =
        vitte_label_text_present(
            left->source_name
        );

    right_named =
        vitte_label_text_present(
            right->source_name
        );

    if (left_named &&
        right_named) {

        int comparison;

        comparison =
            strcmp(
                left->source_name,
                right->source_name
            );

        if (comparison != 0) {
            return comparison;
        }
    } else if (left_named != right_named) {
        /*
         * Named sources sort before anonymous ones.
         */
        return left_named
            ? -1
            : 1;
    }

    if (left->source_id !=
        right->source_id) {

        return left->source_id <
                   right->source_id
            ? -1
            : 1;
    }

    return 0;
}

int
vitte_diagnostic_label_compare(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
)
{
    int comparison;
    unsigned left_rank;
    unsigned right_rank;

    if (left == NULL &&
        right == NULL) {
        return 0;
    }

    if (left == NULL) {
        return 1;
    }

    if (right == NULL) {
        return -1;
    }

    comparison =
        vitte_label_compare_source(
            &left->span,
            &right->span
        );

    if (comparison != 0) {
        return comparison;
    }

    if (left->span.start_offset !=
        right->span.start_offset) {

        return left->span.start_offset <
                   right->span.start_offset
            ? -1
            : 1;
    }

    if (left->span.end_offset !=
        right->span.end_offset) {

        return left->span.end_offset <
                   right->span.end_offset
            ? -1
            : 1;
    }

    left_rank =
        vitte_label_style_rank(
            left->style
        );

    right_rank =
        vitte_label_style_rank(
            right->style
        );

    if (left_rank !=
        right_rank) {

        return left_rank <
                   right_rank
            ? -1
            : 1;
    }

    if (!vitte_label_text_present(
            left->message
        ) &&
        vitte_label_text_present(
            right->message
        )) {

        return -1;
    }

    if (vitte_label_text_present(
            left->message
        ) &&
        !vitte_label_text_present(
            right->message
        )) {

        return 1;
    }

    if (vitte_label_text_present(
            left->message
        )) {

        return strcmp(
            left->message,
            right->message
        );
    }

    return 0;
}

static int
vitte_label_qsort_compare(
    const void *left,
    const void *right
)
{
    return vitte_diagnostic_label_compare(
        (const vitte_diagnostic_label_t *)left,
        (const vitte_diagnostic_label_t *)right
    );
}

/* ========================================================================= */
/* Duplicate lookup                                                          */
/* ========================================================================= */

static size_t
vitte_label_find_duplicate(
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_label_t *candidate
)
{
    size_t index;

    if (diagnostic == NULL ||
        candidate == NULL) {
        return VITTE_DIAGNOSTIC_INDEX_NONE;
    }

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        if (vitte_diagnostic_label_equal(
                &diagnostic->labels[index],
                candidate
            )) {

            return index;
        }
    }

    return VITTE_DIAGNOSTIC_INDEX_NONE;
}

/*
 * Semantic duplicates have identical source/span/message but may differ in
 * style. PRIMARY dominates SECONDARY.
 */
static size_t
vitte_label_find_semantic_duplicate(
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_label_t *candidate
)
{
    size_t index;

    if (diagnostic == NULL ||
        candidate == NULL) {
        return VITTE_DIAGNOSTIC_INDEX_NONE;
    }

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        const vitte_diagnostic_label_t *existing;

        existing =
            &diagnostic->labels[index];

        if (!vitte_diagnostic_label_same_span(
                existing,
                candidate
            )) {
            continue;
        }

        if (!vitte_label_text_equal(
                existing->message,
                candidate->message
            )) {
            continue;
        }

        return index;
    }

    return VITTE_DIAGNOSTIC_INDEX_NONE;
}

/* ========================================================================= */
/* Add                                                                       */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_label_add(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_label_style_t style,
    const vitte_ast_span_t *span,
    const char *message
)
{
    vitte_diagnostic_label_t candidate;
    vitte_status_t status;
    size_t duplicate;
    size_t destination_index;

    if (diagnostic == NULL ||
        !vitte_diagnostic_label_style_is_valid(
            style
        ) ||
        !vitte_label_span_valid(
            span
        )) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_label_init(
        &candidate
    );

    status =
        vitte_diagnostic_label_set(
            &candidate,
            style,
            span,
            message
        );

    if (status != VITTE_STATUS_OK) {
        return status;
    }

    duplicate =
        vitte_label_find_duplicate(
            diagnostic,
            &candidate
        );

    if (duplicate !=
        VITTE_DIAGNOSTIC_INDEX_NONE) {

        return VITTE_STATUS_OK;
    }

    duplicate =
        vitte_label_find_semantic_duplicate(
            diagnostic,
            &candidate
        );

    if (duplicate !=
        VITTE_DIAGNOSTIC_INDEX_NONE) {

        vitte_diagnostic_label_t *existing;

        existing =
            &diagnostic->labels[duplicate];

        if (candidate.style ==
                VITTE_DIAGNOSTIC_LABEL_PRIMARY &&
            existing->style ==
                VITTE_DIAGNOSTIC_LABEL_SECONDARY) {

            existing->style =
                VITTE_DIAGNOSTIC_LABEL_PRIMARY;
        }

        return VITTE_STATUS_OK;
    }

    if (diagnostic->label_count >=
        VITTE_DIAGNOSTIC_MAX_LABELS) {

        return VITTE_STATUS_ERROR_UNSUPPORTED;
    }

    destination_index =
        diagnostic->label_count;

    diagnostic->labels[destination_index] =
        candidate;

    vitte_label_rebind_storage(
        &diagnostic->labels[destination_index]
    );

    diagnostic->label_count++;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Convenience adders                                                        */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_label_add_primary(
    vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span,
    const char *message
)
{
    return vitte_diagnostic_label_add(
        diagnostic,
        VITTE_DIAGNOSTIC_LABEL_PRIMARY,
        span,
        message
    );
}

vitte_status_t
vitte_diagnostic_label_add_secondary(
    vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span,
    const char *message
)
{
    return vitte_diagnostic_label_add(
        diagnostic,
        VITTE_DIAGNOSTIC_LABEL_SECONDARY,
        span,
        message
    );
}

/* ========================================================================= */
/* Access                                                                    */
/* ========================================================================= */

const vitte_diagnostic_label_t *
vitte_diagnostic_label_at(
    const vitte_diagnostic_t *diagnostic,
    size_t index
)
{
    if (diagnostic == NULL ||
        index >= diagnostic->label_count) {
        return NULL;
    }

    return &diagnostic->labels[index];
}

vitte_diagnostic_label_t *
vitte_diagnostic_label_at_mut(
    vitte_diagnostic_t *diagnostic,
    size_t index
)
{
    if (diagnostic == NULL ||
        index >= diagnostic->label_count) {
        return NULL;
    }

    return &diagnostic->labels[index];
}

/* ========================================================================= */
/* Counts                                                                    */
/* ========================================================================= */

size_t
vitte_diagnostic_label_count_style(
    const vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_label_style_t style
)
{
    size_t index;
    size_t count;

    if (diagnostic == NULL ||
        !vitte_diagnostic_label_style_is_valid(
            style
        )) {

        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        if (diagnostic->labels[index].style ==
            style) {

            count++;
        }
    }

    return count;
}

size_t
vitte_diagnostic_label_primary_count(
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_label_count_style(
        diagnostic,
        VITTE_DIAGNOSTIC_LABEL_PRIMARY
    );
}

size_t
vitte_diagnostic_label_secondary_count(
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_label_count_style(
        diagnostic,
        VITTE_DIAGNOSTIC_LABEL_SECONDARY
    );
}

/* ========================================================================= */
/* Primary label                                                             */
/* ========================================================================= */

const vitte_diagnostic_label_t *
vitte_diagnostic_label_primary(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return NULL;
    }

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        if (diagnostic->labels[index].style ==
            VITTE_DIAGNOSTIC_LABEL_PRIMARY) {

            return &diagnostic->labels[index];
        }
    }

    return NULL;
}

/* ========================================================================= */
/* Remove                                                                    */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_label_remove(
    vitte_diagnostic_t *diagnostic,
    size_t index
)
{
    size_t old_count;

    if (diagnostic == NULL ||
        index >= diagnostic->label_count) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    old_count =
        diagnostic->label_count;

    if (index + 1u <
        old_count) {

        memmove(
            &diagnostic->labels[index],
            &diagnostic->labels[index + 1u],
            (old_count - index - 1u) *
                sizeof(diagnostic->labels[0])
        );
    }

    diagnostic->label_count =
        old_count - 1u;

    vitte_label_rebind_range(
        &diagnostic->labels[index],
        diagnostic->label_count - index
    );

    vitte_diagnostic_label_init(
        &diagnostic->labels[
            diagnostic->label_count
        ]
    );

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Clear                                                                     */
/* ========================================================================= */

void
vitte_diagnostic_label_clear(
    vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return;
    }

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        vitte_diagnostic_label_init(
            &diagnostic->labels[index]
        );
    }

    diagnostic->label_count = 0u;
}

/* ========================================================================= */
/* Sort                                                                      */
/* ========================================================================= */

void
vitte_diagnostic_label_sort(
    vitte_diagnostic_t *diagnostic
)
{
    if (diagnostic == NULL ||
        diagnostic->label_count < 2u) {
        return;
    }

    qsort(
        diagnostic->labels,
        diagnostic->label_count,
        sizeof(diagnostic->labels[0]),
        vitte_label_qsort_compare
    );

    vitte_label_rebind_diagnostic(
        diagnostic
    );
}

/* ========================================================================= */
/* Normalize                                                                 */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_label_normalize(
    vitte_diagnostic_t *diagnostic
)
{
    size_t original_count;
    size_t read_index;
    size_t write_index;

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (diagnostic->label_count >
        VITTE_DIAGNOSTIC_MAX_LABELS) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    original_count =
        diagnostic->label_count;

    /*
     * Compact valid labels.
     *
     * Important: update label_count immediately after compaction. The older
     * implementation scanned the original count again after clearing the
     * tail, which could accidentally reconstruct an incorrect count.
     */
    write_index = 0u;

    for (read_index = 0u;
         read_index < original_count;
         read_index++) {

        if (!vitte_diagnostic_label_is_valid(
                &diagnostic->labels[
                    read_index
                ]
            )) {
            continue;
        }

        if (write_index !=
            read_index) {

            diagnostic->labels[write_index] =
                diagnostic->labels[
                    read_index
                ];

            vitte_label_rebind_storage(
                &diagnostic->labels[
                    write_index
                ]
            );
        }

        write_index++;
    }

    diagnostic->label_count =
        write_index;

    for (read_index = write_index;
         read_index < original_count;
         read_index++) {

        vitte_diagnostic_label_init(
            &diagnostic->labels[
                read_index
            ]
        );
    }

    /*
     * Merge semantic duplicates before sorting.
     *
     * This also collapses PRIMARY/SECONDARY duplicates into one PRIMARY
     * label.
     */
    if (diagnostic->label_count > 1u) {
        size_t count_before;
        size_t output_count;

        count_before =
            diagnostic->label_count;

        output_count = 0u;

        for (read_index = 0u;
             read_index < count_before;
             read_index++) {

            vitte_diagnostic_label_t candidate;
            size_t existing_index;
            bool found;

            candidate =
                diagnostic->labels[
                    read_index
                ];

            /*
             * candidate was copied by value and may point into the original
             * label. Rebind it before comparisons.
             */
            vitte_label_rebind_storage(
                &candidate
            );

            found = false;
            existing_index =
                VITTE_DIAGNOSTIC_INDEX_NONE;

            {
                size_t index;

                for (index = 0u;
                     index < output_count;
                     index++) {

                    if (!vitte_diagnostic_label_same_span(
                            &diagnostic->labels[index],
                            &candidate
                        )) {
                        continue;
                    }

                    if (!vitte_label_text_equal(
                            diagnostic->labels[index].message,
                            candidate.message
                        )) {
                        continue;
                    }

                    found = true;
                    existing_index = index;
                    break;
                }
            }

            if (found) {
                if (candidate.style ==
                        VITTE_DIAGNOSTIC_LABEL_PRIMARY &&
                    diagnostic->labels[
                        existing_index
                    ].style ==
                        VITTE_DIAGNOSTIC_LABEL_SECONDARY) {

                    diagnostic->labels[
                        existing_index
                    ].style =
                        VITTE_DIAGNOSTIC_LABEL_PRIMARY;
                }

                continue;
            }

            diagnostic->labels[
                output_count
            ] = candidate;

            vitte_label_rebind_storage(
                &diagnostic->labels[
                    output_count
                ]
            );

            output_count++;
        }

        for (read_index = output_count;
             read_index < count_before;
             read_index++) {

            vitte_diagnostic_label_init(
                &diagnostic->labels[
                    read_index
                ]
            );
        }

        diagnostic->label_count =
            output_count;
    }

    vitte_diagnostic_label_sort(
        diagnostic
    );

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Promote / demote                                                          */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_label_promote(
    vitte_diagnostic_t *diagnostic,
    size_t index
)
{
    if (diagnostic == NULL ||
        index >= diagnostic->label_count) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    diagnostic->labels[index].style =
        VITTE_DIAGNOSTIC_LABEL_PRIMARY;

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_label_demote(
    vitte_diagnostic_t *diagnostic,
    size_t index
)
{
    if (diagnostic == NULL ||
        index >= diagnostic->label_count) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    diagnostic->labels[index].style =
        VITTE_DIAGNOSTIC_LABEL_SECONDARY;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Ensure primary                                                            */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_label_ensure_primary(
    vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (diagnostic->label_count == 0u) {
        return VITTE_STATUS_OK;
    }

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        if (diagnostic->labels[index].style ==
            VITTE_DIAGNOSTIC_LABEL_PRIMARY) {

            return VITTE_STATUS_OK;
        }
    }

    vitte_diagnostic_label_sort(
        diagnostic
    );

    diagnostic->labels[0].style =
        VITTE_DIAGNOSTIC_LABEL_PRIMARY;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Single-primary policy                                                     */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_label_enforce_single_primary(
    vitte_diagnostic_t *diagnostic
)
{
    size_t index;
    size_t primary_index;

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (diagnostic->label_count == 0u) {
        return VITTE_STATUS_OK;
    }

    /*
     * Sort first so the retained primary is deterministic.
     */
    vitte_diagnostic_label_sort(
        diagnostic
    );

    primary_index =
        VITTE_DIAGNOSTIC_INDEX_NONE;

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        if (diagnostic->labels[index].style !=
            VITTE_DIAGNOSTIC_LABEL_PRIMARY) {
            continue;
        }

        if (primary_index ==
            VITTE_DIAGNOSTIC_INDEX_NONE) {

            primary_index = index;
            continue;
        }

        diagnostic->labels[index].style =
            VITTE_DIAGNOSTIC_LABEL_SECONDARY;
    }

    if (primary_index ==
        VITTE_DIAGNOSTIC_INDEX_NONE) {

        diagnostic->labels[0].style =
            VITTE_DIAGNOSTIC_LABEL_PRIMARY;
    }

    /*
     * Style changes affect deterministic ordering.
     */
    vitte_diagnostic_label_sort(
        diagnostic
    );

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Find covering label                                                       */
/* ========================================================================= */

const vitte_diagnostic_label_t *
vitte_diagnostic_label_find_covering(
    const vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_label_t probe;
    const vitte_diagnostic_label_t *best;
    size_t best_length;
    size_t index;

    if (diagnostic == NULL ||
        !vitte_label_span_valid(
            span
        )) {

        return NULL;
    }

    vitte_diagnostic_label_init(
        &probe
    );

    if (vitte_diagnostic_label_set(
            &probe,
            VITTE_DIAGNOSTIC_LABEL_SECONDARY,
            span,
            NULL
        ) != VITTE_STATUS_OK) {

        return NULL;
    }

    best = NULL;
    best_length =
        VITTE_LABEL_INVALID_DISTANCE;

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        const vitte_diagnostic_label_t *candidate;
        size_t length;

        candidate =
            &diagnostic->labels[index];

        if (!vitte_diagnostic_label_contains(
                candidate,
                &probe
            )) {
            continue;
        }

        length =
            vitte_label_span_length(
                &candidate->span
            );

        if (best == NULL ||
            length < best_length ||
            (
                length == best_length &&
                candidate->style ==
                    VITTE_DIAGNOSTIC_LABEL_PRIMARY &&
                best->style ==
                    VITTE_DIAGNOSTIC_LABEL_SECONDARY
            ) ||
            (
                length == best_length &&
                candidate->style ==
                    best->style &&
                vitte_diagnostic_label_compare(
                    candidate,
                    best
                ) < 0
            )) {

            best = candidate;
            best_length = length;
        }
    }

    return best;
}

/* ========================================================================= */
/* Find nearest label                                                        */
/* ========================================================================= */

const vitte_diagnostic_label_t *
vitte_diagnostic_label_find_nearest(
    const vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span
)
{
    vitte_diagnostic_label_t probe;
    const vitte_diagnostic_label_t *best;
    size_t best_distance;
    size_t index;

    if (diagnostic == NULL ||
        !vitte_label_span_valid(
            span
        )) {

        return NULL;
    }

    vitte_diagnostic_label_init(
        &probe
    );

    if (vitte_diagnostic_label_set(
            &probe,
            VITTE_DIAGNOSTIC_LABEL_SECONDARY,
            span,
            NULL
        ) != VITTE_STATUS_OK) {

        return NULL;
    }

    best = NULL;
    best_distance =
        VITTE_LABEL_INVALID_DISTANCE;

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        const vitte_diagnostic_label_t *candidate;
        size_t distance;

        candidate =
            &diagnostic->labels[index];

        distance =
            vitte_diagnostic_label_distance(
                candidate,
                &probe
            );

        if (distance ==
            VITTE_LABEL_INVALID_DISTANCE) {
            continue;
        }

        if (best == NULL ||
            distance < best_distance ||
            (
                distance == best_distance &&
                candidate->style ==
                    VITTE_DIAGNOSTIC_LABEL_PRIMARY &&
                best->style ==
                    VITTE_DIAGNOSTIC_LABEL_SECONDARY
            ) ||
            (
                distance == best_distance &&
                candidate->style ==
                    best->style &&
                vitte_diagnostic_label_compare(
                    candidate,
                    best
                ) < 0
            )) {

            best = candidate;
            best_distance = distance;
        }
    }

    return best;
}

/* ========================================================================= */
/* Source count                                                              */
/* ========================================================================= */

size_t
vitte_diagnostic_label_source_count(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;
    size_t count;

    if (diagnostic == NULL) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        size_t previous;
        bool seen;

        seen = false;

        for (previous = 0u;
             previous < index;
             previous++) {

            if (vitte_label_same_source(
                    &diagnostic->labels[index],
                    &diagnostic->labels[previous]
                )) {

                seen = true;
                break;
            }
        }

        if (!seen) {
            count++;
        }
    }

    return count;
}

bool
vitte_diagnostic_label_is_multisource(
    const vitte_diagnostic_t *diagnostic
)
{
    return vitte_diagnostic_label_source_count(
        diagnostic
    ) > 1u;
}

/* ========================================================================= */
/* Hash helpers                                                              */
/* ========================================================================= */

static uint64_t
vitte_label_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t size
)
{
    const unsigned char *bytes;
    size_t index;

    if (data == NULL) {
        hash ^= UINT64_C(0xff);
        hash *= VITTE_LABEL_FNV_PRIME;
        return hash;
    }

    bytes =
        (const unsigned char *)data;

    for (index = 0u;
         index < size;
         index++) {

        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_LABEL_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_label_hash_u64(
    uint64_t hash,
    uint64_t value
)
{
    unsigned shift;

    /*
     * Encode explicitly in little-endian logical order instead of hashing the
     * host representation. This keeps fingerprints stable across host
     * endianness.
     */
    for (shift = 0u;
         shift < 64u;
         shift += 8u) {

        unsigned char byte;

        byte =
            (unsigned char)(
                (value >> shift) &
                UINT64_C(0xff)
            );

        hash =
            vitte_label_hash_bytes(
                hash,
                &byte,
                1u
            );
    }

    return hash;
}

static uint64_t
vitte_label_hash_size(
    uint64_t hash,
    size_t value
)
{
    return vitte_label_hash_u64(
        hash,
        (uint64_t)value
    );
}

static uint64_t
vitte_label_hash_text(
    uint64_t hash,
    const char *text
)
{
    if (!vitte_label_text_present(text)) {
        static const unsigned char empty_marker = 0u;

        return vitte_label_hash_bytes(
            hash,
            &empty_marker,
            sizeof(empty_marker)
        );
    }

    hash =
        vitte_label_hash_bytes(
            hash,
            text,
            strlen(text)
        );

    {
        static const unsigned char terminator = 0u;

        hash =
            vitte_label_hash_bytes(
                hash,
                &terminator,
                sizeof(terminator)
            );
    }

    return hash;
}

/* ========================================================================= */
/* Label fingerprint                                                         */
/* ========================================================================= */

uint64_t
vitte_diagnostic_label_fingerprint(
    const vitte_diagnostic_label_t *label
)
{
    uint64_t hash;

    if (!vitte_diagnostic_label_is_valid(
            label
        )) {

        return UINT64_C(0);
    }

    hash =
        VITTE_LABEL_FNV_OFFSET;

    hash =
        vitte_label_hash_u64(
            hash,
            (uint64_t)label->style
        );

    /*
     * Hash both source ID and source name. This avoids platform-dependent
     * identity rules and minimizes collisions between independently loaded
     * source contexts.
     */
    hash =
        vitte_label_hash_u64(
            hash,
            (uint64_t)label->span.source_id
        );

    hash =
        vitte_label_hash_text(
            hash,
            label->span.source_name
        );

    hash =
        vitte_label_hash_size(
            hash,
            label->span.start_offset
        );

    hash =
        vitte_label_hash_size(
            hash,
            label->span.end_offset
        );

    hash =
        vitte_label_hash_text(
            hash,
            label->message
        );

    return hash;
}

/* ========================================================================= */
/* Diagnostic label fingerprint                                              */
/* ========================================================================= */

uint64_t
vitte_diagnostic_labels_fingerprint(
    const vitte_diagnostic_t *diagnostic
)
{
    uint64_t hash;
    size_t index;

    if (diagnostic == NULL) {
        return UINT64_C(0);
    }

    hash =
        VITTE_LABEL_FNV_OFFSET;

    hash =
        vitte_label_hash_size(
            hash,
            diagnostic->label_count
        );

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        uint64_t label_hash;

        label_hash =
            vitte_diagnostic_label_fingerprint(
                &diagnostic->labels[index]
            );

        hash =
            vitte_label_hash_u64(
                hash,
                label_hash
            );
    }

    return hash;
}

/* ========================================================================= */
/* Debug escaping                                                            */
/* ========================================================================= */

static vitte_status_t
vitte_label_dump_text(
    FILE *stream,
    const char *text
)
{
    const unsigned char *cursor;

    if (stream == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (!vitte_label_text_present(text)) {
        return VITTE_STATUS_OK;
    }

    cursor =
        (const unsigned char *)text;

    while (*cursor != '\0') {
        unsigned char character;

        character = *cursor++;

        switch (character) {
            case '\\':
                if (fputs("\\\\", stream) < 0) {
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
                break;

            case '\n':
                if (fputs("\\n", stream) < 0) {
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
                break;

            case '\r':
                if (fputs("\\r", stream) < 0) {
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
                break;

            case '\t':
                if (fputs("\\t", stream) < 0) {
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
                break;

            default:
                if (character < 0x20u ||
                    character == 0x7fu) {

                    if (fprintf(
                            stream,
                            "\\x%02x",
                            (unsigned)character
                        ) < 0) {

                        return VITTE_STATUS_ERROR_INVALID_STATE;
                    }
                } else if (fputc(
                               (int)character,
                               stream
                           ) == EOF) {

                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
                break;
        }
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Debug dump                                                                */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_label_dump(
    FILE *stream,
    const vitte_diagnostic_label_t *label
)
{
    const char *source_name;
    vitte_status_t status;

    if (stream == NULL ||
        !vitte_diagnostic_label_is_valid(
            label
        )) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    source_name =
        vitte_label_text_present(
            label->span.source_name
        )
        ? label->span.source_name
        : "<unknown>";

    if (fprintf(
            stream,
            "%s ",
            vitte_diagnostic_label_style_name(
                label->style
            )
        ) < 0) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    status =
        vitte_label_dump_text(
            stream,
            source_name
        );

    if (status != VITTE_STATUS_OK) {
        return status;
    }

    if (fprintf(
            stream,
            ":%zu-%zu",
            label->span.start_offset,
            label->span.end_offset
        ) < 0) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (vitte_label_text_present(
            label->message
        )) {

        if (fputs(": ", stream) < 0) {
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }

        status =
            vitte_label_dump_text(
                stream,
                label->message
            );

        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    if (fputc('\n', stream) == EOF) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return ferror(stream)
        ? VITTE_STATUS_ERROR_INVALID_STATE
        : VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Diagnostic dump                                                           */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_labels_dump(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (stream == NULL ||
        diagnostic == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        vitte_status_t status;

        status =
            vitte_diagnostic_label_dump(
                stream,
                &diagnostic->labels[index]
            );

        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    return ferror(stream)
        ? VITTE_STATUS_ERROR_INVALID_STATE
        : VITTE_STATUS_OK;
}

#undef VITTE_LABEL_FNV_PRIME
#undef VITTE_LABEL_FNV_OFFSET