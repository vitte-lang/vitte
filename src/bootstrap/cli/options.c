/*
 * Vitte Compiler
 * src/bootstrap/cli/options.c
 *
 * Private executable-facing implementation module.
 *
 * ISO C17.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api/version.h"
#include "command_spec.h"
#include "filesystem/filesystem.h"
#include "internal.h"

/* Utility                                                                   */
/* ========================================================================= */

static bool
vitte_main_string_equal(
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
vitte_main_option_is(
    const char *argument,
    vitte_cli_option_id_t expected)
{
    vitte_cli_option_id_t actual;

    return vitte_cli_option_id_for_argument(argument, &actual) &&
        actual == expected;
}

static const char *
vitte_main_config_error_details(
    const vitte_config_message_t *message,
    char *buffer,
    size_t capacity)
{
    const char *key;
    const char *name;
    const char *detail;
    int written;

    if (buffer == NULL || capacity == 0u) {
        return NULL;
    }
    key = message != NULL
        ? vitte_config_key_name(message->key)
        : "invalid";
    name = message != NULL && message->origin.name != NULL
        ? message->origin.name
        : "<unknown>";
    detail = message != NULL && message->detail != NULL
        ? message->detail
        : "invalid configuration";
    written = snprintf(
        buffer,
        capacity,
        "key '%s'; file '%s'; line %zu; column %zu; %s",
        key,
        name,
        message != NULL ? message->origin.line : 0u,
        message != NULL ? message->origin.column : 0u,
        detail);
    if (written < 0 || (size_t)written >= capacity) {
        return NULL;
    }
    return buffer;
}

static void
vitte_main_sync_resolved_configuration(
    vitte_main_options_t *options)
{
    const vitte_config_resolved_t *resolved;

    if (options == NULL) {
        return;
    }
    resolved = vitte_config_resolved(&options->config);
    if (resolved == NULL) {
        return;
    }

    options->quiet = resolved->quiet;
    options->verbose = resolved->verbose;
    options->color = resolved->color != VITTE_CONFIG_COLOR_NEVER;
    options->output_path = resolved->output;
    options->debug_info = resolved->emit_debug_info;

    switch (resolved->diagnostic_format) {
        case VITTE_CONFIG_DIAGNOSTIC_JSON:
            options->diagnostic_format = VITTE_MAIN_DIAGNOSTIC_JSON;
            break;
        case VITTE_CONFIG_DIAGNOSTIC_SARIF:
            options->diagnostic_format = VITTE_MAIN_DIAGNOSTIC_SARIF;
            break;
        case VITTE_CONFIG_DIAGNOSTIC_LSP:
            options->diagnostic_format = VITTE_MAIN_DIAGNOSTIC_LSP;
            break;
        case VITTE_CONFIG_DIAGNOSTIC_DEFAULT:
        case VITTE_CONFIG_DIAGNOSTIC_TERMINAL:
        case VITTE_CONFIG_DIAGNOSTIC_SHORT:
        case VITTE_CONFIG_DIAGNOSTIC_COUNT:
            options->diagnostic_format = VITTE_MAIN_DIAGNOSTIC_TERMINAL;
            break;
    }
    vitte_main_active_diagnostic_format = options->diagnostic_format;
}

/* ========================================================================= */
/* Help                                                                      */
/* ========================================================================= */

void
vitte_main_print_usage(
    FILE *stream)
{
    (void)vitte_cli_render_help(stream);
}

void
vitte_main_print_version(void)
{
    (void)printf(
        "%s %s\n"
        "%s compiler\n"
        "language: %s\n"
        "frontend: C17 bootstrap\n",
        VITTE_COMPILER_NAME,
        vitte_version_string(),
        VITTE_LANGUAGE_NAME,
        VITTE_LANGUAGE_NAME);
}

/* ========================================================================= */
/* Option defaults                                                           */
/* ========================================================================= */

static bool
vitte_main_load_configuration(
    vitte_main_options_t *options)
{
    const char *configured_path;
    const char *candidates[2];
    size_t candidate_count;
    size_t index;

    if (options == NULL) {
        return false;
    }
    if (!vitte_config_init(&options->config) ||
        !vitte_config_apply_defaults(&options->config)) {
        return false;
    }
    options->config_initialized = true;

    configured_path = getenv("VITTE_CONFIG_FILE");
    candidate_count = 0u;
    if (configured_path != NULL && configured_path[0] != '\0') {
        candidates[candidate_count++] = configured_path;
    } else {
        candidates[candidate_count++] = "vitte.toml";
        candidates[candidate_count++] = ".vitte/config";
    }

    for (index = 0u; index < candidate_count; ++index) {
        if (!vitte_filesystem_exists(candidates[index])) {
            if (configured_path != NULL && configured_path[0] != '\0') {
                vitte_main_report_path_error(
                    "VITTE_INFRA_E_CONFIG",
                    "configuration file could not be opened",
                    candidates[index],
                    "file does not exist");
                return false;
            }
            continue;
        }
        if (!vitte_config_load_file(
                &options->config,
                candidates[index],
                VITTE_CONFIG_SOURCE_PROJECT)) {
            const vitte_config_message_t *message =
                vitte_config_message(&options->config);
            char details[1024];
            vitte_main_report_path_error(
                "VITTE_INFRA_E_CONFIG",
                "invalid compiler configuration",
                candidates[index],
                vitte_main_config_error_details(
                    message,
                    details,
                    sizeof(details)));
            return false;
        }
        break;
    }

    if (!vitte_config_apply_environment(&options->config, "VITTE_")) {
        const vitte_config_message_t *message =
            vitte_config_message(&options->config);
        char details[1024];
        vitte_main_report_process_error(
            "VITTE_INFRA_E_CONFIG",
            "invalid compiler environment configuration",
            vitte_main_config_error_details(
                message,
                details,
                sizeof(details)));
        return false;
    }

    vitte_main_sync_resolved_configuration(options);
    return true;
}

void
vitte_main_options_destroy(
    vitte_main_options_t *options)
{
    if (options != NULL && options->config_initialized) {
        vitte_config_destroy(&options->config);
        options->config_initialized = false;
    }
}

static void
vitte_main_options_init(
    vitte_main_options_t *options)
{
    if (options == NULL) {
        return;
    }

    memset(
        options,
        0,
        sizeof(*options));

    options->command =
        VITTE_MAIN_COMMAND_COMPILE;

    options->diagnostic_format =
        VITTE_MAIN_DIAGNOSTIC_TERMINAL;
    vitte_main_active_diagnostic_format =
        VITTE_MAIN_DIAGNOSTIC_TERMINAL;

    options->color = true;
    options->validate_utf8 = true;

    if (!vitte_main_load_configuration(options)) {
        vitte_main_options_destroy(options);
    }
}

/* ========================================================================= */
/* Command parser                                                            */
/* ========================================================================= */

static bool
vitte_main_parse_command(
    const char *argument,
    vitte_main_command_t *command)
{
    size_t index;
    const vitte_cli_command_spec_t *specs;

    if (argument == NULL ||
        command == NULL) {
        return false;
    }

    if (!vitte_cli_command_index(argument, &index)) {
        return false;
    }

    specs = vitte_cli_command_specs(NULL);
    switch ((vitte_cli_command_id_t)specs[index].id) {
        case VITTE_CLI_COMMAND_ID_COMPILE: *command = VITTE_MAIN_COMMAND_COMPILE; break;
        case VITTE_CLI_COMMAND_ID_CHECK: *command = VITTE_MAIN_COMMAND_CHECK; break;
        case VITTE_CLI_COMMAND_ID_RUN: *command = VITTE_MAIN_COMMAND_RUN; break;
        case VITTE_CLI_COMMAND_ID_INSTALL: *command = VITTE_MAIN_COMMAND_INSTALL; break;
        case VITTE_CLI_COMMAND_ID_LEX: *command = VITTE_MAIN_COMMAND_LEX; break;
        case VITTE_CLI_COMMAND_ID_PARSE: *command = VITTE_MAIN_COMMAND_PARSE; break;
        case VITTE_CLI_COMMAND_ID_EXPLAIN: *command = VITTE_MAIN_COMMAND_EXPLAIN; break;
        case VITTE_CLI_COMMAND_ID_COMPLETION: *command = VITTE_MAIN_COMMAND_COMPLETION; break;
        case VITTE_CLI_COMMAND_ID_HELP: *command = VITTE_MAIN_COMMAND_HELP; break;
        case VITTE_CLI_COMMAND_ID_VERSION: *command = VITTE_MAIN_COMMAND_VERSION; break;
        default: return false;
    }
    return true;
}

/* ========================================================================= */
/* Diagnostic format parser                                                  */
/* ========================================================================= */

static bool
vitte_main_parse_diagnostic_format(
    const char *format,
    vitte_main_diagnostic_format_t *result)
{
    if (format == NULL ||
        result == NULL) {
        return false;
    }

    if (vitte_main_string_equal(
            format,
            "terminal")) {
        *result =
            VITTE_MAIN_DIAGNOSTIC_TERMINAL;

        return true;
    }

    if (vitte_main_string_equal(
            format,
            "json")) {
        *result =
            VITTE_MAIN_DIAGNOSTIC_JSON;

        return true;
    }

    if (vitte_main_string_equal(
            format,
            "sarif")) {
        *result =
            VITTE_MAIN_DIAGNOSTIC_SARIF;

        return true;
    }

    if (vitte_main_string_equal(
            format,
            "lsp")) {
        *result =
            VITTE_MAIN_DIAGNOSTIC_LSP;

        return true;
    }

    return false;
}

typedef struct vitte_main_config_arguments {
    char **items;
    size_t count;
    size_t capacity;
} vitte_main_config_arguments_t;

static void
vitte_main_config_arguments_destroy(
    vitte_main_config_arguments_t *arguments)
{
    size_t index;

    if (arguments == NULL) {
        return;
    }
    for (index = 0u; index < arguments->count; ++index) {
        free(arguments->items[index]);
    }
    free(arguments->items);
    memset(arguments, 0, sizeof(*arguments));
}

static bool
vitte_main_config_arguments_append(
    vitte_main_config_arguments_t *arguments,
    const char *key,
    const char *value)
{
    size_t key_length;
    size_t value_length;
    size_t total;
    char *entry;
    char **items;

    if (arguments == NULL || key == NULL || key[0] == '\0') {
        return false;
    }
    key_length = strlen(key);
    value_length = value != NULL ? strlen(value) : 0u;
    if (key_length > SIZE_MAX - 3u ||
        (value != NULL &&
         (key_length > SIZE_MAX - 4u ||
          value_length > SIZE_MAX - key_length - 4u))) {
        return false;
    }
    total = 3u + key_length +
        (value != NULL ? 1u + value_length : 0u);
    entry = (char *)malloc(total);
    if (entry == NULL) {
        return false;
    }
    (void)memcpy(entry, "--", 2u);
    (void)memcpy(entry + 2u, key, key_length + 1u);
    if (value != NULL) {
        entry[2u + key_length] = '=';
        (void)memcpy(
            entry + 3u + key_length,
            value,
            value_length + 1u);
    }

    if (arguments->count == arguments->capacity) {
        size_t next_capacity = arguments->capacity == 0u
            ? 8u
            : arguments->capacity * 2u;
        if (next_capacity < arguments->capacity ||
            next_capacity > SIZE_MAX / sizeof(*items)) {
            free(entry);
            return false;
        }
        items = (char **)realloc(
            arguments->items,
            next_capacity * sizeof(*items));
        if (items == NULL) {
            free(entry);
            return false;
        }
        arguments->items = items;
        arguments->capacity = next_capacity;
    }
    arguments->items[arguments->count++] = entry;
    return true;
}

static const char *
vitte_main_config_key_alias(
    const char *name)
{
    if (name == NULL) {
        return NULL;
    }
    if (strcmp(name, "opt") == 0) {
        return "optimization";
    }
    if (strcmp(name, "diagnostic") == 0 ||
        strcmp(name, "diagnostics") == 0) {
        return "diagnostic-format";
    }
    if (strcmp(name, "debug") == 0 ||
        strcmp(name, "debug-info") == 0) {
        return "emit-debug-info";
    }
    return name;
}

static bool
vitte_main_config_key_is_boolean(
    vitte_config_key_t key)
{
    switch (key) {
        case VITTE_CONFIG_KEY_VERBOSE:
        case VITTE_CONFIG_KEY_QUIET:
        case VITTE_CONFIG_KEY_WARNINGS_AS_ERRORS:
        case VITTE_CONFIG_KEY_EMIT_DEBUG_INFO:
        case VITTE_CONFIG_KEY_EMIT_SOURCE_MAP:
        case VITTE_CONFIG_KEY_EMIT_COMMENTS:
        case VITTE_CONFIG_KEY_RUNTIME_CHECKS:
        case VITTE_CONFIG_KEY_BOUNDS_CHECKS:
        case VITTE_CONFIG_KEY_NULL_CHECKS:
        case VITTE_CONFIG_KEY_OVERFLOW_CHECKS:
        case VITTE_CONFIG_KEY_DETERMINISTIC:
        case VITTE_CONFIG_KEY_INCREMENTAL:
        case VITTE_CONFIG_KEY_OFFLINE:
        case VITTE_CONFIG_KEY_LOCKED:
        case VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES:
        case VITTE_CONFIG_KEY_FORCE:
            return true;
        case VITTE_CONFIG_KEY_INVALID:
        case VITTE_CONFIG_KEY_PROFILE:
        case VITTE_CONFIG_KEY_BACKEND:
        case VITTE_CONFIG_KEY_OPTIMIZATION:
        case VITTE_CONFIG_KEY_TARGET:
        case VITTE_CONFIG_KEY_OUTPUT:
        case VITTE_CONFIG_KEY_MANIFEST_PATH:
        case VITTE_CONFIG_KEY_PROJECT_ROOT:
        case VITTE_CONFIG_KEY_BUILD_DIR:
        case VITTE_CONFIG_KEY_CACHE_DIR:
        case VITTE_CONFIG_KEY_SYSROOT:
        case VITTE_CONFIG_KEY_REGISTRY:
        case VITTE_CONFIG_KEY_JOBS:
        case VITTE_CONFIG_KEY_MAX_ERRORS:
        case VITTE_CONFIG_KEY_MAX_OUTPUT_BYTES:
        case VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT:
        case VITTE_CONFIG_KEY_COLOR:
        case VITTE_CONFIG_KEY_COUNT:
            return false;
    }
    return false;
}

static bool
vitte_main_config_key_from_name(
    const char *name,
    vitte_config_key_t *key)
{
    const char *canonical;

    if (name == NULL || key == NULL) {
        return false;
    }
    canonical = vitte_main_config_key_alias(name);
    return vitte_config_parse_key(canonical, key);
}

static bool
vitte_main_apply_cli_configuration(
    vitte_main_options_t *options,
    int argc,
    char **argv)
{
    vitte_main_config_arguments_t normalized;
    int index;
    bool success;

    if (options == NULL || argc < 0 || argv == NULL) {
        return false;
    }
    (void)memset(&normalized, 0, sizeof(normalized));
    for (index = 1; index < argc; ++index) {
        const char *argument = argv[index];
        const char *value = NULL;
        const char *name;
        const char *equals;
        char name_buffer[128];
        vitte_config_key_t key;
        bool negated = false;
        bool inline_value = false;

        if (argument == NULL) {
            continue;
        }
        if (strcmp(argument, "--") == 0) {
            break;
        }
        if (strcmp(argument, "-q") == 0 ||
            strcmp(argument, "--quiet") == 0) {
            if (!vitte_main_config_arguments_append(
                    &normalized, "quiet", "true")) {
                goto out_of_memory;
            }
            continue;
        }
        if (strcmp(argument, "-v") == 0 ||
            strcmp(argument, "--verbose") == 0) {
            if (!vitte_main_config_arguments_append(
                    &normalized, "verbose", "true")) {
                goto out_of_memory;
            }
            continue;
        }
        if (strcmp(argument, "--color") == 0) {
            if (!vitte_main_config_arguments_append(
                    &normalized, "color", "always")) {
                goto out_of_memory;
            }
            continue;
        }
        if (strcmp(argument, "--no-color") == 0) {
            if (!vitte_main_config_arguments_append(
                    &normalized, "color", "never")) {
                goto out_of_memory;
            }
            continue;
        }
        if (strcmp(argument, "-o") == 0) {
            name = "output";
            if (index + 1 < argc && argv[index + 1] != NULL &&
                strcmp(argv[index + 1], "--") != 0) {
                value = argv[++index];
            }
            if (!vitte_main_config_arguments_append(
                    &normalized, name, value)) {
                goto out_of_memory;
            }
            continue;
        }
        if (argument[0] != '-' || argument[1] != '-') {
            continue;
        }

        name = argument + 2u;
        equals = strchr(name, '=');
        if (equals != NULL) {
            size_t length = (size_t)(equals - name);
            if (length == 0u || length >= sizeof(name_buffer)) {
                continue;
            }
            (void)memcpy(name_buffer, name, length);
            name_buffer[length] = '\0';
            name = name_buffer;
            value = equals + 1u;
            inline_value = true;
        } else if (strlen(name) >= sizeof(name_buffer)) {
            continue;
        }

        if (strcmp(name, "no-color") == 0) {
            if (!vitte_main_config_arguments_append(
                    &normalized, "color", "never")) {
                goto out_of_memory;
            }
            continue;
        }
        if (strcmp(name, "color") == 0) {
            if (!inline_value) {
                value = "always";
            }
            if (!vitte_main_config_arguments_append(
                    &normalized, "color", value)) {
                goto out_of_memory;
            }
            continue;
        }
        if (strncmp(name, "no-", 3u) == 0) {
            name += 3u;
            negated = true;
        }
        if (!vitte_main_config_key_from_name(name, &key)) {
            continue;
        }
        if (negated) {
            if (!vitte_main_config_key_is_boolean(key)) {
                continue;
            }
            value = "false";
            inline_value = true;
        } else if (!inline_value &&
                   vitte_main_config_key_is_boolean(key)) {
            value = "true";
            inline_value = true;
        } else if (!inline_value && index + 1 < argc &&
                   argv[index + 1] != NULL &&
                   strcmp(argv[index + 1], "--") != 0) {
            value = argv[++index];
        }
        if (!vitte_main_config_arguments_append(
                &normalized,
                vitte_config_key_name(key),
                value)) {
            goto out_of_memory;
        }
    }

    success = vitte_config_apply_cli(
        &options->config,
        (int)normalized.count,
        (const char *const *)normalized.items);
    if (!success) {
        const vitte_config_message_t *message =
            vitte_config_message(&options->config);
        char details[1024];
        vitte_main_report_process_error(
            "VITTE_INFRA_E_CONFIG",
            "invalid command-line configuration",
            vitte_main_config_error_details(
                message,
                details,
                sizeof(details)));
    }
    vitte_main_config_arguments_destroy(&normalized);
    if (success) {
        vitte_main_sync_resolved_configuration(options);
    }
    return success;

out_of_memory:
    vitte_main_config_arguments_destroy(&normalized);
    vitte_main_report_process_error(
        "VITTE_INFRA_E_CONFIG",
        "cannot normalize command-line configuration",
        "out of memory while preparing CLI settings");
    return false;
}

static bool
vitte_main_cli_argument_is_configuration(
    const char *argument,
    int argc,
    char **argv,
    int index,
    int *consumed)
{
    const char *name;
    const char *equals;
    char name_buffer[128];
    vitte_config_key_t key;
    bool negated;
    bool has_value;

    if (argument == NULL || consumed == NULL) {
        return false;
    }
    *consumed = 1;
    if (strcmp(argument, "-q") == 0 ||
        strcmp(argument, "-v") == 0 ||
        strcmp(argument, "--quiet") == 0 ||
        strcmp(argument, "--verbose") == 0 ||
        strcmp(argument, "--color") == 0 ||
        strcmp(argument, "--no-color") == 0) {
        return true;
    }
    if (strcmp(argument, "-o") == 0) {
        *consumed = index + 1 < argc ? 2 : 1;
        return true;
    }
    if (argument[0] != '-' || argument[1] != '-') {
        return false;
    }
    name = argument + 2u;
    equals = strchr(name, '=');
    has_value = equals != NULL;
    if (equals != NULL) {
        size_t length = (size_t)(equals - name);
        if (length == 0u || length >= sizeof(name_buffer)) {
            return false;
        }
        (void)memcpy(name_buffer, name, length);
        name_buffer[length] = '\0';
        name = name_buffer;
    } else if (strlen(name) >= sizeof(name_buffer)) {
        return false;
    }
    if (strcmp(name, "no-color") == 0 || strcmp(name, "color") == 0) {
        return true;
    }
    negated = strncmp(name, "no-", 3u) == 0;
    if (negated) {
        name += 3u;
    }
    if (!vitte_main_config_key_from_name(name, &key)) {
        return false;
    }
    if (!vitte_main_config_key_is_boolean(key) && !has_value &&
        index + 1 < argc && argv[index + 1] != NULL &&
        strcmp(argv[index + 1], "--") != 0) {
        *consumed = 2;
    }
    return true;
}

/* ========================================================================= */
/* CLI                                                                       */
/* ========================================================================= */

bool
vitte_main_parse_options(
    int argc,
    char **argv,
    vitte_main_options_t *options)
{
    int index;
    bool command_seen;
    bool positional_seen;
    bool color_seen;
    bool no_color_seen;
    bool validate_utf8_seen;
    bool no_validate_utf8_seen;

    if (argc < 0 ||
        argv == NULL ||
        options == NULL) {
        return false;
    }

    vitte_main_options_init(
        options);

    if (!options->config_initialized) {
        return false;
    }

    /* Normalize every configuration-bearing CLI argument once, after the
     * file and environment layers have been loaded.  The legacy parser below
     * remains responsible for commands and pipeline-only switches. */
    if (!vitte_main_apply_cli_configuration(options, argc, argv)) {
        return false;
    }

    command_seen = false;
    positional_seen = false;
    color_seen = false;
    no_color_seen = false;
    validate_utf8_seen = false;
    no_validate_utf8_seen = false;

    for (index = 1;
         index < argc;
         ++index) {
        const char *argument =
            argv[index];

        if (argument == NULL) {
            continue;
        }

        if (vitte_main_option_is(
                argument,
                VITTE_CLI_OPTION_ID_FORWARD_ARGS)) {
            if (options->command ==
                VITTE_MAIN_COMMAND_RUN) {
                options->run_argc =
                    argc - index - 1;

                options->run_argv =
                    argv + index + 1;

                break;
            }

            /*
             * Outside `run`, -- means the remaining argument is positional.
             */
            if (index + 1 < argc) {
                if (positional_seen) {
                    vitte_main_error(
                        "multiple input files are not supported by this entry point");

                    return false;
                }

                options->input_path =
                    argv[index + 1];

                positional_seen = true;

                if (index + 2 < argc) {
                    vitte_main_error(
                        "unexpected arguments after input file");

                    return false;
                }
            }

            break;
        }

        {
            int consumed = 1;

            if (vitte_main_cli_argument_is_configuration(
                    argument,
                    argc,
                    argv,
                    index,
                    &consumed)) {
                index += consumed - 1;
                continue;
            }
        }

        if (!command_seen &&
            !positional_seen &&
            argument[0] != '-') {
            vitte_main_command_t command;

            if (vitte_main_parse_command(
                    argument,
                    &command)) {
                options->command = command;
                command_seen = true;

                continue;
            }
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_HELP)) {
            options->command =
                VITTE_MAIN_COMMAND_HELP;

            return true;
        }

        if (options->command == VITTE_MAIN_COMMAND_INSTALL &&
            (strcmp(argument, "--global") == 0)) {
            options->package_global = true;
            continue;
        }

        if (options->command == VITTE_MAIN_COMMAND_INSTALL &&
            (strcmp(argument, "--version") == 0 ||
             strncmp(argument, "--version=", 10u) == 0)) {
            const char *version = NULL;
            if (argument[9] == '=') {
                version = argument + 10u;
            } else if (index + 1 < argc && argv[index + 1] != NULL &&
                       argv[index + 1][0] != '-') {
                version = argv[++index];
            }
            if (version == NULL || version[0] == '\0') {
                vitte_main_error("--version requires a package version after install");
                return false;
            }
            options->package_version = version;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_VERSION)) {
            options->command =
                VITTE_MAIN_COMMAND_VERSION;

            return true;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_QUIET)) {
            options->quiet = true;
            (void)vitte_config_set_bool(
                &options->config, VITTE_CONFIG_KEY_QUIET, true,
                vitte_config_origin_make(VITTE_CONFIG_SOURCE_COMMAND_LINE,
                                         "--quiet", 0u, 0u));
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_VERBOSE)) {
            options->verbose = true;
            (void)vitte_config_set_bool(
                &options->config, VITTE_CONFIG_KEY_VERBOSE, true,
                vitte_config_origin_make(VITTE_CONFIG_SOURCE_COMMAND_LINE,
                                         "--verbose", 0u, 0u));
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_COLOR)) {
            if (no_color_seen) {
                vitte_main_error(
                    "--color and --no-color cannot be used together");
                return false;
            }
            color_seen = true;
            options->color = true;
            (void)vitte_config_set_text(
                &options->config, VITTE_CONFIG_KEY_COLOR, "always",
                vitte_config_origin_make(VITTE_CONFIG_SOURCE_COMMAND_LINE,
                                         "--color", 0u, 0u));
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_NO_COLOR)) {
            if (color_seen) {
                vitte_main_error(
                    "--color and --no-color cannot be used together");
                return false;
            }
            no_color_seen = true;
            options->color = false;
            (void)vitte_config_set_text(
                &options->config, VITTE_CONFIG_KEY_COLOR, "never",
                vitte_config_origin_make(VITTE_CONFIG_SOURCE_COMMAND_LINE,
                                         "--no-color", 0u, 0u));
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_VALIDATE_UTF8)) {
            if (no_validate_utf8_seen) {
                vitte_main_error(
                    "--validate-utf8 and --no-validate-utf8 cannot be used together");
                return false;
            }
            validate_utf8_seen = true;
            options->validate_utf8 = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_NO_VALIDATE_UTF8)) {
            if (validate_utf8_seen) {
                vitte_main_error(
                    "--validate-utf8 and --no-validate-utf8 cannot be used together");
                return false;
            }
            no_validate_utf8_seen = true;
            options->validate_utf8 = false;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_TIMINGS)) {
            options->show_timings = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_EMIT_TOKENS)) {
            options->emit_tokens = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_EMIT_AST)) {
            options->emit_ast = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_EMIT_HIR)) {
            options->emit_hir = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_EMIT_IR)) {
            options->emit_ir = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_EMIT_C)) {
            options->emit_c = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_EMIT_OBJECT)) {
            options->emit_object = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_DEBUG_INFO)) {
            options->debug_info = true;
            options->emit_line_directives = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_DEBUG_PROBES)) {
            options->debug_info = true;
            options->debug_probes = true;
            options->emit_line_directives = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_LINE_DIRECTIVES)) {
            options->emit_line_directives = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_STOP_AFTER_LEXER)) {
            options->stop_after_lexer = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_STOP_AFTER_PARSER)) {
            options->stop_after_parser = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_STOP_AFTER_SEMA)) {
            options->stop_after_sema = true;
            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_OUTPUT)) {
            if (index + 1 >= argc) {
                vitte_main_error(
                    "missing path after -o");

                return false;
            }

            index++;

            options->output_path =
                argv[index];

            (void)vitte_config_set_string(
                &options->config,
                VITTE_CONFIG_KEY_OUTPUT,
                options->output_path,
                vitte_config_origin_make(VITTE_CONFIG_SOURCE_COMMAND_LINE,
                                         "-o", 0u, 0u));

            continue;
        }

        if (vitte_main_option_is(argument, VITTE_CLI_OPTION_ID_DIAGNOSTIC)) {
            const char *format = NULL;

            if (!vitte_cli_option_value_for_argument(
                    argument,
                    VITTE_CLI_OPTION_ID_DIAGNOSTIC,
                    &format)) {
                vitte_main_error_argument(
                    "missing value for diagnostic option",
                    argument);
                return false;
            }

            if (!vitte_main_parse_diagnostic_format(
                    format,
                    &options->diagnostic_format)) {
                vitte_main_error_argument(
                    "unknown diagnostic format",
                    format);

                return false;
            }

            vitte_main_active_diagnostic_format =
                options->diagnostic_format;
            (void)vitte_config_set_text(
                &options->config,
                VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT,
                format,
                vitte_config_origin_make(VITTE_CONFIG_SOURCE_COMMAND_LINE,
                                         "--diagnostic", 0u, 0u));
            continue;
        }

        if (argument[0] == '-') {
            vitte_main_error_argument(
                "unknown option",
                argument);

            return false;
        }

        if (positional_seen) {
            vitte_main_error_argument(
                "unexpected positional argument",
                argument);

            return false;
        }

        options->input_path =
            argument;

        positional_seen = true;
    }

    if (options->command ==
            VITTE_MAIN_COMMAND_HELP ||
        options->command ==
            VITTE_MAIN_COMMAND_VERSION) {
        return true;
    }

    if (options->input_path == NULL &&
        options->command != VITTE_MAIN_COMMAND_COMPLETION) {
        if (options->command == VITTE_MAIN_COMMAND_INSTALL) {
            vitte_main_error("install requires a package name");
            return false;
        }
        vitte_main_error(
            "no input file");

        return false;
    }

    if (options->command == VITTE_MAIN_COMMAND_INSTALL &&
        (options->emit_c || options->emit_object || options->emit_ir ||
         options->output_path != NULL || options->run_argc != 0)) {
        vitte_main_error("install accepts a package name, --version and --global");
        return false;
    }

    if (options->command == VITTE_MAIN_COMMAND_COMPLETION &&
        (options->input_path == NULL ||
         (strcmp(options->input_path, "bash") != 0 &&
          strcmp(options->input_path, "zsh") != 0 &&
          strcmp(options->input_path, "fish") != 0))) {
        vitte_main_error(
            "completion requires one of: bash, zsh, fish");
        return false;
    }

    if (options->quiet &&
        options->verbose) {
        vitte_main_error(
            "--quiet and --verbose cannot be used together");
        return false;
    }

    if (options->stop_after_lexer &&
        options->stop_after_parser) {
        vitte_main_error(
            "--stop-after-lexer and --stop-after-parser cannot be used together");

        return false;
    }

    if (options->stop_after_lexer &&
        options->stop_after_sema) {
        vitte_main_error(
            "--stop-after-lexer and --stop-after-sema cannot be used together");

        return false;
    }

    if (options->stop_after_parser &&
        options->stop_after_sema) {
        vitte_main_error(
            "--stop-after-parser and --stop-after-sema cannot be used together");

        return false;
    }

    if (options->command == VITTE_MAIN_COMMAND_RUN &&
        (options->emit_c || options->emit_ir || options->emit_object ||
         options->stop_after_lexer || options->stop_after_parser ||
         options->stop_after_sema)) {
        vitte_main_error(
            "run cannot be combined with emission or stop-after options");
        return false;
    }

    if (options->emit_ir &&
        (options->command != VITTE_MAIN_COMMAND_COMPILE ||
         options->stop_after_lexer || options->stop_after_parser ||
         options->stop_after_sema)) {
        vitte_main_error(
            "--emit-ir is available only for compile and cannot be combined with stop-after options");
        return false;
    }

    if (options->command == VITTE_MAIN_COMMAND_CHECK &&
        (options->emit_c || options->emit_object)) {
        vitte_main_error(
            "check stops after semantic analysis and cannot emit C or objects");
        return false;
    }

    if (options->emit_object &&
        (options->emit_c ||
         options->command != VITTE_MAIN_COMMAND_COMPILE ||
         options->stop_after_lexer || options->stop_after_parser ||
         options->stop_after_sema)) {
        vitte_main_error(
            "-c/--emit-object is available only for compile and cannot be combined with --emit-c");
        return false;
    }
    if (options->emit_object && options->output_path == NULL) {
        vitte_main_error(
            "-c/--emit-object requires -o PATH");
        return false;
    }

    if (options->command == VITTE_MAIN_COMMAND_EXPLAIN &&
        (options->output_path != NULL || options->emit_tokens ||
         options->emit_ast || options->emit_hir || options->emit_ir ||
         options->emit_c || options->emit_object ||
         options->stop_after_lexer ||
         options->stop_after_parser || options->stop_after_sema)) {
        vitte_main_error(
            "explain accepts only a diagnostic code and output options");
        return false;
    }

    if (options->command == VITTE_MAIN_COMMAND_COMPLETION &&
        (options->output_path != NULL || options->diagnostic_format !=
         VITTE_MAIN_DIAGNOSTIC_TERMINAL)) {
        vitte_main_error(
            "completion accepts only a shell name");
        return false;
    }

    /* All layers are validated once after defaults, file, environment and
     * command-line assignments have been merged. */
    vitte_main_sync_resolved_configuration(options);
    if (!vitte_config_validate(&options->config)) {
        const vitte_config_message_t *message =
            vitte_config_message(&options->config);
        char details[1024];
        vitte_main_report_process_error(
            "VITTE_INFRA_E_CONFIG",
            "invalid merged compiler configuration",
            vitte_main_config_error_details(
                message,
                details,
                sizeof(details)));
        return false;
    }

    return true;
}

/* ========================================================================= */

