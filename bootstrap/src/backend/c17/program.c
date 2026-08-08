#include "program.h"

#include <string.h>

#include "module.h"
#include "translation_unit.h"

static void vitte_c17_program_set_error(
    vitte_c17_program_t *program,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (program != NULL) {
        vitte_error_set_details(&program->last_error, status, code, message, details);
    }
}

void vitte_c17_program_init(
    vitte_c17_program_t *program,
    const vitte_ast_t *ast,
    const vitte_c17_options_t *options
) {
    if (program == NULL) {
        return;
    }

    memset(program, 0, sizeof(*program));
    program->ast = ast;
    if (options != NULL) {
        program->options = *options;
    } else {
        vitte_c17_options_init(&program->options);
    }
    vitte_error_init(&program->last_error);
}

const vitte_error_t *vitte_c17_program_last_error(const vitte_c17_program_t *program) {
    return program != NULL ? &program->last_error : vitte_error_last();
}

vitte_status_t vitte_c17_program_emit(vitte_c17_program_t *program, vitte_c17_writer_t *writer) {
    vitte_c17_translation_unit_t unit;
    vitte_c17_module_t module;
    vitte_status_t status;

    if (program == NULL || writer == NULL) {
        vitte_c17_program_set_error(program, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_PROGRAM", "missing C17 program or writer", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (program->ast == NULL || !vitte_ast_is_initialized(program->ast) || program->ast->root == NULL) {
        vitte_c17_program_set_error(program, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_C17_E_AST", "C17 backend requires an initialized AST with a root module", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    status = vitte_c17_options_validate(&program->options, &program->last_error);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    vitte_c17_translation_unit_init(&unit, &program->options);
    vitte_c17_module_init(&module, program->ast->root, &unit);
    status = vitte_c17_module_emit(&module, writer);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&program->last_error, vitte_c17_module_last_error(&module));
        return status;
    }

    program->module_count = 1u;
    return VITTE_STATUS_OK;
}
