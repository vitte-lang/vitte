#include "cli.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../api/context.h"
#include "../diagnostic/diagnostic.h"
#include "../driver/driver.h"

#define VITTE_CLI_USAGE "usage: vitte-bootstrap <check|build|emit-c|run> <input.vit> [-o output] [--cc cc] [--keep-c]\n"

static bool vitte_cli_streq(const char *left, const char *right) {
    return left != NULL && right != NULL && strcmp(left, right) == 0;
}

void vitte_cli_options_init(vitte_cli_options_t *options) {
    const char *cc;

    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->command = VITTE_CLI_COMMAND_HELP;
    options->command_name = "help";
    cc = getenv("CC");
    options->c_compiler = cc != NULL && cc[0] != '\0' ? cc : "cc";
}

const char *vitte_cli_command_name(vitte_cli_command_t command) {
    switch (command) {
        case VITTE_CLI_COMMAND_HELP:
            return "help";
        case VITTE_CLI_COMMAND_VERSION:
            return "version";
        case VITTE_CLI_COMMAND_CHECK:
            return "check";
        case VITTE_CLI_COMMAND_EMIT_C:
            return "emit-c";
        case VITTE_CLI_COMMAND_BUILD:
            return "build";
        case VITTE_CLI_COMMAND_RUN:
            return "run";
        default:
            return "unknown";
    }
}

void vitte_cli_print_help(FILE *stream) {
    if (stream == NULL) {
        return;
    }

    fputs(VITTE_CLI_USAGE, stream);
    fputs("\ncommands:\n", stream);
    fputs("  check    validate a Vitte source file\n", stream);
    fputs("  emit-c   write lowered C17 source\n", stream);
    fputs("  build    build a native executable through a C17 compiler\n", stream);
    fputs("  run      build to a temporary executable and run it\n", stream);
    fputs("\noptions:\n", stream);
    fputs("  -h, --help       show this help\n", stream);
    fputs("  -V, --version    show version\n", stream);
    fputs("  -o, --output     set output path\n", stream);
    fputs("  --cc             set host C compiler\n", stream);
    fputs("  --keep-c         keep sidecar C file after build/run\n", stream);
    fputs("  --emit-c         accepted alias flag for build metadata\n", stream);
}

void vitte_cli_print_version(FILE *stream) {
    if (stream != NULL) {
        fputs(VITTE_CLI_VERSION_TEXT, stream);
        fputc('\n', stream);
    }
}

static bool vitte_cli_command_from_text(const char *text, vitte_cli_command_t *command) {
    if (vitte_cli_streq(text, "help")) {
        *command = VITTE_CLI_COMMAND_HELP;
    } else if (vitte_cli_streq(text, "version")) {
        *command = VITTE_CLI_COMMAND_VERSION;
    } else if (vitte_cli_streq(text, "check")) {
        *command = VITTE_CLI_COMMAND_CHECK;
    } else if (vitte_cli_streq(text, "emit-c")) {
        *command = VITTE_CLI_COMMAND_EMIT_C;
    } else if (vitte_cli_streq(text, "build")) {
        *command = VITTE_CLI_COMMAND_BUILD;
    } else if (vitte_cli_streq(text, "run")) {
        *command = VITTE_CLI_COMMAND_RUN;
    } else {
        return false;
    }
    return true;
}

