#ifndef VITTE_DIAGNOSTIC_RENDER_JSON_H
#define VITTE_DIAGNOSTIC_RENDER_JSON_H

#include <stdbool.h>
#include <stdio.h>

#include "diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Vitte JSON diagnostic renderer.
 *
 * This module serializes structured compiler diagnostics into deterministic
 * machine-readable JSON.
 *
 * It is intended for:
 *
 *     - command-line tooling;
 *     - IDE/editor integrations;
 *     - continuous integration;
 *     - build systems;
 *     - diagnostic snapshots/golden tests;
 *     - external compiler tooling;
 *     - debugging;
 *     - protocol adapters.
 *
 * JSON rendering is independent from terminal rendering. No ANSI escape
 * sequences, terminal widths or presentation-specific source decorations are
 * emitted here.
 *
 * Source offsets remain byte offsets. LSP UTF-16 coordinate conversion belongs
 * to the dedicated LSP renderer.
 */

/* ========================================================================= */
/* Schema                                                                    */
/* ========================================================================= */

/*
 * Current JSON schema name:
 *
 *     vitte-diagnostics
 *
 * Current schema version:
 *
 *     1
 *
 * Consumers should use the schema name/version instead of assuming that every
 * future Vitte compiler release emits exactly the same JSON structure.
 */

/*
 * Return the current JSON diagnostic schema version.
 */
unsigned
vitte_diagnostic_json_schema_version(void);

/*
 * Return the stable schema name.
 *
 * Currently:
 *
 *     "vitte-diagnostics"
 *
 * The returned string has static storage duration.
 */
const char *
vitte_diagnostic_json_schema_name(void);

/* ========================================================================= */
/* Options                                                                   */
/* ========================================================================= */

/*
 * JSON rendering policy.
 */
typedef struct vitte_diagnostic_json_options {
    /*
     * Pretty-print objects and arrays with indentation/newlines.
     *
     * false:
     *
     *     {"code":"E0501","message":"type mismatch",...}
     *
     * true:
     *
     *     {
     *       "code": "E0501",
     *       "message": "type mismatch",
     *       ...
     *     }
     *
     * JSON Lines rendering always disables pretty printing because each
     * diagnostic must occupy exactly one physical line.
     */
    bool pretty;

    /*
     * Include diagnostics marked as suppressed.
     *
     * Suppressed cascade diagnostics are normally omitted from ordinary
     * machine output but can be useful for:
     *
     *     - compiler debugging;
     *     - diagnostic-engine tests;
     *     - IDE debugging;
     *     - cascade analysis.
     */
    bool include_suppressed;

    /*
     * Include cascade metadata inside each diagnostic.
     *
     * When enabled:
     *
     *     "cascade": {
     *       "primary": true,
     *       "cascade": false,
     *       "suppressed": false,
     *       "parent_index": null,
     *       "root_index": null
     *     }
     *
     * This controls metadata emission independently from whether suppressed
     * diagnostics themselves are included.
     */
    bool include_suppressed_metadata;

    /*
     * Append a newline after a complete top-level JSON document.
     *
     * Recommended for command-line output and files.
     */
    bool trailing_newline;
} vitte_diagnostic_json_options_t;

/*
 * Initialize JSON options to the recommended defaults:
 *
 *     pretty                      = false
 *     include_suppressed          = false
 *     include_suppressed_metadata = true
 *     trailing_newline            = true
 */
void
vitte_diagnostic_json_options_init(
    vitte_diagnostic_json_options_t *options
);

/* ========================================================================= */
/* Single diagnostic                                                         */
/* ========================================================================= */

/*
 * Render one diagnostic using default JSON options.
 *
 * The resulting top-level JSON value is the diagnostic object itself.
 *
 * Example:
 *
 * {
 *   "index": 0,
 *   "code": "E0501",
 *   "internal_code": "VITTE_TYPE_E_MISMATCH",
 *   "severity": "error",
 *   "phase": "type-check",
 *   "category": "type",
 *   "message": "type mismatch",
 *   ...
 * }
 */
