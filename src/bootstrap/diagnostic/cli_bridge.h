#ifndef VITTE_DIAGNOSTIC_CLI_BRIDGE_H
#define VITTE_DIAGNOSTIC_CLI_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../source/source.h"

#ifdef __cplusplus
extern "C" {
#endif

/* These transport callbacks intentionally use opaque LSP values so this
 * header stays independent from parser/AST headers. The renderer casts them
 * to the concrete LSP callback types at the serialization boundary. */
typedef bool (*vitte_cli_lsp_position_converter_t)(
    vitte_source_id_t source_id,
    const char *source_name,
    size_t byte_offset,
    int encoding,
    void *position,
    void *user_data);
typedef bool (*vitte_cli_lsp_uri_converter_t)(
    vitte_source_id_t source_id,
    const char *source_name,
    char *buffer,
    size_t capacity,
    void *user_data);

/*
 * Vitte diagnostic CLI bridge.
 *
 * This header defines the type-neutral boundary between compiler/frontend
 * diagnostics and the diagnostic rendering subsystem used by the command-line
 * interface.
 *
 * The bridge deliberately does not expose AST implementation details.
 *
 * It transports structured diagnostic information sufficient for:
 *
 *   - terminal rendering;
 *   - JSON rendering;
 *   - SARIF rendering;
 *   - LSP-oriented consumers;
 *   - diagnostic explanations;
 *   - source snippets;
 *   - primary/secondary labels;
 *   - expected/found information;
 *   - probable causes;
 *   - notes and help;
 *   - related locations;
 *   - suggestions;
 *   - machine-readable fix-its;
 *   - suggestion confidence;
 *   - causal chains;
 *   - root/cascade relationships;
 *   - compiler-phase provenance;
 *   - contract provenance;
 *   - external-tool diagnostics.
 *
 * The bridge should remain a transport layer. Diagnostic intelligence belongs
 * primarily to the producer phases and the diagnostic engine.
 */

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

#define VITTE_CLI_DIAGNOSTIC_CODE_CAPACITY              128u
#define VITTE_CLI_DIAGNOSTIC_SHORT_CODE_CAPACITY         16u
#define VITTE_CLI_DIAGNOSTIC_CATEGORY_CAPACITY          128u
#define VITTE_CLI_DIAGNOSTIC_MESSAGE_CAPACITY          1024u
#define VITTE_CLI_DIAGNOSTIC_DETAILS_CAPACITY          4096u
#define VITTE_CLI_DIAGNOSTIC_SOURCE_NAME_CAPACITY      4096u
#define VITTE_CLI_DIAGNOSTIC_LABEL_CAPACITY            4096u
#define VITTE_CLI_DIAGNOSTIC_TEXT_CAPACITY             1024u
#define VITTE_CLI_DIAGNOSTIC_REPLACEMENT_CAPACITY      1024u
#define VITTE_CLI_DIAGNOSTIC_SYMBOL_CAPACITY             256u
#define VITTE_CLI_DIAGNOSTIC_TYPE_CAPACITY               512u
#define VITTE_CLI_DIAGNOSTIC_CONTEXT_CAPACITY           1024u
#define VITTE_CLI_DIAGNOSTIC_PACKAGE_CAPACITY            256u
#define VITTE_CLI_DIAGNOSTIC_MODULE_CAPACITY             512u
#define VITTE_CLI_DIAGNOSTIC_PROCEDURE_CAPACITY          512u
#define VITTE_CLI_DIAGNOSTIC_TOOL_CAPACITY               256u
#define VITTE_CLI_DIAGNOSTIC_RULE_CAPACITY               256u
#define VITTE_CLI_DIAGNOSTIC_EXPRESSION_CAPACITY        1024u

#define VITTE_CLI_DIAGNOSTIC_MAX_LABELS                   16u
#define VITTE_CLI_DIAGNOSTIC_MAX_RELATED                  16u
#define VITTE_CLI_DIAGNOSTIC_MAX_CAUSES                   16u
#define VITTE_CLI_DIAGNOSTIC_MAX_ANNOTATIONS              16u
#define VITTE_CLI_DIAGNOSTIC_MAX_SUGGESTIONS              16u
#define VITTE_CLI_DIAGNOSTIC_MAX_TRACE                    32u
#define VITTE_CLI_DIAGNOSTIC_MAX_FIX_PARTS                16u

#define VITTE_CLI_DIAGNOSTIC_INDEX_NONE SIZE_MAX

/* ========================================================================= */
/* Severity                                                                  */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_severity {
    VITTE_CLI_DIAGNOSTIC_NOTE = 0,
    VITTE_CLI_DIAGNOSTIC_HELP,
    VITTE_CLI_DIAGNOSTIC_WARNING,
    VITTE_CLI_DIAGNOSTIC_ERROR,
    VITTE_CLI_DIAGNOSTIC_FATAL
} vitte_cli_diagnostic_severity_t;

/* ========================================================================= */
/* Output format                                                             */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_format {
    VITTE_CLI_DIAGNOSTIC_TERMINAL = 0,
    VITTE_CLI_DIAGNOSTIC_JSON,
    VITTE_CLI_DIAGNOSTIC_SARIF,
    VITTE_CLI_DIAGNOSTIC_LSP
} vitte_cli_diagnostic_format_t;

/* ========================================================================= */
/* Compiler origin                                                           */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_origin {
    VITTE_CLI_DIAGNOSTIC_ORIGIN_UNKNOWN = 0,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_IO,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_LEXER,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_SCANNER,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_PARSER,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_IMPORT,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_TYPE_CHECK,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_CONTRACT,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_HIR,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_IR,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_CODEGEN,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_C17_BACKEND,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_C_COMPILER,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_LINKER,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_RUNTIME,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_DRIVER,
    VITTE_CLI_DIAGNOSTIC_ORIGIN_INTERNAL
} vitte_cli_diagnostic_origin_t;

/* ========================================================================= */
/* Labels                                                                    */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_label_style {
    VITTE_CLI_DIAGNOSTIC_LABEL_PRIMARY = 0,
    VITTE_CLI_DIAGNOSTIC_LABEL_SECONDARY
} vitte_cli_diagnostic_label_style_t;

/* ========================================================================= */
/* Related location kind                                                     */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_location_kind {
    VITTE_CLI_DIAGNOSTIC_LOCATION_PRIMARY = 0,
    VITTE_CLI_DIAGNOSTIC_LOCATION_DECLARATION,
    VITTE_CLI_DIAGNOSTIC_LOCATION_DEFINITION,
    VITTE_CLI_DIAGNOSTIC_LOCATION_ORIGIN,
    VITTE_CLI_DIAGNOSTIC_LOCATION_USE,
    VITTE_CLI_DIAGNOSTIC_LOCATION_CALL_SITE,
    VITTE_CLI_DIAGNOSTIC_LOCATION_INSTANTIATION,
    VITTE_CLI_DIAGNOSTIC_LOCATION_RELATED
} vitte_cli_diagnostic_location_kind_t;

/* ========================================================================= */
/* Annotation kind                                                           */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_annotation_kind {
    VITTE_CLI_DIAGNOSTIC_ANNOTATION_NOTE = 0,
    VITTE_CLI_DIAGNOSTIC_ANNOTATION_HELP
} vitte_cli_diagnostic_annotation_kind_t;

/* ========================================================================= */
/* Suggestion applicability                                                  */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_applicability {
    VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MACHINE = 0,
    VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MAYBE,
    VITTE_CLI_DIAGNOSTIC_APPLICABILITY_PLACEHOLDERS,
    VITTE_CLI_DIAGNOSTIC_APPLICABILITY_MANUAL
} vitte_cli_diagnostic_applicability_t;

/* ========================================================================= */
/* Confidence                                                                */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_confidence {
    VITTE_CLI_DIAGNOSTIC_CONFIDENCE_NONE = 0,
    VITTE_CLI_DIAGNOSTIC_CONFIDENCE_LOW,
    VITTE_CLI_DIAGNOSTIC_CONFIDENCE_MEDIUM,
    VITTE_CLI_DIAGNOSTIC_CONFIDENCE_HIGH,
    VITTE_CLI_DIAGNOSTIC_CONFIDENCE_CERTAIN
} vitte_cli_diagnostic_confidence_t;

/* ========================================================================= */
/* Cause kind                                                                */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_cause_kind {
    VITTE_CLI_DIAGNOSTIC_CAUSE_UNKNOWN = 0,
    VITTE_CLI_DIAGNOSTIC_CAUSE_SYNTAX,
    VITTE_CLI_DIAGNOSTIC_CAUSE_MISSING_TOKEN,
    VITTE_CLI_DIAGNOSTIC_CAUSE_UNEXPECTED_TOKEN,
    VITTE_CLI_DIAGNOSTIC_CAUSE_UNRESOLVED_NAME,
    VITTE_CLI_DIAGNOSTIC_CAUSE_TYPE_MISMATCH,
    VITTE_CLI_DIAGNOSTIC_CAUSE_ARITY_MISMATCH,
    VITTE_CLI_DIAGNOSTIC_CAUSE_CONSTRAINT,
    VITTE_CLI_DIAGNOSTIC_CAUSE_IMPORT,
    VITTE_CLI_DIAGNOSTIC_CAUSE_CONTRACT,
    VITTE_CLI_DIAGNOSTIC_CAUSE_CONSTANT_EVAL,
    VITTE_CLI_DIAGNOSTIC_CAUSE_UNSAFE_OPERATION,
    VITTE_CLI_DIAGNOSTIC_CAUSE_LOWERING,
    VITTE_CLI_DIAGNOSTIC_CAUSE_EXTERNAL_TOOL,
    VITTE_CLI_DIAGNOSTIC_CAUSE_LINK,
    VITTE_CLI_DIAGNOSTIC_CAUSE_RUNTIME,
    VITTE_CLI_DIAGNOSTIC_CAUSE_INTERNAL
} vitte_cli_diagnostic_cause_kind_t;

/* ========================================================================= */
/* Trace                                                                     */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_trace_kind {
    VITTE_CLI_DIAGNOSTIC_TRACE_CALL = 0,
    VITTE_CLI_DIAGNOSTIC_TRACE_INSTANTIATION,
    VITTE_CLI_DIAGNOSTIC_TRACE_CONSTRAINT,
    VITTE_CLI_DIAGNOSTIC_TRACE_IMPORT,
    VITTE_CLI_DIAGNOSTIC_TRACE_EXPANSION,
    VITTE_CLI_DIAGNOSTIC_TRACE_LOWERING,
    VITTE_CLI_DIAGNOSTIC_TRACE_GENERATED_CODE,
    VITTE_CLI_DIAGNOSTIC_TRACE_RELATED
} vitte_cli_diagnostic_trace_kind_t;

/* ========================================================================= */
/* Contract                                                                  */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_contract_kind {
    VITTE_CLI_DIAGNOSTIC_CONTRACT_NONE = 0,
    VITTE_CLI_DIAGNOSTIC_CONTRACT_REQUIRES,
    VITTE_CLI_DIAGNOSTIC_CONTRACT_ENSURES,
    VITTE_CLI_DIAGNOSTIC_CONTRACT_INVARIANT,
    VITTE_CLI_DIAGNOSTIC_CONTRACT_ASSERTION
} vitte_cli_diagnostic_contract_kind_t;

/* ========================================================================= */
/* Span                                                                      */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_span {
    bool valid;

    uint32_t file_id;
    vitte_source_id_t source_id;

    const char *source_name;

    size_t start_offset;
    size_t end_offset;

    size_t start_line;
    size_t start_column;

    size_t end_line;
    size_t end_column;
} vitte_cli_diagnostic_span_t;

/* ========================================================================= */
/* Label                                                                     */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_label {
    vitte_cli_diagnostic_label_style_t style;

    vitte_cli_diagnostic_span_t span;

    const char *message;
} vitte_cli_diagnostic_label_t;

/* ========================================================================= */
/* Related location                                                          */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_location {
    vitte_cli_diagnostic_location_kind_t kind;

    vitte_cli_diagnostic_span_t span;

    const char *label;
} vitte_cli_diagnostic_location_t;

/* ========================================================================= */
/* Cause                                                                     */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_cause {
    vitte_cli_diagnostic_cause_kind_t kind;

    const char *message;

    bool has_span;
    vitte_cli_diagnostic_span_t span;

    vitte_cli_diagnostic_confidence_t confidence;
} vitte_cli_diagnostic_cause_t;

/* ========================================================================= */
/* Annotation                                                                */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_annotation {
    vitte_cli_diagnostic_annotation_kind_t kind;

    const char *message;

    bool has_span;
    vitte_cli_diagnostic_span_t span;
} vitte_cli_diagnostic_annotation_t;

/* ========================================================================= */
/* Fix part                                                                  */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_fix_part {
    vitte_cli_diagnostic_span_t span;

    /*
     * Empty string is meaningful and represents deletion.
     *
     * NULL means that no replacement is available.
     */
    const char *replacement;
} vitte_cli_diagnostic_fix_part_t;

/* ========================================================================= */
/* Suggestion                                                                */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_suggestion {
    const char *message;

    vitte_cli_diagnostic_applicability_t applicability;
    vitte_cli_diagnostic_confidence_t confidence;

    bool preferred;

    uint64_t suggestion_id;

    /*
     * Simple single-range fix.
     */
    bool has_span;
    vitte_cli_diagnostic_span_t span;

    bool has_replacement;
    const char *replacement;

    /*
     * Optional multi-part fix.
     *
     * Example:
     *
     *   insert '('
     *   insert ')'
     *
     * or coordinated rename edits.
     */
    const vitte_cli_diagnostic_fix_part_t *parts;
    size_t part_count;
} vitte_cli_diagnostic_suggestion_t;

/* ========================================================================= */
/* Causal trace                                                              */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_trace {
    vitte_cli_diagnostic_trace_kind_t kind;

    const char *message;

    bool has_span;
    vitte_cli_diagnostic_span_t span;
} vitte_cli_diagnostic_trace_t;

/* ========================================================================= */
/* Contract provenance                                                       */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_contract {
    vitte_cli_diagnostic_contract_kind_t kind;

    const char *expression;
    const char *procedure;
    const char *reason;

    bool has_contract_span;
    vitte_cli_diagnostic_span_t contract_span;

    bool has_call_span;
    vitte_cli_diagnostic_span_t call_span;

    bool has_declaration_span;
    vitte_cli_diagnostic_span_t declaration_span;
} vitte_cli_diagnostic_contract_t;

/* ========================================================================= */
/* External tool provenance                                                  */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_external_tool {
    bool present;

    const char *tool;
    const char *rule;

    /*
     * Optional original external diagnostic code.
     *
     * Examples:
     *
     *   clang diagnostic name
     *   linker code
     *   platform toolchain code
     */
    const char *external_code;

    /*
     * Optional original message retained for structured/debug output.
     */
    const char *original_message;
} vitte_cli_diagnostic_external_tool_t;

/* ========================================================================= */
/* Diagnostic                                                               */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic {
    /* --------------------------------------------------------------------- */
    /* Identity                                                              */
    /* --------------------------------------------------------------------- */

    uint64_t diagnostic_id;
    uint64_t fingerprint;

    const char *code;
    const char *short_code;
    const char *category;

    /* --------------------------------------------------------------------- */
    /* Classification                                                        */
    /* --------------------------------------------------------------------- */

    vitte_cli_diagnostic_severity_t severity;
    vitte_cli_diagnostic_origin_t origin;

    /* --------------------------------------------------------------------- */
    /* Main message                                                          */
    /* --------------------------------------------------------------------- */

    const char *message;
    const char *details;

    /*
     * Compatibility storage retained for callers that build details locally.
     */
    char details_storage[
        VITTE_CLI_DIAGNOSTIC_DETAILS_CAPACITY
    ];

    /* --------------------------------------------------------------------- */
    /* Primary source span                                                   */
    /* --------------------------------------------------------------------- */

    bool has_span;

    uint32_t file_id;

    vitte_source_id_t source_id;
    const char *source_name;

    size_t start_offset;
    size_t end_offset;

    size_t start_line;
    size_t start_column;

    size_t end_line;
    size_t end_column;

    /* --------------------------------------------------------------------- */
    /* Structured semantic information                                       */
    /* --------------------------------------------------------------------- */

    const char *symbol;

    /*
     * Legacy/specialized type information.
     */
    const char *expected_type;
    const char *actual_type;

    /*
     * Generic expected/found information.
     *
     * These fields are not restricted to types.
     *
     * Examples:
     *
     *   expected = "`;`"
     *   found    = "`}`"
     *
     *   expected = "2 arguments"
     *   found    = "3 arguments"
     *
     *   expected = "integer"
     *   found    = "string"
     */
    const char *expected;
    const char *found;

    /*
     * Human-readable probable cause.
     */
    const char *probable_cause;

    /* --------------------------------------------------------------------- */
    /* Compilation context                                                   */
    /* --------------------------------------------------------------------- */

    const char *package;
    const char *module;
    const char *procedure;
    const char *context;

    /* --------------------------------------------------------------------- */
    /* Labels                                                                */
    /* --------------------------------------------------------------------- */

    const vitte_cli_diagnostic_label_t *labels;
    size_t label_count;

    /* --------------------------------------------------------------------- */
    /* Related source locations                                              */
    /* --------------------------------------------------------------------- */

    const vitte_cli_diagnostic_location_t *related;
    size_t related_count;

    /* --------------------------------------------------------------------- */
    /* Causes                                                                */
    /* --------------------------------------------------------------------- */

    const vitte_cli_diagnostic_cause_t *causes;
    size_t cause_count;

    /* --------------------------------------------------------------------- */
    /* Notes/help                                                            */
    /* --------------------------------------------------------------------- */

    const vitte_cli_diagnostic_annotation_t *annotations;
    size_t annotation_count;

    /*
     * Compatibility shortcut for callers exposing only one help message.
     */
    const char *help;

    /* --------------------------------------------------------------------- */
    /* Suggestions / fix-its                                                 */
    /* --------------------------------------------------------------------- */

    const vitte_cli_diagnostic_suggestion_t *suggestions;
    size_t suggestion_count;

    bool has_machine_fix;

    /* --------------------------------------------------------------------- */
    /* Causal trace                                                          */
    /* --------------------------------------------------------------------- */

    const vitte_cli_diagnostic_trace_t *trace;
    size_t trace_count;

    /* --------------------------------------------------------------------- */
    /* Root / parent relationships                                           */
    /* --------------------------------------------------------------------- */

    size_t parent_diagnostic;
    size_t root_diagnostic;

    uint64_t parent_id;
    uint64_t root_id;

    bool is_primary;
    bool is_cascade;
    bool is_suppressed;

    /* --------------------------------------------------------------------- */
    /* Compiler propagation                                                  */
    /* --------------------------------------------------------------------- */

    /*
     * Indicates that the underlying compiler object is poisoned by an
     * earlier failure and should not generate independent downstream errors.
     */
    bool poisons_analysis;

    /*
     * Marks compiler-internal diagnostics.
     */
    bool is_internal;

    /* --------------------------------------------------------------------- */
    /* Provenance                                                            */
    /* --------------------------------------------------------------------- */

    uint64_t provenance_id;
    uint64_t poison_id;

    /* --------------------------------------------------------------------- */
    /* Contract                                                              */
    /* --------------------------------------------------------------------- */

    bool has_contract;
    vitte_cli_diagnostic_contract_t contract;

    /* --------------------------------------------------------------------- */
    /* External tool                                                        */
    /* --------------------------------------------------------------------- */

    vitte_cli_diagnostic_external_tool_t external_tool;
} vitte_cli_diagnostic_t;

/* ========================================================================= */
/* Rendering options                                                         */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_render_options {
    vitte_cli_diagnostic_format_t format;

    bool color;

    bool show_source;
    bool show_source_line;
    bool show_codes;
    bool show_details;
    bool show_origin;
    bool show_category;

    bool show_labels;
    bool show_expected_found;
    bool show_causes;
    bool show_notes;
    bool show_help;
    bool show_related;
    bool show_suggestions;
    bool show_trace;

    bool show_suppressed;
    bool show_cascades;
    bool show_internal;

    bool show_summary;

    size_t context_lines_before;
    size_t context_lines_after;

    /* LSP serialization uses the same canonical diagnostic bag. */
    int position_encoding;
    vitte_cli_lsp_position_converter_t position_converter;
    void *position_user_data;
    vitte_cli_lsp_uri_converter_t uri_converter;
    void *uri_user_data;
    bool pretty;
    bool trailing_newline;
} vitte_cli_diagnostic_render_options_t;

/* ========================================================================= */
/* Batch statistics                                                          */
/* ========================================================================= */

typedef struct vitte_cli_diagnostic_statistics {
    size_t total;

    size_t note_count;
    size_t help_count;
    size_t warning_count;
    size_t error_count;
    size_t fatal_count;

    size_t primary_count;
    size_t cascade_count;
    size_t suppressed_count;

    size_t machine_fix_count;
} vitte_cli_diagnostic_statistics_t;

/* ========================================================================= */
/* Initialization helpers                                                    */
/* ========================================================================= */

void
vitte_cli_diagnostic_init(
    vitte_cli_diagnostic_t *diagnostic
);

/*
 * Opaque host allocation boundary for diagnostics produced by native Vitte.
 * The native frontend supplies scalar fields only. These helpers construct
 * the bootstrap C layout consumed by the existing CLI renderers.
 */
void *
vitte_cli_diagnostic_transport_create(
    size_t count
);

bool
vitte_cli_diagnostic_transport_set(
    void *batch,
    size_t count,
    size_t index,
    uint64_t diagnostic_id,
    const char *code,
    uint32_t severity,
    uint32_t origin,
    const char *message,
    const char *details,
    const char *help,
    bool has_span,
    uint32_t file_id,
    uint64_t source_id,
    const char *source_name,
    size_t start_offset,
    size_t end_offset,
    size_t start_line,
    size_t start_column,
    size_t end_line,
    size_t end_column,
    const char *expected,
    const char *found
);

bool
vitte_cli_diagnostic_transport_set_semantic(
    void *batch,
    size_t count,
    size_t index,
    const char *symbol,
    const char *probable_cause,
    const char *context,
    bool has_related,
    uint32_t related_file_id,
    uint64_t related_source_id,
    const char *related_source_name,
    size_t related_start_offset,
    size_t related_end_offset,
    size_t related_start_line,
    size_t related_start_column,
    size_t related_end_line,
    size_t related_end_column,
    const char *related_label
);

void
vitte_cli_diagnostic_transport_destroy(
    void *batch
);

void
vitte_cli_diagnostic_span_init(
    vitte_cli_diagnostic_span_t *span
);

void
vitte_cli_diagnostic_render_options_init(
    vitte_cli_diagnostic_render_options_t *options
);

void
vitte_cli_diagnostic_statistics_init(
    vitte_cli_diagnostic_statistics_t *statistics
);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_cli_diagnostic_severity_is_valid(
    vitte_cli_diagnostic_severity_t severity
);

bool
vitte_cli_diagnostic_format_is_valid(
    vitte_cli_diagnostic_format_t format
);

bool
vitte_cli_diagnostic_origin_is_valid(
    vitte_cli_diagnostic_origin_t origin
);

bool
vitte_cli_diagnostic_span_is_valid(
    const vitte_cli_diagnostic_span_t *span
);

bool
vitte_cli_diagnostic_validate(
    const vitte_cli_diagnostic_t *diagnostic
);

bool
vitte_cli_diagnostic_validate_batch(
    const vitte_cli_diagnostic_t *diagnostics,
    size_t diagnostic_count
);

/* ========================================================================= */
/* Metadata names                                                            */
/* ========================================================================= */

const char *
vitte_cli_diagnostic_severity_name(
    vitte_cli_diagnostic_severity_t severity
);

const char *
vitte_cli_diagnostic_format_name(
    vitte_cli_diagnostic_format_t format
);

const char *
vitte_cli_diagnostic_origin_name(
    vitte_cli_diagnostic_origin_t origin
);

const char *
vitte_cli_diagnostic_applicability_name(
    vitte_cli_diagnostic_applicability_t applicability
);

const char *
vitte_cli_diagnostic_confidence_name(
    vitte_cli_diagnostic_confidence_t confidence
);

const char *
vitte_cli_diagnostic_cause_kind_name(
    vitte_cli_diagnostic_cause_kind_t kind
);

const char *
vitte_cli_diagnostic_trace_kind_name(
    vitte_cli_diagnostic_trace_kind_t kind
);

/* ========================================================================= */
/* Registry / explain bridge                                                 */
/* ========================================================================= */

/*
 * Return true when code exists in the diagnostic registry.
 *
 * Examples:
 *
 *   E0302
 *   W0104
 *   N0001
 *   H0001
 */
bool
vitte_diagnostic_cli_code_exists(
    const char *code
);

/*
 * Render the long-form explanation for a registered diagnostic.
 *
 * Intended CLI use:
 *
 *   vittec --explain E0302
 *
 * json == false:
 *     human-readable explanation
 *
 * json == true:
 *     structured JSON explanation
 */
bool
vitte_diagnostic_explain_cli(
    FILE *stream,
    const char *code,
    bool json
);

/* ========================================================================= */
/* Statistics                                                               */
/* ========================================================================= */

bool
vitte_diagnostic_cli_collect_statistics(
    const vitte_cli_diagnostic_t *diagnostics,
    size_t diagnostic_count,
    vitte_cli_diagnostic_statistics_t *statistics
);

/* ========================================================================= */
/* Rich batch rendering                                                      */
/* ========================================================================= */

/*
 * Render a complete diagnostic batch.
 *
 * This is the preferred rendering API.
 *
 * All structured information available in vitte_cli_diagnostic_t may be
 * consumed by the selected renderer.
 *
 * Terminal:
 *
 *   error[E0302]: incompatible type
 *     --> src/main.vit:12:9
 *      |
 *   12 |     set value = "hello";
 *      |         -----   ^^^^^^^ expected `i32`, found `str`
 *      |         |
 *      |         `value` was declared as `i32`
 *      |
 *      = cause: this assignment changes the value's type
 *      = note: `value` was declared here
 *      = help: convert the string before assigning it
 *
 * JSON:
 *
 *   exposes all structured fields including labels, related locations,
 *   causes, fix-its, confidence and causal relationships.
 *
 * SARIF:
 *
 *   exposes rule/code information, locations, related locations and fixes.
 *
 * LSP:
 *
 *   exposes diagnostics, relatedInformation and data needed for code actions.
 */
bool
vitte_diagnostic_render_cli_batch_ex(
    FILE *stream,
    const vitte_source_context_t *sources,
    vitte_source_id_t source_id,
    const char *source_name,
    const vitte_cli_diagnostic_t *diagnostics,
    size_t diagnostic_count,
    const vitte_cli_diagnostic_render_options_t *options
);

/*
 * Compatibility batch renderer.
 *
 * Equivalent to render_cli_batch_ex() with default rendering options derived
 * from color and format.
 */
bool
vitte_diagnostic_render_cli_batch(
    FILE *stream,
    const vitte_source_context_t *sources,
    vitte_source_id_t source_id,
    const char *source_name,
    const vitte_cli_diagnostic_t *diagnostics,
    size_t diagnostic_count,
    bool color,
    vitte_cli_diagnostic_format_t format
);

/* ========================================================================= */
/* Single structured diagnostic rendering                                    */
/* ========================================================================= */

/*
 * Render one already-structured CLI diagnostic.
 *
 * Prefer this API over the legacy flattened function when the producer has
 * expected/found, labels, related locations, causes or suggestions.
 */
bool
vitte_diagnostic_render_cli_diagnostic(
    FILE *stream,
    const vitte_source_context_t *sources,
    vitte_source_id_t source_id,
    const char *source_name,
    const vitte_cli_diagnostic_t *diagnostic,
    const vitte_cli_diagnostic_render_options_t *options
);

/* ========================================================================= */
/* Legacy flattened rendering                                                */
/* ========================================================================= */

/*
 * Compatibility entry point for simple producers.
 *
 * New compiler phases should normally construct vitte_cli_diagnostic_t and
 * call vitte_diagnostic_render_cli_diagnostic() or
 * vitte_diagnostic_render_cli_batch_ex().
 *
 * This API intentionally remains available so migration of older compiler
 * components does not require an all-at-once rewrite.
 */
bool
vitte_diagnostic_render_cli(
    FILE *stream,
    const vitte_source_context_t *sources,
    vitte_source_id_t source_id,
    const char *source_name,
    vitte_cli_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    bool has_span,
    uint32_t file_id,
    size_t start_offset,
    size_t end_offset,
    size_t start_line,
    size_t start_column,
    size_t end_line,
    size_t end_column,
    const char *procedure,
    const char *context,
    bool color,
    vitte_cli_diagnostic_format_t format
);

/* ========================================================================= */
/* Recommended producer contract                                             */
/* ========================================================================= */

/*
 * Compiler producers should provide as much structured information as they
 * actually know.
 *
 * Lexer:
 *
 *   code
 *   exact span
 *   unexpected input
 *   expected lexical form
 *   recovery information
 *
 * Parser:
 *
 *   expected token
 *   found token
 *   primary label
 *   probable cause
 *   high-confidence missing-token fix-it
 *   parser recovery provenance
 *
 * Name resolution:
 *
 *   unresolved symbol
 *   declaration candidates
 *   similar-symbol suggestions
 *   import/module context
 *
 * Type checker:
 *
 *   expected type
 *   actual type
 *   declaration location
 *   use location
 *   generic/constraint trace
 *
 * Constant evaluator:
 *
 *   expression
 *   failure cause
 *   originating declaration
 *
 * Contract checker:
 *
 *   contract kind
 *   expression
 *   declaration
 *   call site
 *   failure reason
 *
 * HIR/IR:
 *
 *   preserve causal provenance from frontend errors;
 *   avoid generating independent diagnostics for poisoned nodes.
 *
 * C17 backend:
 *
 *   generated-code provenance
 *   original Vitte source location
 *
 * External C compiler:
 *
 *   tool
 *   external rule/code
 *   generated C location
 *   originating Vitte location
 *
 * Linker:
 *
 *   unresolved symbol
 *   object/module provenance
 *   external tool message
 *
 * -------------------------------------------------------------------------
 * Intelligent diagnostic principle
 * -------------------------------------------------------------------------
 *
 * Renderers must not invent semantic explanations.
 *
 * The producer that understands the failure should populate:
 *
 *   expected/found
 *   probable cause
 *   related declaration
 *   causal parent
 *   trace
 *   suggestion
 *   fix-it
 *   confidence
 *
 * The bridge transports those facts.
 *
 * The renderer presents them.
 */

/* ========================================================================= */
/* Lifetime                                                                  */
/* ========================================================================= */

/*
 * Pointer fields in vitte_cli_diagnostic_t and nested structures are borrowed
 * unless the producer explicitly owns corresponding storage.
 *
 * Every referenced object must remain alive for the complete rendering call.
 *
 * In particular:
 *
 *   labels
 *   related
 *   causes
 *   annotations
 *   suggestions
 *   suggestion parts
 *   trace
 *   strings
 *
 * must not refer to expired stack storage when diagnostics are accumulated for
 * later batch rendering.
 *
 * The diagnostic subsystem may provide an owning conversion layer from
 * vitte_diagnostic_t to vitte_cli_diagnostic_t when longer lifetime is needed.
 */

/* ========================================================================= */
/* Machine-readable fix semantics                                            */
/* ========================================================================= */

/*
 * Fix ranges are byte-offset half-open ranges:
 *
 *   [start_offset, end_offset)
 *
 * replacement == "":
 *
 *   delete the range.
 *
 * zero-width range + non-empty replacement:
 *
 *   insert text.
 *
 * non-zero range + non-empty replacement:
 *
 *   replace text.
 *
 * has_replacement == false:
 *
 *   suggestion is descriptive only and must not be applied automatically.
 *
 * MACHINE applicability should only be emitted when the compiler can apply
 * the fix without changing program intent beyond the diagnosed correction.
 *
 * Confidence and applicability are separate concepts.
 *
 * Example:
 *
 *   applicability = MACHINE
 *   confidence     = CERTAIN
 *
 * is suitable for automatic application.
 *
 * A MAYBE or LOW-confidence suggestion should normally require explicit user
 * selection.
 */

/* ========================================================================= */
/* Coordinate convention                                                     */
/* ========================================================================= */

/*
 * Source ranges use byte offsets internally.
 *
 * start_offset is inclusive.
 * end_offset is exclusive.
 *
 * Lines and columns transported by this bridge use the compiler diagnostic
 * subsystem's canonical zero-based internal convention.
 *
 * Human terminal renderers may convert them to one-based display values.
 *
 * LSP renderers must perform the required coordinate conversion, including
 * UTF-16 code-unit conversion when required by the protocol.
 */

/* ========================================================================= */
/* Testing                                                                   */
/* ========================================================================= */

/*
 * Bridge and renderer tests should cover:
 *
 *   - every severity;
 *   - every origin;
 *   - code/category rendering;
 *   - diagnostics without spans;
 *   - zero-width spans;
 *   - multiline spans;
 *   - Unicode source;
 *   - expected/found;
 *   - labels;
 *   - multiple secondary labels;
 *   - probable causes;
 *   - notes;
 *   - help;
 *   - related declarations;
 *   - suggestions;
 *   - insertion fix-it;
 *   - replacement fix-it;
 *   - deletion fix-it with empty replacement;
 *   - multi-part fixes;
 *   - applicability;
 *   - confidence;
 *   - preferred suggestions;
 *   - causal traces;
 *   - parent/root relationships;
 *   - cascades;
 *   - suppressed diagnostics;
 *   - contract provenance;
 *   - external C compiler provenance;
 *   - linker provenance;
 *   - terminal output golden tests;
 *   - JSON golden tests;
 *   - SARIF golden tests;
 *   - LSP golden tests;
 *   - --explain output;
 *   - malformed diagnostic rejection;
 *   - fuzzed spans;
 *   - very large diagnostic batches.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_CLI_BRIDGE_H */
