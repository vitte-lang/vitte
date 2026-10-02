/*
 * Vitte Compiler
 * src/cli/cli.c
 *
 * Canonical command-line interface parser.
 *
 * Responsibilities:
 *   - command recognition
 *   - global option parsing
 *   - command-specific option parsing
 *   - positional argument collection
 *   - target/backend/optimization parsing
 *   - diagnostic-output configuration
 *   - build/run/check/test/fmt/install options
 *   - -- handling
 *   - strict numeric parsing
 *   - duplicate/conflicting option validation
 *   - deterministic configuration
 *   - error reporting
 *   - CLI statistics
 *
 * This layer deliberately does NOT:
 *   - compile source files
 *   - invoke the linker
 *   - execute generated programs
 *   - install packages
 *   - format source code
 *   - render compiler diagnostics
 *
 * It parses and validates user intent. The driver executes that intent.
 *
 * ISO C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

    int argument_index;

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

    vitte_cli_message_t message;
    vitte_cli_stats_t stats;
} vitte_cli_t;

/* ========================================================================= */
/* Internal option-seen mask                                                 */
/* ========================================================================= */

typedef enum vitte_cli_seen_flag {
    VITTE_CLI_SEEN_BACKEND        = UINT64_C(1) << 0,
    VITTE_CLI_SEEN_OPTIMIZATION   = UINT64_C(1) << 1,
    VITTE_CLI_SEEN_TARGET         = UINT64_C(1) << 2,
    VITTE_CLI_SEEN_OUTPUT         = UINT64_C(1) << 3,
    VITTE_CLI_SEEN_MANIFEST       = UINT64_C(1) << 4,
    VITTE_CLI_SEEN_JOBS           = UINT64_C(1) << 5,
    VITTE_CLI_SEEN_MAX_ERRORS     = UINT64_C(1) << 6,
    VITTE_CLI_SEEN_DIAGNOSTICS    = UINT64_C(1) << 7,
    VITTE_CLI_SEEN_COLOR          = UINT64_C(1) << 8,
    VITTE_CLI_SEEN_PACKAGE        = UINT64_C(1) << 9,
    VITTE_CLI_SEEN_TEST_FILTER    = UINT64_C(1) << 10
} vitte_cli_seen_flag_t;

/* ========================================================================= */
/* Safe arithmetic                                                           */
/* ========================================================================= */

static bool
vitte_cli_size_add(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left > SIZE_MAX - right) {
        return false;
    }

    *result = left + right;

    return true;
}

static bool
vitte_cli_size_mul(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left != 0u &&
        right > SIZE_MAX / left) {
        return false;
    }

    *result = left * right;

    return true;
}

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

static uint64_t
vitte_cli_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    size_t index;

    bytes =
        (const unsigned char *)data;

    for (index = 0u;
         index < length;
         ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_CLI_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_cli_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned shift;

    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        unsigned char byte;

        byte =
            (unsigned char)(
                (value >> shift) &
                UINT64_C(0xff));

        hash =
            vitte_cli_hash_bytes(
                hash,
                &byte,
                1u);
    }

    return hash;
}

static uint64_t
vitte_cli_hash_string(
    uint64_t hash,
    const char *text)
{
    if (text == NULL) {
        return
            vitte_cli_hash_u64(
                hash,
                UINT64_MAX);
    }

    hash =
        vitte_cli_hash_bytes(
            hash,
            text,
            strlen(text));

    return
        vitte_cli_hash_u64(
            hash,
            UINT64_C(0));
}

/* ========================================================================= */
/* String helpers                                                            */
/* ========================================================================= */

static bool
vitte_cli_string_equal(
    const char *left,
    const char *right)
{
    if (left == NULL ||
        right == NULL) {
        return false;
    }

    return strcmp(left, right) == 0;
}

static bool
vitte_cli_is_option(
    const char *argument)
{
    return
        argument != NULL &&
        argument[0] == '-' &&
        argument[1] != '\0';
}

static const char *
vitte_cli_inline_value(
    const char *argument,
    const char *option)
{
    size_t length;

    if (argument == NULL ||
        option == NULL) {
        return NULL;
    }

    length = strlen(option);

    if (strncmp(
            argument,
            option,
            length) != 0) {
        return NULL;
    }

    if (argument[length] != '=') {
        return NULL;
    }

    return argument + length + 1u;
}

/* ========================================================================= */
/* Numeric parsing                                                           */
/* ========================================================================= */

