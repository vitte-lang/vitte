#ifndef VITTE_DIAGNOSTIC_DIAGNOSTIC_H
#define VITTE_DIAGNOSTIC_DIAGNOSTIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../api/error.h"
#include "../ast/span.h"
#include "../source/source.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Capacities                                                                */
/* ========================================================================= */

#ifndef VITTE_DIAGNOSTIC_SOURCE_NAME_CAPACITY
#define VITTE_DIAGNOSTIC_SOURCE_NAME_CAPACITY 4096u
#endif

#ifndef VITTE_DIAGNOSTIC_CODE_CAPACITY
#define VITTE_DIAGNOSTIC_CODE_CAPACITY 128u
#endif

#ifndef VITTE_DIAGNOSTIC_SHORT_CODE_CAPACITY
#define VITTE_DIAGNOSTIC_SHORT_CODE_CAPACITY 16u
#endif

#ifndef VITTE_DIAGNOSTIC_CATEGORY_CAPACITY
#define VITTE_DIAGNOSTIC_CATEGORY_CAPACITY 128u
#endif

#ifndef VITTE_DIAGNOSTIC_MESSAGE_CAPACITY
#define VITTE_DIAGNOSTIC_MESSAGE_CAPACITY 512u
#endif

#ifndef VITTE_DIAGNOSTIC_DETAILS_CAPACITY
#define VITTE_DIAGNOSTIC_DETAILS_CAPACITY 4096u
#endif

#ifndef VITTE_DIAGNOSTIC_LABEL_CAPACITY
#define VITTE_DIAGNOSTIC_LABEL_CAPACITY 4096u
#endif

#ifndef VITTE_DIAGNOSTIC_LABEL_SOURCE_CAPACITY
#define VITTE_DIAGNOSTIC_LABEL_SOURCE_CAPACITY 1024u
#endif

#ifndef VITTE_DIAGNOSTIC_REPLACEMENT_CAPACITY
#define VITTE_DIAGNOSTIC_REPLACEMENT_CAPACITY 256u
#endif

#ifndef VITTE_DIAGNOSTIC_SUBJECT_CAPACITY
#define VITTE_DIAGNOSTIC_SUBJECT_CAPACITY 256u
#endif

#ifndef VITTE_DIAGNOSTIC_CONTEXT_CAPACITY
#define VITTE_DIAGNOSTIC_CONTEXT_CAPACITY 512u
#endif

#ifndef VITTE_DIAGNOSTIC_PACKAGE_CAPACITY
#define VITTE_DIAGNOSTIC_PACKAGE_CAPACITY 128u
#endif

#ifndef VITTE_DIAGNOSTIC_MODULE_CAPACITY
#define VITTE_DIAGNOSTIC_MODULE_CAPACITY 256u
#endif

#ifndef VITTE_DIAGNOSTIC_PROCEDURE_CAPACITY
#define VITTE_DIAGNOSTIC_PROCEDURE_CAPACITY 256u
#endif

#ifndef VITTE_DIAGNOSTIC_CONTRACT_EXPRESSION_CAPACITY
#define VITTE_DIAGNOSTIC_CONTRACT_EXPRESSION_CAPACITY 512u
#endif

#ifndef VITTE_DIAGNOSTIC_CONTRACT_REASON_CAPACITY
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

/* ========================================================================= */
/* Collection limits                                                         */
/* ========================================================================= */

#ifndef VITTE_DIAGNOSTIC_MAX_RELATED
#define VITTE_DIAGNOSTIC_MAX_RELATED 16u
#endif

#ifndef VITTE_DIAGNOSTIC_MAX_LABELS
#define VITTE_DIAGNOSTIC_MAX_LABELS 16u
#endif

#ifndef VITTE_DIAGNOSTIC_MAX_CAUSES
#define VITTE_DIAGNOSTIC_MAX_CAUSES 16u
#endif

#ifndef VITTE_DIAGNOSTIC_MAX_ANNOTATIONS
#define VITTE_DIAGNOSTIC_MAX_ANNOTATIONS 16u
#endif

#ifndef VITTE_DIAGNOSTIC_MAX_SUGGESTIONS
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

#ifndef VITTE_DIAGNOSTIC_INDEX_NONE
#define VITTE_DIAGNOSTIC_INDEX_NONE SIZE_MAX
#endif

/* ========================================================================= */
/* Infrastructure diagnostic identifiers                                     */
/* ========================================================================= */

#define VITTE_INFRA_E_FALLBACK \
    "VITTE_INFRA_E_FALLBACK"

#define VITTE_INFRA_E_OUT_OF_MEMORY \
    "VITTE_INFRA_E_OUT_OF_MEMORY"

