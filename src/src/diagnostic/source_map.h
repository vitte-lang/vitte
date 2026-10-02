#ifndef VITTE_DIAGNOSTIC_SOURCE_MAP_H
#define VITTE_DIAGNOSTIC_SOURCE_MAP_H

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

/*
 * Sentinel used whenever no source-map entry/index exists.
 */
#ifndef VITTE_SOURCE_MAP_INDEX_NONE
#define VITTE_SOURCE_MAP_INDEX_NONE SIZE_MAX
#endif

/*
 * Maximum number of provenance steps retained by vitte_source_map_trace_t.
 *
 * The implementation may protect itself with an additional traversal-depth
 * limit. This capacity controls the amount of the trace exposed to callers.
 */
#ifndef VITTE_SOURCE_MAP_TRACE_CAPACITY
#define VITTE_SOURCE_MAP_TRACE_CAPACITY ((size_t)64u)
#endif

/* ========================================================================= */
/* Source-map model                                                          */
/* ========================================================================= */

/*
 * Source maps preserve provenance across compiler transformations.
 *
 * Typical Vitte pipeline:
 *
 *   .vit source
 *       |
 *       v
 *   AST
 *       |
 *       v
 *   semantic analysis
 *       |
 *       v
 *   HIR
 *       |
 *       v
 *   IR
 *       |
 *       v
 *   C17 lowering
 *       |
 *       v
 *   generated C
 *       |
 *       v
 *   external C compiler diagnostic
 *       |
 *       v
 *   source-map remapping
 *       |
 *       v
 *   original .vit diagnostic
 *
 * A source-map entry represents:
 *
 *   generated span -> originating span
 *
 * Source maps may also be chained:
 *
 *   generated C
 *       -> IR provenance
 *       -> HIR provenance
 *       -> AST/source provenance
 *
 * vitte_source_map_remap_to_root() follows this chain.
 */

/* ========================================================================= */
/* Mapping kinds                                                             */
/* ========================================================================= */

/*
 * Ordering matters.
 *
 * Lower enum values represent stronger/more precise provenance. The source-map
 * implementation uses this ordering as a deterministic tie-break when several
 * mappings have the same generated-span specificity.
 */
typedef enum vitte_source_map_kind {
    /*
     * Exact textual/source correspondence.
     *
     * Prefer this whenever generated and original ranges represent the same
     * semantic/textual fragment.
     */
    VITTE_SOURCE_MAP_EXACT = 0,

    /*
     * Token-level provenance.
     *
     * Example:
     *
     *   generated identifier -> original Vitte identifier
     */
    VITTE_SOURCE_MAP_TOKEN,

    /*
     * Expression-level provenance.
     */
    VITTE_SOURCE_MAP_EXPRESSION,

    /*
     * Statement-level provenance.
     */
    VITTE_SOURCE_MAP_STATEMENT,

    /*
     * Declaration-level provenance.
     */
    VITTE_SOURCE_MAP_DECLARATION,

    /*
     * Synthesized compiler construct associated with an original source
     * location.
     *
     * The generated text itself does not have a direct textual equivalent.
     */
    VITTE_SOURCE_MAP_SYNTHETIC,

    /*
     * Mapping produced by semantic/HIR/IR/backend lowering.
     */
    VITTE_SOURCE_MAP_LOWERED,

    /*
     * Broad generated-code provenance.
     *
     * This is intentionally the weakest standard mapping kind and is useful as
     * a fallback when no token/expression/statement mapping exists.
     */
    VITTE_SOURCE_MAP_GENERATED
} vitte_source_map_kind_t;

/* ========================================================================= */
/* Entry                                                                     */
/* ========================================================================= */

typedef struct vitte_source_map_entry {
    /*
     * Provenance precision/category.
     */
    vitte_source_map_kind_t kind;

    /*
     * Span in the generated/transformed representation.
     */
    vitte_ast_span_t generated;

    /*
     * Span responsible for generating it.
     */
    vitte_ast_span_t original;
} vitte_source_map_entry_t;

/* ========================================================================= */
/* Source map                                                                */
/* ========================================================================= */

