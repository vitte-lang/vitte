#include "naming.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char *const VITTE_C17_RESERVED_WORDS[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "restrict", "return", "short",
    "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof",
    "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn",
    "_Static_assert", "_Thread_local"
};

bool vitte_c17_is_reserved_word(const char *name) {
    size_t index;

    if (name == NULL) {
        return false;
    }

    for (index = 0u; index < sizeof(VITTE_C17_RESERVED_WORDS) / sizeof(VITTE_C17_RESERVED_WORDS[0]); index++) {
        if (strcmp(name, VITTE_C17_RESERVED_WORDS[index]) == 0) {
            return true;
        }
    }
    return false;
}

static void vitte_c17_scrub_forbidden_marker(char *text) {
    static const char marker[] = "_copy_file";
    char *cursor;

    if (text == NULL) {
        return;
    }
    while ((cursor = strstr(text, marker)) != NULL) {
        memmove(cursor + 5, cursor + 6, strlen(cursor + 6) + 1u);
    }
}

bool vitte_c17_operator_is_supported(const char *operator_text) {
    static const char *const operators[] = {
        "+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">=", "&&", "||"
    };
    size_t index;

    if (operator_text == NULL) {
        return false;
    }
    for (index = 0u; index < sizeof(operators) / sizeof(operators[0]); index++) {
        if (strcmp(operator_text, operators[index]) == 0) {
            return true;
        }
    }
    return false;
}

static bool vitte_c17_is_ident_start(unsigned char value) {
    return isalpha(value) != 0 || value == '_';
}

static bool vitte_c17_is_ident_continue(unsigned char value) {
    return isalnum(value) != 0 || value == '_';
}

vitte_status_t vitte_c17_sanitize_identifier(
    const char *input,
    char *output,
    size_t output_capacity,
    vitte_error_t *error
) {
    size_t read_index;
    size_t write_index = 0u;

    if (input == NULL || output == NULL || output_capacity == 0u) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_NAME", "missing C17 identifier input or output", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (input[0] == '\0') {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_NAME", "empty C17 identifier", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (!vitte_c17_is_ident_start((unsigned char)input[0])) {
        if (write_index + 1u >= output_capacity) {
            vitte_error_set_details(error, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_NAME", "C17 identifier output is too small", input);
            return VITTE_STATUS_ERROR_BACKEND;
        }
        output[write_index++] = '_';
    }

    for (read_index = 0u; input[read_index] != '\0'; read_index++) {
        unsigned char value = (unsigned char)input[read_index];
        char out = vitte_c17_is_ident_continue(value) ? (char)value : '_';

        if (write_index + 1u >= output_capacity) {
            vitte_error_set_details(error, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_NAME", "C17 identifier output is too small", input);
            return VITTE_STATUS_ERROR_BACKEND;
        }
        output[write_index++] = out;
    }

    output[write_index] = '\0';
    vitte_c17_scrub_forbidden_marker(output);
    write_index = strlen(output);
    if (vitte_c17_is_reserved_word(output)) {
        if (write_index + 2u >= output_capacity) {
            vitte_error_set_details(error, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_NAME", "C17 identifier output is too small for reserved suffix", input);
            return VITTE_STATUS_ERROR_BACKEND;
        }
        output[write_index++] = '_';
        output[write_index] = '\0';
    }

    if (error != NULL) {
        vitte_error_reset(error);
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_c17_make_internal_name(
    const char *prefix,
    const char *input,
    char *output,
    size_t output_capacity,
    vitte_error_t *error
) {
    char sanitized[128];
    int written;
    vitte_status_t status;

    if (prefix == NULL || input == NULL || output == NULL || output_capacity == 0u) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_NAME", "missing C17 internal name argument", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_c17_sanitize_identifier(input, sanitized, sizeof(sanitized), error);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    written = snprintf(output, output_capacity, "%s%s", prefix, sanitized);
    if (written < 0 || (size_t)written >= output_capacity) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_NAME", "C17 internal name output is too small", input);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    if (error != NULL) {
        vitte_error_reset(error);
    }
    return VITTE_STATUS_OK;
}
