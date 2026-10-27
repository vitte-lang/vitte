#!/usr/bin/env python3

from pathlib import Path
import shutil
import time

PATH = Path(
    "/Users/vincent/Documents/Github/vitte/src/src/diagnostic/diagnostic.h"
)

if not PATH.is_file():
    raise SystemExit(f"Erreur: fichier introuvable: {PATH}")

source = PATH.read_text(encoding="utf-8")

backup = PATH.with_name(
    f"{PATH.name}.before-intelligent-diagnostic-header-{int(time.time())}"
)
shutil.copy2(PATH, backup)


def replace_once(old: str, new: str) -> None:
    global source

    count = source.count(old)

    if count != 1:
        raise RuntimeError(
            f"Patch ambigu: motif trouvé {count} fois au lieu de 1:\n"
            f"{old[:500]}"
        )

    source = source.replace(old, new, 1)


# ============================================================================
# 1. Additional capacities
# ============================================================================

replace_once(
"""#ifndef VITTE_DIAGNOSTIC_CONTRACT_REASON_CAPACITY
#define VITTE_DIAGNOSTIC_CONTRACT_REASON_CAPACITY 512u
#endif
""",
"""#ifndef VITTE_DIAGNOSTIC_CONTRACT_REASON_CAPACITY
#define VITTE_DIAGNOSTIC_CONTRACT_REASON_CAPACITY 512u
#endif

#ifndef VITTE_DIAGNOSTIC_EXPECTED_CAPACITY
#define VITTE_DIAGNOSTIC_EXPECTED_CAPACITY 512u
#endif

#ifndef VITTE_DIAGNOSTIC_FOUND_CAPACITY
#define VITTE_DIAGNOSTIC_FOUND_CAPACITY 512u
#endif

#ifndef VITTE_DIAGNOSTIC_CAUSE_CAPACITY
#define VITTE_DIAGNOSTIC_CAUSE_CAPACITY 1024u
#endif

#ifndef VITTE_DIAGNOSTIC_FIX_TITLE_CAPACITY
#define VITTE_DIAGNOSTIC_FIX_TITLE_CAPACITY 256u
#endif

#ifndef VITTE_DIAGNOSTIC_TOOL_CAPACITY
#define VITTE_DIAGNOSTIC_TOOL_CAPACITY 128u
#endif

#ifndef VITTE_DIAGNOSTIC_RULE_CAPACITY
#define VITTE_DIAGNOSTIC_RULE_CAPACITY 128u
#endif
"""
)


# ============================================================================
# 2. Larger collection limits
# ============================================================================

replace_once(
"""#ifndef VITTE_DIAGNOSTIC_MAX_SUGGESTIONS
#define VITTE_DIAGNOSTIC_MAX_SUGGESTIONS 8u
#endif
""",
"""#ifndef VITTE_DIAGNOSTIC_MAX_SUGGESTIONS
#define VITTE_DIAGNOSTIC_MAX_SUGGESTIONS 16u
#endif

#ifndef VITTE_DIAGNOSTIC_MAX_EXPECTED
#define VITTE_DIAGNOSTIC_MAX_EXPECTED 16u
#endif

#ifndef VITTE_DIAGNOSTIC_MAX_TRACE
#define VITTE_DIAGNOSTIC_MAX_TRACE 32u
#endif

#ifndef VITTE_DIAGNOSTIC_MAX_FIX_PARTS
#define VITTE_DIAGNOSTIC_MAX_FIX_PARTS 16u
#endif
"""
)


# ============================================================================
# 3. Add scanner/codegen origins while preserving ordering
# ============================================================================

replace_once(
"""    VITTE_DIAGNOSTIC_ORIGIN_IO,

    VITTE_DIAGNOSTIC_ORIGIN_LEXER,
    VITTE_DIAGNOSTIC_ORIGIN_PARSER,
""",
"""    VITTE_DIAGNOSTIC_ORIGIN_IO,

    VITTE_DIAGNOSTIC_ORIGIN_SCANNER,
    VITTE_DIAGNOSTIC_ORIGIN_LEXER,
    VITTE_DIAGNOSTIC_ORIGIN_PARSER,
"""
)

