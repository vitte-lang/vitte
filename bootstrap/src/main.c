/*
 * Vitte C17 bootstrap compiler.
 *
 * This is intentionally small and dependency-free: it provides the first
 * native binary that can validate a minimal Vitte source and lower the current
 * integer-entrypoint subset to C17, then ask the host C compiler to produce an
 * executable.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VITTE_BOOTSTRAP_VERSION "vitte-bootstrap c17 0.1.0"
#define VITTE_BOOTSTRAP_USAGE \
    "usage: vitte-bootstrap <check|build|emit-c|run> <input.vit> [-o output] [--cc cc]\n"

typedef struct VitteBuffer {
    char *data;
    size_t length;
} VitteBuffer;

typedef struct VitteOptions {
    const char *command;
    const char *input_path;
    const char *output_path;
    const char *cc;
    bool emit_c;
} VitteOptions;

typedef struct VitteProgram {
    int main_return;
    bool has_main_return;
} VitteProgram;

static void vitte_print_help(FILE *stream) {
    fputs(VITTE_BOOTSTRAP_USAGE, stream);
    fputs("\ncommands:\n", stream);
    fputs("  check   validate a Vitte source file\n", stream);
    fputs("  emit-c  write lowered C17 source\n", stream);
    fputs("  build   build a native executable through a C17 compiler\n", stream);
    fputs("  run     build to a temporary executable and run it\n", stream);
}

static bool vitte_streq(const char *left, const char *right) {
    return left != NULL && right != NULL && strcmp(left, right) == 0;
}

static bool vitte_has_suffix(const char *text, const char *suffix) {
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

static char *vitte_strdup_range(const char *begin, const char *end) {
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

static char *vitte_strdup_cstr(const char *text) {
    if (text == NULL) {
        return NULL;
    }

    return vitte_strdup_range(text, text + strlen(text));
}

static bool vitte_read_file(const char *path, VitteBuffer *buffer) {
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

static bool vitte_write_file(const char *path, const char *data) {
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

    fclose(file);
    return true;
}

static bool vitte_is_identifier_boundary(char value) {
    return !isalnum((unsigned char)value) && value != '_';
}

static bool vitte_keyword_at(const char *source, size_t length, size_t offset, const char *keyword) {
    size_t keyword_length;

    if (source == NULL || keyword == NULL) {
        return false;
    }

    keyword_length = strlen(keyword);
    if (offset + keyword_length > length) {
        return false;
    }

    if (memcmp(source + offset, keyword, keyword_length) != 0) {
        return false;
    }

    if (offset > 0u && !vitte_is_identifier_boundary(source[offset - 1u])) {
        return false;
    }

    if (offset + keyword_length < length &&
        !vitte_is_identifier_boundary(source[offset + keyword_length])) {
        return false;
    }

    return true;
}

static void vitte_skip_space(const char *source, size_t length, size_t *offset) {
    while (*offset < length && isspace((unsigned char)source[*offset])) {
        (*offset)++;
    }
}

static bool vitte_validate_balanced(const char *source, size_t length) {
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

static const char *vitte_find_main_body(const char *source, size_t length, const char **body_end) {
    size_t index;

    for (index = 0u; index < length; index++) {
        size_t cursor;
        int depth;

        if (!vitte_keyword_at(source, length, index, "proc")) {
            continue;
        }

        cursor = index + 4u;
        vitte_skip_space(source, length, &cursor);
        if (cursor + 4u > length || memcmp(source + cursor, "main", 4u) != 0) {
            continue;
        }
        cursor += 4u;
        if (cursor < length && !vitte_is_identifier_boundary(source[cursor])) {
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

static size_t vitte_count_main_entrypoints(const char *source, size_t length) {
    size_t index;
    size_t count = 0u;

    for (index = 0u; index < length; index++) {
        size_t cursor;

        if (!vitte_keyword_at(source, length, index, "proc")) {
            continue;
        }

        cursor = index + 4u;
        vitte_skip_space(source, length, &cursor);
        if (cursor + 4u > length || memcmp(source + cursor, "main", 4u) != 0) {
            continue;
        }
        cursor += 4u;
        if (cursor < length && !vitte_is_identifier_boundary(source[cursor])) {
            continue;
        }

        count++;
    }

    return count;
}

static bool vitte_parse_integer_return(const char *body, const char *body_end, int *value) {
    const char *cursor = body;

    while (cursor != NULL && cursor < body_end) {
        if (vitte_keyword_at(body, (size_t)(body_end - body), (size_t)(cursor - body), "give")) {
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

static bool vitte_analyze_source(const char *path, const VitteBuffer *source, VitteProgram *program) {
    const char *body;
    const char *body_end = NULL;

    if (source == NULL || source->data == NULL || program == NULL) {
        return false;
    }

    memset(program, 0, sizeof(*program));

    if (!vitte_has_suffix(path, ".vit") && !vitte_has_suffix(path, ".vitl")) {
        fprintf(stderr, "vitte-bootstrap: expected .vit or .vitl input: %s\n", path);
        return false;
    }

    if (!vitte_validate_balanced(source->data, source->length)) {
        return false;
    }

    if (vitte_count_main_entrypoints(source->data, source->length) > 1u) {
        fputs("vitte-bootstrap: duplicate proc main entrypoint\n", stderr);
        return false;
    }

    body = vitte_find_main_body(source->data, source->length, &body_end);
    if (body == NULL || body_end == NULL) {
        fputs("vitte-bootstrap: missing proc main entrypoint\n", stderr);
        return false;
    }

    program->has_main_return = vitte_parse_integer_return(body, body_end, &program->main_return);
    return true;
}

static char *vitte_c_source_for_program(const char *input_path, const VitteProgram *program) {
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

static char *vitte_default_output_path(const char *input_path, const char *suffix) {
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

static char *vitte_sidecar_c_path(const char *output_path) {
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

static char *vitte_shell_quote(const char *text) {
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

static bool vitte_compile_c(const char *cc, const char *c_path, const char *output_path) {
    char *q_cc = vitte_shell_quote(cc);
    char *q_c_path = vitte_shell_quote(c_path);
    char *q_output_path = vitte_shell_quote(output_path);
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

static bool vitte_parse_options(int argc, char **argv, VitteOptions *options) {
    int index;

    memset(options, 0, sizeof(*options));
    options->cc = getenv("CC");
    if (options->cc == NULL || options->cc[0] == '\0') {
        options->cc = "cc";
    }

    if (argc <= 1) {
        options->command = "help";
        return true;
    }

    if (vitte_streq(argv[1], "--help") || vitte_streq(argv[1], "-h")) {
        options->command = "help";
        return true;
    }

    if (vitte_streq(argv[1], "--version") || vitte_streq(argv[1], "-V")) {
        options->command = "version";
        return true;
    }

    options->command = argv[1];
    index = 2;

    while (index < argc) {
        const char *argument = argv[index];

        if (vitte_streq(argument, "-o") || vitte_streq(argument, "--output")) {
            index++;
            if (index >= argc) {
                fputs("vitte-bootstrap: missing value for -o\n", stderr);
                return false;
            }
            options->output_path = argv[index++];
            continue;
        }

        if (vitte_streq(argument, "--cc")) {
            index++;
            if (index >= argc) {
                fputs("vitte-bootstrap: missing value for --cc\n", stderr);
                return false;
            }
            options->cc = argv[index++];
            continue;
        }

        if (vitte_streq(argument, "--emit-c")) {
            options->emit_c = true;
            index++;
            continue;
        }

        if (argument[0] == '-' && !vitte_streq(argument, "-")) {
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
        !vitte_streq(options->command, "help") &&
        !vitte_streq(options->command, "version")) {
        fputs("vitte-bootstrap: missing input path\n", stderr);
        return false;
    }

    return true;
}

static int vitte_command_check(const VitteOptions *options) {
    VitteBuffer source;
    VitteProgram program;
    bool ok;

    if (!vitte_read_file(options->input_path, &source)) {
        return 2;
    }

    ok = vitte_analyze_source(options->input_path, &source, &program);
    free(source.data);

    if (!ok) {
        return 2;
    }

    printf("[vitte-bootstrap] check ok: %s\n", options->input_path);
    return 0;
}

static int vitte_command_emit_or_build(const VitteOptions *options, bool build, bool run) {
    VitteBuffer source;
    VitteProgram program;
    char *c_source;
    char *owned_c_path = NULL;
    char *owned_output_path = NULL;
    const char *c_path;
    const char *output_path;
    int result = 0;

    if (!vitte_read_file(options->input_path, &source)) {
        return 2;
    }

    if (!vitte_analyze_source(options->input_path, &source, &program)) {
        free(source.data);
        return 2;
    }

    c_source = vitte_c_source_for_program(options->input_path, &program);
    free(source.data);
    if (c_source == NULL) {
        fputs("vitte-bootstrap: out of memory generating C17 source\n", stderr);
        return 3;
    }

    if (!build) {
        output_path = options->output_path;
        if (output_path == NULL) {
            fputs(c_source, stdout);
        } else if (!vitte_write_file(output_path, c_source)) {
            result = 2;
        } else {
            printf("[vitte-bootstrap] wrote C17: %s\n", output_path);
        }
        free(c_source);
        return result;
    }

    owned_output_path = options->output_path == NULL
        ? vitte_default_output_path(options->input_path, "")
        : vitte_strdup_cstr(options->output_path);
    owned_c_path = vitte_sidecar_c_path(owned_output_path);

    if (owned_c_path == NULL || owned_output_path == NULL) {
        free(c_source);
        free(owned_c_path);
        free(owned_output_path);
        fputs("vitte-bootstrap: out of memory preparing build paths\n", stderr);
        return 3;
    }

    c_path = owned_c_path;
    output_path = owned_output_path;

    if (!vitte_write_file(c_path, c_source)) {
        result = 2;
    } else if (!vitte_compile_c(options->cc, c_path, output_path)) {
        fprintf(stderr, "vitte-bootstrap: C17 backend failed for %s\n", options->input_path);
        result = 2;
    } else {
        printf("[vitte-bootstrap] built: %s\n", output_path);
    }

    if (result == 0 && run) {
        char *quoted = vitte_shell_quote(output_path);
        if (quoted == NULL) {
            result = 3;
        } else {
            result = system(quoted);
            free(quoted);
        }
    }

    free(c_source);
    free(owned_c_path);
    free(owned_output_path);
    return result == 0 ? 0 : 2;
}

int main(int argc, char **argv) {
    VitteOptions options;

    if (!vitte_parse_options(argc, argv, &options)) {
        vitte_print_help(stderr);
        return 1;
    }

    if (vitte_streq(options.command, "help")) {
        vitte_print_help(stdout);
        return 0;
    }

    if (vitte_streq(options.command, "version")) {
        puts(VITTE_BOOTSTRAP_VERSION);
        return 0;
    }

    if (vitte_streq(options.command, "check")) {
        return vitte_command_check(&options);
    }

    if (vitte_streq(options.command, "emit-c")) {
        return vitte_command_emit_or_build(&options, false, false);
    }

    if (vitte_streq(options.command, "build")) {
        return vitte_command_emit_or_build(&options, true, false);
    }

    if (vitte_streq(options.command, "run")) {
        return vitte_command_emit_or_build(&options, true, true);
    }

    fprintf(stderr, "vitte-bootstrap: unknown command: %s\n", options.command);
    vitte_print_help(stderr);
    return 1;
}
