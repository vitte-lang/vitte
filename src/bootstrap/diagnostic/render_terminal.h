#ifndef VITTE_DIAGNOSTIC_RENDER_TERMINAL_H
#define VITTE_DIAGNOSTIC_RENDER_TERMINAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Terminal renderer                                                         */
/* ========================================================================= */

/*
 * Human-readable terminal renderer for Vitte diagnostics.
 *
 * This module is the presentation layer for structured compiler diagnostics.
 * Compiler phases must not build terminal strings directly.
 *
 * Recommended architecture:
 *
 *   lexer
 *      |
 *   parser
 *      |
 *   imports / resolver
 *      |
 *   semantic analysis
 *      |
 *   type / contract checking
 *      |
 *   HIR
 *      |
 *   IR
 *      |
 *   C17 backend
 *      |
 *      v
 *   Diagnostic Engine
 *      |
 *      +-- terminal
 *      +-- JSON
 *      +-- SARIF
 *      +-- LSP
 *
 * The terminal renderer supports:
 *
 *   - severity-aware output;
 *   - stable diagnostic codes;
 *   - compiler phase/category display;
 *   - source locations;
 *   - source excerpts;
 *   - primary labels;
 *   - secondary labels;
 *   - multi-line spans;
 *   - multi-source diagnostics;
 *   - related locations;
 *   - declaration/origin/call/instantiation locations;
 *   - expected/found type context;
 *   - causal chains;
 *   - notes;
 *   - help messages;
 *   - structured suggestions;
 *   - replacement previews;
 *   - applicability;
 *   - contract diagnostics;
 *   - cascade metadata;
 *   - fingerprints;
 *   - compact machine-friendly terminal output;
 *   - diagnostic summaries;
 *   - ANSI color;
 *   - tab-aware marker placement.
 *
 * Exact Unicode display width should ultimately be supplied by the canonical
 * source/display subsystem rather than approximated independently by every
 * renderer.
 */

/* ========================================================================= */
/* Defaults                                                                  */
/* ========================================================================= */

#define VITTE_DIAGNOSTIC_TERMINAL_DEFAULT_TAB_WIDTH     ((size_t)4u)
#define VITTE_DIAGNOSTIC_TERMINAL_DEFAULT_CONTEXT_LINES ((size_t)1u)

/* ========================================================================= */
/* Options                                                                   */
/* ========================================================================= */

typedef struct vitte_diagnostic_terminal_options {
    /*
     * Enable ANSI terminal styling.
     *
     * When false, no ANSI escape sequence is emitted.
     *
     * CLI policy should normally decide this from:
     *
     *   --color=auto
     *   --color=always
     *   --color=never
     *
     * rather than forcing terminal detection into the renderer.
     */
    bool color;

    /*
     * Display stable public diagnostic codes.
     *
     * Example:
     *
     *   error[E0501]: incompatible types
     */
    bool show_code;

    /*
     * Display the compiler phase responsible for the diagnostic.
     *
     * Example:
     *
     *   phase: type-check
     */
    bool show_phase;

    /*
     * Display the diagnostic category.
     *
     * Example:
     *
     *   category: type
     */
    bool show_category;

    /*
     * Display source location/excerpts.
     */
    bool show_source;

    /*
     * Display primary and secondary diagnostic labels.
     */
    bool show_labels;

    /*
     * Display related semantic locations.
     *
     * Examples:
     *
     *   declared here
     *   defined here
     *   originates here
     *   used here
     *   called here
     *   instantiated here
     */
    bool show_related;

    /*
     * Display causal diagnostic information.
     */
    bool show_causes;

    /*
     * Display note annotations.
     */
    bool show_notes;

    /*
     * Display help annotations.
     */
    bool show_help;

    /*
     * Display structured suggestions and replacement previews.
     */
    bool show_suggestions;

    /*
     * Display Vitte contract metadata.
     */
    bool show_contract;

    /*
     * Display semantic context such as:
     *
     *   package
     *   module
     *   procedure
     *   symbol
     *   expected type
     *   actual type
     *   context
     */
    bool show_context;

    /*
     * Display diagnostic-engine cascade information.
     *
     * Intended primarily for:
     *
     *   compiler debugging;
     *   diagnostic-engine tests;
     *   cascade classifier inspection.
     *
     * Normally false for end users.
     */
    bool show_cascade;

    /*
     * Display the deterministic diagnostic fingerprint.
     *
     * Intended primarily for debugging and tooling.
     */
    bool show_fingerprint;

    /*
     * Include diagnostics marked as suppressed.
     *
     * Normally false.
     */
    bool include_suppressed;

    /*
     * Use compact one-line output.
     *
     * Example:
     *
     *   src/main.vit:12:18: error[E0501]: incompatible types [type-check]
     *
     * Compact mode intentionally omits rich source excerpts.
     */
    bool compact;

    /*
     * Append a newline after a fully rendered diagnostic.
     */
    bool trailing_newline;

    /*
     * Number of display columns used for a tab stop.
     *
     * Zero is normalized to the renderer default.
     */
    size_t tab_width;

    /*
     * Number of source lines to display before and after a primary source
     * location when rich source context is available.
     *
     * The current renderer reserves this policy field even if the source
     * adapter does not yet expose context-line lookup.
     */
    size_t context_lines;

    /*
     * Borrowed source manager.
     *
     * Used for:
     *
     *   - source text lookup;
     *   - line extraction;
     *   - byte offset -> line/column conversion;
     *   - Unicode-aware display information;
     *   - generated-source remapping where appropriate.
     *
     * NULL is valid.
     *
     * When unavailable, the renderer falls back to location-only output.
     */
    const vitte_source_manager_t *sources;
} vitte_diagnostic_terminal_options_t;

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

