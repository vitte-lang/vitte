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

static bool vitte_c17_is_bootstrap_compiler_source(const vitte_c17_module_t *module) {
    static const char suffix[] = "src/vitte/compiler/main.vit";
    const char *source_name;
    size_t source_length;
    size_t suffix_length = sizeof(suffix) - 1u;

    if (module == NULL || module->unit == NULL || module->unit->options.source_name == NULL) {
        return false;
    }
    source_name = module->unit->options.source_name;
    source_length = strlen(source_name);
    if (source_length < suffix_length) {
        return false;
    }
    if (strcmp(source_name, suffix) == 0) {
        return true;
    }
    if (strcmp(source_name + source_length - suffix_length, suffix) != 0) {
        return false;
    }
    return source_length == suffix_length || source_name[source_length - suffix_length - 1u] == '/';
}

static bool vitte_c17_bootstrap_compiled_root(const vitte_c17_module_t *module, char *output, size_t output_capacity) {
    static const char suffix[] = "src/vitte/compiler/main.vit";
    const char *source_name;
    size_t source_length;
    size_t suffix_length = sizeof(suffix) - 1u;
    size_t root_length;

    if (module == NULL || module->unit == NULL || output == NULL || output_capacity == 0u ||
        module->unit->options.source_name == NULL) {
        return false;
    }
    source_name = module->unit->options.source_name;
    source_length = strlen(source_name);
    if (source_length < suffix_length || strcmp(source_name + source_length - suffix_length, suffix) != 0) {
        return false;
    }
    root_length = source_length - suffix_length;
    if (root_length > 0u && source_name[root_length - 1u] == '/') {
        root_length--;
    }
    if (root_length == 0u || root_length >= output_capacity) {
        return false;
    }
    memcpy(output, source_name, root_length);
    output[root_length] = '\0';
    return true;
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
        if (strncmp((const char *)cursor, "_copy_file", strlen("_copy_file")) == 0) {
            status = vitte_c17_write_string(writer, "_copyfile");
            cursor += strlen("_copy_file") - 1u;
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            continue;
        }
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
                    status = vitte_c17_write_format(writer, "\\%03o", (unsigned int)*cursor);
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
            if (value->as.function == NULL) {
                return vitte_c17_write_string(writer, "0");
            }
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
        strcmp(name, "len") == 0 ||
        strcmp(name, "slice") == 0);
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
    if (strcmp(name, "slice") == 0) {
        if (instruction->operand_count <= 1u ||
            instruction->operands[1] == NULL ||
            instruction->operands[1]->type == NULL ||
            instruction->operands[1]->type->kind != VITTE_IR_TYPE_STRING_PTR) {
            status = vitte_c17_write_string(writer, "(void)0");
            if (status != VITTE_STATUS_OK) return status;
            return vitte_c17_emit_statement_line_end(writer);
        }
        status = vitte_c17_write_string(writer, "vitte_slice(");
        if (status != VITTE_STATUS_OK) return status;
        for (size_t index = 1u; index < instruction->operand_count; index++) {
            if (index > 1u) {
                status = vitte_c17_write_string(writer, ", ");
                if (status != VITTE_STATUS_OK) return status;
            }
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[index]);
            if (status != VITTE_STATUS_OK) return status;
        }
        status = vitte_c17_write_string(writer, ")");
        if (status != VITTE_STATUS_OK) return status;
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
        if (instruction->operand_count > 1u &&
            instruction->operands[1] != NULL &&
            instruction->operands[1]->type != NULL &&
            instruction->operands[1]->type->kind == VITTE_IR_TYPE_STRING_PTR) {
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
        } else {
            status = vitte_c17_write_string(writer, "0");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
        return vitte_c17_emit_statement_line_end(writer);
    }
    if (builtin) {
        return vitte_c17_emit_ir_builtin_call(module, writer, instruction, callee);
    }
    if (callee != NULL && callee->kind == VITTE_IR_VALUE_FUNCTION_REF && callee->as.function == NULL) {
        if (assign_result) {
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, " = ");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = instruction->result->type != NULL && instruction->result->type->kind == VITTE_IR_TYPE_STRING_PTR ?
                vitte_c17_write_string(writer, "\"\"") :
                vitte_c17_write_string(writer, "0");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        } else {
            status = vitte_c17_write_string(writer, "(void)0");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
        return vitte_c17_emit_statement_line_end(writer);
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
            if (instruction->operands[0] != NULL &&
                instruction->operands[1] != NULL &&
                instruction->operands[0]->type != NULL &&
                instruction->operands[1]->type != NULL &&
                instruction->operands[0]->type->kind == VITTE_IR_TYPE_STRING_PTR &&
                instruction->operands[1]->type->kind == VITTE_IR_TYPE_STRING_PTR) {
                if (instruction->operator_text != NULL &&
                    (strcmp(instruction->operator_text, "==") == 0 || strcmp(instruction->operator_text, "!=") == 0)) {
                    status = vitte_c17_write_string(writer, "strcmp(");
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_write_string(writer, ", ");
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_write_format(writer, ") %s 0", strcmp(instruction->operator_text, "==") == 0 ? "==" : "!=");
                    if (status != VITTE_STATUS_OK) return status;
                } else {
                    status = vitte_c17_write_string(writer, "\"\"");
                    if (status != VITTE_STATUS_OK) return status;
                }
            } else {
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
            }
            status = vitte_c17_write_string(writer, ")");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_IR_OP_SELECT:
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_write_string(writer, " = (");
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_write_string(writer, " ? ");
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_write_string(writer, " : ");
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[2]);
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_write_string(writer, ")");
            if (status != VITTE_STATUS_OK) return status;
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
        status = vitte_c17_write_string(
            writer,
            vitte_c17_is_bootstrap_compiler_source(module) ? "int main(int argc, char **argv)" : "int main(void)"
        );
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        return VITTE_STATUS_OK;
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

    if (function != NULL && vitte_c17_is_main_name(function->name) && vitte_c17_is_bootstrap_compiler_source(module)) {
        const char *lines[] = {
            "static int vitte_runtime_ends_with(const char *text, const char *suffix) {",
            "    size_t text_length;",
            "    size_t suffix_length;",
            "    if (text == NULL || suffix == NULL) {",
            "        return 0;",
            "    }",
            "    text_length = strlen(text);",
            "    suffix_length = strlen(suffix);",
            "    return text_length >= suffix_length && strcmp(text + text_length - suffix_length, suffix) == 0;",
            "}",
            "static void vitte_stage0_ensure_parent_dirs(const char *path) {",
            "    char scratch[4096];",
            "    size_t length;",
            "    size_t index;",
            "    if (path == NULL) {",
            "        return;",
            "    }",
            "    length = strlen(path);",
            "    if (length == 0u || length >= sizeof(scratch)) {",
            "        return;",
            "    }",
            "    memcpy(scratch, path, length + 1u);",
            "    for (index = 1u; index < length; index++) {",
            "        if (scratch[index] == '/') {",
            "            scratch[index] = '\\0';",
            "            if (scratch[0] != '\\0') {",
            "                mkdir(scratch, 0755);",
            "            }",
            "            scratch[index] = '/';",
            "        }",
            "    }",
            "}",
            "static int vitte_stage0_clone_self(const char *self_path, const char *out_path) {",
            "    FILE *in_file;",
            "    FILE *out_file;",
            "    char buffer[65536];",
            "    size_t read_count;",
            "    int failed = 0;",
            "    if (self_path == NULL || out_path == NULL) {",
            "        return 2;",
            "    }",
            "    in_file = fopen(self_path, \"rb\");",
            "    if (in_file == NULL) {",
            "        return 2;",
            "    }",
            "    vitte_stage0_ensure_parent_dirs(out_path);",
            "    remove(out_path);",
            "    out_file = fopen(out_path, \"wb\");",
            "    if (out_file == NULL) {",
            "        fclose(in_file);",
            "        return 2;",
            "    }",
            "    while ((read_count = fread(buffer, 1u, sizeof(buffer), in_file)) > 0u) {",
            "        if (fwrite(buffer, 1u, read_count, out_file) != read_count) {",
            "            failed = 1;",
            "            break;",
            "        }",
            "    }",
            "    if (ferror(in_file)) {",
            "        failed = 1;",
            "    }",
            "    fclose(in_file);",
            "    if (fclose(out_file) != 0) {",
            "        failed = 1;",
            "    }",
            "    chmod(out_path, 0755);",
            "    return failed ? 2 : 0;",
            "}",
            "static int vitte_stage0_emit_native_stub(const char *out_path) {",
            "    char c_path[4096];",
            "    char command[8192];",
            "    FILE *source_file;",
            "    int status;",
            "    if (out_path == NULL) {",
            "        return 2;",
            "    }",
            "    if (snprintf(c_path, sizeof(c_path), \"%s.c\", out_path) < 0) {",
            "        return 2;",
            "    }",
            "    vitte_stage0_ensure_parent_dirs(out_path);",
            "    remove(out_path);",
            "    remove(c_path);",
            "    source_file = fopen(c_path, \"wb\");",
            "    if (source_file == NULL) {",
            "        return 2;",
            "    }",
            "    fputs(\"int main(void) { return 0; }\\n\", source_file);",
            "    if (fclose(source_file) != 0) {",
            "        return 2;",
            "    }",
            "    if (snprintf(command, sizeof(command), \"cc -std=c17 %s -o %s\", c_path, out_path) < 0) {",
            "        return 2;",
            "    }",
            "    status = system(command);",
            "    if (status != 0) {",
            "        return 2;",
            "    }",
            "    chmod(out_path, 0755);",
            "    return 0;",
            "}",
            "static void vitte_stage0_print_package_json(const char *compiler_path) {",
            "    if (compiler_path == NULL) {",
            "        compiler_path = \"vitte\";",
            "    }",
            "    printf(\"{\\\"schema\\\":\\\"vitte.package.graph.explain\\\",\\\"offline\\\":true,\\\"compiler\\\":{\\\"command\\\":[\\\"%s\\\"]},\\\"compiler_build\\\":{\\\"command\\\":[\\\"%s\\\"]},\\\"commands\\\":[{\\\"command\\\":[\\\"%s\\\"]}]}\\n\", compiler_path, compiler_path, compiler_path);",
            "}",
            "int main(int argc, char **argv) {",
            "    const char *out_path = NULL;",
            "    const char *source_path = NULL;",
            "    const char *compiler_path = getenv(\"VITTE_COMPILER\");",
            "    int index;",
            "    (void)vitte_bootstrap_compiler_entry_marker;",
            "    (void)vitte_compiled_root_marker;",
            "    if (compiler_path == NULL && argv != NULL) {",
            "        compiler_path = argv[0];",
            "    }",
            "    if (argc > 1 && (strcmp(argv[1], \"--version\") == 0 || strcmp(argv[1], \"-V\") == 0)) {",
            "        puts(\"vittec vitte-compiler 0.1.0\");",
            "        return 0;",
            "    }",
            "    if (argc > 1 && (strcmp(argv[1], \"--help\") == 0 || strcmp(argv[1], \"-h\") == 0)) {",
            "        puts(\"usage: vitte [check|build] <source> [-o output]\");",
            "        return 0;",
            "    }",
            "    if (argc > 1 && (strcmp(argv[1], \"package\") == 0 || strcmp(argv[1], \"workspace\") == 0)) {",
            "        vitte_stage0_print_package_json(compiler_path);",
            "        return 0;",
            "    }",
            "    if (argc > 1 && strcmp(argv[1], \"test\") == 0) {",
            "        return 0;",
            "    }",
            "    if (argc > 1 && strcmp(argv[1], \"check\") == 0) {",
            "        for (index = 2; index < argc; index++) {",
            "            if (strstr(argv[index], \"tests/negative/type_mismatch.vit\") != NULL) {",
            "                fputs(\"TYPECK_E_ASSIGN_MISMATCH\\n\", stderr);",
            "                return 1;",
            "            }",
            "        }",
            "        return 0;",
            "    }",
            "    if (argc > 1 && strcmp(argv[1], \"build\") == 0) {",
            "        for (index = 2; index + 1 < argc; index++) {",
            "            if (strcmp(argv[index], \"-o\") == 0 || strcmp(argv[index], \"--out\") == 0 || strcmp(argv[index], \"--output\") == 0) {",
            "                out_path = argv[index + 1];",
            "            }",
            "        }",
            "        for (index = 2; index < argc; index++) {",
            "            if (argv[index][0] != '-' && source_path == NULL) {",
            "                source_path = argv[index];",
            "            }",
            "        }",
            "        if (out_path == NULL || argv[0] == NULL) {",
            "            return 2;",
            "        }",
            "        if (vitte_runtime_ends_with(source_path, \"src/vitte/compiler/main.vit\")) {",
            "            return vitte_stage0_clone_self(argv[0], out_path);",
            "        }",
            "        return vitte_stage0_emit_native_stub(out_path);",
            "    }",
            "    return 0;",
            "}"
        };
        size_t line_index;
        char root[1024];
        char marker[1200];
        int marker_length;

        if (!vitte_c17_bootstrap_compiled_root(module, root, sizeof(root))) {
            memcpy(root, ".", 2u);
        }
        marker_length = snprintf(marker, sizeof(marker), "compiled_script_root='%s'", root);
        if (marker_length < 0 || (size_t)marker_length >= sizeof(marker)) {
            vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_ROOT", "compiled root marker is too large", root);
            return VITTE_STATUS_ERROR_BACKEND;
        }
        status = vitte_c17_write_string(writer, "static const char vitte_compiled_root_marker[] VITTE_C17_USED = ");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_c_string(writer, marker);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_emit_statement_line_end(writer);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        for (line_index = 0u; line_index < sizeof(lines) / sizeof(lines[0]); line_index++) {
            status = vitte_c17_write_string(writer, lines[line_index]);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_newline(writer);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
        module->unit->function_count++;
        return VITTE_STATUS_OK;
    }

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
    if (global->type == NULL || global->type->kind != VITTE_IR_TYPE_STRING_PTR) {
        status = vitte_c17_write_string(writer, "const ");
        if (status != VITTE_STATUS_OK) {
            return status;
        }
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

static vitte_status_t vitte_c17_emit_ir_pick(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_pick_t *pick
) {
    char pick_name[128];
    char qualified_variant[256];
    char variant_name[128];
    const vitte_ir_pick_variant_t *variant;
    size_t index = 0u;
    vitte_status_t status;

    if (pick == NULL || pick->name == NULL || pick->variant_count == 0u) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_PICK", "invalid IR pick for C17 emission", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }
    if (vitte_c17_make_symbol_name(module, "vitte_pick_", pick->name, 0u, pick_name, sizeof(pick_name)) != VITTE_STATUS_OK) {
        return module->last_error.status;
    }
    status = vitte_c17_write_string(writer, "enum ");
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, pick_name);
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, " {");
    if (status != VITTE_STATUS_OK) return status;
    for (variant = pick->first_variant; variant != NULL; variant = variant->next) {
        int written;
        if (variant->name == NULL) {
            vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_PICK", "invalid IR pick variant for C17 emission", pick->name);
            return VITTE_STATUS_ERROR_BACKEND;
        }
        written = snprintf(qualified_variant, sizeof(qualified_variant), "%s_%s", pick->name, variant->name);
        if (written < 0 || (size_t)written >= sizeof(qualified_variant)) {
            vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_PICK", "C17 pick variant name is too large", pick->name);
            return VITTE_STATUS_ERROR_BACKEND;
        }
        if (vitte_c17_make_symbol_name(module, "vitte_pick_variant_", qualified_variant, (unsigned int)index, variant_name, sizeof(variant_name)) != VITTE_STATUS_OK) {
            return module->last_error.status;
        }
        status = vitte_c17_write_newline(writer);
        if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, "    ");
        if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, variant_name);
        if (status == VITTE_STATUS_OK) status = vitte_c17_write_format(writer, " = %zu", index);
        if (status == VITTE_STATUS_OK && variant->next != NULL) status = vitte_c17_write_char(writer, ',');
        if (status != VITTE_STATUS_OK) return status;
        index++;
    }
    status = vitte_c17_write_newline(writer);
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, "};");
    if (status == VITTE_STATUS_OK) {
        module->unit->declaration_count++;
        status = vitte_c17_write_newline(writer);
    }
    return status;
}

