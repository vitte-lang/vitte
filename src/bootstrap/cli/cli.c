/*
 * Vitte Compiler
 * src/bootstrap/cli/cli.c
 *
 * Canonical command-line interface parser.
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

#define VITTE_CLI_FINGERPRINT_VERSION \
    UINT64_C(2)

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

    if (data == NULL) {
        hash ^= UINT64_C(0xff);
        hash *= VITTE_CLI_FNV_PRIME;
        return hash;
    }

    bytes = (const unsigned char *)data;

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
/* Strings                                                                   */
/* ========================================================================= */

static bool
vitte_cli_string_equal(
    const char *left,
    const char *right)
{
    return
        left != NULL &&
        right != NULL &&
        strcmp(left, right) == 0;
}

static bool
vitte_cli_string_has_suffix(
    const char *text,
    const char *suffix)
{
    size_t text_length;
    size_t suffix_length;

    if (text == NULL ||
        suffix == NULL) {
        return false;
    }

    text_length = strlen(text);
    suffix_length = strlen(suffix);

    if (suffix_length > text_length) {
        return false;
    }

    return
        memcmp(
            text + text_length - suffix_length,
            suffix,
            suffix_length) == 0;
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
            length) != 0 ||
        argument[length] != '=') {
        return NULL;
    }

    return argument + length + 1u;
}

