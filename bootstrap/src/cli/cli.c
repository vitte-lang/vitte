#include "cli.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VITTE_CLI_USAGE "usage: vitte-bootstrap <check|build|emit-c|run> <input.vit> [-o output] [--cc cc] [--keep-c]\n"

typedef struct vitte_cli_buffer {
    char *data;
    size_t length;
} vitte_cli_buffer_t;

typedef struct vitte_cli_program {
    int main_return;
    bool has_main_return;
} vitte_cli_program_t;

static bool vitte_cli_streq(const char *left, const char *right) {
    return left != NULL && right != NULL && strcmp(left, right) == 0;
}

static bool vitte_cli_has_suffix(const char *text, const char *suffix) {
    size_t text_length;
    size_t suffix_length;

    if (text == NULL || suffix == NULL) {
        return false;
    }

    text_length = strlen(text);
    suffix_length = strlen(suffix);
    return text_length >= suffix_length &&
        strcmp(text + text_length - suffix_length, suffix) == 0;
}

static char *vitte_cli_strdup_range(const char *begin, const char *end) {
    size_t length;
    char *copy;

    if (begin == NULL || end == NULL || end < begin) {
        return NULL;
    }

    length = (size_t)(end - begin);
    copy = (char *)malloc(length + 1u);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, begin, length);
    copy[length] = '\0';
    return copy;
}

static char *vitte_cli_strdup_cstr(const char *text) {
    return text != NULL ? vitte_cli_strdup_range(text, text + strlen(text)) : NULL;
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

static bool vitte_cli_read_file(const char *path, vitte_cli_buffer_t *buffer) {
    FILE *file;
    long end;
    size_t read_count;

    if (path == NULL || buffer == NULL) {
        return false;
    }

    buffer->data = NULL;
    buffer->length = 0u;
    file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "vitte-bootstrap: cannot open %s: %s\n", path, strerror(errno));
        return false;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "vitte-bootstrap: cannot seek %s\n", path);
        fclose(file);
        return false;
    }
    end = ftell(file);
    if (end < 0) {
        fprintf(stderr, "vitte-bootstrap: cannot size %s\n", path);
        fclose(file);
        return false;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "vitte-bootstrap: cannot rewind %s\n", path);
        fclose(file);
        return false;
    }

    buffer->data = (char *)malloc((size_t)end + 1u);
    if (buffer->data == NULL) {
        fprintf(stderr, "vitte-bootstrap: out of memory reading %s\n", path);
        fclose(file);
        return false;
    }

    read_count = fread(buffer->data, 1u, (size_t)end, file);
    if (read_count != (size_t)end) {
        fprintf(stderr, "vitte-bootstrap: short read from %s\n", path);
        free(buffer->data);
        buffer->data = NULL;
        fclose(file);
        return false;
    }
    buffer->data[read_count] = '\0';
    buffer->length = read_count;
    fclose(file);
    return true;
}

static bool vitte_cli_write_file(const char *path, const char *data) {
    FILE *file;
    size_t length;

    if (path == NULL || data == NULL) {
        return false;
    }

    file = fopen(path, "wb");
    if (file == NULL) {
        fprintf(stderr, "vitte-bootstrap: cannot write %s: %s\n", path, strerror(errno));
        return false;
    }

    length = strlen(data);
    if (fwrite(data, 1u, length, file) != length) {
        fprintf(stderr, "vitte-bootstrap: write failed for %s\n", path);
        fclose(file);
        return false;
    }
    if (fclose(file) != 0) {
        fprintf(stderr, "vitte-bootstrap: close failed for %s\n", path);
        return false;
    }
    return true;
}

static bool vitte_cli_identifier_boundary(char value) {
    return !isalnum((unsigned char)value) && value != '_';
}

static bool vitte_cli_keyword_at(const char *source, size_t length, size_t offset, const char *keyword) {
    size_t keyword_length;

    if (source == NULL || keyword == NULL) {
        return false;
    }

    keyword_length = strlen(keyword);
    if (offset + keyword_length > length ||
        memcmp(source + offset, keyword, keyword_length) != 0) {
        return false;
    }
    if (offset > 0u && !vitte_cli_identifier_boundary(source[offset - 1u])) {
        return false;
    }
    if (offset + keyword_length < length &&
        !vitte_cli_identifier_boundary(source[offset + keyword_length])) {
        return false;
    }
    return true;
}