static bool
vitte_cli_parse_size(
    const char *text,
    size_t *value)
{
    size_t result;
    size_t index;

    if (text == NULL ||
        value == NULL ||
        text[0] == '\0') {
        return false;
    }

    result = 0u;

    for (index = 0u;
         text[index] != '\0';
         ++index) {
        unsigned digit;

        if (text[index] < '0' ||
            text[index] > '9') {
            return false;
        }

        digit =
            (unsigned)(
                text[index] - '0');

        if (result >
            (SIZE_MAX - (size_t)digit) /
                10u) {
            return false;
        }

        result =
            result * 10u +
            (size_t)digit;
    }

    *value = result;

    return true;
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_cli_error_name(
    vitte_cli_error_t error)
{
    switch (error) {
        case VITTE_CLI_ERROR_NONE:
            return "none";

        case VITTE_CLI_ERROR_INVALID_CLI:
            return "invalid-cli";

        case VITTE_CLI_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_CLI_ERROR_INVALID_STATE:
            return "invalid-state";

        case VITTE_CLI_ERROR_UNKNOWN_COMMAND:
            return "unknown-command";

        case VITTE_CLI_ERROR_UNKNOWN_OPTION:
            return "unknown-option";

        case VITTE_CLI_ERROR_MISSING_OPTION_VALUE:
            return "missing-option-value";

        case VITTE_CLI_ERROR_INVALID_OPTION_VALUE:
            return "invalid-option-value";

        case VITTE_CLI_ERROR_DUPLICATE_OPTION:
            return "duplicate-option";

        case VITTE_CLI_ERROR_CONFLICTING_OPTIONS:
            return "conflicting-options";

        case VITTE_CLI_ERROR_MISSING_INPUT:
            return "missing-input";

        case VITTE_CLI_ERROR_TOO_MANY_INPUTS:
            return "too-many-inputs";

        case VITTE_CLI_ERROR_TOO_MANY_FORWARD_ARGS:
            return "too-many-forward-args";

        case VITTE_CLI_ERROR_INVALID_TARGET:
            return "invalid-target";

        case VITTE_CLI_ERROR_INVALID_BACKEND:
            return "invalid-backend";

        case VITTE_CLI_ERROR_INVALID_OPTIMIZATION:
            return "invalid-optimization";

        case VITTE_CLI_ERROR_INVALID_DIAGNOSTIC_FORMAT:
            return "invalid-diagnostic-format";

        case VITTE_CLI_ERROR_INVALID_COLOR_MODE:
            return "invalid-color-mode";

        case VITTE_CLI_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_CLI_ERROR_OUT_OF_MEMORY:
            return "out-of-memory";

        case VITTE_CLI_ERROR_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_cli_state_name(
    vitte_cli_state_t state)
{
    switch (state) {
        case VITTE_CLI_STATE_INVALID:
            return "invalid";

        case VITTE_CLI_STATE_READY:
            return "ready";

        case VITTE_CLI_STATE_PARSING:
            return "parsing";

        case VITTE_CLI_STATE_PARSED:
            return "parsed";

        case VITTE_CLI_STATE_FAILED:
            return "failed";

        case VITTE_CLI_STATE_DESTROYED:
            return "destroyed";

        case VITTE_CLI_STATE_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_cli_command_name(
    vitte_cli_command_t command)
{
    switch (command) {
        case VITTE_CLI_COMMAND_NONE:
            return "none";

        case VITTE_CLI_COMMAND_BUILD:
            return "build";

        case VITTE_CLI_COMMAND_RUN:
            return "run";

        case VITTE_CLI_COMMAND_CHECK:
            return "check";

        case VITTE_CLI_COMMAND_TEST:
            return "test";

        case VITTE_CLI_COMMAND_FMT:
            return "fmt";

        case VITTE_CLI_COMMAND_INSTALL:
            return "install";

        case VITTE_CLI_COMMAND_UNINSTALL:
            return "uninstall";

        case VITTE_CLI_COMMAND_UPDATE:
            return "update";

        case VITTE_CLI_COMMAND_CLEAN:
            return "clean";

        case VITTE_CLI_COMMAND_VERSION:
            return "version";

        case VITTE_CLI_COMMAND_HELP:
            return "help";

        case VITTE_CLI_COMMAND_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_cli_backend_name(
    vitte_cli_backend_t backend)
{
    switch (backend) {
        case VITTE_CLI_BACKEND_DEFAULT:
            return "default";

        case VITTE_CLI_BACKEND_C17:
            return "c17";

        case VITTE_CLI_BACKEND_NATIVE:
            return "native";

        case VITTE_CLI_BACKEND_BYTECODE:
            return "bytecode";

        case VITTE_CLI_BACKEND_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_cli_optimization_name(
    vitte_cli_optimization_t optimization)
{
    switch (optimization) {
        case VITTE_CLI_OPTIMIZATION_DEFAULT:
            return "default";

        case VITTE_CLI_OPTIMIZATION_NONE:
            return "none";

        case VITTE_CLI_OPTIMIZATION_DEBUG:
            return "debug";

        case VITTE_CLI_OPTIMIZATION_BALANCED:
            return "balanced";

        case VITTE_CLI_OPTIMIZATION_SPEED:
            return "speed";

        case VITTE_CLI_OPTIMIZATION_SIZE:
            return "size";

        case VITTE_CLI_OPTIMIZATION_AGGRESSIVE:
            return "aggressive";

        case VITTE_CLI_OPTIMIZATION_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_cli_diagnostic_format_name(
    vitte_cli_diagnostic_format_t format)
{
    switch (format) {
        case VITTE_CLI_DIAGNOSTIC_FORMAT_DEFAULT:
            return "default";

        case VITTE_CLI_DIAGNOSTIC_FORMAT_TERMINAL:
            return "terminal";

        case VITTE_CLI_DIAGNOSTIC_FORMAT_SHORT:
            return "short";

        case VITTE_CLI_DIAGNOSTIC_FORMAT_JSON:
            return "json";

        case VITTE_CLI_DIAGNOSTIC_FORMAT_SARIF:
            return "sarif";

        case VITTE_CLI_DIAGNOSTIC_FORMAT_LSP:
            return "lsp";

        case VITTE_CLI_DIAGNOSTIC_FORMAT_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_cli_color_name(
    vitte_cli_color_mode_t color)
{
    switch (color) {
        case VITTE_CLI_COLOR_AUTO:
            return "auto";

        case VITTE_CLI_COLOR_ALWAYS:
            return "always";

        case VITTE_CLI_COLOR_NEVER:
            return "never";

        case VITTE_CLI_COLOR_COUNT:
            return "count";
    }

    return "invalid";
}

/* ========================================================================= */
/* Defaults                                                                  */
/* ========================================================================= */

vitte_cli_config_t
vitte_cli_config_default(void)
{
    vitte_cli_config_t config;

    memset(
        &config,
        0,
        sizeof(config));

    config.command =
        VITTE_CLI_COMMAND_NONE;

    config.backend =
        VITTE_CLI_BACKEND_DEFAULT;

    config.optimization =
        VITTE_CLI_OPTIMIZATION_DEFAULT;

    config.diagnostic_format =
        VITTE_CLI_DIAGNOSTIC_FORMAT_DEFAULT;

    config.color =
        VITTE_CLI_COLOR_AUTO;

    config.jobs =
        VITTE_CLI_DEFAULT_JOBS;

    config.max_errors =
        VITTE_CLI_DEFAULT_MAX_ERRORS;

    config.runtime_checks = true;
    config.bounds_checks = true;
    config.null_checks = true;
    config.overflow_checks = true;

    config.deterministic = true;
    config.incremental = true;

    return config;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_cli_config_validate(
    const vitte_cli_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->command < VITTE_CLI_COMMAND_NONE ||
        config->command >= VITTE_CLI_COMMAND_COUNT) {
        return false;
    }

    if (config->backend < VITTE_CLI_BACKEND_DEFAULT ||
        config->backend >= VITTE_CLI_BACKEND_COUNT) {
        return false;
    }

    if (config->optimization <
            VITTE_CLI_OPTIMIZATION_DEFAULT ||
        config->optimization >=
            VITTE_CLI_OPTIMIZATION_COUNT) {
        return false;
    }

    if (config->diagnostic_format <
            VITTE_CLI_DIAGNOSTIC_FORMAT_DEFAULT ||
        config->diagnostic_format >=
            VITTE_CLI_DIAGNOSTIC_FORMAT_COUNT) {
        return false;
    }

    if (config->color < VITTE_CLI_COLOR_AUTO ||
        config->color >= VITTE_CLI_COLOR_COUNT) {
        return false;
    }

    if (config->jobs == 0u ||
        config->max_errors == 0u) {
        return false;
    }

    if (config->verbose &&
        config->quiet) {
        return false;
    }

    if (config->release &&
        config->debug) {
        return false;
    }

    if (config->check_format &&
        config->write_format) {
        return false;
    }

    return true;
}

bool
vitte_cli_is_valid(
    const vitte_cli_t *cli)
{
    if (cli == NULL) {
        return false;
    }

    if (cli->magic !=
        VITTE_CLI_MAGIC) {
        return false;
    }

    if (cli->state <=
            VITTE_CLI_STATE_INVALID ||
        cli->state >=
            VITTE_CLI_STATE_DESTROYED) {
        return false;
    }

    if (cli->input_count >
        cli->input_capacity) {
        return false;
    }

    if (cli->forward_arg_count >
        cli->forward_arg_capacity) {
        return false;
    }

    if (cli->input_count != 0u &&
        cli->inputs == NULL) {
        return false;
    }

    if (cli->forward_arg_count != 0u &&
        cli->forward_args == NULL) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Failure                                                                   */
/* ========================================================================= */

static bool
vitte_cli_fail(
    vitte_cli_t *cli,
    vitte_cli_error_t error,
    int argument_index,
    const char *argument,
    const char *detail)
{
    if (cli == NULL) {
        return false;
    }

    cli->last_error = error;
    cli->state = VITTE_CLI_STATE_FAILED;

    cli->message.error = error;
    cli->message.argument_index = argument_index;
    cli->message.argument = argument;
    cli->message.detail = detail;

    return false;
}

/* ========================================================================= */
/* Capacity                                                                  */
/* ========================================================================= */

static bool
vitte_cli_reserve_pointer_array(
    vitte_cli_t *cli,
    const char ***array,
    size_t *capacity,
    size_t required,
    size_t maximum)
{
    const char **new_array;
    size_t next_capacity;
    size_t bytes;

    if (required <= *capacity) {
        return true;
    }

    if (required > maximum) {
        return false;
    }

    next_capacity = *capacity;

    if (next_capacity == 0u) {
        next_capacity =
            VITTE_CLI_DEFAULT_INITIAL_CAPACITY;
    }

    while (next_capacity < required) {
        size_t doubled;

        if (!vitte_cli_size_add(
                next_capacity,
                next_capacity,
                &doubled)) {
            return
                vitte_cli_fail(
                    cli,
                    VITTE_CLI_ERROR_OVERFLOW,
                    -1,
                    NULL,
                    "argument storage capacity overflow");
        }

        next_capacity = doubled;

        if (next_capacity > maximum) {
            next_capacity = maximum;
            break;
        }
    }

    if (next_capacity < required) {
        return false;
    }

    if (!vitte_cli_size_mul(
            next_capacity,
            sizeof(*new_array),
            &bytes)) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_OVERFLOW,
                -1,
                NULL,
                "argument storage allocation overflow");
    }

    new_array =
        (const char **)realloc(
            (void *)*array,
            bytes);

    if (new_array == NULL) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_OUT_OF_MEMORY,
                -1,
                NULL,
                "unable to grow CLI argument storage");
    }

    *array = new_array;
    *capacity = next_capacity;

    return true;
}

/* ========================================================================= */
/* Input management                                                          */
/* ========================================================================= */

static bool
vitte_cli_add_input(
    vitte_cli_t *cli,
    const char *input,
    int argument_index)
{
    size_t required;

    if (input == NULL ||
        input[0] == '\0') {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_INVALID_ARGUMENT,
                argument_index,
                input,
                "empty input argument");
    }

    if (!vitte_cli_size_add(
            cli->input_count,
            1u,
            &required)) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_OVERFLOW,
                argument_index,
                input,
                "input count overflow");
    }

    if (required >
        cli->max_inputs) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_TOO_MANY_INPUTS,
                argument_index,
                input,
                "maximum number of input arguments exceeded");
    }

    if (!vitte_cli_reserve_pointer_array(
            cli,
            &cli->inputs,
            &cli->input_capacity,
            required,
            cli->max_inputs)) {
        if (cli->state ==
            VITTE_CLI_STATE_FAILED) {
            return false;
        }

        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_TOO_MANY_INPUTS,
                argument_index,
                input,
                "unable to store input argument");
    }

    cli->inputs[cli->input_count] =
        input;

    ++cli->input_count;

    return true;
}

static bool
vitte_cli_add_forward_arg(
    vitte_cli_t *cli,
    const char *argument,
    int argument_index)
{
    size_t required;

    if (argument == NULL) {
        return false;
    }

    if (!vitte_cli_size_add(
            cli->forward_arg_count,
            1u,
            &required)) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_OVERFLOW,
                argument_index,
                argument,
                "forward argument count overflow");
    }

    if (required >
        cli->max_forward_args) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_TOO_MANY_FORWARD_ARGS,
                argument_index,
                argument,
                "maximum number of forwarded arguments exceeded");
    }

    if (!vitte_cli_reserve_pointer_array(
            cli,
            &cli->forward_args,
            &cli->forward_arg_capacity,
            required,
            cli->max_forward_args)) {
        if (cli->state ==
            VITTE_CLI_STATE_FAILED) {
            return false;
        }

        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_TOO_MANY_FORWARD_ARGS,
                argument_index,
                argument,
                "unable to store forwarded argument");
    }

    cli->forward_args[
        cli->forward_arg_count] =
        argument;

    ++cli->forward_arg_count;

    return true;
}

