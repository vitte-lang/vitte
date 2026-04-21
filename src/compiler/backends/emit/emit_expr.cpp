#include "emit.hpp"

#include <ostream>

namespace vitte::backend::emit {

/* -------------------------------------------------
 * Forward
 * ------------------------------------------------- */
static void emit_expr_impl(
    std::ostream& os,
    const ast::cpp::CppExpr& expr
);

/* -------------------------------------------------
 * Entry point
 * ------------------------------------------------- */
void emit_expr(
    std::ostream& os,
    const ast::cpp::CppExpr& expr
) {
    emit_expr_impl(os, expr);
}

/* -------------------------------------------------
 * Implementation
 * ------------------------------------------------- */
static void emit_expr_impl(
    std::ostream& os,
    const ast::cpp::CppExpr& expr
) {
    using K = ast::cpp::CppExpr::Kind;

    switch (expr.kind) {

    case K::Literal: {
        auto& e = static_cast<const ast::cpp::CppLiteral&>(expr);
        os << e.value;
        break;
    }

    case K::Variable: {
        auto& e = static_cast<const ast::cpp::CppVar&>(expr);
        os << e.name;
        break;
    }

    case K::Unary: {
        auto& e = static_cast<const ast::cpp::CppUnary&>(expr);
        os << e.op;
        emit_expr_impl(os, *e.expr);
        break;
    }

    case K::Binary: {
        auto& e = static_cast<const ast::cpp::CppBinary&>(expr);
        os << "(";
        emit_expr_impl(os, *e.lhs);
        os << " " << e.op << " ";
        emit_expr_impl(os, *e.rhs);
        os << ")";
        break;
    }

    case K::Call: {
        auto& e = static_cast<const ast::cpp::CppCall&>(expr);
        if (e.callee_expr) {
            emit_expr_impl(os, *e.callee_expr);
        } else {
            os << e.callee;
        }
        os << "(";
        bool first = true;
        for (const auto& arg : e.args) {
            if (!first) {
                os << ", ";
            }
            first = false;
            emit_expr_impl(os, *arg);
        }
        os << ")";
        break;
    }

    case K::Cast: {
        auto& e = static_cast<const ast::cpp::CppCast&>(expr);
        os << "static_cast<" << e.target_type->name << ">(";
        emit_expr_impl(os, *e.expr);
        os << ")";
        break;
    }

    case K::Member: {
        auto& e = static_cast<const ast::cpp::CppMember&>(expr);
        emit_expr_impl(os, *e.base);
        os << (e.pointer ? "->" : ".");
        os << e.member;
        break;
    }

    case K::Index: {
        auto& e = static_cast<const ast::cpp::CppIndex&>(expr);
        emit_expr_impl(os, *e.base);
        os << "[";
        emit_expr_impl(os, *e.index);
        os << "]";
        break;
    }

    case K::Lambda: {
        auto& e = static_cast<const ast::cpp::CppLambda&>(expr);
        os << "[&]() {\n";
        for (const auto& st : e.body) {
            emit_stmt(os, *st, 1);
        }
        os << "}";
        break;
    }

    default:
        os << "/* <unknown expr> */";
        break;
    }
}

} // namespace vitte::backend::emit