/*
 * Initialize options with the standard human-readable terminal policy.
 *
 * Defaults:
 *
 *   color               = true
 *
 *   show_code           = true
 *   show_phase          = true
 *   show_category       = true
 *
 *   show_source         = true
 *   show_labels         = true
 *   show_related        = true
 *
 *   show_causes         = true
 *   show_notes          = true
 *   show_help           = true
 *   show_suggestions    = true
 *
 *   show_contract       = true
 *   show_context        = true
 *
 *   show_cascade        = false
 *   show_fingerprint    = false
 *
 *   include_suppressed  = false
 *
 *   compact             = false
 *   trailing_newline    = true
 *
 *   tab_width           = 4
 *   context_lines       = 1
 *
 *   sources             = NULL
 */
void
vitte_diagnostic_terminal_options_init(
    vitte_diagnostic_terminal_options_t *options
);

/* ========================================================================= */
/* One diagnostic                                                           */
/* ========================================================================= */

/*
 * Render one structured diagnostic.
 *
 * Suppressed diagnostics are omitted unless:
 *
 *   options->include_suppressed == true
 *
 * When options is NULL, default terminal options are used.
 *
 * The diagnostic is borrowed and is never modified.
 */
vitte_status_t
vitte_diagnostic_render_terminal_one(
    FILE *stream,
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_terminal_options_t *options
);

/* ========================================================================= */
/* Diagnostic bag                                                            */
/* ========================================================================= */

/*
 * Render every visible diagnostic in a bag.
 *
 * Rich mode:
 *
 *   - renders diagnostics individually;
 *   - separates them visually;
 *   - emits a final summary.
 *
 * Compact mode:
 *
 *   - emits one line per diagnostic;
 *   - omits the rich final presentation.
 *
 * Diagnostic order is the order presented by the bag.
 *
 * Sorting policy belongs to the diagnostic engine/render-order layer, not to
 * this renderer.
 */
vitte_status_t
vitte_diagnostic_render_terminal(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_terminal_options_t *options
);

/* ========================================================================= */
/* Summary                                                                   */
/* ========================================================================= */

/*
 * Render only the diagnostic summary.
 *
 * Example:
 *
 *   diagnostics: 2 errors, 3 warnings, 1 note, 4 suppressed
 */
vitte_status_t
vitte_diagnostic_render_terminal_summary(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag,
    const vitte_diagnostic_terminal_options_t *options
);

/* ========================================================================= */
/* Generic compatibility adapter                                             */
/* ========================================================================= */

/*
 * Render a diagnostic bag with default terminal options.
 *
 * Conceptually equivalent to:
 *
 *   vitte_diagnostic_terminal_options_t options;
 *
 *   vitte_diagnostic_terminal_options_init(&options);
 *
 *   return vitte_diagnostic_render_terminal(
 *       stream,
 *       bag,
 *       &options
 *   );
 */
vitte_status_t
vitte_diagnostic_write_terminal(
    FILE *stream,
    const vitte_diagnostic_bag_t *bag
);

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

