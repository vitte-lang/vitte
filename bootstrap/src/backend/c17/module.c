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

void vitte_c17_module_init(
    vitte_c17_module_t *module,
    const vitte_ast_module_t *ast_module,
    vitte_c17_translation_unit_t *unit
) {
    if (module == NULL) {
        return;
    }

    memset(module, 0, sizeof(*module));
    module->ast_module = ast_module;
    module->unit = unit;
    vitte_error_init(&module->last_error);
}

const vitte_error_t *vitte_c17_module_last_error(const vitte_c17_module_t *module) {
    return module != NULL ? &module->last_error : vitte_error_last();
}

static vitte_status_t vitte_c17_emit_expr(vitte_c17_module_t *module, vitte_c17_writer_t *writer, const vitte_ast_expr_t *expr);
static vitte_status_t vitte_c17_emit_stmt(vitte_c17_module_t *module, vitte_c17_writer_t *writer, const vitte_ast_stmt_t *stmt);

static vitte_status_t vitte_c17_emit_type(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ast_type_ref_t *type
) {
    const char *name;

    if (type == NULL) {
        return vitte_c17_write_string(writer, "int");
    }
    if (type->kind != VITTE_AST_NODE_TYPE_NAME || type->as.type_name.name == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_TYPE", "unsupported C17 type node", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    name = type->as.type_name.name;
    if (strcmp(name, "int") == 0) {
        return vitte_c17_write_string(writer, "int");
    }
    if (strcmp(name, "i64") == 0) {
        return vitte_c17_write_string(writer, "int64_t");
    }
    if (strcmp(name, "bool") == 0) {
        return vitte_c17_write_string(writer, "bool");
    }
    if (strcmp(name, "string") == 0 || strcmp(name, "str") == 0) {
        return vitte_c17_write_string(writer, "const char *");
    }

    vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_TYPE", "unknown C17 bootstrap type", name);
    return VITTE_STATUS_ERROR_BACKEND;
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

static vitte_status_t vitte_c17_emit_identifier(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const char *name
) {
    char sanitized[128];
    vitte_status_t status;

    status = vitte_c17_sanitize_identifier(name, sanitized, sizeof(sanitized), &module->last_error);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    return vitte_c17_write_string(writer, sanitized);
}

static vitte_status_t vitte_c17_emit_expr_list(
    vitte_c17_module_t *module,
    vitte_c17_writer_t *writer,
    const vitte_ast_list_t *arguments
) {
    const vitte_ast_node_t *arg;
    bool first = true;

    if (arguments == NULL) {
        return VITTE_STATUS_OK;
    }

    for (arg = arguments->first; arg != NULL; arg = arg->next) {
        vitte_status_t status;
        if (!first) {
            status = vitte_c17_write_string(writer, ", ");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
        status = vitte_c17_emit_expr(module, writer, arg);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        first = false;
    }

    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_c17_emit_expr(vitte_c17_module_t *module, vitte_c17_writer_t *writer, const vitte_ast_expr_t *expr) {
    vitte_status_t status;

    if (expr == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_EXPR", "missing expression for C17 emission", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    switch (expr->kind) {
        case VITTE_AST_NODE_INTEGER_LITERAL:
            return vitte_c17_write_format(writer, "%" PRId64, expr->as.integer_literal.value);
        case VITTE_AST_NODE_STRING_LITERAL:
            return vitte_c17_emit_c_string(writer, expr->as.string_literal.value);
        case VITTE_AST_NODE_IDENTIFIER:
            return vitte_c17_emit_identifier(module, writer, expr->as.identifier.name);
        case VITTE_AST_NODE_BINARY_EXPR:
            if (!vitte_c17_operator_is_supported(expr->as.binary_expr.operator_text)) {
                vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_OPERATOR", "unsupported C17 binary operator", expr->as.binary_expr.operator_text);
                return VITTE_STATUS_ERROR_BACKEND;
            }
            status = vitte_c17_write_char(writer, '(');
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_expr(module, writer, expr->as.binary_expr.left);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_format(writer, " %s ", expr->as.binary_expr.operator_text);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_expr(module, writer, expr->as.binary_expr.right);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_c17_write_char(writer, ')');
        case VITTE_AST_NODE_CALL_EXPR:
            status = vitte_c17_emit_expr(module, writer, expr->as.call_expr.callee);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_char(writer, '(');
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_expr_list(module, writer, &expr->as.call_expr.arguments);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_c17_write_char(writer, ')');
        default:
            vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_EXPR", "unsupported expression node for C17 emission", vitte_ast_node_kind_name(expr->kind));
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

static vitte_status_t vitte_c17_emit_block(vitte_c17_module_t *module, vitte_c17_writer_t *writer, const vitte_ast_stmt_t *block) {
    const vitte_ast_node_t *stmt;
    vitte_status_t status;

    if (block == NULL || block->kind != VITTE_AST_NODE_BLOCK_STMT) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_BLOCK", "expected block statement for C17 emission", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    status = vitte_c17_write_open_block(writer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    for (stmt = block->as.block_stmt.statements.first; stmt != NULL; stmt = stmt->next) {
        status = vitte_c17_emit_stmt(module, writer, stmt);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }

    status = vitte_c17_write_close_block(writer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    return vitte_c17_write_newline(writer);
}

static vitte_status_t vitte_c17_emit_stmt(vitte_c17_module_t *module, vitte_c17_writer_t *writer, const vitte_ast_stmt_t *stmt) {
    vitte_status_t status;

    if (stmt == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_STMT", "missing statement for C17 emission", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    switch (stmt->kind) {
        case VITTE_AST_NODE_BLOCK_STMT:
            return vitte_c17_emit_block(module, writer, stmt);
        case VITTE_AST_NODE_GIVE_STMT:
            status = vitte_c17_write_string(writer, "return");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            if (stmt->as.give_stmt.value != NULL) {
                status = vitte_c17_write_char(writer, ' ');
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
                status = vitte_c17_emit_expr(module, writer, stmt->as.give_stmt.value);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_AST_NODE_LET_STMT:
            status = vitte_c17_emit_type(module, writer, stmt->as.let_stmt.type);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_char(writer, ' ');
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_identifier(module, writer, stmt->as.let_stmt.name);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            if (stmt->as.let_stmt.value != NULL) {
                status = vitte_c17_write_string(writer, " = ");
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
                status = vitte_c17_emit_expr(module, writer, stmt->as.let_stmt.value);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            return vitte_c17_emit_statement_line_end(writer);
        case VITTE_AST_NODE_IF_STMT:
            status = vitte_c17_write_string(writer, "if (");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_expr(module, writer, stmt->as.if_stmt.condition);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_write_string(writer, ") ");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_c17_emit_stmt(module, writer, stmt->as.if_stmt.then_branch);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            if (stmt->as.if_stmt.else_branch != NULL) {
                status = vitte_c17_write_string(writer, "else ");
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
                status = vitte_c17_emit_stmt(module, writer, stmt->as.if_stmt.else_branch);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            return VITTE_STATUS_OK;
        default:
            vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_STMT", "unsupported statement node for C17 emission", vitte_ast_node_kind_name(stmt->kind));
            return VITTE_STATUS_ERROR_BACKEND;
    }
}

static vitte_status_t vitte_c17_emit_const_decl(vitte_c17_module_t *module, vitte_c17_writer_t *writer, const vitte_ast_decl_t *decl) {
    vitte_status_t status = vitte_c17_write_string(writer, "static const ");

    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_emit_type(module, writer, decl->as.const_decl.type);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_char(writer, ' ');
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_emit_identifier(module, writer, decl->as.const_decl.name);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_string(writer, " = ");
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_emit_expr(module, writer, decl->as.const_decl.value);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    module->unit->declaration_count++;
    return vitte_c17_emit_statement_line_end(writer);
}

static vitte_status_t vitte_c17_emit_proc_decl(vitte_c17_module_t *module, vitte_c17_writer_t *writer, const vitte_ast_decl_t *decl) {
    vitte_status_t status = vitte_c17_emit_type(module, writer, decl->as.proc_decl.return_type);

    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_char(writer, ' ');
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_emit_identifier(module, writer, decl->as.proc_decl.name);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_write_string(writer, "(void) ");
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_c17_emit_block(module, writer, decl->as.proc_decl.body);
    if (status == VITTE_STATUS_OK) {
        module->unit->function_count++;
    }
    return status;
}

vitte_status_t vitte_c17_module_emit(vitte_c17_module_t *module, vitte_c17_writer_t *writer) {
    const vitte_ast_node_t *decl;
    vitte_status_t status;

    if (module == NULL || writer == NULL || module->unit == NULL) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_MODULE", "missing C17 module, unit, or writer", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (module->ast_module == NULL || module->ast_module->kind != VITTE_AST_NODE_MODULE) {
        vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_MODULE", "C17 backend expected AST module root", NULL);
        return VITTE_STATUS_ERROR_BACKEND;
    }

    status = vitte_c17_translation_unit_emit_prelude(module->unit, writer);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    for (decl = module->ast_module->as.module.declarations.first; decl != NULL; decl = decl->next) {
        switch (decl->kind) {
            case VITTE_AST_NODE_CONST_DECL:
                status = vitte_c17_emit_const_decl(module, writer, decl);
                break;
            case VITTE_AST_NODE_PROC_DECL:
                status = vitte_c17_emit_proc_decl(module, writer, decl);
                break;
            default:
                vitte_c17_module_set_error(module, VITTE_STATUS_ERROR_BACKEND, "VITTE_C17_E_DECL", "unsupported declaration node for C17 emission", vitte_ast_node_kind_name(decl->kind));
                return VITTE_STATUS_ERROR_BACKEND;
        }

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
