#ifndef VITTE_BOOTSTRAP_CLI_H
#define VITTE_BOOTSTRAP_CLI_H

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_CLI_VERSION_TEXT "vitte-bootstrap c17 0.1.0"

typedef enum vitte_cli_exit_code {
    VITTE_CLI_EXIT_OK = 0,
    VITTE_CLI_EXIT_USAGE = 1,
    VITTE_CLI_EXIT_ERROR = 2,
    VITTE_CLI_EXIT_INTERNAL = 3
} vitte_cli_exit_code_t;

typedef enum vitte_cli_command {
    VITTE_CLI_COMMAND_HELP = 0,
    VITTE_CLI_COMMAND_VERSION,
    VITTE_CLI_COMMAND_CHECK,
    VITTE_CLI_COMMAND_EMIT_C,
    VITTE_CLI_COMMAND_BUILD,
    VITTE_CLI_COMMAND_RUN
} vitte_cli_command_t;

typedef struct vitte_cli_options {
    vitte_cli_command_t command;
    const char *command_name;
    const char *input_path;
    const char *output_path;
    const char *c_compiler;
    bool keep_intermediate_c;
} vitte_cli_options_t;

void vitte_cli_options_init(vitte_cli_options_t *options);
const char *vitte_cli_command_name(vitte_cli_command_t command);
void vitte_cli_print_help(FILE *stream);
void vitte_cli_print_version(FILE *stream);
bool vitte_cli_parse_options(int argc, char **argv, vitte_cli_options_t *options);
int vitte_cli_run(const vitte_cli_options_t *options);
int vitte_cli_main(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_CLI_H */