/*
 * Return the renderer name.
 *
 * Current value:
 *
 *   "terminal"
 *
 * Returned storage has static lifetime.
 */
const char *
vitte_diagnostic_terminal_renderer_name(void);

/* ========================================================================= */
/* Convenience configuration                                                 */
/* ========================================================================= */

static inline void
vitte_diagnostic_terminal_set_sources(
    vitte_diagnostic_terminal_options_t *options,
    const vitte_source_manager_t *sources
)
{
    if (options == NULL) {
        return;
    }

    options->sources = sources;
}

static inline void
vitte_diagnostic_terminal_set_color(
    vitte_diagnostic_terminal_options_t *options,
    bool enabled
)
{
    if (options == NULL) {
        return;
    }

    options->color = enabled;
}

static inline void
vitte_diagnostic_terminal_set_compact(
    vitte_diagnostic_terminal_options_t *options,
    bool enabled
)
{
    if (options == NULL) {
        return;
    }

    options->compact = enabled;
}

static inline void
vitte_diagnostic_terminal_set_tab_width(
    vitte_diagnostic_terminal_options_t *options,
    size_t width
)
{
    if (options == NULL) {
        return;
    }

    options->tab_width =
        width != 0u
            ? width
            : VITTE_DIAGNOSTIC_TERMINAL_DEFAULT_TAB_WIDTH;
}

static inline void
vitte_diagnostic_terminal_set_context_lines(
    vitte_diagnostic_terminal_options_t *options,
    size_t lines
)
{
    if (options == NULL) {
        return;
    }

    options->context_lines = lines;
}

/* ========================================================================= */
/* Standard presets                                                          */
/* ========================================================================= */

/*
 * Configure options for normal human-facing terminal diagnostics.
 */
static inline void
vitte_diagnostic_terminal_preset_human(
    vitte_diagnostic_terminal_options_t *options
)
{
    if (options == NULL) {
        return;
    }

    vitte_diagnostic_terminal_options_init(
        options
    );
}

/*
 * Configure options for compact compiler/build output.
 */
static inline void
vitte_diagnostic_terminal_preset_compact(
    vitte_diagnostic_terminal_options_t *options
)
{
    if (options == NULL) {
        return;
    }

    vitte_diagnostic_terminal_options_init(
        options
    );

    options->compact = true;
    options->color = false;
    options->show_category = false;
    options->show_cascade = false;
    options->show_fingerprint = false;
}

/*
 * Configure options for diagnostic-engine debugging.
 */
static inline void
vitte_diagnostic_terminal_preset_debug(
    vitte_diagnostic_terminal_options_t *options
)
{
    if (options == NULL) {
        return;
    }

    vitte_diagnostic_terminal_options_init(
        options
    );

    options->show_cascade = true;
    options->show_fingerprint = true;
    options->include_suppressed = true;
}

/*
 * Configure options for plain text output without ANSI sequences.
 */
static inline void
vitte_diagnostic_terminal_preset_plain(
    vitte_diagnostic_terminal_options_t *options
)
{
    if (options == NULL) {
        return;
    }

    vitte_diagnostic_terminal_options_init(
        options
    );

    options->color = false;
}

/* ========================================================================= */
/* Output model                                                              */
/* ========================================================================= */

/*
 * A normal error may render approximately as:
 *
 *   error[E0501]: incompatible types
 *     phase: type-check | category: type
 *     --> src/main.vit:12:22
 *      |
 *   12 |     let result: i32 = calculate("42");
 *      |                                 ^^^^ expected `i32`, found `str`
 *      |
 *      = declared here: parameter `value`
 *      = called here: src/main.vit:12:22
 *      note: `calculate` requires an integer argument
 *      help: convert the value before passing it [machine-applicable]
 *            + parse_i32("42")
 *
 *   diagnostics: 1 error, 0 warnings
 *
 * Exact visual layout is presentation policy and may evolve without changing
 * the structured diagnostic model.
 */

/* ========================================================================= */
/* Primary and secondary labels                                              */
/* ========================================================================= */

/*
 * Primary labels identify the principal failing expression/location.
 *
 * Secondary labels provide explanatory relationships.
 *
 * Example:
 *
 *   12 |     let result: i32 = value;
 *      |                 ---   ^^^^^
 *      |                 |     |
 *      |                 |     found `str`
 *      |                 expected `i32`
 *
 * The diagnostic engine should preserve label semantics.
 *
 * The terminal renderer should not infer semantic relationships merely from
 * physical proximity.
 */

