#ifndef VITTE_DIAGNOSTIC_DIAGNOSTIC_H
#define VITTE_DIAGNOSTIC_DIAGNOSTIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../api/error.h"
#include "../ast/ast.h"
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
#define VITTE_DIAGNOSTIC_LABEL_CAPACITY 256u
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
#define VITTE_DIAGNOSTIC_MAX_SUGGESTIONS 8u
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

    VITTE_DIAGNOSTIC_ORIGIN_LEXER,
    VITTE_DIAGNOSTIC_ORIGIN_PARSER,

    VITTE_DIAGNOSTIC_ORIGIN_IMPORT,
    VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION,
    VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK,
    VITTE_DIAGNOSTIC_ORIGIN_CONTRACT,
    VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL,

    VITTE_DIAGNOSTIC_ORIGIN_HIR,
    VITTE_DIAGNOSTIC_ORIGIN_IR,

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
    VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE = 0,
    VITTE_DIAGNOSTIC_APPLICABILITY_MAYBE,
    VITTE_DIAGNOSTIC_APPLICABILITY_PLACEHOLDERS,
    VITTE_DIAGNOSTIC_APPLICABILITY_MANUAL
} vitte_diagnostic_applicability_t;

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
    const char *message;

    char message_storage[
        VITTE_DIAGNOSTIC_MESSAGE_CAPACITY
    ];

    vitte_ast_span_t span;

    bool has_span;
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
} vitte_diagnostic_suggestion_t;

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
 * Array indices are currently used for relationships. If bags become
 * reorderable, persistent, or concurrent, replace them with stable
 * DiagnosticId values.
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
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_DIAGNOSTIC_H */