typedef struct vitte_source_map {
    /*
     * Owned dynamic array.
     */
    vitte_source_map_entry_t *entries;

    /*
     * Number of valid entries.
     */
    size_t count;

    /*
     * Allocated entry capacity.
     */
    size_t capacity;

    /*
     * True when entries are in deterministic canonical order and exact
     * duplicates have been removed.
     */
    bool normalized;
} vitte_source_map_t;

/* ========================================================================= */
/* Remapped position                                                         */
/* ========================================================================= */

typedef struct vitte_source_map_position {
    /*
     * Original/root source.
     */
    vitte_source_id_t source_id;
    const char *source_name;

    /*
     * Remapped byte offset.
     *
     * This is intentionally byte-oriented. Exact line/column resolution should
     * be performed through vitte_source_manager_t.
     */
    size_t offset;

    /*
     * Mapping responsible for this result.
     */
    vitte_source_map_kind_t mapping_kind;

    /*
     * False when no mapping was found.
     */
    bool mapped;
} vitte_source_map_position_t;

/* ========================================================================= */
/* Trace                                                                     */
/* ========================================================================= */

typedef struct vitte_source_map_trace_step {
    vitte_source_map_kind_t kind;

    /*
     * Span before this provenance step.
     */
    vitte_ast_span_t generated;

    /*
     * Span after this provenance step.
     */
    vitte_ast_span_t original;
} vitte_source_map_trace_step_t;

typedef struct vitte_source_map_trace {
    vitte_source_map_trace_step_t
        steps[VITTE_SOURCE_MAP_TRACE_CAPACITY];

    /*
     * Number of retained steps.
     *
     * This never exceeds VITTE_SOURCE_MAP_TRACE_CAPACITY.
     */
    size_t count;

    /*
     * True when traversal exceeded the implementation's safe provenance depth
     * or the public trace capacity.
     */
    bool truncated;

    /*
     * Last source span reached by traversal.
     */
    vitte_ast_span_t final_span;

    bool has_final_span;
} vitte_source_map_trace_t;

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

typedef struct vitte_source_map_validation {
    /*
     * Overall structural validity.
     */
    bool valid;

    /*
     * Number of source-map entries inspected.
     */
    size_t entry_count;

    /*
     * Number of structurally invalid entries.
     */
    size_t invalid_entry_count;

    /*
     * Number of exact duplicate mappings.
     */
    size_t duplicate_count;

    /*
     * First structurally invalid entry.
     *
     * VITTE_SOURCE_MAP_INDEX_NONE when none exists.
     */
    size_t first_invalid_index;
} vitte_source_map_validation_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_source_map_statistics {
    size_t entry_count;
    size_t capacity;

    /*
     * Sum of generated span lengths.
     */
    size_t generated_bytes;

    /*
     * Sum of original span lengths.
     */
    size_t original_bytes;

    /*
     * Number of mappings whose generated/original byte lengths are identical.
     */
    size_t exact_length_count;

    /*
     * Number of mappings where lowering changed the byte length.
     */
    size_t transformed_length_count;

    /*
     * Number of point/zero-width generated mappings.
     */
    size_t zero_width_count;

    bool normalized;
} vitte_source_map_statistics_t;

/* ========================================================================= */
/* Visitor                                                                   */
/* ========================================================================= */

/*
 * Return true to continue traversal.
 * Return false to stop traversal successfully.
 */
typedef bool
(*vitte_source_map_visitor_t)(
    const vitte_source_map_entry_t *entry,
    size_t index,
    void *user_data
);

/* ========================================================================= */
/* Lifetime                                                                  */
/* ========================================================================= */

/*
 * Initialize an empty source map.
 *
 * Safe state:
 *
 *   entries    = NULL
 *   count      = 0
 *   capacity   = 0
 *   normalized = true
 */
void
vitte_source_map_init(
    vitte_source_map_t *map
);

/*
 * Release owned entry storage.
 *
 * Does not own strings or source-manager data referenced by spans.
 */
void
vitte_source_map_destroy(
    vitte_source_map_t *map
);

/*
 * Remove every mapping while retaining allocated capacity.
 */
void
vitte_source_map_clear(
    vitte_source_map_t *map
);

/* ========================================================================= */
/* Basic queries                                                             */
/* ========================================================================= */

