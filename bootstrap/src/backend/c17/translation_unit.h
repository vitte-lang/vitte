#ifndef VITTE_BOOTSTRAP_BACKEND_C17_TRANSLATION_UNIT_H
#define VITTE_BOOTSTRAP_BACKEND_C17_TRANSLATION_UNIT_H

#include <stddef.h>

#include "../../api/error.h"
#include "options.h"
#include "writer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_c17_translation_unit {
    vitte_c17_options_t options;
    size_t include_count;
    size_t declaration_count;
    size_t function_count;
    vitte_error_t last_error;
} vitte_c17_translation_unit_t;

void vitte_c17_translation_unit_init(
    vitte_c17_translation_unit_t *unit,
    const vitte_c17_options_t *options
);

void vitte_c17_translation_unit_reset(vitte_c17_translation_unit_t *unit);
const vitte_error_t *vitte_c17_translation_unit_last_error(const vitte_c17_translation_unit_t *unit);
vitte_status_t vitte_c17_translation_unit_emit_prelude(
    vitte_c17_translation_unit_t *unit,
    vitte_c17_writer_t *writer
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_BACKEND_C17_TRANSLATION_UNIT_H */
