#pragma once
#include <ostream>
#include <string>

#include "../ast/cpp_decl.hpp"
#include "../ast/cpp_stmt.hpp"
#include "../ast/cpp_expr.hpp"
#include "../context/cpp_context.hpp"

namespace vitte::backend::emit {

/* -------------------------------------------------
 * Expressions
 * ------------------------------------------------- */
void emit_expr(
    std::ostream& os,
    const ast::cpp::CppExpr& expr
);

/* -------------------------------------------------
 * Statements
 * ------------------------------------------------- */
void emit_stmt(
    std::ostream& os,
    const ast::cpp::CppStmt& stmt,
    int indent_level = 0
);

/* -------------------------------------------------
 * Declarations
 * ------------------------------------------------- */
void emit_function(
    std::ostream& os,
    const ast::cpp::CppFunction& fn,
    int indent_level = 0
);

void emit_struct(
    std::ostream& os,
    const ast::cpp::CppStruct& s,
    int indent_level = 0
);

void emit_enum(
    std::ostream& os,
    const ast::cpp::CppEnum& e,
    int indent_level = 0
);

void emit_global(
    std::ostream& os,
    const ast::cpp::CppGlobal& g,
    int indent_level = 0
);

void emit_namespace(
    std::ostream& os,
    const ast::cpp::CppNamespace& ns,
    int indent_level = 0
);

void emit_translation_unit(
    std::ostream& os,
    const ast::cpp::CppTranslationUnit& tu,
    context::CppContext& ctx
);

/* -------------------------------------------------
 * File emission
 * ------------------------------------------------- */
bool emit_file(
    const std::string& path,
    const ast::cpp::CppTranslationUnit& tu,
    context::CppContext& ctx
);

} // namespace vitte::backend::emit