/* ========================================================================= */
/* Multi-source diagnostics                                                  */
/* ========================================================================= */

/*
 * Labels and related locations may refer to different source files.
 *
 * Example:
 *
 *   error[E0402]: duplicate declaration
 *     --> src/a.vit:10:6
 *      |
 *   10 | proc calculate() {
 *      |      ^^^^^^^^^ duplicate declaration
 *
 *     = previously declared at src/b.vit:4:6
 *
 * The renderer must therefore never assume every span belongs to the primary
 * diagnostic source.
 */

/* ========================================================================= */
/* Multi-line spans                                                          */
/* ========================================================================= */

/*
 * A diagnostic span may cover multiple lines.
 *
 * The renderer should preserve:
 *
 *   start line
 *   start column
 *   end line
 *   end column
 *
 * and avoid truncating the semantic span merely to simplify terminal output.
 *
 * Very large spans may eventually use an abbreviated representation:
 *
 *   10 | / ...
 *      | |
 *   42 | \ ...
 *
 * while retaining the complete span in JSON/SARIF/LSP output.
 */

/* ========================================================================= */
/* Source coordinates                                                        */
/* ========================================================================= */

/*
 * Source offsets and terminal columns are different concepts.
 *
 * Vitte source spans may use UTF-8 byte offsets.
 *
 * Terminal rendering requires display columns.
 *
 * For example:
 *
 *   ASCII:
 *       one byte often corresponds to one terminal cell.
 *
 *   UTF-8:
 *       one character may use multiple bytes.
 *
 *   combining characters:
 *       multiple code points may occupy one display cell.
 *
 *   CJK:
 *       one code point may occupy two terminal cells.
 *
 *   emoji:
 *       width may depend on grapheme composition and terminal behavior.
 *
 * Therefore:
 *
 *   byte offset != Unicode scalar index != grapheme index != display column
 *
 * The long-term canonical solution should live in the source/Unicode display
 * subsystem.
 */

/* ========================================================================= */
/* Tabs                                                                      */
/* ========================================================================= */

/*
 * Tabs must be expanded consistently for both:
 *
 *   source text
 *   underline/marker placement
 *
 * Otherwise:
 *
 *       let x = foo();
 *               ^^^
 *
 * can become visually misaligned.
 *
 * tab_width controls this presentation behavior.
 */

/* ========================================================================= */
/* Source manager                                                            */
/* ========================================================================= */

/*
 * The source manager should eventually provide canonical operations similar
 * in responsibility to:
 *
 *   source text lookup
 *   line lookup
 *   offset -> line/column
 *   line/column -> offset
 *   display-column calculation
 *   source filename/path
 *
 * Exact API names are deliberately not defined by this header.
 *
 * render_terminal.c contains an isolated source lookup adapter so the renderer
 * can be connected to the real source-manager API without spreading
 * source-storage assumptions throughout the renderer.
 */

/* ========================================================================= */
/* Expected / found types                                                    */
/* ========================================================================= */

/*
 * When structured type information is available:
 *
 *   diagnostic.expected_type
 *   diagnostic.actual_type
 *
 * the renderer may present:
 *
 *   type: expected `i32`, found `str`
 *
 * Long-term structural type diffs should preferably be produced from typed
 * diagnostic data rather than by parsing formatted type strings.
 */

/* ========================================================================= */
/* Causes                                                                    */
/* ========================================================================= */

/*
 * Cause chains explain why a diagnostic exists.
 *
 * Example:
 *
 *   error[E0810]: trait requirement not satisfied
 *     ...
 *     caused by: generic parameter `T` requires `Printable`
 *     caused by: instantiated by `render<User>`
 *
 * Causes are semantic provenance, not merely nearby diagnostics.
 */

/* ========================================================================= */
/* Notes and help                                                            */
/* ========================================================================= */

/*
 * Notes provide explanation.
 *
 * Help provides an actionable direction.
 *
 * Examples:
 *
 *   note: this value is inferred as `str`
 *
 *   help: convert the value to `i32` before this call
 *
 * They remain separate structured annotations even when terminal rendering
 * makes them visually similar.
 */

/* ========================================================================= */
/* Suggestions                                                               */
/* ========================================================================= */

