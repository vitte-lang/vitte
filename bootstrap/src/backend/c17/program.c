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

void vitte_c17_program_init_ast(
    vitte_c17_program_t *program,
    const vitte_ast_t *ast,
    const vitte_c17_options_t *options
) {
    if (program == NULL) {
        return;
    }

    memset(program, 0, sizeof(*program));
    program->input_kind = VITTE_C17_PROGRAM_INPUT_AST;
    program->ast = ast;
    if (options != NULL) {
        program->options = *options;
    } else {
        vitte_c17_options_init(&program->options);
    }
    vitte_error_init(&program->last_error);
}

void vitte_c17_program_init_ir(
    vitte_c17_program_t *program,
    const vitte_ir_t *ir,
    const vitte_c17_options_t *options
) {
    if (program == NULL) {
        return;
    }

    memset(program, 0, sizeof(*program));
    program->input_kind = VITTE_C17_PROGRAM_INPUT_IR;
    program->ir = ir;
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

    if (program->input_kind == VITTE_C17_PROGRAM_INPUT_AST) {
        if (program->ast == NULL || !vitte_ast_is_initialized(program->ast) || program->ast->root == NULL) {
            vitte_c17_program_set_error(program, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_C17_E_AST", "C17 backend requires an initialized AST with a root module", NULL);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    } else if (program->input_kind == VITTE_C17_PROGRAM_INPUT_IR) {
        if (program->ir == NULL || !vitte_ir_is_initialized(program->ir) || program->ir->module == NULL) {
            vitte_c17_program_set_error(program, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_C17_E_IR", "C17 backend requires an initialized IR with a module", NULL);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    } else {
        vitte_c17_program_set_error(program, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_PROGRAM", "unknown C17 program input kind", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_c17_options_validate(&program->options, &program->last_error);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    vitte_c17_translation_unit_init(&unit, &program->options);
    if (program->input_kind == VITTE_C17_PROGRAM_INPUT_AST) {
        vitte_c17_module_init_ast(&module, program->ast->root, &unit);
    } else {
        vitte_c17_module_init_ir(&module, program->ir->module, &unit);
    }
    status = vitte_c17_module_emit(&module, writer);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&program->last_error, vitte_c17_module_last_error(&module));
        return status;
    }

    program->module_count = 1u;
    return VITTE_STATUS_OK;
}
