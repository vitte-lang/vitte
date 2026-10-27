#ifndef VITTE_DIAGNOSTIC_RENDER_SARIF_H
#define VITTE_DIAGNOSTIC_RENDER_SARIF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Vitte SARIF diagnostic renderer.
 *
 * Serializes structured Vitte compiler diagnostics using SARIF 2.1.0.
 *
 * Main structure:
 *
 *   sarifLog
 *     |
 *     +-- runs[]
 *           |
 *           +-- tool.driver
 *           |     |
 *           |     +-- rules[]
 *           |
 *           +-- results[]
 *                 |
 *                 +-- ruleId
 *                 +-- level
 *                 +-- message
 *                 +-- locations[]
 *                 +-- relatedLocations[]
 *                 +-- fixes[]
 *                 +-- partialFingerprints
 *                 +-- properties
 *
 * Vitte-specific metadata is stored under SARIF "properties" so the output
 * remains consumable by generic SARIF tools while preserving compiler
 * information required by Vitte tooling.
 */

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

/*
 * Maximum temporary URI size used by the renderer.
 *
 * This is storage used while serializing an artifact location; it does not
 * impose a permanent limit on source-manager path storage.
 */
#define VITTE_DIAGNOSTIC_SARIF_URI_CAPACITY ((size_t)4096u)

/* ========================================================================= */
/* URI conversion                                                            */
/* ========================================================================= */

/*
 * Convert a Vitte source into an artifact URI suitable for SARIF.
 *
 * Parameters:
 *
 *   source_id
 *       Canonical source-manager identifier when available.
 *
 *   source_name
 *       Original source path/name.
 *
 *   buffer
 *       Destination buffer.
 *
 *   capacity
 *       Destination capacity including the terminating NUL byte.
 *
 *   user_data
 *       Opaque caller/source-manager context.
 *
 * Return true when a valid URI was written.
 *
 * Typical output:
 *
 *   file:///Users/vincent/Documents/Github/vitte/example.vit
 *
 * The callback should handle:
 *
 *   - absolute paths;
 *   - URI percent encoding;
 *   - spaces;
 *   - '#';
 *   - '?';
 *   - Unicode paths;
 *   - Windows drive letters;
 *   - UNC paths;
 *   - virtual/generated sources.
 *
 * The renderer deliberately does not blindly prepend "file://".
 */
typedef bool
(*vitte_diagnostic_sarif_uri_converter_t)(
    vitte_source_id_t source_id,
    const char *source_name,
    char *buffer,
    size_t capacity,
    void *user_data
);

/* ========================================================================= */
/* Renderer options                                                          */
/* ========================================================================= */

typedef struct vitte_diagnostic_sarif_options {
    /*
     * Pretty-print generated JSON.
     *
     * Recommended for SARIF files intended for inspection.
     */
    bool pretty;

    /*
     * Append one newline after the complete SARIF document.
     */
    bool trailing_newline;

    /*
     * Include diagnostics that were classified as suppressed cascades.
     *
     * Normally false.
     */
    bool include_suppressed;

    /*
     * Emit reportingDescriptor entries under:
     *
     *   runs[].tool.driver.rules
     *
     * Rules are obtained from the Vitte diagnostic registry.
     */
    bool include_rules;

    /*
     * Emit secondary labels, related source locations and located causes as
     * SARIF relatedLocations.
     */
    bool include_related_locations;

    /*
     * Convert structured Vitte suggestions with source spans into SARIF fixes.
     */
    bool include_fixes;

    /*
     * Emit Vitte-specific structured metadata in result.properties.
     */
    bool include_properties;

    /*
     * Preserve the structured cause chain in properties and, when source
     * locations exist, relatedLocations.
     */
    bool include_causes;

    /*
     * Preserve Vitte contract metadata.
     */
    bool include_contract;

    /*
     * Preserve diagnostic cascade/root/parent/suppression information.
     */
    bool include_cascade;

    /*
     * SARIF tool.driver.name.
     *
     * Borrowed string.
     *
     * Default:
     *
     *   "Vitte"
     */
    const char *tool_name;

    /*
     * SARIF tool.driver.version.
     *
     * Borrowed string.
     *
     * NULL means the field is omitted.
     */
    const char *tool_version;

    /*
     * SARIF tool.driver.informationUri.
     *
     * Borrowed string.
     */
    const char *information_uri;

    /*
     * Optional source path -> URI converter.
     *
     * Strongly recommended for portable SARIF output.
     */
    vitte_diagnostic_sarif_uri_converter_t uri_converter;

    /*
     * Opaque context passed to uri_converter.
     */
    void *uri_user_data;
} vitte_diagnostic_sarif_options_t;

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

