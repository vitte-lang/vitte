#include "cascade.h"
#include "diagnostic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Vitte diagnostic cascade engine.
 *
 * The cascade engine is responsible for causal classification only.
 *
 * It does not create semantic knowledge. The producer that detects an error
 * remains responsible for emitting the richest possible structured
 * diagnostic:
 *
 *   lexer/scanner:
 *       token, lexical expectation, recovery point
 *
 *   parser:
 *       expected/found syntax, recovery token, missing delimiter
 *
 *   import/name resolution:
 *       unresolved entity, candidate declarations, import provenance
 *
 *   type/sema:
 *       expected/found types, symbol, declaration/use relationship
 *
 *   contract/const-eval:
 *       failing expression and causal provenance
 *
 *   HIR/IR/codegen/backend:
 *       lowering and generated-code provenance
 *
 * cascade.c consumes this information and decides whether an error is:
 *
 *   primary:
 *       independent/root diagnostic
 *
 *   cascade:
 *       consequence of an earlier diagnostic
 *
 *   suppressed:
 *       retained internally but hidden from normal human-facing output
 *
 * Design rules:
 *
 *   1. Explicit compiler-provided causal relationships always win.
 *   2. Heuristics must be conservative.
 *   3. Source proximity alone must never suppress an independent error.
 *   4. Same diagnostic code alone is weak evidence.
 *   5. Same symbol is strong semantic evidence.
 *   6. Same expected/found type relationship is useful semantic evidence.
 *   7. Cross-phase relationships must respect compiler dependencies.
 *   8. Causal edges must point to already emitted diagnostics.
 *   9. Cycles and corrupted relationships must never cause suppression.
 *  10. Suppression never deletes a diagnostic.
 *  11. Root diagnostics remain visible.
 *  12. Reclassification must be deterministic.
 *  13. A fatal/internal diagnostic must not accidentally absorb unrelated
 *      frontend errors.
 *  14. Missing-token zero-width spans use half-open span semantics.
 *  15. Automatic classification is a fallback, not a substitute for
 *      producer-provided causal metadata.
 */

#ifndef VITTE_DIAGNOSTIC_INDEX_NONE
#define VITTE_DIAGNOSTIC_INDEX_NONE SIZE_MAX
#endif

/* ========================================================================= */
/* Scoring policy                                                            */
/* ========================================================================= */

/*
 * Automatic cascade classification requires at least this score.
 *
 * Strong evidence:
 *
 *   exact span                  120
 *   semantic subject            80
 *   expected/found relation     70
 *   containment                 70
 *   overlap                     60
 *
 * Medium evidence:
 *
 *   same context                35
 *   same package/module/proc    25
 *
 * Weak evidence:
 *
 *   same code                   12
 *   phase relationship          6..20
 *   source proximity            3..10
 *
 * A candidate must additionally possess at least one strong semantic or
 * location relationship. Merely accumulating weak evidence is forbidden.
 */
#define VITTE_CASCADE_SCORE_MINIMUM               70u

#define VITTE_CASCADE_SCORE_SAME_SPAN             120u
#define VITTE_CASCADE_SCORE_CONTAINS               70u
#define VITTE_CASCADE_SCORE_OVERLAP                60u

#define VITTE_CASCADE_SCORE_SAME_SYMBOL            80u
#define VITTE_CASCADE_SCORE_TYPE_RELATION          70u
#define VITTE_CASCADE_SCORE_SAME_CONTEXT           35u
#define VITTE_CASCADE_SCORE_SAME_COMPILATION_CTX   25u
#define VITTE_CASCADE_SCORE_SAME_CODE              12u

#define VITTE_CASCADE_SCORE_SAME_PHASE             20u
#define VITTE_CASCADE_SCORE_ADJACENT_PHASE         14u
#define VITTE_CASCADE_SCORE_LATER_PHASE             6u

#define VITTE_CASCADE_SCORE_NEAR_4                 10u
#define VITTE_CASCADE_SCORE_NEAR_16                 6u
#define VITTE_CASCADE_SCORE_NEAR_64                 3u

/*
 * Penalties reduce false-positive cascade classification.
 */