/* ========================================================================= */
/* Value parsers                                                             */
/* ========================================================================= */

static bool
vitte_cli_parse_backend_value(
    const char *value,
    vitte_cli_backend_t *backend)
{
    if (value == NULL ||
        backend == NULL) {
        return false;
    }

    if (vitte_cli_string_equal(
            value,
            "default")) {
        *backend =
            VITTE_CLI_BACKEND_DEFAULT;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "c17") ||
        vitte_cli_string_equal(
            value,
            "c")) {
        *backend =
            VITTE_CLI_BACKEND_C17;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "native")) {
        *backend =
            VITTE_CLI_BACKEND_NATIVE;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "bytecode")) {
        *backend =
            VITTE_CLI_BACKEND_BYTECODE;
        return true;
    }

    return false;
}

static bool
vitte_cli_parse_optimization_value(
    const char *value,
    vitte_cli_optimization_t *optimization)
{
    if (value == NULL ||
        optimization == NULL) {
        return false;
    }

    if (vitte_cli_string_equal(
            value,
            "default")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_DEFAULT;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "0") ||
        vitte_cli_string_equal(
            value,
            "none")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_NONE;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "debug")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_DEBUG;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "1") ||
        vitte_cli_string_equal(
            value,
            "balanced")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_BALANCED;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "2") ||
        vitte_cli_string_equal(
            value,
            "speed")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_SPEED;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "s") ||
        vitte_cli_string_equal(
            value,
            "size")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_SIZE;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "3") ||
        vitte_cli_string_equal(
            value,
            "aggressive")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_AGGRESSIVE;
        return true;
    }

    return false;
}