size_t
vitte_source_map_count(
    const vitte_source_map_t *map
);

bool
vitte_source_map_empty(
    const vitte_source_map_t *map
);

const vitte_source_map_entry_t *
vitte_source_map_at(
    const vitte_source_map_t *map,
    size_t index
);

/*
 * Mutable access invalidates canonical-normalized state because the caller may
 * alter ordering keys.
 */
vitte_source_map_entry_t *
vitte_source_map_at_mut(
    vitte_source_map_t *map,
    size_t index
);

/* ========================================================================= */
/* Add                                                                       */
/* ========================================================================= */

/*
 * Add a complete source-map entry.
 *
 * Exact duplicates are ignored.
 */
vitte_status_t
vitte_source_map_add_entry(
    vitte_source_map_t *map,
    const vitte_source_map_entry_t *entry
);

/*
 * Convenience entry creation.
 */
vitte_status_t
vitte_source_map_add(
    vitte_source_map_t *map,
    vitte_source_map_kind_t kind,
    const vitte_ast_span_t *generated,
    const vitte_ast_span_t *original
);

/* ========================================================================= */
/* Remove                                                                    */
/* ========================================================================= */

vitte_status_t
vitte_source_map_remove(
    vitte_source_map_t *map,
    size_t index
);

/* ========================================================================= */
/* Normalization                                                             */
/* ========================================================================= */

/*
 * Canonicalize source-map storage.
 *
 * Effects:
 *
 *   - deterministic ordering;
 *   - exact duplicate removal;
 *   - normalized = true.
 *
 * Canonical ordering is primarily:
 *
 *   generated source
 *   generated start
 *   generated end
 *   original source
 *   original start
 *   original end
 *   mapping kind
 */
vitte_status_t
vitte_source_map_normalize(
    vitte_source_map_t *map
);

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

/*
 * Find the most specific mapping containing a generated byte offset.
 *
 * Source identity resolution:
 *
 *   1. source_id when both IDs are valid;
 *   2. source_name fallback.
 *
 * Selection prefers the smallest generated span.
 */
const vitte_source_map_entry_t *
vitte_source_map_find_offset(
    const vitte_source_map_t *map,
    vitte_source_id_t generated_source_id,
    const char *generated_source_name,
    size_t generated_offset
);

/*
 * Find the most specific mapping for a generated span.
 *
 * Selection:
 *
 *   1. containing mappings;
 *   2. overlapping mappings as fallback;
 *   3. smallest generated mapping;
 *   4. strongest mapping kind as deterministic tie-break.
 *
 * Overlap fallback is useful when an external C compiler reports a span that
 * does not align exactly with backend token boundaries.
 */
const vitte_source_map_entry_t *
vitte_source_map_find_span(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *generated_span
);

/*
 * Reverse lookup from an original source byte offset to a mapping that
 * generated code from that source location.
 *
 * Useful for:
 *
 *   debugging;
 *   provenance inspection;
 *   editor tooling;
 *   generated-code visualization.
 */
const vitte_source_map_entry_t *
vitte_source_map_find_original_offset(
    const vitte_source_map_t *map,
    vitte_source_id_t original_source_id,
    const char *original_source_name,
    size_t original_offset
);

/* ========================================================================= */
/* Fast presence queries                                                     */
/* ========================================================================= */

bool
vitte_source_map_has_mapping_for_span(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *span
);

bool
vitte_source_map_has_mapping_for_offset(
    const vitte_source_map_t *map,
    vitte_source_id_t source_id,
    const char *source_name,
    size_t offset
);

/* ========================================================================= */
/* Remapping                                                                 */
/* ========================================================================= */

/*
 * Remap one generated byte offset.
 *
 * For 1:1 generated/original mappings:
 *
 *   original_offset =
 *       original.start +
 *       (generated_offset - generated.start)
 *
 * For non-1:1 transformations, the implementation performs a best-effort
 * projection.
 *
 * Semantic backend mappings should be sufficiently fine-grained that
 * proportional projection remains a fallback rather than the primary
 * provenance mechanism.
 */
bool
vitte_source_map_remap_offset(
    const vitte_source_map_t *map,
    vitte_source_id_t generated_source_id,
    const char *generated_source_name,
    size_t generated_offset,
    vitte_source_map_position_t *result
);