/*
 * Structured suggestions may contain:
 *
 *   message
 *   source span
 *   replacement text
 *   applicability
 *
 * Example:
 *
 *   help: convert the value [machine-applicable]
 *         + parse_i32(value)
 *
 * Applicability must remain visible to tooling.
 *
 * The renderer must not imply that:
 *
 *   maybe-incorrect
 *
 * is equivalent to:
 *
 *   machine-applicable
 */

/* ========================================================================= */
/* Contracts                                                                 */
/* ========================================================================= */

/*
 * Vitte contract diagnostics may display:
 *
 *   requires
 *   ensures
 *   invariant
 *   assertion
 *
 * together with:
 *
 *   expression
 *   procedure
 *   reason
 *   contract declaration span
 *   call span
 *   procedure declaration span
 *
 * Example:
 *
 *   error[E0701]: precondition is not satisfied
 *     contract: requires `size > 0`
 *     procedure: allocate
 *     reason: argument `size` may be zero
 *     contract declared: src/memory.vit:8:5
 *     call site: src/main.vit:42:18
 */

/* ========================================================================= */
/* Cascades                                                                  */
/* ========================================================================= */

/*
 * Cascade information is normally hidden.
 *
 * Debug mode can expose:
 *
 *   primary
 *   cascade
 *   suppressed
 *   parent index
 *   root index
 *
 * Example:
 *
 *   diagnostic relation: cascade suppressed parent=2 root=0
 *
 * Array indexes are currently supported because the diagnostic model uses
 * them.
 *
 * Long-term serialized relationships should use stable DiagnosticId values so
 * canonical storage order and rendering order can be independent.
 */

/* ========================================================================= */
/* Fingerprints                                                              */
/* ========================================================================= */

/*
 * Fingerprints are debugging/tooling identifiers.
 *
 * They should be:
 *
 *   deterministic;
 *   stable for equivalent diagnostics;
 *   independent of terminal color/layout;
 *   based on semantic diagnostic identity.
 *
 * Example:
 *
 *   fingerprint: 92b7cdd9840c39e1
 */

/* ========================================================================= */
/* Suppression                                                               */
/* ========================================================================= */

/*
 * Suppressed cascades are omitted by default.
 *
 * Debugging:
 *
 *   vitte_diagnostic_terminal_preset_debug(&options);
 *
 * enables them.
 *
 * Suppression is diagnostic-engine policy.
 *
 * The renderer only obeys the structured suppression state and must not
 * independently decide whether a diagnostic is a cascade.
 */

/* ========================================================================= */
/* Determinism                                                               */
/* ========================================================================= */

/*
 * Rendering should be deterministic for an equivalent diagnostic bag.
 *
 * The renderer must not:
 *
 *   - reorder canonical bag storage;
 *   - mutate diagnostics;
 *   - mutate label arrays;
 *   - classify cascades;
 *   - deduplicate diagnostics.
 *
 * Those responsibilities belong to the diagnostic engine.
 */

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

/*
 * All inputs are borrowed.
 *
 * The renderer does not take ownership of:
 *
 *   FILE *
 *   vitte_diagnostic_t
 *   vitte_diagnostic_bag_t
 *   vitte_source_manager_t
 *   strings
 *
 * The renderer never closes the supplied stream.
 */

/* ========================================================================= */
/* Thread safety                                                             */
/* ========================================================================= */

/*
 * The renderer does not use mutable global state.
 *
 * Separate renderer invocations are therefore structurally suitable for
 * concurrent use provided:
 *
 *   - each output stream is externally synchronized when shared;
 *   - the diagnostic bag is not concurrently mutated;
 *   - the source manager supports concurrent reads.
 */

/* ========================================================================= */
/* Error handling                                                            */
/* ========================================================================= */

/*
 * Expected statuses:
 *
 * VITTE_STATUS_OK
 *
 *   Rendering completed successfully.
 *
 *
 * VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *
 *   A required pointer was invalid.
 *
 *
 * VITTE_STATUS_ERROR_INVALID_STATE
 *
 *   Output failed or the FILE stream entered an error state.
 *
 * If Vitte later introduces a dedicated I/O status, FILE write failures should
 * use that status instead.
 */

/* ========================================================================= */
/* CLI integration                                                           */
/* ========================================================================= */

