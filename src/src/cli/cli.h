#ifndef VITTE_CLI_CLI_H
#define VITTE_CLI_CLI_H

/*
 * Vitte Compiler
 * src/cli/cli.h
 *
 * Canonical command-line interface API.
 *
 * The CLI layer parses and validates command-line intent.
 * Execution is delegated to the compiler driver and specialized subsystems.
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

#define VITTE_CLI_MAGIC \
    UINT64_C(0x5649545445434C49)

#define VITTE_CLI_DEAD_MAGIC \
    UINT64_C(0x44454144434C4921)

#define VITTE_CLI_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_CLI_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_CLI_DEFAULT_MAX_INPUTS \
    ((size_t)65536u)

#define VITTE_CLI_DEFAULT_MAX_FORWARD_ARGS \
    ((size_t)65536u)

#define VITTE_CLI_DEFAULT_INITIAL_CAPACITY \
    ((size_t)8u)

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
/* Message                                                                   */
/* ========================================================================= */

typedef struct vitte_cli_message {
    vitte_cli_error_t error;

    /*
     * argv index associated with the failure.
     *
     * -1 means that the error is not associated with one specific argument.
     */
    int argument_index;

    /*
     * Borrowed pointers.
     *
     * The CLI parser never takes ownership of argv strings.
     */
    const char *argument;
    const char *detail;
} vitte_cli_message_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_cli_stats {
    size_t argument_count;
    size_t option_count;

    size_t input_count;
    size_t forward_argument_count;

    uint64_t fingerprint;
} vitte_cli_stats_t;

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