static void vitte_cli_skip_space(const char *source, size_t length, size_t *offset) {
    while (*offset < length && isspace((unsigned char)source[*offset])) {
        (*offset)++;
    }
}

static bool vitte_cli_validate_balanced(const char *source, size_t length) {
    int braces = 0;
    int parens = 0;
    int brackets = 0;
    bool in_string = false;
    bool escaped = false;
    size_t line = 1u;
    size_t index;

    for (index = 0u; index < length; index++) {
        char ch = source[index];

        if (ch == '\n') {
            line++;
        }
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_string = false;
            }
            continue;
        }
        if (ch == '/' && index + 1u < length && source[index + 1u] == '/') {
            while (index < length && source[index] != '\n') {
                index++;
            }
            if (index < length) {
                line++;
            }
            continue;
        }
        if (ch == '"') {
            in_string = true;
        } else if (ch == '{') {
            braces++;
        } else if (ch == '}') {
            braces--;
        } else if (ch == '(') {
            parens++;
        } else if (ch == ')') {
            parens--;
        } else if (ch == '[') {
            brackets++;
        } else if (ch == ']') {
            brackets--;
        }
        if (braces < 0 || parens < 0 || brackets < 0) {
            fprintf(stderr, "vitte-bootstrap: unbalanced delimiter near line %lu\n", (unsigned long)line);
            return false;
        }
    }
    if (in_string) {
        fputs("vitte-bootstrap: unterminated string literal\n", stderr);
        return false;
    }
    if (braces != 0 || parens != 0 || brackets != 0) {
        fputs("vitte-bootstrap: unbalanced delimiters\n", stderr);
        return false;
    }
    return true;
}

static const char *vitte_cli_find_main_body(const char *source, size_t length, const char **body_end) {
    size_t index;

    for (index = 0u; index < length; index++) {
        size_t cursor;
        int depth;

        if (!vitte_cli_keyword_at(source, length, index, "proc")) {
            continue;
        }
        cursor = index + 4u;
        vitte_cli_skip_space(source, length, &cursor);
        if (cursor + 4u > length || memcmp(source + cursor, "main", 4u) != 0) {
            continue;
        }
        cursor += 4u;
        if (cursor < length && !vitte_cli_identifier_boundary(source[cursor])) {
            continue;
        }
        while (cursor < length && source[cursor] != '{') {
            cursor++;
        }
        if (cursor >= length) {
            return NULL;
        }
        cursor++;
        depth = 1;
        index = cursor;
        while (index < length && depth > 0) {
            if (source[index] == '{') {
                depth++;
            } else if (source[index] == '}') {
                depth--;
            }
            index++;
        }
        if (depth == 0) {
            *body_end = source + index - 1u;
            return source + cursor;
        }
    }
    return NULL;
}

static size_t vitte_cli_count_main_entrypoints(const char *source, size_t length) {
    size_t index;
    size_t count = 0u;

    for (index = 0u; index < length; index++) {
        size_t cursor;

        if (!vitte_cli_keyword_at(source, length, index, "proc")) {
            continue;
        }
        cursor = index + 4u;
        vitte_cli_skip_space(source, length, &cursor);
        if (cursor + 4u > length || memcmp(source + cursor, "main", 4u) != 0) {
            continue;
        }
        cursor += 4u;
        if (cursor < length && !vitte_cli_identifier_boundary(source[cursor])) {
            continue;
        }
        count++;
    }
    return count;
}

static bool vitte_cli_parse_integer_return(const char *body, const char *body_end, int *value) {
    const char *cursor = body;

    while (cursor != NULL && cursor < body_end) {
        if (vitte_cli_keyword_at(body, (size_t)(body_end - body), (size_t)(cursor - body), "give")) {
            char *endptr;
            long parsed;

            cursor += 4;
            while (cursor < body_end && isspace((unsigned char)*cursor)) {
                cursor++;
            }
            errno = 0;
            parsed = strtol(cursor, &endptr, 10);
            if (cursor == endptr || errno != 0 || parsed < INT_MIN || parsed > INT_MAX) {
                return false;
            }
            while (endptr < body_end && isspace((unsigned char)*endptr)) {
                endptr++;
            }
            if (endptr < body_end && *endptr == ';') {
                endptr++;
            }
            *value = (int)parsed;
            return true;
        }
        cursor++;
    }
    return false;
}