#define VITTE_INFRA_E_INVALID_ARGUMENT \
    "VITTE_INFRA_E_INVALID_ARGUMENT"

#define VITTE_INFRA_E_INVALID_STATE \
    "VITTE_INFRA_E_INVALID_STATE"

#define VITTE_INFRA_E_IO \
    "VITTE_INFRA_E_IO"

#define VITTE_INFRA_E_INTERNAL \
    "VITTE_INFRA_E_INTERNAL"

#define VITTE_INFRA_E_EXTERNAL_TOOL \
    "VITTE_INFRA_E_EXTERNAL_TOOL"

#define VITTE_INFRA_E_LIMIT \
    "VITTE_INFRA_E_LIMIT"

#define VITTE_INFRA_E_UNSUPPORTED \
    "VITTE_INFRA_E_UNSUPPORTED"

#define VITTE_INFRA_E_COMPONENT \
    "VITTE_INFRA_E_COMPONENT"

#define VITTE_INFRA_E_INVARIANT "VITTE_INFRA_E_INVARIANT"
#define VITTE_INFRA_E_IMPOSSIBLE "VITTE_INFRA_E_IMPOSSIBLE"
#define VITTE_INFRA_E_RESOURCE "VITTE_INFRA_E_RESOURCE"
#define VITTE_INFRA_E_SOURCE_READ "VITTE_INFRA_E_SOURCE_READ"
#define VITTE_INFRA_E_ENCODING "VITTE_INFRA_E_ENCODING"
#define VITTE_INFRA_E_SOURCE_MAP "VITTE_INFRA_E_SOURCE_MAP"
#define VITTE_INFRA_E_DIAGNOSTIC "VITTE_INFRA_E_DIAGNOSTIC"
#define VITTE_INFRA_E_FORMAT "VITTE_INFRA_E_FORMAT"
#define VITTE_INFRA_E_CONFIG "VITTE_INFRA_E_CONFIG"
#define VITTE_INFRA_E_PHASE "VITTE_INFRA_E_PHASE"

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef vitte_source_context_t
    vitte_source_manager_t;

/* ========================================================================= */
/* Severity                                                                  */
/* ========================================================================= */

typedef enum vitte_diagnostic_severity {
    VITTE_DIAGNOSTIC_NOTE = 0,
    VITTE_DIAGNOSTIC_HELP,
    VITTE_DIAGNOSTIC_WARNING,
    VITTE_DIAGNOSTIC_ERROR,
    VITTE_DIAGNOSTIC_FATAL
} vitte_diagnostic_severity_t;

/* ========================================================================= */
/* Origin / phase                                                            */
/* ========================================================================= */

typedef enum vitte_diagnostic_origin {
    VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN = 0,

    VITTE_DIAGNOSTIC_ORIGIN_IO,

    VITTE_DIAGNOSTIC_ORIGIN_SCANNER,
    VITTE_DIAGNOSTIC_ORIGIN_LEXER,
    VITTE_DIAGNOSTIC_ORIGIN_PARSER,

    VITTE_DIAGNOSTIC_ORIGIN_IMPORT,
    VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION,
    VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK,
    VITTE_DIAGNOSTIC_ORIGIN_CONTRACT,
    VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL,

    VITTE_DIAGNOSTIC_ORIGIN_HIR,
    VITTE_DIAGNOSTIC_ORIGIN_IR,
    VITTE_DIAGNOSTIC_ORIGIN_CODEGEN,

    VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND,
    VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER,
    VITTE_DIAGNOSTIC_ORIGIN_LINKER,

    VITTE_DIAGNOSTIC_ORIGIN_RUNTIME,
    VITTE_DIAGNOSTIC_ORIGIN_DRIVER,

    VITTE_DIAGNOSTIC_ORIGIN_INTERNAL
} vitte_diagnostic_origin_t;

static inline bool vitte_diagnostic_severity_is_valid(vitte_diagnostic_severity_t severity) {
    return severity >= VITTE_DIAGNOSTIC_NOTE && severity <= VITTE_DIAGNOSTIC_FATAL;
}

static inline bool vitte_diagnostic_origin_is_valid(vitte_diagnostic_origin_t origin) {
    return origin >= VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN && origin <= VITTE_DIAGNOSTIC_ORIGIN_INTERNAL;
}

/* ========================================================================= */
/* Location kind                                                             */
/* ========================================================================= */