vitte_status_t
vitte_diagnostic_render_json_one(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
);

/*
 * Render one diagnostic with explicit options.
 *
 * options may be NULL, in which case default options are used.
 */
vitte_status_t
vitte_diagnostic_render_json_one_ex(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_json_options_t *options
);

/* ========================================================================= */
/* Diagnostic bag                                                            */
/* ========================================================================= */

/*
 * Render a complete diagnostic bag as one JSON document.
 *
 * Top-level shape:
 *
 * {
 *   "schema": "vitte-diagnostics",
 *   "version": 1,
 *   "counts": {
 *     "note": 0,
 *     "help": 0,
 *     "warning": 1,
 *     "error": 2,
 *     "fatal": 0,
 *     "suppressed": 1
 *   },
 *   "stored": 4,
 *   "diagnostics": [
 *     ...
 *   ],
 *   "emitted": 3
 * }
 *
 * "stored" is the number of diagnostics physically stored in the bag.
 *
 * "emitted" is the number serialized after applying suppression policy.
 *
 * options may be NULL.
 */
vitte_status_t
vitte_diagnostic_render_json_bag(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_json_options_t *options
);

/* ========================================================================= */
/* JSON Lines                                                                */
/* ========================================================================= */

/*
 * Render diagnostics using JSON Lines / NDJSON.
 *
 * Each non-suppressed diagnostic is emitted as one complete JSON object
 * followed by '\n':
 *
 *     {"code":"E0501",...}
 *     {"code":"E0402",...}
 *     {"code":"W0012",...}
 *
 * Pretty printing is forcibly disabled even when options->pretty is true.
 *
 * This format is useful for:
 *
 *     - streaming compiler diagnostics;
 *     - build servers;
 *     - process pipelines;
 *     - incremental consumers;
 *     - log ingestion;
 *     - large compilations.
 *
 * options may be NULL.
 */
vitte_status_t
vitte_diagnostic_render_json_lines(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_json_options_t *options
);

/* ========================================================================= */
/* Generic rendering adapters                                                */
/* ========================================================================= */

/*
 * Compatibility adapter for the generic diagnostic rendering layer.
 *
 * Equivalent to:
 *
 *     vitte_diagnostic_render_json_one(stream, diagnostic)
 */
vitte_status_t
vitte_diagnostic_write_json(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
);

/*
 * Compatibility adapter for rendering a complete bag using default options.
 */
vitte_status_t
vitte_diagnostic_write_json_bag(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag
);

/* ========================================================================= */
/* Convenience wrappers                                                      */
/* ========================================================================= */

/*
 * Render one diagnostic as compact JSON without a trailing newline.
 *
 * Useful when embedding a diagnostic inside another protocol/document.
 */
static inline vitte_status_t
vitte_diagnostic_render_json_compact(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
)
{
    vitte_diagnostic_json_options_t options;

    vitte_diagnostic_json_options_init(
        &options
    );

    options.pretty = false;
    options.trailing_newline = false;

    return vitte_diagnostic_render_json_one_ex(
        stream,
        diagnostic,
        &options
    );
}

/*
 * Render one diagnostic as human-readable pretty JSON.
 */
static inline vitte_status_t
vitte_diagnostic_render_json_pretty(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic
)
{
    vitte_diagnostic_json_options_t options;

    vitte_diagnostic_json_options_init(
        &options
    );

    options.pretty = true;

    return vitte_diagnostic_render_json_one_ex(
        stream,
        diagnostic,
        &options
    );
}

/*
 * Render a complete bag as pretty JSON.
 */
static inline vitte_status_t
vitte_diagnostic_render_json_bag_pretty(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag
)
{
    vitte_diagnostic_json_options_t options;

    vitte_diagnostic_json_options_init(
        &options
    );

    options.pretty = true;

    return vitte_diagnostic_render_json_bag(
        stream,
        bag,
        &options
    );
}

