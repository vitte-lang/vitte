#ifndef VITTE_BOOTSTRAP_CLI_COMMAND_SPEC_H
#define VITTE_BOOTSTRAP_CLI_COMMAND_SPEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_CLI_COMMAND(name, summary, requires_input, forwards_arguments, id) \
    VITTE_CLI_COMMAND_ID_##id,
#define VITTE_CLI_OPTION(spelling, value_name, summary, id)
typedef enum vitte_cli_command_id {
#include "command_spec.def"
} vitte_cli_command_id_t;
#undef VITTE_CLI_COMMAND
#undef VITTE_CLI_OPTION

#define VITTE_CLI_COMMAND(name, summary, requires_input, forwards_arguments, id)
#define VITTE_CLI_OPTION(spelling, value_name, summary, id) \
    VITTE_CLI_OPTION_ID_##id,
typedef enum vitte_cli_option_id {
#include "command_spec.def"
} vitte_cli_option_id_t;
#undef VITTE_CLI_COMMAND
#undef VITTE_CLI_OPTION

typedef struct vitte_cli_command_spec {
    const char *name;
    const char *summary;
    bool requires_input;
    bool forwards_arguments;
    vitte_cli_command_id_t id;
} vitte_cli_command_spec_t;

typedef struct vitte_cli_option_spec {
    const char *spelling;
    const char *value_name;
    const char *summary;
    vitte_cli_option_id_t id;
} vitte_cli_option_spec_t;

const vitte_cli_command_spec_t *
vitte_cli_command_specs(size_t *count);

const vitte_cli_option_spec_t *
vitte_cli_option_specs(size_t *count);

bool
vitte_cli_command_is_known(const char *name);

bool
vitte_cli_command_index(const char *name, size_t *index);

bool
vitte_cli_option_id_for_argument(const char *argument,
                                  vitte_cli_option_id_t *id);

bool
vitte_cli_option_value_for_argument(const char *argument,
                                    vitte_cli_option_id_t id,
                                    const char **value);

bool
vitte_cli_render_help(FILE *stream);

bool
vitte_cli_render_completion(FILE *stream, const char *shell);

#ifdef __cplusplus
}
#endif

#endif
