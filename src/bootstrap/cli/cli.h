#ifndef VITTE_CLI_CLI_H
#define VITTE_CLI_CLI_H

/*
 * Vitte Compiler
 * src/bootstrap/cli/cli.h
 *
 * Canonical public command-line interface parser API.
 *
 * ISO C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_CLI_DEFAULT_MAX_INPUTS \
    ((size_t)65536u)

#define VITTE_CLI_DEFAULT_MAX_FORWARD_ARGS \
    ((size_t)65536u)

#define VITTE_CLI_DEFAULT_MAX_ERRORS \
    ((size_t)100u)

#define VITTE_CLI_DEFAULT_JOBS \
    ((size_t)1u)

/* ========================================================================= */
/* Error                                                                     */
/* ========================================================================= */

typedef enum vitte_cli_error {
    VITTE_CLI_ERROR_NONE = 0,

    VITTE_CLI_ERROR_INVALID_CLI,
    VITTE_CLI_ERROR_INVALID_ARGUMENT,
    VITTE_CLI_ERROR_INVALID_STATE,

    VITTE_CLI_ERROR_UNKNOWN_COMMAND,
    VITTE_CLI_ERROR_UNKNOWN_OPTION,

    VITTE_CLI_ERROR_MISSING_OPTION_VALUE,
    VITTE_CLI_ERROR_INVALID_OPTION_VALUE,

    VITTE_CLI_ERROR_DUPLICATE_OPTION,
    VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
    VITTE_CLI_ERROR_OPTION_NOT_ALLOWED,

    VITTE_CLI_ERROR_MISSING_INPUT,
    VITTE_CLI_ERROR_TOO_MANY_INPUTS,
    VITTE_CLI_ERROR_TOO_MANY_FORWARD_ARGS,

    VITTE_CLI_ERROR_INVALID_TARGET,
    VITTE_CLI_ERROR_INVALID_BACKEND,
    VITTE_CLI_ERROR_INVALID_OPTIMIZATION,
    VITTE_CLI_ERROR_INVALID_DIAGNOSTIC_FORMAT,
    VITTE_CLI_ERROR_INVALID_COLOR_MODE,

    VITTE_CLI_ERROR_OVERFLOW,
    VITTE_CLI_ERROR_OUT_OF_MEMORY,

    VITTE_CLI_ERROR_COUNT
} vitte_cli_error_t;

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

typedef enum vitte_cli_state {
    VITTE_CLI_STATE_INVALID = 0,

    VITTE_CLI_STATE_READY,
    VITTE_CLI_STATE_PARSING,
    VITTE_CLI_STATE_PARSED,
    VITTE_CLI_STATE_FAILED,
    VITTE_CLI_STATE_DESTROYED,

    VITTE_CLI_STATE_COUNT
} vitte_cli_state_t;

/* ========================================================================= */
/* Command                                                                   */
/* ========================================================================= */

typedef enum vitte_cli_command {
    VITTE_CLI_COMMAND_NONE = 0,

    VITTE_CLI_COMMAND_BUILD,
    VITTE_CLI_COMMAND_RUN,
    VITTE_CLI_COMMAND_CHECK,
    VITTE_CLI_COMMAND_TEST,
    VITTE_CLI_COMMAND_FMT,

    VITTE_CLI_COMMAND_INSTALL,
    VITTE_CLI_COMMAND_UNINSTALL,
    VITTE_CLI_COMMAND_UPDATE,

    VITTE_CLI_COMMAND_CLEAN,

    VITTE_CLI_COMMAND_VERSION,
    VITTE_CLI_COMMAND_HELP,

    VITTE_CLI_COMMAND_COUNT
} vitte_cli_command_t;

/* ========================================================================= */
/* Backend                                                                   */
/* ========================================================================= */

typedef enum vitte_cli_backend {
    VITTE_CLI_BACKEND_DEFAULT = 0,

    VITTE_CLI_BACKEND_C17,
    VITTE_CLI_BACKEND_NATIVE,
    VITTE_CLI_BACKEND_BYTECODE,

    VITTE_CLI_BACKEND_COUNT
} vitte_cli_backend_t;

/* ========================================================================= */
/* Optimization                                                              */
/* ========================================================================= */