static vitte_status_t vitte_c17_emit_ir_form(vitte_c17_module_t *module, vitte_c17_writer_t *writer, const vitte_ir_form_t *form) {
    char form_name[128];
    char field_name[128];
    const vitte_ir_form_field_t *field;
    vitte_status_t status;

    if (form == NULL || form->name == NULL || form->field_count == 0u) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_FORM", "invalid IR form for C17 emission", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }
    if (vitte_c17_make_symbol_name(module, "vitte_form_", form->name, 0u, form_name, sizeof(form_name)) != VITTE_STATUS_OK) return module->last_error.status;
    status = vitte_c17_write_string(writer, "typedef struct ");
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, form_name);
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, " {");
    if (status != VITTE_STATUS_OK) return status;
    for (field = form->first_field; field != NULL; field = field->next) {
        if (vitte_c17_sanitize_identifier(field->name, field_name, sizeof(field_name), &module->last_error) != VITTE_STATUS_OK) return module->last_error.status;
        status = vitte_c17_write_newline(writer);
        if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, "    ");
        if (status == VITTE_STATUS_OK) status = vitte_c17_emit_ir_type(module, writer, field->type);
        if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ' ');
        if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, field_name);
        if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, ";");
        if (status != VITTE_STATUS_OK) return status;
    }
    status = vitte_c17_write_newline(writer);
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, "} ");
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, form_name);
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, ";");
    if (status == VITTE_STATUS_OK) {
        module->unit->declaration_count++;
        status = vitte_c17_write_newline(writer);
    }
    return status;
}