/*
 * Render a complete bag including suppressed cascade diagnostics.
 */
static inline vitte_status_t
vitte_diagnostic_render_json_bag_debug(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag
)
{
    vitte_diagnostic_json_options_t options;

    vitte_diagnostic_json_options_init(
        &options
    );

    options.pretty = true;
    options.include_suppressed = true;
    options.include_suppressed_metadata = true;

    return vitte_diagnostic_render_json_bag(
        stream,
        bag,
        &options
    );
}

/* ========================================================================= */
/* Output contract                                                           */
/* ========================================================================= */

/*
 * String encoding
 * ---------------
 *
 * Diagnostic strings are expected to contain UTF-8.
 *
 * JSON control characters, quotes and backslashes are escaped by the
 * renderer. UTF-8 bytes outside the ASCII control range are preserved.
 *
 *
 * Nullability
 * -----------
 *
 * Optional strings and spans are emitted as JSON null when unavailable.
 *
 * Collections are emitted as empty arrays rather than null:
 *
 *     "labels": []
 *     "causes": []
 *     "annotations": []
 *     "suggestions": []
 *
 *
 * Source positions
 * ----------------
 *
 * Source offsets are byte offsets:
 *
 *     start_byte
 *     end_byte
 *
 * They must not be interpreted as UTF-16 offsets.
 *
 * Dedicated conversion is required for LSP.
 *
 *
 * Fingerprints
 * ------------
 *
 * Diagnostic fingerprints are serialized as hexadecimal JSON strings rather
 * than JSON numbers.
 *
 * This avoids precision loss in consumers whose JSON number representation is
 * based on IEEE-754 double precision.
 *
 * Example:
 *
 *     "fingerprint": "9c53c60e7fb28a11"
 *
 * Fingerprints are implementation metadata and are not stable public
 * diagnostic identifiers.
 *
 *
 * Public codes
 * ------------
 *
 * Stable user-facing diagnostic identifiers remain E/W/N/H codes:
 *
 *     E0501
 *     W0004
 *     N0002
 *     H0001
 *
 *
 * Determinism
 * -----------
 *
 * Given an equivalent normalized diagnostic bag and identical rendering
 * options, field ordering and serialization are deterministic.
 *
 * This property is required for:
 *
 *     - golden tests;
 *     - reproducible compiler output;
 *     - CI snapshots;
 *     - cache keys;
 *     - regression testing.
 *
 *
 * Ownership
 * ---------
 *
 * The renderer does not take ownership of:
 *
 *     - FILE streams;
 *     - diagnostics;
 *     - diagnostic bags;
 *     - source-manager data.
 *
 * It never closes the supplied FILE stream.
 *
 *
 * Failure
 * -------
 *
 * Functions return:
 *
 *     VITTE_STATUS_OK
 *         serialization completed successfully.
 *
 *     VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *         invalid stream, diagnostic or bag.
 *
 *     VITTE_STATUS_ERROR_INVALID_STATE
 *         output/stream failure.
 */

/* ========================================================================= */
/* Recommended CLI mappings                                                  */
/* ========================================================================= */

/*
 * Suggested CLI integration:
 *
 *     vitte check file.vit --diagnostic-format=json
 *
 *         -> vitte_diagnostic_render_json_bag()
 *
 *
 *     vitte check file.vit --diagnostic-format=json-pretty
 *
 *         -> vitte_diagnostic_render_json_bag_pretty()
 *
 *
 *     vitte check file.vit --diagnostic-format=json-lines
 *
 *         -> vitte_diagnostic_render_json_lines()
 *
 *
 * JSON is intended for machine consumers.
 *
 * Terminal diagnostics should continue through render_terminal.c.
 * SARIF output should continue through render_sarif.c.
 * LSP output should continue through render_lsp.c.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_RENDER_JSON_H */