/*
 * Initialize SARIF options with Vitte defaults.
 *
 * Current defaults:
 *
 *   pretty                    = true
 *   trailing_newline          = true
 *   include_suppressed        = false
 *   include_rules             = true
 *   include_related_locations = true
 *   include_fixes             = true
 *   include_properties        = true
 *   include_causes            = true
 *   include_contract          = true
 *   include_cascade           = true
 *
 *   tool_name                 = "Vitte"
 *   tool_version              = NULL
 *   information_uri           = Vitte project URI
 *
 *   uri_converter             = NULL
 *   uri_user_data             = NULL
 */
void
vitte_diagnostic_sarif_options_init(
    vitte_diagnostic_sarif_options_t *options
);

/* ========================================================================= */
/* Severity                                                                  */
/* ========================================================================= */

/*
 * Convert Vitte severity into a SARIF result.level string.
 *
 * Mapping:
 *
 *   fatal   -> "error"
 *   error   -> "error"
 *   warning -> "warning"
 *   note    -> "note"
 *   help    -> "note"
 *   unknown -> "none"
 *
 * Returned strings have static lifetime.
 */
const char *
vitte_diagnostic_sarif_level(
    vitte_diagnostic_severity_t severity
);

/* ========================================================================= */
/* Complete SARIF log                                                        */
/* ========================================================================= */

/*
 * Serialize a complete SARIF 2.1.0 document.
 *
 * Typical output:
 *
 * {
 *   "$schema": "...",
 *   "version": "2.1.0",
 *   "runs": [
 *     {
 *       "tool": {
 *         "driver": {
 *           "name": "Vitte",
 *           "rules": [...]
 *         }
 *       },
 *       "results": [...]
 *     }
 *   ]
 * }
 *
 * Suppressed diagnostics are omitted unless:
 *
 *   options->include_suppressed == true
 *
 * The function borrows both stream and bag.
 */
vitte_status_t
vitte_diagnostic_render_sarif(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_sarif_options_t *options
);

/* ========================================================================= */
/* Single diagnostic                                                         */
/* ========================================================================= */

/*
 * Serialize one diagnostic as a complete valid SARIF log containing one run
 * and one result.
 *
 * This is useful for:
 *
 *   - tests;
 *   - snapshots;
 *   - debugging;
 *   - isolated diagnostic inspection.
 *
 * The diagnostic is borrowed and is not copied or modified.
 */
vitte_status_t
vitte_diagnostic_render_sarif_one(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_sarif_options_t *options
);

/* ========================================================================= */
/* Generic compatibility adapter                                             */
/* ========================================================================= */

/*
 * Serialize a diagnostic bag using default SARIF options.
 *
 * Equivalent conceptually to:
 *
 *   vitte_diagnostic_sarif_options_t options;
 *
 *   vitte_diagnostic_sarif_options_init(&options);
 *
 *   vitte_diagnostic_render_sarif(
 *       stream,
 *       bag,
 *       &options
 *   );
 */
vitte_status_t
vitte_diagnostic_write_sarif(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag
);

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

/*
 * Return the SARIF specification version emitted by this renderer.
 *
 * Current value:
 *
 *   "2.1.0"
 */
const char *
vitte_diagnostic_sarif_version(void);

/*
 * Return the JSON schema URI written to "$schema".
 */
const char *
vitte_diagnostic_sarif_schema(void);

/*
 * Return the default SARIF tool name.
 *
 * Current value:
 *
 *   "Vitte"
 */
const char *
vitte_diagnostic_sarif_tool_name(void);

/* ========================================================================= */
/* Convenience configuration                                                 */
/* ========================================================================= */

static inline void
vitte_diagnostic_sarif_set_uri_converter(
    vitte_diagnostic_sarif_options_t *options,
    vitte_diagnostic_sarif_uri_converter_t converter,
    void *user_data
)
{
    if (options == NULL) {
        return;
    }

    options->uri_converter = converter;
    options->uri_user_data = user_data;
}

static inline void
vitte_diagnostic_sarif_set_tool_version(
    vitte_diagnostic_sarif_options_t *options,
    const char *version
)
{
    if (options == NULL) {
        return;
    }

    options->tool_version = version;
}