bool vitte_cli_parse_options(int argc, char **argv, vitte_cli_options_t *options) {
    int index;

    if (options == NULL) {
        return false;
    }

    vitte_cli_options_init(options);
    if (argc <= 1) {
        return true;
    }

    if (vitte_cli_streq(argv[1], "--help") || vitte_cli_streq(argv[1], "-h")) {
        options->command = VITTE_CLI_COMMAND_HELP;
        options->command_name = "help";
        return true;
    }
    if (vitte_cli_streq(argv[1], "--version") || vitte_cli_streq(argv[1], "-V")) {
        options->command = VITTE_CLI_COMMAND_VERSION;
        options->command_name = "version";
        return true;
    }
    if (!vitte_cli_command_from_text(argv[1], &options->command)) {
        fprintf(stderr, "vitte-bootstrap: unknown command: %s\n", argv[1]);
        return false;
    }

    options->command_name = vitte_cli_command_name(options->command);
    index = 2;
    while (index < argc) {
        const char *argument = argv[index];

        if (vitte_cli_streq(argument, "--")) {
            index++;
            if (index >= argc) {
                fputs("vitte-bootstrap: missing input path after --\n", stderr);
                return false;
            }
            if (options->input_path != NULL) {
                fprintf(stderr, "vitte-bootstrap: unexpected argument: %s\n", argv[index]);
                return false;
            }
            options->input_path = argv[index++];
            if (index < argc) {
                fprintf(stderr, "vitte-bootstrap: unexpected argument: %s\n", argv[index]);
                return false;
            }
            break;
        }
        if (vitte_cli_streq(argument, "-o") || vitte_cli_streq(argument, "--output")) {
            index++;
            if (index >= argc) {
                fputs("vitte-bootstrap: missing value for -o\n", stderr);
                return false;
            }
            options->output_path = argv[index++];
            continue;
        }
        if (vitte_cli_streq(argument, "--cc")) {
            index++;
            if (index >= argc) {
                fputs("vitte-bootstrap: missing value for --cc\n", stderr);
                return false;
            }
            options->c_compiler = argv[index++];
            continue;
        }
        if (vitte_cli_streq(argument, "--keep-c")) {
            options->keep_intermediate_c = true;
            index++;
            continue;
        }
        if (vitte_cli_streq(argument, "--emit-c")) {
            index++;
            continue;
        }
        if (argument[0] == '-' && !vitte_cli_streq(argument, "-")) {
            fprintf(stderr, "vitte-bootstrap: unknown option: %s\n", argument);
            return false;
        }
        if (options->input_path != NULL) {
            fprintf(stderr, "vitte-bootstrap: unexpected argument: %s\n", argument);
            return false;
        }
        options->input_path = argument;
        index++;
    }

    if (options->input_path == NULL &&
        options->command != VITTE_CLI_COMMAND_HELP &&
        options->command != VITTE_CLI_COMMAND_VERSION) {
        fputs("vitte-bootstrap: missing input path\n", stderr);
        return false;
    }

    return true;
}

static char *vitte_cli_default_output_path(const char *input_path, const char *suffix) {
    const char *slash;
    const char *base;
    const char *dot;
    size_t stem_length;
    size_t suffix_length;
    char *path;

    if (input_path == NULL || suffix == NULL) {
        return NULL;
    }

    slash = strrchr(input_path, '/');
    base = slash == NULL ? input_path : slash + 1;
    dot = strrchr(base, '.');
    stem_length = dot == NULL ? strlen(base) : (size_t)(dot - base);
    suffix_length = strlen(suffix);
    path = (char *)malloc(stem_length + suffix_length + 1u);
    if (path == NULL) {
        return NULL;
    }
    memcpy(path, base, stem_length);
    memcpy(path + stem_length, suffix, suffix_length);
    path[stem_length + suffix_length] = '\0';
    return path;
}

static char *vitte_cli_shell_quote(const char *text) {
    size_t extra = 2u;
    size_t index;
    size_t out = 0u;
    char *quoted;

    if (text == NULL) {
        return NULL;
    }
    for (index = 0u; text[index] != '\0'; index++) {
        extra += text[index] == '\'' ? 4u : 1u;
    }
    quoted = (char *)malloc(extra + 1u);
    if (quoted == NULL) {
        return NULL;
    }
    quoted[out++] = '\'';
    for (index = 0u; text[index] != '\0'; index++) {
        if (text[index] == '\'') {
            memcpy(quoted + out, "'\\''", 4u);
            out += 4u;
        } else {
            quoted[out++] = text[index];
        }
    }
    quoted[out++] = '\'';
    quoted[out] = '\0';
    return quoted;
}