/*
 * Recommended CLI mapping:
 *
 *   vitte build
 *       -> rich terminal diagnostics
 *
 *   vitte build --diagnostic-format=human
 *       -> vitte_diagnostic_render_terminal
 *
 *   vitte build --diagnostic-format=short
 *       -> compact=true
 *
 *   vitte build --diagnostic-format=json
 *       -> render_json
 *
 *   vitte build --diagnostic-format=sarif
 *       -> render_sarif
 *
 *   language server
 *       -> render_lsp / structured LSP conversion
 *
 * Color policy should ideally be decided by the CLI:
 *
 *   --color=auto
 *   --color=always
 *   --color=never
 */

/* ========================================================================= */
/* Testing                                                                   */
/* ========================================================================= */

/*
 * Recommended golden-test coverage:
 *
 *   1. error without source
 *   2. warning without source
 *   3. single-line primary label
 *   4. zero-width insertion span
 *   5. secondary label
 *   6. multiple labels on one line
 *   7. labels across several lines
 *   8. multi-line primary span
 *   9. multi-source diagnostic
 *  10. declaration + use
 *  11. call-site chain
 *  12. generic instantiation location
 *  13. expected/found types
 *  14. causes
 *  15. notes
 *  16. help
 *  17. replacement suggestion
 *  18. all applicability levels
 *  19. contract diagnostics
 *  20. cascade diagnostics
 *  21. suppressed diagnostics
 *  22. compact output
 *  23. ANSI enabled
 *  24. ANSI disabled
 *  25. tabs
 *  26. UTF-8 identifiers
 *  27. combining characters
 *  28. CJK text
 *  29. emoji
 *  30. empty source line
 *  31. very long source line
 *  32. source unavailable fallback
 *  33. fatal diagnostics
 *  34. summary pluralization
 *  35. deterministic repeated rendering
 *  36. write failure
 *  37. malformed/nonfatal diagnostic data
 *  38. ASan
 *  39. UBSan
 *  40. fuzzed spans
 */

/* ========================================================================= */
/* Fuzzing                                                                   */
/* ========================================================================= */

/*
 * Terminal rendering is an important fuzz target because diagnostics are often
 * invoked while the compiler is already processing malformed input.
 *
 * Fuzz:
 *
 *   arbitrary UTF-8
 *   invalid UTF-8
 *   huge columns
 *   zero-length spans
 *   reversed spans
 *   huge line numbers
 *   empty messages
 *   many labels
 *   many causes
 *   many suggestions
 *   overlapping spans
 *   multi-source spans
 *
 * A malformed user program must never make diagnostic rendering crash the
 * compiler.
 */

/* ========================================================================= */
/* Internal compiler errors                                                  */
/* ========================================================================= */

/*
 * Internal compiler errors should use the same structured rendering pipeline.
 *
 * Example:
 *
 *   fatal[E0001]: internal compiler error
 *     phase: ir
 *     --> src/main.vit:42:9
 *      |
 *   42 |     expression
 *      |     ^^^^^^^^^^ while lowering this expression
 *      |
 *      note: unexpected IR state
 *
 * Raw assert()/abort()/fprintf(stderr, ...) should not be the normal compiler
 * diagnostic path.
 */

/* ========================================================================= */
/* Backend remapping                                                         */
/* ========================================================================= */

/*
 * C17 backend diagnostics should be remapped before reaching this renderer:
 *
 *   generated C location
 *          |
 *          v
 *   backend source map
 *          |
 *          v
 *   original Vitte span
 *          |
 *          v
 *   structured Diagnostic
 *          |
 *          v
 *   terminal renderer
 *
 * Users should normally see:
 *
 *   src/main.vit:...
 *
 * rather than:
 *
 *   /tmp/vitte-xxxx/generated.c:...
 */

/* ========================================================================= */
/* Design rule                                                               */
/* ========================================================================= */

/*
 * This renderer is intentionally "dumb" about compiler semantics.
 *
 * It knows how to DISPLAY:
 *
 *   diagnostics
 *   spans
 *   labels
 *   causes
 *   suggestions
 *   contracts
 *
 * It does not decide:
 *
 *   which error occurred;
 *   which symbol is responsible;
 *   which type is correct;
 *   whether a contract is valid;
 *   whether two diagnostics share a cause;
 *   whether a diagnostic should be suppressed.
 *
 * Those decisions belong to compiler phases and the diagnostic engine.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_RENDER_TERMINAL_H */
