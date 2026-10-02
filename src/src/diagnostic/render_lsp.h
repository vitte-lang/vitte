#ifndef VITTE_DIAGNOSTIC_RENDER_LSP_H
#define VITTE_DIAGNOSTIC_RENDER_LSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Vitte Language Server Protocol diagnostic renderer.
 *
 * This module converts structured compiler diagnostics into LSP-compatible
 * JSON.
 *
 * Supported output forms:
 *
 *     - one LSP Diagnostic;
 *     - an array of Diagnostic objects;
 *     - PublishDiagnosticsParams;
 *     - textDocument/publishDiagnostics JSON-RPC notification.
 *
 * Position conversion is intentionally delegated to the source/LSP layer.
 *
 * Vitte compiler spans use byte offsets while LSP positions normally use
 * UTF-16 code units. The renderer must therefore never assume that:
 *
 *     byte_offset == LSP character
 *
 * because that fails for UTF-8 source containing non-ASCII characters.
 */

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_DIAGNOSTIC_LSP_URI_CAPACITY ((size_t)4096u)

/* ========================================================================= */
/* Position encoding                                                         */
/* ========================================================================= */

/*
 * LSP position encoding negotiated between the client and server.
 *
 * UTF-16 is the historical/default LSP encoding and remains the recommended
 * default unless positionEncoding negotiation selects another encoding.
 */
typedef enum vitte_diagnostic_lsp_position_encoding {
    VITTE_DIAGNOSTIC_LSP_POSITION_UTF8 = 0,
    VITTE_DIAGNOSTIC_LSP_POSITION_UTF16,
    VITTE_DIAGNOSTIC_LSP_POSITION_UTF32
} vitte_diagnostic_lsp_position_encoding_t;

/*
 * Return the protocol spelling:
 *
 *     "utf-8"
 *     "utf-16"
 *     "utf-32"
 *
 * Unknown values return "unknown".
 */
const char *
vitte_diagnostic_lsp_position_encoding_name(
    vitte_diagnostic_lsp_position_encoding_t encoding
);

/* ========================================================================= */
/* Position                                                                  */
/* ========================================================================= */

/*
 * Zero-based LSP source position.
 *
 * line
 *     Zero-based source line.
 *
 * character
 *     Zero-based character offset according to the negotiated position
 *     encoding.
 *
 * For UTF-16 this is a count of UTF-16 code units, not Unicode scalar values
 * and not UTF-8 bytes.
 */
typedef struct vitte_diagnostic_lsp_position {
    size_t line;
    size_t character;
    bool valid;
} vitte_diagnostic_lsp_position_t;

/* ========================================================================= */
/* Range                                                                     */
/* ========================================================================= */

/*
 * LSP Range.
 *
 * Both endpoints use zero-based LSP positions.
 *
 * The end position is exclusive.
 */
typedef struct vitte_diagnostic_lsp_range {
    vitte_diagnostic_lsp_position_t start;
    vitte_diagnostic_lsp_position_t end;
    bool valid;
} vitte_diagnostic_lsp_range_t;

/* ========================================================================= */
/* Position conversion callback                                              */
/* ========================================================================= */

/*
 * Convert one Vitte source byte offset into an LSP position.
 *
 * Parameters:
 *
 *     source_id
 *         Canonical source-manager identifier when available.
 *
 *     source_name
 *         Source path/name fallback.
 *
 *     byte_offset
 *         Byte offset in the original Vitte source.
 *
 *     encoding
 *         Requested LSP position encoding.
 *
 *     position
 *         Output position.
 *
 *     user_data
 *         Opaque source-manager/LSP context.
 *
 * On success:
 *
 *     - return true;
 *     - position->line is zero-based;
 *     - position->character is zero-based;
 *     - position->valid is true.
 *
 * On failure return false.
 *
 * A typical implementation delegates to vitte_source_manager_t.
 */
typedef bool
(*vitte_diagnostic_lsp_position_converter_t)(
    vitte_source_id_t source_id,
    const char *source_name,
    size_t byte_offset,
    vitte_diagnostic_lsp_position_encoding_t encoding,
    vitte_diagnostic_lsp_position_t *position,
    void *user_data
);

/* ========================================================================= */
/* URI conversion callback                                                   */
/* ========================================================================= */

