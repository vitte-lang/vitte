#include "suggestion.h"

#include "diagnostic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Internal configuration                                                    */
/* ========================================================================= */

#ifndef VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE
#define VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE SIZE_MAX
#endif

#ifndef VITTE_DIAGNOSTIC_SUGGESTION_MAX_EDITS
#define VITTE_DIAGNOSTIC_SUGGESTION_MAX_EDITS VITTE_DIAGNOSTIC_MAX_SUGGESTIONS
#endif

/* ========================================================================= */
/* Internal helpers                                                          */
/* ========================================================================= */

static bool
vitte_suggestion_text_present(
    const char *text
)
{
    return text != NULL &&
           text[0] != '\0';
}

static size_t
vitte_suggestion_bounded_length(
    const char *text,
    size_t capacity
)
{
    size_t length;

    if (text == NULL ||
        capacity == 0u) {

        return 0u;
    }

    length = 0u;

    while (length + 1u < capacity &&
           text[length] != '\0') {

        length++;
    }

    return length;
}

static void
vitte_suggestion_copy_text(
    char *destination,
    size_t capacity,
    const char *source
)
{
    size_t length;

    if (destination == NULL ||
        capacity == 0u) {

        return;
    }

    destination[0] = '\0';

    if (source == NULL) {
        return;
    }

    length =
        vitte_suggestion_bounded_length(
            source,
            capacity
        );

    if (length != 0u) {
        memcpy(
            destination,
            source,
            length
        );
    }

    destination[length] = '\0';
}

static bool
vitte_suggestion_source_id_valid(
    vitte_source_id_t source_id
)
{
#ifdef VITTE_SOURCE_ID_INVALID
    return source_id != VITTE_SOURCE_ID_INVALID;
#else
    /*
     * Replace this fallback with the canonical source-id validity helper once
     * source.h exposes one.
     */
    return source_id != (vitte_source_id_t)0;
#endif
}