replace_once(
"""    VITTE_DIAGNOSTIC_ORIGIN_HIR,
    VITTE_DIAGNOSTIC_ORIGIN_IR,

    VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND,
""",
"""    VITTE_DIAGNOSTIC_ORIGIN_HIR,
    VITTE_DIAGNOSTIC_ORIGIN_IR,
    VITTE_DIAGNOSTIC_ORIGIN_CODEGEN,

    VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND,
"""
)


# ============================================================================
# 4. Applicability confidence model
# ============================================================================

replace_once(
"""typedef enum vitte_diagnostic_applicability {
    VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE = 0,
    VITTE_DIAGNOSTIC_APPLICABILITY_MAYBE,
    VITTE_DIAGNOSTIC_APPLICABILITY_PLACEHOLDERS,
    VITTE_DIAGNOSTIC_APPLICABILITY_MANUAL
} vitte_diagnostic_applicability_t;
""",
"""typedef enum vitte_diagnostic_applicability {
    /*
     * Safe for automatic application without user interpretation.
     */
    VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE = 0,

    /*
     * Compiler believes the edit is useful but semantic intent is not
     * sufficiently certain for unconditional automatic application.
     */
    VITTE_DIAGNOSTIC_APPLICABILITY_MAYBE,

    /*
     * Edit contains placeholders that require user input.
     */
    VITTE_DIAGNOSTIC_APPLICABILITY_PLACEHOLDERS,

    /*
     * Human review is required.
     */
    VITTE_DIAGNOSTIC_APPLICABILITY_MANUAL
} vitte_diagnostic_applicability_t;

typedef enum vitte_diagnostic_confidence {
    VITTE_DIAGNOSTIC_CONFIDENCE_NONE = 0,
    VITTE_DIAGNOSTIC_CONFIDENCE_LOW,
    VITTE_DIAGNOSTIC_CONFIDENCE_MEDIUM,
    VITTE_DIAGNOSTIC_CONFIDENCE_HIGH,
    VITTE_DIAGNOSTIC_CONFIDENCE_CERTAIN
} vitte_diagnostic_confidence_t;
"""
)


# ============================================================================
# 5. Cause classification
# ============================================================================

marker = """/* ========================================================================= */
/* Output format                                                             */
/* ========================================================================= */
"""

addition = """/* ========================================================================= */
/* Cause classification                                                      */
/* ========================================================================= */

typedef enum vitte_diagnostic_cause_kind {
    VITTE_DIAGNOSTIC_CAUSE_UNKNOWN = 0,
    VITTE_DIAGNOSTIC_CAUSE_SYNTAX,
    VITTE_DIAGNOSTIC_CAUSE_MISSING_TOKEN,
    VITTE_DIAGNOSTIC_CAUSE_UNEXPECTED_TOKEN,
    VITTE_DIAGNOSTIC_CAUSE_UNRESOLVED_NAME,
    VITTE_DIAGNOSTIC_CAUSE_TYPE_MISMATCH,
    VITTE_DIAGNOSTIC_CAUSE_ARITY_MISMATCH,
    VITTE_DIAGNOSTIC_CAUSE_CONSTRAINT,
    VITTE_DIAGNOSTIC_CAUSE_IMPORT,
    VITTE_DIAGNOSTIC_CAUSE_CONTRACT,
    VITTE_DIAGNOSTIC_CAUSE_CONSTANT_EVAL,
    VITTE_DIAGNOSTIC_CAUSE_UNSAFE_OPERATION,
    VITTE_DIAGNOSTIC_CAUSE_LOWERING,
    VITTE_DIAGNOSTIC_CAUSE_EXTERNAL_TOOL,
    VITTE_DIAGNOSTIC_CAUSE_LINK,
    VITTE_DIAGNOSTIC_CAUSE_RUNTIME,
    VITTE_DIAGNOSTIC_CAUSE_INTERNAL
} vitte_diagnostic_cause_kind_t;

/* ========================================================================= */
/* Trace                                                                     */
/* ========================================================================= */

typedef enum vitte_diagnostic_trace_kind {
    VITTE_DIAGNOSTIC_TRACE_CALL = 0,
    VITTE_DIAGNOSTIC_TRACE_INSTANTIATION,
    VITTE_DIAGNOSTIC_TRACE_CONSTRAINT,
    VITTE_DIAGNOSTIC_TRACE_IMPORT,
    VITTE_DIAGNOSTIC_TRACE_EXPANSION,
    VITTE_DIAGNOSTIC_TRACE_LOWERING,
    VITTE_DIAGNOSTIC_TRACE_GENERATED_CODE,
    VITTE_DIAGNOSTIC_TRACE_RELATED
} vitte_diagnostic_trace_kind_t;

"""

