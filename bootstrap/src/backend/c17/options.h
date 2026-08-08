#ifndef VITTE_BOOTSTRAP_BACKEND_C17_OPTIONS_H
#define VITTE_BOOTSTRAP_BACKEND_C17_OPTIONS_H

#include <stdbool.h>
#include <stddef.h>

#include "../../api/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vitte_c17_newline {
    VITTE_C17_NEWLINE_LF = 0,
    VITTE_C17_NEWLINE_CRLF
} vitte_c17_newline_t;

typedef struct vitte_c17_options {
    const char *source_name;
    const char *output_path;
    size_t indent_width;
    vitte_c17_newline_t newline;
    bool emit_includes;
    bool emit_main_wrapper;
    bool emit_debug_comments;
} vitte_c17_options_t;

void vitte_c17_options_init(vitte_c17_options_t *options);
vitte_status_t vitte_c17_options_validate(const vitte_c17_options_t *options, vitte_error_t *error);
const char *vitte_c17_options_newline_text(vitte_c17_newline_t newline);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_BACKEND_C17_OPTIONS_H */