#define VITTE_CASCADE_PENALTY_DIFFERENT_SOURCE     200u
#define VITTE_CASCADE_PENALTY_DIFFERENT_SYMBOL      20u
#define VITTE_CASCADE_PENALTY_UNRELATED_CONTEXT     10u

/* ========================================================================= */
/* Candidate                                                                 */
/* ========================================================================= */

typedef struct vitte_cascade_candidate {
    size_t parent_index;
    size_t root_index;

    unsigned score;

    bool valid;

    bool same_source;
    bool strong_location;
    bool strong_semantic;
    bool same_symbol;
    bool same_context;
    bool type_relation;
} vitte_cascade_candidate_t;

/* ========================================================================= */
/* Basic helpers                                                             */
/* ========================================================================= */

static bool
vitte_cascade_text_present(
    const char *text
)
{
    return text != NULL &&
           text[0] != '\0';
}

static bool
vitte_cascade_text_equal(
    const char *left,
    const char *right
)
{
    if (!vitte_cascade_text_present(left) ||
        !vitte_cascade_text_present(right)) {
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

static unsigned
vitte_cascade_saturating_add(
    unsigned left,
    unsigned right
)
{
    if (UINT32_MAX - left < right) {
        return UINT32_MAX;
    }

    return left + right;
}

static unsigned
vitte_cascade_saturating_sub(
    unsigned left,
    unsigned right
)
{
    if (right >= left) {
        return 0u;
    }

    return left - right;
}

/* ========================================================================= */
/* Phase ordering                                                            */
/* ========================================================================= */

static unsigned
vitte_cascade_phase_rank(
    vitte_diagnostic_origin_t origin
)
{
    switch (origin) {
        case VITTE_DIAGNOSTIC_ORIGIN_IO:
            return 10u;

        case VITTE_DIAGNOSTIC_ORIGIN_SCANNER:
            return 15u;

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

        case VITTE_DIAGNOSTIC_ORIGIN_CODEGEN:
            return 85u;

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
    unsigned difference;

    root_rank =
        vitte_cascade_phase_rank(root);

    derived_rank =
        vitte_cascade_phase_rank(derived);

    if (root_rank == 0u ||
        derived_rank == 0u ||
        derived_rank < root_rank) {
        return false;
    }

    difference =
        derived_rank - root_rank;

    return difference <= 15u;
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
    if (root == VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN ||
        derived == VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN) {
        return false;
    }

    if (root == derived) {
        return true;
    }

    /*
     * Internal/driver/runtime failures must not become generic roots for
     * arbitrary frontend diagnostics.
     */
    if (root == VITTE_DIAGNOSTIC_ORIGIN_INTERNAL ||
        root == VITTE_DIAGNOSTIC_ORIGIN_DRIVER ||
        root == VITTE_DIAGNOSTIC_ORIGIN_RUNTIME) {
        return false;
    }

    switch (root) {
        case VITTE_DIAGNOSTIC_ORIGIN_IO:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_LEXER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_PARSER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IMPORT;

        case VITTE_DIAGNOSTIC_ORIGIN_SCANNER:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_LEXER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_PARSER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CODEGEN ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_LEXER:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_PARSER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_PARSER:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONTRACT ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_IMPORT:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONTRACT ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_HIR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_IR ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_LINKER;

        case VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION:
            return derived == VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_CONTRACT ||
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
            return derived == VITTE_DIAGNOSTIC_ORIGIN_CODEGEN ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
                   derived == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER;

        case VITTE_DIAGNOSTIC_ORIGIN_CODEGEN:
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

    /*
     * Half-open ranges [start,end).
     *
     * Zero-width spans represent insertion points.
     */
    if (inner->start_offset ==
        inner->end_offset) {

        return inner->start_offset >=
                   outer->start_offset &&
               inner->start_offset <=
                   outer->end_offset;
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
    bool left_empty;
    bool right_empty;

    if (!vitte_cascade_same_source(
            left,
            right
        )) {
        return false;
    }

    left_empty =
        left->start_offset ==
        left->end_offset;

    right_empty =
        right->start_offset ==
        right->end_offset;

    if (left_empty &&
        right_empty) {

        return left->start_offset ==
               right->start_offset;
    }

    if (left_empty) {
        return left->start_offset >=
                   right->start_offset &&
               left->start_offset <=
                   right->end_offset;
    }

    if (right_empty) {
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
    if (left == NULL ||
        right == NULL) {
        return false;
    }

    if (vitte_cascade_text_equal(
            left->short_code,
            right->short_code
        )) {
        return true;
    }

    return vitte_cascade_text_equal(
        left->code,
        right->code
    );
}

static bool
vitte_cascade_same_compilation_context(
    const vitte_diagnostic_t *left,
    const vitte_diagnostic_t *right
)
{
    bool any;
    bool mismatch;

    if (left == NULL ||
        right == NULL) {
        return false;
    }

    any = false;
    mismatch = false;

    if (vitte_cascade_text_present(left->package) &&
        vitte_cascade_text_present(right->package)) {

        any = true;

        if (!vitte_cascade_text_equal(
                left->package,
                right->package
            )) {
            mismatch = true;
        }
    }

    if (vitte_cascade_text_present(left->module) &&
        vitte_cascade_text_present(right->module)) {

        any = true;

        if (!vitte_cascade_text_equal(
                left->module,
                right->module
            )) {
            mismatch = true;
        }
    }

    if (vitte_cascade_text_present(left->procedure) &&
        vitte_cascade_text_present(right->procedure)) {

        any = true;

        if (!vitte_cascade_text_equal(
                left->procedure,
                right->procedure
            )) {
            mismatch = true;
        }
    }

    return any && !mismatch;
}

static bool
vitte_cascade_type_relation(
    const vitte_diagnostic_t *candidate,
    const vitte_diagnostic_t *derived
)
{
    if (candidate == NULL ||
        derived == NULL) {
        return false;
    }

    /*
     * Exact same mismatch propagated downstream.
     */
    if (vitte_cascade_text_equal(
            candidate->expected_type,
            derived->expected_type
        ) &&
        vitte_cascade_text_equal(
            candidate->actual_type,
            derived->actual_type
        )) {
        return true;
    }

    /*
     * A previously found type may become the problematic type expected or
     * observed by a downstream phase.
     */
    if (vitte_cascade_text_equal(
            candidate->actual_type,
            derived->actual_type
        )) {
        return true;
    }

    if (vitte_cascade_text_equal(
            candidate->actual_type,
            derived->expected_type
        )) {
        return true;
    }

    if (vitte_cascade_text_equal(
            candidate->expected_type,
            derived->expected_type
        )) {
        return true;
    }

    return false;
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

    for (steps = 0u;
         steps <= bag->count;
         steps++) {

        const vitte_diagnostic_t *diagnostic;
        size_t root;

        if (!vitte_cascade_index_valid(
                bag,
                current
            )) {
            return VITTE_DIAGNOSTIC_INDEX_NONE;
        }

        diagnostic =
            &bag->storage[current];

        root =
            diagnostic->root_diagnostic;

        if (root ==
            VITTE_DIAGNOSTIC_INDEX_NONE) {

            /*
             * Unclassified diagnostics are considered their own roots.
             */
            return current;
        }

        if (root == current) {
            return current;
        }

        if (root >= bag->count) {
            return VITTE_DIAGNOSTIC_INDEX_NONE;
        }

        /*
         * During normal emission causal references point backwards.
         *
         * engine_sort() is responsible for remapping indices after physical
         * reordering, so this remains a useful graph-corruption invariant.
         */
        if (root > current) {
            return VITTE_DIAGNOSTIC_INDEX_NONE;
        }

        current = root;
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
        size_t parent;

        if (current == child_index) {
            return true;
        }

        if (!vitte_cascade_index_valid(
                bag,
                current
            )) {
            return true;
        }

        diagnostic =
            &bag->storage[current];

        parent =
            diagnostic->parent_diagnostic;

        if (parent ==
            VITTE_DIAGNOSTIC_INDEX_NONE) {
            return false;
        }

        if (parent == current) {
            return true;
        }

        if (parent >= bag->count) {
            return true;
        }

        current = parent;
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
    size_t parent;
    size_t root;

    if (!vitte_cascade_index_valid(
            bag,
            child_index
        )) {
        return false;
    }

    diagnostic =
        &bag->storage[child_index];

    parent =
        diagnostic->parent_diagnostic;

    root =
        diagnostic->root_diagnostic;

    if (!diagnostic->is_cascade ||
        parent == VITTE_DIAGNOSTIC_INDEX_NONE) {
        return false;
    }

    if (parent >= child_index) {
        return false;
    }

    if (!vitte_cascade_index_valid(
            bag,
            parent
        )) {
        return false;
    }

    if (root != VITTE_DIAGNOSTIC_INDEX_NONE &&
        !vitte_cascade_index_valid(
            bag,
            root
        )) {
        return false;
    }

    if (vitte_cascade_would_cycle(
            bag,
            child_index,
            parent
        )) {
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
    const vitte_diagnostic_t *derived,
    vitte_cascade_candidate_t *metadata
)
{
    unsigned score;
    size_t distance;

    bool same_source;
    bool same_symbol;
    bool same_context;
    bool same_compilation_context;
    bool type_relation;
    bool strong_location;
    bool strong_semantic;

    if (metadata != NULL) {
        memset(
            metadata,
            0,
            sizeof(*metadata)
        );
    }

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

    same_source =
        vitte_cascade_same_source(
            candidate,
            derived
        );

    same_symbol =
        vitte_cascade_same_symbol(
            candidate,
            derived
        );

    same_context =
        vitte_cascade_same_context(
            candidate,
            derived
        );

    same_compilation_context =
        vitte_cascade_same_compilation_context(
            candidate,
            derived
        );

    type_relation =
        vitte_cascade_type_relation(
            candidate,
            derived
        );

    strong_location = false;
    strong_semantic = false;

    /* --------------------------------------------------------------------- */
    /* Location evidence                                                     */
    /* --------------------------------------------------------------------- */

    if (same_source) {
        if (vitte_cascade_same_span(
                candidate,
                derived
            )) {

            score =
                vitte_cascade_saturating_add(
                    score,
                    VITTE_CASCADE_SCORE_SAME_SPAN
                );

            strong_location = true;

        } else if (vitte_cascade_contains(
                       candidate,
                       derived
                   ) ||
                   vitte_cascade_contains(
                       derived,
                       candidate
                   )) {

            score =
                vitte_cascade_saturating_add(
                    score,
                    VITTE_CASCADE_SCORE_CONTAINS
                );

            strong_location = true;

        } else if (vitte_cascade_overlap(
                       candidate,
                       derived
                   )) {

            score =
                vitte_cascade_saturating_add(
                    score,
                    VITTE_CASCADE_SCORE_OVERLAP
                );

            strong_location = true;
        }
    }

    /* --------------------------------------------------------------------- */
    /* Semantic evidence                                                     */
    /* --------------------------------------------------------------------- */

    if (same_symbol) {
        score =
            vitte_cascade_saturating_add(
                score,
                VITTE_CASCADE_SCORE_SAME_SYMBOL
            );

        strong_semantic = true;
    }

    if (type_relation) {
        score =
            vitte_cascade_saturating_add(
                score,
                VITTE_CASCADE_SCORE_TYPE_RELATION
            );

        strong_semantic = true;
    }

    if (same_context) {
        score =
            vitte_cascade_saturating_add(
                score,
                VITTE_CASCADE_SCORE_SAME_CONTEXT
            );

        strong_semantic = true;
    }

    if (same_compilation_context) {
        score =
            vitte_cascade_saturating_add(
                score,
                VITTE_CASCADE_SCORE_SAME_COMPILATION_CTX
            );
    }

    if (vitte_cascade_same_code(
            candidate,
            derived
        )) {

        score =
            vitte_cascade_saturating_add(
                score,
                VITTE_CASCADE_SCORE_SAME_CODE
            );
    }

    /* --------------------------------------------------------------------- */
    /* Phase evidence                                                        */
    /* --------------------------------------------------------------------- */

    if (candidate->origin ==
        derived->origin) {

        score =
            vitte_cascade_saturating_add(
                score,
                VITTE_CASCADE_SCORE_SAME_PHASE
            );

    } else if (vitte_cascade_phase_is_adjacent(
                   candidate->origin,
                   derived->origin
               )) {

        score =
            vitte_cascade_saturating_add(
                score,
                VITTE_CASCADE_SCORE_ADJACENT_PHASE
            );

    } else {
        score =
            vitte_cascade_saturating_add(
                score,
                VITTE_CASCADE_SCORE_LATER_PHASE
            );
    }

    /* --------------------------------------------------------------------- */
    /* Distance                                                              */
    /* --------------------------------------------------------------------- */

    if (same_source &&
        !strong_location &&
        strong_semantic) {

        distance =
            vitte_cascade_distance(
                candidate,
                derived
            );

        if (distance <= 4u) {
            score =
                vitte_cascade_saturating_add(
                    score,
                    VITTE_CASCADE_SCORE_NEAR_4
                );

        } else if (distance <= 16u) {
            score =
                vitte_cascade_saturating_add(
                    score,
                    VITTE_CASCADE_SCORE_NEAR_16
                );

        } else if (distance <= 64u) {
            score =
                vitte_cascade_saturating_add(
                    score,
                    VITTE_CASCADE_SCORE_NEAR_64
                );
        }
    }

    /* --------------------------------------------------------------------- */
    /* Conservative penalties                                                */
    /* --------------------------------------------------------------------- */

    if (candidate->has_span &&
        derived->has_span &&
        !same_source) {

        /*
         * Cross-file causal relationships should normally be explicit
         * (imports, generated code, macro expansion, etc.).
         *
         * Automatic suppression across unrelated source files is dangerous.
         */
        score =
            vitte_cascade_saturating_sub(
                score,
                VITTE_CASCADE_PENALTY_DIFFERENT_SOURCE
            );
    }

    if (vitte_cascade_text_present(candidate->symbol) &&
        vitte_cascade_text_present(derived->symbol) &&
        !same_symbol) {

        score =
            vitte_cascade_saturating_sub(
                score,
                VITTE_CASCADE_PENALTY_DIFFERENT_SYMBOL
            );
    }

    if (vitte_cascade_text_present(candidate->context) &&
        vitte_cascade_text_present(derived->context) &&
        !same_context) {

        score =
            vitte_cascade_saturating_sub(
                score,
                VITTE_CASCADE_PENALTY_UNRELATED_CONTEXT
            );
    }

    /*
     * Weak evidence must never be enough.
     *
     * A phase dependency, same code and proximity can all occur between two
     * genuinely independent errors.
     */
    if (!strong_location &&
        !strong_semantic) {
        return 0u;
    }

    if (metadata != NULL) {
        metadata->same_source =
            same_source;

        metadata->strong_location =
            strong_location;

        metadata->strong_semantic =
            strong_semantic;

        metadata->same_symbol =
            same_symbol;

        metadata->same_context =
            same_context;

        metadata->type_relation =
            type_relation;
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

    if (candidate->score !=
        best->score) {

        return candidate->score >
               best->score;
    }

    /*
     * Prefer semantic evidence over pure source-location evidence.
     */
    if (candidate->strong_semantic !=
        best->strong_semantic) {

        return candidate->strong_semantic;
    }

    if (candidate->same_symbol !=
        best->same_symbol) {

        return candidate->same_symbol;
    }

    if (candidate->type_relation !=
        best->type_relation) {

        return candidate->type_relation;
    }

    if (candidate->same_context !=
        best->same_context) {

        return candidate->same_context;
    }

    /*
     * Then prefer the nearest causal predecessor in emission order.
     *
     * This preserves the most specific intermediate causal chain:
     *
     *   root
     *     -> dependent type failure
     *         -> lowering failure
     *
     * rather than flattening every child directly onto root.
     */
    if (candidate->parent_index !=
        best->parent_index) {

        return candidate->parent_index >
               best->parent_index;
    }

    /*
     * Deterministic final tie-breaker.
     */
    return candidate->root_index <
           best->root_index;
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
         * Suppressed diagnostics are valid intermediate parents only when
         * they themselves belong to a valid cascade.
         */
        if (candidate->is_suppressed &&
            !candidate->is_cascade) {
            continue;
        }

        /*
         * Never use an invalid/corrupt cascade as a parent.
         */
        if (candidate->is_cascade &&
            vitte_cascade_resolve_root(
                bag,
                index
            ) == VITTE_DIAGNOSTIC_INDEX_NONE) {
            continue;
        }

        memset(
            &current,
            0,
            sizeof(current)
        );

        current.parent_index =
            VITTE_DIAGNOSTIC_INDEX_NONE;

        current.root_index =
            VITTE_DIAGNOSTIC_INDEX_NONE;

        current.score =
            vitte_cascade_score(
                candidate,
                derived,
                &current
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

        if (current.root_index >=
            derived_index) {
            continue;
        }

        if (vitte_cascade_would_cycle(
                bag,
                derived_index,
                index
            )) {
            continue;
        }

        current.parent_index =
            index;

        current.valid =
            true;

        if (vitte_cascade_candidate_is_better(
                &current,
                &best
            )) {

            best =
                current;
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

    diagnostic->is_primary =
        true;

    diagnostic->is_cascade =
        false;
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

    diagnostic->is_primary =
        false;

    diagnostic->is_cascade =
        true;
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

    diagnostic->is_suppressed =
        true;

    bag->counts.suppressed_count++;
}

static void
vitte_cascade_unsuppress(
    vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_t *diagnostic
)
{
    if (bag == NULL ||
        diagnostic == NULL ||
        !diagnostic->is_suppressed) {
        return;
    }

    diagnostic->is_suppressed =
        false;

    if (bag->counts.suppressed_count != 0u) {
        bag->counts.suppressed_count--;
    }
}

static size_t
vitte_cascade_recount_suppressed(
    const vitte_diagnostic_bag_t *bag
)
{
    size_t index;
    size_t count;

    if (bag == NULL) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < bag->count;
         index++) {

        if (bag->storage[index].is_suppressed) {
            count++;
        }
    }

    return count;
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

    if (parent_index >=
        child_index) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (!vitte_cascade_index_valid(
            bag,
            parent_index
        )) {

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

    if (root_index >=
        child_index) {

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

    if (root_index >=
        child_index) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (!vitte_cascade_index_valid(
            bag,
            root_index
        )) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (vitte_cascade_would_cycle(
            bag,
            child_index,
            root_index
        )) {

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
        &bag->storage[
            bag->count - 1u
        ]
    );

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Pre-emission suppression                                                  */
/* ========================================================================= */

static bool
vitte_cascade_span_matches_diagnostic(
    const vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span
)
{
    bool same_source;
    bool diagnostic_empty;
    bool span_empty;

    if (diagnostic == NULL ||
        span == NULL ||
        !diagnostic->has_span) {
        return false;
    }

    same_source = false;

    if (diagnostic->source_id !=
            VITTE_SOURCE_ID_INVALID &&
        span->source_id !=
            VITTE_SOURCE_ID_INVALID) {

        same_source =
            diagnostic->source_id ==
            span->source_id;

    } else if (
        vitte_cascade_text_present(
            diagnostic->source_name
        ) &&
        vitte_cascade_text_present(
            span->source_name
        )) {

        same_source =
            strcmp(
                diagnostic->source_name,
                span->source_name
            ) == 0;
    }

    if (!same_source) {
        return false;
    }

    diagnostic_empty =
        diagnostic->start_offset ==
        diagnostic->end_offset;

    span_empty =
        span->start_offset ==
        span->end_offset;

    if (diagnostic_empty &&
        span_empty) {

        return diagnostic->start_offset ==
               span->start_offset;
    }

    if (diagnostic_empty) {
        return diagnostic->start_offset >=
                   span->start_offset &&
               diagnostic->start_offset <=
                   span->end_offset;
    }

    if (span_empty) {
        return span->start_offset >=
                   diagnostic->start_offset &&
               span->start_offset <=
                   diagnostic->end_offset;
    }

    return diagnostic->start_offset <
               span->end_offset &&
           span->start_offset <
               diagnostic->end_offset;
}

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
        !span->valid ||
        !vitte_ast_span_is_valid(
            *span
        ) ||
        span->end_offset < span->start_offset ||
        span->end_line < span->start_line ||
        (span->end_line == span->start_line &&
         span->end_column < span->start_column) ||
        !vitte_diagnostic_origin_is_valid(
            phase
        )) {

        return false;
    }

    /*
     * Search newest-first.
     *
     * Recent root diagnostics are generally more relevant to current parser
     * or semantic recovery state.
     */
    index =
        bag->count;

    while (index != 0u) {
        const vitte_diagnostic_t *root;

        index--;

        root =
            &bag->storage[index];

        if (!vitte_cascade_is_error(
                root
            ) ||
            root->is_suppressed ||
            root->is_cascade ||
            !root->is_primary ||
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

        /*
         * Pre-emission suppression has less semantic information than normal
         * post-emission classification. Therefore require actual overlap.
         */
        if (vitte_cascade_span_matches_diagnostic(
                root,
                span
            )) {

            return true;
        }
    }

    return false;
}

/* ========================================================================= */
/* Automatic classification                                                  */
/* ========================================================================= */

static vitte_status_t
vitte_cascade_classify_one(
    vitte_diagnostic_bag_t *bag,
    size_t index,
    bool suppress
)
{
    vitte_diagnostic_t *diagnostic;
    vitte_cascade_candidate_t parent;

    if (!vitte_cascade_index_valid(
            bag,
            index
        )) {

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    diagnostic =
        &bag->storage[index];

    if (!vitte_cascade_is_error(
            diagnostic
        )) {

        /*
         * Notes/help/warnings are not automatically classified as causal
         * compiler failures.
         */
        return VITTE_STATUS_OK;
    }

    /*
     * Explicit compiler relationships are authoritative.
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
        } else {
            vitte_cascade_unsuppress(
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

        /*
         * A primary error must never remain hidden merely because an earlier
         * classification pass suppressed it.
         */
        vitte_cascade_unsuppress(
            bag,
            diagnostic
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
    } else {
        vitte_cascade_unsuppress(
            bag,
            diagnostic
        );
    }

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_diagnostic_classify_last_cascade(
    vitte_diagnostic_bag_t *bag,
    bool suppress
)
{
    if (!vitte_diagnostic_bag_is_initialized(
            bag
        ) ||
        bag->count == 0u) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_cascade_classify_one(
        bag,
        bag->count - 1u,
        suppress
    );
}

vitte_status_t
vitte_diagnostic_classify_cascades(
    vitte_diagnostic_bag_t *bag,
    bool suppress
)
{
    size_t index;
    vitte_status_t status;

    if (!vitte_diagnostic_bag_is_initialized(
            bag
        )) {

        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    /*
     * Rebuild in emission order so every possible parent has already been
     * classified before its children.
     */
    for (index = 0u;
         index < bag->count;
         index++) {

        status =
            vitte_cascade_classify_one(
                bag,
                index,
                suppress
            );

        if (status !=
            VITTE_STATUS_OK) {

            return status;
        }
    }

    /*
     * Defensive recount.
     *
     * This guarantees that repeated finalization/classification cannot drift
     * suppressed_count away from physical diagnostic state.
     */
    bag->counts.suppressed_count =
        vitte_cascade_recount_suppressed(
            bag
        );

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
        diagnostic_index ==
            root_index) {

        return false;
    }

    if (!bag->storage[
            diagnostic_index
        ].is_cascade) {

        return false;
    }

    resolved =
        vitte_cascade_resolve_root(
            bag,
            diagnostic_index
        );

    return resolved !=
               VITTE_DIAGNOSTIC_INDEX_NONE &&
           resolved ==
               root_index;
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

        const vitte_diagnostic_t *diagnostic;

        diagnostic =
            &bag->storage[index];

        if (vitte_cascade_is_error(
                diagnostic
            ) &&
            diagnostic->is_cascade) {

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
            !diagnostic->is_cascade &&
            !diagnostic->is_suppressed) {

            count++;
        }
    }

    return count;
}