static bool vitte_cli_analyze_source(const char *path, const vitte_cli_buffer_t *source, vitte_cli_program_t *program) {
    const char *body;
    const char *body_end = NULL;

    if (source == NULL || source->data == NULL || program == NULL) {
        return false;
    }
    memset(program, 0, sizeof(*program));
    if (!vitte_cli_has_suffix(path, ".vit") && !vitte_cli_has_suffix(path, ".vitl")) {
        fprintf(stderr, "vitte-bootstrap: expected .vit or .vitl input: %s\n", path);
        return false;
    }
    if (!vitte_cli_validate_balanced(source->data, source->length)) {
        return false;
    }
    if (vitte_cli_count_main_entrypoints(source->data, source->length) > 1u) {
        fputs("vitte-bootstrap: duplicate proc main entrypoint\n", stderr);
        return false;
    }
    body = vitte_cli_find_main_body(source->data, source->length, &body_end);
    if (body == NULL || body_end == NULL) {
        fputs("vitte-bootstrap: missing proc main entrypoint\n", stderr);
        return false;
    }
    program->has_main_return = vitte_cli_parse_integer_return(body, body_end, &program->main_return);
    return true;
}

static char *vitte_cli_c_source_for_program(const char *input_path, const vitte_cli_program_t *program) {
    const char *template_text =
        "/* generated by vitte-bootstrap c17 from %s */\n"
        "#include <stdint.h>\n"
        "int main(void) {\n"
        "    return %d;\n"
        "}\n";
    size_t needed;
    char *output;
    int return_value = 0;

    if (program != NULL && program->has_main_return) {
        return_value = program->main_return;
    }
    needed = (size_t)snprintf(NULL, 0, template_text, input_path, return_value);
    output = (char *)malloc(needed + 1u);
    if (output == NULL) {
        return NULL;
    }
    (void)snprintf(output, needed + 1u, template_text, input_path, return_value);
    return output;
}