/*
 * Convert a Vitte source identifier/path into an LSP document URI.
 *
 * Typical result:
 *
 *     file:///Users/vincent/project/main.vit
 *
 * The callback is responsible for:
 *
 *     - absolute path handling;
 *     - file URI construction;
 *     - percent encoding;
 *     - platform-specific path handling;
 *     - virtual/generated source URIs;
 *     - workspace-specific URI policy.
 *
 * The renderer deliberately does not prepend "file://" itself because doing
 * so without proper URI/path normalization can generate invalid LSP URIs.
 *
 * Return true when buffer contains a valid URI.
 */
typedef bool
(*vitte_diagnostic_lsp_uri_converter_t)(
    vitte_source_id_t source_id,
    const char *source_name,
    char *buffer,
    size_t capacity,
    void *user_data
);

/* ========================================================================= */
/* Options                                                                   */
/* ========================================================================= */

typedef struct vitte_diagnostic_lsp_options {
    /*
     * Pretty-print JSON.
     *
     * Normally false for JSON-RPC transport.
     */
    bool pretty;

    /*
     * Append '\n' after the complete serialized value.
     */
    bool trailing_newline;

    /*
     * Include diagnostics marked as suppressed.
     *
     * Normally false.
     */
    bool include_suppressed;

    /*
     * Emit Diagnostic.relatedInformation.
     */
    bool include_related_information;

    /*
     * Emit the optional LSP Diagnostic.data object containing Vitte-specific
     * structured metadata.
     */
    bool include_data;

    /*
     * Include Vitte source labels inside Diagnostic.data.
     */
    bool include_labels_in_data;

    /*
     * Include fix suggestions inside Diagnostic.data.
     *
     * Actual LSP CodeAction generation should normally consume these
     * suggestions rather than requiring the client to interpret them itself.
     */
    bool include_suggestions_in_data;

    /*
     * Include structured cause-chain information inside Diagnostic.data.
     */
    bool include_causes_in_data;

    /*
     * Include Vitte contract metadata inside Diagnostic.data.
     */
    bool include_contract_in_data;

    /*
     * Include root/parent/suppression information inside Diagnostic.data.
     */
    bool include_cascade_in_data;

    /*
     * LSP Diagnostic.source.
     *
     * Default:
     *
     *     "vitte"
     *
     * The string is borrowed and must remain valid while rendering.
     */
    const char *source_name;

    /*
     * Position encoding negotiated with the LSP client.
     *
     * Default:
     *
     *     UTF-16
     */
    vitte_diagnostic_lsp_position_encoding_t position_encoding;

    /*
     * Required source-position converter.
     *
     * No unsafe byte-offset fallback is performed when this callback is NULL.
     */
    vitte_diagnostic_lsp_position_converter_t position_converter;

    /*
     * Opaque context passed to position_converter.
     */
    void *position_user_data;

    /*
     * Optional document URI converter.
     *
     * Strongly recommended for relatedInformation and normal LSP integration.
     */
    vitte_diagnostic_lsp_uri_converter_t uri_converter;

    /*
     * Opaque context passed to uri_converter.
     */
    void *uri_user_data;
} vitte_diagnostic_lsp_options_t;

/*
 * Initialize recommended defaults:
 *
 *     pretty                       = false
 *     trailing_newline             = true
 *     include_suppressed           = false
 *     include_related_information  = true
 *     include_data                 = true
 *     include_labels_in_data       = true
 *     include_suggestions_in_data  = true
 *     include_causes_in_data       = true
 *     include_contract_in_data     = true
 *     include_cascade_in_data      = true
 *     source_name                  = "vitte"
 *     position_encoding            = UTF-16
 *     position_converter           = NULL
 *     position_user_data           = NULL
 *     uri_converter                = NULL
 *     uri_user_data                = NULL
 *
 * A position converter must be installed before diagnostics with source
 * ranges can be emitted.
 */
void
vitte_diagnostic_lsp_options_init(
    vitte_diagnostic_lsp_options_t *options
);

/* ========================================================================= */
/* Severity                                                                  */
/* ========================================================================= */

/*
 * Convert Vitte severity to LSP DiagnosticSeverity:
 *
 *     Vitte fatal/error -> 1 Error
 *     Vitte warning     -> 2 Warning
 *     Vitte note        -> 3 Information
 *     Vitte help        -> 4 Hint
 */