static bool
vitte_cli_parse_diagnostic_value(
    const char *value,
    vitte_cli_diagnostic_format_t *format)
{
    if (value == NULL ||
        format == NULL) {
        return false;
    }

    if (vitte_cli_string_equal(
            value,
            "default")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_DEFAULT;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "terminal")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_TERMINAL;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "short")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_SHORT;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "json")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_JSON;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "sarif")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_SARIF;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "lsp")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_LSP;
        return true;
    }

    return false;
}

static bool
vitte_cli_parse_color_value(
    const char *value,
    vitte_cli_color_mode_t *color)
{
    if (value == NULL ||
        color == NULL) {
        return false;
    }

    if (vitte_cli_string_equal(
            value,
            "auto")) {
        *color =
            VITTE_CLI_COLOR_AUTO;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "always")) {
        *color =
            VITTE_CLI_COLOR_ALWAYS;
        return true;
    }

    if (vitte_cli_string_equal(
            value,
            "never")) {
        *color =
            VITTE_CLI_COLOR_NEVER;
        return true;
    }

    return false;
}

/* ========================================================================= */
/* Command parser                                                            */
/* ========================================================================= */

static bool
vitte_cli_parse_command_value(
    const char *value,
    vitte_cli_command_t *command)
{
    if (value == NULL ||
        command == NULL) {
        return false;
    }

    if (vitte_cli_string_equal(value, "build")) {
        *command = VITTE_CLI_COMMAND_BUILD;
        return true;
    }

    if (vitte_cli_string_equal(value, "run")) {
        *command = VITTE_CLI_COMMAND_RUN;
        return true;
    }

    if (vitte_cli_string_equal(value, "check")) {
        *command = VITTE_CLI_COMMAND_CHECK;
        return true;
    }

    if (vitte_cli_string_equal(value, "test")) {
        *command = VITTE_CLI_COMMAND_TEST;
        return true;
    }

    if (vitte_cli_string_equal(value, "fmt") ||
        vitte_cli_string_equal(value, "format")) {
        *command = VITTE_CLI_COMMAND_FMT;
        return true;
    }

    if (vitte_cli_string_equal(value, "install")) {
        *command = VITTE_CLI_COMMAND_INSTALL;
        return true;
    }

    if (vitte_cli_string_equal(value, "uninstall")) {
        *command = VITTE_CLI_COMMAND_UNINSTALL;
        return true;
    }

    if (vitte_cli_string_equal(value, "update")) {
        *command = VITTE_CLI_COMMAND_UPDATE;
        return true;
    }

    if (vitte_cli_string_equal(value, "clean")) {
        *command = VITTE_CLI_COMMAND_CLEAN;
        return true;
    }

    if (vitte_cli_string_equal(value, "version")) {
        *command = VITTE_CLI_COMMAND_VERSION;
        return true;
    }

    if (vitte_cli_string_equal(value, "help")) {
        *command = VITTE_CLI_COMMAND_HELP;
        return true;
    }

    return false;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_cli_init(
    vitte_cli_t *cli)
{
    if (cli == NULL) {
        return false;
    }

    memset(
        cli,
        0,
        sizeof(*cli));

    cli->magic =
        VITTE_CLI_MAGIC;

    cli->state =
        VITTE_CLI_STATE_READY;

    cli->last_error =
        VITTE_CLI_ERROR_NONE;

    cli->config =
        vitte_cli_config_default();

    cli->max_inputs =
        VITTE_CLI_DEFAULT_MAX_INPUTS;

    cli->max_forward_args =
        VITTE_CLI_DEFAULT_MAX_FORWARD_ARGS;

    return true;
}

void
vitte_cli_destroy(
    vitte_cli_t *cli)
{
    if (cli == NULL ||
        cli->magic !=
            VITTE_CLI_MAGIC) {
        return;
    }

    free((void *)cli->inputs);
    free((void *)cli->forward_args);

    cli->inputs = NULL;
    cli->input_count = 0u;
    cli->input_capacity = 0u;

    cli->forward_args = NULL;
    cli->forward_arg_count = 0u;
    cli->forward_arg_capacity = 0u;

    cli->state =
        VITTE_CLI_STATE_DESTROYED;

    cli->magic =
        VITTE_CLI_DEAD_MAGIC;
}

bool
vitte_cli_reset(
    vitte_cli_t *cli)
{
    if (cli == NULL ||
        cli->magic !=
            VITTE_CLI_MAGIC) {
        return false;
    }

    if (cli->state ==
        VITTE_CLI_STATE_PARSING) {
        return false;
    }

    cli->config =
        vitte_cli_config_default();

    cli->input_count = 0u;
    cli->forward_arg_count = 0u;

    memset(
        &cli->message,
        0,
        sizeof(cli->message));

    memset(
        &cli->stats,
        0,
        sizeof(cli->stats));

    cli->last_error =
        VITTE_CLI_ERROR_NONE;

    cli->state =
        VITTE_CLI_STATE_READY;

    return true;
}

/* ========================================================================= */
/* Option helpers                                                            */
/* ========================================================================= */

static bool
vitte_cli_mark_seen(
    vitte_cli_t *cli,
    uint64_t *seen,
    uint64_t flag,
    int argument_index,
    const char *argument)
{
    if ((*seen & flag) != 0u) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_DUPLICATE_OPTION,
                argument_index,
                argument,
                "option specified more than once");
    }

    *seen |= flag;

    return true;
}