static int vitte_cli_run_executable(const char *path) {
    char *quoted;
    int status;

    quoted = vitte_cli_shell_quote(path);
    if (quoted == NULL) {
        fputs("vitte-bootstrap: out of memory preparing run command\n", stderr);
        return VITTE_CLI_EXIT_INTERNAL;
    }
    status = system(quoted);
    free(quoted);
    return status == 0 ? VITTE_CLI_EXIT_OK : VITTE_CLI_EXIT_ERROR;
}

static int vitte_cli_exit_from_status(vitte_status_t status) {
    if (status == VITTE_STATUS_ERROR_OUT_OF_MEMORY || status == VITTE_STATUS_ERROR_INTERNAL) {
        return VITTE_CLI_EXIT_INTERNAL;
    }
    return VITTE_CLI_EXIT_ERROR;
}

static void vitte_cli_print_driver_failure(const vitte_driver_t *driver) {
    const vitte_diagnostic_bag_t *diagnostics;
    const vitte_error_t *error;

    diagnostics = vitte_driver_diagnostics(driver);
    if (diagnostics != NULL && vitte_diagnostic_bag_total_count(diagnostics) > 0u) {
        (void)vitte_diagnostic_write_all(diagnostics, stderr);
    }

    error = vitte_driver_last_error(driver);
    if (error != NULL && vitte_error_is_set(error)) {
        if (error->details != NULL && error->details[0] != '\0') {
            fprintf(stderr, "vitte-bootstrap: %s: %s\n", error->message, error->details);
        } else {
            fprintf(stderr, "vitte-bootstrap: %s\n", error->message);
        }
    }
}

static void vitte_cli_fill_driver_options(
    const vitte_cli_options_t *options,
    vitte_driver_emit_kind_t emit_kind,
    const char *output_path,
    vitte_driver_options_t *driver_options
) {
    vitte_driver_options_init(driver_options);
    driver_options->input_path = options->input_path;
    driver_options->output_path = output_path;
    driver_options->emit_kind = emit_kind;
    driver_options->c_compiler = options->c_compiler;
    driver_options->keep_intermediate_c = options->keep_intermediate_c;
}

