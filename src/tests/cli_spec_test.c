#include "cli/command_spec.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static size_t
read_stream(FILE *stream, char *buffer, size_t capacity)
{
    size_t length;

    assert(stream != NULL);
    assert(buffer != NULL);
    assert(capacity > 0u);
    assert(fflush(stream) == 0);
    assert(fseek(stream, 0L, SEEK_SET) == 0);
    length = fread(buffer, 1u, capacity - 1u, stream);
    assert(!ferror(stream));
    buffer[length] = '\0';
    return length;
}

static void
test_table(void)
{
    static const char *const commands[] = {
        "compile", "check", "run", "lex", "parse", "explain",
        "completion", "help", "version"
    };
    size_t count;
    size_t index;
    const vitte_cli_command_spec_t *specs = vitte_cli_command_specs(&count);

    assert(specs != NULL);
    assert(count == sizeof(commands) / sizeof(commands[0]));
    for (index = 0u; index < count; ++index) {
        size_t found;
        assert(strcmp(specs[index].name, commands[index]) == 0);
        assert(vitte_cli_command_is_known(commands[index]));
        assert(vitte_cli_command_index(commands[index], &found));
        assert(found == index);
    }
}

static void
test_rendering(void)
{
    const char *const shells[] = {"bash", "zsh", "fish"};
    size_t index;
    char output[8192];
    FILE *stream = tmpfile();

    assert(stream != NULL);
    assert(vitte_cli_render_help(stream));
    (void)read_stream(stream, output, sizeof(output));
    assert(strstr(output, "compile") != NULL);
    assert(strstr(output, "explain") != NULL);
    assert(strstr(output, "--diagnostic=FMT") != NULL);
    assert(strstr(output, "defaults < project config < environment < CLI") != NULL);

    for (index = 0u; index < sizeof(shells) / sizeof(shells[0]); ++index) {
        assert(fclose(stream) == 0);
        stream = tmpfile();
        assert(stream != NULL);
        assert(vitte_cli_render_completion(stream, shells[index]));
        (void)read_stream(stream, output, sizeof(output));
        assert(strstr(output, "compile") != NULL);
        if (strcmp(shells[index], "fish") == 0) {
            assert(strstr(output, "-l 'help'") != NULL);
            assert(strstr(output, "-s 'h'") != NULL);
            assert(strstr(output, "-l 'output'") != NULL);
            assert(strstr(output, "-s 'o'") != NULL);
        } else {
            assert(strstr(output, "--help") != NULL);
            assert(strstr(output, "-h") != NULL);
            assert(strstr(output, "--output") != NULL);
            assert(strstr(output, "-o") != NULL);
        }
    }
    assert(fclose(stream) == 0);
    stream = tmpfile();
    assert(stream != NULL);
    assert(!vitte_cli_render_completion(stream, "powershell"));
    (void)fclose(stream);
}

static void
test_option_lookup(void)
{
    struct option_case {
        const char *argument;
        vitte_cli_option_id_t id;
    } cases[] = {
        {"-h", VITTE_CLI_OPTION_ID_HELP},
        {"--help", VITTE_CLI_OPTION_ID_HELP},
        {"--debug", VITTE_CLI_OPTION_ID_DEBUG_INFO},
        {"--debug-info", VITTE_CLI_OPTION_ID_DEBUG_INFO},
        {"--diagnostic=json", VITTE_CLI_OPTION_ID_DIAGNOSTIC},
        {"-o", VITTE_CLI_OPTION_ID_OUTPUT},
        {"--", VITTE_CLI_OPTION_ID_FORWARD_ARGS}
    };
    size_t index;

    for (index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        vitte_cli_option_id_t id;
        assert(vitte_cli_option_id_for_argument(cases[index].argument, &id));
        assert(id == cases[index].id);
    }
    {
        const char *value = NULL;
        assert(vitte_cli_option_value_for_argument(
            "--diagnostic=json", VITTE_CLI_OPTION_ID_DIAGNOSTIC, &value));
        assert(strcmp(value, "json") == 0);
        assert(!vitte_cli_option_value_for_argument(
            "--diagnostic", VITTE_CLI_OPTION_ID_DIAGNOSTIC, &value));
    }
    {
        vitte_cli_option_id_t id;
        assert(!vitte_cli_option_id_for_argument("--diagnostic", &id));
        assert(!vitte_cli_option_id_for_argument("--unknown", &id));
    }
}

int
main(void)
{
    test_table();
    test_option_lookup();
    test_rendering();
    puts("CLI command table, help and completion parity passed.");
    return 0;
}