typedef enum vitte_cli_optimization {
    VITTE_CLI_OPTIMIZATION_DEFAULT = 0,

    VITTE_CLI_OPTIMIZATION_NONE,
    VITTE_CLI_OPTIMIZATION_DEBUG,
    VITTE_CLI_OPTIMIZATION_BALANCED,
    VITTE_CLI_OPTIMIZATION_SPEED,
    VITTE_CLI_OPTIMIZATION_SIZE,
    VITTE_CLI_OPTIMIZATION_AGGRESSIVE,

    VITTE_CLI_OPTIMIZATION_COUNT
} vitte_cli_optimization_t;

/* ========================================================================= */
/* Diagnostic format                                                         */
/* ========================================================================= */

typedef enum vitte_cli_diagnostic_format {
    VITTE_CLI_DIAGNOSTIC_FORMAT_DEFAULT = 0,

    VITTE_CLI_DIAGNOSTIC_FORMAT_TERMINAL,
    VITTE_CLI_DIAGNOSTIC_FORMAT_SHORT,
    VITTE_CLI_DIAGNOSTIC_FORMAT_JSON,
    VITTE_CLI_DIAGNOSTIC_FORMAT_SARIF,
    VITTE_CLI_DIAGNOSTIC_FORMAT_LSP,

    VITTE_CLI_DIAGNOSTIC_FORMAT_COUNT
} vitte_cli_diagnostic_format_t;

/* ========================================================================= */
/* Color                                                                     */
/* ========================================================================= */

typedef enum vitte_cli_color_mode {
    VITTE_CLI_COLOR_AUTO = 0,

    VITTE_CLI_COLOR_ALWAYS,
    VITTE_CLI_COLOR_NEVER,

    VITTE_CLI_COLOR_COUNT
} vitte_cli_color_mode_t;

/* ========================================================================= */
/* Option provenance                                                         */
/* ========================================================================= */

/* C17 enumerators must fit in int; these flags require 64 bits. */
typedef uint64_t vitte_cli_seen_flag_t;

#define VITTE_CLI_SEEN_BACKEND (UINT64_C(1) << 0)
#define VITTE_CLI_SEEN_OPTIMIZATION (UINT64_C(1) << 1)
#define VITTE_CLI_SEEN_TARGET (UINT64_C(1) << 2)
#define VITTE_CLI_SEEN_OUTPUT (UINT64_C(1) << 3)
#define VITTE_CLI_SEEN_MANIFEST (UINT64_C(1) << 4)
#define VITTE_CLI_SEEN_JOBS (UINT64_C(1) << 5)
#define VITTE_CLI_SEEN_MAX_ERRORS (UINT64_C(1) << 6)
#define VITTE_CLI_SEEN_DIAGNOSTICS (UINT64_C(1) << 7)
#define VITTE_CLI_SEEN_COLOR (UINT64_C(1) << 8)
#define VITTE_CLI_SEEN_PACKAGE (UINT64_C(1) << 9)
#define VITTE_CLI_SEEN_TEST_FILTER (UINT64_C(1) << 10)
#define VITTE_CLI_SEEN_VERBOSE (UINT64_C(1) << 11)
#define VITTE_CLI_SEEN_QUIET (UINT64_C(1) << 12)
#define VITTE_CLI_SEEN_RELEASE (UINT64_C(1) << 13)
#define VITTE_CLI_SEEN_DEBUG (UINT64_C(1) << 14)
#define VITTE_CLI_SEEN_DEBUG_INFO (UINT64_C(1) << 15)
#define VITTE_CLI_SEEN_SOURCE_MAP (UINT64_C(1) << 16)
#define VITTE_CLI_SEEN_NO_SOURCE_MAP (UINT64_C(1) << 17)
#define VITTE_CLI_SEEN_EMIT_COMMENTS (UINT64_C(1) << 18)
#define VITTE_CLI_SEEN_NO_RUNTIME (UINT64_C(1) << 19)
#define VITTE_CLI_SEEN_NO_BOUNDS (UINT64_C(1) << 20)
#define VITTE_CLI_SEEN_NO_NULL (UINT64_C(1) << 21)
#define VITTE_CLI_SEEN_NO_OVERFLOW (UINT64_C(1) << 22)
#define VITTE_CLI_SEEN_WERROR (UINT64_C(1) << 23)
#define VITTE_CLI_SEEN_DETERMINISTIC (UINT64_C(1) << 24)
#define VITTE_CLI_SEEN_NO_DETERMINISTIC (UINT64_C(1) << 25)
#define VITTE_CLI_SEEN_INCREMENTAL (UINT64_C(1) << 26)
#define VITTE_CLI_SEEN_NO_INCREMENTAL (UINT64_C(1) << 27)
#define VITTE_CLI_SEEN_OFFLINE (UINT64_C(1) << 28)
#define VITTE_CLI_SEEN_LOCKED (UINT64_C(1) << 29)
#define VITTE_CLI_SEEN_FROZEN (UINT64_C(1) << 30)
#define VITTE_CLI_SEEN_FORCE (UINT64_C(1) << 31)
#define VITTE_CLI_SEEN_FMT_CHECK (UINT64_C(1) << 32)
#define VITTE_CLI_SEEN_FMT_WRITE (UINT64_C(1) << 33)
#define VITTE_CLI_SEEN_HELP (UINT64_C(1) << 34)
#define VITTE_CLI_SEEN_VERSION (UINT64_C(1) << 35)