if marker not in source:
    raise RuntimeError("Section Output format introuvable")

source = source.replace(marker, addition + marker, 1)


# ============================================================================
# 6. Cause gains kind/confidence
# ============================================================================

replace_once(
"""typedef struct vitte_diagnostic_cause {
    const char *message;

    char message_storage[
        VITTE_DIAGNOSTIC_MESSAGE_CAPACITY
    ];

    vitte_ast_span_t span;

    bool has_span;
} vitte_diagnostic_cause_t;
""",
"""typedef struct vitte_diagnostic_cause {
    vitte_diagnostic_cause_kind_t kind;

    const char *message;

    char message_storage[
        VITTE_DIAGNOSTIC_CAUSE_CAPACITY
    ];

    vitte_ast_span_t span;

    bool has_span;

    /*
     * Confidence is informational. It must never be used as a substitute
     * for semantic proof when deciding whether a fix is machine applicable.
     */
    vitte_diagnostic_confidence_t confidence;
} vitte_diagnostic_cause_t;
"""
)


# ============================================================================
# 7. Suggestion gains confidence/id/preferred
# ============================================================================

replace_once(
"""    bool has_replacement;

    vitte_diagnostic_applicability_t applicability;
} vitte_diagnostic_suggestion_t;
""",
"""    bool has_replacement;

    vitte_diagnostic_applicability_t applicability;
    vitte_diagnostic_confidence_t confidence;

    /*
     * preferred is suitable for LSP CodeAction.isPreferred.
     */
    bool preferred;

    /*
     * Stable within one diagnostic. It is not a globally persistent ID.
     */
    uint32_t suggestion_id;
} vitte_diagnostic_suggestion_t;
"""
)


# ============================================================================
# 8. Add causal trace structure
# ============================================================================

marker = """/* ========================================================================= */
/* Contract information                                                      */
/* ========================================================================= */
"""

addition = """/* ========================================================================= */
/* Causal trace                                                              */
/* ========================================================================= */

typedef struct vitte_diagnostic_trace {
    vitte_diagnostic_trace_kind_t kind;

    vitte_ast_span_t span;
    bool has_span;

    const char *message;

    char message_storage[
        VITTE_DIAGNOSTIC_MESSAGE_CAPACITY
    ];
} vitte_diagnostic_trace_t;

"""

source = source.replace(marker, addition + marker, 1)


# ============================================================================
# 9. Rendering options expanded
# ============================================================================

replace_once(
"""    bool show_causes;
    bool show_suggestions;

    const vitte_source_manager_t *sources;
""",
"""    bool show_causes;
    bool show_suggestions;

    bool show_expected_found;
    bool show_related;
    bool show_trace;
    bool show_suppressed;
    bool show_summary;

    /*
     * Number of source context lines requested before/after a primary span.
     * Renderers may clamp this value to implementation-specific limits.
     */
    size_t context_lines_before;
    size_t context_lines_after;

    const vitte_source_manager_t *sources;
"""
)


# ============================================================================
# 10. Counts gains primary/cascade
# ============================================================================

replace_once(
"""    size_t suppressed_count;
} vitte_diagnostic_counts_t;
""",
"""    size_t suppressed_count;

    size_t primary_count;
    size_t cascade_count;
} vitte_diagnostic_counts_t;
"""
)


# ============================================================================
# 11. Diagnostic identity IDs
# ============================================================================