static inline void
vitte_diagnostic_sarif_set_tool_name(
    vitte_diagnostic_sarif_options_t *options,
    const char *name
)
{
    if (options == NULL) {
        return;
    }

    options->tool_name = name;
}

static inline void
vitte_diagnostic_sarif_set_information_uri(
    vitte_diagnostic_sarif_options_t *options,
    const char *uri
)
{
    if (options == NULL) {
        return;
    }

    options->information_uri = uri;
}

/* ========================================================================= */
/* SARIF mapping                                                             */
/* ========================================================================= */

/*
 * Vitte diagnostic -> SARIF result
 *
 *   diagnostic.short_code
 *       -> result.ruleId
 *
 *   diagnostic.severity
 *       -> result.level
 *
 *   diagnostic.message
 *       -> result.message.text
 *
 *   primary label / diagnostic span
 *       -> result.locations[0]
 *
 *   secondary labels
 *       -> result.relatedLocations
 *
 *   diagnostic.related[]
 *       -> result.relatedLocations
 *
 *   located causes
 *       -> result.relatedLocations
 *
 *   diagnostic.suggestions[]
 *       -> result.fixes[]
 *
 *   diagnostic.fingerprint
 *       -> result.partialFingerprints
 *
 *   compiler-specific metadata
 *       -> result.properties
 */

/* ========================================================================= */
/* Rule mapping                                                              */
/* ========================================================================= */

/*
 * Diagnostic registry entries are emitted as SARIF reportingDescriptor
 * objects:
 *
 *   registry.public_code
 *       -> rule.id
 *
 *   registry.internal_code
 *       -> rule.name
 *
 *   registry.title
 *       -> rule.shortDescription.text
 *
 *   registry.explanation
 *       -> rule.fullDescription.text
 *
 *   registry.default_severity
 *       -> rule.defaultConfiguration.level
 *
 *   registry.category
 *       -> rule.properties.category
 *
 *   registry.origin
 *       -> rule.properties.phase
 *
 * This keeps the registry authoritative instead of duplicating diagnostic
 * documentation in the SARIF renderer.
 */

/* ========================================================================= */
/* Locations                                                                 */
/* ========================================================================= */

/*
 * SARIF region coordinates are one-based:
 *
 *   startLine
 *   startColumn
 *   endLine
 *   endColumn
 *
 * Vitte compiler/source-manager coordinates may use another representation.
 *
 * The current render_sarif.c also emits:
 *
 *   byteOffset
 *   byteLength
 *
 * so the original compiler span remains available.
 *
 * IMPORTANT:
 *
 * If vitte_ast_span_t does not own reliable line/column information, line and
 * column conversion should be delegated to vitte_source_manager_t instead of
 * inventing or caching duplicate position data in the diagnostic layer.
 */

/* ========================================================================= */
/* Byte offsets                                                              */
/* ========================================================================= */

/*
 * Vitte spans are fundamentally byte-oriented.
 *
 * Example:
 *
 *   let café = "😀";
 *
 * UTF-8 bytes, Unicode code points and display columns are not interchangeable.
 *
 * SARIF byteOffset/byteLength are therefore useful for retaining the exact
 * compiler span, while line/column presentation should come from the source
 * manager.
 */

/* ========================================================================= */
/* Related locations                                                         */
/* ========================================================================= */

/*
 * Related locations are intended for semantic context such as:
 *
 *   - previous declaration;
 *   - definition site;
 *   - parameter declaration;
 *   - import origin;
 *   - call site;
 *   - generic instantiation;
 *   - contract declaration;
 *   - located cause.
 *
 * They should not be generated merely because another diagnostic happens to
 * be nearby in the same source file.
 */

/* ========================================================================= */
/* Fixes                                                                     */
/* ========================================================================= */

/*
 * Vitte suggestions carrying a source span can become SARIF fixes:
 *
 *   suggestion.span
 *       -> replacement.deletedRegion
 *
 *   suggestion.replacement
 *       -> replacement.insertedContent.text
 *
 *   suggestion.message
 *       -> fix.description.text
 *
 *   suggestion.applicability
 *       -> fix.properties.applicability
 *
 * The renderer preserves applicability rather than pretending every proposed
 * edit is automatically safe.
 */

/* ========================================================================= */
/* Fingerprints                                                              */
/* ========================================================================= */