/* ========================================================================= */
/* Message                                                                   */
/* ========================================================================= */

typedef struct vitte_cli_message {
    vitte_cli_error_t error;

    int argument_index;

    const char *argument;
    const char *detail;
} vitte_cli_message_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_cli_stats {
    size_t argument_count;
    size_t parsed_argument_count;

    size_t option_count;
    size_t positional_count;
    size_t consumed_value_count;

    size_t input_count;
    size_t forward_argument_count;

    uint64_t fingerprint;
} vitte_cli_stats_t;

/* ========================================================================= */
/* Parsed configuration                                                      */
/* ========================================================================= */

typedef struct vitte_cli_config {
    vitte_cli_command_t command;

    vitte_cli_backend_t backend;
    vitte_cli_optimization_t optimization;

    vitte_cli_diagnostic_format_t diagnostic_format;
    vitte_cli_color_mode_t color;

    const char *target;
    const char *output;
    const char *manifest_path;

    const char *package;
    const char *test_filter;

    size_t jobs;
    size_t max_errors;

    bool verbose;
    bool quiet;

    bool release;
    bool debug;

    bool emit_debug_info;
    bool emit_source_map;
    bool emit_comments;

    bool runtime_checks;
    bool bounds_checks;
    bool null_checks;
    bool overflow_checks;

    bool warnings_as_errors;

    bool deterministic;
    bool incremental;

    bool offline;
    bool locked;
    bool frozen;

    bool force;

    bool check_format;
    bool write_format;

    bool show_help;
    bool show_version;
} vitte_cli_config_t;

/* ========================================================================= */
/* CLI object                                                                */
/* ========================================================================= */

/*
 * The CLI object owns only its pointer arrays.
 *
 * Strings referenced by:
 *
 *   inputs
 *   forward_args
 *   target
 *   output
 *   manifest_path
 *   package
 *   test_filter
 *   message.argument
 *   message.detail
 *
 * are borrowed. Their lifetime must therefore cover the lifetime of the
 * parsed CLI state.
 *
 * In the usual argc/argv use case this condition is naturally satisfied.
 */
typedef struct vitte_cli {
    uint64_t magic;

    vitte_cli_state_t state;
    vitte_cli_error_t last_error;

    vitte_cli_config_t config;

    const char **inputs;
    size_t input_count;
    size_t input_capacity;

    const char **forward_args;
    size_t forward_arg_count;
    size_t forward_arg_capacity;

    size_t max_inputs;
    size_t max_forward_args;

    uint64_t seen_options;

    vitte_cli_message_t message;
    vitte_cli_stats_t stats;
} vitte_cli_t;

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize a CLI parser.
 *
 * The object must either be uninitialized storage or no longer own resources.
 * Call vitte_cli_destroy() before discarding an initialized object.
 */
bool
vitte_cli_init(
    vitte_cli_t *cli);

/*
 * Reset an initialized parser while retaining allocated pointer storage.
 *
 * The configured logical limits are preserved.
 */