replace_once(
"""    uint64_t fingerprint;

    /* --------------------------------------------------------------------- */
    /* Main text                                                             */
""",
"""    uint64_t fingerprint;

    /*
     * Stable logical IDs for causal/provenance-aware engines.
     *
     * zero means "not assigned".
     */
    uint64_t diagnostic_id;
    uint64_t root_id;
    uint64_t parent_id;
    uint64_t provenance_id;
    uint64_t poison_id;

    /* --------------------------------------------------------------------- */
    /* Main text                                                             */
"""
)


# ============================================================================
# 12. Structured expected/found/cause
# ============================================================================

replace_once(
"""    const char *symbol;
    const char *expected_type;
    const char *actual_type;
    const char *context;
""",
"""    const char *symbol;
    const char *expected_type;
    const char *actual_type;

    /*
     * expected/found are generic human-readable semantic values.
     * expected_type/actual_type remain dedicated type-system fields.
     *
     * Examples:
     *
     *   expected = "`;`"
     *   found    = "`}`"
     *
     *   expected = "2 arguments"
     *   found    = "3 arguments"
     */
    const char *expected;
    const char *found;

    const char *probable_cause;
    const char *context;
"""
)

replace_once(
"""    char actual_type_storage[
        VITTE_DIAGNOSTIC_SUBJECT_CAPACITY
    ];

    char context_storage[
""",
"""    char actual_type_storage[
        VITTE_DIAGNOSTIC_SUBJECT_CAPACITY
    ];

    char expected_storage[
        VITTE_DIAGNOSTIC_EXPECTED_CAPACITY
    ];

    char found_storage[
        VITTE_DIAGNOSTIC_FOUND_CAPACITY
    ];

    char probable_cause_storage[
        VITTE_DIAGNOSTIC_CAUSE_CAPACITY
    ];

    char context_storage[
"""
)


# ============================================================================
# 13. Add causal trace to diagnostic
# ============================================================================

replace_once(
"""    size_t cause_count;

    /* --------------------------------------------------------------------- */
    /* Notes / help                                                          */
""",
"""    size_t cause_count;

    /* --------------------------------------------------------------------- */
    /* Causal trace                                                          */
    /* --------------------------------------------------------------------- */

    vitte_diagnostic_trace_t trace[
        VITTE_DIAGNOSTIC_MAX_TRACE
    ];

    size_t trace_count;

    /* --------------------------------------------------------------------- */
    /* Notes / help                                                          */
"""
)


# ============================================================================
# 14. Diagnostic flags
# ============================================================================

replace_once(
"""    bool is_primary;
    bool is_cascade;
    bool is_suppressed;
} vitte_diagnostic_t;
""",
"""    bool is_primary;
    bool is_cascade;
    bool is_suppressed;

    /*
     * Set when later phases should treat the affected semantic value as
     * poisoned/error-valued and avoid emitting redundant diagnostics.
     */
    bool poisons_analysis;

    /*
     * Marks diagnostics originating from compiler bugs rather than invalid
     * user programs.
     */
    bool is_internal;

    /*
     * True when at least one suggestion can be exposed as an automatic
     * machine-readable edit.
     */
    bool has_machine_fix;
} vitte_diagnostic_t;
"""
)


# ============================================================================
# 15. Bag gets sequence generation and saturation state
# ============================================================================

replace_once(
"""    vitte_error_t last_error;
} vitte_diagnostic_bag_t;
""",
"""    vitte_error_t last_error;

    /*
     * Monotonic ID generator. Zero is reserved for "unassigned".
     */
    uint64_t next_diagnostic_id;

    /*
     * True after the configured diagnostic limit is reached.
     */
    bool limit_reached;
} vitte_diagnostic_bag_t;
"""
)


# ============================================================================
# 16. New metadata API
# ============================================================================