int
vitte_diagnostic_lsp_severity(
    vitte_diagnostic_severity_t severity
);

/* ========================================================================= */
/* Single diagnostic                                                         */
/* ========================================================================= */

/*
 * Serialize one Vitte diagnostic as one LSP Diagnostic object.
 *
 * The position converter is required.
 *
 * Example:
 *
 * {
 *   "range": {
 *     "start": {
 *       "line": 4,
 *       "character": 8
 *     },
 *     "end": {
 *       "line": 4,
 *       "character": 13
 *     }
 *   },
 *   "severity": 1,
 *   "code": "E0501",
 *   "source": "vitte",
 *   "message": "type mismatch",
 *   "tags": [],
 *   "relatedInformation": [],
 *   "data": {
 *     "version": 1,
 *     "internalCode": "VITTE_TYPE_E_MISMATCH",
 *     "phase": "type-check",
 *     "category": "type",
 *     ...
 *   }
 * }
 *
 * Returns INVALID_STATE when the mandatory LSP range cannot be produced.
 */
vitte_status_t
vitte_diagnostic_render_lsp_one(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_lsp_options_t *options
);

/* ========================================================================= */
/* Diagnostic array                                                          */
/* ========================================================================= */

/*
 * Serialize matching diagnostics as an LSP Diagnostic[] JSON array.
 *
 * source_name:
 *
 *     non-empty
 *         only diagnostics whose diagnostic->source_name exactly matches are
 *         emitted.
 *
 *     NULL / empty
 *         diagnostics from all sources may be emitted.
 *
 * Diagnostics without a valid convertible LSP range are skipped because
 * Diagnostic.range is mandatory in LSP.
 */
vitte_status_t
vitte_diagnostic_render_lsp_array(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    const char *source_name,
    const vitte_diagnostic_lsp_options_t *options
);

/* ========================================================================= */
/* PublishDiagnosticsParams                                                  */
/* ========================================================================= */

/*
 * Serialize the params object used by:
 *
 *     textDocument/publishDiagnostics
 *
 * Result:
 *
 * {
 *   "uri": "file:///workspace/main.vit",
 *   "diagnostics": [
 *     ...
 *   ]
 * }
 *
 * uri must already be a valid LSP document URI.
 */
vitte_status_t
vitte_diagnostic_render_lsp_publish(
    FILE *stream,
    const char *uri,
    const char *source_name,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_lsp_options_t *options
);

/* ========================================================================= */
/* JSON-RPC notification                                                     */
/* ========================================================================= */

/*
 * Serialize a complete JSON-RPC notification:
 *
 * {
 *   "jsonrpc": "2.0",
 *   "method": "textDocument/publishDiagnostics",
 *   "params": {
 *     "uri": "file:///workspace/main.vit",
 *     "diagnostics": [
 *       ...
 *     ]
 *   }
 * }
 *
 * This function serializes the JSON-RPC message body only.
 *
 * Transport framing, if required by the LSP server implementation, belongs to
 * the transport layer.
 */
vitte_status_t
vitte_diagnostic_render_lsp_notification(
    FILE *stream,
    const char *uri,
    const char *source_name,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_lsp_options_t *options
);

/* ========================================================================= */
/* Compatibility adapter                                                     */
/* ========================================================================= */

/*
 * Generic diagnostic-rendering adapter.
 */
vitte_status_t
vitte_diagnostic_write_lsp(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_lsp_options_t *options
);

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

/*
 * Version of the Vitte-specific object stored in Diagnostic.data.
 *
 * This is independent from the LSP protocol version itself.
 */
unsigned
vitte_diagnostic_lsp_data_version(void);

/*
 * Default Diagnostic.source value.
 *
 * Currently:
 *
 *     "vitte"
 */
const char *
vitte_diagnostic_lsp_source_name(void);

/* ========================================================================= */
/* Convenience                                                               */
/* ========================================================================= */

/*
 * Configure a position converter and its context.
 */
static inline void
vitte_diagnostic_lsp_set_position_converter(
    vitte_diagnostic_lsp_options_t *options,
    vitte_diagnostic_lsp_position_converter_t converter,
    void *user_data
)
{
    if (options == NULL) {
        return;
    }

    options->position_converter = converter;
    options->position_user_data = user_data;
}