static const char *
vitte_cli_require_value(
    vitte_cli_t *cli,
    int argc,
    const char *const argv[],
    int *index,
    const char *argument,
    const char *inline_value)
{
    if (inline_value != NULL) {
        if (inline_value[0] == '\0') {
            (void)vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_MISSING_OPTION_VALUE,
                *index,
                argument,
                "option value is empty");

            return NULL;
        }

        return inline_value;
    }

    if (*index + 1 >= argc) {
        (void)vitte_cli_fail(
            cli,
            VITTE_CLI_ERROR_MISSING_OPTION_VALUE,
            *index,
            argument,
            "option requires a value");

        return NULL;
    }

    ++(*index);

    if (argv[*index] == NULL ||
        argv[*index][0] == '\0') {
        (void)vitte_cli_fail(
            cli,
            VITTE_CLI_ERROR_MISSING_OPTION_VALUE,
            *index,
            argv[*index],
            "option value is empty");

        return NULL;
    }

    return argv[*index];
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_cli_fingerprint(
    const vitte_cli_t *cli)
{
    uint64_t hash;
    size_t index;

    if (!vitte_cli_is_valid(cli)) {
        return 0u;
    }

    hash = VITTE_CLI_FNV_OFFSET;

    hash =
        vitte_cli_hash_u64(
            hash,
            (uint64_t)cli->config.command);

    hash =
        vitte_cli_hash_u64(
            hash,
            (uint64_t)cli->config.backend);

    hash =
        vitte_cli_hash_u64(
            hash,
            (uint64_t)cli->config.optimization);

    hash =
        vitte_cli_hash_u64(
            hash,
            (uint64_t)cli->config.diagnostic_format);

    hash =
        vitte_cli_hash_u64(
            hash,
            (uint64_t)cli->config.color);

    hash =
        vitte_cli_hash_string(
            hash,
            cli->config.target);

    hash =
        vitte_cli_hash_string(
            hash,
            cli->config.output);

    hash =
        vitte_cli_hash_string(
            hash,
            cli->config.manifest_path);

    hash =
        vitte_cli_hash_string(
            hash,
            cli->config.package);

    hash =
        vitte_cli_hash_string(
            hash,
            cli->config.test_filter);

    hash =
        vitte_cli_hash_u64(
            hash,
            (uint64_t)cli->config.jobs);

    hash =
        vitte_cli_hash_u64(
            hash,
            (uint64_t)cli->config.max_errors);

#define VITTE_CLI_HASH_BOOL(field)                                      \
    do {                                                                \
        hash = vitte_cli_hash_u64(                                      \
            hash,                                                       \
            cli->config.field ? UINT64_C(1) : UINT64_C(0));             \
    } while (0)

    VITTE_CLI_HASH_BOOL(verbose);
    VITTE_CLI_HASH_BOOL(quiet);
    VITTE_CLI_HASH_BOOL(release);
    VITTE_CLI_HASH_BOOL(debug);
    VITTE_CLI_HASH_BOOL(emit_debug_info);
    VITTE_CLI_HASH_BOOL(emit_source_map);
    VITTE_CLI_HASH_BOOL(emit_comments);
    VITTE_CLI_HASH_BOOL(runtime_checks);
    VITTE_CLI_HASH_BOOL(bounds_checks);
    VITTE_CLI_HASH_BOOL(null_checks);
    VITTE_CLI_HASH_BOOL(overflow_checks);
    VITTE_CLI_HASH_BOOL(warnings_as_errors);
    VITTE_CLI_HASH_BOOL(deterministic);
    VITTE_CLI_HASH_BOOL(incremental);
    VITTE_CLI_HASH_BOOL(offline);
    VITTE_CLI_HASH_BOOL(locked);
    VITTE_CLI_HASH_BOOL(frozen);
    VITTE_CLI_HASH_BOOL(force);
    VITTE_CLI_HASH_BOOL(check_format);
    VITTE_CLI_HASH_BOOL(write_format);

#undef VITTE_CLI_HASH_BOOL

    for (index = 0u;
         index < cli->input_count;
         ++index) {
        hash =
            vitte_cli_hash_string(
                hash,
                cli->inputs[index]);
    }

    for (index = 0u;
         index < cli->forward_arg_count;
         ++index) {
        hash =
            vitte_cli_hash_string(
                hash,
                cli->forward_args[index]);
    }

    return hash;
}

/* ========================================================================= */
/* Final validation                                                          */
/* ========================================================================= */

static bool
vitte_cli_command_requires_input(
    vitte_cli_command_t command)
{
    switch (command) {
        case VITTE_CLI_COMMAND_BUILD:
        case VITTE_CLI_COMMAND_RUN:
        case VITTE_CLI_COMMAND_CHECK:
            return true;

        case VITTE_CLI_COMMAND_NONE:
        case VITTE_CLI_COMMAND_TEST:
        case VITTE_CLI_COMMAND_FMT:
        case VITTE_CLI_COMMAND_INSTALL:
        case VITTE_CLI_COMMAND_UNINSTALL:
        case VITTE_CLI_COMMAND_UPDATE:
        case VITTE_CLI_COMMAND_CLEAN:
        case VITTE_CLI_COMMAND_VERSION:
        case VITTE_CLI_COMMAND_HELP:
        case VITTE_CLI_COMMAND_COUNT:
            return false;
    }

    return false;
}