/*
 * Remap a generated span through the best available mapping.
 *
 * The resulting source_id/source_name come from the original mapping.
 *
 * Byte offsets are projected.
 *
 * Exact projected line/column coordinates should subsequently be resolved
 * through vitte_source_manager_t.
 */
bool
vitte_source_map_remap_span(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *generated_span,
    vitte_ast_span_t *original_span
);

/*
 * Return the complete original span stored by the best matching mapping
 * without projecting the requested generated subspan.
 */
bool
vitte_source_map_original_span(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *generated_span,
    vitte_ast_span_t *original_span
);

/* ========================================================================= */
/* Provenance chains                                                         */
/* ========================================================================= */

/*
 * Trace provenance repeatedly:
 *
 *   generated
 *       -> original
 *       -> previous original
 *       -> ...
 *
 * Example:
 *
 *   generated C expression
 *       -> IR expression
 *       -> HIR expression
 *       -> original .vit expression
 *
 * Returns traversal depth.
 */
size_t
vitte_source_map_trace(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *start,
    vitte_source_map_trace_t *trace
);

/*
 * Follow mappings until no further mapping exists.
 *
 * Returns true when at least one remapping occurred.
 */
bool
vitte_source_map_remap_to_root(
    const vitte_source_map_t *map,
    const vitte_ast_span_t *generated_span,
    vitte_ast_span_t *root_span
);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

vitte_source_map_validation_t
vitte_source_map_validate(
    const vitte_source_map_t *map
);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_source_map_statistics_t
vitte_source_map_statistics(
    const vitte_source_map_t *map
);

/* ========================================================================= */
/* Fingerprints                                                              */
/* ========================================================================= */

/*
 * Deterministic fingerprint for one mapping.
 *
 * Intended for:
 *
 *   diagnostics;
 *   debugging;
 *   cache validation;
 *   tests.
 *
 * Not a cryptographic hash.
 */
uint64_t
vitte_source_map_entry_fingerprint(
    const vitte_source_map_entry_t *entry
);

/*
 * Fingerprint of source-map storage in current order.
 *
 * For a canonical fingerprint independent of insertion order, normalize the
 * map first.
 */
uint64_t
vitte_source_map_fingerprint(
    const vitte_source_map_t *map
);

/* ========================================================================= */
/* Mapping-kind metadata                                                     */
/* ========================================================================= */

const char *
vitte_source_map_kind_name(
    vitte_source_map_kind_t kind
);

/* ========================================================================= */
/* Merge                                                                     */
/* ========================================================================= */

/*
 * Merge mappings from source into destination.
 *
 * Exact duplicates are ignored.
 *
 * Destination becomes non-normalized when new mappings are inserted.
 */
vitte_status_t
vitte_source_map_merge(
    vitte_source_map_t *destination,
    const vitte_source_map_t *source
);

/* ========================================================================= */
/* Copy                                                                      */
/* ========================================================================= */

/*
 * Deep-copy the owned entry array.
 *
 * Span string/source references remain borrowed exactly as in the source map.
 */
vitte_status_t
vitte_source_map_copy(
    vitte_source_map_t *destination,
    const vitte_source_map_t *source
);

/* ========================================================================= */
/* Swap                                                                      */
/* ========================================================================= */

void
vitte_source_map_swap(
    vitte_source_map_t *left,
    vitte_source_map_t *right
);

/* ========================================================================= */
/* Source statistics                                                        */
/* ========================================================================= */

size_t
vitte_source_map_generated_source_count(
    const vitte_source_map_t *map
);

size_t
vitte_source_map_original_source_count(
    const vitte_source_map_t *map
);

/* ========================================================================= */
/* Visitor                                                                   */
/* ========================================================================= */

vitte_status_t
vitte_source_map_visit(
    const vitte_source_map_t *map,
    vitte_source_map_visitor_t visitor,
    void *user_data
);

/* ========================================================================= */
/* Storage                                                                   */
/* ========================================================================= */

/*
 * Reduce allocated storage to exactly count entries.
 */
vitte_status_t
vitte_source_map_shrink_to_fit(
    vitte_source_map_t *map
);

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

const char *
vitte_source_map_component_name(void);

