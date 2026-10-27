#ifndef VITTE_DIAGNOSTIC_SUGGESTION_H
#define VITTE_DIAGNOSTIC_SUGGESTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#ifndef VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE
#define VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE SIZE_MAX
#endif

#ifndef VITTE_DIAGNOSTIC_SUGGESTION_MAX_EDITS
#define VITTE_DIAGNOSTIC_SUGGESTION_MAX_EDITS \
    VITTE_DIAGNOSTIC_MAX_SUGGESTIONS
#endif

/* ========================================================================= */
/* Model                                                                     */
/* ========================================================================= */

/*
 * Suggestions are structured diagnostic repairs.
 *
 * They can represent:
 *
 *   - explanatory help;
 *   - insertion;
 *   - deletion;
 *   - replacement;
 *   - machine-applicable fix;
 *   - uncertain fix;
 *   - placeholder-containing fix;
 *   - manual repair.
 *
 * Example:
 *
 *   error[E0501]: incompatible types
 *      |
 *   12 | let value: i32 = "hello";
 *      |                  ^^^^^^^ expected `i32`
 *      |
 *      = help: replace the string with an integer
 *
 * A structured suggestion can additionally contain:
 *
 *   span        = span of `"hello"`
 *   replacement = "0"
 *   applicability = MACHINE
 *
 * allowing:
 *
 *   vitte fix
 *   editor Code Actions
 *   LSP
 *   SARIF fixes
 *   JSON tooling
 */

/* ========================================================================= */
/* Fix plan                                                                  */
/* ========================================================================= */

/*
 * A fix plan is a bounded collection of structured edits prepared for
 * validation and eventual application.
 *
 * The source manager/file-editing layer remains responsible for actually
 * mutating source files.
 *
 * suggestion.c deliberately does not perform filesystem writes.
 */
typedef struct vitte_diagnostic_fix_plan {
    vitte_diagnostic_suggestion_t
        edits[VITTE_DIAGNOSTIC_SUGGESTION_MAX_EDITS];

    size_t count;

    /*
     * True only when every collected edit is machine-applicable.
     */
    bool machine_applicable;

    /*
     * False if any pair of edits overlaps/conflicts.
     */
    bool conflict_free;
} vitte_diagnostic_fix_plan_t;

/* ========================================================================= */
/* Suggestion validation result                                              */
/* ========================================================================= */

typedef struct vitte_diagnostic_suggestion_validation {
    bool valid;

    bool conflict_free;

    size_t suggestion_count;

    size_t invalid_count;
    size_t duplicate_count;
    size_t conflict_count;

    size_t machine_applicable_count;

    size_t insertion_count;
    size_t deletion_count;
    size_t replacement_count;

    /*
     * VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE when no invalid suggestion
     * exists.
     */
    size_t first_invalid_index;
} vitte_diagnostic_suggestion_validation_t;

/* ========================================================================= */
/* Fix-plan validation result                                                */
/* ========================================================================= */

typedef struct vitte_diagnostic_fix_plan_validation {
    bool valid;

    /*
     * True only when every edit has MACHINE applicability.
     */
    bool machine_applicable;

    /*
     * True when no edit pair conflicts.
     */
    bool conflict_free;

    size_t edit_count;
    size_t invalid_count;
    size_t conflict_count;

    /*
     * VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE when all entries are valid.
     */
    size_t first_invalid_index;
} vitte_diagnostic_fix_plan_validation_t;

/* ========================================================================= */
/* Applicability                                                             */
/* ========================================================================= */

/*
 * Stable textual representation used by:
 *
 *   terminal
 *   JSON
 *   SARIF
 *   LSP data
 *   debugging
 */
const char *
vitte_diagnostic_applicability_name(
    vitte_diagnostic_applicability_t applicability
);

/*
 * True only for fixes that can safely be applied automatically without user
 * interpretation.
 */
bool
vitte_diagnostic_applicability_machine_applicable(
    vitte_diagnostic_applicability_t applicability
);

/*
 * True for MAYBE, PLACEHOLDERS and MANUAL.
 */
bool
vitte_diagnostic_applicability_requires_review(
    vitte_diagnostic_applicability_t applicability
);

/* ========================================================================= */
/* Suggestion lifetime                                                       */
/* ========================================================================= */

/*
 * Initialize a suggestion to a safe empty state.
 *
 * Default applicability:
 *
 *   VITTE_DIAGNOSTIC_APPLICABILITY_MANUAL
 */
