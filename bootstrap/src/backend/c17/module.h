#ifndef VITTE_BOOTSTRAP_BACKEND_C17_MODULE_H
#define VITTE_BOOTSTRAP_BACKEND_C17_MODULE_H

#include "../../api/error.h"
#include "../../ast/ast.h"
#include "../../ir/ir.h"
#include "translation_unit.h"
#include "writer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vitte_c17_module_input_kind {
    VITTE_C17_MODULE_INPUT_AST = 0,
    VITTE_C17_MODULE_INPUT_IR
} vitte_c17_module_input_kind_t;

typedef struct vitte_c17_module {
    vitte_c17_module_input_kind_t input_kind;
    const vitte_ast_module_t *ast_module;
    const vitte_ir_module_t *ir_module;
    vitte_c17_translation_unit_t *unit;
    vitte_error_t last_error;
} vitte_c17_module_t;

void vitte_c17_module_init_ast(
    vitte_c17_module_t *module,
    const vitte_ast_module_t *ast_module,
    vitte_c17_translation_unit_t *unit
);

void vitte_c17_module_init_ir(
    vitte_c17_module_t *module,
    const vitte_ir_module_t *ir_module,
    vitte_c17_translation_unit_t *unit
);

const vitte_error_t *vitte_c17_module_last_error(const vitte_c17_module_t *module);
vitte_status_t vitte_c17_module_emit(vitte_c17_module_t *module, vitte_c17_writer_t *writer);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_BACKEND_C17_MODULE_H */
