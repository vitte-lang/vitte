#include "command_spec.h"

#include <string.h>

#define VITTE_CLI_COMMAND(name, summary, requires_input, forwards_arguments, id) \
    {name, summary, requires_input, forwards_arguments, VITTE_CLI_COMMAND_ID_##id},
#define VITTE_CLI_OPTION(spelling, value_name, summary, id)
static const vitte_cli_command_spec_t vitte_cli_commands[] = {
#include "command_spec.def"
};
#undef VITTE_CLI_COMMAND
#undef VITTE_CLI_OPTION

#define VITTE_CLI_COMMAND(name, summary, requires_input, forwards_arguments, id)
#define VITTE_CLI_OPTION(spelling, value_name, summary, id) \
    {spelling, value_name, summary, VITTE_CLI_OPTION_ID_##id},
static const vitte_cli_option_spec_t vitte_cli_options[] = {
#include "command_spec.def"
};
#undef VITTE_CLI_COMMAND
#undef VITTE_CLI_OPTION

static bool
vitte_cli_copy_option_token(
    const char *begin,
    const char *end,
    char *buffer,
    size_t capacity)
{
    size_t length;

    while (begin < end && (*begin == ' ' || *begin == '\t')) {
        ++begin;
    }
    while (end > begin && (end[-1] == ' ' || end[-1] == '\t')) {
        --end;
    }
    {
        const char *value = begin;
        while (value < end && *value != ' ' && *value != '\t' && *value != '=') {
            ++value;
        }
        end = value;
    }
    if (begin == end || *begin != '-') {
        return false;
    }
    length = (size_t)(end - begin);
    if (length == 0u || length + 1u > capacity) {
        return false;
    }
    memcpy(buffer, begin, length);
    buffer[length] = '\0';
    return true;
}

static bool
vitte_cli_completion_option_name(
    const char *spelling,
    size_t wanted,
    char *buffer,
    size_t capacity)
{
    const char *begin;
    const char *cursor;
    size_t index;

    if (spelling == NULL || buffer == NULL || capacity == 0u) {
        return false;
    }
    begin = spelling;
    index = 0u;
    for (cursor = spelling;; ++cursor) {
        if (*cursor == ',' || *cursor == '\0') {
            const char *end = cursor;
            while (end > begin && (end[-1] == ' ' || end[-1] == '\t')) {
                --end;
            }
            if (index == wanted) {
                return vitte_cli_copy_option_token(
                    begin, end, buffer, capacity);
            }
            if (*cursor == '\0') {
                return false;
            }
            ++index;
            begin = cursor + 1;
        }
    }
}

static void
vitte_cli_render_completion_options(
    FILE *stream,
    const char *spelling,
    bool fish)
{
    size_t alias;
    char option[64];

    for (alias = 0u; alias < 8u; ++alias) {
        if (!vitte_cli_completion_option_name(
                spelling, alias, option, sizeof(option))) {
            break;
        }
        if (fish) {
            if (option[0] == '-' && option[1] == '-' && option[2] != '\0') {
                (void)fprintf(stream, "complete -c vitte -l '%s'\n", option + 2);
            } else if (option[0] == '-' && option[1] != '\0' &&
                       option[2] == '\0') {
                (void)fprintf(stream, "complete -c vitte -s '%c'\n", option[1]);
            }
        } else {
            (void)fprintf(stream, "%s ", option);
        }
    }
}

const vitte_cli_command_spec_t *
vitte_cli_command_specs(size_t *count)
{
    if (count != NULL) {
        *count = sizeof(vitte_cli_commands) / sizeof(vitte_cli_commands[0]);
    }
    return vitte_cli_commands;
}

const vitte_cli_option_spec_t *
vitte_cli_option_specs(size_t *count)
{
    if (count != NULL) {
        *count = sizeof(vitte_cli_options) / sizeof(vitte_cli_options[0]);
    }
    return vitte_cli_options;
}

bool
vitte_cli_command_is_known(const char *name)
{
    size_t count;
    size_t index;

    if (name == NULL) {
        return false;
    }
    (void)vitte_cli_command_specs(&count);
    for (index = 0u; index < count; ++index) {
        if (strcmp(name, vitte_cli_commands[index].name) == 0) {
            return true;
        }
    }
    return false;
}

bool
vitte_cli_command_index(const char *name, size_t *index)
{
    size_t count;
    size_t cursor;

    if (name == NULL || index == NULL) {
        return false;
    }
    (void)vitte_cli_command_specs(&count);
    for (cursor = 0u; cursor < count; ++cursor) {
        if (strcmp(name, vitte_cli_commands[cursor].name) == 0) {
            *index = cursor;
            return true;
        }
    }
    return false;
}

static bool
vitte_cli_option_spelling_matches(
    const char *spelling,
    const char *argument)
{
    const char *begin = spelling;
    const char *cursor;

    if (spelling == NULL || argument == NULL) {
        return false;
    }
    for (cursor = spelling;; ++cursor) {
        if (*cursor == ',' || *cursor == '\0') {
            const char *name = begin;
            const char *end = cursor;
            const char *value_marker;
            size_t length;

            while (name < end && (*name == ' ' || *name == '\t')) {
                ++name;
            }
            while (end > name && (end[-1] == ' ' || end[-1] == '\t')) {
                --end;
            }
            value_marker = name;
            while (value_marker < end && value_marker[0] != ' ' &&
                   value_marker[0] != '\t' && value_marker[0] != '=') {
                ++value_marker;
            }
            length = (size_t)(value_marker - name);
            if (length > 0u && name[0] == '-') {
                const bool inline_value = value_marker < end &&
                    value_marker[0] == '=';
                if (inline_value) {
                    if (strncmp(argument, name, length) == 0 &&
                        argument[length] == '=') {
                        return true;
                    }
                } else if (strlen(argument) == length &&
                           memcmp(argument, name, length) == 0) {
                    return true;
                }
            }
            if (*cursor == '\0') {
                return false;
            }
            begin = cursor + 1;
        }
    }
}

bool
vitte_cli_option_id_for_argument(
    const char *argument,
    vitte_cli_option_id_t *id)
{
    size_t count;
    size_t index;

    if (argument == NULL || id == NULL) {
        return false;
    }
    (void)vitte_cli_option_specs(&count);
    for (index = 0u; index < count; ++index) {
        if (vitte_cli_option_spelling_matches(
                vitte_cli_options[index].spelling, argument)) {
            *id = vitte_cli_options[index].id;
            return true;
        }
    }
    return false;
}

bool
vitte_cli_option_value_for_argument(
    const char *argument,
    vitte_cli_option_id_t id,
    const char **value)
{
    size_t count;
    size_t index;
    vitte_cli_option_id_t actual;

    if (argument == NULL || value == NULL ||
        !vitte_cli_option_id_for_argument(argument, &actual) ||
        actual != id) {
        return false;
    }
    (void)vitte_cli_option_specs(&count);
    for (index = 0u; index < count; ++index) {
        const vitte_cli_option_spec_t *option = &vitte_cli_options[index];
        if (option->id != id || option->value_name == NULL) {
            continue;
        }
        {
            const char *equals = strchr(argument, '=');
            if (equals != NULL && equals[1] != '\0') {
                *value = equals + 1;
                return true;
            }
        }
    }
    return false;
}

bool
vitte_cli_render_help(FILE *stream)
{
    size_t command_count;
    size_t option_count;
    size_t index;

    if (stream == NULL) {
        return false;
    }

    (void)fprintf(stream,
        "Usage:\n"
        "  vitte [command] [options] <input.vit>\n\n"
        "Commands:\n");
    (void)vitte_cli_command_specs(&command_count);
    for (index = 0u; index < command_count; ++index) {
        (void)fprintf(stream, "  %-12s %s\n",
            vitte_cli_commands[index].name,
            vitte_cli_commands[index].summary);
    }
    (void)fprintf(stream,
        "\nThe default command is 'compile'.\n"
        "Configuration precedence: defaults < project config < environment < CLI.\n"
        "Project files: vitte.toml, .vitte/config; explicit file: VITTE_CONFIG_FILE.\n"
        "Environment variables use VITTE_<KEY>, for example VITTE_MAX_ERRORS.\n"
        "VITTE_MODULE_PATH adds colon-separated module search roots.\n\n"
        "Options:\n");
    (void)vitte_cli_option_specs(&option_count);
    for (index = 0u; index < option_count; ++index) {
        const vitte_cli_option_spec_t *option = &vitte_cli_options[index];
        (void)fprintf(stream, "  %-25s %s\n", option->spelling,
            option->summary);
    }
    (void)fprintf(stream,
        "\nExamples:\n"
        "  vitte hello.vit\n"
        "  vitte compile hello.vit -o hello\n"
        "  vitte check hello.vit --diagnostic=json\n"
        "  vitte lex hello.vit\n"
        "  vitte parse hello.vit --emit-ast\n"
        "  vitte run hello.vit -- arg1 arg2\n"
        "  vitte explain E0501\n"
        "  vitte completion bash > vitte.bash\n");
    return !ferror(stream);
}

bool
vitte_cli_render_completion(FILE *stream, const char *shell)
{
    size_t command_count;
    size_t option_count;
    size_t index;

    if (stream == NULL || shell == NULL) {
        return false;
    }
    (void)vitte_cli_command_specs(&command_count);
    (void)vitte_cli_option_specs(&option_count);

    if (strcmp(shell, "bash") == 0) {
        (void)fprintf(stream,
            "_vitte() {\n"
            "  local cur; cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
            "  COMPREPLY=( $(compgen -W \"");
        for (index = 0u; index < command_count; ++index) {
            (void)fprintf(stream, "%s ", vitte_cli_commands[index].name);
        }
        for (index = 0u; index < option_count; ++index) {
            vitte_cli_render_completion_options(
                stream, vitte_cli_options[index].spelling, false);
        }
        (void)fprintf(stream,
            "\" -- \"$cur\" ) );\n"
            "}\n"
            "complete -F _vitte vitte\n");
    } else if (strcmp(shell, "zsh") == 0) {
        (void)fprintf(stream, "#compdef vitte\n_arguments '1:command:(");
        for (index = 0u; index < command_count; ++index) {
            (void)fprintf(stream, "%s ", vitte_cli_commands[index].name);
        }
        (void)fprintf(stream, ")'");
        for (index = 0u; index < option_count; ++index) {
            size_t alias;
            char option[64];
            for (alias = 0u; alias < 8u; ++alias) {
                if (!vitte_cli_completion_option_name(
                        vitte_cli_options[index].spelling, alias,
                        option, sizeof(option))) {
                    break;
                }
                (void)fprintf(stream, " '%s[compiler option]'", option);
            }
        }
        (void)fprintf(stream, " '*:file:_files'\n");
    } else if (strcmp(shell, "fish") == 0) {
        for (index = 0u; index < command_count; ++index) {
            (void)fprintf(stream,
                "complete -c vitte -n '__fish_use_subcommand' -a '%s' -d '%s'\n",
                vitte_cli_commands[index].name,
                vitte_cli_commands[index].summary);
        }
        for (index = 0u; index < option_count; ++index) {
            vitte_cli_render_completion_options(
                stream, vitte_cli_options[index].spelling, true);
        }
    } else {
        return false;
    }
    return !ferror(stream);
}