void
vitte_diagnostic_suggestion_init(
    vitte_diagnostic_suggestion_t *suggestion
);

/*
 * Reset to the same state as init.
 *
 * Suggestion storage is embedded; no heap allocation is owned by an
 * individual suggestion.
 */
void
vitte_diagnostic_suggestion_reset(
    vitte_diagnostic_suggestion_t *suggestion
);

/* ========================================================================= */
/* Suggestion construction                                                   */
/* ========================================================================= */

/*
 * Populate one structured suggestion.
 *
 * message:
 *   Human-readable explanation.
 *
 * span:
 *   Source range affected by the edit. May be NULL only for explanation-only
 *   suggestions.
 *
 * replacement:
 *   Replacement text.
 *
 *   NULL:
 *      no structured edit
 *
 *   "":
 *      delete span
 *
 *   non-empty:
 *      insert or replace
 *
 * applicability:
 *   Confidence/safety classification.
 *
 * The function copies message and replacement into the suggestion's owned
 * inline storage.
 */
vitte_status_t
vitte_diagnostic_suggestion_set(
    vitte_diagnostic_suggestion_t *suggestion,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability
);

/* ========================================================================= */
/* Validation / classification                                               */
/* ========================================================================= */

bool
vitte_diagnostic_suggestion_is_valid(
    const vitte_diagnostic_suggestion_t *suggestion
);

/*
 * True when both a span and replacement operation exist.
 *
 * Empty replacement text still counts as an edit because it represents a
 * deletion.
 */
bool
vitte_diagnostic_suggestion_has_edit(
    const vitte_diagnostic_suggestion_t *suggestion
);

/*
 * Insertion:
 *
 *   start_offset == end_offset
 *   replacement != ""
 */
bool
vitte_diagnostic_suggestion_is_insertion(
    const vitte_diagnostic_suggestion_t *suggestion
);

/*
 * Deletion:
 *
 *   end_offset > start_offset
 *   replacement == ""
 */
bool
vitte_diagnostic_suggestion_is_deletion(
    const vitte_diagnostic_suggestion_t *suggestion
);

/*
 * Replacement:
 *
 *   end_offset > start_offset
 *   replacement != ""
 */
bool
vitte_diagnostic_suggestion_is_replacement(
    const vitte_diagnostic_suggestion_t *suggestion
);

/* ========================================================================= */
/* Equality / compatibility                                                 */
/* ========================================================================= */

/*
 * Structural equality.
 *
 * Compares:
 *
 *   applicability
 *   span presence/value
 *   replacement presence/value
 *   message
 */
bool
vitte_diagnostic_suggestion_equal(
    const vitte_diagnostic_suggestion_t *left,
    const vitte_diagnostic_suggestion_t *right
);

/*
 * Two structured edits conflict when their affected ranges overlap in the
 * same source.
 *
 * Two insertions at exactly the same byte offset are considered conflicting
 * because deterministic application order would otherwise need extra
 * semantics.
 */
bool
vitte_diagnostic_suggestion_conflicts(
    const vitte_diagnostic_suggestion_t *left,
    const vitte_diagnostic_suggestion_t *right
);

/*
 * Equal suggestions are compatible.
 *
 * Distinct non-overlapping suggestions are compatible.
 */
bool
vitte_diagnostic_suggestion_compatible_with(
    const vitte_diagnostic_suggestion_t *candidate,
    const vitte_diagnostic_suggestion_t *other
);

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

/*
 * Find an equal suggestion already attached to a diagnostic.
 *
 * Returns:
 *
 *   index
 *
 * or:
 *
 *   VITTE_DIAGNOSTIC_SUGGESTION_INDEX_NONE
 */
size_t
vitte_diagnostic_suggestion_find(
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_suggestion_t *suggestion
);

/* ========================================================================= */
/* Add                                                                       */
/* ========================================================================= */

/*
 * Copy a fully constructed suggestion into a diagnostic.
 *
 * Exact duplicates are ignored.
 *
 * Internal pointers are rebound to the destination object's inline storage.
 */
vitte_status_t
vitte_diagnostic_add_suggestion_object(
    vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_suggestion_t *suggestion
);

/*
 * General high-level constructor + add operation.
 */
vitte_status_t
vitte_diagnostic_suggest(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability
);

/* ========================================================================= */
/* Specialized edit constructors                                             */
/* ========================================================================= */

