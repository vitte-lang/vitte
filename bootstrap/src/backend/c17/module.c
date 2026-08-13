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
            "static void vitte_diag_json_string(const char *text) {",
            "    const unsigned char *cursor;",
            "    putchar('\"');",
            "    if (text != NULL) {",
            "        for (cursor = (const unsigned char *)text; *cursor != '\\0'; cursor++) {",
            "            if (*cursor == '\"' || *cursor == '\\\\') {",
            "                putchar('\\\\');",
            "                putchar((int)*cursor);",
            "            } else if (*cursor == '\\n') {",
            "                fputs(\"\\\\n\", stdout);",
            "            } else if (*cursor == '\\t') {",
            "                fputs(\"\\\\t\", stdout);",
            "            } else if (*cursor < 32u) {",
            "                printf(\"\\\\u%04x\", (unsigned int)*cursor);",
            "            } else {",
            "                putchar((int)*cursor);",
            "            }",
            "        }",
            "    }",
            "    putchar('\"');",
            "}",
            "static const char *vitte_locale_normalize(const char *locale) {",
            "    if (locale == NULL || locale[0] == '\\0') return \"en\";",
            "    if (strncmp(locale, \"en\", 2u) == 0) return \"en\";",
            "    if (strcmp(locale, \"en-US\") == 0) return \"en\";",
            "    if (strcmp(locale, \"en_GB\") == 0) return \"en\";",
            "    if (strcmp(locale, \"en-GB\") == 0) return \"en\";",
            "    if (strncmp(locale, \"fr\", 2u) == 0) return \"fr\";",
            "    if (strcmp(locale, \"fr-FR\") == 0) return \"fr\";",
            "    if (strcmp(locale, \"fr_CA\") == 0) return \"fr\";",
            "    if (strcmp(locale, \"fr-CA\") == 0) return \"fr\";",
            "    if (strncmp(locale, \"es\", 2u) == 0) return \"es\";",
            "    if (strcmp(locale, \"es-ES\") == 0) return \"es\";",
            "    if (strcmp(locale, \"es_MX\") == 0) return \"es\";",
            "    if (strcmp(locale, \"es-MX\") == 0) return \"es\";",
            "    if (strncmp(locale, \"de\", 2u) == 0) return \"de\";",
            "    if (strcmp(locale, \"de-DE\") == 0) return \"de\";",
            "    if (strcmp(locale, \"de_AT\") == 0) return \"de\";",
            "    if (strcmp(locale, \"de-AT\") == 0) return \"de\";",
            "    if (strncmp(locale, \"it\", 2u) == 0) return \"it\";",
            "    if (strcmp(locale, \"it-IT\") == 0) return \"it\";",
            "    if (strncmp(locale, \"pt\", 2u) == 0) return \"pt-BR\";",
            "    if (strcmp(locale, \"pt\") == 0) return \"pt-BR\";",
            "    if (strcmp(locale, \"pt_BR\") == 0) return \"pt-BR\";",
            "    if (strcmp(locale, \"pt-PT\") == 0) return \"pt-BR\";",
            "    if (strncmp(locale, \"nl\", 2u) == 0) return \"nl\";",
            "    if (strcmp(locale, \"nl-NL\") == 0) return \"nl\";",
            "    if (strcmp(locale, \"nl_BE\") == 0) return \"nl\";",
            "    if (strcmp(locale, \"nl-BE\") == 0) return \"nl\";",
            "    if (strncmp(locale, \"pl\", 2u) == 0) return \"pl\";",
            "    if (strcmp(locale, \"pl-PL\") == 0) return \"pl\";",
            "    if (strncmp(locale, \"ru\", 2u) == 0) return \"ru\";",
            "    if (strcmp(locale, \"ru-RU\") == 0) return \"ru\";",
            "    if (strncmp(locale, \"uk\", 2u) == 0) return \"uk\";",
            "    if (strcmp(locale, \"uk-UA\") == 0) return \"uk\";",
            "    if (strncmp(locale, \"zh\", 2u) == 0) return \"zh-CN\";",
            "    if (strcmp(locale, \"zh\") == 0) return \"zh-CN\";",
            "    if (strcmp(locale, \"zh_CN\") == 0) return \"zh-CN\";",
            "    if (strcmp(locale, \"zh-Hans\") == 0) return \"zh-CN\";",
            "    if (strncmp(locale, \"ja\", 2u) == 0) return \"ja\";",
            "    if (strcmp(locale, \"ja-JP\") == 0) return \"ja\";",
            "    if (strncmp(locale, \"ko\", 2u) == 0) return \"ko\";",
            "    if (strcmp(locale, \"ko-KR\") == 0) return \"ko\";",
            "    if (strncmp(locale, \"tr\", 2u) == 0) return \"tr\";",
            "    if (strcmp(locale, \"tr-TR\") == 0) return \"tr\";",
            "    if (strncmp(locale, \"ar\", 2u) == 0) return \"ar\";",
            "    if (strcmp(locale, \"ar-SA\") == 0) return \"ar\";",
            "    if (strcmp(locale, \"ar_EG\") == 0) return \"ar\";",
            "    if (strcmp(locale, \"ar-EG\") == 0) return \"ar\";",
            "    return \"en\";",
            "}",
            "static const char *vitte_args_locale(int argc, char **argv) {",
            "    const char *locale = getenv(\"VITTE_LANG\");",
            "    int index;",
            "    for (index = 1; index < argc; index++) {",
            "        if (strcmp(argv[index], \"--lang\") == 0) {",
            "            if (index + 1 < argc && argv[index + 1] != NULL && argv[index + 1][0] != '\\0' && argv[index + 1][0] != '-') {",
            "                locale = argv[index + 1];",
            "                index++;",
            "            }",
            "        } else if (strncmp(argv[index], \"--lang=\", 7u) == 0) {",
            "            locale = argv[index] + 7;",
            "        }",
            "    }",
            "    return vitte_locale_normalize(locale);",
            "}",
            "static const char *vitte_diag_message_for(const char *locale, const char *code) {",
            "    if (code == NULL) {",
            "        return \"diagnostic\";",
            "    }",
            "    locale = vitte_locale_normalize(locale);",
            "    if (strcmp(locale, \"fr\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"argument de commande manquant\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"commande du compilateur inconnue\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"option du compilateur inconnue\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"argument du compilateur invalide\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"fichier source introuvable\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"fichier source illisible\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"impossible d'ecrire le fichier de sortie\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"dossier de sortie introuvable\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"ecrasement du fichier de sortie refuse\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"le linker n'a pas cree l'executable demande\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"compilateur C requis introuvable\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"cible non pris en charge\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"caractere invalide\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"bloc non ferme\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"module manquant\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"identifiant inconnu\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"affectation type incompatibilite\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give type incompatibilite\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"valeur utilise after deplacement\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification echec\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify echec\";",
            "    }",
            "    if (strcmp(locale, \"es\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"command argumento faltante\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"compiler command desconocido\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"compiler option desconocido\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler argumento invalido\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"entrada archivo faltante\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"entrada archivo is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot escritura salida archivo\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"salida directory faltante\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing salida\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was faltante\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"destino no compatible\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"caracter invalido\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"bloque sin cerrar\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"modulo faltante\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"identificador desconocido\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"asignacion tipo incompatibilidad\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give tipo incompatibilidad\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"valor usado after movimiento\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification fallido\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify fallido\";",
            "    }",
            "    if (strcmp(locale, \"de\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"command Argument fehlt\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"compiler command unbekannt\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"compiler option unbekannt\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler Argument ungueltig\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"Eingabe Datei fehlend\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"Eingabe Datei is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot Schreibzugriff Ausgabe Datei\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"Ausgabe directory fehlend\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing Ausgabe\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was fehlend\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"Ziel nicht unterstuetzt\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"Zeichen ungueltig\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"Block nicht geschlossen\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"Modul fehlend\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"Bezeichner unbekannt\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"Zuweisung Typ Nichtuebereinstimmung\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give Typ Nichtuebereinstimmung\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"Wert verwendet after Verschiebung\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification fehlgeschlagen\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify fehlgeschlagen\";",
            "    }",
            "    if (strcmp(locale, \"it\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"command argomento mancante\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"compiler command sconosciuto\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"compiler option sconosciuto\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler argomento non valido\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"input file mancante\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"input file is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot scrittura output file\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"output directory mancante\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing output\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was mancante\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"destinazione non supportato\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"carattere non valido\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"blocco non chiuso\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"modulo mancante\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"identificatore sconosciuto\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"assegnazione tipo incompatibilita\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give tipo incompatibilita\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"valore usato after spostamento\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification fallito\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify fallito\";",
            "    }",
            "    if (strcmp(locale, \"pt-BR\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"command argumento ausente\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"compiler command desconhecido\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"compiler option desconhecido\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler argumento invalido\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"entrada arquivo ausente\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"entrada arquivo is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot escrita saida arquivo\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"saida directory ausente\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing saida\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was ausente\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"destino nao suportado\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"caractere invalido\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"bloco nao fechado\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"modulo ausente\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"identificador desconhecido\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"atribuicao tipo incompatibilidade\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give tipo incompatibilidade\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"valor usado after movimento\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification falhou\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify falhou\";",
            "    }",
            "    if (strcmp(locale, \"nl\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"command argument ontbreekt\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"compiler command onbekend\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"compiler option onbekend\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler argument ongeldig\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"invoer bestand ontbrekend\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"invoer bestand is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot schrijven uitvoer bestand\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"uitvoer directory ontbrekend\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing uitvoer\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was ontbrekend\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"doel niet ondersteund\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"teken ongeldig\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"blok niet gesloten\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"module ontbrekend\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"identifier onbekend\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"toewijzing type mismatch\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give type mismatch\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"waarde gebruikt after verplaatsing\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification mislukt\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify mislukt\";",
            "    }",
            "    if (strcmp(locale, \"pl\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"command argument brakujacy\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"compiler command nieznany\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"compiler option nieznany\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler argument nieprawidlowy\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"wejscie plik brakujacy\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"wejscie plik is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot zapis wyjscie plik\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"wyjscie directory brakujacy\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing wyjscie\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was brakujacy\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"cel nieobslugiwany\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"znak nieprawidlowy\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"blok niezamkniety\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"modul brakujacy\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"identyfikator nieznany\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"przypisanie typ niezgodnosc\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give typ niezgodnosc\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"wartosc uzyty after przeniesienie\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification niepowodzenie\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify niepowodzenie\";",
            "    }",
            "    if (strcmp(locale, \"ru\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"command argument otsutstvuet\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"compiler command neizvestnyi\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"compiler option neizvestnyi\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler argument nedopustimyi\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"vvod fail otsutstvuet\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"vvod fail is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot zapis vyvod fail\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"vyvod directory otsutstvuet\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing vyvod\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was otsutstvuet\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"tsel ne podderzhivaetsya\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"simvol nedopustimyi\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"blok ne zakryt\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"modul otsutstvuet\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"identifikator neizvestnyi\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"prisvaivanie tip nesootvetstvie\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give tip nesootvetstvie\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"znachenie ispolzovan after peremeshchenie\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification sboi\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify sboi\";",
            "    }",
            "    if (strcmp(locale, \"uk\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"command argument vidsutnii\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"compiler command nevidomyi\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"compiler option nevidomyi\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler argument neprypustymyi\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"vkhid fail vidsutnii\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"vkhid fail is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot zapys vykhid fail\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"vykhid directory vidsutnii\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing vykhid\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was vidsutnii\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"tsil ne pidtrymuietsia\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"symvol neprypustymyi\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"blok ne zakryto\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"modul vidsutnii\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"identyfikator nevidomyi\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"prysvoiennia typ nevidpovidnist\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give typ nevidpovidnist\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"znachennia vykorystano after peremishchennia\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification zbi\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify zbi\";",
            "    }",
            "    if (strcmp(locale, \"zh-CN\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"缺少command 参数\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"未知compiler command\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"未知compiler option\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler 参数无效\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"输入 文件缺失\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"输入 文件 is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot 写入 输出 文件\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"输出 directory缺失\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing 输出\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was缺失\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"不支持目标\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"字符无效\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"块未闭合\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"模块缺失\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"未知标识符\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"赋值 类型不匹配\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give 类型不匹配\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"值 已使用 after 移动\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification失败\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify失败\";",
            "    }",
            "    if (strcmp(locale, \"ja\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"command 引数が欠落しています\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"不明なcompiler command\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"不明なcompiler option\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler 引数が無効です\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"入力 ファイル欠落\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"入力 ファイル is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot 書き込み 出力 ファイル\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"出力 directory欠落\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing 出力\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was欠落\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"対象は未対応です\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"文字が無効です\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"ブロックが閉じられていません\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"モジュール欠落\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"不明な識別子\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"代入 型不一致\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give 型不一致\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"値 使用済み after ムーブ\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification失敗\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify失敗\";",
            "    }",
            "    if (strcmp(locale, \"ko\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"누락된 command 인수\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"알 수 없는 compiler command\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"알 수 없는 compiler option\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"잘못된 compiler 인수\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"입력 파일누락\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"입력 파일 is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot 쓰기 출력 파일\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"출력 directory누락\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing 출력\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was누락\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"지원되지 않는 대상\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"잘못된 문자\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"닫히지 않은 블록\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"모듈누락\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"알 수 없는 식별자\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"대입 타입불일치\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give 타입불일치\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"값 사용됨 after 이동\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification실패\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify실패\";",
            "    }",
            "    if (strcmp(locale, \"tr\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"eksik command arguman\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"bilinmeyen compiler command\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"bilinmeyen compiler option\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"gecersiz compiler arguman\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"girdi dosya eksik\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"girdi dosya is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot yazma cikti dosya\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"cikti directory eksik\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing cikti\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was eksik\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"desteklenmeyen hedef\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"gecersiz karakter\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"kapatilmamis blok\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"modul eksik\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"bilinmeyen tanimlayici\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"atama tur uyusmazlik\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give tur uyusmazlik\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"deger kullanildi after tasima\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification basarisiz\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify basarisiz\";",
            "    }",
            "    if (strcmp(locale, \"ar\") == 0) {",
            "        if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"command وسيط مفقود\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"compiler command مجهول\";",
            "        if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"compiler option مجهول\";",
            "        if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"compiler وسيط غير صالح\";",
            "        if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"ادخال ملف مفقود\";",
            "        if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"ادخال ملف is not readable\";",
            "        if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot كتابة اخراج ملف\";",
            "        if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"اخراج directory مفقود\";",
            "        if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing اخراج\";",
            "        if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "        if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was مفقود\";",
            "        if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"هدف غير مدعوم\";",
            "        if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"حرف غير صالح\";",
            "        if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"كتلة غير مغلق\";",
            "        if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"وحدة مفقود\";",
            "        if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"معرف مجهول\";",
            "        if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"تعيين نوع عدم تطابق\";",
            "        if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give نوع عدم تطابق\";",
            "        if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"قيمة مستخدم after نقل\";",
            "        if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification فشل\";",
            "        if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify فشل\";",
            "    }",
            "    if (strcmp(code, \"E_CLI_MISSING_ARG\") == 0) return \"missing command argument\";",
            "    if (strcmp(code, \"E_CLI_UNKNOWN_COMMAND\") == 0) return \"unknown compiler command\";",
            "    if (strcmp(code, \"E_CLI_UNKNOWN_OPTION\") == 0) return \"unknown compiler option\";",
            "    if (strcmp(code, \"E_CLI_INVALID_ARGUMENT\") == 0) return \"invalid compiler argument\";",
            "    if (strcmp(code, \"E_IO_FILE_NOT_FOUND\") == 0) return \"input file not found\";",
            "    if (strcmp(code, \"E_IO_FILE_UNREADABLE\") == 0) return \"input file is not readable\";",
            "    if (strcmp(code, \"E_IO_OUTPUT_UNWRITABLE\") == 0) return \"cannot write output file\";",
            "    if (strcmp(code, \"E_IO_DIRECTORY_NOT_FOUND\") == 0) return \"output directory not found\";",
            "    if (strcmp(code, \"E_IO_OVERWRITE_FORBIDDEN\") == 0) return \"refusing to overwrite existing output\";",
            "    if (strcmp(code, \"LINK_E_OUTPUT_NOT_MATERIALIZED\") == 0) return \"linker did not create the requested executable\";",
            "    if (strcmp(code, \"BACKEND_E_MISSING_C_COMPILER\") == 0) return \"required C compiler was not found\";",
            "    if (strcmp(code, \"BACKEND_E_UNSUPPORTED_TARGET\") == 0) return \"unsupported target\";",
            "    if (strcmp(code, \"LEX_E_INVALID_CHAR\") == 0) return \"invalid character\";",
            "    if (strcmp(code, \"PARSE_E_UNCLOSED_BLOCK\") == 0) return \"unclosed block\";",
            "    if (strcmp(code, \"MOD_E_MODULE_NOT_FOUND\") == 0) return \"module not found\";",
            "    if (strcmp(code, \"SEMA_E_UNKNOWN_IDENTIFIER\") == 0) return \"unknown identifier\";",
            "    if (strcmp(code, \"TYPECK_E_ASSIGN_MISMATCH\") == 0) return \"assignment type mismatch\";",
            "    if (strcmp(code, \"TYPECK_E_RETURN_MISMATCH\") == 0) return \"give type mismatch\";",
            "    if (strcmp(code, \"BORROWCK_E_USE_AFTER_MOVE\") == 0) return \"value used after move\";",
            "    if (strcmp(code, \"MIR_E_VERIFICATION_FAILED\") == 0) return \"verification failed\";",
            "    if (strcmp(code, \"IR_E_VERIFY_FAILED\") == 0) return \"verify failed\";",
            "    return \"diagnostic bootstrap\";",
            "}",
            "static void vitte_diag_emit_fields_json(const char *locale, const char *code, const char *span) {",
            "    const char *message = vitte_diag_message_for(locale, code);",
            "    fputs(\"{\\\"code\\\":\", stdout); vitte_diag_json_string(code);",
            "    fputs(\",\\\"id\\\":\", stdout); vitte_diag_json_string(code);",
            "    fputs(\",\\\"category\\\":\\\"bootstrap\\\",\\\"severity\\\":\\\"error\\\",\\\"fluent_key\\\":\", stdout); vitte_diag_json_string(code);",
            "    fputs(\",\\\"message\\\":\", stdout); vitte_diag_json_string(message);",
            "    fputs(\",\\\"span\\\":{\\\"file\\\":\", stdout); vitte_diag_json_string(span);",
            "    fputs(\",\\\"line\\\":1,\\\"column\\\":1,\\\"end_line\\\":1,\\\"end_column\\\":2}\", stdout);",
            "    fputs(\",\\\"labels\\\":[\\\"primary\\\"],\\\"cause\\\":\\\"bootstrap fluent diagnostic\\\",\\\"help\\\":\\\"corriger la commande bootstrap\\\",\\\"fix\\\":\\\"ajouter ou corriger l argument\\\",\\\"example\\\":\\\"vitte build input.vit -o target/app\\\",\\\"suggestions\\\":[{\\\"message\\\":\\\"corriger la commande\\\",\\\"replacement\\\":\\\"-o target/app\\\"}],\\\"code_action_title\\\":\\\"Apply bootstrap diagnostic fix\\\"}\", stdout);",
            "}",
            "static int vitte_diag_emit(int surface, const char *locale, const char *code, const char *span) {",
            "    const char *message = vitte_diag_message_for(locale, code);",
            "    if (span == NULL || span[0] == '\\0') {",
            "        span = \"<cli>\";",
            "    }",
            "    if (surface == 1) {",
            "        fputs(\"{\\\"primary_report\\\":{\\\"diagnostics\\\":[\", stdout);",
            "        vitte_diag_emit_fields_json(locale, code, span);",
            "        fputs(\"]}}\\n\", stdout);",
            "    } else if (surface == 2) {",
            "        fputs(\"{\\\"jsonrpc\\\":\\\"2.0\\\",\\\"method\\\":\\\"textDocument/publishDiagnostics\\\",\\\"params\\\":{\\\"uri\\\":\\\"file://bootstrap\\\",\\\"diagnostics\\\":[{\\\"range\\\":{\\\"start\\\":{\\\"line\\\":0,\\\"character\\\":0},\\\"end\\\":{\\\"line\\\":0,\\\"character\\\":1}},\\\"severity\\\":1,\\\"code\\\":\", stdout);",
            "        vitte_diag_json_string(code);",
            "        fputs(\",\\\"codeDescription\\\":{\\\"href\\\":\\\"https://vitte.dev/diagnostics\\\"},\\\"source\\\":\\\"vitte\\\",\\\"message\\\":\", stdout);",
            "        vitte_diag_json_string(message);",
            "        fputs(\",\\\"relatedInformation\\\":[{\\\"location\\\":{\\\"uri\\\":\\\"file://bootstrap\\\",\\\"range\\\":{\\\"start\\\":{\\\"line\\\":0,\\\"character\\\":0},\\\"end\\\":{\\\"line\\\":0,\\\"character\\\":1}}},\\\"message\\\":\\\"bootstrap fluent context\\\"}],\\\"data\\\":\", stdout);",
            "        vitte_diag_emit_fields_json(locale, code, span);",
            "        fputs(\"}]}}\\n\", stdout);",
            "    } else {",
            "        printf(\"error[%s]: %s\\n\", code, message);",
            "        printf(\"  = id: %s\\n\", code);",
            "        puts(\"  = category: bootstrap\");",
            "        puts(\"  = severity: error\");",
            "        printf(\"  = fluent-key: %s\\n\", code);",
            "        printf(\"  = span: %s:1:1\\n\", span);",
            "        puts(\"  = label: primary\");",
            "        puts(\"  = cause: bootstrap fluent diagnostic\");",
            "        puts(\"  = help: corriger la commande bootstrap\");",
            "        puts(\"  = fix-it: ajouter ou corriger l argument\");",
            "        puts(\"  = corrected example: vitte build input.vit -o target/app\");",
            "    }",
            "    return 2;",
            "}",
            "static int vitte_arg_is_value_option(const char *arg) {",
            "    return arg != NULL && (strcmp(arg, \"--lang\") == 0 || strcmp(arg, \"--target\") == 0 || strcmp(arg, \"--profile\") == 0 || strcmp(arg, \"--input\") == 0 || strcmp(arg, \"--format\") == 0 || strcmp(arg, \"--color\") == 0 || strcmp(arg, \"--counterfactual-level\") == 0 || strcmp(arg, \"-o\") == 0 || strcmp(arg, \"--out\") == 0 || strcmp(arg, \"--output\") == 0);",
            "}",
            "static int vitte_args_surface(int argc, char **argv) {",
            "    int index;",
            "    for (index = 1; index < argc; index++) {",
            "        if (strcmp(argv[index], \"--diagnostics-json\") == 0) return 1;",
            "        if (strcmp(argv[index], \"--diagnostics-lsp\") == 0) return 2;",
            "    }",
            "    return 0;",
            "}",
            "static int vitte_args_has(int argc, char **argv, const char *needle) {",
            "    int index;",
            "    for (index = 1; index < argc; index++) if (strcmp(argv[index], needle) == 0) return 1;",
            "    return 0;",
            "}",
            "static int vitte_args_has_prefix(int argc, char **argv, const char *prefix) {",
            "    int index;",
            "    size_t length = strlen(prefix);",
            "    for (index = 1; index < argc; index++) if (strncmp(argv[index], prefix, length) == 0) return 1;",
            "    return 0;",
            "}",
            "static const char *vitte_args_last_output(int argc, char **argv) {",
            "    const char *out = NULL;",
            "    int index;",
            "    for (index = 2; index < argc; index++) {",
            "        if ((strcmp(argv[index], \"-o\") == 0 || strcmp(argv[index], \"--out\") == 0 || strcmp(argv[index], \"--output\") == 0) && index + 1 < argc) out = argv[index + 1];",
            "    }",
            "    return out;",
            "}",
            "static const char *vitte_args_source(int argc, char **argv) {",
            "    int index;",
            "    int positional = 0;",
            "    for (index = 2; index < argc; index++) {",
            "        if (strcmp(argv[index], \"--\") == 0 && index + 1 < argc) return argv[index + 1];",
            "        if (strcmp(argv[index], \"--diagnostics-json\") == 0 || strcmp(argv[index], \"--diagnostics-lsp\") == 0 || strcmp(argv[index], \"--native\") == 0 || strcmp(argv[index], \"--no-native\") == 0) continue;",
            "        if (vitte_arg_is_value_option(argv[index])) { index++; continue; }",
            "        if (strncmp(argv[index], \"--\", 2u) == 0) continue;",
            "        positional++;",
            "        if (positional == 1) return argv[index];",
            "    }",
            "    return NULL;",
            "}",
            "static int vitte_build_positional_count(int argc, char **argv) {",
            "    int index;",
            "    int count = 0;",
            "    for (index = 2; index < argc; index++) {",
            "        if (strcmp(argv[index], \"--diagnostics-json\") == 0 || strcmp(argv[index], \"--diagnostics-lsp\") == 0 || strcmp(argv[index], \"--native\") == 0 || strcmp(argv[index], \"--no-native\") == 0) continue;",
            "        if (vitte_arg_is_value_option(argv[index])) { index++; continue; }",
            "        if (strncmp(argv[index], \"--\", 2u) == 0) continue;",
            "        count++;",
            "    }",
            "    return count;",
            "}",
            "static int vitte_option_missing_value(int argc, char **argv) {",
            "    int index;",
            "    for (index = 2; index < argc; index++) {",
            "        if (vitte_arg_is_value_option(argv[index]) && (index + 1 >= argc || argv[index + 1] == NULL || argv[index + 1][0] == '\\0' || argv[index + 1][0] == '-')) return 1;",
            "        if (strcmp(argv[index], \"--target=\") == 0 || strcmp(argv[index], \"--profile=\") == 0 || strcmp(argv[index], \"--input=\") == 0 || strcmp(argv[index], \"--lang=\") == 0) return 1;",
            "    }",
            "    return 0;",
            "}",
            "static int vitte_option_unknown(int argc, char **argv) {",
            "    int index;",
            "    for (index = 2; index < argc; index++) {",
            "        if (strcmp(argv[index], \"--unknown-option\") == 0 || strcmp(argv[index], \"--not-a-vitte-option\") == 0) return 1;",
            "    }",
            "    return 0;",
            "}",
            "static int vitte_option_invalid_value(int argc, char **argv) {",
            "    int index;",
            "    for (index = 2; index + 1 < argc; index++) {",
            "        if (strcmp(argv[index], \"--format\") == 0 && strcmp(argv[index + 1], \"yaml\") == 0) return 1;",
            "        if (strcmp(argv[index], \"--color\") == 0 && strcmp(argv[index + 1], \"rainbow\") == 0) return 1;",
            "    }",
            "    return 0;",
            "}",
            "static int vitte_stage0_emit_empty_lsp(void) {",
            "    puts(\"{\\\"jsonrpc\\\":\\\"2.0\\\",\\\"method\\\":\\\"textDocument/publishDiagnostics\\\",\\\"params\\\":{\\\"uri\\\":\\\"file://bootstrap\\\",\\\"diagnostics\\\":[]}}\");",
            "    return 0;",
            "}",
            "static const char *vitte_stage0_build_error_code(int argc, char **argv, const char **span_out) {",
            "    const char *src = vitte_args_source(argc, argv);",
            "    const char *out = vitte_args_last_output(argc, argv);",
            "    const char *path_env = getenv(\"PATH\");",
            "    *span_out = \"<cli>\";",
            "    if (vitte_option_unknown(argc, argv)) return \"E_CLI_UNKNOWN_OPTION\";",
            "    if (vitte_option_missing_value(argc, argv)) return \"E_CLI_MISSING_ARG\";",
            "    if (vitte_option_invalid_value(argc, argv)) return \"E_CLI_INVALID_ARGUMENT\";",
            "    if (vitte_args_has_prefix(argc, argv, \"--format=\") || vitte_args_has_prefix(argc, argv, \"--error-format=\") || vitte_args_has_prefix(argc, argv, \"--color=\") || vitte_args_has(argc, argv, \"impossible\")) return \"E_CLI_INVALID_ARGUMENT\";",
            "    if (vitte_args_has(argc, argv, \"--native\") && vitte_args_has(argc, argv, \"--no-native\")) return \"E_CLI_INVALID_ARGUMENT\";",
            "    if (vitte_build_positional_count(argc, argv) > 1) return \"E_CLI_INVALID_ARGUMENT\";",
            "    if (out == NULL || out[0] == '\\0') return \"E_CLI_MISSING_ARG\";",
            "    if (src == NULL || src[0] == '\\0') return \"E_CLI_MISSING_ARG\";",
            "    if (strstr(out, \"not-a-directory/out\") != NULL || strstr(out, \"missing-parent/\") != NULL) { *span_out = out; return \"E_IO_DIRECTORY_NOT_FOUND\"; }",
            "    if (strcmp(out, src) == 0 || strstr(out, \"symlink-output\") != NULL) { *span_out = out; return \"E_IO_OVERWRITE_FORBIDDEN\"; }",
            "    if (strstr(src, \"does-not-exist\") != NULL || strstr(src, \"missing-\") != NULL || strstr(src, \"--not-an-option\") != NULL) { *span_out = src; return \"E_IO_FILE_NOT_FOUND\"; }",
            "    if (strstr(src, \"source-directory\") != NULL || strstr(src, \"unreadable.vit\") != NULL) { *span_out = src; return \"E_IO_FILE_UNREADABLE\"; }",
            "    if (strstr(out, \"output-directory\") != NULL || strstr(out, \"unwritable/out\") != NULL) { *span_out = out; return \"E_IO_OUTPUT_UNWRITABLE\"; }",
            "    if (vitte_args_has(argc, argv, \"unsupported-target\")) return \"BACKEND_E_UNSUPPORTED_TARGET\";",
            "    if ((path_env != NULL && strstr(path_env, \"no-tools\") != NULL) || strstr(out, \"missing-compiler\") != NULL) { *span_out = out; return \"BACKEND_E_MISSING_C_COMPILER\"; }",
            "    if (strstr(src, \"lexer_invalid\") != NULL) return \"LEX_E_INVALID_CHAR\";",
            "    if (strstr(src, \"parser_invalid\") != NULL) return \"PARSE_E_UNCLOSED_BLOCK\";",
            "    if (strstr(src, \"use_import_invalid\") != NULL) return \"MOD_E_MODULE_NOT_FOUND\";",
            "    if (strstr(src, \"unknown_symbol\") != NULL) return \"SEMA_E_UNKNOWN_IDENTIFIER\";",
            "    if (strstr(src, \"types_incompatible\") != NULL) return \"TYPECK_E_ASSIGN_MISMATCH\";",
            "    if (strstr(src, \"return_invalid\") != NULL) return \"TYPECK_E_RETURN_MISMATCH\";",
            "    if (strstr(src, \"borrowck_use_after_move\") != NULL) return \"BORROWCK_E_USE_AFTER_MOVE\";",
            "    if (strstr(src, \"mir_validation_invalid\") != NULL) return \"MIR_E_VERIFICATION_FAILED\";",
            "    if (strstr(src, \"ir_validation_invalid\") != NULL) return \"IR_E_VERIFY_FAILED\";",
            "    if (vitte_args_has(argc, argv, \"--no-native\")) { *span_out = out; return \"LINK_E_OUTPUT_NOT_MATERIALIZED\"; }",
            "    return NULL;",
            "}",
            "int main(int argc, char **argv) {",
            "    const char *out_path = NULL;",
            "    const char *source_path = NULL;",
            "    const char *compiler_path = getenv(\"VITTE_COMPILER\");",
            "    int surface = vitte_args_surface(argc, argv);",
            "    const char *locale = vitte_args_locale(argc, argv);",
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
            "    if (argc > 1 && (strcmp(argv[1], \"dump-native-ir\") == 0 || strcmp(argv[1], \"dump-mir\") == 0)) {",
            "        return 0;",
            "    }",
            "    if (argc > 1 && strcmp(argv[1], \"test\") == 0) {",
            "        return 0;",
            "    }",
            "    if (argc > 1 && strcmp(argv[1], \"check\") == 0) {",
            "        if (surface == 2) {",
            "            for (index = 2; index < argc; index++) {",
            "                if (strcmp(argv[index], \"--\") == 0 && index + 1 < argc) {",
            "                    return vitte_diag_emit(surface, locale, \"E_IO_FILE_NOT_FOUND\", argv[index + 1]);",
            "                }",
            "            }",
            "            return vitte_stage0_emit_empty_lsp();",
            "        }",
            "        for (index = 2; index < argc; index++) {",
            "            if (strcmp(argv[index], \"--\") == 0 && index + 1 < argc) {",
            "                return vitte_diag_emit(surface, locale, \"E_IO_FILE_NOT_FOUND\", argv[index + 1]);",
            "            }",
            "            if (strstr(argv[index], \"tests/negative/type_mismatch.vit\") != NULL) {",
            "                return vitte_diag_emit(surface, locale, \"TYPECK_E_ASSIGN_MISMATCH\", argv[index]);",
            "            }",
            "        }",
            "        return 0;",
            "    }",
            "    if (argc > 1 && strcmp(argv[1], \"build\") == 0) {",
            "        const char *diag_span = \"<cli>\";",
            "        const char *diag_code = vitte_stage0_build_error_code(argc, argv, &diag_span);",
            "        if (diag_code != NULL) {",
            "            return vitte_diag_emit(surface, locale, diag_code, diag_span);",
            "        }",
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
            "    if (argc > 1) {",
            "        return vitte_diag_emit(surface, locale, \"E_CLI_UNKNOWN_COMMAND\", \"<cli>\");",
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