replace_once(
"""const char *
vitte_diagnostic_format_name(
    vitte_diagnostic_format_t format
);
""",
"""const char *
vitte_diagnostic_format_name(
    vitte_diagnostic_format_t format
);

/*
 * These helpers are intentionally declared as part of the public model even
 * if their implementation lives in engine/suggestion/cascade modules.
 */
const char *
vitte_diagnostic_confidence_name(
    vitte_diagnostic_confidence_t confidence
);

const char *
vitte_diagnostic_applicability_name(
    vitte_diagnostic_applicability_t applicability
);

const char *
vitte_diagnostic_cause_kind_name(
    vitte_diagnostic_cause_kind_t kind
);

const char *
vitte_diagnostic_trace_kind_name(
    vitte_diagnostic_trace_kind_t kind
);
"""
)


# ============================================================================
# 17. Structured expected/found API
# ============================================================================

replace_once(
"""void
vitte_diagnostic_set_context(
    vitte_diagnostic_t *diagnostic,
    const char *package,
    const char *module,
    const char *procedure,
    const char *context
);
""",
"""void
vitte_diagnostic_set_context(
    vitte_diagnostic_t *diagnostic,
    const char *package,
    const char *module,
    const char *procedure,
    const char *context
);

void
vitte_diagnostic_set_expected_found(
    vitte_diagnostic_t *diagnostic,
    const char *expected,
    const char *found
);

void
vitte_diagnostic_set_probable_cause(
    vitte_diagnostic_t *diagnostic,
    const char *probable_cause
);
"""
)


# ============================================================================
# 18. Extended cause API
# ============================================================================

replace_once(
"""vitte_status_t
vitte_diagnostic_add_cause(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span
);
""",
"""vitte_status_t
vitte_diagnostic_add_cause(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span
);

vitte_status_t
vitte_diagnostic_add_cause_ex(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_cause_kind_t kind,
    const char *message,
    const vitte_ast_span_t *span,
    vitte_diagnostic_confidence_t confidence
);
"""
)


# ============================================================================
# 19. Trace API
# ============================================================================

marker = """/* ========================================================================= */
/* Notes / help                                                              */
/* ========================================================================= */
"""

addition = """/* ========================================================================= */
/* Causal trace                                                              */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_trace(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_trace_kind_t kind,
    const char *message,
    const vitte_ast_span_t *span
);

"""

source = source.replace(marker, addition + marker, 1)


# ============================================================================
# 20. Extended suggestion API
# ============================================================================

replace_once(
"""vitte_status_t
vitte_diagnostic_add_suggestion(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability
);
""",
"""vitte_status_t
vitte_diagnostic_add_suggestion(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability
);

vitte_status_t
vitte_diagnostic_add_suggestion_ex(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span,
    const char *replacement,
    vitte_diagnostic_applicability_t applicability,
    vitte_diagnostic_confidence_t confidence,
    bool preferred
);
"""
)


# ============================================================================
# 21. Query APIs for richer diagnostic engine
# ============================================================================

replace_once(
"""bool
vitte_diagnostic_has_warnings(
    const vitte_diagnostic_bag_t *bag
);

vitte_status_t
""",
"""bool
vitte_diagnostic_has_warnings(
    const vitte_diagnostic_bag_t *bag
);

bool
vitte_diagnostic_has_machine_fix(
    const vitte_diagnostic_t *diagnostic
);

bool
vitte_diagnostic_bag_limit_reached(
    const vitte_diagnostic_bag_t *bag
);

const vitte_diagnostic_t *
vitte_diagnostic_root(
    const vitte_diagnostic_bag_t *bag,
    size_t diagnostic_index
);

const vitte_diagnostic_t *
vitte_diagnostic_parent(
    const vitte_diagnostic_bag_t *bag,
    size_t diagnostic_index
);

vitte_status_t
"""
)


# ============================================================================
# 22. Statistics APIs
# ============================================================================

replace_once(
"""size_t
vitte_diagnostic_suppressed_count(
    const vitte_diagnostic_bag_t *bag
);
""",
"""size_t
vitte_diagnostic_suppressed_count(
    const vitte_diagnostic_bag_t *bag
);

size_t
vitte_diagnostic_primary_count(
    const vitte_diagnostic_bag_t *bag
);

size_t
vitte_diagnostic_cascade_count(
    const vitte_diagnostic_bag_t *bag
);
"""
)


