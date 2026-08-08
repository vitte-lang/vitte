#include "api.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct vitte_source_buffer {
    char *data;
    size_t length;
} vitte_source_buffer_t;

typedef struct vitte_bootstrap_program {
    int main_return;
    bool has_main_return;
} vitte_bootstrap_program_t;

static bool g_vitte_api_initialized;
static vitte_context_t g_vitte_api_context;

static bool vitte_api_has_suffix(const char *text, const char *suffix) {
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

static bool vitte_api_identifier_boundary(char value) {
    return !isalnum((unsigned char)value) && value != '_';
}

static bool vitte_api_keyword_at(
    const char *source,
    size_t length,
    size_t offset,
    const char *keyword
) {
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
    if (offset > 0u && !vitte_api_identifier_boundary(source[offset - 1u])) {
        return false;
    }
    if (offset + keyword_length < length &&
        !vitte_api_identifier_boundary(source[offset + keyword_length])) {
        return false;
    }

    return true;
}

static void vitte_api_skip_space(const char *source, size_t length, size_t *offset) {
    while (*offset < length && isspace((unsigned char)source[*offset])) {
        (*offset)++;
    }
}

static vitte_status_t vitte_api_read_file(
    vitte_context_t *context,
    const char *path,
    vitte_source_buffer_t *buffer
) {
    FILE *file;
    long end;
    size_t read_count;

    if (context == NULL || path == NULL || buffer == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    buffer->data = NULL;
    buffer->length = 0u;

    file = fopen(path, "rb");
    if (file == NULL) {
        vitte_context_set_error(
            context,
            VITTE_STATUS_ERROR_IO,
            "VITTE_API_E_OPEN",
            "cannot open input file",
            path
        );
        return VITTE_STATUS_ERROR_IO;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        vitte_context_set_error(context, VITTE_STATUS_ERROR_IO, "VITTE_API_E_SEEK", "cannot seek input file", path);
        return VITTE_STATUS_ERROR_IO;
    }

    end = ftell(file);
    if (end < 0) {
        fclose(file);
        vitte_context_set_error(context, VITTE_STATUS_ERROR_IO, "VITTE_API_E_SIZE", "cannot size input file", path);
        return VITTE_STATUS_ERROR_IO;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        vitte_context_set_error(context, VITTE_STATUS_ERROR_IO, "VITTE_API_E_REWIND", "cannot rewind input file", path);
        return VITTE_STATUS_ERROR_IO;
    }

    buffer->data = (char *)vitte_context_alloc(context, (size_t)end + 1u);
    if (buffer->data == NULL) {
        fclose(file);
        vitte_context_set_error(context, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_API_E_OOM", "out of memory reading file", path);
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    read_count = fread(buffer->data, 1u, (size_t)end, file);
    fclose(file);
    if (read_count != (size_t)end) {
        vitte_context_free(context, buffer->data);
        buffer->data = NULL;
        vitte_context_set_error(context, VITTE_STATUS_ERROR_IO, "VITTE_API_E_READ", "short read from input file", path);
        return VITTE_STATUS_ERROR_IO;
    }

    buffer->data[read_count] = '\0';
    buffer->length = read_count;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_api_write_file(
    vitte_context_t *context,
    const char *path,
    const char *data
) {
    FILE *file;
    size_t length;

    if (context == NULL || path == NULL || data == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    file = fopen(path, "wb");
    if (file == NULL) {
        vitte_context_set_error(context, VITTE_STATUS_ERROR_IO, "VITTE_API_E_WRITE_OPEN", "cannot open output file", path);
        return VITTE_STATUS_ERROR_IO;
    }

    length = strlen(data);
    if (fwrite(data, 1u, length, file) != length) {
        fclose(file);
        vitte_context_set_error(context, VITTE_STATUS_ERROR_IO, "VITTE_API_E_WRITE", "cannot write output file", path);
        return VITTE_STATUS_ERROR_IO;
    }

    fclose(file);
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_api_validate_balanced(
    vitte_context_t *context,
    const char *source,
    size_t length
) {
    int braces = 0;
    int parens = 0;
    int brackets = 0;
    bool in_string = false;
    bool escaped = false;
    size_t index;

    for (index = 0u; index < length; index++) {
        char ch = source[index];

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
            vitte_context_set_error(context, VITTE_STATUS_ERROR_PARSE, "VITTE_API_E_DELIMITER", "unbalanced delimiter", NULL);
            return VITTE_STATUS_ERROR_PARSE;
        }
    }

    if (in_string) {
        vitte_context_set_error(context, VITTE_STATUS_ERROR_PARSE, "VITTE_API_E_STRING", "unterminated string literal", NULL);
        return VITTE_STATUS_ERROR_PARSE;
    }

    if (braces != 0 || parens != 0 || brackets != 0) {
        vitte_context_set_error(context, VITTE_STATUS_ERROR_PARSE, "VITTE_API_E_DELIMITER", "unbalanced delimiters", NULL);
        return VITTE_STATUS_ERROR_PARSE;
    }

    return VITTE_STATUS_OK;
}

static const char *vitte_api_find_main_body(
    const char *source,
    size_t length,
    const char **body_end
) {
    size_t index;

    for (index = 0u; index < length; index++) {
        size_t cursor;
        int depth;

        if (!vitte_api_keyword_at(source, length, index, "proc")) {
            continue;
        }

        cursor = index + 4u;
        vitte_api_skip_space(source, length, &cursor);
        if (cursor + 4u > length || memcmp(source + cursor, "main", 4u) != 0) {
            continue;
        }
        cursor += 4u;
        if (cursor < length && !vitte_api_identifier_boundary(source[cursor])) {
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

static size_t vitte_api_count_main(const char *source, size_t length) {
    size_t index;
    size_t count = 0u;

    for (index = 0u; index < length; index++) {
        size_t cursor;

        if (!vitte_api_keyword_at(source, length, index, "proc")) {
            continue;
        }

        cursor = index + 4u;
        vitte_api_skip_space(source, length, &cursor);
        if (cursor + 4u > length || memcmp(source + cursor, "main", 4u) != 0) {
            continue;
        }
        cursor += 4u;
        if (cursor < length && !vitte_api_identifier_boundary(source[cursor])) {
            continue;
        }
        count++;
    }

    return count;
}

static bool vitte_api_parse_main_return(
    const char *body,
    const char *body_end,
    int *value
) {
    const char *cursor = body;

    while (cursor != NULL && cursor < body_end) {
        if (vitte_api_keyword_at(body, (size_t)(body_end - body), (size_t)(cursor - body), "give")) {
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

            *value = (int)parsed;
            return true;
        }
        cursor++;
    }

    return false;
}

static vitte_status_t vitte_api_analyze_source(
    vitte_context_t *context,
    const char *input_path,
    const vitte_source_buffer_t *source,
    vitte_bootstrap_program_t *program
) {
    const char *body;
    const char *body_end = NULL;
    vitte_status_t status;

    if (context == NULL || input_path == NULL || source == NULL || program == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(program, 0, sizeof(*program));

    if (!vitte_api_has_suffix(input_path, ".vit") && !vitte_api_has_suffix(input_path, ".vitl")) {
        vitte_context_set_error(context, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_API_E_EXTENSION", "expected .vit or .vitl input", input_path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_api_validate_balanced(context, source->data, source->length);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    if (vitte_api_count_main(source->data, source->length) > 1u) {
        vitte_context_set_error(context, VITTE_STATUS_ERROR_PARSE, "VITTE_API_E_DUP_MAIN", "duplicate proc main entrypoint", input_path);
        return VITTE_STATUS_ERROR_PARSE;
    }

    body = vitte_api_find_main_body(source->data, source->length, &body_end);
    if (body == NULL || body_end == NULL) {
        vitte_context_set_error(context, VITTE_STATUS_ERROR_PARSE, "VITTE_API_E_MAIN", "missing proc main entrypoint", input_path);
        return VITTE_STATUS_ERROR_PARSE;
    }

    program->has_main_return = vitte_api_parse_main_return(body, body_end, &program->main_return);
    return VITTE_STATUS_OK;
}

static char *vitte_api_make_c_source(
    vitte_context_t *context,
    const char *input_path,
    const vitte_bootstrap_program_t *program
) {
    const char *template_text =
        "/* generated by vitte bootstrap API from %s */\n"
        "#include <stdint.h>\n"
        "int main(void) {\n"
        "    return %d;\n"
        "}\n";
    int return_value = program != NULL && program->has_main_return ? program->main_return : 0;
    size_t needed = (size_t)snprintf(NULL, 0, template_text, input_path, return_value);
    char *output = (char *)vitte_context_alloc(context, needed + 1u);

    if (output == NULL) {
        return NULL;
    }

    (void)snprintf(output, needed + 1u, template_text, input_path, return_value);
    return output;
}

static char *vitte_api_sidecar_path(vitte_context_t *context, const char *output_path) {
    const char suffix[] = ".bootstrap.c";
    size_t length;
    char *path;

    if (context == NULL || output_path == NULL) {
        return NULL;
    }

    length = strlen(output_path);
    path = (char *)vitte_context_alloc(context, length + sizeof(suffix));
    if (path == NULL) {
        return NULL;
    }

    memcpy(path, output_path, length);
    memcpy(path + length, suffix, sizeof(suffix));
    return path;
}

static char *vitte_api_shell_quote(vitte_context_t *context, const char *text) {
    size_t needed = 2u;
    size_t index;
    size_t out = 0u;
    char *quoted;

    if (context == NULL || text == NULL) {
        return NULL;
    }

    for (index = 0u; text[index] != '\0'; index++) {
        needed += text[index] == '\'' ? 4u : 1u;
    }

    quoted = (char *)vitte_context_alloc(context, needed + 1u);
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

static vitte_status_t vitte_api_compile_c(
    vitte_context_t *context,
    const char *cc,
    const char *c_path,
    const char *output_path
) {
    char *q_cc = vitte_api_shell_quote(context, cc);
    char *q_c = vitte_api_shell_quote(context, c_path);
    char *q_out = vitte_api_shell_quote(context, output_path);
    char *command;
    size_t needed;
    int status;

    if (q_cc == NULL || q_c == NULL || q_out == NULL) {
        vitte_context_free(context, q_cc);
        vitte_context_free(context, q_c);
        vitte_context_free(context, q_out);
        vitte_context_set_error(context, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_API_E_OOM", "out of memory building command", NULL);
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    needed = (size_t)snprintf(NULL, 0, "%s -std=c17 -O2 %s -o %s", q_cc, q_c, q_out);
    command = (char *)vitte_context_alloc(context, needed + 1u);
    if (command == NULL) {
        vitte_context_free(context, q_cc);
        vitte_context_free(context, q_c);
        vitte_context_free(context, q_out);
        vitte_context_set_error(context, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_API_E_OOM", "out of memory building command", NULL);
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    (void)snprintf(command, needed + 1u, "%s -std=c17 -O2 %s -o %s", q_cc, q_c, q_out);
    status = system(command);

    vitte_context_free(context, command);
    vitte_context_free(context, q_cc);
    vitte_context_free(context, q_c);
    vitte_context_free(context, q_out);

    if (status != 0) {
        vitte_context_set_error(context, VITTE_STATUS_ERROR_BACKEND, "VITTE_API_E_C_BACKEND", "C17 backend command failed", c_path);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    return VITTE_STATUS_OK;
}

vitte_status_t vitte_api_init(const vitte_api_config_t *config) {
    vitte_status_t status;

    if (g_vitte_api_initialized) {
        return VITTE_STATUS_OK;
    }

    status = vitte_context_init(&g_vitte_api_context, config);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    g_vitte_api_initialized = true;
    return VITTE_STATUS_OK;
}

void vitte_api_shutdown(void) {
    if (!g_vitte_api_initialized) {
        return;
    }

    vitte_context_destroy(&g_vitte_api_context);
    g_vitte_api_initialized = false;
}

bool vitte_api_is_initialized(void) {
    return g_vitte_api_initialized;
}

vitte_status_t vitte_api_context_create(
    const vitte_api_config_t *config,
    vitte_context_t **context
) {
    return vitte_context_create(config, context);
}

void vitte_compile_options_init(vitte_compile_options_t *options) {
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->c_compiler = "cc";
    options->output_kind = VITTE_OUTPUT_CHECK;
    options->keep_intermediate_c = true;
}

void vitte_compile_result_init(vitte_compile_result_t *result) {
    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
}

vitte_status_t vitte_api_check_file(
    vitte_context_t *context,
    const char *input_path,
    vitte_compile_result_t *result
) {
    vitte_source_buffer_t source;
    vitte_bootstrap_program_t program;
    vitte_status_t status;

    if (result != NULL) {
        vitte_compile_result_init(result);
        result->input_path = input_path;
    }

    if (!vitte_context_is_initialized(context) || input_path == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_api_read_file(context, input_path, &source);
    if (status == VITTE_STATUS_OK) {
        status = vitte_api_analyze_source(context, input_path, &source, &program);
        vitte_context_free(context, source.data);
    }

    if (result != NULL) {
        result->status = status;
        result->error_count = status == VITTE_STATUS_OK ? 0u : 1u;
    }

    return status;
}

vitte_status_t vitte_api_emit_c_file(
    vitte_context_t *context,
    const char *input_path,
    const char *output_path,
    vitte_compile_result_t *result
) {
    vitte_source_buffer_t source;
    vitte_bootstrap_program_t program;
    char *c_source;
    vitte_status_t status;

    if (result != NULL) {
        vitte_compile_result_init(result);
        result->input_path = input_path;
        result->output_path = output_path;
    }

    if (!vitte_context_is_initialized(context) || input_path == NULL || output_path == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_api_read_file(context, input_path, &source);
    if (status != VITTE_STATUS_OK) {
        goto done;
    }

    status = vitte_api_analyze_source(context, input_path, &source, &program);
    vitte_context_free(context, source.data);
    if (status != VITTE_STATUS_OK) {
        goto done;
    }

    c_source = vitte_api_make_c_source(context, input_path, &program);
    if (c_source == NULL) {
        status = VITTE_STATUS_ERROR_OUT_OF_MEMORY;
        vitte_context_set_error(context, status, "VITTE_API_E_OOM", "out of memory generating C17 source", NULL);
        goto done;
    }

    status = vitte_api_write_file(context, output_path, c_source);
    vitte_context_free(context, c_source);

done:
    if (result != NULL) {
        result->status = status;
        result->error_count = status == VITTE_STATUS_OK ? 0u : 1u;
    }
    return status;
}

vitte_status_t vitte_api_build_file(
    vitte_context_t *context,
    const vitte_compile_options_t *options,
    vitte_compile_result_t *result
) {
    vitte_compile_options_t defaults;
    char *c_path = NULL;
    vitte_status_t status;

    if (options == NULL) {
        vitte_compile_options_init(&defaults);
        options = &defaults;
    }

    if (result != NULL) {
        vitte_compile_result_init(result);
        result->input_path = options->input_path;
        result->output_path = options->output_path;
    }

    if (!vitte_context_is_initialized(context) ||
        options->input_path == NULL ||
        options->output_path == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (options->output_kind == VITTE_OUTPUT_CHECK) {
        return vitte_api_check_file(context, options->input_path, result);
    }

    if (options->output_kind == VITTE_OUTPUT_C17) {
        return vitte_api_emit_c_file(context, options->input_path, options->output_path, result);
    }

    if (options->output_kind != VITTE_OUTPUT_EXECUTABLE) {
        vitte_context_set_error(context, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_API_E_OUTPUT", "unsupported output kind", NULL);
        return VITTE_STATUS_ERROR_UNSUPPORTED;
    }

    c_path = vitte_api_sidecar_path(context, options->output_path);
    if (c_path == NULL) {
        status = VITTE_STATUS_ERROR_OUT_OF_MEMORY;
        vitte_context_set_error(context, status, "VITTE_API_E_OOM", "out of memory preparing C17 sidecar", NULL);
        goto done;
    }

    status = vitte_api_emit_c_file(context, options->input_path, c_path, NULL);
    if (status == VITTE_STATUS_OK) {
        const char *cc = options->c_compiler != NULL ? options->c_compiler : "cc";
        status = vitte_api_compile_c(context, cc, c_path, options->output_path);
    }

    if (status == VITTE_STATUS_OK && !options->keep_intermediate_c) {
        (void)remove(c_path);
    }

done:
    if (result != NULL) {
        result->status = status;
        result->error_count = status == VITTE_STATUS_OK ? 0u : 1u;
    }
    vitte_context_free(context, c_path);
    return status;
}

const vitte_error_t *vitte_api_last_error(void) {
    if (g_vitte_api_initialized) {
        return vitte_context_last_error(&g_vitte_api_context);
    }

    return vitte_error_last();
}
