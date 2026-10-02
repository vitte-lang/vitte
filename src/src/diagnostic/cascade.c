#include "cascade.h"
#include "diagnostic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Vitte diagnostic cascade engine.
 *
 * This module classifies diagnostics as either:
 *
 *   - primary: an independent/root compiler diagnostic;
 *   - cascade: a diagnostic caused by an earlier failure;
 *   - suppressed: retained internally but hidden from normal output.
 *
 * Explicit causal information established by compiler phases always has
 * priority over heuristic inference.
 *
 * Heuristics are intentionally conservative. Proximity alone must never hide
 * an independent diagnostic.
 */

#ifndef VITTE_DIAGNOSTIC_INDEX_NONE
#define VITTE_DIAGNOSTIC_INDEX_NONE SIZE_MAX
#endif

#define VITTE_CASCADE_SCORE_MINIMUM          60u
#define VITTE_CASCADE_SCORE_EXPLICIT         1000u
#define VITTE_CASCADE_SCORE_SAME_SPAN        100u
#define VITTE_CASCADE_SCORE_CONTAINS          80u
#define VITTE_CASCADE_SCORE_OVERLAP           65u
#define VITTE_CASCADE_SCORE_SAME_SYMBOL       55u
#define VITTE_CASCADE_SCORE_SAME_CONTEXT      30u
#define VITTE_CASCADE_SCORE_SAME_CODE         10u
#define VITTE_CASCADE_SCORE_SAME_PHASE        20u
#define VITTE_CASCADE_SCORE_ADJACENT_PHASE    15u
#define VITTE_CASCADE_SCORE_LATER_PHASE        8u
#define VITTE_CASCADE_SCORE_NEAR_8             8u
#define VITTE_CASCADE_SCORE_NEAR_32            4u

typedef struct vitte_cascade_candidate {
    size_t parent_index;
    size_t root_index;
    unsigned score;
    bool valid;
} vitte_cascade_candidate_t;

/* ========================================================================= */
/* Basic helpers                                                             */
/* ========================================================================= */

static bool
vitte_cascade_text_equal(
    const char *left,
    const char *right
)
{
    if (left == NULL ||
        right == NULL ||
        left[0] == '\0' ||
        right[0] == '\0') {
        return false;
    }

    return strcmp(left, right) == 0;
}

static bool
vitte_cascade_is_error(
    const vitte_diagnostic_t *diagnostic
)
{
    if (diagnostic == NULL) {
        return false;
    }

    return diagnostic->severity == VITTE_DIAGNOSTIC_ERROR ||
           diagnostic->severity == VITTE_DIAGNOSTIC_FATAL;
}

static bool
vitte_cascade_index_valid(
    const vitte_diagnostic_bag_t *bag,
    size_t index
)
{
    return bag != NULL &&
           index != VITTE_DIAGNOSTIC_INDEX_NONE &&
           index < bag->count;
}

/* ========================================================================= */
/* Phase ordering                                                            */
/* ========================================================================= */

/*
 * Never depend on the numeric order of vitte_diagnostic_origin_t.
 *
 * Explicit ranks allow the enum itself to evolve without silently changing
 * cascade semantics.
 */
static unsigned
vitte_cascade_phase_rank(
    vitte_diagnostic_origin_t origin
)
{
    switch (origin) {
        case VITTE_DIAGNOSTIC_ORIGIN_IO:
            return 10u;

        case VITTE_DIAGNOSTIC_ORIGIN_LEXER:
            return 20u;

        case VITTE_DIAGNOSTIC_ORIGIN_PARSER:
            return 30u;

        case VITTE_DIAGNOSTIC_ORIGIN_IMPORT:
            return 40u;

        case VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION:
            return 50u;

        case VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK:
            return 60u;

        case VITTE_DIAGNOSTIC_ORIGIN_CONTRACT:
            return 63u;

        case VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL:
            return 65u;

        case VITTE_DIAGNOSTIC_ORIGIN_HIR:
            return 70u;

        case VITTE_DIAGNOSTIC_ORIGIN_IR:
            return 80u;

        case VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND:
            return 90u;

        case VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER:
            return 100u;

        case VITTE_DIAGNOSTIC_ORIGIN_LINKER:
            return 110u;

        case VITTE_DIAGNOSTIC_ORIGIN_RUNTIME:
            return 120u;

        case VITTE_DIAGNOSTIC_ORIGIN_DRIVER:
            return 130u;

        case VITTE_DIAGNOSTIC_ORIGIN_INTERNAL:
            return 140u;

        case VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN:
        default:
            return 0u;
    }
}