# ============================================================================
# 23. Convenience accessors
# ============================================================================

replace_once(
"""static inline size_t
vitte_diagnostic_suggestion_count_inline(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL
        ? diagnostic->suggestion_count
        : 0u;
}
""",
"""static inline size_t
vitte_diagnostic_suggestion_count_inline(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL
        ? diagnostic->suggestion_count
        : 0u;
}

static inline size_t
vitte_diagnostic_trace_count(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL
        ? diagnostic->trace_count
        : 0u;
}

static inline bool
vitte_diagnostic_is_internal(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL &&
           diagnostic->is_internal;
}

static inline bool
vitte_diagnostic_poisons_analysis(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL &&
           diagnostic->poisons_analysis;
}
"""
)


# ============================================================================
# 24. Upgrade invariant documentation
# ============================================================================

replace_once(
""" * 14. Internal compiler errors must remain distinguishable from user errors.
 */
""",
""" * 14. Internal compiler errors must remain distinguishable from user errors.
 *
 * 15. expected/found are structured diagnostic metadata and must not be
 *     reconstructed by parsing the final human-readable message.
 *
 * 16. Machine-applicable suggestions require a valid source span and a
 *     concrete replacement. Empty replacement is a valid deletion.
 *
 * 17. Confidence is advisory. MACHINE applicability must only be selected
 *     when the producing compiler phase can justify the transformation.
 *
 * 18. Causal relationships must form an acyclic graph. With index-based
 *     storage, parent/root indices must refer to earlier diagnostics.
 *
 * 19. Poisoned semantic values should suppress derivative diagnostics rather
 *     than causing one user mistake to produce errors in every later phase.
 *
 * 20. HIR, IR and backend phases should not manufacture diagnostics for
 *     frontend nodes already represented by a semantic error value.
 *
 * 21. Renderers must never infer compiler semantics. They only present
 *     structured information produced by analysis phases.
 *
 * 22. LSP/SARIF/JSON output must preserve structured fixes, related
 *     locations and stable public diagnostic codes.
 */
"""
)


# ============================================================================
# 25. Update cascade documentation with stable IDs
# ============================================================================

replace_once(
""" * Array indices are currently used for relationships. If bags become
 * reorderable, persistent, or concurrent, replace them with stable
 * DiagnosticId values.
 */
""",
""" * Array indices remain available for efficient local bag traversal.
 *
 * diagnostic_id/root_id/parent_id provide a path toward stable causal
 * identity across merging, serialization, IDE transport and future
 * concurrent diagnostic production.
 *
 * provenance_id identifies the semantic origin propagated through compiler
 * representations.
 *
 * poison_id identifies an error value propagated through later phases and
 * allows derivative failures to be classified as cascades without relying
 * solely on source proximity.
 */
"""
)


# ============================================================================
# 26. More complete test documentation
# ============================================================================

replace_once(
""" *   fuzzing
 */
""",
""" *   fuzzing
 *   expected/found metadata
 *   probable causes
 *   confidence levels
 *   machine applicability
 *   preferred fixes
 *   causal traces
 *   parent/root graph integrity
 *   provenance IDs
 *   poison IDs
 *   semantic cascade suppression
 *   max diagnostic saturation
 *   warnings-as-errors accounting
 *   merge relationship rebasing
 *   deterministic fingerprints
 *   self-referential pointer rebinding
 *   insertion fix-its
 *   deletion fix-its
 *   multiline replacements
 *   overlapping fix detection
 *   conflicting fix detection
 *   parser recovery diagnostics
 *   error AST nodes
 *   error types
 *   frontend-to-HIR poisoning
 *   HIR-to-IR poisoning
 *   backend suppression after frontend failure
 */
"""
)


# ============================================================================
# 27. Add production responsibilities section
# ============================================================================

marker = """/* ========================================================================= */
/* Testing                                                                   */
/* ========================================================================= */
"""

