#include "module.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "naming.h"

static void vitte_c17_module_set_error(
    vitte_c17_module_t *module,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (module != NULL) {
        vitte_error_set_details(&module->last_error, status, code, message, details);
    }
}

void vitte_c17_module_init_ir(
    vitte_c17_module_t *module,
    const vitte_ir_module_t *ir_module,
    vitte_c17_translation_unit_t *unit
) {
    if (module == NULL) {
        return;
    }

    memset(module, 0, sizeof(*module));
    module->ir_module = ir_module;
    module->unit = unit;
    vitte_error_init(&module->last_error);
}

const vitte_error_t *vitte_c17_module_last_error(const vitte_c17_module_t *module) {
    return module != NULL ? &module->last_error : vitte_error_last();
}

static vitte_status_t vitte_c17_emit_c_string(vitte_c17_writer_t *writer, const char *value) {
    const unsigned char *cursor;
    vitte_status_t status;

    if (value == NULL) {
        value = "";
    }

    status = vitte_c17_write_char(writer, '"');
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    for (cursor = (const unsigned char *)value; *cursor != '\0'; cursor++) {
        switch (*cursor) {
            case '\n':
                status = vitte_c17_write_string(writer, "\\n");
                break;
            case '\r':
                status = vitte_c17_write_string(writer, "\\r");
                break;
            case '\t':
                status = vitte_c17_write_string(writer, "\\t");
                break;
            case '"':
                status = vitte_c17_write_string(writer, "\\\"");
                break;
            case '\\':
                status = vitte_c17_write_string(writer, "\\\\");
                break;
            default:
                if (isprint(*cursor) != 0) {
                    status = vitte_c17_write_char(writer, (char)*cursor);
                } else {
                    status = vitte_c17_write_format(writer, "\\x%02X", (unsigned int)*cursor);
                }
                break;
        }
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    return vitte_c17_write_char(writer, '"');
}

static bool vitte_c17_is_main_name(const char *name) {
    return name != NULL && strcmp(name, "main") == 0;
}

static vitte_status_t vitte_c17_make_symbol_name(
    vitte_c17_module_t *module,
    const char *prefix,
    const char *base,
    uint32_t id,
    char *output,
    size_t output_capacity
) {
    char sanitized[128];
    int written;
    vitte_status_t status;

    if (output == NULL || output_capacity == 0u) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_NAME", "missing C17 symbol output buffer", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_c17_sanitize_identifier(
        base != NULL && base[0] != '\0' ? base : "value",
        sanitized,
        sizeof(sanitized),
        &module->last_error
    );
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    written = snprintf(output, output_capacity, "%s%s_%" PRIu32, prefix, sanitized, id);
    if (written < 0 || (size_t)written >= output_capacity) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_NAME", "C17 symbol name buffer is too small", base);
        return VITTE_STATUS_ERROR_BACKEND;
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_c17_make_block_label(
    vitte_c17_module_t *module,
    const vitte_ir_block_t *block,
    char *output,
    size_t output_capacity
) {
    return vitte_c17_make_symbol_name(module, "vitte_block_", block != NULL ? block->name : "block", block != NULL ? block->id : 0u, output, output_capacity);
}

static vitte_status_t vitte_c17_make_value_name(
    vitte_c17_module_t *module,
    const vitte_ir_value_t *value,
    char *output,
    size_t output_capacity
) {
    if (value == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_VALUE", "missing IR value", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    switch (value->kind) {
        case VITTE_IR_VALUE_LOCAL:
            return vitte_c17_make_symbol_name(module, "vitte_local_", value->name, value->id, output, output_capacity);
        case VITTE_IR_VALUE_CONST_INT:
        case VITTE_IR_VALUE_CONST_STRING:
        case VITTE_IR_VALUE_INSTRUCTION:
            return vitte_c17_make_symbol_name(module, "vitte_tmp_", value->name != NULL ? value->name : "tmp", value->id, output, output_capacity);
        case VITTE_IR_VALUE_FUNCTION_REF:
            if (value->as.function != NULL) {
                if (vitte_c17_is_main_name(value->as.function->name)) {
                    if (output_capacity < sizeof("main")) {
                        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_NAME", "C17 symbol name buffer is too small", value->name);
                        return VITTE_STATUS_ERROR_BACKEND;
                    }
                    memcpy(output, "main", sizeof("main"));
                    return VITTE_STATUS_OK;
                }
                return vitte_c17_make_symbol_name(module, "vitte_fn_", value->name, value->as.function->id, output, output_capacity);
            }
            return vitte_c17_sanitize_identifier(value->name, output, output_capacity, &module->last_error);
        case VITTE_IR_VALUE_PARAMETER:
            return vitte_c17_make_symbol_name(module, "vitte_param_", value->name, value->id, output, output_capacity);
        case VITTE_IR_VALUE_ERROR:
        case VITTE_IR_VALUE_COUNT:
        default:
            vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_VALUE", "unsupported IR value kind for naming", value->name);
            return VITTE_STATUS_ERROR_BACKEND;
    }
}

static vitte_status_t vitte_c17_emit_ir_type(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_type_t *type
) {
    if (type == NULL) {
        return vitte_c17_write_string(writer, "int");
    }

    switch (type->kind) {
        case VITTE_IR_TYPE_VOID:
            return vitte_c17_write_string(writer, "void");
        case VITTE_IR_TYPE_BOOL:
            return vitte_c17_write_string(writer, "bool");
        case VITTE_IR_TYPE_I32:
            return vitte_c17_write_string(writer, "int");
        case VITTE_IR_TYPE_I64:
            return vitte_c17_write_string(writer, "int64_t");
        case VITTE_IR_TYPE_USIZE:
            return vitte_c17_write_string(writer, "size_t");
        case VITTE_IR_TYPE_STRING_PTR:
            return vitte_c17_write_string(writer, "const char *");
        case VITTE_IR_TYPE_UNKNOWN:
        case VITTE_IR_TYPE_ERROR:
        case VITTE_IR_TYPE_COUNT:
        default:
            vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_TYPE", "unsupported IR type for C17 emission", type != NULL ? vitte_ir_type_name(type) : NULL);
            return VITTE_STATUS_ERROR_BACKEND;
    }
}

static vitte_status_t vitte_c17_emit_ir_value_ref(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_value_t *value
) {
    char name[128];

    if (value == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_VALUE", "missing IR value reference", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    switch (value->kind) {
        case VITTE_IR_VALUE_CONST_INT:
            return vitte_c17_write_format(writer, "%" PRId64, value->as.int_value);
        case VITTE_IR_VALUE_CONST_STRING:
            return vitte_c17_emit_c_string(writer, value->as.string_value);
        case VITTE_IR_VALUE_LOCAL:
        case VITTE_IR_VALUE_INSTRUCTION:
        case VITTE_IR_VALUE_PARAMETER:
            if (vitte_c17_make_value_name(module, value, name, sizeof(name)) != VITTE_STATUS_OK) {
                return module->last_error.status;
            }
            return vitte_c17_write_string(writer, name);
        case VITTE_IR_VALUE_FUNCTION_REF:
            if (vitte_c17_make_value_name(module, value, name, sizeof(name)) != VITTE_STATUS_OK) {
                return module->last_error.status;
            }
            return vitte_c17_write_string(writer, name);
        case VITTE_IR_VALUE_ERROR:
        case VITTE_IR_VALUE_COUNT:
        default:
            vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_VALUE", "unsupported IR value reference", value->name);
            return VITTE_STATUS_ERROR_BACKEND;
    }
}

static vitte_status_t vitte_c17_emit_statement_line_end(vitte_c17_writer_t *writer) {
    vitte_status_t status = vitte_c17_write_string(writer, ";");
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    return vitte_c17_write_newline(writer);
}

static bool vitte_c17_ir_builtin_supported(const char *name) {
    return name != NULL &&
        (strcmp(name, "print") == 0 ||
        strcmp(name, "println") == 0 ||
        strcmp(name, "eprint") == 0 ||
        strcmp(name, "eprintln") == 0 ||
        strcmp(name, "panic") == 0 ||
        strcmp(name, "assert") == 0 ||
        strcmp(name, "len") == 0);
}

static vitte_status_t vitte_c17_emit_ir_builtin_call(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_instruction_t *instruction,
    const vitte_ir_value_t *callee
) {
    const char *name = callee != NULL ? callee->name : NULL;
    const vitte_ir_value_t *argument = instruction->operand_count > 1u ? instruction->operands[1] : NULL;
    vitte_status_t status;

    if (name == NULL || !vitte_c17_ir_builtin_supported(name)) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_CALL", "unsupported builtin function for C17 IR emission", name);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    if (strcmp(name, "print") == 0) {
        status = vitte_c17_write_string(writer, "fputs(");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, argument);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, ", stdout)");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        return vitte_c17_emit_statement_line_end(writer);
    }
    if (strcmp(name, "println") == 0) {
        status = vitte_c17_write_string(writer, "puts(");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, argument);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, ")");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        return vitte_c17_emit_statement_line_end(writer);
    }
    if (strcmp(name, "eprint") == 0) {
        status = vitte_c17_write_string(writer, "fputs(");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, argument);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, ", stderr)");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        return vitte_c17_emit_statement_line_end(writer);
    }
    if (strcmp(name, "eprintln") == 0) {
        status = vitte_c17_write_string(writer, "fprintf(stderr, \"%s\\n\", ");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, argument);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, ")");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        return vitte_c17_emit_statement_line_end(writer);
    }
    if (strcmp(name, "panic") == 0) {
        status = vitte_c17_write_string(writer, "fprintf(stderr, \"%s\\n\", ");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, argument);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, ")");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_statement_line_end(writer);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, "abort()");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        return vitte_c17_emit_statement_line_end(writer);
    }
    if (strcmp(name, "assert") == 0) {
        status = vitte_c17_write_string(writer, "assert(");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, argument);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, ")");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        return vitte_c17_emit_statement_line_end(writer);
    }

    return VITTE_STATUS_ERROR_BACKEND;
}

