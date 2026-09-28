#include "options.h"

#include <string.h>

void vitte_c17_options_init(vitte_c17_options_t *options) {
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->source_name = "<memory>";
    options->indent_width = 4u;
    options->newline = VITTE_C17_NEWLINE_LF;
    options->emit_includes = true;
    options->emit_main_wrapper = false;
    options->emit_debug_comments = false;
}

vitte_status_t vitte_c17_options_validate(const vitte_c17_options_t *options, vitte_error_t *error) {
    if (options == NULL) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_OPTIONS", "missing C17 backend options", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (options->indent_width > 16u) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_INDENT", "invalid C17 indentation width", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (options->newline != VITTE_C17_NEWLINE_LF && options->newline != VITTE_C17_NEWLINE_CRLF) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_NEWLINE", "invalid C17 newline mode", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (error != NULL) {
        vitte_error_reset(error);
    }
    return VITTE_STATUS_OK;
}

const char *vitte_c17_options_newline_text(vitte_c17_newline_t newline) {
    return newline == VITTE_C17_NEWLINE_CRLF ? "\r\n" : "\n";
}