size_t
vitte_source_map_max_trace_depth(void);

/* ========================================================================= */
/* Convenience helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_source_map_is_normalized(
    const vitte_source_map_t *map
)
{
    return map != NULL &&
           map->normalized;
}

static inline size_t
vitte_source_map_capacity(
    const vitte_source_map_t *map
)
{
    return map != NULL
        ? map->capacity
        : 0u;
}

static inline void
vitte_source_map_mark_dirty(
    vitte_source_map_t *map
)
{
    if (map == NULL) {
        return;
    }

    map->normalized = false;
}

/* ========================================================================= */
/* Specialized add helpers                                                   */
/* ========================================================================= */

static inline vitte_status_t
vitte_source_map_add_exact(
    vitte_source_map_t *map,
    const vitte_ast_span_t *generated,
    const vitte_ast_span_t *original
)
{
    return vitte_source_map_add(
        map,
        VITTE_SOURCE_MAP_EXACT,
        generated,
        original
    );
}

static inline vitte_status_t
vitte_source_map_add_token(
    vitte_source_map_t *map,
    const vitte_ast_span_t *generated,
    const vitte_ast_span_t *original
)
{
    return vitte_source_map_add(
        map,
        VITTE_SOURCE_MAP_TOKEN,
        generated,
        original
    );
}

static inline vitte_status_t
vitte_source_map_add_expression(
    vitte_source_map_t *map,
    const vitte_ast_span_t *generated,
    const vitte_ast_span_t *original
)
{
    return vitte_source_map_add(
        map,
        VITTE_SOURCE_MAP_EXPRESSION,
        generated,
        original
    );
}

static inline vitte_status_t
vitte_source_map_add_statement(
    vitte_source_map_t *map,
    const vitte_ast_span_t *generated,
    const vitte_ast_span_t *original
)
{
    return vitte_source_map_add(
        map,
        VITTE_SOURCE_MAP_STATEMENT,
        generated,
        original
    );
}

static inline vitte_status_t
vitte_source_map_add_declaration(
    vitte_source_map_t *map,
    const vitte_ast_span_t *generated,
    const vitte_ast_span_t *original
)
{
    return vitte_source_map_add(
        map,
        VITTE_SOURCE_MAP_DECLARATION,
        generated,
        original
    );
}

static inline vitte_status_t
vitte_source_map_add_synthetic(
    vitte_source_map_t *map,
    const vitte_ast_span_t *generated,
    const vitte_ast_span_t *original
)
{
    return vitte_source_map_add(
        map,
        VITTE_SOURCE_MAP_SYNTHETIC,
        generated,
        original
    );
}

static inline vitte_status_t
vitte_source_map_add_lowered(
    vitte_source_map_t *map,
    const vitte_ast_span_t *generated,
    const vitte_ast_span_t *original
)
{
    return vitte_source_map_add(
        map,
        VITTE_SOURCE_MAP_LOWERED,
        generated,
        original
    );
}

static inline vitte_status_t
vitte_source_map_add_generated(
    vitte_source_map_t *map,
    const vitte_ast_span_t *generated,
    const vitte_ast_span_t *original
)
{
    return vitte_source_map_add(
        map,
        VITTE_SOURCE_MAP_GENERATED,
        generated,
        original
    );
}

/* ========================================================================= */
/* Backend integration                                                       */
/* ========================================================================= */

/*
 * Recommended C17 backend flow:
 *
 *   1. Backend begins emitting generated C.
 *
 *   2. Record generated start offset.
 *
 *   3. Emit one token/expression/statement/declaration.
 *
 *   4. Record generated end offset.
 *
 *   5. Construct generated span.
 *
 *   6. Associate generated span with the Vitte provenance span:
 *
 *          vitte_source_map_add_expression(...)
 *
 *   7. External C compiler emits:
 *
 *          generated.c:line:column
 *
 *   8. Resolve C line/column -> generated byte offset.
 *
 *   9. Call:
 *
 *          vitte_source_map_remap_offset(...)
 *
 *      or:
 *
 *          vitte_source_map_remap_span(...)
 *
 *  10. Resolve remapped Vitte byte offsets through the Vitte source manager.
 *
 *  11. Emit a normal structured Vitte diagnostic.
 *
 * Raw generated-C locations should therefore not normally reach end users.
 */