bool
vitte_cli_reset(
    vitte_cli_t *cli);

/*
 * Release all storage owned by the parser.
 *
 * Safe to call with NULL.
 */
void
vitte_cli_destroy(
    vitte_cli_t *cli);

/* ========================================================================= */
/* Parsing                                                                   */
/* ========================================================================= */

/*
 * Parse argc/argv.
 *
 * argv strings are borrowed and are not copied.
 *
 * The parser must be in VITTE_CLI_STATE_READY.
 *
 * On success:
 *
 *   state      = VITTE_CLI_STATE_PARSED
 *   last_error = VITTE_CLI_ERROR_NONE
 *
 * On failure:
 *
 *   state      = VITTE_CLI_STATE_FAILED
 *   last_error identifies the parser failure
 *   message contains contextual information where available
 *
 * Call vitte_cli_reset() before parsing another command line.
 */
bool
vitte_cli_parse(
    vitte_cli_t *cli,
    int argc,
    const char *const argv[]);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate the internal structure of a CLI object.
 *
 * This validates structural invariants. It does not reparse argv.
 */
bool
vitte_cli_is_valid(
    const vitte_cli_t *cli);

/*
 * Validate a parsed/default configuration independently of a CLI object.
 */
bool
vitte_cli_config_validate(
    const vitte_cli_config_t *config);

/* ========================================================================= */
/* Defaults                                                                  */
/* ========================================================================= */

vitte_cli_config_t
vitte_cli_config_default(void);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

const vitte_cli_config_t *
vitte_cli_config(
    const vitte_cli_t *cli);

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

vitte_cli_state_t
vitte_cli_state(
    const vitte_cli_t *cli);

vitte_cli_error_t
vitte_cli_last_error(
    const vitte_cli_t *cli);

const vitte_cli_message_t *
vitte_cli_message(
    const vitte_cli_t *cli);

vitte_cli_stats_t
vitte_cli_stats(
    const vitte_cli_t *cli);

/* ========================================================================= */
/* Inputs                                                                    */
/* ========================================================================= */

size_t
vitte_cli_input_count(
    const vitte_cli_t *cli);

const char *
vitte_cli_input_at(
    const vitte_cli_t *cli,
    size_t index);

/* ========================================================================= */
/* Forwarded arguments                                                       */
/* ========================================================================= */

/*
 * Arguments following "--" for commands that execute code are stored here.
 *
 * Example:
 *
 *   vitte run main.vit -- hello world
 *
 * produces:
 *
 *   inputs[0]       = "main.vit"
 *   forward_args[0] = "hello"
 *   forward_args[1] = "world"
 */
size_t
vitte_cli_forward_arg_count(
    const vitte_cli_t *cli);

const char *
vitte_cli_forward_arg_at(
    const vitte_cli_t *cli,
    size_t index);

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

/*
 * Set parser storage limits.
 *
 * Must be called while the parser is in READY state.
 *
 * Existing allocated capacity is never silently discarded. Therefore a new
 * limit smaller than already reserved capacity is rejected.
 */
bool
vitte_cli_set_limits(
    vitte_cli_t *cli,
    size_t max_inputs,
    size_t max_forward_args);

/* ========================================================================= */
/* Option provenance                                                         */
/* ========================================================================= */

uint64_t
vitte_cli_seen_options(
    const vitte_cli_t *cli);

bool
vitte_cli_option_was_seen(
    const vitte_cli_t *cli,
    uint64_t flag);

/* ========================================================================= */
/* Command classification                                                    */
/* ========================================================================= */

bool
vitte_cli_command_is_compilation(
    vitte_cli_command_t command);

bool
vitte_cli_command_uses_package_manager(
    vitte_cli_command_t command);

bool
vitte_cli_command_executes_program(
    vitte_cli_command_t command);

/* ========================================================================= */
/* Stable names                                                              */
/* ========================================================================= */

const char *
vitte_cli_error_name(
    vitte_cli_error_t error);

const char *
vitte_cli_state_name(
    vitte_cli_state_t state);

const char *
vitte_cli_command_name(
    vitte_cli_command_t command);

const char *
vitte_cli_backend_name(
    vitte_cli_backend_t backend);

const char *
vitte_cli_optimization_name(
    vitte_cli_optimization_t optimization);