static bool
vitte_cascade_phase_is_same_or_later(
    vitte_diagnostic_origin_t root,
    vitte_diagnostic_origin_t derived
)
{
    unsigned root_rank;
    unsigned derived_rank;

    root_rank =
        vitte_cascade_phase_rank(root);

    derived_rank =
        vitte_cascade_phase_rank(derived);

    if (root_rank == 0u ||
        derived_rank == 0u) {
        return false;
    }

    return derived_rank >= root_rank;
}

static bool
vitte_cascade_phase_is_adjacent(
    vitte_diagnostic_origin_t root,
    vitte_diagnostic_origin_t derived
)
{
    unsigned root_rank;
    unsigned derived_rank;

    root_rank =
        vitte_cascade_phase_rank(root);

    derived_rank =
        vitte_cascade_phase_rank(derived);

    if (root_rank == 0u ||
        derived_rank == 0u ||
        derived_rank < root_rank) {
        return false;
    }

    return derived_rank - root_rank <= 15u;
}

/* ========================================================================= */
/* Phase dependency                                                          */
/* ========================================================================= */

static bool
vitte_cascade_phase_dependency(
    vitte_diagnostic_origin_t root,
    vitte_diagnostic_origin_t derived
)
{
    if (root == derived &&
        root != VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN) {
        return true;
    }

    switch (root) {
        case VITTE_DIAGNOSTIC_ORIGIN_IO:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_LEXER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_PARSER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IMPORT;

        case VITTE_DIAGNOSTIC_ORIGIN_LEXER:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_PARSER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_PARSER:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_IMPORT:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_LINKER;

        case VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_CONTRACT ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_CONTRACT:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_HIR:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_IR:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_LINKER;

        case VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_LINKER;

        default:
            return false;
    }
}

/* ========================================================================= */
/* Source identity                                                           */
/* ========================================================================= */

static bool
vitte_cascade_same_source(
    const vitte_diagnostic_t *left,
    const vitte_diagnostic_t *right
)
{
    if (left == NULL ||
        right == NULL ||
        !left->has_span ||
        !right->has_span) {
        return false;
    }

    if (left->source_id != VITTE_SOURCE_ID_INVALID &&
        right->source_id != VITTE_SOURCE_ID_INVALID) {
        return left->source_id ==
               right->source_id;
    }

    return vitte_cascade_text_equal(
        left->source_name,
        right->source_name
    );
}

/* ========================================================================= */
/* Span relationships                                                        */
/* ========================================================================= */