static bool
vitte_cli_finalize(
    vitte_cli_t *cli)
{
    if (cli->config.command ==
        VITTE_CLI_COMMAND_NONE) {
        cli->config.command =
            VITTE_CLI_COMMAND_HELP;

        cli->config.show_help = true;
    }

    if (cli->config.release &&
        cli->config.debug) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                -1,
                NULL,
                "--release and --debug cannot be used together");
    }

    if (cli->config.verbose &&
        cli->config.quiet) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                -1,
                NULL,
                "--verbose and --quiet cannot be used together");
    }

    if (cli->config.check_format &&
        cli->config.write_format) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                -1,
                NULL,
                "--check and --write formatting modes conflict");
    }

    if (vitte_cli_command_requires_input(
            cli->config.command) &&
        cli->input_count == 0u) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_MISSING_INPUT,
                -1,
                NULL,
                "command requires at least one input");
    }

    if (cli->forward_arg_count != 0u &&
        cli->config.command !=
            VITTE_CLI_COMMAND_RUN &&
        cli->config.command !=
            VITTE_CLI_COMMAND_TEST) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                -1,
                NULL,
                "forwarded arguments are only valid for run or test");
    }

    if (cli->config.command ==
            VITTE_CLI_COMMAND_FMT &&
        cli->input_count == 0u) {
        /*
         * No input is allowed here because the driver may interpret this as
         * formatting the current project.
         */
    }

    if (!vitte_cli_config_validate(
            &cli->config)) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_INVALID_ARGUMENT,
                -1,
                NULL,
                "final CLI configuration is invalid");
    }

    cli->stats.input_count =
        cli->input_count;

    cli->stats.forward_argument_count =
        cli->forward_arg_count;

    cli->state =
        VITTE_CLI_STATE_PARSED;

    cli->last_error =
        VITTE_CLI_ERROR_NONE;

    cli->stats.fingerprint =
        vitte_cli_fingerprint(cli);

    return true;
}

/* ========================================================================= */
/* Parse                                                                     */
/* ========================================================================= */

