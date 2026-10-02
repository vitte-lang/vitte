#include "label.h"

#include "diagnostic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Vitte diagnostic labels.
 *
 * A label associates a source span with a short explanation.
 *
 * Typical rendering:
 *
 *     error[E0501]: type mismatch
 *       --> src/main.vit:12:17
 *        |
 *     12 |     let x: i32 = value;
 *        |            ---   ^^^^^ expected i32
 *        |            |
 *        |            declared as i32
 *
 * Design goals:
 *
 *   - exact source spans;
 *   - primary and secondary labels;
 *   - multiple labels per diagnostic;
 *   - deterministic ordering;
 *   - deduplication;
 *   - safe bounded storage;
 *   - zero-width span support;
 *   - multi-file diagnostics;
 *   - Unicode-safe byte offsets;
 *   - no ownership of external source buffers;
 *   - stable behavior for terminal/JSON/SARIF/LSP renderers.
 *
 * Important:
 *
 * Byte offsets are preserved here. Character/display-column conversion belongs
 * to the source/rendering layer because UTF-8 bytes, Unicode scalar values,
 * grapheme clusters and terminal display columns are different concepts.
 */

/* ========================================================================= */
/* Internal constants                                                        */
/* ========================================================================= */

#define VITTE_LABEL_INVALID_DISTANCE SIZE_MAX

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

    if (source == NULL) {
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

static bool
vitte_label_text_equal(
    const char *left,
    const char *right
)
{
    if (left == NULL ||
        left[0] == '\0') {

        return right == NULL ||
               right[0] == '\0';
    }

    if (right == NULL ||
        right[0] == '\0') {
        return false;
    }

    return strcmp(left, right) == 0;
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

    if (span->end_offset <
        span->start_offset) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Span copy                                                                 */
/* ========================================================================= */

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

    if (source == NULL) {
        return;
    }

    /* Copying preserves source identity, byte offsets and line/column metadata. */
    *destination = *source;
}

/* ========================================================================= */
/* Source identity                                                           */
/* ========================================================================= */

static bool
vitte_label_source_id_valid(
    uint64_t source_id
)
{
#ifdef VITTE_SOURCE_ID_INVALID
    return source_id !=
           VITTE_SOURCE_ID_INVALID;
#else
    /*
     * Vitte currently treats zero as an unavailable source identity when no
     * explicit invalid-source constant is provided.
     */
    return source_id != 0u;
#endif
}

static bool
vitte_label_same_source_span(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    bool left_has_id;
    bool right_has_id;

    if (!vitte_label_span_valid(left) ||
        !vitte_label_span_valid(right)) {
        return false;
    }

    left_has_id =
        vitte_label_source_id_valid(
            left->source_id
        );

    right_has_id =
        vitte_label_source_id_valid(
            right->source_id
        );

    if (left_has_id &&
        right_has_id) {

        return left->source_id ==
               right->source_id;
    }

    if (vitte_label_text_present(
            left->source_name
        ) &&
        vitte_label_text_present(
            right->source_name
        )) {

        return strcmp(
            left->source_name,
            right->source_name
        ) == 0;
    }

    return false;
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

static bool
vitte_label_span_is_empty(
    const vitte_ast_span_t *span
)
{
    return vitte_label_span_valid(span) &&
           span->start_offset ==
           span->end_offset;
}

bool
vitte_diagnostic_label_same_span(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
)
{
    if (left == NULL ||
        right == NULL) {
        return false;
    }

    if (!vitte_label_same_source(
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
        vitte_label_span_is_empty(
            &left->span
        );

    right_empty =
        vitte_label_span_is_empty(
            &right->span
        );

    /*
     * Normal source ranges are half-open:
     *
     *     [start, end)
     *
     * Zero-width labels represent insertion points.
     */
    if (left_empty &&
        right_empty) {

        return left->span.start_offset ==
               right->span.start_offset;
    }

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

    if (left->span.end_offset <
        right->span.start_offset) {

        return right->span.start_offset -
               left->span.end_offset;
    }

    if (right->span.end_offset <
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
    if (label == NULL ||
        !vitte_diagnostic_label_style_is_valid(
            style
        ) ||
        !vitte_label_span_valid(
            span
        )) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_label_init(
        label
    );

    label->style = style;

    vitte_label_copy_span(
        &label->span,
        span
    );

    if (vitte_label_text_present(
            message
        )) {

        vitte_label_copy_text(
            label->message_storage,
            sizeof(label->message_storage),
            message
        );

        label->message =
            label->message_storage;
    } else {
        label->message = NULL;
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
/* Label priority                                                            */
/* ========================================================================= */

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
/* Ordering                                                                  */
/* ========================================================================= */

int
vitte_diagnostic_label_compare(
    const vitte_diagnostic_label_t *left,
    const vitte_diagnostic_label_t *right
)
{
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

    /*
     * Source ID first when both IDs are available.
     */
    if (vitte_label_source_id_valid(
            left->span.source_id
        ) &&
        vitte_label_source_id_valid(
            right->span.source_id
        ) &&
        left->span.source_id !=
            right->span.source_id) {

        return left->span.source_id <
                   right->span.source_id
            ? -1
            : 1;
    }

    /*
     * Otherwise use stable source names.
     */
    if (vitte_label_text_present(
            left->span.source_name
        ) &&
        vitte_label_text_present(
            right->span.source_name
        )) {

        int comparison;

        comparison =
            strcmp(
                left->span.source_name,
                right->span.source_name
            );

        if (comparison != 0) {
            return comparison;
        }
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

    if (left->message == NULL &&
        right->message != NULL) {
        return -1;
    }

    if (left->message != NULL &&
        right->message == NULL) {
        return 1;
    }

    if (left->message != NULL &&
        right->message != NULL) {

        return strcmp(
            left->message,
            right->message
        );
    }

    return 0;
}

/* ========================================================================= */
/* qsort adapter                                                             */
/* ========================================================================= */

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
/* Rebind owned storage                                                      */
/* ========================================================================= */

/*
 * Labels contain pointers into their own storage.
 *
 * qsort()/assignment copies structures byte-for-byte, so after moving a label
 * the pointer may still refer to the old structure's storage.
 *
 * Rebinding after structural copies is therefore mandatory.
 */
static void
vitte_label_rebind_storage(
    vitte_diagnostic_label_t *label
)
{
    if (label == NULL) {
        return;
    }

    if (label->message_storage[0] != '\0') {
        label->message =
            label->message_storage;
    } else {
        label->message = NULL;
    }
}

static void
vitte_label_rebind_diagnostic(
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

        vitte_label_rebind_storage(
            &diagnostic->labels[index]
        );
    }
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

/* ========================================================================= */
/* Semantic duplicate                                                        */
/* ========================================================================= */

/*
 * Same source/span/message but possibly different style.
 *
 * If a secondary label already exists and a primary equivalent is added, the
 * existing label is upgraded to primary rather than duplicated.
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
    size_t duplicate;

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

    if (vitte_diagnostic_label_set(
            &candidate,
            style,
            span,
            message
        ) != VITTE_STATUS_OK) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
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

    diagnostic->labels[
        diagnostic->label_count
    ] = candidate;

    /*
     * candidate contained self-referential storage pointers.
     * Rebind them after copying candidate into the diagnostic.
     */
    vitte_label_rebind_storage(
        &diagnostic->labels[
            diagnostic->label_count
        ]
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
    size_t cursor;

    if (diagnostic == NULL ||
        index >= diagnostic->label_count) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (cursor = index;
         cursor + 1u <
             diagnostic->label_count;
         cursor++) {

        diagnostic->labels[cursor] =
            diagnostic->labels[cursor + 1u];

        vitte_label_rebind_storage(
            &diagnostic->labels[cursor]
        );
    }

    diagnostic->label_count--;

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

    /*
     * qsort moved self-contained structures, so restore pointers to owned
     * buffers.
     */
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
    size_t read_index;
    size_t write_index;

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    /*
     * Drop invalid labels first.
     */
    write_index = 0u;

    for (read_index = 0u;
         read_index < diagnostic->label_count;
         read_index++) {

        if (!vitte_diagnostic_label_is_valid(
                &diagnostic->labels[read_index]
            )) {
            continue;
        }

        if (write_index != read_index) {
            diagnostic->labels[write_index] =
                diagnostic->labels[read_index];

            vitte_label_rebind_storage(
                &diagnostic->labels[write_index]
            );
        }

        write_index++;
    }

    while (write_index <
           diagnostic->label_count) {

        vitte_diagnostic_label_init(
            &diagnostic->labels[write_index]
        );

        write_index++;
    }

    /*
     * Recompute the valid count.
     */
    write_index = 0u;

    for (read_index = 0u;
         read_index < diagnostic->label_count;
         read_index++) {

        if (vitte_diagnostic_label_is_valid(
                &diagnostic->labels[read_index]
            )) {
            write_index++;
        }
    }

    diagnostic->label_count =
        write_index;

    /*
     * Sort for deterministic output.
     */
    vitte_diagnostic_label_sort(
        diagnostic
    );

    /*
     * Remove exact duplicates after sorting.
     */
    if (diagnostic->label_count > 1u) {
        write_index = 1u;

        for (read_index = 1u;
             read_index < diagnostic->label_count;
             read_index++) {

            vitte_diagnostic_label_t *previous;
            vitte_diagnostic_label_t *current;

            previous =
                &diagnostic->labels[
                    write_index - 1u
                ];

            current =
                &diagnostic->labels[
                    read_index
                ];

            if (vitte_diagnostic_label_equal(
                    previous,
                    current
                )) {
                continue;
            }

            if (write_index != read_index) {
                diagnostic->labels[write_index] =
                    *current;

                vitte_label_rebind_storage(
                    &diagnostic->labels[
                        write_index
                    ]
                );
            }

            write_index++;
        }

        for (read_index = write_index;
             read_index < diagnostic->label_count;
             read_index++) {

            vitte_diagnostic_label_init(
                &diagnostic->labels[read_index]
            );
        }

        diagnostic->label_count =
            write_index;
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Promote                                                                   */
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

/* ========================================================================= */
/* Demote                                                                    */
/* ========================================================================= */

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

    /*
     * No explicit primary label: promote the first deterministic label.
     */
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
    bool found_primary;

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    found_primary = false;

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        vitte_diagnostic_label_t *label;

        label =
            &diagnostic->labels[index];

        if (label->style !=
            VITTE_DIAGNOSTIC_LABEL_PRIMARY) {
            continue;
        }

        if (!found_primary) {
            found_primary = true;
            continue;
        }

        label->style =
            VITTE_DIAGNOSTIC_LABEL_SECONDARY;
    }

    if (!found_primary &&
        diagnostic->label_count != 0u) {

        return vitte_diagnostic_label_ensure_primary(
            diagnostic
        );
    }

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
    size_t index;
    const vitte_diagnostic_label_t *best;
    size_t best_length;

    vitte_diagnostic_label_t probe;

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
    best_length = SIZE_MAX;

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
            candidate->span.end_offset -
            candidate->span.start_offset;

        if (best == NULL ||
            length < best_length ||
            (
                length == best_length &&
                candidate->style ==
                    VITTE_DIAGNOSTIC_LABEL_PRIMARY &&
                best->style ==
                    VITTE_DIAGNOSTIC_LABEL_SECONDARY
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
    size_t index;
    size_t best_distance;

    const vitte_diagnostic_label_t *best;
    vitte_diagnostic_label_t probe;

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
    size_t previous;
    size_t count;

    if (diagnostic == NULL) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

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

/* ========================================================================= */
/* Multi-source                                                              */
/* ========================================================================= */

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
/* Label fingerprint                                                         */
/* ========================================================================= */

#define VITTE_LABEL_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_LABEL_FNV_PRIME \
    UINT64_C(1099511628211)

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
vitte_label_hash_text(
    uint64_t hash,
    const char *text
)
{
    if (text == NULL) {
        return vitte_label_hash_bytes(
            hash,
            NULL,
            0u
        );
    }

    return vitte_label_hash_bytes(
        hash,
        text,
        strlen(text) + 1u
    );
}

uint64_t
vitte_diagnostic_label_fingerprint(
    const vitte_diagnostic_label_t *label
)
{
    uint64_t hash;
    uint32_t style_value;

    if (!vitte_diagnostic_label_is_valid(
            label
        )) {
        return UINT64_C(0);
    }

    hash =
        VITTE_LABEL_FNV_OFFSET;

    style_value =
        (uint32_t)label->style;

    hash =
        vitte_label_hash_bytes(
            hash,
            &style_value,
            sizeof(style_value)
        );

    if (vitte_label_source_id_valid(
            label->span.source_id
        )) {

        hash =
            vitte_label_hash_bytes(
                hash,
                &label->span.source_id,
                sizeof(label->span.source_id)
            );
    } else {
        hash =
            vitte_label_hash_text(
                hash,
                label->span.source_name
            );
    }

    hash =
        vitte_label_hash_bytes(
            hash,
            &label->span.start_offset,
            sizeof(label->span.start_offset)
        );

    hash =
        vitte_label_hash_bytes(
            hash,
            &label->span.end_offset,
            sizeof(label->span.end_offset)
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

    for (index = 0u;
         index < diagnostic->label_count;
         index++) {

        uint64_t label_hash;

        label_hash =
            vitte_diagnostic_label_fingerprint(
                &diagnostic->labels[index]
            );

        hash =
            vitte_label_hash_bytes(
                hash,
                &label_hash,
                sizeof(label_hash)
            );
    }

    return hash;
}

#undef VITTE_LABEL_FNV_PRIME
#undef VITTE_LABEL_FNV_OFFSET

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
            "%s %s:%zu-%zu",
            vitte_diagnostic_label_style_name(
                label->style
            ),
            source_name,
            label->span.start_offset,
            label->span.end_offset
        ) < 0) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (vitte_label_text_present(
            label->message
        )) {

        if (fprintf(
                stream,
                ": %s",
                label->message
            ) < 0) {

            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }

    if (fputc('\n', stream) == EOF) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return VITTE_STATUS_OK;
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

        if (status !=
            VITTE_STATUS_OK) {

            return status;
        }
    }

    return VITTE_STATUS_OK;
}