/* ========================================================================= */
/* Mapping granularity                                                       */
/* ========================================================================= */

/*
 * Prefer fine-grained mappings.
 *
 * Better:
 *
 *   identifier -> identifier
 *   argument   -> argument
 *   expression -> expression
 *
 * rather than only:
 *
 *   generated translation unit -> original module
 *
 * Fine-grained mappings significantly improve external compiler diagnostic
 * remapping.
 *
 * Broad mappings remain useful as fallback provenance.
 */

/* ========================================================================= */
/* Half-open spans                                                           */
/* ========================================================================= */

/*
 * Source-map ranges use the compiler span convention:
 *
 *   [start_offset, end_offset)
 *
 * Example:
 *
 *   start = 10
 *   end   = 14
 *
 * represents bytes:
 *
 *   10, 11, 12, 13
 *
 * Zero-width mappings:
 *
 *   start == end
 *
 * represent point/insertion provenance.
 */

/* ========================================================================= */
/* Non-1:1 transformations                                                   */
/* ========================================================================= */

/*
 * Generated and original text lengths may differ.
 *
 * Example:
 *
 * Vitte:
 *
 *   give value;
 *
 * Generated C:
 *
 *   return value;
 *
 * A whole-statement mapping is therefore not necessarily byte-for-byte.
 *
 * source_map.c can perform proportional best-effort projection, but the
 * preferred solution is to additionally create fine-grained mappings:
 *
 *   generated "value"
 *       -> original "value"
 *
 * This gives exact provenance for diagnostics targeting the expression.
 */

/* ========================================================================= */
/* Line and column policy                                                    */
/* ========================================================================= */

/*
 * Source-map identity is fundamentally byte-offset based.
 *
 * Do not make line/column the canonical mapping identity.
 *
 * Reasons:
 *
 *   - UTF-8;
 *   - tabs;
 *   - generated transformations;
 *   - source edits;
 *   - display-width differences;
 *   - Unicode grapheme complexity.
 *
 * Recommended model:
 *
 *   source_id + byte offsets
 *       |
 *       v
 *   source manager
 *       |
 *       v
 *   line / column / display column
 */

/* ========================================================================= */
/* Chained provenance                                                        */
/* ========================================================================= */

/*
 * A single source map may contain several transformation layers.
 *
 * Example:
 *
 *   C17 generated span
 *       |
 *       v
 *   IR span
 *       |
 *       v
 *   HIR span
 *       |
 *       v
 *   AST / .vit span
 *
 * vitte_source_map_trace() exposes this provenance chain.
 *
 * vitte_source_map_remap_to_root() resolves to the final reachable source.
 */

/* ========================================================================= */
/* Cycle protection                                                          */
/* ========================================================================= */

/*
 * Provenance must conceptually be acyclic.
 *
 * Nevertheless, corrupted compiler state must not cause infinite traversal.
 *
 * source_map.c therefore limits traversal depth and stops on trivial
 * self-mappings.
 *
 * A future debug validation mode may additionally detect arbitrary cycles and
 * report an internal compiler diagnostic.
 */

/* ========================================================================= */
/* Diagnostic integration                                                    */
/* ========================================================================= */

/*
 * Backend diagnostics should conceptually follow:
 *
 *   external diagnostic
 *       |
 *       v
 *   generated C span
 *       |
 *       v
 *   source map
 *       |
 *       v
 *   root Vitte span
 *       |
 *       v
 *   vitte_diagnostic_t
 *       |
 *       +-- terminal
 *       +-- JSON
 *       +-- SARIF
 *       +-- LSP
 *
 * The source map itself does not render diagnostics.
 */

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

/*
 * vitte_source_map_t owns:
 *
 *   entries
 *
 * It does not necessarily own:
 *
 *   span.source_name
 *   source-manager objects
 *   source buffers
 *
 * Those references must remain valid for the lifetime of the source map unless
 * the canonical span representation later changes to fully owned identifiers.
 */

/* ========================================================================= */
/* Mutation and pointer stability                                            */
/* ========================================================================= */