const char *
vitte_cli_diagnostic_format_name(
    vitte_cli_diagnostic_format_t format);

const char *
vitte_cli_color_name(
    vitte_cli_color_mode_t color);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Compute a deterministic fingerprint of the parsed semantic command line.
 *
 * The fingerprint covers:
 *
 *   command
 *   backend
 *   optimization
 *   diagnostics
 *   color policy
 *   target/output/manifest/package/filter
 *   compilation switches
 *   package policy
 *   input ordering
 *   forwarded argument ordering
 *
 * Parser-internal allocation capacities are deliberately excluded.
 *
 * Returns zero for an invalid CLI object.
 */
uint64_t
vitte_cli_fingerprint(
    const vitte_cli_t *cli);

/* ========================================================================= */
/* Process integration                                                       */
/* ========================================================================= */

/*
 * Return the conventional process exit status suggested by the CLI parser.
 *
 *   0 -> parser state itself is successful
 *   2 -> command-line usage/parsing failure
 *
 * Compilation/runtime failures are intentionally outside this function.
 */
int
vitte_cli_suggested_exit_status(
    const vitte_cli_t *cli);

/* ========================================================================= */
/* Convenience                                                               */
/* ========================================================================= */

static inline bool
vitte_cli_is_ready(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        vitte_cli_state(cli) ==
            VITTE_CLI_STATE_READY;
}

static inline bool
vitte_cli_is_parsed(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        vitte_cli_state(cli) ==
            VITTE_CLI_STATE_PARSED;
}

static inline bool
vitte_cli_has_failed(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        vitte_cli_state(cli) ==
            VITTE_CLI_STATE_FAILED;
}

static inline bool
vitte_cli_has_inputs(
    const vitte_cli_t *cli)
{
    return
        vitte_cli_input_count(cli) != 0u;
}

static inline bool
vitte_cli_has_forward_args(
    const vitte_cli_t *cli)
{
    return
        vitte_cli_forward_arg_count(cli) != 0u;
}

static inline bool
vitte_cli_wants_help(
    const vitte_cli_t *cli)
{
    const vitte_cli_config_t *config;

    config = vitte_cli_config(cli);

    return
        config != NULL &&
        config->show_help;
}

static inline bool
vitte_cli_wants_version(
    const vitte_cli_t *cli)
{
    const vitte_cli_config_t *config;

    config = vitte_cli_config(cli);

    return
        config != NULL &&
        config->show_version;
}

static inline vitte_cli_command_t
vitte_cli_selected_command(
    const vitte_cli_t *cli)
{
    const vitte_cli_config_t *config;

    config = vitte_cli_config(cli);

    if (config == NULL) {
        return VITTE_CLI_COMMAND_NONE;
    }

    return config->command;
}

static inline vitte_cli_backend_t
vitte_cli_selected_backend(
    const vitte_cli_t *cli)
{
    const vitte_cli_config_t *config;

    config = vitte_cli_config(cli);

    if (config == NULL) {
        return VITTE_CLI_BACKEND_DEFAULT;
    }

    return config->backend;
}

static inline vitte_cli_optimization_t
vitte_cli_selected_optimization(
    const vitte_cli_t *cli)
{
    const vitte_cli_config_t *config;

    config = vitte_cli_config(cli);

    if (config == NULL) {
        return VITTE_CLI_OPTIMIZATION_DEFAULT;
    }

    return config->optimization;
}

static inline vitte_cli_diagnostic_format_t
vitte_cli_selected_diagnostic_format(
    const vitte_cli_t *cli)
{
    const vitte_cli_config_t *config;

    config = vitte_cli_config(cli);

    if (config == NULL) {
        return
            VITTE_CLI_DIAGNOSTIC_FORMAT_DEFAULT;
    }

    return config->diagnostic_format;
}

static inline vitte_cli_color_mode_t
vitte_cli_selected_color_mode(
    const vitte_cli_t *cli)
{
    const vitte_cli_config_t *config;

    config = vitte_cli_config(cli);

    if (config == NULL) {
        return VITTE_CLI_COLOR_AUTO;
    }

    return config->color;
}

#ifdef __cplusplus
}
#endif

#endif /* VITTE_CLI_CLI_H */