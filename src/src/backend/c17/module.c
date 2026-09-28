#include "module.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "naming.h"

static bool vitte_c17_ir_value_is_used(
    const vitte_ir_function_t *function,
    const vitte_ir_value_t *value
);

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

static bool vitte_c17_is_list_type(const vitte_ir_type_t *type) {
    if (type == NULL || type->kind != VITTE_IR_TYPE_AGGREGATE_PTR || type->name == NULL) {
        return false;
    }
    return strncmp(type->name, "list[", strlen("list[")) == 0 || type->name[0] == '[';
}

static vitte_status_t vitte_c17_emit_ir_value_ref(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_value_t *value
);
static vitte_status_t vitte_c17_emit_statement_line_end(vitte_c17_writer_t *writer);

static const char *vitte_c17_function_source_name(const vitte_ir_function_t *function) {
    if (function != NULL && function->source != NULL && function->source->kind == VITTE_HIR_FUNCTION &&
        function->source->as.function.source_name != NULL) {
        return function->source->as.function.source_name;
    }
    return function != NULL ? function->name : NULL;
}

static const char *vitte_c17_host_intrinsic_helper(const vitte_ir_function_t *function) {
    const char *name = vitte_c17_function_source_name(function);

    if (name == NULL) return NULL;
    if (strcmp(name, "vitte_host_runtime_available") == 0) return "vitte_c17_host_runtime_available";
    if (strcmp(name, "vitte_host_read_file") == 0) return "vitte_c17_host_read_file";
    if (strcmp(name, "vitte_host_write_file") == 0) return "vitte_c17_host_write_file";
    if (strcmp(name, "vitte_host_append_file") == 0) return "vitte_c17_host_append_file";
    if (strcmp(name, "vitte_host_file_exists") == 0) return "vitte_c17_host_file_exists";
    if (strcmp(name, "vitte_host_is_file") == 0) return "vitte_c17_host_is_file";
    if (strcmp(name, "vitte_host_is_directory") == 0) return "vitte_c17_host_is_directory";
    if (strcmp(name, "vitte_host_list_directory") == 0) return "vitte_c17_host_list_directory";
    if (strcmp(name, "vitte_host_mkdir_all") == 0) return "vitte_c17_host_mkdir_all";
    if (strcmp(name, "vitte_host_system") == 0) return "vitte_c17_host_system";
    if (strcmp(name, "vitte_host_emit_llvm_object") == 0) return "vitte_c17_host_emit_llvm_object";
    if (strcmp(name, "vitte_host_emit_assembly_object") == 0) return "vitte_c17_host_emit_assembly_object";
    if (strcmp(name, "vitte_host_verify_native_object") == 0) return "vitte_c17_host_verify_native_object";
    if (strcmp(name, "vitte_host_link_executable") == 0) return "vitte_c17_host_link_executable";
    if (strcmp(name, "vitte_host_run_executable") == 0) return "vitte_c17_host_run_executable";
    if (strcmp(name, "vitte_host_memory_checkpoint") == 0) return "vitte_c17_host_memory_checkpoint";
    if (strcmp(name, "vitte_host_memory_rewind") == 0) return "vitte_c17_host_memory_rewind";
    return NULL;
}

static vitte_status_t vitte_c17_emit_host_intrinsic_return(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_function_t *function,
    const char *helper
) {
    const vitte_ir_value_t *parameter;
    bool first = true;
    vitte_status_t status = vitte_c17_write_string(writer, "return ");

    if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, helper);
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, '(');
    for (parameter = function->first_parameter; status == VITTE_STATUS_OK && parameter != NULL; parameter = parameter->next) {
        if (!first) status = vitte_c17_write_string(writer, ", ");
        if (status == VITTE_STATUS_OK) status = vitte_c17_emit_ir_value_ref(module, writer, parameter);
        first = false;
    }
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ')');
    if (status == VITTE_STATUS_OK) status = vitte_c17_emit_statement_line_end(writer);
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_close_block(writer);
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_newline(writer);
    if (status == VITTE_STATUS_OK) module->unit->function_count++;
    return status;
}