/*
 * Insert text at position->start_offset.
 *
 * The resulting edit span is zero-width regardless of position->end_offset.
 */
vitte_status_t
vitte_diagnostic_suggest_insert(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *position,
    const char *text,
    vitte_diagnostic_applicability_t applicability
);

/*
 * Replace span with replacement.
 */
vitte_status_t
vitte_diagnostic_suggest_replace(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability
);

/*
 * Delete span.
 *
 * Internally represented as replacement text "".
 */
vitte_status_t
vitte_diagnostic_suggest_delete(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    vitte_diagnostic_applicability_t applicability
);

/* ========================================================================= */
/* Machine-applicable convenience API                                        */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_suggest_machine_replace(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement
);

vitte_status_t
vitte_diagnostic_suggest_machine_insert(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *position,
    const char *text
);

vitte_status_t
vitte_diagnostic_suggest_machine_delete(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span
);

/* ========================================================================= */
/* Access                                                                    */
/* ========================================================================= */

const vitte_diagnostic_suggestion_t *
vitte_diagnostic_suggestion_at(
    const vitte_diagnostic_t *diagnostic,
    size_t index
);

vitte_diagnostic_suggestion_t *
vitte_diagnostic_suggestion_at_mut(
    vitte_diagnostic_t *diagnostic,
    size_t index
);