static vitte_status_t vitte_c17_emit_ir_call(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_instruction_t *instruction
) {
    const vitte_ir_value_t *callee;
    size_t index;
    bool builtin = false;
    bool assign_result = false;
    vitte_status_t status;

    if (instruction == NULL || instruction->operand_count == 0u) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_CALL", "invalid IR call instruction", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    callee = instruction->operands[0];
    builtin = callee != NULL && callee->kind == VITTE_IR_VALUE_FUNCTION_REF && callee->as.function == NULL && vitte_c17_ir_builtin_supported(callee->name);
    assign_result = instruction->result != NULL && instruction->result->type != NULL && instruction->result->type->kind != VITTE_IR_TYPE_VOID;

    if (builtin && strcmp(callee->name, "len") == 0) {
        if (assign_result) {
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, " = ");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
        status = vitte_c17_write_string(writer, "strlen(");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, ")");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        return vitte_c17_emit_statement_line_end(writer);
    }
    if (builtin) {
        return vitte_c17_emit_ir_builtin_call(module, writer, instruction, callee);
    }

    if (assign_result) {
        status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, " = ");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    status = vitte_c17_emit_ir_value_ref(module, writer, callee);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_char(writer, '(');
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    for (index = 1u; index < instruction->operand_count; index++) {
        if (index > 1u) {
            status = vitte_c17_write_string(writer, ", ");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[index]);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    status = vitte_c17_write_char(writer, ')');
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    return vitte_c17_emit_statement_line_end(writer);
}