/*
 * Diagnostic fingerprints are emitted as:
 *
 *   result.partialFingerprints[
 *       "vitteDiagnosticFingerprint/v1"
 *   ]
 *
 * This allows SARIF consumers to correlate equivalent findings between runs.
 *
 * The compiler's fingerprint algorithm should therefore remain deterministic
 * for equivalent diagnostics.
 *
 * If the fingerprint algorithm changes incompatibly, increment the "/v1"
 * identifier rather than silently changing its semantics.
 */

/* ========================================================================= */
/* Vitte properties                                                          */
/* ========================================================================= */

/*
 * Vitte-specific metadata may include:
 *
 *   vittePropertiesVersion
 *   internalCode
 *   phase
 *   category
 *   fingerprint
 *   package
 *   module
 *   procedure
 *   symbol
 *   subject
 *   context
 *   expectedType
 *   actualType
 *   causes
 *   contract
 *   cascade
 *
 * These fields are extensions stored in SARIF property bags and do not replace
 * standard SARIF fields.
 */

/* ========================================================================= */
/* Contracts                                                                 */
/* ========================================================================= */

/*
 * Contract diagnostics can preserve:
 *
 *   kind
 *   expression
 *   procedure
 *   reason
 *   hasContractSpan
 *   hasCallSpan
 *   hasDeclarationSpan
 *
 * Source locations themselves should continue to be represented using SARIF
 * physicalLocation / relatedLocations where possible.
 */

/* ========================================================================= */
/* Cascade diagnostics                                                       */
/* ========================================================================= */

/*
 * Cascade metadata can preserve:
 *
 *   primary
 *   cascade
 *   suppressed
 *   parentIndex
 *   rootIndex
 *
 * Suppressed cascades are omitted from results by default.
 *
 * For debugging the diagnostic engine:
 *
 *   options.include_suppressed = true;
 *
 * can expose them.
 *
 * Long-term, stable DiagnosticId values are preferable to array indexes for
 * serialized parent/root relationships because canonical diagnostic storage
 * may eventually be reordered independently from rendering order.
 */

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

/*
 * All inputs are borrowed.
 *
 * This module does not take ownership of:
 *
 *   FILE *
 *   vitte_diagnostic_t
 *   vitte_diagnostic_bag_t
 *   option strings
 *   URI callback contexts
 *
 * The renderer never closes the supplied FILE stream.
 */

/* ========================================================================= */
/* Failure contract                                                          */
/* ========================================================================= */

/*
 * Expected status values:
 *
 * VITTE_STATUS_OK
 *
 *   Serialization completed.
 *
 *
 * VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *
 *   A required pointer/argument was invalid.
 *
 *
 * VITTE_STATUS_ERROR_INVALID_STATE
 *
 *   Serialization or output failed.
 *
 * If the project's status API provides a dedicated I/O error status, the
 * implementation may later use that status for FILE write failures.
 */

/* ========================================================================= */
/* Recommended use                                                           */
/* ========================================================================= */

/*
 * Example:
 *
 *   vitte_diagnostic_sarif_options_t options;
 *
 *   vitte_diagnostic_sarif_options_init(
 *       &options
 *   );
 *
 *   options.tool_version =
 *       VITTE_VERSION_STRING;
 *
 *   vitte_diagnostic_sarif_set_uri_converter(
 *       &options,
 *       source_to_uri,
 *       source_manager
 *   );
 *
 *   status =
 *       vitte_diagnostic_render_sarif(
 *           output,
 *           diagnostics,
 *           &options
 *       );
 */

/* ========================================================================= */
/* Integration architecture                                                  */
/* ========================================================================= */

/*
 * Recommended flow:
 *
 *   lexer
 *      |
 *   parser
 *      |
 *   resolver/imports
 *      |
 *   semantic/type/contracts
 *      |
 *   HIR
 *      |
 *   IR
 *      |
 *   C17 backend
 *      |
 *      +-------------------------------+
 *                                      |
 *                                      v
 *                             Diagnostic Engine
 *                                      |
 *                 +--------------------+--------------------+
 *                 |                    |                    |
 *                 v                    v                    v
 *              terminal              JSON                SARIF
 *                                                           |
 *                                                           v
 *                                                CI / code scanning
 *
 * SARIF rendering is a presentation/export concern. Compiler phases should
 * emit structured vitte_diagnostic_t objects rather than constructing SARIF
 * directly.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_RENDER_SARIF_H */