static bool
vitte_cli_looks_like_path(
    const char *text)
{
    if (text == NULL ||
        text[0] == '\0') {
        return false;
    }

    if (strchr(text, '/') != NULL ||
        strchr(text, '\\') != NULL) {
        return true;
    }

    if (text[0] == '.' ||
        text[0] == '~') {
        return true;
    }

    if (vitte_cli_string_has_suffix(text, ".vit") ||
        vitte_cli_string_has_suffix(text, ".vitl") ||
        vitte_cli_string_has_suffix(text, ".vitte")) {
        return true;
    }

    return false;
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
/* Enum validation                                                           */
/* ========================================================================= */

static bool
vitte_cli_error_is_valid(
    vitte_cli_error_t value)
{
    return
        (unsigned)value <
        (unsigned)VITTE_CLI_ERROR_COUNT;
}

static bool
vitte_cli_state_is_live(
    vitte_cli_state_t value)
{
    return
        value == VITTE_CLI_STATE_READY ||
        value == VITTE_CLI_STATE_PARSING ||
        value == VITTE_CLI_STATE_PARSED ||
        value == VITTE_CLI_STATE_FAILED;
}

static bool
vitte_cli_command_is_valid(
    vitte_cli_command_t value)
{
    return
        (unsigned)value <
        (unsigned)VITTE_CLI_COMMAND_COUNT;
}

static bool
vitte_cli_backend_is_valid(
    vitte_cli_backend_t value)
{
    return
        (unsigned)value <
        (unsigned)VITTE_CLI_BACKEND_COUNT;
}

static bool
vitte_cli_optimization_is_valid(
    vitte_cli_optimization_t value)
{
    return
        (unsigned)value <
        (unsigned)VITTE_CLI_OPTIMIZATION_COUNT;
}

static bool
vitte_cli_diagnostic_format_is_valid(
    vitte_cli_diagnostic_format_t value)
{
    return
        (unsigned)value <
        (unsigned)VITTE_CLI_DIAGNOSTIC_FORMAT_COUNT;
}

static bool
vitte_cli_color_is_valid(
    vitte_cli_color_mode_t value)
{
    return
        (unsigned)value <
        (unsigned)VITTE_CLI_COLOR_COUNT;
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
        case VITTE_CLI_ERROR_OPTION_NOT_ALLOWED:
            return "option-not-allowed";
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
            break;
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
            break;
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
            break;
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
            break;
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
            break;
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
            break;
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
            break;
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
/* Structural validation                                                     */
/* ========================================================================= */

bool
vitte_cli_config_validate(
    const vitte_cli_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (!vitte_cli_command_is_valid(
            config->command) ||
        !vitte_cli_backend_is_valid(
            config->backend) ||
        !vitte_cli_optimization_is_valid(
            config->optimization) ||
        !vitte_cli_diagnostic_format_is_valid(
            config->diagnostic_format) ||
        !vitte_cli_color_is_valid(
            config->color)) {
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

    if (config->frozen &&
        (!config->locked ||
         !config->offline)) {
        return false;
    }

    return true;
}

bool
vitte_cli_is_valid(
    const vitte_cli_t *cli)
{
    if (cli == NULL ||
        cli->magic != VITTE_CLI_MAGIC ||
        !vitte_cli_state_is_live(cli->state) ||
        !vitte_cli_error_is_valid(cli->last_error)) {
        return false;
    }

    if (cli->max_inputs == 0u ||
        cli->max_forward_args == 0u) {
        return false;
    }

    if (cli->input_count >
            cli->input_capacity ||
        cli->input_count >
            cli->max_inputs) {
        return false;
    }

    if (cli->forward_arg_count >
            cli->forward_arg_capacity ||
        cli->forward_arg_count >
            cli->max_forward_args) {
        return false;
    }

    if (cli->input_capacity >
            cli->max_inputs ||
        cli->forward_arg_capacity >
            cli->max_forward_args) {
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

    if ((cli->state == VITTE_CLI_STATE_READY ||
         cli->state == VITTE_CLI_STATE_PARSED) &&
        !vitte_cli_config_validate(
            &cli->config)) {
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
    cli->message.argument_index =
        argument_index;
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

    if (cli == NULL ||
        array == NULL ||
        capacity == NULL) {
        return false;
    }

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

        if (next_capacity > maximum) {
            next_capacity = maximum;
        }
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

        if (doubled > maximum) {
            next_capacity = maximum;
        } else {
            next_capacity = doubled;
        }

        if (next_capacity < required &&
            next_capacity == maximum) {
            return false;
        }
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
        realloc(
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
/* Input storage                                                             */
/* ========================================================================= */

static bool
vitte_cli_add_input(
    vitte_cli_t *cli,
    const char *input,
    int argument_index)
{
    size_t required;

    if (cli == NULL ||
        input == NULL ||
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

    cli->inputs[cli->input_count++] =
        input;

    ++cli->stats.positional_count;

    return true;
}

static bool
vitte_cli_add_forward_arg(
    vitte_cli_t *cli,
    const char *argument,
    int argument_index)
{
    size_t required;

    if (cli == NULL ||
        argument == NULL) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_INVALID_ARGUMENT,
                argument_index,
                argument,
                "invalid forwarded argument");
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
        cli->forward_arg_count++] =
        argument;

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

    if (vitte_cli_string_equal(value, "default")) {
        *backend = VITTE_CLI_BACKEND_DEFAULT;
        return true;
    }

    if (vitte_cli_string_equal(value, "c17") ||
        vitte_cli_string_equal(value, "c")) {
        *backend = VITTE_CLI_BACKEND_C17;
        return true;
    }

    if (vitte_cli_string_equal(value, "native")) {
        *backend = VITTE_CLI_BACKEND_NATIVE;
        return true;
    }

    if (vitte_cli_string_equal(value, "bytecode")) {
        *backend = VITTE_CLI_BACKEND_BYTECODE;
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

    if (vitte_cli_string_equal(value, "default")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_DEFAULT;
        return true;
    }

    if (vitte_cli_string_equal(value, "0") ||
        vitte_cli_string_equal(value, "none")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_NONE;
        return true;
    }

    if (vitte_cli_string_equal(value, "debug")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_DEBUG;
        return true;
    }

    if (vitte_cli_string_equal(value, "1") ||
        vitte_cli_string_equal(value, "balanced")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_BALANCED;
        return true;
    }

    if (vitte_cli_string_equal(value, "2") ||
        vitte_cli_string_equal(value, "speed")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_SPEED;
        return true;
    }

    if (vitte_cli_string_equal(value, "s") ||
        vitte_cli_string_equal(value, "size")) {
        *optimization =
            VITTE_CLI_OPTIMIZATION_SIZE;
        return true;
    }

    if (vitte_cli_string_equal(value, "3") ||
        vitte_cli_string_equal(value, "aggressive")) {
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

    if (vitte_cli_string_equal(value, "default")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_DEFAULT;
        return true;
    }

    if (vitte_cli_string_equal(value, "terminal")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_TERMINAL;
        return true;
    }

    if (vitte_cli_string_equal(value, "short")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_SHORT;
        return true;
    }

    if (vitte_cli_string_equal(value, "json")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_JSON;
        return true;
    }

    if (vitte_cli_string_equal(value, "sarif")) {
        *format =
            VITTE_CLI_DIAGNOSTIC_FORMAT_SARIF;
        return true;
    }

    if (vitte_cli_string_equal(value, "lsp")) {
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

    if (vitte_cli_string_equal(value, "auto")) {
        *color = VITTE_CLI_COLOR_AUTO;
        return true;
    }

    if (vitte_cli_string_equal(value, "always")) {
        *color = VITTE_CLI_COLOR_ALWAYS;
        return true;
    }

    if (vitte_cli_string_equal(value, "never")) {
        *color = VITTE_CLI_COLOR_NEVER;
        return true;
    }

    return false;
}

static bool
vitte_cli_parse_command_value(
    const char *value,
    vitte_cli_command_t *command)
{
    if (value == NULL ||
        command == NULL) {
        return false;
    }

#define VITTE_CLI_COMMAND_CASE(text_, value_) \
    if (vitte_cli_string_equal(value, text_)) { \
        *command = value_; \
        return true; \
    }

    VITTE_CLI_COMMAND_CASE(
        "build",
        VITTE_CLI_COMMAND_BUILD)

    VITTE_CLI_COMMAND_CASE(
        "run",
        VITTE_CLI_COMMAND_RUN)

    VITTE_CLI_COMMAND_CASE(
        "check",
        VITTE_CLI_COMMAND_CHECK)

    VITTE_CLI_COMMAND_CASE(
        "test",
        VITTE_CLI_COMMAND_TEST)

    if (vitte_cli_string_equal(value, "fmt") ||
        vitte_cli_string_equal(value, "format")) {
        *command =
            VITTE_CLI_COMMAND_FMT;
        return true;
    }

    VITTE_CLI_COMMAND_CASE(
        "install",
        VITTE_CLI_COMMAND_INSTALL)

    VITTE_CLI_COMMAND_CASE(
        "uninstall",
        VITTE_CLI_COMMAND_UNINSTALL)

    VITTE_CLI_COMMAND_CASE(
        "update",
        VITTE_CLI_COMMAND_UPDATE)

    VITTE_CLI_COMMAND_CASE(
        "clean",
        VITTE_CLI_COMMAND_CLEAN)

    VITTE_CLI_COMMAND_CASE(
        "version",
        VITTE_CLI_COMMAND_VERSION)

    VITTE_CLI_COMMAND_CASE(
        "help",
        VITTE_CLI_COMMAND_HELP)

#undef VITTE_CLI_COMMAND_CASE

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

    memset(
        cli,
        0,
        sizeof(*cli));

    cli->magic =
        VITTE_CLI_DEAD_MAGIC;

    cli->state =
        VITTE_CLI_STATE_DESTROYED;
}

bool
vitte_cli_reset(
    vitte_cli_t *cli)
{
    size_t max_inputs;
    size_t max_forward_args;
    const char **inputs;
    size_t input_capacity;
    const char **forward_args;
    size_t forward_arg_capacity;

    if (cli == NULL ||
        cli->magic !=
            VITTE_CLI_MAGIC ||
        cli->state ==
            VITTE_CLI_STATE_PARSING) {
        return false;
    }

    max_inputs =
        cli->max_inputs;

    max_forward_args =
        cli->max_forward_args;

    inputs =
        cli->inputs;

    input_capacity =
        cli->input_capacity;

    forward_args =
        cli->forward_args;

    forward_arg_capacity =
        cli->forward_arg_capacity;

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

    cli->inputs = inputs;
    cli->input_capacity =
        input_capacity;

    cli->forward_args =
        forward_args;
    cli->forward_arg_capacity =
        forward_arg_capacity;

    cli->max_inputs =
        max_inputs;

    cli->max_forward_args =
        max_forward_args;

    return true;
}

/* ========================================================================= */
/* Seen helpers                                                              */
/* ========================================================================= */

static bool
vitte_cli_mark_seen(
    vitte_cli_t *cli,
    uint64_t flag,
    int argument_index,
    const char *argument)
{
    if ((cli->seen_options & flag) != 0u) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_DUPLICATE_OPTION,
                argument_index,
                argument,
                "option specified more than once");
    }

    cli->seen_options |= flag;
    return true;
}

static bool
vitte_cli_mark_boolean(
    vitte_cli_t *cli,
    uint64_t flag,
    uint64_t conflict,
    int argument_index,
    const char *argument)
{
    if ((cli->seen_options & flag) != 0u) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_DUPLICATE_OPTION,
                argument_index,
                argument,
                "option specified more than once");
    }

    if ((cli->seen_options & conflict) != 0u) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                argument_index,
                argument,
                "option conflicts with an earlier option");
    }

    cli->seen_options |= flag;
    return true;
}

/* ========================================================================= */
/* Option value                                                              */
/* ========================================================================= */

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
    ++cli->stats.consumed_value_count;

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
/* Command properties                                                        */
/* ========================================================================= */

bool
vitte_cli_command_is_compilation(
    vitte_cli_command_t command)
{
    return
        command == VITTE_CLI_COMMAND_BUILD ||
        command == VITTE_CLI_COMMAND_RUN ||
        command == VITTE_CLI_COMMAND_CHECK ||
        command == VITTE_CLI_COMMAND_TEST;
}

bool
vitte_cli_command_uses_package_manager(
    vitte_cli_command_t command)
{
    return
        command == VITTE_CLI_COMMAND_INSTALL ||
        command == VITTE_CLI_COMMAND_UNINSTALL ||
        command == VITTE_CLI_COMMAND_UPDATE;
}

bool
vitte_cli_command_executes_program(
    vitte_cli_command_t command)
{
    return
        command == VITTE_CLI_COMMAND_RUN ||
        command == VITTE_CLI_COMMAND_TEST;
}

static bool
vitte_cli_command_accepts_project(
    vitte_cli_command_t command)
{
    return
        command == VITTE_CLI_COMMAND_BUILD ||
        command == VITTE_CLI_COMMAND_RUN ||
        command == VITTE_CLI_COMMAND_CHECK ||
        command == VITTE_CLI_COMMAND_TEST ||
        command == VITTE_CLI_COMMAND_FMT ||
        command == VITTE_CLI_COMMAND_CLEAN;
}

static bool
vitte_cli_has_project_target(
    const vitte_cli_t *cli)
{
    return
        cli->input_count != 0u ||
        cli->config.manifest_path != NULL ||
        cli->config.package != NULL;
}

/* ========================================================================= */
/* Command/option validation                                                 */
/* ========================================================================= */

static bool
vitte_cli_validate_command_options(
    vitte_cli_t *cli)
{
    vitte_cli_command_t command;
    uint64_t seen;

    command = cli->config.command;
    seen = cli->seen_options;

    if (!vitte_cli_command_is_compilation(command)) {
        const uint64_t compilation_options =
            VITTE_CLI_SEEN_BACKEND |
            VITTE_CLI_SEEN_OPTIMIZATION |
            VITTE_CLI_SEEN_TARGET |
            VITTE_CLI_SEEN_JOBS |
            VITTE_CLI_SEEN_MAX_ERRORS |
            VITTE_CLI_SEEN_DEBUG_INFO |
            VITTE_CLI_SEEN_SOURCE_MAP |
            VITTE_CLI_SEEN_NO_SOURCE_MAP |
            VITTE_CLI_SEEN_EMIT_COMMENTS |
            VITTE_CLI_SEEN_NO_RUNTIME |
            VITTE_CLI_SEEN_NO_BOUNDS |
            VITTE_CLI_SEEN_NO_NULL |
            VITTE_CLI_SEEN_NO_OVERFLOW |
            VITTE_CLI_SEEN_WERROR |
            VITTE_CLI_SEEN_RELEASE |
            VITTE_CLI_SEEN_DEBUG;

        if ((seen & compilation_options) != 0u) {
            return
                vitte_cli_fail(
                    cli,
                    VITTE_CLI_ERROR_OPTION_NOT_ALLOWED,
                    -1,
                    NULL,
                    "compilation option used with a non-compilation command");
        }
    }

    if ((seen & VITTE_CLI_SEEN_TEST_FILTER) != 0u &&
        command != VITTE_CLI_COMMAND_TEST) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_OPTION_NOT_ALLOWED,
                -1,
                NULL,
                "--filter is only valid for test");
    }

    if ((seen &
         (VITTE_CLI_SEEN_FMT_CHECK |
          VITTE_CLI_SEEN_FMT_WRITE)) != 0u &&
        command != VITTE_CLI_COMMAND_FMT) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_OPTION_NOT_ALLOWED,
                -1,
                NULL,
                "--check and --write are only valid for fmt");
    }

    if ((seen &
         (VITTE_CLI_SEEN_OFFLINE |
          VITTE_CLI_SEEN_LOCKED |
          VITTE_CLI_SEEN_FROZEN |
          VITTE_CLI_SEEN_FORCE)) != 0u &&
        !vitte_cli_command_uses_package_manager(command) &&
        !vitte_cli_command_accepts_project(command)) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_OPTION_NOT_ALLOWED,
                -1,
                NULL,
                "package policy option is not valid for this command");
    }

    if ((seen & VITTE_CLI_SEEN_OUTPUT) != 0u &&
        command != VITTE_CLI_COMMAND_BUILD &&
        command != VITTE_CLI_COMMAND_RUN) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_OPTION_NOT_ALLOWED,
                -1,
                NULL,
                "--output is only valid for build or run");
    }

    if ((seen & VITTE_CLI_SEEN_MANIFEST) != 0u &&
        !vitte_cli_command_accepts_project(command) &&
        !vitte_cli_command_uses_package_manager(command)) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_OPTION_NOT_ALLOWED,
                -1,
                NULL,
                "--manifest-path is not valid for this command");
    }

    return true;
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
        return UINT64_C(0);
    }

    hash =
        VITTE_CLI_FNV_OFFSET;

    hash =
        vitte_cli_hash_u64(
            hash,
            VITTE_CLI_FINGERPRINT_VERSION);

