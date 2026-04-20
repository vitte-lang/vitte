#include "emit.hpp"

#include <ostream>

namespace vitte::backend::emit {

/* -------------------------------------------------
 * Helpers
 * ------------------------------------------------- */
static void indent(std::ostream& os, int level) {
    for (int i = 0; i < level; ++i)
        os << "    ";
}

static void emit_type(
    std::ostream& os,
    const ast::cpp::CppType* type
) {
    if (!type) {
        os << "<unknown>";
        return;
    }
    if (type->kind == ast::cpp::CppTypeKind::Pointer) {
        emit_type(os, type->pointee);
        os << "*";
        return;
    }
    os << type->name;
}

/* Forward */
static void emit_stmt_impl(
    std::ostream& os,
    const ast::cpp::CppStmt& stmt,
    int indent_level
);

static void emit_cpp_string(std::ostream& os, const std::string& value) {
    os << "\"";
    for (char c : value) {
        switch (c) {
            case '\\': os << "\\\\"; break;
            case '"': os << "\\\""; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default: os << c; break;
        }
    }
    os << "\"";
}

static void emit_stmt_inline(
    std::ostream& os,
    const ast::cpp::CppStmt& stmt
) {
    using K = ast::cpp::CppStmt::Kind;

    switch (stmt.kind) {
    case K::Expr: {
        auto& s = static_cast<const ast::cpp::CppExprStmt&>(stmt);
        emit_expr(os, *s.expr);
        break;
    }
    case K::Assign: {
        auto& s = static_cast<const ast::cpp::CppAssign&>(stmt);
        emit_expr(os, *s.lhs);
        os << " = ";
        emit_expr(os, *s.rhs);
        break;
    }
    case K::Decl: {
        auto& s = static_cast<const ast::cpp::CppVarDecl&>(stmt);
        if (s.is_const) {
            os << "const ";
        }
        if (s.type && s.type->kind == ast::cpp::CppTypeKind::Function) {
            emit_type(os, s.type->return_type);
            os << " (*" << s.name << ")(";
            bool first = true;
            for (const auto* param_type : s.type->param_types) {
                if (!first) {
                    os << ", ";
                }
                first = false;
                emit_type(os, param_type);
            }
            os << ")";
        } else {
            emit_type(os, s.type);
            os << " " << s.name;
        }
        if (s.init) {
            os << " = ";
            emit_expr(os, *(*s.init));
        }
        break;
    }
    default:
        emit_stmt_impl(os, stmt, 0);
        break;
    }
}

/* -------------------------------------------------
 * Entry point
 * ------------------------------------------------- */
void emit_stmt(
    std::ostream& os,
    const ast::cpp::CppStmt& stmt,
    int indent_level
) {
    emit_stmt_impl(os, stmt, indent_level);
}

/* -------------------------------------------------
 * Implementation
 * ------------------------------------------------- */
static void emit_stmt_impl(
    std::ostream& os,
    const ast::cpp::CppStmt& stmt,
    int indent_level
) {
    using K = ast::cpp::CppStmt::Kind;

    switch (stmt.kind) {

    case K::Expr: {
        auto& s = static_cast<const ast::cpp::CppExprStmt&>(stmt);
        indent(os, indent_level);
        emit_expr(os, *s.expr);
        os << ";\n";
        break;
    }

    case K::Asm: {
        auto& s = static_cast<const ast::cpp::CppAsm&>(stmt);
        indent(os, indent_level);
        os << "asm ";
        if (s.is_volatile) {
            os << "volatile ";
        }
        os << "(";
        emit_cpp_string(os, s.code);
        os << ");\n";
        break;
    }

    case K::Decl: {
        auto& s = static_cast<const ast::cpp::CppVarDecl&>(stmt);
        indent(os, indent_level);
        if (s.is_const)
            os << "const ";
        if (s.type && s.type->kind == ast::cpp::CppTypeKind::Function) {
            emit_type(os, s.type->return_type);
            os << " (*" << s.name << ")(";
            bool first = true;
            for (const auto* param_type : s.type->param_types) {
                if (!first) {
                    os << ", ";
                }
                first = false;
                emit_type(os, param_type);
            }
            os << ")";
        } else {
            emit_type(os, s.type);
            os << " " << s.name;
        }
        if (s.init) {
            os << " = ";
            emit_expr(os, *(*s.init));
        }
        os << ";\n";
        break;
    }

    case K::Assign: {
        auto& s = static_cast<const ast::cpp::CppAssign&>(stmt);
        indent(os, indent_level);
        emit_expr(os, *s.lhs);
        os << " = ";
        emit_expr(os, *s.rhs);
        os << ";\n";
        break;
    }

    case K::Label: {
        auto& s = static_cast<const ast::cpp::CppLabel&>(stmt);
        indent(os, indent_level);
        os << s.name << ":\n";
        break;
    }

    case K::Goto: {
        auto& s = static_cast<const ast::cpp::CppGoto&>(stmt);
        indent(os, indent_level);
        os << "goto " << s.target << ";\n";
        break;
    }

    case K::Return: {
        auto& s = static_cast<const ast::cpp::CppReturn&>(stmt);
        indent(os, indent_level);
        os << "return";
        if (s.value) {
            os << " ";
            emit_expr(os, *(*s.value));
        }
        os << ";\n";
        break;
    }

    case K::If: {
        auto& s = static_cast<const ast::cpp::CppIf&>(stmt);
        indent(os, indent_level);
        os << "if (";
        emit_expr(os, *s.condition);
        os << ") {\n";

        for (const auto& st : s.then_body)
            emit_stmt_impl(os, *st, indent_level + 1);

        indent(os, indent_level);
        os << "}";

        if (!s.else_body.empty()) {
            os << " else {\n";
            for (const auto& st : s.else_body)
                emit_stmt_impl(os, *st, indent_level + 1);
            indent(os, indent_level);
            os << "}";
        }

        os << "\n";
        break;
    }

    case K::While: {
        auto& s = static_cast<const ast::cpp::CppWhile&>(stmt);
        indent(os, indent_level);
        os << "while (";
        emit_expr(os, *s.condition);
        os << ") {\n";

        for (const auto& st : s.body)
            emit_stmt_impl(os, *st, indent_level + 1);

        indent(os, indent_level);
        os << "}\n";
        break;
    }

    case K::For: {
        auto& s = static_cast<const ast::cpp::CppFor&>(stmt);
        indent(os, indent_level);
        os << "for (";

        if (s.init) {
            emit_stmt_inline(os, *s.init);
        } else {
            os << ";";
        }

        if (s.condition) {
            os << " ";
            emit_expr(os, *s.condition);
        }
        os << "; ";

        if (s.step) {
            emit_stmt_inline(os, *s.step);
        }

        os << ") {\n";

        for (const auto& st : s.body)
            emit_stmt_impl(os, *st, indent_level + 1);

        indent(os, indent_level);
        os << "}\n";
        break;
    }

    case K::Break: {
        indent(os, indent_level);
        os << "break;\n";
        break;
    }

    case K::Continue: {
        indent(os, indent_level);
        os << "continue;\n";
        break;
    }

    case K::Block: {
        auto& s = static_cast<const ast::cpp::CppBlock&>(stmt);
        indent(os, indent_level);
        os << "{\n";
        for (const auto& st : s.stmts)
            emit_stmt_impl(os, *st, indent_level + 1);
        indent(os, indent_level);
        os << "}\n";
        break;
    }

    default:
        indent(os, indent_level);
        os << "/* <unsupported stmt> */\n";
        break;
    }
}

} // namespace vitte::backend::emit