size_t
vitte_diagnostic_suggestion_count(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Remove / clear                                                            */
/* ========================================================================= */

/*
 * Remove one suggestion.
 *
 * The implementation repairs inline-storage pointers after moving entries.
 */
vitte_status_t
vitte_diagnostic_suggestion_remove(
    vitte_diagnostic_t *diagnostic,
    size_t index
);

void
vitte_diagnostic_suggestion_clear(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Deterministic ordering                                                    */
/* ========================================================================= */

/*
 * Sort suggestions deterministically.
 *
 * Ordering is based primarily on:
 *
 *   source
 *   start offset
 *   end offset
 *   applicability
 *   message
 *   replacement
 *
 * Because suggestions contain pointers into their own inline storage, the
 * implementation rebinds those pointers after qsort().
 */
void
vitte_diagnostic_suggestions_sort(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Deduplication                                                             */
/* ========================================================================= */

/*
 * Remove structurally identical suggestions.
 *
 * Returns the number removed.
 */
size_t
vitte_diagnostic_suggestions_deduplicate(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Conflict analysis                                                         */
/* ========================================================================= */

bool
vitte_diagnostic_suggestions_have_conflicts(
    const vitte_diagnostic_t *diagnostic
);

size_t
vitte_diagnostic_suggestion_conflict_count(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Machine-applicable queries                                                */
/* ========================================================================= */

/*
 * Number of machine-applicable structured edits.
 *
 * Explanation-only suggestions are not counted.
 */
size_t
vitte_diagnostic_machine_suggestion_count(
    const vitte_diagnostic_t *diagnostic
);

bool
vitte_diagnostic_has_machine_suggestions(
    const vitte_diagnostic_t *diagnostic
);

/*
 * True when:
 *
 *   - at least one structured edit exists;
 *   - every structured edit is machine-applicable;
 *   - no edits conflict.
 *
 * This is the basic safety gate for unattended `vitte fix`.
 */
bool
vitte_diagnostic_suggestions_machine_safe(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Fix plans                                                                 */
/* ========================================================================= */

void
vitte_diagnostic_fix_plan_init(
    vitte_diagnostic_fix_plan_t *plan
);

/*
 * Add one structured edit to a plan.
 *
 * Duplicate edits are ignored.
 *
 * Conflict state and machine applicability are updated incrementally.
 */
vitte_status_t
vitte_diagnostic_fix_plan_add(
    vitte_diagnostic_fix_plan_t *plan,
    const vitte_diagnostic_suggestion_t *suggestion
);

/*
 * Build a fix plan from one diagnostic.
 *
 * machine_only:
 *
 *   false:
 *      collect all structured edits
 *
 *   true:
 *      collect only MACHINE edits
 */
vitte_status_t
vitte_diagnostic_build_fix_plan(
    const vitte_diagnostic_t *diagnostic,
    bool machine_only,
    vitte_diagnostic_fix_plan_t *plan
);

/*
 * Sort edits into safe application order:
 *
 *   later source offsets first.
 *
 * Applying edits from the end of a source toward its beginning prevents
 * earlier modifications from invalidating the offsets of later edits.
 *
 * For multiple source files, source identity remains part of deterministic
 * ordering.
 */
void
vitte_diagnostic_fix_plan_sort_for_application(
    vitte_diagnostic_fix_plan_t *plan
);

vitte_diagnostic_fix_plan_validation_t
vitte_diagnostic_fix_plan_validate(
    const vitte_diagnostic_fix_plan_t *plan
);

/* ========================================================================= */
/* Edit sizes                                                                */
/* ========================================================================= */

size_t
vitte_diagnostic_suggestion_removed_length(
    const vitte_diagnostic_suggestion_t *suggestion
);

size_t
vitte_diagnostic_suggestion_inserted_length(
    const vitte_diagnostic_suggestion_t *suggestion
);

/* ========================================================================= */
/* Fingerprints                                                              */
/* ========================================================================= */

/*
 * Stable non-cryptographic fingerprint of a structured suggestion.
 *
 * Useful for:
 *
 *   dedup debugging
 *   tests
 *   deterministic output
 *   tooling identity
 *
 * Hash collisions must never be treated as structural equality.
 */
uint64_t
vitte_diagnostic_suggestion_fingerprint(
    const vitte_diagnostic_suggestion_t *suggestion
);

/*
 * Fingerprint suggestions in their current order.
 *
 * Sort first when a canonical ordering-independent workflow is required.
 */
uint64_t
vitte_diagnostic_suggestions_fingerprint(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

vitte_diagnostic_suggestion_validation_t
vitte_diagnostic_suggestions_validate(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Best suggestion                                                          */
/* ========================================================================= */

/*
 * Select the preferred valid suggestion.
 *
 * Preference:
 *
 *   structured edit
 *       >
 *   explanation-only suggestion
 *
 * then stronger applicability:
 *
 *   MACHINE
 *       >
 *   MAYBE
 *       >
 *   PLACEHOLDERS
 *       >
 *   MANUAL
 *
 * then the most local source span.
 */
const vitte_diagnostic_suggestion_t *
vitte_diagnostic_best_suggestion(
    const vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

const char *
vitte_diagnostic_suggestion_component_name(void);

size_t
vitte_diagnostic_suggestion_capacity(void);

/* ========================================================================= */
/* Convenience helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_diagnostic_suggestion_is_machine_applicable(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    return suggestion != NULL &&
           suggestion->applicability ==
               VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE;
}

static inline bool
vitte_diagnostic_suggestion_is_manual(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    return suggestion != NULL &&
           suggestion->applicability ==
               VITTE_DIAGNOSTIC_APPLICABILITY_MANUAL;
}

static inline bool
vitte_diagnostic_suggestion_has_placeholders(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    return suggestion != NULL &&
           suggestion->applicability ==
               VITTE_DIAGNOSTIC_APPLICABILITY_PLACEHOLDERS;
}

static inline bool
vitte_diagnostic_suggestion_is_uncertain(
    const vitte_diagnostic_suggestion_t *suggestion
)
{
    return suggestion != NULL &&
           suggestion->applicability ==
               VITTE_DIAGNOSTIC_APPLICABILITY_MAYBE;
}

static inline bool
vitte_diagnostic_fix_plan_empty(
    const vitte_diagnostic_fix_plan_t *plan
)
{
    return plan == NULL ||
           plan->count == 0u;
}

static inline size_t
vitte_diagnostic_fix_plan_count(
    const vitte_diagnostic_fix_plan_t *plan
)
{
    return plan != NULL
        ? plan->count
        : 0u;
}

static inline const vitte_diagnostic_suggestion_t *
vitte_diagnostic_fix_plan_at(
    const vitte_diagnostic_fix_plan_t *plan,
    size_t index
)
{
    if (plan == NULL ||
        index >= plan->count) {

        return NULL;
    }

    return &plan->edits[index];
}

/* ========================================================================= */
/* Design rules                                                              */
/* ========================================================================= */

/*
 * 1. Suggestions are data, not formatted strings.
 *
 *    Bad:
 *
 *       "replace foo with bar at line 12"
 *
 *    Better:
 *
 *       message     = "replace with `bar`"
 *       span        = exact source span
 *       replacement = "bar"
 *       applicability = MACHINE
 *
 *
 * 2. Never mark a fix MACHINE merely because it is syntactically possible.
 *
 *    MACHINE means the compiler has enough information to apply the change
 *    without requiring a semantic choice from the user.
 *
 *
 * 3. MAYBE is appropriate when the proposed edit is plausible but may alter
 *    intent.
 *
 *
 * 4. PLACEHOLDERS means generated replacement text contains values the user
 *    must complete or inspect.
 *
 *
 * 5. MANUAL is appropriate for guidance that cannot safely be represented as
 *    an unattended source edit.
 */

/* ========================================================================= */
/* Automatic fix architecture                                                */
/* ========================================================================= */

/*
 * Recommended pipeline:
 *
 *   compiler phase
 *       |
 *       v
 *   vitte_diagnostic_t
 *       |
 *       v
 *   structured suggestion(s)
 *       |
 *       +-----------------------------+
 *       |                             |
 *       v                             v
 *   renderer                     fix planner
 *       |                             |
 *       + terminal                    + dedup
 *       + JSON                        + conflicts
 *       + SARIF                       + applicability
 *       + LSP                         + ordering
 *                                     |
 *                                     v
 *                                source editor
 *                                     |
 *                                     v
 *                                updated .vit
 *
 * suggestion.c intentionally stops before the source-editor/filesystem layer.
 */

/* ========================================================================= */
/* Source-edit semantics                                                     */
/* ========================================================================= */

/*
 * Ranges are half-open:
 *
 *   [start_offset, end_offset)
 *
 * Replacement:
 *
 *   source[start:end] = replacement
 *
 * Insertion:
 *
 *   start == end
 *
 * Deletion:
 *
 *   replacement == ""
 *
 * This matches the byte-span model used by the compiler.
 */

/* ========================================================================= */
/* Offset stability                                                          */
/* ========================================================================= */

/*
 * Multiple edits to one source must normally be applied in descending byte
 * offset order.
 *
 * Example:
 *
 * Original:
 *
 *   abc def ghi
 *
 * edits:
 *
 *   replace abc
 *   replace ghi
 *
 * If the first replacement changes byte length, applying it first would shift
 * the original offset of `ghi`.
 *
 * Therefore:
 *
 *   highest offset -> lowest offset
 *
 * vitte_diagnostic_fix_plan_sort_for_application() implements that ordering.
 */

/* ========================================================================= */
/* Conflicting edits                                                         */
/* ========================================================================= */

/*
 * Overlapping fixes must not be silently combined.
 *
 * Example:
 *
 *   edit A: [10, 20)
 *   edit B: [15, 18)
 *
 * Both cannot be independently applied against the same source snapshot.
 *
 * Such a plan is:
 *
 *   conflict_free = false
 *
 * and must not be treated as an unattended machine-safe fix plan.
 */

/* ========================================================================= */
/* Multiple insertions at one point                                          */
/* ========================================================================= */

/*
 * Two zero-width insertions at the same offset are conservatively considered
 * conflicting.
 *
 * Supporting them safely would require an explicit deterministic insertion
 * ordering contract.
 *
 * Until that contract exists, rejecting automatic combination is safer.
 */

/* ========================================================================= */
/* Self-referential storage                                                  */
/* ========================================================================= */

/*
 * vitte_diagnostic_suggestion_t stores text in inline arrays and exposes
 * pointers to those arrays.
 *
 * Conceptually:
 *
 *   message ----------+
 *                     |
 *                     v
 *   message_storage[...]
 *
 *   replacement ------+
 *                     |
 *                     v
 *   replacement_storage[...]
 *
 * A raw struct move performed by:
 *
 *   qsort()
 *   memmove()
 *
 * copies the pointer value as well as the storage. The pointer can therefore
 * still refer to the old object's storage.
 *
 * suggestion.c repairs these pointers after moving entries.
 *
 * Any future code that moves suggestion structs must preserve this invariant.
 */

/* ========================================================================= */
/* Diagnostic capacity                                                       */
/* ========================================================================= */

/*
 * Suggestions currently use bounded storage inside vitte_diagnostic_t:
 *
 *   suggestions[VITTE_DIAGNOSTIC_MAX_SUGGESTIONS]
 *
 * This keeps diagnostics:
 *
 *   deterministic;
 *   allocation-light;
 *   simple to serialize;
 *   robust under compiler error storms.
 *
 * If a future diagnostic needs an unbounded number of fixes, use a separate
 * owned fix-set abstraction rather than silently overflowing this array.
 */

/* ========================================================================= */
/* Renderer integration                                                      */
/* ========================================================================= */

/*
 * Terminal:
 *
 *   = help: replace `foo` with `bar`
 *
 * JSON:
 *
 *   {
 *     "message": "...",
 *     "replacement": "bar",
 *     "applicability": "machine-applicable",
 *     "span": { ... }
 *   }
 *
 * SARIF:
 *
 *   fixes[]
 *     artifactChanges[]
 *       replacements[]
 *
 * LSP:
 *
 *   diagnostic.data.suggestions
 *
 * and eventually:
 *
 *   textDocument/codeAction
 *
 * All renderers should consume the same structured suggestion model.
 */

/* ========================================================================= */
/* `vitte fix` integration                                                   */
/* ========================================================================= */

/*
 * Recommended command behavior:
 *
 *   vitte fix
 *
 * Default:
 *
 *   apply only:
 *
 *       MACHINE
 *       valid
 *       non-conflicting
 *
 * Suggested modes:
 *
 *   vitte fix --dry-run
 *   vitte fix --check
 *   vitte fix --allow-maybe
 *   vitte fix --format=json
 *
 * MAYBE, PLACEHOLDERS and MANUAL should require explicit user review unless a
 * future command-line policy deliberately opts into them.
 */

/* ========================================================================= */
/* Backend remapping                                                         */
/* ========================================================================= */

/*
 * Suggestions originating from generated C diagnostics must never blindly
 * edit generated C when the user-facing source is Vitte.
 *
 * Correct flow:
 *
 *   generated C diagnostic
 *       |
 *       v
 *   source_map
 *       |
 *       v
 *   Vitte source span
 *       |
 *       v
 *   structured Vitte suggestion
 *
 * Only the remapped Vitte source range should become a normal user-facing
 * fix-it.
 */

/* ========================================================================= */
/* Testing                                                                   */
/* ========================================================================= */

/*
 * Recommended unit tests:
 *
 *   1. empty suggestion
 *   2. explanation-only suggestion
 *   3. insertion
 *   4. deletion
 *   5. replacement
 *   6. machine applicability
 *   7. maybe applicability
 *   8. placeholders applicability
 *   9. manual applicability
 *  10. invalid span
 *  11. edit without span
 *  12. exact duplicate
 *  13. deduplication
 *  14. overlapping edits
 *  15. adjacent non-overlapping edits
 *  16. same-point insertions
 *  17. different source files
 *  18. source-name fallback
 *  19. remove first
 *  20. remove middle
 *  21. remove last
 *  22. pointer rebinding after memmove
 *  23. pointer rebinding after qsort
 *  24. deterministic sorting
 *  25. best suggestion
 *  26. machine-safe plan
 *  27. unsafe mixed-applicability plan
 *  28. conflicting fix plan
 *  29. reverse application order
 *  30. fingerprint determinism
 *  31. maximum capacity
 *  32. empty replacement
 *  33. long message truncation
 *  34. long replacement truncation
 *  35. Unicode replacement
 *  36. UTF-8 byte offsets
 *  37. ASan
 *  38. UBSan
 *  39. fuzzing
 */

/* ========================================================================= */
/* Fuzzing                                                                   */
/* ========================================================================= */

/*
 * Fuzz:
 *
 *   spans
 *   source IDs
 *   source names
 *   applicability values
 *   replacement lengths
 *   messages
 *   duplicate suggestions
 *   overlapping suggestions
 *   zero-width edits
 *
 * Invariants:
 *
 *   - no out-of-bounds writes;
 *   - no dangling inline-storage pointers after moves;
 *   - no automatic plan marked safe when edits conflict;
 *   - deterministic sorting;
 *   - deterministic fingerprints;
 *   - count never exceeds VITTE_DIAGNOSTIC_MAX_SUGGESTIONS.
 */

/* ========================================================================= */
/* Future extensions                                                         */
/* ========================================================================= */

/*
 * Potential extensions:
 *
 *   - multi-edit suggestion groups;
 *   - atomic fixes spanning multiple files;
 *   - stable suggestion IDs;
 *   - diagnostic IDs;
 *   - source revision IDs;
 *   - expected-source-text guards;
 *   - edit preconditions;
 *   - semantic fix categories;
 *   - code-action kinds;
 *   - preferred-fix marker;
 *   - command-backed LSP actions;
 *   - transactional `vitte fix`;
 *   - source-map-aware fix remapping;
 *   - conflict graph;
 *   - fix dependency graph;
 *   - formatting-after-fix hooks.
 *
 * The current API deliberately establishes the core invariants needed for
 * these features without coupling suggestions to filesystem mutation.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_SUGGESTION_H */