static vitte_status_t vitte_c17_make_symbol_name(
    vitte_c17_module_t *module,
    const char *prefix,
    const char *base,
    uint32_t id,
    char *output,
    size_t output_capacity
) {
    char sanitized[512];
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
        case VITTE_IR_TYPE_AGGREGATE_PTR:
            return vitte_c17_write_string(writer, "vitte_aggregate *");
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
    char name[512];

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

static vitte_status_t vitte_c17_emit_aggregate_ref(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ir_value_t *value
) {
    vitte_status_t status = vitte_c17_write_string(writer, "(vitte_aggregate *)(uintptr_t)(");
    if (status == VITTE_STATUS_OK) status = vitte_c17_emit_ir_value_ref(module, writer, value);
    if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ')');
    return status;
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
        strcmp(name, "slice") == 0 ||
        strcmp(name, "find") == 0 ||
        strcmp(name, "trim") == 0 ||
        strcmp(name, "starts_with") == 0 ||
        strcmp(name, "ends_with") == 0 ||
        strcmp(name, "to_string") == 0 ||
        strcmp(name, "to_string_int") == 0 ||
        strcmp(name, "to_string_i64") == 0 ||
        strcmp(name, "to_string_u64") == 0 ||
        strcmp(name, "to_string_usize") == 0);
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
    if (strcmp(name, "find") == 0) {
        if (instruction->result == NULL || instruction->operand_count < 3u) {
            return vitte_c17_emit_statement_line_end(writer);
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_write_string(writer, " = ");
        if (status != VITTE_STATUS_OK) return status;
        if (instruction->operands[1] != NULL && instruction->operands[1]->type != NULL &&
            instruction->operands[1]->type->kind == VITTE_IR_TYPE_STRING_PTR) {
            status = vitte_c17_write_string(writer, "vitte_string_find(");
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_write_string(writer, ", ");
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[2]);
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_write_char(writer, ')');
        } else {
            status = vitte_c17_write_string(writer, "0");
        }
        if (status != VITTE_STATUS_OK) return status;
        return vitte_c17_emit_statement_line_end(writer);
    }
    if (strcmp(name, "trim") == 0) {
        if (instruction->result == NULL || argument == NULL || argument->type == NULL ||
            argument->type->kind != VITTE_IR_TYPE_STRING_PTR) {
            return vitte_c17_emit_statement_line_end(writer);
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_write_string(writer, " = vitte_string_trim(");
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_emit_ir_value_ref(module, writer, argument);
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_write_char(writer, ')');
        if (status != VITTE_STATUS_OK) return status;
        return vitte_c17_emit_statement_line_end(writer);
    }
    if (strcmp(name, "starts_with") == 0 || strcmp(name, "ends_with") == 0) {
        if (instruction->result == NULL || instruction->operand_count < 3u ||
            instruction->operands[1] == NULL || instruction->operands[2] == NULL ||
            instruction->operands[1]->type == NULL ||
            instruction->operands[1]->type->kind != VITTE_IR_TYPE_STRING_PTR) {
            return vitte_c17_emit_statement_line_end(writer);
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_write_string(writer, strcmp(name, "starts_with") == 0 ? " = vitte_string_starts_with(" : " = vitte_string_ends_with(");
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_write_string(writer, ", ");
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[2]);
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_write_char(writer, ')');
        if (status != VITTE_STATUS_OK) return status;
        return vitte_c17_emit_statement_line_end(writer);
    }
    if (strcmp(name, "to_string") == 0 || strcmp(name, "to_string_int") == 0 ||
        strcmp(name, "to_string_i64") == 0 || strcmp(name, "to_string_u64") == 0 ||
        strcmp(name, "to_string_usize") == 0) {
        if (instruction->result == NULL || instruction->result->type == NULL || instruction->result->type->kind == VITTE_IR_TYPE_VOID || argument == NULL) {
            return vitte_c17_emit_statement_line_end(writer);
        }
        status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_write_string(writer, " = vitte_string_from_i64((int64_t)(");
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_emit_ir_value_ref(module, writer, argument);
        if (status != VITTE_STATUS_OK) return status;
        status = vitte_c17_write_string(writer, "))");
        if (status != VITTE_STATUS_OK) return status;
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
        if (instruction->result != NULL && instruction->result->type != NULL &&
            instruction->result->type->kind != VITTE_IR_TYPE_VOID) {
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_write_string(writer, " = ");
            if (status != VITTE_STATUS_OK) return status;
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
    const vitte_ir_function_t *function,
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
    assign_result =
    instruction->result != NULL &&
    instruction->result->type != NULL &&
    instruction->result->type->kind != VITTE_IR_TYPE_VOID &&
    vitte_c17_ir_value_is_used(function, instruction->result);

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
        } else if (instruction->operand_count > 1u &&
            instruction->operands[1] != NULL &&
            instruction->operands[1]->type != NULL &&
            instruction->operands[1]->type->kind == VITTE_IR_TYPE_AGGREGATE_PTR) {
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_write_string(writer, " != NULL ? ");
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_c17_write_string(writer, "->count : 0u");
            if (status != VITTE_STATUS_OK) return status;
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
            if (instruction->result->type->kind == VITTE_IR_TYPE_STRING_PTR && instruction->operand_count > 1u &&
                instruction->operands[1] != NULL && instruction->operands[1]->type != NULL &&
                (instruction->operands[1]->type->kind == VITTE_IR_TYPE_I32 ||
                 instruction->operands[1]->type->kind == VITTE_IR_TYPE_I64 ||
                 instruction->operands[1]->type->kind == VITTE_IR_TYPE_USIZE)) {
                status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
                if (status != VITTE_STATUS_OK) return status;
                status = vitte_c17_write_string(writer, " = vitte_string_from_i64((int64_t)(");
                if (status != VITTE_STATUS_OK) return status;
                status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
                if (status != VITTE_STATUS_OK) return status;
                status = vitte_c17_write_string(writer, "))");
                if (status != VITTE_STATUS_OK) return status;
                return vitte_c17_emit_statement_line_end(writer);
            }
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
    char label[512];
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
                    status = vitte_c17_write_string(writer, "vitte_string_equal(");
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_write_string(writer, ", ");
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_write_format(writer, ") %s true", strcmp(instruction->operator_text, "==") == 0 ? "==" : "!=");
                    if (status != VITTE_STATUS_OK) return status;
                } else if (instruction->operator_text != NULL && strcmp(instruction->operator_text, "+") == 0) {
                    status = vitte_c17_write_string(writer, "vitte_string_concat(");
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_write_string(writer, ", ");
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_write_char(writer, ')');
                    if (status != VITTE_STATUS_OK) return status;
                } else if (instruction->operator_text != NULL &&
                    (strcmp(instruction->operator_text, "<") == 0 || strcmp(instruction->operator_text, "<=") == 0 ||
                        strcmp(instruction->operator_text, ">") == 0 || strcmp(instruction->operator_text, ">=") == 0)) {
                    status = vitte_c17_write_string(writer, "vitte_string_compare(");
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[0]);
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_write_string(writer, ", ");
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
                    if (status != VITTE_STATUS_OK) return status;
                    status = vitte_c17_write_format(writer, ") %s 0", instruction->operator_text);
                    if (status != VITTE_STATUS_OK) return status;
                } else {
                    status = vitte_c17_write_string(writer, "\"\"");
                    if (status != VITTE_STATUS_OK) return status;
                }
            } else if (instruction->operands[0] != NULL &&
                instruction->operands[1] != NULL &&
                instruction->operands[0]->type != NULL &&
                instruction->operands[1]->type != NULL &&
                instruction->operands[0]->type->kind == VITTE_IR_TYPE_AGGREGATE_PTR &&
                instruction->operands[1]->type->kind == VITTE_IR_TYPE_AGGREGATE_PTR &&
                instruction->operator_text != NULL) {
                if (instruction->result != NULL && instruction->result->type != NULL &&
                    instruction->result->type->kind == VITTE_IR_TYPE_AGGREGATE_PTR) {
                    if (strcmp(instruction->operator_text, "+") == 0 &&
                        vitte_c17_is_list_type(instruction->operands[0]->type) &&
                        vitte_c17_is_list_type(instruction->operands[1]->type)) {
                        status = vitte_c17_write_string(writer, "vitte_aggregate_concat(");
                        if (status == VITTE_STATUS_OK) status = vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[0]);
                        if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, ", ");
                        if (status == VITTE_STATUS_OK) status = vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[1]);
                        if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ')');
                    } else {
                        status = vitte_c17_write_string(writer, "vitte_aggregate_binary_int(");
                        if (status == VITTE_STATUS_OK) status = vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[0]);
                        if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, ", ");
                        if (status == VITTE_STATUS_OK) status = vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[1]);
                        if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, ", ");
                        if (status == VITTE_STATUS_OK) status = vitte_c17_emit_c_string(writer, instruction->operator_text);
                        if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ')');
                    }
                } else {
                    status = vitte_c17_write_string(writer, "vitte_aggregate_unbox_int(");
                    if (status == VITTE_STATUS_OK) status = vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[0]);
                    if (status == VITTE_STATUS_OK) status = vitte_c17_write_format(writer, ") %s vitte_aggregate_unbox_int(", instruction->operator_text);
                    if (status == VITTE_STATUS_OK) status = vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[1]);
                    if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ')');
                }
                if (status != VITTE_STATUS_OK) return status;
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
            return vitte_c17_emit_ir_call(
                module,
                writer,
                function,
                instruction
            );
        case VITTE_IR_OP_AGGREGATE_NEW:
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, " = vitte_aggregate_new()");
            return status == VITTE_STATUS_OK ? vitte_c17_emit_statement_line_end(writer) : status;
        case VITTE_IR_OP_LIST_APPEND: {
            const vitte_ir_value_t *value = instruction->operands[1];
            const char *helper = value != NULL && value->type != NULL && value->type->kind == VITTE_IR_TYPE_STRING_PTR ?
                "vitte_aggregate_append_string(" :
                value != NULL && value->type != NULL && value->type->kind == VITTE_IR_TYPE_AGGREGATE_PTR ?
                    "vitte_aggregate_append_aggregate(" : "vitte_aggregate_append_int(";
            status = vitte_c17_write_string(writer, helper);
            if (status == VITTE_STATUS_OK) status = vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[0]);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, ", ");
            if (status == VITTE_STATUS_OK) status = vitte_c17_emit_ir_value_ref(module, writer, value);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ')');
            return status == VITTE_STATUS_OK ? vitte_c17_emit_statement_line_end(writer) : status;
        }
        case VITTE_IR_OP_INDEX_GET: {
            const char *helper = instruction->result != NULL && instruction->result->type != NULL && instruction->result->type->kind == VITTE_IR_TYPE_STRING_PTR ?
                "vitte_aggregate_get_string(" :
                instruction->result != NULL && instruction->result->type != NULL && instruction->result->type->kind == VITTE_IR_TYPE_AGGREGATE_PTR ?
                    "vitte_aggregate_get_aggregate(" : "vitte_aggregate_get_int(";
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, " = ");
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, helper);
            if (status == VITTE_STATUS_OK) status = vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[0]);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, ", ");
            if (status == VITTE_STATUS_OK) status = vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[1]);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ')');
            return status == VITTE_STATUS_OK ? vitte_c17_emit_statement_line_end(writer) : status;
        }
        case VITTE_IR_OP_INDEX_SET: {
            const vitte_ir_value_t *value = instruction->operands[2];
            const char *helper = value != NULL && value->type != NULL && value->type->kind == VITTE_IR_TYPE_STRING_PTR ?
                "vitte_aggregate_set_index_string(" :
                value != NULL && value->type != NULL && value->type->kind == VITTE_IR_TYPE_AGGREGATE_PTR ?
                    "vitte_aggregate_set_index_aggregate(" : "vitte_aggregate_set_index_int(";
            status = vitte_c17_write_string(writer, helper);
            for (size_t index = 0u; status == VITTE_STATUS_OK && index < 3u; index++) {
                if (index > 0u) status = vitte_c17_write_string(writer, ", ");
                if (status == VITTE_STATUS_OK) status = index == 0u ?
                    vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[index]) :
                    vitte_c17_emit_ir_value_ref(module, writer, instruction->operands[index]);
            }
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ')');
            return status == VITTE_STATUS_OK ? vitte_c17_emit_statement_line_end(writer) : status;
        }
        case VITTE_IR_OP_FIELD_GET: {
            const char *helper = instruction->result != NULL && instruction->result->type != NULL && instruction->result->type->kind == VITTE_IR_TYPE_STRING_PTR ?
                "vitte_aggregate_get_field_string(" :
                instruction->result != NULL && instruction->result->type != NULL && instruction->result->type->kind == VITTE_IR_TYPE_AGGREGATE_PTR ?
                    "vitte_aggregate_get_field_aggregate(" : "vitte_aggregate_get_field_int(";
            status = vitte_c17_emit_ir_value_ref(module, writer, instruction->result);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, " = ");
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, helper);
            if (status == VITTE_STATUS_OK) status = vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[0]);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, ", ");
            if (status == VITTE_STATUS_OK) status = vitte_c17_emit_c_string(writer, instruction->operator_text);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ')');
            return status == VITTE_STATUS_OK ? vitte_c17_emit_statement_line_end(writer) : status;
        }
        case VITTE_IR_OP_FIELD_SET: {
            const vitte_ir_value_t *value = instruction->operands[1];
            const char *helper = value != NULL && value->type != NULL && value->type->kind == VITTE_IR_TYPE_STRING_PTR ?
                "vitte_aggregate_set_field_string(" :
                value != NULL && value->type != NULL && value->type->kind == VITTE_IR_TYPE_AGGREGATE_PTR ?
                    "vitte_aggregate_set_field_aggregate(" : "vitte_aggregate_set_field_int(";
            status = vitte_c17_write_string(writer, helper);
            if (status == VITTE_STATUS_OK) status = vitte_c17_emit_aggregate_ref(module, writer, instruction->operands[0]);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, ", ");
            if (status == VITTE_STATUS_OK) status = vitte_c17_emit_c_string(writer, instruction->operator_text);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_string(writer, ", ");
            if (status == VITTE_STATUS_OK) status = vitte_c17_emit_ir_value_ref(module, writer, value);
            if (status == VITTE_STATUS_OK) status = vitte_c17_write_char(writer, ')');
            return status == VITTE_STATUS_OK ? vitte_c17_emit_statement_line_end(writer) : status;
        }
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
static bool vitte_c17_ir_value_is_used(
    const vitte_ir_function_t *function,
    const vitte_ir_value_t *value
) {
    const vitte_ir_block_t *block;
    const vitte_ir_instruction_t *instruction;
    size_t i;

    if (function == NULL || value == NULL) {
        return false;
    }

    for (block = function->first_block;
         block != NULL;
         block = block->next) {

        for (instruction = block->first;
             instruction != NULL;
             instruction = instruction->next) {

            for (i = 0; i < instruction->operand_count; ++i) {
                if (instruction->operands[i] == value ||
                    (instruction->operands[i] != NULL && value->id != 0u &&
                     instruction->operands[i]->id == value->id)) {
                    return true;
                }
            }
        }
    }

    return false;
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
            if (vitte_c17_ir_instruction_needs_declaration(instruction) &&
            (instruction->opcode != VITTE_IR_OP_CALL ||
                vitte_c17_ir_value_is_used(function, instruction->result))) {
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
    char function_name[512];
    vitte_status_t status;

    if (function == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_FUNCTION", "missing IR function", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }
    if (vitte_c17_is_main_name(function->name)) {
        status = vitte_c17_write_string(
            writer,
            function->parameter_count > 0u ?
                "int main(int argc, char **argv)" : "int main(void)"
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
    const char *host_intrinsic_helper;
    char entry_label[512];
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

    host_intrinsic_helper = vitte_c17_host_intrinsic_helper(function);
    if (host_intrinsic_helper != NULL) {
        return vitte_c17_emit_host_intrinsic_return(module, writer, function, host_intrinsic_helper);
    }

    status = vitte_c17_emit_ir_function_declarations(module, writer, function);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    if (vitte_c17_is_main_name(function->name)) {
        const vitte_ir_value_t *parameter;
        for (parameter = function->first_parameter; parameter != NULL; parameter = parameter->next) {
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
            status = parameter == function->first_parameter && parameter->type != NULL && parameter->type->kind == VITTE_IR_TYPE_AGGREGATE_PTR ?
                vitte_c17_write_string(writer, " = vitte_aggregate_from_argv(argc, argv)") :
                vitte_c17_write_string(writer, " = 0");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_statement_line_end(writer);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
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
        char label[512];
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
    char name[512];
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
    char pick_name[512];
    char qualified_variant[256];
    char variant_name[512];
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
    char form_name[512];
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