/*
 * Pointers returned by:
 *
 *   vitte_source_map_at()
 *   vitte_source_map_at_mut()
 *   vitte_source_map_find_offset()
 *   vitte_source_map_find_span()
 *   vitte_source_map_find_original_offset()
 *
 * are borrowed pointers into map->entries.
 *
 * They may be invalidated by:
 *
 *   vitte_source_map_add()
 *   vitte_source_map_add_entry()
 *   vitte_source_map_merge()
 *   vitte_source_map_normalize()
 *   vitte_source_map_remove()
 *   vitte_source_map_shrink_to_fit()
 *   vitte_source_map_destroy()
 *
 * Do not retain them across mutations.
 */

/* ========================================================================= */
/* Determinism                                                               */
/* ========================================================================= */

/*
 * Normalize maps before:
 *
 *   deterministic serialization;
 *   stable golden tests;
 *   canonical fingerprints;
 *   persistent caches.
 *
 * Insertion order is otherwise preserved only until normalization.
 */

/* ========================================================================= */
/* Complexity                                                                */
/* ========================================================================= */

/*
 * The current implementation prioritizes correctness and deterministic
 * behavior.
 *
 * Some lookup/count operations are linear, and duplicate/source validation may
 * be quadratic.
 *
 * This is acceptable for the initial diagnostic provenance implementation.
 *
 * If backend source maps become very large, the internal representation can
 * later add:
 *
 *   per-source indexes;
 *   binary search;
 *   interval trees;
 *   immutable finalized maps;
 *
 * without changing this public semantic model.
 */

/* ========================================================================= */
/* Testing                                                                   */
/* ========================================================================= */

/*
 * Recommended tests:
 *
 *   1. empty map
 *   2. one exact mapping
 *   3. token mapping
 *   4. expression mapping
 *   5. statement mapping
 *   6. declaration mapping
 *   7. synthetic mapping
 *   8. lowered mapping
 *   9. generated fallback mapping
 *  10. nested mappings
 *  11. most-specific mapping wins
 *  12. equal-size mapping-kind tie-break
 *  13. exact duplicate removal
 *  14. zero-width mapping
 *  15. half-open end boundary
 *  16. overlap fallback
 *  17. different source IDs
 *  18. source-name fallback
 *  19. 1:1 offset projection
 *  20. non-1:1 projection
 *  21. reverse lookup
 *  22. provenance trace
 *  23. root remapping
 *  24. self-cycle protection
 *  25. maximum-depth protection
 *  26. normalization determinism
 *  27. fingerprint determinism
 *  28. copy
 *  29. merge
 *  30. swap
 *  31. remove
 *  32. shrink-to-fit
 *  33. visitor early stop
 *  34. validation
 *  35. invalid span
 *  36. large offsets
 *  37. ASan
 *  38. UBSan
 *  39. fuzzing
 */

/* ========================================================================= */
/* Fuzzing                                                                   */
/* ========================================================================= */

/*
 * Source-map fuzzing should generate:
 *
 *   random source IDs
 *   random source names
 *   random offsets
 *   reversed ranges
 *   zero-width ranges
 *   overlapping ranges
 *   nested ranges
 *   duplicate ranges
 *   large provenance chains
 *
 * Core invariants:
 *
 *   - no out-of-bounds access;
 *   - no infinite loop;
 *   - no integer-wrap allocation;
 *   - deterministic normalized output;
 *   - remapped span end >= start;
 *   - failed lookup leaves no false mapped result.
 */

/* ========================================================================= */
/* Future evolution                                                         */
/* ========================================================================= */

/*
 * Potential future additions without changing the fundamental model:
 *
 *   - stable mapping IDs;
 *   - provenance IDs;
 *   - transformation/pass IDs;
 *   - macro expansion chains;
 *   - inline expansion chains;
 *   - generated-file registry;
 *   - source-map serialization;
 *   - compact binary source maps;
 *   - interval indexing;
 *   - backend-specific metadata;
 *   - exact token correspondence;
 *   - source-manager-native position resolution.
 *
 * In particular, stable provenance IDs can eventually connect:
 *
 *   AST
 *   HIR
 *   IR
 *   backend
 *   generated C
 *   diagnostics
 *
 * without relying exclusively on span matching.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_SOURCE_MAP_H */