static int vitte_cli_run_driver_command(
    const vitte_cli_options_t *options,
    vitte_driver_emit_kind_t emit_kind,
    bool run_output
) {
    vitte_api_config_t config;
    vitte_context_t context;
    vitte_driver_options_t driver_options;
    vitte_driver_t driver;
    vitte_driver_input_t input;
    vitte_driver_result_t result;
    vitte_status_t status;
    const char *effective_output_path = options->output_path;
    char *owned_output_path = NULL;
    int exit_code = VITTE_CLI_EXIT_OK;

    if (options == NULL || options->input_path == NULL) {
        return VITTE_CLI_EXIT_USAGE;
    }
    if ((emit_kind == VITTE_DRIVER_EMIT_BINARY || run_output) &&
        (effective_output_path == NULL || effective_output_path[0] == '\0')) {
        owned_output_path = vitte_cli_default_output_path(options->input_path, "");
        if (owned_output_path == NULL) {
            fputs("vitte-bootstrap: out of memory preparing output path\n", stderr);
            return VITTE_CLI_EXIT_INTERNAL;
        }
        effective_output_path = owned_output_path;
    }

    vitte_api_config_init(&config);
    if (vitte_context_init(&context, &config) != VITTE_STATUS_OK) {
        free(owned_output_path);
        return VITTE_CLI_EXIT_INTERNAL;
    }

    vitte_cli_fill_driver_options(options, emit_kind, effective_output_path, &driver_options);
    status = vitte_driver_init(&driver, &context, &driver_options);
    if (status != VITTE_STATUS_OK) {
        const vitte_error_t *error = vitte_driver_last_error(&driver);
        if (error != NULL && vitte_error_is_set(error)) {
            fprintf(stderr, "vitte-bootstrap: %s\n", error->message);
        }
        vitte_context_destroy(&context);
        free(owned_output_path);
        return vitte_cli_exit_from_status(status);
    }

    vitte_driver_input_init(&input);
    status = vitte_driver_input_from_file(&input, options->input_path, 0u);
    if (status != VITTE_STATUS_OK) {
        fprintf(stderr, "vitte-bootstrap: cannot open %s: %s\n", options->input_path, strerror(errno));
        vitte_driver_shutdown(&driver);
        vitte_context_destroy(&context);
        free(owned_output_path);
        return VITTE_CLI_EXIT_ERROR;
    }

    vitte_driver_result_init(&result);
    if (emit_kind == VITTE_DRIVER_EMIT_CHECK) {
        status = vitte_driver_check(&driver, &input, &result);
    } else if (emit_kind == VITTE_DRIVER_EMIT_C) {
        status = vitte_driver_emit_c(&driver, &input, effective_output_path, &result);
    } else {
        status = vitte_driver_build(&driver, &input, effective_output_path, &result);
    }

    if (status != VITTE_STATUS_OK) {
        vitte_cli_print_driver_failure(&driver);
        exit_code = vitte_cli_exit_from_status(status);
    } else if (emit_kind == VITTE_DRIVER_EMIT_CHECK) {
        printf("[vitte-bootstrap] check ok: %s\n", options->input_path);
    } else if (emit_kind == VITTE_DRIVER_EMIT_C) {
        if (effective_output_path != NULL) {
            printf("[vitte-bootstrap] wrote C17: %s\n", effective_output_path);
        } else if (result.output.buffer != NULL) {
            fputs(result.output.buffer, stdout);
        }
    } else {
        printf("[vitte-bootstrap] built: %s\n", effective_output_path);
        if (run_output) {
            exit_code = vitte_cli_run_executable(effective_output_path);
        }
    }

    vitte_driver_input_destroy(&input);
    vitte_driver_shutdown(&driver);
    vitte_context_destroy(&context);
    free(owned_output_path);
    return exit_code;
}

int vitte_cli_run(const vitte_cli_options_t *options) {
    if (options == NULL) {
        return VITTE_CLI_EXIT_USAGE;
    }

    switch (options->command) {
        case VITTE_CLI_COMMAND_HELP:
            vitte_cli_print_help(stdout);
            return VITTE_CLI_EXIT_OK;
        case VITTE_CLI_COMMAND_VERSION:
            vitte_cli_print_version(stdout);
            return VITTE_CLI_EXIT_OK;
        case VITTE_CLI_COMMAND_CHECK:
            return vitte_cli_run_driver_command(options, VITTE_DRIVER_EMIT_CHECK, false);
        case VITTE_CLI_COMMAND_EMIT_C:
            return vitte_cli_run_driver_command(options, VITTE_DRIVER_EMIT_C, false);
        case VITTE_CLI_COMMAND_BUILD:
            return vitte_cli_run_driver_command(options, VITTE_DRIVER_EMIT_BINARY, false);
        case VITTE_CLI_COMMAND_RUN:
            return vitte_cli_run_driver_command(options, VITTE_DRIVER_EMIT_BINARY, true);
        default:
            fprintf(stderr, "vitte-bootstrap: unknown command: %s\n", options->command_name);
            return VITTE_CLI_EXIT_USAGE;
    }
}

int vitte_cli_main(int argc, char **argv) {
    vitte_cli_options_t options;

    if (!vitte_cli_parse_options(argc, argv, &options)) {
        vitte_cli_print_help(stderr);
        return VITTE_CLI_EXIT_USAGE;
    }
    return vitte_cli_run(&options);
}
