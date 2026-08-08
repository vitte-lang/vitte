#ifndef VITTE_BOOTSTRAP_BACKEND_C17_NAMING_H
#define VITTE_BOOTSTRAP_BACKEND_C17_NAMING_H

#include <stdbool.h>
#include <stddef.h>

#include "../../api/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_c17_symbol_name {
    char *data;
    size_t length;
    size_t capacity;
} vitte_c17_symbol_name_t;

bool vitte_c17_is_reserved_word(const char *name);
bool vitte_c17_operator_is_supported(const char *operator_text);
vitte_status_t vitte_c17_sanitize_identifier(
    const char *input,
    char *output,
    size_t output_capacity,
    vitte_error_t *error
);
vitte_status_t vitte_c17_make_internal_name(
    const char *prefix,
    const char *input,
    char *output,
    size_t output_capacity,
    vitte_error_t *error
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_BACKEND_C17_NAMING_H */