static vitte_status_t vitte_c17_emit_ir_instruction(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_function_t *function,
    const vitte_ir_instruction_t *instruction
) {
    char label[128];
    vitte_status_t status;

    if (instruction == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_INSTRUCTION", "missing IR instruction", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    switch (instruction->opcode) {
        case VITTE_IR_OP_CONST_INT:
            return VITTE_STATUS_OK;
        case VITTE_IR_OP_CONST_STRING:
            return VITTE_STATUS_OK;
        case VITTE_IR_OP_LOCAL:
            return VITTE_STATUS_OK;
        case VITTE_IR_OP_STORE:
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, " = ");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_IR_OP_LOAD:
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, " = ");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_IR_OP_CAST:
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, " = (");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_ir_type(module, writer, instruction->result->type);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, ")(");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_char(writer, ')');
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_IR_OP_BINARY:
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, " = (");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_format(writer, " %s ", instruction->operator_text != NULL ? instruction->operator_text : "?");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, ")");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_IR_OP_CALL:
            return vitte_c17_emit_ir_call(module, writer, instruction);
        case VITTE_IR_OP_RETURN:
            status = vitte_c17_write_string(writer, "return");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            if (instruction->operand_count > 0u) {
                status = vitte_c17_write_char(writer, ' ');
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
                status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_IR_OP_BRANCH:
            if (vitte_c17_make_block_label(module, instruction->target, label, sizeof(label)) != VITTE_STATUS_OK) {
                return module->last_error.status;
            }
            status = vitte_c17_write_string(writer, "goto ");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, label);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_IR_OP_COND_BRANCH:
            if (vitte_c17_make_block_label(module, instruction->target, label, sizeof(label)) != VITTE_STATUS_OK) {
                return module->last_error.status;
            }
            status = vitte_c17_write_string(writer, "if (");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, ") goto ");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, label);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            if (vitte_c17_make_block_label(module, instruction->else_target, label, sizeof(label)) != VITTE_STATUS_OK) {
                return module->last_error.status;
            }
            status = vitte_c17_write_string(writer, "; else goto ");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, label);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_IR_OP_UNREACHABLE:
            if (function != NULL && function->return_type != NULL && function->return_type->kind != VITTE_IR_TYPE_VOID) {
                status = vitte_c17_write_string(writer, "return 0");
            } else {
                status = vitte_c17_write_string(writer, "return");
            }
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_IR_OP_ERROR:
        case VITTE_IR_OP_COUNT:
        default:
            vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_INSTRUCTION", "unsupported IR instruction for C17 emission", vitte_ir_opcode_name(instruction->opcode));
            return VITTE_STATUS_ERROR_BACKEND;
    }
}