static bool
vitte_cascade_same_span(
    const vitte_diagnostic_t *left,
    const vitte_diagnostic_t *right
)
{
    if (!vitte_cascade_same_source(
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
vitte_cascade_contains(
    const vitte_diagnostic_t *outer,
    const vitte_diagnostic_t *inner
)
{
    if (!vitte_cascade_same_source(
            outer,
            inner
        )) {
        return false;
    }

    return outer->start_offset <=
               inner->start_offset &&
           outer->end_offset >=
               inner->end_offset;
}

static bool
vitte_cascade_overlap(
    const vitte_diagnostic_t *left,
    const vitte_diagnostic_t *right
)
{
    if (!vitte_cascade_same_source(
            left,
            right
        )) {
        return false;
    }

    /*
     * Zero-width diagnostics are common for missing-token errors.
     */
    if (left->start_offset == left->end_offset &&
        right->start_offset == right->end_offset) {
        return left->start_offset ==
               right->start_offset;
    }

    if (left->start_offset == left->end_offset) {
        return left->start_offset >=
                   right->start_offset &&
               left->start_offset <=
                   right->end_offset;
    }

    if (right->start_offset == right->end_offset) {
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
vitte_cascade_distance(
    const vitte_diagnostic_t *left,
    const vitte_diagnostic_t *right
)
{
    if (!vitte_cascade_same_source(
            left,
            right
        )) {
        return SIZE_MAX;
    }

    if (vitte_cascade_overlap(
            left,
            right
        )) {
        return 0u;
    }

    if (left->end_offset <=
        right->start_offset) {
        return right->start_offset -
               left->end_offset;
    }

    if (right->end_offset <=
        left->start_offset) {
        return left->start_offset -
               right->end_offset;
    }

    return 0u;
}

/* ========================================================================= */
/* Semantic relationships                                                    */
/* ========================================================================= */

static bool
vitte_cascade_same_symbol(
    const vitte_diagnostic_t *left,
    const vitte_diagnostic_t *right
)
{
    return left != NULL &&
           right != NULL &&
           vitte_cascade_text_equal(
               left->symbol,
               right->symbol
           );
}

static bool
vitte_cascade_same_context(
    const vitte_diagnostic_t *left,
    const vitte_diagnostic_t *right
)
{
    return left != NULL &&
           right != NULL &&
           vitte_cascade_text_equal(
               left->context,
               right->context
           );
}

static bool
vitte_cascade_same_code(
    const vitte_diagnostic_t *left,
    const vitte_diagnostic_t *right
)
{
    return left != NULL &&
           right != NULL &&
           vitte_cascade_text_equal(
               left->code,
               right->code
           );
}

/* ========================================================================= */
/* Root graph                                                                */
/* ========================================================================= */

static size_t
vitte_cascade_resolve_root(
    const vitte_diagnostic_bag_t *bag,
    size_t index
)
{
    size_t current;
    size_t steps;

    if (!vitte_cascade_index_valid(
            bag,
            index
        )) {
        return VITTE_DIAGNOSTIC_INDEX_NONE;
    }

    current = index;

    /*
     * A valid parent/root chain can never contain more nodes than the bag.
     * This bound also protects against corrupted cyclic metadata.
     */
    for (steps = 0u;
         steps <= bag->count;
         steps++) {

        const vitte_diagnostic_t *diagnostic;

        if (!vitte_cascade_index_valid(
                bag,
                current
            )) {
            return VITTE_DIAGNOSTIC_INDEX_NONE;
        }

        diagnostic =
            &bag->storage[current];

        if (diagnostic->root_diagnostic ==
                VITTE_DIAGNOSTIC_INDEX_NONE) {
            return current;
        }

        if (diagnostic->root_diagnostic ==
                current) {
            return current;
        }

        /*
         * Causal edges must always point backwards. A forward reference is
         * invalid and is never allowed to suppress another diagnostic.
         */
        if (diagnostic->root_diagnostic >
            current) {
            return VITTE_DIAGNOSTIC_INDEX_NONE;
        }

        current =
            diagnostic->root_diagnostic;
    }

    return VITTE_DIAGNOSTIC_INDEX_NONE;
}

static bool
vitte_cascade_would_cycle(
    const vitte_diagnostic_bag_t *bag,
    size_t child_index,
    size_t parent_index
)
{
    size_t current;
    size_t steps;

    if (!vitte_cascade_index_valid(
            bag,
            child_index
        ) ||
        !vitte_cascade_index_valid(
            bag,
            parent_index
        )) {
        return true;
    }

    if (child_index == parent_index) {
        return true;
    }

    current = parent_index;

    for (steps = 0u;
         steps <= bag->count;
         steps++) {

        const vitte_diagnostic_t *diagnostic;

        if (current == child_index) {
            return true;
        }

        if (!vitte_cascade_index_valid(
                bag,
                current
            )) {
            return false;
        }

        diagnostic =
            &bag->storage[current];

        if (diagnostic->parent_diagnostic ==
                VITTE_DIAGNOSTIC_INDEX_NONE ||
            diagnostic->parent_diagnostic ==
                current) {
            return false;
        }

        if (diagnostic->parent_diagnostic >
            current) {
            return true;
        }

        current =
            diagnostic->parent_diagnostic;
    }

    return true;
}

/* ========================================================================= */
/* Explicit relationships                                                    */
/* ========================================================================= */

static bool
vitte_cascade_has_explicit_relation(
    const vitte_diagnostic_bag_t *bag,
    size_t child_index
)
{
    const vitte_diagnostic_t *diagnostic;

    if (!vitte_cascade_index_valid(
            bag,
            child_index
        )) {
        return false;
    }

    diagnostic =
        &bag->storage[child_index];

    if (!diagnostic->is_cascade) {
        return false;
    }

    if (diagnostic->parent_diagnostic ==
        VITTE_DIAGNOSTIC_INDEX_NONE) {
        return false;
    }

    if (diagnostic->parent_diagnostic >=
        child_index) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Scoring                                                                   */
/* ========================================================================= */

static unsigned
vitte_cascade_score(
    const vitte_diagnostic_t *candidate,
    const vitte_diagnostic_t *derived
)
{
    unsigned score;
    size_t distance;
    bool strong_semantic_relation;
    bool strong_location_relation;

    if (candidate == NULL ||
        derived == NULL ||
        !vitte_cascade_is_error(candidate) ||
        !vitte_cascade_is_error(derived)) {
        return 0u;
    }

    if (!vitte_cascade_phase_is_same_or_later(
            candidate->origin,
            derived->origin
        )) {
        return 0u;
    }

    if (!vitte_cascade_phase_dependency(
            candidate->origin,
            derived->origin
        )) {
        return 0u;
    }

    score = 0u;
    strong_semantic_relation = false;
    strong_location_relation = false;

    if (vitte_cascade_same_span(
            candidate,
            derived
        )) {
        score +=
            VITTE_CASCADE_SCORE_SAME_SPAN;

        strong_location_relation = true;
    } else if (vitte_cascade_contains(
                   candidate,
                   derived
               ) ||
               vitte_cascade_contains(
                   derived,
                   candidate
               )) {

        score +=
            VITTE_CASCADE_SCORE_CONTAINS;

        strong_location_relation = true;
    } else if (vitte_cascade_overlap(
                   candidate,
                   derived
               )) {

        score +=
            VITTE_CASCADE_SCORE_OVERLAP;

        strong_location_relation = true;
    }

    if (vitte_cascade_same_symbol(
            candidate,
            derived
        )) {
        score +=
            VITTE_CASCADE_SCORE_SAME_SYMBOL;

        strong_semantic_relation = true;
    }

    if (vitte_cascade_same_context(
            candidate,
            derived
        )) {
        score +=
            VITTE_CASCADE_SCORE_SAME_CONTEXT;

        strong_semantic_relation = true;
    }

    if (vitte_cascade_same_code(
            candidate,
            derived
        )) {
        score +=
            VITTE_CASCADE_SCORE_SAME_CODE;
    }

    if (candidate->origin ==
        derived->origin) {
        score +=
            VITTE_CASCADE_SCORE_SAME_PHASE;
    } else if (vitte_cascade_phase_is_adjacent(
                   candidate->origin,
                   derived->origin
               )) {
        score +=
            VITTE_CASCADE_SCORE_ADJACENT_PHASE;
    } else {
        score +=
            VITTE_CASCADE_SCORE_LATER_PHASE;
    }

    /*
     * Distance is weak evidence and is only accepted in combination with a
     * semantic relationship. Two unrelated errors next to one another must
     * remain independent.
     */
    if (!strong_location_relation &&
        strong_semantic_relation) {

        distance =
            vitte_cascade_distance(
                candidate,
                derived
            );

        if (distance <= 8u) {
            score +=
                VITTE_CASCADE_SCORE_NEAR_8;
        } else if (distance <= 32u) {
            score +=
                VITTE_CASCADE_SCORE_NEAR_32;
        }
    }

    /*
     * A phase dependency by itself is insufficient. Require either source
     * locality or a semantic relation.
     */
    if (!strong_location_relation &&
        !strong_semantic_relation) {
        return 0u;
    }

    return score;
}

/* ========================================================================= */
/* Candidate ordering                                                        */
/* ========================================================================= */

static bool
vitte_cascade_candidate_is_better(
    const vitte_cascade_candidate_t *candidate,
    const vitte_cascade_candidate_t *best
)
{
    if (candidate == NULL ||
        !candidate->valid) {
        return false;
    }

    if (best == NULL ||
        !best->valid) {
        return true;
    }

    if (candidate->score != best->score) {
        return candidate->score >
               best->score;
    }

    /*
     * Prefer the nearest causal parent on a score tie. The root remains
     * available separately through root_index.
     */
    return candidate->parent_index >
           best->parent_index;
}

static vitte_cascade_candidate_t
vitte_cascade_find_best_parent(
    const vitte_diagnostic_bag_t *bag,
    size_t derived_index
)
{
    vitte_cascade_candidate_t best;
    size_t index;

    memset(
        &best,
        0,
        sizeof(best)
    );

    best.parent_index =
        VITTE_DIAGNOSTIC_INDEX_NONE;

    best.root_index =
        VITTE_DIAGNOSTIC_INDEX_NONE;

    if (!vitte_cascade_index_valid(
            bag,
            derived_index
        )) {
        return best;
    }

    for (index = 0u;
         index < derived_index;
         index++) {

        const vitte_diagnostic_t *candidate;
        const vitte_diagnostic_t *derived;
        vitte_cascade_candidate_t current;

        candidate =
            &bag->storage[index];

        derived =
            &bag->storage[derived_index];

        if (!vitte_cascade_is_error(
                candidate
            )) {
            continue;
        }

        /*
         * A suppressed diagnostic may still be a useful immediate parent if
         * it belongs to a valid existing cascade. Its root remains visible.
         */
        if (candidate->is_suppressed &&
            !candidate->is_cascade) {
            continue;
        }

        memset(
            &current,
            0,
            sizeof(current)
        );

        current.score =
            vitte_cascade_score(
                candidate,
                derived
            );

        if (current.score <
            VITTE_CASCADE_SCORE_MINIMUM) {
            continue;
        }

        current.root_index =
            vitte_cascade_resolve_root(
                bag,
                index
            );

        if (current.root_index ==
            VITTE_DIAGNOSTIC_INDEX_NONE) {
            continue;
        }

        if (vitte_cascade_would_cycle(
                bag,
                derived_index,
                index
            )) {
            continue;
        }

        current.parent_index = index;
        current.valid = true;

        if (vitte_cascade_candidate_is_better(
                &current,
                &best
            )) {
            best = current;
        }
    }

    return best;
}

/* ========================================================================= */
/* State helpers                                                             */
/* ========================================================================= */

static void
vitte_cascade_mark_primary(
    vitte_diagnostic_t *diagnostic,
    size_t index
)
{
    if (diagnostic == NULL) {
        return;
    }

    diagnostic->parent_diagnostic =
        VITTE_DIAGNOSTIC_INDEX_NONE;

    diagnostic->root_diagnostic =
        index;

    diagnostic->is_primary = true;
    diagnostic->is_cascade = false;
}

static void
vitte_cascade_mark_derived(
    vitte_diagnostic_t *diagnostic,
    size_t parent_index,
    size_t root_index
)
{
    if (diagnostic == NULL) {
        return;
    }

    diagnostic->parent_diagnostic =
        parent_index;

    diagnostic->root_diagnostic =
        root_index;

    diagnostic->is_primary = false;
    diagnostic->is_cascade = true;
}

static void
vitte_cascade_suppress(
    vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_t *diagnostic
)
{
    if (bag == NULL ||
        diagnostic == NULL ||
        diagnostic->is_suppressed) {
        return;
    }

    diagnostic->is_suppressed = true;

    bag->counts.suppressed_count++;
}

/* ========================================================================= */
/* Explicit API                                                              */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_set_last_parent(
    vitte_diagnostic_bag_t *bag,
    size_t parent_index
)
{
    size_t child_index;
    size_t root_index;
    vitte_diagnostic_t *child;

    if (!vitte_diagnostic_bag_is_initialized(
            bag
        ) ||
        bag->count == 0u) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    child_index =
        bag->count - 1u;

    if (parent_index >= child_index) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (vitte_cascade_would_cycle(
            bag,
            child_index,
            parent_index
        )) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    root_index =
        vitte_cascade_resolve_root(
            bag,
            parent_index
        );

    if (root_index ==
        VITTE_DIAGNOSTIC_INDEX_NONE) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    child =
        &bag->storage[child_index];

    vitte_cascade_mark_derived(
        child,
        parent_index,
        root_index
    );

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_mark_last_cascade(
    vitte_diagnostic_bag_t *bag,
    size_t root_index
)
{
    size_t child_index;
    size_t resolved_root;
    vitte_diagnostic_t *child;

    if (!vitte_diagnostic_bag_is_initialized(
            bag
        ) ||
        bag->count == 0u) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    child_index =
        bag->count - 1u;

    if (root_index >= child_index) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    resolved_root =
        vitte_cascade_resolve_root(
            bag,
            root_index
        );

    if (resolved_root ==
        VITTE_DIAGNOSTIC_INDEX_NONE) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    child =
        &bag->storage[child_index];

    vitte_cascade_mark_derived(
        child,
        root_index,
        resolved_root
    );

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_mark_last_suppressed(
    vitte_diagnostic_bag_t *bag
)
{
    if (!vitte_diagnostic_bag_is_initialized(
            bag
        ) ||
        bag->count == 0u) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    vitte_cascade_suppress(
        bag,
        &bag->storage[bag->count - 1u]
    );

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Pre-emission suppression query                                            */
/* ========================================================================= */

bool
vitte_diagnostic_should_suppress_cascade(
    const vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_origin_t phase,
    const vitte_ast_span_t *span
)
{
    size_t index;

    if (!vitte_diagnostic_bag_is_initialized(
            bag
        ) ||
        span == NULL ||
        !vitte_ast_span_is_valid(
            *span
        )) {
        return false;
    }

    for (index = 0u;
         index < bag->count;
         index++) {

        const vitte_diagnostic_t *root;
        bool same_source;
        bool overlaps;

        root =
            &bag->storage[index];

        if (!vitte_cascade_is_error(root) ||
            root->is_suppressed ||
            root->is_cascade ||
            !root->has_span) {
            continue;
        }

        if (!vitte_cascade_phase_dependency(
                root->origin,
                phase
            )) {
            continue;
        }

        if (!vitte_cascade_phase_is_same_or_later(
                root->origin,
                phase
            )) {
            continue;
        }

        same_source = false;

        if (root->source_id !=
                VITTE_SOURCE_ID_INVALID &&
            span->source_id !=
                VITTE_SOURCE_ID_INVALID) {

            same_source =
                root->source_id ==
                span->source_id;

        } else if (root->source_name != NULL &&
                   span->source_name != NULL) {

            same_source =
                strcmp(
                    root->source_name,
                    span->source_name
                ) == 0;
        }

        if (!same_source) {
            continue;
        }

        overlaps =
            root->start_offset <= span->end_offset &&
            span->start_offset <= root->end_offset;

        if (overlaps) {
            return true;
        }
    }

    return false;
}

/* ========================================================================= */
/* Automatic classification                                                  */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_classify_last_cascade(
    vitte_diagnostic_bag_t *bag,
    bool suppress
)
{
    size_t index;
    vitte_diagnostic_t *diagnostic;
    vitte_cascade_candidate_t parent;

    if (!vitte_diagnostic_bag_is_initialized(
            bag
        ) ||
        bag->count == 0u) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    index =
        bag->count - 1u;

    diagnostic =
        &bag->storage[index];

    if (!vitte_cascade_is_error(
            diagnostic
        )) {
        return VITTE_STATUS_OK;
    }

    /*
     * Compiler phases may establish exact causal relationships. Never replace
     * those relationships with heuristic guesses.
     */
    if (vitte_cascade_has_explicit_relation(
            bag,
            index
        )) {

        if (suppress) {
            vitte_cascade_suppress(
                bag,
                diagnostic
            );
        }

        return VITTE_STATUS_OK;
    }

    parent =
        vitte_cascade_find_best_parent(
            bag,
            index
        );

    if (!parent.valid) {
        vitte_cascade_mark_primary(
            diagnostic,
            index
        );

        return VITTE_STATUS_OK;
    }

    vitte_cascade_mark_derived(
        diagnostic,
        parent.parent_index,
        parent.root_index
    );

    if (suppress) {
        vitte_cascade_suppress(
            bag,
            diagnostic
        );
    }

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_classify_cascades(
    vitte_diagnostic_bag_t *bag,
    bool suppress
)
{
    size_t index;

    if (!vitte_diagnostic_bag_is_initialized(
            bag
        )) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    for (index = 0u;
         index < bag->count;
         index++) {

        vitte_diagnostic_t *diagnostic;
        vitte_cascade_candidate_t parent;

        diagnostic =
            &bag->storage[index];

        if (!vitte_cascade_is_error(
                diagnostic
            )) {
            continue;
        }

        if (vitte_cascade_has_explicit_relation(
                bag,
                index
            )) {

            if (suppress) {
                vitte_cascade_suppress(
                    bag,
                    diagnostic
                );
            }

            continue;
        }

        parent =
            vitte_cascade_find_best_parent(
                bag,
                index
            );

        if (!parent.valid) {
            vitte_cascade_mark_primary(
                diagnostic,
                index
            );

            continue;
        }

        vitte_cascade_mark_derived(
            diagnostic,
            parent.parent_index,
            parent.root_index
        );

        if (suppress) {
            vitte_cascade_suppress(
                bag,
                diagnostic
            );
        }
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Query API                                                                 */
/* ========================================================================= */

size_t
vitte_diagnostic_root_index(
    const vitte_diagnostic_bag_t *bag,
    size_t diagnostic_index
)
{
    return vitte_cascade_resolve_root(
        bag,
        diagnostic_index
    );
}

bool
vitte_diagnostic_is_cascade_of(
    const vitte_diagnostic_bag_t *bag,
    size_t diagnostic_index,
    size_t root_index
)
{
    size_t resolved;

    if (!vitte_cascade_index_valid(
            bag,
            diagnostic_index
        ) ||
        !vitte_cascade_index_valid(
            bag,
            root_index
        ) ||
        diagnostic_index == root_index) {
        return false;
    }

    resolved =
        vitte_cascade_resolve_root(
            bag,
            diagnostic_index
        );

    return resolved !=
               VITTE_DIAGNOSTIC_INDEX_NONE &&
           resolved == root_index;
}

size_t
vitte_diagnostic_cascade_count(
    const vitte_diagnostic_bag_t *bag
)
{
    size_t index;
    size_t count;

    if (!vitte_diagnostic_bag_is_initialized(
            bag
        )) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < bag->count;
         index++) {

        if (bag->storage[index].is_cascade) {
            count++;
        }
    }

    return count;
}

size_t
vitte_diagnostic_primary_count(
    const vitte_diagnostic_bag_t *bag
)
{
    size_t index;
    size_t count;

    if (!vitte_diagnostic_bag_is_initialized(
            bag
        )) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < bag->count;
         index++) {

        const vitte_diagnostic_t *diagnostic;

        diagnostic =
            &bag->storage[index];

        if (vitte_cascade_is_error(
                diagnostic
            ) &&
            diagnostic->is_primary &&
            !diagnostic->is_suppressed) {
            count++;
        }
    }

    return count;
}