static bool
vitte_suggestion_span_valid(
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
vitte_suggestion_same_source(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    if (left == NULL ||
        right == NULL) {

        return false;
    }

    if (vitte_suggestion_source_id_valid(
            left->source_id
        ) &&
        vitte_suggestion_source_id_valid(
            right->source_id
        )) {

        return left->source_id ==
               right->source_id;
    }

    if (vitte_suggestion_text_present(
            left->source_name
        ) &&
        vitte_suggestion_text_present(
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
vitte_suggestion_span_equal(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    if (!vitte_suggestion_span_valid(left) ||
        !vitte_suggestion_span_valid(right) ||
        !vitte_suggestion_same_source(
            left,
            right
        )) {

        return false;
    }

    return left->start_offset ==
               right->start_offset &&
           left->end_offset ==
               right->end_offset;
}

static bool
vitte_suggestion_span_overlaps(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    if (!vitte_suggestion_span_valid(left) ||
        !vitte_suggestion_span_valid(right) ||
        !vitte_suggestion_same_source(
            left,
            right
        )) {

        return false;
    }

    /*
     * Two zero-width insertions overlap only when they target exactly the same
     * insertion point.
     */
    if (left->start_offset ==
            left->end_offset &&
        right->start_offset ==
            right->end_offset) {

        return left->start_offset ==
               right->start_offset;
    }

    /*
     * A zero-width insertion inside a replacement range is considered a
     * conflict because applying both edits without an explicit ordering policy
     * is ambiguous.
     */
    if (left->start_offset ==
        left->end_offset) {

        return left->start_offset >=
                   right->start_offset &&
               left->start_offset <=
                   right->end_offset;
    }

    if (right->start_offset ==
        right->end_offset) {

        return right->start_offset >=
                   left->start_offset &&
               right->start_offset <=
                   left->end_offset;
    }

    return left->start_offset <
               right->end_offset &&
           right->start_offset <
               left->end_offset;
}

static size_t
vitte_suggestion_span_length(
    const vitte_ast_span_t *span
)
{
    if (!vitte_suggestion_span_valid(span)) {
        return SIZE_MAX;
    }

    return span->end_offset -
           span->start_offset;
}

/* ========================================================================= */
/* Applicability                                                             */
/* ========================================================================= */

const char *
vitte_diagnostic_applicability_name(
    vitte_diagnostic_applicability_t applicability
)
{
    switch (applicability) {
        case VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE:
            return "machine-applicable";

        case VITTE_DIAGNOSTIC_APPLICABILITY_MAYBE:
            return "maybe-incorrect";

        case VITTE_DIAGNOSTIC_APPLICABILITY_PLACEHOLDERS:
            return "has-placeholders";

        case VITTE_DIAGNOSTIC_APPLICABILITY_MANUAL:
            return "manual";

        default:
            return "unknown";
    }
}

bool
vitte_diagnostic_applicability_machine_applicable(
    vitte_diagnostic_applicability_t applicability
)
{
    return applicability ==
           VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE;
}

bool
vitte_diagnostic_applicability_requires_review(
    vitte_diagnostic_applicability_t applicability
)
{
    return applicability !=
           VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE;
}

/* ========================================================================= */
/* Suggestion initialization                                                 */
/* ========================================================================= */

void
vitte_diagnostic_suggestion_init(
    vitte_diagnostic_suggestion_t *suggestion
)
{
    if (suggestion == NULL) {
        return;
    }

    memset(
        suggestion,
        0,
        sizeof(*suggestion)
    );

    suggestion->applicability =
        VITTE_DIAGNOSTIC_APPLICABILITY_MANUAL;
}

void
vitte_diagnostic_suggestion_reset(
    vitte_diagnostic_suggestion_t *suggestion
)
{
    vitte_diagnostic_suggestion_init(
        suggestion
    );
}

/* ========================================================================= */
/* Suggestion construction                                                   */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_suggestion_set(
    vitte_diagnostic_suggestion_t *suggestion,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability
)
{
    if (suggestion == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_suggestion_init(
        suggestion
    );

    if (vitte_suggestion_text_present(
            message
        )) {

        vitte_suggestion_copy_text(
            suggestion->message_storage,
            sizeof(suggestion->message_storage),
            message
        );

        suggestion->message =
            suggestion->message_storage;
    }

    if (span != NULL) {
        if (!vitte_suggestion_span_valid(
                span
            )) {

            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }

        suggestion->span =
            *span;

        suggestion->has_span =
            true;
    }

    if (replacement != NULL) {
        vitte_suggestion_copy_text(
            suggestion->replacement_storage,
            sizeof(suggestion->replacement_storage),
            replacement
        );

        suggestion->replacement =
            suggestion->replacement_storage;

        suggestion->has_replacement =
            true;
    }

    suggestion->applicability =
        applicability;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Suggestion validation                                                     */
/* ========================================================================= */

bool
vitte_diagnostic_suggestion_is_valid(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    if (suggestion == NULL) {
        return false;
    }

    /*
     * A useful suggestion needs at least explanatory text or an edit.
     */
    if (!vitte_suggestion_text_present(
            suggestion->message
        ) &&
        !suggestion->has_replacement) {

        return false;
    }

    if (suggestion->has_span &&
        !vitte_suggestion_span_valid(
            &suggestion->span
        )) {

        return false;
    }

    /*
     * An automatic replacement without a span has no edit target.
     */
    if (suggestion->has_replacement &&
        !suggestion->has_span) {

        return false;
    }

    return true;
}

bool
vitte_diagnostic_suggestion_has_edit(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    return suggestion != NULL &&
           suggestion->has_span &&
           suggestion->has_replacement;
}

bool
vitte_diagnostic_suggestion_is_insertion(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    return
        vitte_diagnostic_suggestion_has_edit(
            suggestion
        ) &&
        suggestion->span.start_offset ==
            suggestion->span.end_offset &&
        vitte_suggestion_text_present(
            suggestion->replacement
        );
}

bool
vitte_diagnostic_suggestion_is_deletion(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    return
        vitte_diagnostic_suggestion_has_edit(
            suggestion
        ) &&
        suggestion->span.end_offset >
            suggestion->span.start_offset &&
        !vitte_suggestion_text_present(
            suggestion->replacement
        );
}

bool
vitte_diagnostic_suggestion_is_replacement(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    return
        vitte_diagnostic_suggestion_has_edit(
            suggestion
        ) &&
        suggestion->span.end_offset >
            suggestion->span.start_offset &&
        vitte_suggestion_text_present(
            suggestion->replacement
        );
}

/* ========================================================================= */
/* Equality                                                                  */
/* ========================================================================= */

bool
vitte_diagnostic_suggestion_equal(
    const vitte_diagnostic_suggestion_t *left,
    const vitte_diagnostic_suggestion_t *right
)
{
    if (left == NULL ||
        right == NULL) {

        return false;
    }

    if (left->applicability !=
        right->applicability) {

        return false;
    }

    if (left->has_span !=
        right->has_span) {

        return false;
    }

    if (left->has_replacement !=
        right->has_replacement) {

        return false;
    }

    if (left->has_span &&
        !vitte_suggestion_span_equal(
            &left->span,
            &right->span
        )) {

        return false;
    }

    if (strcmp(
            left->message != NULL
                ? left->message
                : "",
            right->message != NULL
                ? right->message
                : ""
        ) != 0) {

        return false;
    }

    if (strcmp(
            left->replacement != NULL
                ? left->replacement
                : "",
            right->replacement != NULL
                ? right->replacement
                : ""
        ) != 0) {

        return false;
    }

    return true;
}

/* ========================================================================= */
/* Conflicts                                                                 */
/* ========================================================================= */

bool
vitte_diagnostic_suggestion_conflicts(
    const vitte_diagnostic_suggestion_t *left,
    const vitte_diagnostic_suggestion_t *right
)
{
    if (!vitte_diagnostic_suggestion_has_edit(
            left
        ) ||
        !vitte_diagnostic_suggestion_has_edit(
            right
        )) {

        return false;
    }

    return vitte_suggestion_span_overlaps(
        &left->span,
        &right->span
    );
}

/* ========================================================================= */
/* Diagnostic suggestion lookup                                              */
/* ========================================================================= */

size_t
vitte_diagnostic_suggestion_find(
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    size_t index;

    if (diagnostic == NULL ||
        suggestion == NULL) {

        return VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE;
    }

    for (index = 0u;
         index < diagnostic->suggestion_count;
         index++) {

        if (vitte_diagnostic_suggestion_equal(
                &diagnostic->suggestions[index],
                suggestion
            )) {

            return index;
        }
    }

    return VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE;
}

/* ========================================================================= */
/* Add suggestion                                                           */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_suggestion_object(
    vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    vitte_diagnostic_suggestion_t *destination;

    if (diagnostic == NULL ||
        suggestion == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (!vitte_diagnostic_suggestion_is_valid(
            suggestion
        )) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (vitte_diagnostic_suggestion_find(
            diagnostic,
            suggestion
        ) !=
        VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE) {

        return VITTE_STATUS_OK;
    }

    if (diagnostic->suggestion_count >=
        VITTE_DIAGNOSTIC_MAX_SUGGESTIONS) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    destination =
        &diagnostic->suggestions[
            diagnostic->suggestion_count
        ];

    vitte_diagnostic_suggestion_init(
        destination
    );

    if (vitte_suggestion_text_present(
            suggestion->message
        )) {

        vitte_suggestion_copy_text(
            destination->message_storage,
            sizeof(destination->message_storage),
            suggestion->message
        );

        destination->message =
            destination->message_storage;
    }

    if (suggestion->has_span) {
        destination->span =
            suggestion->span;

        destination->has_span =
            true;
    }

    if (suggestion->has_replacement) {
        vitte_suggestion_copy_text(
            destination->replacement_storage,
            sizeof(destination->replacement_storage),
            suggestion->replacement
        );

        destination->replacement =
            destination->replacement_storage;

        destination->has_replacement =
            true;
    }

    destination->applicability =
        suggestion->applicability;

    diagnostic->suggestion_count++;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* High-level add                                                            */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_suggest(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability
)
{
    vitte_diagnostic_suggestion_t suggestion;
    vitte_status_t status;

    if (diagnostic == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status =
        vitte_diagnostic_suggestion_set(
            &suggestion,
            message,
            span,
            replacement,
            applicability
        );

    if (status !=
        VITTE_STATUS_OK) {

        return status;
    }

    return vitte_diagnostic_add_suggestion_object(
        diagnostic,
        &suggestion
    );
}

/* ========================================================================= */
/* Specialized suggestions                                                  */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_suggest_insert(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *position,
    const char *text,
    vitte_diagnostic_applicability_t applicability
)
{
    vitte_ast_span_t insertion;

    if (diagnostic == NULL ||
        position == NULL ||
        !vitte_suggestion_span_valid(position) ||
        text == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    insertion =
        *position;

    insertion.end_offset =
        insertion.start_offset;

    return vitte_diagnostic_suggest(
        diagnostic,
        message,
        &insertion,
        text,
        applicability
    );
}

vitte_status_t
vitte_diagnostic_suggest_replace(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability
)
{
    if (diagnostic == NULL ||
        span == NULL ||
        !vitte_suggestion_span_valid(span) ||
        replacement == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    return vitte_diagnostic_suggest(
        diagnostic,
        message,
        span,
        replacement,
        applicability
    );
}

vitte_status_t
vitte_diagnostic_suggest_delete(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    vitte_diagnostic_applicability_t applicability
)
{
    if (diagnostic == NULL ||
        span == NULL ||
        !vitte_suggestion_span_valid(span)) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    return vitte_diagnostic_suggest(
        diagnostic,
        message,
        span,
        "",
        applicability
    );
}

/* ========================================================================= */
/* Machine-applicable helpers                                                */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_suggest_machine_replace(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement
)
{
    return vitte_diagnostic_suggest_replace(
        diagnostic,
        message,
        span,
        replacement,
        VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE
    );
}

vitte_status_t
vitte_diagnostic_suggest_machine_insert(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *position,
    const char *text
)
{
    return vitte_diagnostic_suggest_insert(
        diagnostic,
        message,
        position,
        text,
        VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE
    );
}

vitte_status_t
vitte_diagnostic_suggest_machine_delete(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span
)
{
    return vitte_diagnostic_suggest_delete(
        diagnostic,
        message,
        span,
        VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE
    );
}

/* ========================================================================= */
/* Suggestion access                                                         */
/* ========================================================================= */

const vitte_diagnostic_suggestion_t *
vitte_diagnostic_suggestion_at(
    const vitte_diagnostic_t *diagnostic,
    size_t index
)
{
    if (diagnostic == NULL ||
        index >=
            diagnostic->suggestion_count) {

        return NULL;
    }

    return &diagnostic->suggestions[index];
}

vitte_diagnostic_suggestion_t *
vitte_diagnostic_suggestion_at_mut(
    vitte_diagnostic_t *diagnostic,
    size_t index
)
{
    if (diagnostic == NULL ||
        index >=
            diagnostic->suggestion_count) {

        return NULL;
    }

    return &diagnostic->suggestions[index];
}

size_t
vitte_diagnostic_suggestion_count(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL
        ? diagnostic->suggestion_count
        : 0u;
}

/* ========================================================================= */
/* Remove / clear                                                            */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_suggestion_remove(
    vitte_diagnostic_t *diagnostic,
    size_t index
)
{
    size_t move_count;

    if (diagnostic == NULL ||
        index >=
            diagnostic->suggestion_count) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    move_count =
        diagnostic->suggestion_count -
        index -
        1u;

    if (move_count != 0u) {
        memmove(
            &diagnostic->suggestions[index],
            &diagnostic->suggestions[index + 1u],
            move_count *
            sizeof(diagnostic->suggestions[0])
        );
    }

    diagnostic->suggestion_count--;

    /*
     * Clear the now-unused tail entry.
     */
    vitte_diagnostic_suggestion_init(
        &diagnostic->suggestions[
            diagnostic->suggestion_count
        ]
    );

    /*
     * memmove copied self-referential message/replacement pointers from their
     * previous slots. Rebind every surviving entry to its own storage.
     */
    {
        size_t current;

        for (current = index;
             current <
                 diagnostic->suggestion_count;
             current++) {

            vitte_diagnostic_suggestion_t *item;

            item =
                &diagnostic->suggestions[current];

            if (item->message_storage[0] != '\0') {
                item->message =
                    item->message_storage;
            } else {
                item->message = NULL;
            }

            if (item->has_replacement) {
                item->replacement =
                    item->replacement_storage;
            } else {
                item->replacement = NULL;
            }
        }
    }

    return VITTE_STATUS_OK;
}

void
vitte_diagnostic_suggestion_clear(
    vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return;
    }

    for (index = 0u;
         index <
             diagnostic->suggestion_count;
         index++) {

        vitte_diagnostic_suggestion_init(
            &diagnostic->suggestions[index]
        );
    }

    diagnostic->suggestion_count = 0u;
}

/* ========================================================================= */
/* Sorting                                                                   */
/* ========================================================================= */

static int
vitte_suggestion_compare_source(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
)
{
    if (vitte_suggestion_source_id_valid(
            left->source_id
        ) &&
        vitte_suggestion_source_id_valid(
            right->source_id
        )) {

        if (left->source_id <
            right->source_id) {

            return -1;
        }

        if (left->source_id >
            right->source_id) {

            return 1;
        }
    }

    if (vitte_suggestion_text_present(
            left->source_name
        ) &&
        vitte_suggestion_text_present(
            right->source_name
        )) {

        int result;

        result =
            strcmp(
                left->source_name,
                right->source_name
            );

        if (result != 0) {
            return result;
        }
    } else if (vitte_suggestion_text_present(
                   left->source_name
               )) {

        return 1;
    } else if (vitte_suggestion_text_present(
                   right->source_name
               )) {

        return -1;
    }

    return 0;
}

static int
vitte_suggestion_compare(
    const vitte_diagnostic_suggestion_t *left,
    const vitte_diagnostic_suggestion_t *right
)
{
    int result;

    if (left == NULL &&
        right == NULL) {

        return 0;
    }

    if (left == NULL) {
        return -1;
    }

    if (right == NULL) {
        return 1;
    }

    if (left->has_span !=
        right->has_span) {

        return left->has_span
            ? -1
            : 1;
    }

    if (left->has_span) {
        result =
            vitte_suggestion_compare_source(
                &left->span,
                &right->span
            );

        if (result != 0) {
            return result;
        }

        if (left->span.start_offset <
            right->span.start_offset) {

            return -1;
        }

        if (left->span.start_offset >
            right->span.start_offset) {

            return 1;
        }

        /*
         * Larger end first for identical starts.
         *
         * This deterministic order is useful when later applying edits in
         * reverse source order.
         */
        if (left->span.end_offset >
            right->span.end_offset) {

            return -1;
        }

        if (left->span.end_offset <
            right->span.end_offset) {

            return 1;
        }
    }

    if (left->applicability <
        right->applicability) {

        return -1;
    }

    if (left->applicability >
        right->applicability) {

        return 1;
    }

    result =
        strcmp(
            left->message != NULL
                ? left->message
                : "",
            right->message != NULL
                ? right->message
                : ""
        );

    if (result != 0) {
        return result;
    }

    return strcmp(
        left->replacement != NULL
            ? left->replacement
            : "",
        right->replacement != NULL
            ? right->replacement
            : ""
    );
}

static int
vitte_suggestion_qsort_compare(
    const void *left,
    const void *right
)
{
    return vitte_suggestion_compare(
        (const vitte_diagnostic_suggestion_t *)left,
        (const vitte_diagnostic_suggestion_t *)right
    );
}

static void
vitte_suggestion_rebind_storage(
    vitte_diagnostic_suggestion_t *suggestion
)
{
    if (suggestion == NULL) {
        return;
    }

    if (suggestion->message_storage[0] != '\0') {
        suggestion->message =
            suggestion->message_storage;
    } else {
        suggestion->message = NULL;
    }

    if (suggestion->has_replacement) {
        suggestion->replacement =
            suggestion->replacement_storage;
    } else {
        suggestion->replacement = NULL;
    }
}

void
vitte_diagnostic_suggestions_sort(
    vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL ||
        diagnostic->suggestion_count <= 1u) {

        return;
    }

    qsort(
        diagnostic->suggestions,
        diagnostic->suggestion_count,
        sizeof(diagnostic->suggestions[0]),
        vitte_suggestion_qsort_compare
    );

    /*
     * Suggestion objects contain pointers into their own storage. qsort moves
     * structures byte-for-byte, therefore those pointers must be rebound.
     */
    for (index = 0u;
         index <
             diagnostic->suggestion_count;
         index++) {

        vitte_suggestion_rebind_storage(
            &diagnostic->suggestions[index]
        );
    }
}

/* ========================================================================= */
/* Deduplication                                                             */
/* ========================================================================= */

size_t
vitte_diagnostic_suggestions_deduplicate(
    vitte_diagnostic_t *diagnostic
)
{
    size_t removed;
    size_t index;

    if (diagnostic == NULL ||
        diagnostic->suggestion_count <= 1u) {

        return 0u;
    }

    removed = 0u;
    index = 0u;

    while (index <
           diagnostic->suggestion_count) {

        size_t other;

        other = index + 1u;

        while (other <
               diagnostic->suggestion_count) {

            if (vitte_diagnostic_suggestion_equal(
                    &diagnostic->suggestions[index],
                    &diagnostic->suggestions[other]
                )) {

                if (vitte_diagnostic_suggestion_remove(
                        diagnostic,
                        other
                    ) ==
                    VITTE_STATUS_OK) {

                    removed++;
                    continue;
                }
            }

            other++;
        }

        index++;
    }

    return removed;
}

/* ========================================================================= */
/* Conflict detection                                                        */
/* ========================================================================= */

bool
vitte_diagnostic_suggestions_have_conflicts(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t left;

    if (diagnostic == NULL) {
        return false;
    }

    for (left = 0u;
         left <
             diagnostic->suggestion_count;
         left++) {

        size_t right;

        for (right = left + 1u;
             right <
                 diagnostic->suggestion_count;
             right++) {

            if (vitte_diagnostic_suggestion_conflicts(
                    &diagnostic->suggestions[left],
                    &diagnostic->suggestions[right]
                )) {

                return true;
            }
        }
    }

    return false;
}

size_t
vitte_diagnostic_suggestion_conflict_count(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t count;
    size_t left;

    if (diagnostic == NULL) {
        return 0u;
    }

    count = 0u;

    for (left = 0u;
         left <
             diagnostic->suggestion_count;
         left++) {

        size_t right;

        for (right = left + 1u;
             right <
                 diagnostic->suggestion_count;
             right++) {

            if (vitte_diagnostic_suggestion_conflicts(
                    &diagnostic->suggestions[left],
                    &diagnostic->suggestions[right]
                )) {

                count++;
            }
        }
    }

    return count;
}

/* ========================================================================= */
/* Applicability statistics                                                  */
/* ========================================================================= */

size_t
vitte_diagnostic_machine_suggestion_count(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t count;
    size_t index;

    if (diagnostic == NULL) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index <
             diagnostic->suggestion_count;
         index++) {

        const vitte_diagnostic_suggestion_t *suggestion;

        suggestion =
            &diagnostic->suggestions[index];

        if (vitte_diagnostic_suggestion_has_edit(
                suggestion
            ) &&
            suggestion->applicability ==
                VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE) {

            count++;
        }
    }

    return count;
}

bool
vitte_diagnostic_has_machine_suggestions(
    const vitte_diagnostic_t *diagnostic
)
{
    return
        vitte_diagnostic_machine_suggestion_count(
            diagnostic
        ) != 0u;
}

/* ========================================================================= */
/* Automatic-fix safety                                                      */
/* ========================================================================= */

bool
vitte_diagnostic_suggestions_machine_safe(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;
    bool found;

    if (diagnostic == NULL) {
        return false;
    }

    found = false;

    for (index = 0u;
         index <
             diagnostic->suggestion_count;
         index++) {

        const vitte_diagnostic_suggestion_t *suggestion;

        suggestion =
            &diagnostic->suggestions[index];

        if (!vitte_diagnostic_suggestion_has_edit(
                suggestion
            )) {

            continue;
        }

        found = true;

        if (suggestion->applicability !=
            VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE) {

            return false;
        }
    }

    if (!found) {
        return false;
    }

    if (vitte_diagnostic_suggestions_have_conflicts(
            diagnostic
        )) {

        return false;
    }

    return true;
}

/* ========================================================================= */
/* Fix-plan model                                                            */
/* ========================================================================= */

void
vitte_diagnostic_fix_plan_init(
    vitte_diagnostic_fix_plan_t *plan
)
{
    if (plan == NULL) {
        return;
    }

    memset(
        plan,
        0,
        sizeof(*plan)
    );

    plan->machine_applicable = true;
    plan->conflict_free = true;
}

static bool
vitte_suggestion_fix_plan_contains(
    const vitte_diagnostic_fix_plan_t *plan,
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    size_t index;

    if (plan == NULL ||
        suggestion == NULL) {

        return false;
    }

    for (index = 0u;
         index < plan->count;
         index++) {

        if (vitte_diagnostic_suggestion_equal(
                &plan->edits[index],
                suggestion
            )) {

            return true;
        }
    }

    return false;
}

vitte_status_t
vitte_diagnostic_fix_plan_add(
    vitte_diagnostic_fix_plan_t *plan,
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    vitte_diagnostic_suggestion_t *destination;
    size_t index;

    if (plan == NULL ||
        suggestion == NULL ||
        !vitte_diagnostic_suggestion_has_edit(
            suggestion
        )) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (vitte_suggestion_fix_plan_contains(
            plan,
            suggestion
        )) {

        return VITTE_STATUS_OK;
    }

    if (plan->count >=
        VITTE_DIAGNOSTIC_SUGGESTION_MAX_EDITS) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    for (index = 0u;
         index < plan->count;
         index++) {

        if (vitte_diagnostic_suggestion_conflicts(
                &plan->edits[index],
                suggestion
            )) {

            plan->conflict_free = false;
        }
    }

    destination =
        &plan->edits[plan->count];

    vitte_diagnostic_suggestion_init(
        destination
    );

    if (vitte_suggestion_text_present(
            suggestion->message
        )) {

        vitte_suggestion_copy_text(
            destination->message_storage,
            sizeof(destination->message_storage),
            suggestion->message
        );

        destination->message =
            destination->message_storage;
    }

    destination->span =
        suggestion->span;

    destination->has_span =
        suggestion->has_span;

    if (suggestion->has_replacement) {
        vitte_suggestion_copy_text(
            destination->replacement_storage,
            sizeof(destination->replacement_storage),
            suggestion->replacement
        );

        destination->replacement =
            destination->replacement_storage;

        destination->has_replacement =
            true;
    }

    destination->applicability =
        suggestion->applicability;

    if (suggestion->applicability !=
        VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE) {

        plan->machine_applicable = false;
    }

    plan->count++;

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_build_fix_plan(
    const vitte_diagnostic_t *diagnostic,
    bool machine_only,
    vitte_diagnostic_fix_plan_t *plan
)
{
    size_t index;
    vitte_status_t status;

    if (diagnostic == NULL ||
        plan == NULL) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_diagnostic_fix_plan_init(
        plan
    );

    for (index = 0u;
         index <
             diagnostic->suggestion_count;
         index++) {

        const vitte_diagnostic_suggestion_t *suggestion;

        suggestion =
            &diagnostic->suggestions[index];

        if (!vitte_diagnostic_suggestion_has_edit(
                suggestion
            )) {

            continue;
        }

        if (machine_only &&
            suggestion->applicability !=
                VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE) {

            continue;
        }

        status =
            vitte_diagnostic_fix_plan_add(
                plan,
                suggestion
            );

        if (status !=
            VITTE_STATUS_OK) {

            return status;
        }
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Fix-plan ordering                                                         */
/* ========================================================================= */

static int
vitte_fix_plan_apply_compare(
    const vitte_diagnostic_suggestion_t *left,
    const vitte_diagnostic_suggestion_t *right
)
{
    int source_result;

    source_result =
        vitte_suggestion_compare_source(
            &left->span,
            &right->span
        );

    if (source_result != 0) {
        return source_result;
    }

    /*
     * Reverse byte order within one source.
     *
     * Applying edits from the end of a source towards its beginning prevents
     * earlier edits from invalidating offsets of later edits.
     */
    if (left->span.start_offset >
        right->span.start_offset) {

        return -1;
    }

    if (left->span.start_offset <
        right->span.start_offset) {

        return 1;
    }

    if (left->span.end_offset >
        right->span.end_offset) {

        return -1;
    }

    if (left->span.end_offset <
        right->span.end_offset) {

        return 1;
    }

    return 0;
}

static int
vitte_fix_plan_qsort_compare(
    const void *left,
    const void *right
)
{
    return vitte_fix_plan_apply_compare(
        (const vitte_diagnostic_suggestion_t *)left,
        (const vitte_diagnostic_suggestion_t *)right
    );
}

void
vitte_diagnostic_fix_plan_sort_for_application(
    vitte_diagnostic_fix_plan_t *plan
)
{
    size_t index;

    if (plan == NULL ||
        plan->count <= 1u) {

        return;
    }

    qsort(
        plan->edits,
        plan->count,
        sizeof(plan->edits[0]),
        vitte_fix_plan_qsort_compare
    );

    for (index = 0u;
         index < plan->count;
         index++) {

        vitte_suggestion_rebind_storage(
            &plan->edits[index]
        );
    }
}

/* ========================================================================= */
/* Fix-plan validation                                                       */
/* ========================================================================= */

vitte_diagnostic_fix_plan_validation_t
vitte_diagnostic_fix_plan_validate(
    const vitte_diagnostic_fix_plan_t *plan
)
{
    vitte_diagnostic_fix_plan_validation_t result;
    size_t index;

    memset(
        &result,
        0,
        sizeof(result)
    );

    result.valid = true;
    result.machine_applicable = true;
    result.conflict_free = true;
    result.first_invalid_index =
        VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE;

    if (plan == NULL) {
        result.valid = false;
        return result;
    }

    result.edit_count =
        plan->count;

    for (index = 0u;
         index < plan->count;
         index++) {

        size_t other;

        if (!vitte_diagnostic_suggestion_has_edit(
                &plan->edits[index]
            ) ||
            !vitte_diagnostic_suggestion_is_valid(
                &plan->edits[index]
            )) {

            result.valid = false;
            result.invalid_count++;

            if (result.first_invalid_index ==
                VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE) {

                result.first_invalid_index =
                    index;
            }
        }

        if (plan->edits[index].applicability !=
            VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE) {

            result.machine_applicable = false;
        }

        for (other = index + 1u;
             other < plan->count;
             other++) {

            if (vitte_diagnostic_suggestion_conflicts(
                    &plan->edits[index],
                    &plan->edits[other]
                )) {

                result.conflict_free = false;
                result.conflict_count++;
            }
        }
    }

    if (!result.conflict_free) {
        result.valid = false;
    }

    return result;
}

/* ========================================================================= */
/* Replacement length                                                        */
/* ========================================================================= */

size_t
vitte_diagnostic_suggestion_removed_length(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    if (!vitte_diagnostic_suggestion_has_edit(
            suggestion
        )) {

        return 0u;
    }

    return vitte_suggestion_span_length(
        &suggestion->span
    );
}

size_t
vitte_diagnostic_suggestion_inserted_length(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    if (!vitte_diagnostic_suggestion_has_edit(
            suggestion
        ) ||
        suggestion->replacement == NULL) {

        return 0u;
    }

    return strlen(
        suggestion->replacement
    );
}

/* ========================================================================= */
/* Fingerprinting                                                            */
/* ========================================================================= */

static uint64_t
vitte_suggestion_hash_byte(
    uint64_t hash,
    uint8_t byte
)
{
    hash ^= (uint64_t)byte;
    hash *= UINT64_C(1099511628211);

    return hash;
}

static uint64_t
vitte_suggestion_hash_size(
    uint64_t hash,
    size_t value
)
{
    size_t index;

    for (index = 0u;
         index < sizeof(value);
         index++) {

        hash =
            vitte_suggestion_hash_byte(
                hash,
                (uint8_t)(
                    (value >>
                     (index * 8u)) &
                    (size_t)0xffu
                )
            );
    }

    return hash;
}

static uint64_t
vitte_suggestion_hash_text(
    uint64_t hash,
    const char *text
)
{
    if (text == NULL) {
        return vitte_suggestion_hash_byte(
            hash,
            0u
        );
    }

    while (*text != '\0') {
        hash =
            vitte_suggestion_hash_byte(
                hash,
                (uint8_t)*text
            );

        text++;
    }

    return vitte_suggestion_hash_byte(
        hash,
        0u
    );
}

uint64_t
vitte_diagnostic_suggestion_fingerprint(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    uint64_t hash;

    if (suggestion == NULL) {
        return UINT64_C(0);
    }

    hash =
        UINT64_C(1469598103934665603);

    hash =
        vitte_suggestion_hash_size(
            hash,
            (size_t)suggestion->applicability
        );

    hash =
        vitte_suggestion_hash_text(
            hash,
            suggestion->message
        );

    hash =
        vitte_suggestion_hash_size(
            hash,
            suggestion->has_span
                ? 1u
                : 0u
        );

    if (suggestion->has_span) {
        hash =
            vitte_suggestion_hash_size(
                hash,
                (size_t)suggestion->span.source_id
            );

        hash =
            vitte_suggestion_hash_text(
                hash,
                suggestion->span.source_name
            );

        hash =
            vitte_suggestion_hash_size(
                hash,
                suggestion->span.start_offset
            );

        hash =
            vitte_suggestion_hash_size(
                hash,
                suggestion->span.end_offset
            );
    }

    hash =
        vitte_suggestion_hash_size(
            hash,
            suggestion->has_replacement
                ? 1u
                : 0u
        );

    if (suggestion->has_replacement) {
        hash =
            vitte_suggestion_hash_text(
                hash,
                suggestion->replacement
            );
    }

    return hash;
}

uint64_t
vitte_diagnostic_suggestions_fingerprint(
    const vitte_diagnostic_t *diagnostic
)
{
    uint64_t hash;
    size_t index;

    if (diagnostic == NULL) {
        return UINT64_C(0);
    }

    hash =
        UINT64_C(1469598103934665603);

    for (index = 0u;
         index <
             diagnostic->suggestion_count;
         index++) {

        uint64_t item_hash;
        size_t byte_index;

        item_hash =
            vitte_diagnostic_suggestion_fingerprint(
                &diagnostic->suggestions[index]
            );

        for (byte_index = 0u;
             byte_index < sizeof(item_hash);
             byte_index++) {

            hash =
                vitte_suggestion_hash_byte(
                    hash,
                    (uint8_t)(
                        (item_hash >>
                         (byte_index * 8u)) &
                        UINT64_C(0xff)
                    )
                );
        }
    }

    return hash;
}

/* ========================================================================= */
/* Validation of diagnostic suggestions                                      */
/* ========================================================================= */

vitte_diagnostic_suggestion_validation_t
vitte_diagnostic_suggestions_validate(
    const vitte_diagnostic_t *diagnostic
)
{
    vitte_diagnostic_suggestion_validation_t result;
    size_t index;

    memset(
        &result,
        0,
        sizeof(result)
    );

    result.valid = true;
    result.conflict_free = true;
    result.first_invalid_index =
        VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE;

    if (diagnostic == NULL) {
        result.valid = false;
        return result;
    }

    result.suggestion_count =
        diagnostic->suggestion_count;

    for (index = 0u;
         index <
             diagnostic->suggestion_count;
         index++) {

        const vitte_diagnostic_suggestion_t *suggestion;
        size_t other;

        suggestion =
            &diagnostic->suggestions[index];

        if (!vitte_diagnostic_suggestion_is_valid(
                suggestion
            )) {

            result.valid = false;
            result.invalid_count++;

            if (result.first_invalid_index ==
                VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE) {

                result.first_invalid_index =
                    index;
            }
        }

        if (suggestion->applicability ==
            VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE) {

            result.machine_applicable_count++;
        }

        if (vitte_diagnostic_suggestion_is_insertion(
                suggestion
            )) {

            result.insertion_count++;
        } else if (
            vitte_diagnostic_suggestion_is_deletion(
                suggestion
            )) {

            result.deletion_count++;
        } else if (
            vitte_diagnostic_suggestion_is_replacement(
                suggestion
            )) {

            result.replacement_count++;
        }

        for (other = index + 1u;
             other <
                 diagnostic->suggestion_count;
             other++) {

            if (vitte_diagnostic_suggestion_equal(
                    suggestion,
                    &diagnostic->suggestions[other]
                )) {

                result.duplicate_count++;
            }

            if (vitte_diagnostic_suggestion_conflicts(
                    suggestion,
                    &diagnostic->suggestions[other]
                )) {

                result.conflict_count++;
                result.conflict_free = false;
            }
        }
    }

    return result;
}

/* ========================================================================= */
/* Best suggestion                                                           */
/* ========================================================================= */

const vitte_diagnostic_suggestion_t *
vitte_diagnostic_best_suggestion(
    const vitte_diagnostic_t *diagnostic
)
{
    const vitte_diagnostic_suggestion_t *best;
    size_t index;

    if (diagnostic == NULL) {
        return NULL;
    }

    best = NULL;

    for (index = 0u;
         index <
             diagnostic->suggestion_count;
         index++) {

        const vitte_diagnostic_suggestion_t *candidate;

        candidate =
            &diagnostic->suggestions[index];

        if (!vitte_diagnostic_suggestion_is_valid(
                candidate
            )) {

            continue;
        }

        if (best == NULL) {
            best = candidate;
            continue;
        }

        /*
         * Prefer edits over explanation-only suggestions.
         */
        if (vitte_diagnostic_suggestion_has_edit(
                candidate
            ) &&
            !vitte_diagnostic_suggestion_has_edit(
                best
            )) {

            best = candidate;
            continue;
        }

        /*
         * Prefer stronger applicability.
         *
         * Enum ordering is:
         *
         *   MACHINE
         *   MAYBE
         *   PLACEHOLDERS
         *   MANUAL
         */
        if (candidate->applicability <
            best->applicability) {

            best = candidate;
            continue;
        }

        if (candidate->applicability >
            best->applicability) {

            continue;
        }

        /*
         * Prefer the more local/specific edit.
         */
        if (candidate->has_span &&
            best->has_span &&
            vitte_suggestion_span_length(
                &candidate->span
            ) <
            vitte_suggestion_span_length(
                &best->span
            )) {

            best = candidate;
        }
    }

    return best;
}

/* ========================================================================= */
/* Compatibility check                                                       */
/* ========================================================================= */

bool
vitte_diagnostic_suggestion_compatible_with(
    const vitte_diagnostic_suggestion_t *candidate,
    const vitte_diagnostic_suggestion_t *other
)
{
    if (candidate == NULL ||
        other == NULL) {

        return false;
    }

    if (vitte_diagnostic_suggestion_equal(
            candidate,
            other
        )) {

        return true;
    }

    return !vitte_diagnostic_suggestion_conflicts(
        candidate,
        other
    );
}

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

const char *
vitte_diagnostic_suggestion_component_name(void)
{
    return "diagnostic-suggestion";
}

size_t
vitte_diagnostic_suggestion_capacity(void)
{
    return VITTE_DIAGNOSTIC_MAX_SUGGESTIONS;
}