typedef enum vitte_diagnostic_location_kind {
    VITTE_DIAGNOSTIC_LOCATION_PRIMARY = 0,
    VITTE_DIAGNOSTIC_LOCATION_DECLARATION,
    VITTE_DIAGNOSTIC_LOCATION_DEFINITION,
    VITTE_DIAGNOSTIC_LOCATION_ORIGIN,
    VITTE_DIAGNOSTIC_LOCATION_USE,
    VITTE_DIAGNOSTIC_LOCATION_CALL_SITE,
    VITTE_DIAGNOSTIC_LOCATION_INSTANTIATION,
    VITTE_DIAGNOSTIC_LOCATION_RELATED
} vitte_diagnostic_location_kind_t;

/* ========================================================================= */
/* Labels                                                                    */
/* ========================================================================= */

typedef enum vitte_diagnostic_label_style {
    VITTE_DIAGNOSTIC_LABEL_PRIMARY = 0,
    VITTE_DIAGNOSTIC_LABEL_SECONDARY
} vitte_diagnostic_label_style_t;

/* ========================================================================= */
/* Annotations                                                               */
/* ========================================================================= */

typedef enum vitte_diagnostic_annotation_kind {
    VITTE_DIAGNOSTIC_ANNOTATION_NOTE = 0,
    VITTE_DIAGNOSTIC_ANNOTATION_HELP
} vitte_diagnostic_annotation_kind_t;

/* ========================================================================= */
/* Suggestion applicability                                                  */
/* ========================================================================= */