#define VITTE_CLI_HASH_U64(value_) \
    do { \
        hash = vitte_cli_hash_u64( \
            hash, \
            (uint64_t)(value_)); \
    } while (0)

#define VITTE_CLI_HASH_BOOL(field_) \
    VITTE_CLI_HASH_U64( \
        cli->config.field_ \
            ? UINT64_C(1) \
            : UINT64_C(0))

    VITTE_CLI_HASH_U64(
        cli->config.command);

    VITTE_CLI_HASH_U64(
        cli->config.backend);

    VITTE_CLI_HASH_U64(
        cli->config.optimization);

    VITTE_CLI_HASH_U64(
        cli->config.diagnostic_format);

    VITTE_CLI_HASH_U64(
        cli->config.color);

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

    VITTE_CLI_HASH_U64(cli->config.jobs);
    VITTE_CLI_HASH_U64(cli->config.max_errors);

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
    VITTE_CLI_HASH_BOOL(show_help);
    VITTE_CLI_HASH_BOOL(show_version);

    VITTE_CLI_HASH_U64(cli->input_count);

    for (index = 0u;
         index < cli->input_count;
         ++index) {
        hash =
            vitte_cli_hash_string(
                hash,
                cli->inputs[index]);
    }

    VITTE_CLI_HASH_U64(
        cli->forward_arg_count);

    for (index = 0u;
         index < cli->forward_arg_count;
         ++index) {
        hash =
            vitte_cli_hash_string(
                hash,
                cli->forward_args[index]);
    }