static char *vitte_cli_default_output_path(const char *input_path, const char *suffix) {
    const char *slash;
    const char *base;
    const char *dot;
    size_t stem_length;
    size_t suffix_length;
    char *path;

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

static char *vitte_cli_sidecar_c_path(const char *output_path) {
    const char *suffix = ".bootstrap.c";
    size_t output_length;
    size_t suffix_length;
    char *path;

    if (output_path == NULL) {
        return NULL;
    }
    output_length = strlen(output_path);
    suffix_length = strlen(suffix);
    path = (char *)malloc(output_length + suffix_length + 1u);
    if (path == NULL) {
        return NULL;
    }
    memcpy(path, output_path, output_length);
    memcpy(path + output_length, suffix, suffix_length);
    path[output_length + suffix_length] = '\0';
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

static bool vitte_cli_compile_c(const char *cc, const char *c_path, const char *output_path) {
    char *q_cc = vitte_cli_shell_quote(cc);
    char *q_c_path = vitte_cli_shell_quote(c_path);
    char *q_output_path = vitte_cli_shell_quote(output_path);
    char *command;
    size_t needed;
    int status;

    if (q_cc == NULL || q_c_path == NULL || q_output_path == NULL) {
        free(q_cc);
        free(q_c_path);
        free(q_output_path);
        return false;
    }
    needed = (size_t)snprintf(NULL, 0, "%s -std=c17 -O2 %s -o %s", q_cc, q_c_path, q_output_path);
    command = (char *)malloc(needed + 1u);
    if (command == NULL) {
        free(q_cc);
        free(q_c_path);
        free(q_output_path);
        return false;
    }
    (void)snprintf(command, needed + 1u, "%s -std=c17 -O2 %s -o %s", q_cc, q_c_path, q_output_path);
    status = system(command);
    free(command);
    free(q_cc);
    free(q_c_path);
    free(q_output_path);
    return status == 0;
}

static int vitte_cli_command_check(const vitte_cli_options_t *options) {
    vitte_cli_buffer_t source;
    vitte_cli_program_t program;
    bool ok;

    if (!vitte_cli_read_file(options->input_path, &source)) {
        return VITTE_CLI_EXIT_ERROR;
    }
    ok = vitte_cli_analyze_source(options->input_path, &source, &program);
    free(source.data);
    if (!ok) {
        return VITTE_CLI_EXIT_ERROR;
    }
    printf("[vitte-bootstrap] check ok: %s\n", options->input_path);
    return VITTE_CLI_EXIT_OK;
}

static int vitte_cli_command_emit_or_build(const vitte_cli_options_t *options, bool build, bool run) {
    vitte_cli_buffer_t source;
    vitte_cli_program_t program;
    char *c_source;
    char *owned_c_path = NULL;
    char *owned_output_path = NULL;
    const char *output_path;
    int result = VITTE_CLI_EXIT_OK;

    if (!vitte_cli_read_file(options->input_path, &source)) {
        return VITTE_CLI_EXIT_ERROR;
    }
    if (!vitte_cli_analyze_source(options->input_path, &source, &program)) {
        free(source.data);
        return VITTE_CLI_EXIT_ERROR;
    }
    c_source = vitte_cli_c_source_for_program(options->input_path, &program);
    free(source.data);
    if (c_source == NULL) {
        fputs("vitte-bootstrap: out of memory generating C17 source\n", stderr);
        return VITTE_CLI_EXIT_INTERNAL;
    }

    if (!build) {
        output_path = options->output_path;
        if (output_path == NULL) {
            fputs(c_source, stdout);
        } else if (!vitte_cli_write_file(output_path, c_source)) {
            result = VITTE_CLI_EXIT_ERROR;
        } else {
            printf("[vitte-bootstrap] wrote C17: %s\n", output_path);
        }
        free(c_source);
        return result;
    }

    owned_output_path = options->output_path == NULL
        ? vitte_cli_default_output_path(options->input_path, "")
        : vitte_cli_strdup_cstr(options->output_path);
    owned_c_path = vitte_cli_sidecar_c_path(owned_output_path);
    if (owned_c_path == NULL || owned_output_path == NULL) {
        free(c_source);
        free(owned_c_path);
        free(owned_output_path);
        fputs("vitte-bootstrap: out of memory preparing build paths\n", stderr);
        return VITTE_CLI_EXIT_INTERNAL;
    }

    if (!vitte_cli_write_file(owned_c_path, c_source)) {
        result = VITTE_CLI_EXIT_ERROR;
    } else if (!vitte_cli_compile_c(options->c_compiler, owned_c_path, owned_output_path)) {
        fprintf(stderr, "vitte-bootstrap: C17 backend failed for %s\n", options->input_path);
        result = VITTE_CLI_EXIT_ERROR;
    } else {
        printf("[vitte-bootstrap] built: %s\n", owned_output_path);
    }

    if (result == VITTE_CLI_EXIT_OK && run) {
        char *quoted = vitte_cli_shell_quote(owned_output_path);
        if (quoted == NULL) {
            result = VITTE_CLI_EXIT_INTERNAL;
        } else {
            result = system(quoted);
            free(quoted);
            result = result == 0 ? VITTE_CLI_EXIT_OK : VITTE_CLI_EXIT_ERROR;
        }
    }

    if (!options->keep_intermediate_c) {
        (void)remove(owned_c_path);
    }
    free(c_source);
    free(owned_c_path);
    free(owned_output_path);
    return result;
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
            return vitte_cli_command_check(options);
        case VITTE_CLI_COMMAND_EMIT_C:
            return vitte_cli_command_emit_or_build(options, false, false);
        case VITTE_CLI_COMMAND_BUILD:
            return vitte_cli_command_emit_or_build(options, true, false);
        case VITTE_CLI_COMMAND_RUN:
            return vitte_cli_command_emit_or_build(options, true, true);
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