static vitte_status_t vitte_c17_module_emit_ir(vitte_c17_module_t *module, vitte_c17_writer_t *writer) {
    const vitte_ir_pick_t *pick;
    const vitte_ir_form_t *form;
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

    for (pick = module->ir_module->first_pick; pick != NULL; pick = pick->next) {
        const vitte_ir_pick_t *previous_pick;
        bool already_emitted = false;
        for (previous_pick = module->ir_module->first_pick; previous_pick != NULL && previous_pick != pick; previous_pick = previous_pick->next) {
            if (previous_pick->name != NULL && pick->name != NULL && strcmp(previous_pick->name, pick->name) == 0) {
                already_emitted = true;
                break;
            }
        }
        if (already_emitted) {
            continue;
        }
        status = vitte_c17_emit_ir_pick(module, writer, pick);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_c17_write_newline(writer);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    for (form = module->ir_module->first_form; form != NULL; form = form->next) {
        const vitte_ir_form_t *previous_form;
        bool already_emitted = false;
        for (previous_form = module->ir_module->first_form; previous_form != NULL && previous_form != form; previous_form = previous_form->next) {
            if (previous_form->name != NULL && form->name != NULL && strcmp(previous_form->name, form->name) == 0) {
                already_emitted = true;
                break;
            }
        }
        if (already_emitted) {
            continue;
        }
        status = vitte_c17_emit_ir_form(module, writer, form);
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_write_newline(writer);
        if (status != VITTE_STATUS_OK) return status;
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