static bool vitte_c17_ir_instruction_needs_declaration(const vitte_ir_instruction_t *instruction) {
    return instruction != NULL &&
        instruction->opcode != VITTE_IR_OP_CONST_INT &&
        instruction->opcode != VITTE_IR_OP_CONST_STRING &&
        instruction->result != NULL &&
        instruction->result->type != NULL &&
        instruction->result->type->kind != VITTE_IR_TYPE_VOID;
}

static vitte_status_t vitte_c17_emit_ir_function_declarations(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_function_t *function
) {
    const vitte_ir_block_t *block;

    for (block = function->first_block; block != NULL; block = block->next) {
        const vitte_ir_instruction_t *instruction;
        for (instruction = block->first; instruction != NULL; instruction = instruction->next) {
            if (vitte_c17_ir_instruction_needs_declaration(instruction)) {
                vitte_status_t status = vitte_c17_emit_ir_type(module, writer, instruction->result->type);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
                status = vitte_c17_write_char(writer, ' ');
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
                status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
                status = vitte_c17_emit_statement_line_end(writer);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
        }
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_c17_emit_ir_function_signature(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_function_t *function
) {
    const vitte_ir_value_t *parameter;
    char function_name[128];
    vitte_status_t status;

    if (function == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_FUNCTION", "missing IR function", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }
    if (vitte_c17_is_main_name(function->name)) {
        memcpy(function_name, "main", sizeof("main"));
    } else if (vitte_c17_make_symbol_name(module, "vitte_fn_", function->name, function->id, function_name, sizeof(function_name)) != VITTE_STATUS_OK) {
        return module->last_error.status;
    }
    status = vitte_c17_emit_ir_type(module, writer, function->return_type);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_char(writer, ' ');
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_string(writer, function_name);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_char(writer, '(');
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    if (function->parameter_count == 0u) {
        status = vitte_c17_write_string(writer, "void");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    } else {
        for (parameter = function->first_parameter; parameter != NULL; parameter = parameter->next) {
            if (parameter != function->first_parameter) {
                status = vitte_c17_write_string(writer, ", ");
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            status = vitte_c17_emit_ir_type(module, writer, parameter->type);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_char(writer, ' ');
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_ir_value_ref(module, writer, parameter);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
    }
    return vitte_c17_write_char(writer, ')');
}

static vitte_status_t vitte_c17_emit_ir_function_prototype(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_function_t *function
) {
    vitte_status_t status = vitte_c17_emit_ir_function_signature(module, writer, function);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    module->unit->declaration_count++;
    return vitte_c17_emit_statement_line_end(writer);
}

static vitte_status_t vitte_c17_emit_ir_function_body(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_function_t *function
) {
    const vitte_ir_block_t *block;
    char entry_label[128];
    vitte_status_t status;

    status = vitte_c17_emit_ir_function_signature(module, writer, function);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_char(writer, ' ');
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_open_block(writer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    status = vitte_c17_emit_ir_function_declarations(module, writer, function);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    if (function->entry != NULL) {
        if (vitte_c17_make_block_label(module, function->entry, entry_label, sizeof(entry_label)) != VITTE_STATUS_OK) {
            return module->last_error.status;
        }
        status = vitte_c17_write_string(writer, "goto ");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, entry_label);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_statement_line_end(writer);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    for (block = function->first_block; block != NULL; block = block->next) {
        char label[128];
        const vitte_ir_instruction_t *instruction;

        if (vitte_c17_make_block_label(module, block, label, sizeof(label)) != VITTE_STATUS_OK) {
            return module->last_error.status;
        }
        status = vitte_c17_write_string(writer, label);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_string(writer, ":");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_newline(writer);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        writer->indent_level++;
        for (instruction = block->first; instruction != NULL; instruction = instruction->next) {
            status = vitte_c17_emit_ir_instruction(module, writer, function, instruction);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
        if (writer->indent_level > 0u) {
            writer->indent_level--;
        }
    }

    status = vitte_c17_write_close_block(writer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_newline(writer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    module->unit->function_count++;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_c17_emit_ir_global(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_global_t *global
) {
    char name[128];
    vitte_status_t status;

    if (global == NULL || global->initializer == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_GLOBAL", "invalid IR global for C17 emission", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }
    if (vitte_c17_make_symbol_name(module, "vitte_global_", global->name, 0u, name, sizeof(name)) != VITTE_STATUS_OK) {
        return module->last_error.status;
    }
    status = vitte_c17_write_string(writer, "const ");
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_emit_ir_type(module, writer, global->type);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_char(writer, ' ');
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_string(writer, name);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_string(writer, " = ");
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_emit_ir_value_ref(module, writer, global->initializer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    module->unit->declaration_count++;
    return vitte_c17_emit_statement_line_end(writer);
}

static vitte_status_t vitte_c17_module_emit_ir(vitte_c17_module_t *module, vitte_c17_writer_t *writer) {
    const vitte_ir_global_t *global;
    const vitte_ir_function_t *function;
    vitte_status_t status;

    if (module->ir_module == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_MODULE", "C17 backend expected IR module root", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    status = vitte_c17_translation_unit_emit_prelude(module->unit, writer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    for (global = module->ir_module->first_global; global != NULL; global = global->next) {
        status = vitte_c17_emit_ir_global(module, writer, global);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_newline(writer);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    for (function = module->ir_module->first_function; function != NULL; function = function->next) {
        status = vitte_c17_emit_ir_function_prototype(module, writer, function);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    if (module->ir_module->first_function != NULL) {
        status = vitte_c17_write_newline(writer);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    for (function = module->ir_module->first_function; function != NULL; function = function->next) {
        status = vitte_c17_emit_ir_function_body(module, writer, function);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_newline(writer);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    return VITTE_STATUS_OK;
}

vitte_status_t vitte_c17_module_emit(vitte_c17_module_t *module, vitte_c17_writer_t *writer) {
    if (module == NULL || writer == NULL || module->unit == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_MODULE", "missing C17 module, unit, or writer", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    return vitte_c17_module_emit_ir(module, writer);
}