#undef VITTE_CLI_HASH_BOOL
#undef VITTE_CLI_HASH_U64

    return hash;
}

/* ========================================================================= */
/* Finalization                                                              */
/* ========================================================================= */

static bool
vitte_cli_finalize(
    vitte_cli_t *cli)
{
    uint64_t seen;

    if (cli == NULL) {
        return false;
    }

    seen = cli->seen_options;

    if (cli->config.command ==
        VITTE_CLI_COMMAND_NONE) {
        cli->config.command =
            VITTE_CLI_COMMAND_HELP;

        cli->config.show_help = true;
    }

    if ((seen & VITTE_CLI_SEEN_RELEASE) != 0u &&
        (seen & VITTE_CLI_SEEN_DEBUG) != 0u) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                -1,
                NULL,
                "--release and --debug cannot be used together");
    }

    if ((seen & VITTE_CLI_SEEN_VERBOSE) != 0u &&
        (seen & VITTE_CLI_SEEN_QUIET) != 0u) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                -1,
                NULL,
                "--verbose and --quiet cannot be used together");
    }

    if ((seen & VITTE_CLI_SEEN_FMT_CHECK) != 0u &&
        (seen & VITTE_CLI_SEEN_FMT_WRITE) != 0u) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                -1,
                NULL,
                "--check and --write cannot be used together");
    }

    if ((seen & VITTE_CLI_SEEN_RELEASE) != 0u) {
        if ((seen & VITTE_CLI_SEEN_OPTIMIZATION) != 0u &&
            cli->config.optimization ==
                VITTE_CLI_OPTIMIZATION_DEBUG) {
            return
                vitte_cli_fail(
                    cli,
                    VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                    -1,
                    NULL,
                    "--release conflicts with --opt=debug");
        }

        if ((seen & VITTE_CLI_SEEN_OPTIMIZATION) == 0u) {
            cli->config.optimization =
                VITTE_CLI_OPTIMIZATION_SPEED;
        }
    }

    if ((seen & VITTE_CLI_SEEN_DEBUG) != 0u) {
        if ((seen & VITTE_CLI_SEEN_OPTIMIZATION) != 0u &&
            cli->config.optimization !=
                VITTE_CLI_OPTIMIZATION_DEBUG &&
            cli->config.optimization !=
                VITTE_CLI_OPTIMIZATION_NONE) {
            return
                vitte_cli_fail(
                    cli,
                    VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                    -1,
                    NULL,
                    "--debug conflicts with the selected optimization level");
        }

        if ((seen & VITTE_CLI_SEEN_OPTIMIZATION) == 0u) {
            cli->config.optimization =
                VITTE_CLI_OPTIMIZATION_DEBUG;
        }
    }

    if (cli->config.frozen) {
        cli->config.locked = true;
        cli->config.offline = true;
    }

    if (!vitte_cli_validate_command_options(
            cli)) {
        return false;
    }

    switch (cli->config.command) {
        case VITTE_CLI_COMMAND_BUILD:
        case VITTE_CLI_COMMAND_RUN:
        case VITTE_CLI_COMMAND_CHECK:
            if (!vitte_cli_has_project_target(cli) &&
                !cli->config.show_help) {
                return
                    vitte_cli_fail(
                        cli,
                        VITTE_CLI_ERROR_MISSING_INPUT,
                        -1,
                        NULL,
                        "command requires an input, package, or manifest");
            }
            break;

        case VITTE_CLI_COMMAND_UNINSTALL:
            if (cli->input_count == 0u &&
                cli->config.package == NULL &&
                !cli->config.show_help) {
                return
                    vitte_cli_fail(
                        cli,
                        VITTE_CLI_ERROR_MISSING_INPUT,
                        -1,
                        NULL,
                        "uninstall requires at least one package");
            }
            break;

        case VITTE_CLI_COMMAND_VERSION:
        case VITTE_CLI_COMMAND_HELP:
            if (cli->forward_arg_count != 0u) {
                return
                    vitte_cli_fail(
                        cli,
                        VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                        -1,
                        NULL,
                        "help/version do not accept forwarded arguments");
            }
            break;

        case VITTE_CLI_COMMAND_NONE:
        case VITTE_CLI_COMMAND_TEST:
        case VITTE_CLI_COMMAND_FMT:
        case VITTE_CLI_COMMAND_INSTALL:
        case VITTE_CLI_COMMAND_UPDATE:
        case VITTE_CLI_COMMAND_CLEAN:
        case VITTE_CLI_COMMAND_COUNT:
            break;
    }

    if (cli->forward_arg_count != 0u &&
        !vitte_cli_command_executes_program(
            cli->config.command)) {
        return
            vitte_cli_fail(
                cli,
                VITTE_CLI_ERROR_CONFLICTING_OPTIONS,
                -1,
                NULL,
                "forwarded arguments are only valid for run or test");
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

    memset(
        &cli->message,
        0,
        sizeof(cli->message));

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
    bool options_enabled;

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
            ? (size_t)(argc - 1)
            : 0u;

    command_seen = false;
    forward_mode = false;
    options_enabled = true;

    for (index = argc > 0 ? 1 : 0;
         index < argc;
         ++index) {
        const char *argument;
        const char *value;

        argument = argv[index];

        ++cli->stats.parsed_argument_count;

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

        if (options_enabled &&
            vitte_cli_string_equal(
                argument,
                "--")) {
            if (vitte_cli_command_executes_program(
                    cli->config.command)) {
                forward_mode = true;
            } else {
                options_enabled = false;
            }

            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Command                                                            */
        /* ----------------------------------------------------------------- */

        if (!command_seen &&
            !vitte_cli_is_option(argument)) {
            vitte_cli_command_t command;

            if (vitte_cli_parse_command_value(
                    argument,
                    &command)) {
                cli->config.command =
                    command;

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

            if (vitte_cli_looks_like_path(
                    argument)) {
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

            return
                vitte_cli_fail(
                    cli,
                    VITTE_CLI_ERROR_UNKNOWN_COMMAND,
                    index,
                    argument,
                    "unknown command; use an explicit source path for implicit build mode");
        }

        if (!options_enabled) {
            if (!vitte_cli_add_input(
                    cli,
                    argument,
                    index)) {
                return false;
            }

            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Help/version                                                       */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "-h") ||
            vitte_cli_string_equal(argument, "--help")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_HELP,
                    index,
                    argument)) {
                return false;
            }

            cli->config.show_help = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "-V") ||
            vitte_cli_string_equal(argument, "--version")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_VERSION,
                    index,
                    argument)) {
                return false;
            }

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
        /* Verbosity                                                          */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "-v") ||
            vitte_cli_string_equal(argument, "--verbose")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_VERBOSE,
                    VITTE_CLI_SEEN_QUIET,
                    index,
                    argument)) {
                return false;
            }

            cli->config.verbose = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "-q") ||
            vitte_cli_string_equal(argument, "--quiet")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_QUIET,
                    VITTE_CLI_SEEN_VERBOSE,
                    index,
                    argument)) {
                return false;
            }

            cli->config.quiet = true;
            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Profiles                                                           */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--release")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_RELEASE,
                    VITTE_CLI_SEEN_DEBUG,
                    index,
                    argument)) {
                return false;
            }

            cli->config.release = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--debug")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_DEBUG,
                    VITTE_CLI_SEEN_RELEASE,
                    index,
                    argument)) {
                return false;
            }

            cli->config.debug = true;
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

            /*
             * Target syntax is deliberately not hard-coded here.
             * Target semantic validation belongs to the target subsystem.
             */
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
        /* Maximum errors                                                     */
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

            cli->config.max_errors =
                maximum;

            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Diagnostic format                                                  */
        /* ----------------------------------------------------------------- */

        value =
            vitte_cli_inline_value(
                argument,
                "--diagnostics");

        if (vitte_cli_string_equal(argument, "--diagnostics") ||
            value != NULL) {
            if (!vitte_cli_mark_seen(
                    cli,
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
        /* Emission                                                           */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--debug-info")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_DEBUG_INFO,
                    index,
                    argument)) {
                return false;
            }

            cli->config.emit_debug_info = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--source-map")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_SOURCE_MAP,
                    VITTE_CLI_SEEN_NO_SOURCE_MAP,
                    index,
                    argument)) {
                return false;
            }

            cli->config.emit_source_map = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-source-map")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_NO_SOURCE_MAP,
                    VITTE_CLI_SEEN_SOURCE_MAP,
                    index,
                    argument)) {
                return false;
            }

            cli->config.emit_source_map = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--emit-comments")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_EMIT_COMMENTS,
                    index,
                    argument)) {
                return false;
            }

            cli->config.emit_comments = true;
            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Runtime checks                                                     */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--no-runtime-checks")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_NO_RUNTIME,
                    index,
                    argument)) {
                return false;
            }

            cli->config.runtime_checks = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-bounds-checks")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_NO_BOUNDS,
                    index,
                    argument)) {
                return false;
            }

            cli->config.bounds_checks = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-null-checks")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_NO_NULL,
                    index,
                    argument)) {
                return false;
            }

            cli->config.null_checks = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-overflow-checks")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_NO_OVERFLOW,
                    index,
                    argument)) {
                return false;
            }

            cli->config.overflow_checks = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "-Werror") ||
            vitte_cli_string_equal(argument, "--warnings-as-errors")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_WERROR,
                    index,
                    argument)) {
                return false;
            }

            cli->config.warnings_as_errors = true;
            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Reproducibility                                                    */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--deterministic")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_DETERMINISTIC,
                    VITTE_CLI_SEEN_NO_DETERMINISTIC,
                    index,
                    argument)) {
                return false;
            }

            cli->config.deterministic = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-deterministic")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_NO_DETERMINISTIC,
                    VITTE_CLI_SEEN_DETERMINISTIC,
                    index,
                    argument)) {
                return false;
            }

            cli->config.deterministic = false;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--incremental")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_INCREMENTAL,
                    VITTE_CLI_SEEN_NO_INCREMENTAL,
                    index,
                    argument)) {
                return false;
            }

            cli->config.incremental = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--no-incremental")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_NO_INCREMENTAL,
                    VITTE_CLI_SEEN_INCREMENTAL,
                    index,
                    argument)) {
                return false;
            }

            cli->config.incremental = false;
            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Package/network policy                                             */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--offline")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_OFFLINE,
                    index,
                    argument)) {
                return false;
            }

            cli->config.offline = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--locked")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_LOCKED,
                    index,
                    argument)) {
                return false;
            }

            cli->config.locked = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--frozen")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_FROZEN,
                    index,
                    argument)) {
                return false;
            }

            cli->config.frozen = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--force")) {
            if (!vitte_cli_mark_seen(
                    cli,
                    VITTE_CLI_SEEN_FORCE,
                    index,
                    argument)) {
                return false;
            }

            cli->config.force = true;
            ++cli->stats.option_count;
            continue;
        }

        /* ----------------------------------------------------------------- */
        /* Formatter                                                          */
        /* ----------------------------------------------------------------- */

        if (vitte_cli_string_equal(argument, "--check")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_FMT_CHECK,
                    VITTE_CLI_SEEN_FMT_WRITE,
                    index,
                    argument)) {
                return false;
            }

            cli->config.check_format = true;
            ++cli->stats.option_count;
            continue;
        }

        if (vitte_cli_string_equal(argument, "--write")) {
            if (!vitte_cli_mark_boolean(
                    cli,
                    VITTE_CLI_SEEN_FMT_WRITE,
                    VITTE_CLI_SEEN_FMT_CHECK,
                    index,
                    argument)) {
                return false;
            }

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
        cli->magic !=
            VITTE_CLI_MAGIC) {
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
        cli->magic !=
            VITTE_CLI_MAGIC) {
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
        cli->magic !=
            VITTE_CLI_MAGIC) {
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
        cli->magic !=
            VITTE_CLI_MAGIC) {
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
/* Option provenance                                                         */
/* ========================================================================= */

uint64_t
vitte_cli_seen_options(
    const vitte_cli_t *cli)
{
    if (!vitte_cli_is_valid(cli)) {
        return UINT64_C(0);
    }

    return cli->seen_options;
}

bool
vitte_cli_option_was_seen(
    const vitte_cli_t *cli,
    uint64_t flag)
{
    if (!vitte_cli_is_valid(cli) ||
        flag == UINT64_C(0)) {
        return false;
    }

    return
        (cli->seen_options & flag) ==
        flag;
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
        cli->state !=
            VITTE_CLI_STATE_READY ||
        max_inputs == 0u ||
        max_forward_args == 0u) {
        return false;
    }

    if (max_inputs <
            cli->input_count ||
        max_forward_args <
            cli->forward_arg_count) {
        return false;
    }

    /*
     * Existing reserved storage may exceed a newly requested logical limit.
     * Shrinking the allocation is unnecessary; structural validity therefore
     * requires the capacity to fit the new maximum. Refuse the operation
     * rather than silently creating an inconsistent CLI object.
     */
    if (cli->input_capacity >
            max_inputs ||
        cli->forward_arg_capacity >
            max_forward_args) {
        return false;
    }

    cli->max_inputs =
        max_inputs;

    cli->max_forward_args =
        max_forward_args;

    return true;
}

/* ========================================================================= */
/* Suggested exit status                                                     */
/* ========================================================================= */

int
vitte_cli_suggested_exit_status(
    const vitte_cli_t *cli)
{
    if (cli == NULL ||
        cli->magic !=
            VITTE_CLI_MAGIC) {
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