addition = """/* ========================================================================= */
/* Producer responsibilities                                                 */
/* ========================================================================= */

/*
 * The diagnostic subsystem cannot infer semantic facts that were discarded
 * by an earlier compiler phase.
 *
 * Producers should therefore populate diagnostics at the point where the
 * relevant information is known.
 *
 * scanner / lexer:
 *   invalid byte/character
 *   malformed escape
 *   unterminated literal
 *   malformed number
 *   exact lexical span
 *
 * parser:
 *   unexpected token
 *   expected token set
 *   missing delimiters
 *   insertion/deletion fix-its
 *   recovery point
 *   primary syntax error vs recovery cascades
 *
 * import / module:
 *   unresolved module
 *   candidate modules
 *   import cycle
 *   declaration/import locations
 *
 * scope / symbol:
 *   unresolved name
 *   duplicate declaration
 *   previous declaration span
 *   nearest-name candidates
 *
 * type / sema:
 *   expected type
 *   actual type
 *   coercion failure
 *   parameter declaration
 *   argument position
 *   generic constraints
 *   trait/impl origin
 *   semantic causal chain
 *
 * constant evaluation:
 *   operation
 *   operands
 *   overflow/division/domain cause
 *   evaluation trace
 *
 * HIR / IR:
 *   lowering context
 *   provenance
 *   poison propagation
 *
 * C17 backend:
 *   generated-code provenance
 *   source remapping information
 *
 * external C compiler:
 *   captured structured tool output when possible
 *   generated-C span
 *   remapped Vitte span
 *
 * linker:
 *   missing symbol
 *   originating declaration/use
 *
 * driver:
 *   phase orchestration
 *   diagnostic limits
 *   output format
 *   warnings-as-errors policy
 *
 * No producer should parse another diagnostic's human-readable message in
 * order to recover structured information.
 */

"""

source = source.replace(marker, addition + marker, 1)


# ============================================================================
# 28. Sanity checks
# ============================================================================

required = [
    "vitte_diagnostic_confidence_t",
    "VITTE_DIAGNOSTIC_MAX_TRACE",
    "vitte_diagnostic_trace_t",
    "diagnostic_id",
    "provenance_id",
    "poison_id",
    "expected_storage",
    "found_storage",
    "probable_cause_storage",
    "vitte_diagnostic_add_cause_ex",
    "vitte_diagnostic_add_suggestion_ex",
    "vitte_diagnostic_add_trace",
    "vitte_diagnostic_has_machine_fix",
    "vitte_diagnostic_root",
    "vitte_diagnostic_parent",
]

for item in required:
    if item not in source:
        raise RuntimeError(f"Élément attendu absent après patch: {item}")

if source.count("#ifndef VITTE_DIAGNOSTIC_DIAGNOSTIC_H") != 1:
    raise RuntimeError("Include guard corrompu")

if source.count("#endif /* VITTE_DIAGNOSTIC_DIAGNOSTIC_H */") != 1:
    raise RuntimeError("Fin d'include guard corrompue")

if not source.endswith("\n"):
    source += "\n"


# ============================================================================
# 29. Atomic write
# ============================================================================

temporary = PATH.with_suffix(PATH.suffix + ".tmp")

temporary.write_text(
    source,
    encoding="utf-8",
    newline="\n",
)

temporary.replace(PATH)

print(f"Header mis à jour : {PATH}")
print(f"Sauvegarde         : {backup}")
print()
print("Ajouts principaux :")
print("  - expected/found structurés")
print("  - probable cause structurée")
print("  - confidence des suggestions")
print("  - confidence des causes")
print("  - causal trace")
print("  - scanner/codegen origins")
print("  - diagnostic_id/root_id/parent_id")
print("  - provenance_id")
print("  - poison_id")
print("  - machine-fix metadata")
print("  - primary/cascade counters")
print("  - saturation du diagnostic bag")
print("  - API add_cause_ex")
print("  - API add_suggestion_ex")
print("  - API add_trace")
print("  - API root/parent")
print("  - invariants industriels documentés")
print("  - responsabilités des producteurs documentées")
print()
print("IMPORTANT:")
print("Ce patch étend l'API publique.")
print("diagnostic.c, engine.c, cascade.c et suggestion.c doivent maintenant")
print("implémenter les nouvelles fonctions déclarées avant le link final.")