bool
vitte_cli_parse(
    vitte_cli_t *cli,
    int argc,
    const char *const argv[])
{
    int index;
    bool command_seen;
    bool forward_mode;
    uint64_t seen;

    if (!vitte_cli_is_valid(cli) ||
        argc < 0 ||
        (argc > 0 && argv == NULL)) {
        return false;
    }

    if (cli->state !=
        VITTE_CLI_STATE_READY) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_INVALID_STATE,
                -1,
                NULL,
                "CLI parser is not in ready state");
    }

    cli->state =
        VITTE_CLI_STATE_PARSING;

    cli->stats.argument_count =
        argc > 0
            ? (size_t)argc
            : 0u;

    command_seen = false;
    forward_mode = false;
    seen = UINT64_C(0);

    /*
     * argv[0] is the executable name when argc > 0.
     */
    for (index = argc > 0 ? 1 : 0;
         index < argc;
         ++index) {
        const char *argument;
        const char *value;

        argument = argv[index];

        if (argument == NULL) {
            return
                vitte_cli_fail(
                    cli,
                    VITTE_CLI_ERROR_INVALID_ARGUMENT,
                    index,
                    NULL,
                    "NULL command-line argument");
        }

        if (forward_mode) {
            if (!vitte_cli_add_forward_arg(
                    cli,
                    argument,
                    index)) {
                return false;
            }

            continue;
        }

        if (vitte_cli_string_equal(
                argument,
                "--")) {
            if (cli->config.command ==
                    VITTE_CLI_COMMAND_RUN ||
                cli->config.command ==
                    VITTE_CLI_COMMAND_TEST) {
                forward_mode = true;
            } else {
                /*
                 * For non-executing commands, -- terminates option parsing.
                 * Remaining arguments become inputs.
                 */
                ++index;

                for (;
                     index < argc;
                     ++index) {
                    if (!vitte_cli_add_input(
                            cli,
                            argv[index],
                            index)) {
                        return false;
                    }
                }

                break;
            }

            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Help/version                                                       */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "-h") ||
            vitte_cli_string_equal(argument, "--help")) {
            cli->config.show_help = true;

            if (!command_seen) {
                cli->config.command =
                    VITTE_CLI_COMMAND_HELP;

                command_seen = true;
            }

            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "-V") ||
            vitte_cli_string_equal(argument, "--version")) {
            cli->config.show_version = true;

            if (!command_seen) {
                cli->config.command =
                    VITTE_CLI_COMMAND_VERSION;

                command_seen = true;
            }

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Command                                                            */
        /* ----------------------------------------------------------------- */

        if (!command_seen &&
            !vitte_cli_is_option(argument)) {
            vitte_cli_command_t command;

            if (!vitte_cli_parse_command_value(
                    argument,
                    &command)) {
                /*
                 * Compatibility/convenience mode:
                 *
                 *     vitte main.vit
                 *
                 * behaves like:
                 *
                 *     vitte build main.vit
                 */
                cli->config.command =
                    VITTE_CLI_COMMAND_BUILD;

                command_seen = true;

                if (!vitte_cli_add_input(
                        cli,
                        argument,
                        index)) {
                    return false;
                }

                continue;
            }

            cli->config.command = command;
            command_seen = true;

            if (command ==
                VITTE_CLI_COMMAND_HELP) {
                cli->config.show_help = true;
            }

            if (command ==
                VITTE_CLI_COMMAND_VERSION) {
                cli->config.show_version = true;
            }

            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Verbosity                                                          */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "-v") ||
            vitte_cli_string_equal(argument, "--verbose")) {
            cli->config.verbose = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "-q") ||
            vitte_cli_string_equal(argument, "--quiet")) {
            cli->config.quiet = true;
            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Build profile                                                      */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--release")) {
            cli->config.release = true;

            if (cli->config.optimization ==
                VITTE_CLI_OPTIMIZATION_DEFAULT) {
                cli->config.optimization =
                    VITTE_CLI_OPTIMIZATION_SPEED;
            }

            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--debug")) {
            cli->config.debug = true;

            if (cli->config.optimization ==
                VITTE_CLI_OPTIMIZATION_DEFAULT) {
                cli->config.optimization =
                    VITTE_CLI_OPTIMIZATION_DEBUG;
            }

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Backend                                                            */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--backend");

        if (vitte_cli_string_equal(argument, "--backend") ||
            value != NULL) {
            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_BACKEND,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            if (!vitte_cli_parse_backend_value(
                    value,
                    &cli->config.backend)) {
                return
                    vitte_cli_fail(
                        cli,
                        VITTE_CLI_ERROR_INVALID_BACKEND,
                        index,
                        value,
                        "unknown code-generation backend");
            }

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Optimization                                                       */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--opt");

        if (vitte_cli_string_equal(argument, "--opt") ||
            value != NULL) {
            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_OPTIMIZATION,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            if (!vitte_cli_parse_optimization_value(
                    value,
                    &cli->config.optimization)) {
                return
                    vitte_cli_fail(
                        cli,
                        VITTE_CLI_ERROR_INVALID_OPTIMIZATION,
                        index,
                        value,
                        "unknown optimization level");
            }

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Target                                                             */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--target");

        if (vitte_cli_string_equal(argument, "--target") ||
            value != NULL) {
            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_TARGET,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            cli->config.target = value;

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Output                                                             */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--output");

        if (vitte_cli_string_equal(argument, "-o") ||
            vitte_cli_string_equal(argument, "--output") ||
            value != NULL) {
            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_OUTPUT,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            cli->config.output = value;

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Manifest                                                           */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--manifest-path");

        if (vitte_cli_string_equal(argument, "--manifest-path") ||
            value != NULL) {
            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_MANIFEST,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            cli->config.manifest_path = value;

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Jobs                                                               */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--jobs");

        if (vitte_cli_string_equal(argument, "-j") ||
            vitte_cli_string_equal(argument, "--jobs") ||
            value != NULL) {
            size_t jobs;

            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_JOBS,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            if (!vitte_cli_parse_size(
                    value,
                    &jobs) ||
                jobs == 0u) {
                return
                    vitte_cli_fail(
                        cli,
                        VITTE_CLI_ERROR_INVALID_OPTION_VALUE,
                        index,
                        value,
                        "jobs must be a positive integer");
            }

            cli->config.jobs = jobs;

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Max errors                                                         */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--max-errors");

        if (vitte_cli_string_equal(argument, "--max-errors") ||
            value != NULL) {
            size_t maximum;

            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_MAX_ERRORS,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            if (!vitte_cli_parse_size(
                    value,
                    &maximum) ||
                maximum == 0u) {
                return
                    vitte_cli_fail(
                        cli,
                        VITTE_CLI_ERROR_INVALID_OPTION_VALUE,
                        index,
                        value,
                        "max-errors must be a positive integer");
            }

            cli->config.max_errors = maximum;

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Diagnostics                                                        */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--diagnostics");

        if (vitte_cli_string_equal(argument, "--diagnostics") ||
            value != NULL) {
            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_DIAGNOSTICS,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            if (!vitte_cli_parse_diagnostic_value(
                    value,
                    &cli->config.diagnostic_format)) {
                return
                    vitte_cli_fail(
                        cli,
                        VITTE_CLI_ERROR_INVALID_DIAGNOSTIC_FORMAT,
                        index,
                        value,
                        "unknown diagnostic output format");
            }

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Color                                                              */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--color");

        if (vitte_cli_string_equal(argument, "--color") ||
            value != NULL) {
            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_COLOR,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            if (!vitte_cli_parse_color_value(
                    value,
                    &cli->config.color)) {
                return
                    vitte_cli_fail(
                        cli,
                        VITTE_CLI_ERROR_INVALID_COLOR_MODE,
                        index,
                        value,
                        "color must be auto, always or never");
            }

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Package                                                            */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--package");

        if (vitte_cli_string_equal(argument, "-p") ||
            vitte_cli_string_equal(argument, "--package") ||
            value != NULL) {
            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_PACKAGE,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            cli->config.package = value;

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Test filter                                                        */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--filter");

        if (vitte_cli_string_equal(argument, "--filter") ||
            value != NULL) {
            if (!vitte_cli_mark_seen(
                    cli,
                    &seen,
                    VITTE_CLI_SEEN_TEST_FILTER,
                    index,
                    argument)) {
                return false;
            }

            value =
                vitte_cli_require_value(
                    cli,
                    argc,
                    argv,
                    &index,
                    argument,
                    value);

            if (value == NULL) {
                return false;
            }

            cli->config.test_filter = value;

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Diagnostics/safety toggles                                         */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--debug-info")) {
            cli->config.emit_debug_info = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--source-map")) {
            cli->config.emit_source_map = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-source-map")) {
            cli->config.emit_source_map = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--emit-comments")) {
            cli->config.emit_comments = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-runtime-checks")) {
            cli->config.runtime_checks = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-bounds-checks")) {
            cli->config.bounds_checks = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-null-checks")) {
            cli->config.null_checks = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-overflow-checks")) {
            cli->config.overflow_checks = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "-Werror") ||
            vitte_cli_string_equal(argument, "--warnings-as-errors")) {
            cli->config.warnings_as_errors = true;
            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Reproducibility                                                    */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--deterministic")) {
            cli->config.deterministic = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-deterministic")) {
            cli->config.deterministic = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--incremental")) {
            cli->config.incremental = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-incremental")) {
            cli->config.incremental = false;
            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Package/network policy                                             */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--offline")) {
            cli->config.offline = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--locked")) {
            cli->config.locked = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--frozen")) {
            cli->config.frozen = true;
            cli->config.locked = true;
            cli->config.offline = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--force")) {
            cli->config.force = true;
            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Formatter                                                          */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--check")) {
            cli->config.check_format = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--write")) {
            cli->config.write_format = true;
            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Unknown option                                                     */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_is_option(argument)) {
            return
                vitte_cli_fail(
                    cli,
                    VITTE_CLI_ERROR_UNKNOWN_OPTION,
                    index,
                    argument,
                    "unknown command-line option");
        }

        /* ----------------------------------------------------------------- */
        /* Positional                                                         */
        /* ----------------------------------------------------------------- */

        if (!vitte_cli_add_input(
                cli,
                argument,
                index)) {
            return false;
        }
    }

    return vitte_cli_finalize(cli);
}

/* ========================================================================= */
/* Accessors                                                                 */
/* ========================================================================= */

const vitte_cli_config_t *
vitte_cli_config(
    const vitte_cli_t *cli)
{
    if (!vitte_cli_is_valid(cli)) {
        return NULL;
    }

    return &cli->config;
}

vitte_cli_error_t
vitte_cli_last_error(
    const vitte_cli_t *cli)
{
    if (cli == NULL ||
        cli->magic != VITTE_CLI_MAGIC) {
        return
            VITTE_CLI_ERROR_INVALID_CLI;
    }

    return cli->last_error;
}

vitte_cli_state_t
vitte_cli_state(
    const vitte_cli_t *cli)
{
    if (cli == NULL ||
        cli->magic != VITTE_CLI_MAGIC) {
        return
            VITTE_CLI_STATE_INVALID;
    }

    return cli->state;
}

const vitte_cli_message_t *
vitte_cli_message(
    const vitte_cli_t *cli)
{
    if (cli == NULL ||
        cli->magic != VITTE_CLI_MAGIC) {
        return NULL;
    }

    return &cli->message;
}

vitte_cli_stats_t
vitte_cli_stats(
    const vitte_cli_t *cli)
{
    vitte_cli_stats_t stats;

    memset(
        &stats,
        0,
        sizeof(stats));

    if (cli == NULL ||
        cli->magic != VITTE_CLI_MAGIC) {
        return stats;
    }

    return cli->stats;
}

/* ========================================================================= */
/* Inputs                                                                    */
/* ========================================================================= */

size_t
vitte_cli_input_count(
    const vitte_cli_t *cli)
{
    if (!vitte_cli_is_valid(cli)) {
        return 0u;
    }

    return cli->input_count;
}

const char *
vitte_cli_input_at(
    const vitte_cli_t *cli,
    size_t index)
{
    if (!vitte_cli_is_valid(cli) ||
        index >= cli->input_count) {
        return NULL;
    }

    return cli->inputs[index];
}

/* ========================================================================= */
/* Forward arguments                                                         */
/* ========================================================================= */

size_t
vitte_cli_forward_arg_count(
    const vitte_cli_t *cli)
{
    if (!vitte_cli_is_valid(cli)) {
        return 0u;
    }

    return cli->forward_arg_count;
}

const char *
vitte_cli_forward_arg_at(
    const vitte_cli_t *cli,
    size_t index)
{
    if (!vitte_cli_is_valid(cli) ||
        index >= cli->forward_arg_count) {
        return NULL;
    }

    return cli->forward_args[index];
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_cli_set_limits(
    vitte_cli_t *cli,
    size_t max_inputs,
    size_t max_forward_args)
{
    if (!vitte_cli_is_valid(cli) ||
        cli->state != VITTE_CLI_STATE_READY ||
        max_inputs == 0u ||
        max_forward_args == 0u) {
        return false;
    }

    if (max_inputs < cli->input_count ||
        max_forward_args <
            cli->forward_arg_count) {
        return false;
    }

    cli->max_inputs = max_inputs;
    cli->max_forward_args =
        max_forward_args;

    return true;
}

/* ========================================================================= */
/* Command properties                                                        */
/* ========================================================================= */

bool
vitte_cli_command_is_compilation(
    vitte_cli_command_t command)
{
    switch (command) {
        case VITTE_CLI_COMMAND_BUILD:
        case VITTE_CLI_COMMAND_RUN:
        case VITTE_CLI_COMMAND_CHECK:
        case VITTE_CLI_COMMAND_TEST:
            return true;

        case VITTE_CLI_COMMAND_NONE:
        case VITTE_CLI_COMMAND_FMT:
        case VITTE_CLI_COMMAND_INSTALL:
        case VITTE_CLI_COMMAND_UNINSTALL:
        case VITTE_CLI_COMMAND_UPDATE:
        case VITTE_CLI_COMMAND_CLEAN:
        case VITTE_CLI_COMMAND_VERSION:
        case VITTE_CLI_COMMAND_HELP:
        case VITTE_CLI_COMMAND_COUNT:
            return false;
    }

    return false;
}

bool
vitte_cli_command_uses_package_manager(
    vitte_cli_command_t command)
{
    switch (command) {
        case VITTE_CLI_COMMAND_INSTALL:
        case VITTE_CLI_COMMAND_UNINSTALL:
        case VITTE_CLI_COMMAND_UPDATE:
            return true;

        case VITTE_CLI_COMMAND_NONE:
        case VITTE_CLI_COMMAND_BUILD:
        case VITTE_CLI_COMMAND_RUN:
        case VITTE_CLI_COMMAND_CHECK:
        case VITTE_CLI_COMMAND_TEST:
        case VITTE_CLI_COMMAND_FMT:
        case VITTE_CLI_COMMAND_CLEAN:
        case VITTE_CLI_COMMAND_VERSION:
        case VITTE_CLI_COMMAND_HELP:
        case VITTE_CLI_COMMAND_COUNT:
            return false;
    }

    return false;
}

bool
vitte_cli_command_executes_program(
    vitte_cli_command_t command)
{
    return
        command ==
            VITTE_CLI_COMMAND_RUN ||
        command ==
            VITTE_CLI_COMMAND_TEST;
}

/* ========================================================================= */
/* Suggested exit status                                                     */
/* ========================================================================= */

int
vitte_cli_suggested_exit_status(
    const vitte_cli_t *cli)
{
    if (cli == NULL ||
        cli->magic != VITTE_CLI_MAGIC) {
        return 2;
    }

    if (cli->state ==
        VITTE_CLI_STATE_FAILED) {
        return 2;
    }

    return 0;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