/*
 * Configure a URI converter and its context.
 */
static inline void
vitte_diagnostic_lsp_set_uri_converter(
    vitte_diagnostic_lsp_options_t *options,
    vitte_diagnostic_lsp_uri_converter_t converter,
    void *user_data
)
{
    if (options == NULL) {
        return;
    }

    options->uri_converter = converter;
    options->uri_user_data = user_data;
}

/*
 * Select UTF-8 LSP positions.
 */
static inline void
vitte_diagnostic_lsp_use_utf8(
    vitte_diagnostic_lsp_options_t *options
)
{
    if (options != NULL) {
        options->position_encoding =
            VITTE_DIAGNOSTIC_LSP_POSITION_UTF8;
    }
}

/*
 * Select UTF-16 LSP positions.
 */
static inline void
vitte_diagnostic_lsp_use_utf16(
    vitte_diagnostic_lsp_options_t *options
)
{
    if (options != NULL) {
        options->position_encoding =
            VITTE_DIAGNOSTIC_LSP_POSITION_UTF16;
    }
}

/*
 * Select UTF-32 LSP positions.
 */
static inline void
vitte_diagnostic_lsp_use_utf32(
    vitte_diagnostic_lsp_options_t *options
)
{
    if (options != NULL) {
        options->position_encoding =
            VITTE_DIAGNOSTIC_LSP_POSITION_UTF32;
    }
}

/* ========================================================================= */
/* Position rules                                                            */
/* ========================================================================= */

/*
 * LSP line and character positions are zero-based.
 *
 *
 * UTF-8
 * -----
 *
 * character counts UTF-8 code units (bytes).
 *
 *
 * UTF-16
 * ------
 *
 * character counts UTF-16 code units.
 *
 * A Unicode code point outside the Basic Multilingual Plane therefore counts
 * as two UTF-16 code units.
 *
 * Example:
 *
 *     source: "a😀b"
 *
 * UTF-8 byte offsets:
 *
 *     a  -> 0
 *     😀 -> 1
 *     b  -> 5
 *
 * UTF-16 character positions:
 *
 *     a  -> 0
 *     😀 -> 1
 *     b  -> 3
 *
 * Consequently a raw Vitte byte offset must not be emitted as an LSP UTF-16
 * character position.
 *
 *
 * UTF-32
 * ------
 *
 * character counts Unicode scalar/code-point units according to the position
 * encoding contract implemented by the source layer.
 */

/* ========================================================================= */
/* Source-manager integration                                                */
/* ========================================================================= */

/*
 * Recommended architecture:
 *
 *     Vitte diagnostic
 *           |
 *           | SourceId + byte offset
 *           v
 *     vitte_source_manager_t
 *           |
 *           | line lookup
 *           | UTF decoding
 *           | negotiated position encoding
 *           v
 *     vitte_diagnostic_lsp_position_t
 *           |
 *           v
 *     render_lsp.c
 *
 * The renderer therefore remains independent from source storage internals.
 *
 * A future source-manager adapter can expose:
 *
 *     bool vitte_source_lsp_position_converter(...);
 *
 * and be installed with:
 *
 *     vitte_diagnostic_lsp_set_position_converter(
 *         &options,
 *         vitte_source_lsp_position_converter,
 *         source_manager
 *     );
 */

/* ========================================================================= */
/* URI rules                                                                 */
/* ========================================================================= */

/*
 * Source paths are not automatically valid LSP URIs.
 *
 * Incorrect:
 *
 *     /Users/name/project/main.vit
 *
 * Expected:
 *
 *     file:///Users/name/project/main.vit
 *
 * Paths may also contain:
 *
 *     - spaces;
 *     - '#';
 *     - '?';
 *     - Unicode;
 *     - Windows drive letters;
 *     - UNC paths.
 *
 * URI construction should therefore be performed by the LSP/source layer
 * rather than by blindly prefixing "file://".
 *
 * Virtual sources can use another URI scheme if the Vitte language server and
 * client agree on it.
 */

/* ========================================================================= */
/* Diagnostic.data                                                           */
/* ========================================================================= */