typedef enum vitte_diagnostic_applicability {
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

/* ========================================================================= */
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

/* ========================================================================= */
/* Output format                                                             */
/* ========================================================================= */

typedef enum vitte_diagnostic_format {
    VITTE_DIAGNOSTIC_FORMAT_TERMINAL = 0,
    VITTE_DIAGNOSTIC_FORMAT_JSON,
    VITTE_DIAGNOSTIC_FORMAT_SARIF,
    VITTE_DIAGNOSTIC_FORMAT_LSP
} vitte_diagnostic_format_t;

/* ========================================================================= */
/* Contract                                                                  */
/* ========================================================================= */

typedef enum vitte_diagnostic_contract_kind {
    VITTE_DIAGNOSTIC_CONTRACT_NONE = 0,
    VITTE_DIAGNOSTIC_CONTRACT_REQUIRES,
    VITTE_DIAGNOSTIC_CONTRACT_ENSURES,
    VITTE_DIAGNOSTIC_CONTRACT_INVARIANT,
    VITTE_DIAGNOSTIC_CONTRACT_ASSERTION
} vitte_diagnostic_contract_kind_t;

/* ========================================================================= */
/* Label                                                                     */
/* ========================================================================= */

typedef struct vitte_diagnostic_label {
    vitte_diagnostic_label_style_t style;

    vitte_ast_span_t span;

    const char *message;

    char message_storage[
        VITTE_DIAGNOSTIC_LABEL_CAPACITY
    ];
} vitte_diagnostic_label_t;

/* ========================================================================= */
/* Related location                                                          */
/* ========================================================================= */

typedef struct vitte_diagnostic_location {
    vitte_diagnostic_location_kind_t kind;

    vitte_ast_span_t span;

    const char *label;

    char label_storage[
        VITTE_DIAGNOSTIC_LABEL_CAPACITY
    ];
} vitte_diagnostic_location_t;

/* ========================================================================= */
/* Cause                                                                     */
/* ========================================================================= */

typedef struct vitte_diagnostic_cause {
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

/* ========================================================================= */
/* Annotation                                                                */
/* ========================================================================= */

typedef struct vitte_diagnostic_annotation {
    vitte_diagnostic_annotation_kind_t kind;

    const char *message;

    char message_storage[
        VITTE_DIAGNOSTIC_MESSAGE_CAPACITY
    ];

    vitte_ast_span_t span;

    bool has_span;
} vitte_diagnostic_annotation_t;

/* ========================================================================= */
/* Suggestion                                                                */
/* ========================================================================= */

typedef struct vitte_diagnostic_suggestion {
    const char *message;

    char message_storage[
        VITTE_DIAGNOSTIC_MESSAGE_CAPACITY
    ];

    vitte_ast_span_t span;

    bool has_span;

    const char *replacement;

    char replacement_storage[
        VITTE_DIAGNOSTIC_REPLACEMENT_CAPACITY
    ];

    /*
     * Required because:
     *
     *   replacement == NULL
     *
     * means no structured edit, while:
     *
     *   replacement == ""
     *
     * represents a deletion.
     */
    bool has_replacement;

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

/* ========================================================================= */
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

/* ========================================================================= */
/* Contract information                                                      */
/* ========================================================================= */

typedef struct vitte_diagnostic_contract {
    vitte_diagnostic_contract_kind_t kind;

    const char *expression;
    const char *procedure;
    const char *reason;

    char expression_storage[
        VITTE_DIAGNOSTIC_CONTRACT_EXPRESSION_CAPACITY
    ];

    char procedure_storage[
        VITTE_DIAGNOSTIC_PROCEDURE_CAPACITY
    ];

    char reason_storage[
        VITTE_DIAGNOSTIC_CONTRACT_REASON_CAPACITY
    ];

    vitte_ast_span_t contract_span;
    vitte_ast_span_t call_span;
    vitte_ast_span_t declaration_span;

    bool has_contract_span;
    bool has_call_span;
    bool has_declaration_span;
} vitte_diagnostic_contract_t;

/* ========================================================================= */
/* Rendering options                                                         */
/* ========================================================================= */

typedef struct vitte_diagnostic_options {
    size_t max_diagnostics;

    bool warnings_as_errors;

    bool color_enabled;

    bool show_source_line;
    bool show_codes;
    bool show_details;
    bool show_phase;
    bool show_causes;
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
} vitte_diagnostic_options_t;

/* ========================================================================= */
/* Counts                                                                    */
/* ========================================================================= */

typedef struct vitte_diagnostic_counts {
    size_t note_count;
    size_t help_count;

    size_t warning_count;

    size_t error_count;
    size_t fatal_count;

    size_t suppressed_count;

    size_t primary_count;
    size_t cascade_count;
} vitte_diagnostic_counts_t;

/* ========================================================================= */
/* Diagnostic                                                               */
/* ========================================================================= */

typedef struct vitte_diagnostic {
    /* --------------------------------------------------------------------- */
    /* Classification                                                        */
    /* --------------------------------------------------------------------- */

    vitte_diagnostic_severity_t severity;
    vitte_diagnostic_origin_t origin;

    /* --------------------------------------------------------------------- */
    /* Stable codes                                                          */
    /* --------------------------------------------------------------------- */

    const char *code;
    const char *short_code;
    const char *category;

    char code_storage[
        VITTE_DIAGNOSTIC_CODE_CAPACITY
    ];

    char short_code_storage[
        VITTE_DIAGNOSTIC_SHORT_CODE_CAPACITY
    ];

    char category_storage[
        VITTE_DIAGNOSTIC_CATEGORY_CAPACITY
    ];

    /* --------------------------------------------------------------------- */
    /* Identity                                                              */
    /* --------------------------------------------------------------------- */

    uint64_t fingerprint;

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
    /* --------------------------------------------------------------------- */

    const char *message;
    const char *details;

    char message_storage[
        VITTE_DIAGNOSTIC_MESSAGE_CAPACITY
    ];

    char details_storage[
        VITTE_DIAGNOSTIC_DETAILS_CAPACITY
    ];

    /* --------------------------------------------------------------------- */
    /* Semantic subject                                                      */
    /* --------------------------------------------------------------------- */

    const char *symbol;
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

    char symbol_storage[
        VITTE_DIAGNOSTIC_SUBJECT_CAPACITY
    ];

    char expected_type_storage[
        VITTE_DIAGNOSTIC_SUBJECT_CAPACITY
    ];

    char actual_type_storage[
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
        VITTE_DIAGNOSTIC_CONTEXT_CAPACITY
    ];

    /* --------------------------------------------------------------------- */
    /* Compilation context                                                   */
    /* --------------------------------------------------------------------- */

    const char *package;
    const char *module;
    const char *procedure;

    char package_storage[
        VITTE_DIAGNOSTIC_PACKAGE_CAPACITY
    ];

    char module_storage[
        VITTE_DIAGNOSTIC_MODULE_CAPACITY
    ];

    char procedure_storage[
        VITTE_DIAGNOSTIC_PROCEDURE_CAPACITY
    ];

    /* --------------------------------------------------------------------- */
    /* Primary source location                                               */
    /* --------------------------------------------------------------------- */

    const char *source_name;

    char source_name_storage[
        VITTE_DIAGNOSTIC_SOURCE_NAME_CAPACITY
    ];

    vitte_source_id_t source_id;

    size_t start_offset;
    size_t end_offset;

    size_t start_line;
    size_t start_column;

    size_t end_line;
    size_t end_column;

    bool has_span;

    /* --------------------------------------------------------------------- */
    /* Labels                                                                */
    /* --------------------------------------------------------------------- */

    vitte_diagnostic_label_t labels[
        VITTE_DIAGNOSTIC_MAX_LABELS
    ];

    size_t label_count;

    /* --------------------------------------------------------------------- */
    /* Related locations                                                     */
    /* --------------------------------------------------------------------- */

    vitte_diagnostic_location_t related[
        VITTE_DIAGNOSTIC_MAX_RELATED
    ];

    size_t related_count;

    /* --------------------------------------------------------------------- */
    /* Cause chain                                                           */
    /* --------------------------------------------------------------------- */

    vitte_diagnostic_cause_t causes[
        VITTE_DIAGNOSTIC_MAX_CAUSES
    ];

    size_t cause_count;

    /* --------------------------------------------------------------------- */
    /* Causal trace                                                          */
    /* --------------------------------------------------------------------- */

    vitte_diagnostic_trace_t trace[
        VITTE_DIAGNOSTIC_MAX_TRACE
    ];

    size_t trace_count;

    /* --------------------------------------------------------------------- */
    /* Notes / help                                                          */
    /* --------------------------------------------------------------------- */

    vitte_diagnostic_annotation_t annotations[
        VITTE_DIAGNOSTIC_MAX_ANNOTATIONS
    ];

    size_t annotation_count;

    /* --------------------------------------------------------------------- */
    /* Structured suggestions                                                */
    /* --------------------------------------------------------------------- */

    vitte_diagnostic_suggestion_t suggestions[
        VITTE_DIAGNOSTIC_MAX_SUGGESTIONS
    ];

    size_t suggestion_count;

    /* --------------------------------------------------------------------- */
    /* Contracts                                                             */
    /* --------------------------------------------------------------------- */

    vitte_diagnostic_contract_t contract;

    bool has_contract;

    /* --------------------------------------------------------------------- */
    /* Causal diagnostic graph                                               */
    /* --------------------------------------------------------------------- */

    size_t root_diagnostic;
    size_t parent_diagnostic;

    bool is_primary;
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

/* ========================================================================= */
/* Diagnostic bag                                                           */
/* ========================================================================= */

typedef struct vitte_diagnostic_bag {
    bool initialized;

    vitte_diagnostic_t *storage;

    size_t capacity;
    size_t count;

    vitte_diagnostic_counts_t counts;

    vitte_diagnostic_options_t options;

    vitte_error_t last_error;

    /*
     * Monotonic ID generator. Zero is reserved for "unassigned".
     */
    uint64_t next_diagnostic_id;

    /*
     * True after the configured diagnostic limit is reached.
     */
    bool limit_reached;
} vitte_diagnostic_bag_t;

/* ========================================================================= */
/* Metadata names                                                            */
/* ========================================================================= */

const char *
vitte_diagnostic_severity_name(
    vitte_diagnostic_severity_t severity
);

bool
vitte_diagnostic_severity_is_error(
    vitte_diagnostic_severity_t severity
);

const char *
vitte_diagnostic_origin_name(
    vitte_diagnostic_origin_t origin
);

const char *
vitte_diagnostic_location_kind_name(
    vitte_diagnostic_location_kind_t kind
);

const char *
vitte_diagnostic_label_style_name(
    vitte_diagnostic_label_style_t style
);

const char *
vitte_diagnostic_annotation_kind_name(
    vitte_diagnostic_annotation_kind_t kind
);

const char *
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

/* ========================================================================= */
/* Options                                                                   */
/* ========================================================================= */

void
vitte_diagnostic_options_init(
    vitte_diagnostic_options_t *options
);

/* ========================================================================= */
/* Counts                                                                    */
/* ========================================================================= */

void
vitte_diagnostic_counts_init(
    vitte_diagnostic_counts_t *counts
);

/* ========================================================================= */
/* Diagnostic lifetime                                                       */
/* ========================================================================= */

void
vitte_diagnostic_init(
    vitte_diagnostic_t *diagnostic
);

void
vitte_diagnostic_reset(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Semantic/context metadata                                                 */
/* ========================================================================= */

void
vitte_diagnostic_set_subject(
    vitte_diagnostic_t *diagnostic,
    const char *symbol,
    const char *expected_type,
    const char *actual_type
);

void
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

/* ========================================================================= */
/* Source span                                                               */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_set_span(
    vitte_diagnostic_t *diagnostic,
    const vitte_ast_span_t *span
);

/* ========================================================================= */
/* Labels                                                                    */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_label(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_label_style_t style,
    const vitte_ast_span_t *span,
    const char *message
);

/* ========================================================================= */
/* Related locations                                                         */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_location(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_location_kind_t kind,
    const vitte_ast_span_t *span,
    const char *label_text
);

/* ========================================================================= */
/* Cause chain                                                               */
/* ========================================================================= */

vitte_status_t
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

/* ========================================================================= */
/* Causal trace                                                              */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_trace(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_trace_kind_t kind,
    const char *message,
    const vitte_ast_span_t *span
);

/* ========================================================================= */
/* Notes / help                                                              */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_add_annotation(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_annotation_kind_t kind,
    const char *message,
    const vitte_ast_span_t *span
);

vitte_status_t
vitte_diagnostic_add_note(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span
);

vitte_status_t
vitte_diagnostic_add_help(
    vitte_diagnostic_t *diagnostic,
    const char *message,
    const vitte_ast_span_t *span
);

/* ========================================================================= */
/* Suggestions                                                               */
/* ========================================================================= */

vitte_status_t
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

/* ========================================================================= */
/* Contracts                                                                 */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_set_contract(
    vitte_diagnostic_t *diagnostic,
    vitte_diagnostic_contract_kind_t kind,
    const char *expression,
    const char *procedure,
    const char *reason,
    const vitte_ast_span_t *contract_span,
    const vitte_ast_span_t *call_span,
    const vitte_ast_span_t *declaration_span
);

/* ========================================================================= */
/* Fingerprints                                                              */
/* ========================================================================= */

uint64_t
vitte_diagnostic_fingerprint(
    const vitte_diagnostic_t *diagnostic
);

void
vitte_diagnostic_refresh_fingerprint(
    vitte_diagnostic_t *diagnostic
);

/* ========================================================================= */
/* Bag lifetime                                                              */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_bag_init(
    vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_t *storage,
    size_t capacity
);

void
vitte_diagnostic_bag_reset(
    vitte_diagnostic_bag_t *bag
);

bool
vitte_diagnostic_bag_is_initialized(
    const vitte_diagnostic_bag_t *bag
);

/* ========================================================================= */
/* Bag access                                                                */
/* ========================================================================= */

size_t
vitte_diagnostic_bag_count(
    const vitte_diagnostic_bag_t *bag
);

bool
vitte_diagnostic_bag_empty(
    const vitte_diagnostic_bag_t *bag
);

const vitte_diagnostic_t *
vitte_diagnostic_at(
    const vitte_diagnostic_bag_t *bag,
    size_t index
);

vitte_diagnostic_t *
vitte_diagnostic_at_mut(
    vitte_diagnostic_bag_t *bag,
    size_t index
);

/* ========================================================================= */
/* Add / emit                                                                */
/* ========================================================================= */

/*
 * Accept a registered internal identifier or public E/W/N/H code. Registry
 * metadata supplies the canonical codes, category and an unknown origin;
 * an explicit origin is preserved. Severity is supplied by the caller, with
 * warnings promoted by bag.options.warnings_as_errors without changing code.
 * Invalid severity/origin values are rejected before the bag is modified.
 */
vitte_status_t
vitte_diagnostic_add(
    vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_t *diagnostic
);

vitte_status_t
vitte_diagnostic_emit(
    vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_severity_t severity,
    vitte_diagnostic_origin_t origin,
    const char *code,
    const char *message,
    const vitte_ast_span_t *span
);

/* ========================================================================= */
/* Cascade                                                                   */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_set_parent(
    vitte_diagnostic_bag_t *bag,
    size_t diagnostic_index,
    size_t parent_index
);

vitte_status_t
vitte_diagnostic_mark_cascade(
    vitte_diagnostic_bag_t *bag,
    size_t diagnostic_index,
    size_t root_index
);

vitte_status_t
vitte_diagnostic_suppress(
    vitte_diagnostic_bag_t *bag,
    size_t index
);

/* ========================================================================= */
/* Queries                                                                   */
/* ========================================================================= */

bool
vitte_diagnostic_has_errors(
    const vitte_diagnostic_bag_t *bag
);

bool
vitte_diagnostic_has_fatal(
    const vitte_diagnostic_bag_t *bag
);

bool
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
vitte_diagnostic_compilation_status(
    const vitte_diagnostic_bag_t *bag
);

/* ========================================================================= */
/* Merge                                                                     */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_merge(
    vitte_diagnostic_bag_t *destination,
    const vitte_diagnostic_bag_t *source
);

/* ========================================================================= */
/* Formatting                                                               */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_format_one(
    const vitte_diagnostic_t *diagnostic,
    char *buffer,
    size_t capacity
);

vitte_status_t
vitte_diagnostic_format_one_ex(
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_options_t *options,
    char *buffer,
    size_t capacity
);

vitte_status_t
vitte_diagnostic_format_summary(
    const vitte_diagnostic_bag_t *bag,
    char *buffer,
    size_t capacity
);

/* ========================================================================= */
/* Stream output                                                             */
/* ========================================================================= */

vitte_status_t
vitte_diagnostic_write_one(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
);

vitte_status_t
vitte_diagnostic_write_all(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag
);

vitte_status_t
vitte_diagnostic_write_all_format(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    vitte_diagnostic_format_t format
);

/* ========================================================================= */
/* Error conversion                                                          */
/* ========================================================================= */

vitte_error_t
vitte_diagnostic_last_error(
    const vitte_diagnostic_bag_t *bag
);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

size_t
vitte_diagnostic_error_count(
    const vitte_diagnostic_bag_t *bag
);

size_t
vitte_diagnostic_warning_count(
    const vitte_diagnostic_bag_t *bag
);

size_t
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

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_diagnostic_validate(
    const vitte_diagnostic_t *diagnostic
);

bool
vitte_diagnostic_bag_validate(
    const vitte_diagnostic_bag_t *bag
);

/* ========================================================================= */
/* Component metadata                                                        */
/* ========================================================================= */

const char *
vitte_diagnostic_component_name(void);

uint32_t
vitte_diagnostic_component_version(void);

/* ========================================================================= */
/* Convenience                                                               */
/* ========================================================================= */

static inline bool
vitte_diagnostic_is_suppressed(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL &&
           diagnostic->is_suppressed;
}

static inline bool
vitte_diagnostic_is_cascade(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL &&
           diagnostic->is_cascade;
}

static inline bool
vitte_diagnostic_is_primary(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL &&
           diagnostic->is_primary;
}

static inline bool
vitte_diagnostic_has_span(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL &&
           diagnostic->has_span;
}

static inline bool
vitte_diagnostic_has_contract(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL &&
           diagnostic->has_contract;
}

static inline size_t
vitte_diagnostic_label_count(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL
        ? diagnostic->label_count
        : 0u;
}

static inline size_t
vitte_diagnostic_related_count(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL
        ? diagnostic->related_count
        : 0u;
}

static inline size_t
vitte_diagnostic_cause_count(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL
        ? diagnostic->cause_count
        : 0u;
}

static inline size_t
vitte_diagnostic_annotation_count(
    const vitte_diagnostic_t *diagnostic
)
{
    return diagnostic != NULL
        ? diagnostic->annotation_count
        : 0u;
}

static inline size_t
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

/* ========================================================================= */
/* Architecture                                                              */
/* ========================================================================= */

/*
 * Diagnostic flow:
 *
 *   source (.vit)
 *       |
 *       v
 *   lexer
 *       |
 *       v
 *   parser
 *       |
 *       v
 *   AST
 *       |
 *       v
 *   import / name resolution
 *       |
 *       v
 *   semantic / type / contract checking
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
 *       +-----------------------------+
 *       |                             |
 *       v                             v
 *   source map                  generated C
 *                                     |
 *                                     v
 *                               Clang / GCC
 *                                     |
 *                                     v
 *                              diagnostic remap
 *                                     |
 *                                     v
 *                          vitte_diagnostic_t
 *                                     |
 *          +--------------------------+--------------------------+
 *          |              |              |                       |
 *          v              v              v                       v
 *       terminal         JSON           SARIF                    LSP
 */

/* ========================================================================= */
/* Diagnostic invariants                                                     */
/* ========================================================================= */

/*
 * 1. Every user-facing compiler failure should eventually become a
 *    vitte_diagnostic_t.
 *
 * 2. Raw fprintf/perror output from compiler phases should be eliminated.
 *
 * 3. Stable public codes such as E0501 are independent from human wording.
 *
 * 4. Internal symbolic codes may evolve independently from public codes.
 *
 * 5. Source spans are byte based and half-open:
 *
 *        [start_offset, end_offset)
 *
 * 6. Line/column data is zero based internally.
 *
 * 7. Terminal/SARIF/LSP renderers are responsible for converting coordinate
 *    conventions where required.
 *
 * 8. A diagnostic may carry multiple source locations.
 *
 * 9. Suggestions are structured edits, not formatted help strings.
 *
 * 10. Cascade suppression must preserve the original/root diagnostic.
 *
 * 11. Fingerprints are for deterministic identity/dedup assistance, not
 *     cryptographic security.
 *
 * 12. Fingerprint equality alone must never be considered structural
 *     equality.
 *
 * 13. Generated C diagnostics should be remapped to Vitte source whenever
 *     provenance exists.
 *
 * 14. Internal compiler errors must remain distinguishable from user errors.
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

/* ========================================================================= */
/* Diagnostic ownership                                                      */
/* ========================================================================= */

/*
 * Most textual fields use embedded storage:
 *
 *   const char *message;
 *   char message_storage[...];
 *
 * The pointer normally points to the corresponding inline array.
 *
 * This makes diagnostics allocation-light and deterministic.
 *
 * IMPORTANT:
 *
 * Raw struct movement using:
 *
 *   memcpy
 *   memmove
 *   qsort
 *   assignment
 *
 * also copies self-referential pointer values.
 *
 * Code moving complete diagnostics must therefore rebind those pointers.
 *
 * diagnostic.c handles rebinding when diagnostics enter a bag.
 *
 * Specialized modules such as suggestion.c and label.c must similarly repair
 * their own inline pointers after sorting or moving their structures.
 */

/* ========================================================================= */
/* Error code namespaces                                                     */
/* ========================================================================= */

/*
 * E0000-E0099  compiler infrastructure
 * E0100-E0199  lexer
 * E0200-E0299  parser
 * E0300-E0399  modules/imports/packages
 * E0400-E0499  names/scopes
 * E0500-E0599  types
 * E0600-E0699  calls/procedures
 * E0700-E0799  contracts
 * E0800-E0899  generics/traits/impl
 * E0900-E0999  C17 backend
 * E1000-E1099  HIR/IR/lowering
 * E1100-E1199  control flow
 * E1200-E1299  memory/references/pointers
 * E1300-E1399  unsafe/low-level
 * E1400-E1499  FFI/ABI
 * E1500-E1599  constant evaluation
 * E1600-E1699  pattern matching
 * E1700-E1799  concurrency/async
 * E1800-E1899  macros/compiler/pass
 * E1900-E1999  target/platform
 *
 * Wxxxx        warnings
 * Nxxxx        notes
 * Hxxxx        help
 */

/* ========================================================================= */
/* Contract diagnostics                                                      */
/* ========================================================================= */

/*
 * Suggested stable contract namespace:
 *
 * E0700 invalid contract
 * E0701 precondition
 * E0702 postcondition
 * E0703 invariant
 * E0704 requires
 * E0705 ensures
 * E0706 old-value
 * E0707 result
 * E0708 unprovable
 * E0709 contradiction
 * E0710 effect
 * E0711 call contract
 * E0712 override contract
 * E0713 runtime contract
 * E0714 internal contract failure
 */

/* ========================================================================= */
/* Cascade model                                                             */
/* ========================================================================= */

/*
 * A compiler error can poison later analysis:
 *
 *   unknown symbol
 *       |
 *       +-> unknown type
 *       |
 *       +-> invalid call
 *       |
 *       +-> backend cannot lower expression
 *
 * Only the first error may be actionable.
 *
 * root_diagnostic:
 *   index of the originating diagnostic.
 *
 * parent_diagnostic:
 *   immediate causal parent.
 *
 * is_cascade:
 *   diagnostic was classified as derived.
 *
 * is_suppressed:
 *   diagnostic remains available structurally but is hidden from ordinary
 *   output.
 *
 * Array indices remain available for efficient local bag traversal.
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

/* ========================================================================= */
/* Source manager                                                            */
/* ========================================================================= */

/*
 * vitte_source_manager_t is deliberately opaque here.
 *
 * The source subsystem should eventually provide:
 *
 *   SourceId -> source
 *   source name/path
 *   source bytes
 *   line-start table
 *   byte offset -> line/column
 *   line lookup
 *   source excerpts
 *   Unicode-aware display columns
 *   generated-source identity
 *
 * Renderers should consume that API instead of implementing their own source
 * scanning.
 */

/* ========================================================================= */
/* Future provenance                                                         */
/* ========================================================================= */

/*
 * A future causal provenance layer should use stable IDs such as:
 *
 *   typedef uint64_t vitte_diagnostic_provenance_id_t;
 *   typedef uint64_t vitte_diagnostic_poison_id_t;
 *
 * and propagate them through:
 *
 *   AST -> HIR -> IR -> C17
 *
 * This provides stronger cascade classification than source proximity alone.
 *
 * Do not add these fields to only one compiler phase: provenance must be
 * introduced coherently through the entire pipeline.
 */

/* ========================================================================= */
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

/* ========================================================================= */
/* Testing                                                                   */
/* ========================================================================= */

/*
 * Core diagnostic tests should cover:
 *
 *   initialization
 *   reset
 *   severity
 *   origin
 *   registry normalization
 *   source spans
 *   zero-width spans
 *   multiline spans
 *   labels
 *   multiple labels
 *   related locations
 *   declarations
 *   call sites
 *   instantiation sites
 *   causes
 *   notes
 *   help
 *   suggestions
 *   deletions
 *   insertions
 *   replacements
 *   contracts
 *   fingerprints
 *   hash collision structural confirmation
 *   duplicate diagnostics
 *   cascade relationships
 *   suppression
 *   counters
 *   bag capacity
 *   merge
 *   deterministic output
 *   malformed UTF-8
 *   Unicode
 *   tabs
 *   very long source lines
 *   invalid source IDs
 *   generated C remapping
 *   ASan
 *   UBSan
 *   fuzzing
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

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_DIAGNOSTIC_H */
