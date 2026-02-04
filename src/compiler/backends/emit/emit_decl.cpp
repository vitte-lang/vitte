#include "emit_decl.hpp"

#include "../ast/cpp_decl.hpp"
#include "../ast/cpp_type.hpp"
#include "../ast/cpp_stmt.hpp"
#include "../context/cpp_context.hpp"

#include <ostream>

namespace vitte::backend::emit {

/* -------------------------------------------------
 * Helpers
 * ------------------------------------------------- */

static void indent(std::ostream& os, int level) {
    for (int i = 0; i < level; ++i)
        os << "    ";
}

/* -------------------------------------------------
 * Type emission (forward – real impl in emit_type)
 * ------------------------------------------------- */

static void emit_type(
    std::ostream& os,
    const ast::cpp::CppType& type
) {
    os << type.name;
}

/* -------------------------------------------------
 * Forward decls
 * ------------------------------------------------- */

static void emit_stmt(
    std::ostream& os,
    const ast::cpp::CppStmt& stmt,
    int indent_level
);

/* -------------------------------------------------
 * Function
 * ------------------------------------------------- */

void emit_function(
    std::ostream& os,
    const ast::cpp::CppFunction& fn,
    int indent_level
) {
    indent(os, indent_level);

    if (fn.is_inline)
        os << "inline ";

    emit_type(os, fn.return_type);
    os << " " << fn.name << "(";

    for (size_t i = 0; i < fn.params.size(); ++i) {
        const auto& p = fn.params[i];
        emit_type(os, *p.type);
        os << " " << p.name;
        if (i + 1 < fn.params.size())
            os << ", ";
    }

    os << ") {\n";

    for (const auto& stmt : fn.body)
        emit_stmt(os, *stmt, indent_level + 1);

    indent(os, indent_level);
    os << "}\n\n";
}

/* -------------------------------------------------
 * Global variable
 * ------------------------------------------------- */

void emit_global(
    std::ostream& os,
    const ast::cpp::CppGlobal& g,
    int indent_level
) {
    indent(os, indent_level);

    if (g.is_const)
        os << "const ";

    emit_type(os, *g.type);
    os << " " << g.name;

    if (g.init) {
        os << " = ";
        os << (*g.init)->value;
    }

    os << ";\n";
}

/* -------------------------------------------------
 * Struct
 * ------------------------------------------------- */

void emit_struct(
    std::ostream& os,
    const ast::cpp::CppStruct& s,
    int indent_level
) {
    indent(os, indent_level);
    os << "struct " << s.name << " {\n";

    for (const auto& f : s.fields) {
        indent(os, indent_level + 1);
        emit_type(os, *f.type);
        os << " " << f.name << ";\n";
    }

    indent(os, indent_level);
    os << "};\n\n";
}

/* -------------------------------------------------
 * Enum
 * ------------------------------------------------- */

void emit_enum(
    std::ostream& os,
    const ast::cpp::CppEnum& e,
    int indent_level
) {
    indent(os, indent_level);
    os << "enum class " << e.name << " {\n";

    for (size_t i = 0; i < e.items.size(); ++i) {
        const auto& it = e.items[i];
        indent(os, indent_level + 1);
        os << it.name;
        if (it.value)
            os << " = " << *it.value;
        if (i + 1 < e.items.size())
            os << ",";
        os << "\n";
    }

    indent(os, indent_level);
    os << "};\n\n";
}

/* -------------------------------------------------
 * Namespace
 * ------------------------------------------------- */

void emit_namespace(
    std::ostream& os,
    const ast::cpp::CppNamespace& ns,
    int indent_level
) {
    indent(os, indent_level);
    os << "namespace " << ns.name << " {\n\n";

    for (const auto& s : ns.structs)
        emit_struct(os, s, indent_level + 1);

    for (const auto& e : ns.enums)
        emit_enum(os, e, indent_level + 1);

    for (const auto& g : ns.globals)
        emit_global(os, g, indent_level + 1);

    for (const auto& f : ns.functions)
        emit_function(os, f, indent_level + 1);

    indent(os, indent_level);
    os << "} // namespace " << ns.name << "\n\n";
}

/* -------------------------------------------------
 * Translation unit
 * ------------------------------------------------- */

void emit_translation_unit(
    std::ostream& os,
    const ast::cpp::CppTranslationUnit& tu,
    context::CppContext& ctx
) {
    for (const auto& inc : ctx.get_includes())
        os << "#include " << inc << "\n";

    if (!ctx.get_includes().empty())
        os << "\n";

    for (const auto& ns : tu.namespaces)
        emit_namespace(os, ns, 0);

    for (const auto& s : tu.structs)
        emit_struct(os, s, 0);

    for (const auto& e : tu.enums)
        emit_enum(os, e, 0);

    for (const auto& g : tu.globals)
        emit_global(os, g, 0);

    for (const auto& f : tu.functions)
        emit_function(os, f, 0);
}

/* -------------------------------------------------
 * Statement emission (minimal bridge)
 * ------------------------------------------------- */

static void emit_stmt(
    std::ostream& os,
    const ast::cpp::CppStmt& stmt,
    int indent_level
) {
    using K = ast::cpp::CppStmt::Kind;

    indent(os, indent_level);

    switch (stmt.kind) {
        case K::Return: {
            auto& r = static_cast<const ast::cpp::CppReturn&>(stmt);
            os << "return";
            if (r.value) {
                os << " ";
                os << (*r.value)->value;
            }
            os << ";\n";
            break;
        }

        case K::Expr: {
            auto& e = static_cast<const ast::cpp::CppExprStmt&>(stmt);
            os << e.expr->value << ";\n";
            break;
        }

        default:
            os << "/* unsupported stmt */\n";
            break;
    }
}

} // namespace vitte::backend::emit