/*
 * Vitte uses LSP Diagnostic.data to preserve compiler-specific information
 * that does not have a direct standard LSP Diagnostic field.
 *
 * Current data can include:
 *
 *     version
 *     internalCode
 *     phase
 *     category
 *     fingerprint
 *     package
 *     module
 *     procedure
 *     symbol
 *     subject
 *     context
 *     expectedType
 *     actualType
 *     positionEncoding
 *     labels
 *     causes
 *     suggestions
 *     contract
 *     cascade
 *
 * This data is useful for:
 *
 *     - Vitte Studio;
 *     - code-action generation;
 *     - diagnostic inspection;
 *     - debugging;
 *     - richer editor UI;
 *     - correlation with compiler diagnostics.
 *
 * Consumers must use the "version" field before depending on the exact
 * extension shape.
 */

/* ========================================================================= */
/* Suggestions and Code Actions                                              */
/* ========================================================================= */

/*
 * Suggestions are intentionally preserved in Diagnostic.data, but applying
 * them belongs to the LSP CodeAction layer.
 *
 * Recommended flow:
 *
 *     vitte_diagnostic_suggestion_t
 *             |
 *             v
 *     source byte span
 *             |
 *             v
 *     source manager
 *             |
 *             v
 *     LSP Range
 *             |
 *             v
 *     TextEdit
 *             |
 *             v
 *     WorkspaceEdit
 *             |
 *             v
 *     CodeAction
 *
 * Only MACHINE_APPLICABLE suggestions should normally be offered as automatic
 * fixes without additional user review.
 */

/* ========================================================================= */
/* Related information                                                       */
/* ========================================================================= */

/*
 * Diagnostic.relatedInformation is generated from:
 *
 *     - secondary diagnostic labels;
 *     - explicit related locations.
 *
 * Typical examples:
 *
 *     error[E0402]: symbol `count` is already defined
 *
 *         primary:
 *             second declaration
 *
 *         related:
 *             first declaration
 *
 *
 *     error[E0602]: argument has incompatible type
 *
 *         primary:
 *             argument expression
 *
 *         related:
 *             parameter declaration
 */

/* ========================================================================= */
/* Suppression                                                               */
/* ========================================================================= */

/*
 * Cascade diagnostics marked is_suppressed are omitted by default.
 *
 * They can be included for diagnostic-engine debugging by setting:
 *
 *     options.include_suppressed = true;
 *
 * Cascade metadata can remain available independently through:
 *
 *     options.include_cascade_in_data = true;
 */

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

/*
 * The renderer borrows all inputs.
 *
 * It does not take ownership of:
 *
 *     - FILE streams;
 *     - diagnostic objects;
 *     - diagnostic bags;
 *     - option strings;
 *     - callback contexts;
 *     - source-manager objects.
 *
 * The supplied FILE stream is never closed by this module.
 */

/* ========================================================================= */
/* Failure contract                                                          */
/* ========================================================================= */

/*
 * Expected status values:
 *
 * VITTE_STATUS_OK
 *
 *     Rendering completed successfully.
 *
 *
 * VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *
 *     Required stream/diagnostic/bag/URI argument is invalid.
 *
 *
 * VITTE_STATUS_ERROR_INVALID_STATE
 *
 *     Position conversion failed;
 *     mandatory LSP range could not be produced;
 *     or output failed.
 *
 *
 * A missing position converter is considered INVALID_STATE rather than
 * silently degrading to incorrect coordinates.
 */

/* ========================================================================= */
/* Recommended initialization                                                */
/* ========================================================================= */

/*
 * Example:
 *
 *     vitte_diagnostic_lsp_options_t options;
 *
 *     vitte_diagnostic_lsp_options_init(&options);
 *
 *     options.position_encoding =
 *         VITTE_DIAGNOSTIC_LSP_POSITION_UTF16;
 *
 *     options.position_converter =
 *         my_source_position_converter;
 *
 *     options.position_user_data =
 *         source_manager;
 *
 *     options.uri_converter =
 *         my_source_uri_converter;
 *
 *     options.uri_user_data =
 *         source_manager;
 *
 *     vitte_diagnostic_render_lsp_notification(
 *         stdout,
 *         "file:///workspace/main.vit",
 *         "/workspace/main.vit",
 *         &diagnostics,
 *         &options
 *     );
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_RENDER_LSP_H */