typedef struct vitte_cli_config {
    vitte_cli_command_t command;

    vitte_cli_backend_t backend;
    vitte_cli_optimization_t optimization;

    vitte_cli_diagnostic_format_t diagnostic_format;
    vitte_cli_color_mode_t color;

    /*
     * Borrowed strings originating from argv.
     */
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

typedef struct vitte_cli {
    uint64_t magic;

    vitte_cli_state_t state;
    vitte_cli_error_t last_error;

    vitte_cli_config_t config;

    /*
     * Dynamic arrays are owned by the CLI object.
     *
     * The strings referenced by the arrays are borrowed from argv.
     */
    const char **inputs;
    size_t input_count;
    size_t input_capacity;

    const char **forward_args;
    size_t forward_arg_count;
    size_t forward_arg_capacity;

    size_t max_inputs;
    size_t max_forward_args;

    vitte_cli_message_t message;
    vitte_cli_stats_t stats;
} vitte_cli_t;

/* ========================================================================= */
/* Error / state names                                                       */
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
/* Defaults                                                                  */
/* ========================================================================= */

vitte_cli_config_t
vitte_cli_config_default(void);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_cli_config_validate(
    const vitte_cli_config_t *config);

bool
vitte_cli_is_valid(
    const vitte_cli_t *cli);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_cli_init(
    vitte_cli_t *cli);

void
vitte_cli_destroy(
    vitte_cli_t *cli);

bool
vitte_cli_reset(
    vitte_cli_t *cli);

/* ========================================================================= */
/* Parsing                                                                   */
/* ========================================================================= */

/*
 * Parse argc/argv.
 *
 * argv strings remain owned by the caller.
 *
 * The caller must therefore keep argv and all referenced strings alive while
 * using the resulting CLI configuration.
 *
 * Typical usage:
 *
 *     vitte_cli_t cli;
 *
 *     if (!vitte_cli_init(&cli)) {
 *         return 1;
 *     }
 *
 *     if (!vitte_cli_parse(
 *             &cli,
 *             argc,
 *             (const char *const *)argv)) {
 *         ...
 *     }
 *
 *     ...
 *
 *     vitte_cli_destroy(&cli);
 */
bool
vitte_cli_parse(
    vitte_cli_t *cli,
    int argc,
    const char *const argv[]);

/* ========================================================================= */
/* Configuration access                                                      */
/* ========================================================================= */

const vitte_cli_config_t *
vitte_cli_config(
    const vitte_cli_t *cli);

/* ========================================================================= */
/* Error access                                                              */
/* ========================================================================= */

vitte_cli_error_t
vitte_cli_last_error(
    const vitte_cli_t *cli);

const vitte_cli_message_t *
vitte_cli_message(
    const vitte_cli_t *cli);

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

vitte_cli_state_t
vitte_cli_state(
    const vitte_cli_t *cli);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_cli_stats_t
vitte_cli_stats(
    const vitte_cli_t *cli);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Compute a deterministic fingerprint of parsed CLI intent.
 *
 * The fingerprint is based on semantic configuration and argument strings,
 * not pointer addresses.
 */
uint64_t
vitte_cli_fingerprint(
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
 * Arguments following "--" for commands such as:
 *
 *     vitte run main.vit -- arg1 arg2
 *
 * are available through this API.
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
 * Configure parser storage limits.
 *
 * This operation is only valid while the CLI object is in READY state.
 */
bool
vitte_cli_set_limits(
    vitte_cli_t *cli,
    size_t max_inputs,
    size_t max_forward_args);

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
/* Exit status                                                               */
/* ========================================================================= */

/*
 * Return a suggested process exit status for CLI parsing itself.
 *
 * 0:
 *     CLI parsing/configuration is usable.
 *
 * 2:
 *     command-line usage/parsing error.
 *
 * Compiler, linker, test and executed-program exit codes belong to the
 * driver rather than this parser.
 */
int
vitte_cli_suggested_exit_status(
    const vitte_cli_t *cli);

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_cli_error_is_valid(
    vitte_cli_error_t error)
{
    return
        error >= VITTE_CLI_ERROR_NONE &&
        error < VITTE_CLI_ERROR_COUNT;
}

static inline bool
vitte_cli_state_is_valid(
    vitte_cli_state_t state)
{
    return
        state > VITTE_CLI_STATE_INVALID &&
        state < VITTE_CLI_STATE_COUNT;
}

static inline bool
vitte_cli_command_is_valid(
    vitte_cli_command_t command)
{
    return
        command >= VITTE_CLI_COMMAND_NONE &&
        command < VITTE_CLI_COMMAND_COUNT;
}

static inline bool
vitte_cli_backend_is_valid(
    vitte_cli_backend_t backend)
{
    return
        backend >= VITTE_CLI_BACKEND_DEFAULT &&
        backend < VITTE_CLI_BACKEND_COUNT;
}

static inline bool
vitte_cli_optimization_is_valid(
    vitte_cli_optimization_t optimization)
{
    return
        optimization >=
            VITTE_CLI_OPTIMIZATION_DEFAULT &&
        optimization <
            VITTE_CLI_OPTIMIZATION_COUNT;
}

static inline bool
vitte_cli_diagnostic_format_is_valid(
    vitte_cli_diagnostic_format_t format)
{
    return
        format >=
            VITTE_CLI_DIAGNOSTIC_FORMAT_DEFAULT &&
        format <
            VITTE_CLI_DIAGNOSTIC_FORMAT_COUNT;
}

static inline bool
vitte_cli_color_is_valid(
    vitte_cli_color_mode_t color)
{
    return
        color >= VITTE_CLI_COLOR_AUTO &&
        color < VITTE_CLI_COLOR_COUNT;
}

/* ========================================================================= */
/* Inline lifecycle queries                                                  */
/* ========================================================================= */

static inline bool
vitte_cli_is_ready(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->state == VITTE_CLI_STATE_READY;
}

static inline bool
vitte_cli_is_parsing(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->state == VITTE_CLI_STATE_PARSING;
}

static inline bool
vitte_cli_is_parsed(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->state == VITTE_CLI_STATE_PARSED;
}

static inline bool
vitte_cli_has_failed(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->state == VITTE_CLI_STATE_FAILED;
}

/* ========================================================================= */
/* Inline configuration queries                                              */
/* ========================================================================= */

static inline bool
vitte_cli_has_target(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.target != NULL;
}

static inline bool
vitte_cli_has_output(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.output != NULL;
}

static inline bool
vitte_cli_has_manifest(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.manifest_path != NULL;
}

static inline bool
vitte_cli_has_package(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.package != NULL;
}

static inline bool
vitte_cli_has_test_filter(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.test_filter != NULL;
}

/* ========================================================================= */
/* Inline command queries                                                    */
/* ========================================================================= */

static inline bool
vitte_cli_is_build_command(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.command ==
            VITTE_CLI_COMMAND_BUILD;
}

static inline bool
vitte_cli_is_run_command(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.command ==
            VITTE_CLI_COMMAND_RUN;
}

static inline bool
vitte_cli_is_check_command(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.command ==
            VITTE_CLI_COMMAND_CHECK;
}

static inline bool
vitte_cli_is_test_command(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.command ==
            VITTE_CLI_COMMAND_TEST;
}

static inline bool
vitte_cli_is_fmt_command(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.command ==
            VITTE_CLI_COMMAND_FMT;
}

static inline bool
vitte_cli_is_install_command(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.command ==
            VITTE_CLI_COMMAND_INSTALL;
}

static inline bool
vitte_cli_is_help_command(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.command ==
            VITTE_CLI_COMMAND_HELP;
}

static inline bool
vitte_cli_is_version_command(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.command ==
            VITTE_CLI_COMMAND_VERSION;
}

/* ========================================================================= */
/* Inline safety policy                                                      */
/* ========================================================================= */

static inline bool
vitte_cli_all_runtime_checks_enabled(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.runtime_checks &&
        cli->config.bounds_checks &&
        cli->config.null_checks &&
        cli->config.overflow_checks;
}

static inline bool
vitte_cli_is_offline(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.offline;
}

static inline bool
vitte_cli_is_frozen(
    const vitte_cli_t *cli)
{
    return
        cli != NULL &&
        cli->magic == VITTE_CLI_MAGIC &&
        cli->config.frozen;
}

/* ========================================================================= */
/* C++                                                                      */
/* ========================================================================= */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_CLI_CLI_H */
