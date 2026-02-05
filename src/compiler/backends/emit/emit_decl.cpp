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

/* -------------------------------------------------
 * Type emission (forward – real impl in emit_type)
 * ------------------------------------------------- */

static void emit_type(
    std::ostream& os,
    const ast::cpp::CppType* type
) {
    if (!type) {
        os << "<unknown>";
        return;
    }
    os << type->name;
}

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
        emit_type(os, p.type);
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

    emit_type(os, g.type);
    os << " " << g.name;

    if (g.init) {
        os << " = ";
        emit_expr(os, *(*g.init));
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
        emit_type(os, f.type);
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

} // namespace vitte::backend::emit
