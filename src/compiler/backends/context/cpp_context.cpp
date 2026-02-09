#include "cpp_context.hpp"

#include <sstream>
#include <algorithm>
#include <unordered_set>

namespace vitte::backend::context {

/* ----------------------------------------
 * Construction
 * ---------------------------------------- */

CppContext::CppContext() = default;

/* ----------------------------------------
 * Namespace management
 * ---------------------------------------- */

void CppContext::push_namespace(const std::string& ns) {
    namespace_stack.push_back(ns);
}

void CppContext::pop_namespace() {
    if (!namespace_stack.empty())
        namespace_stack.pop_back();
}

std::string CppContext::current_namespace() const {
    if (namespace_stack.empty())
        return {};

    std::ostringstream oss;
    for (size_t i = 0; i < namespace_stack.size(); ++i) {
        if (i) oss << "::";
        oss << namespace_stack[i];
    }
    return oss.str();
}

/* ----------------------------------------
 * Symbol table
 * ---------------------------------------- */

void CppContext::register_symbol(
    const std::string& vitte_name,
    const std::string& cpp_name
) {
    symbols[vitte_name] = cpp_name;
}

bool CppContext::has_symbol(const std::string& vitte_name) const {
    return symbols.find(vitte_name) != symbols.end();
}

std::string CppContext::resolve_symbol(const std::string& vitte_name) const {
    auto it = symbols.find(vitte_name);
    if (it != symbols.end())
        return it->second;
    return vitte_name;
}

/* ----------------------------------------
 * Type registry
 * ---------------------------------------- */

void CppContext::register_type(
    const std::string& vitte_name,
    ast::cpp::CppType* cpp_type
) {
    types[vitte_name] = cpp_type;
}

ast::cpp::CppType* CppContext::resolve_type(
    const std::string& vitte_name
) const {
    auto it = types.find(vitte_name);
    if (it != types.end())
        return it->second;
    return nullptr;
}

/* ----------------------------------------
 * Name mangling
 * ---------------------------------------- */

std::string CppContext::mangle(
    const std::string& base,
    const std::vector<std::string>& params
) const {
    std::ostringstream oss;

    // prefix
    oss << "vitte__";

    // namespace
    for (const auto& ns : namespace_stack) {
        oss << ns << "__";
    }

    // base symbol
    oss << base;

    // parameters (for overloads / generics)
    if (!params.empty()) {
        oss << "__";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i) oss << "_";
            oss << params[i];
        }
    }

    return oss.str();
}

std::string CppContext::safe_ident(const std::string& name) const {
    static const std::unordered_set<std::string> kKeywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit",
        "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch",
        "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const",
        "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await",
        "co_return", "co_yield", "decltype", "default", "delete", "do", "double",
        "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
        "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable",
        "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or",
        "or_eq", "private", "protected", "public", "register", "reinterpret_cast",
        "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
        "static_cast", "struct", "switch", "synchronized", "template", "this",
        "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
        "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
        "while", "xor", "xor_eq"
    };

    std::string out;
    out.reserve(name.size());
    for (char ch : name) {
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_') {
            out.push_back(ch);
        } else {
            out.push_back('_');
        }
    }

    if (out.empty()) {
        out = "_";
    }

    if ((out[0] >= '0' && out[0] <= '9')) {
        out.insert(out.begin(), '_');
    }

    if (kKeywords.count(out) > 0) {
        out += "_";
    }

    return out;
}

/* ----------------------------------------
 * Include management
 * ---------------------------------------- */

void CppContext::add_include(const std::string& inc) {
    if (std::find(includes.begin(), includes.end(), inc) == includes.end())
        includes.push_back(inc);
}

const std::vector<std::string>& CppContext::get_includes() const {
    return includes;
}

/* ----------------------------------------
 * Emission options
 * ---------------------------------------- */

void CppContext::set_debug(bool v) {
    debug = v;
}

void CppContext::set_optimize(bool v) {
    optimize = v;
}

bool CppContext::is_debug() const {
    return debug;
}

bool CppContext::is_optimize() const {
    return optimize;
}

void CppContext::set_repro_strict(bool v) {
    repro_strict_ = v;
}

bool CppContext::repro_strict() const {
    return repro_strict_;
}

/* ----------------------------------------
 * Entry mode
 * ---------------------------------------- */

void CppContext::set_entry_mode(EntryMode mode) {
    entry_mode_ = mode;
}

CppContext::EntryMode CppContext::entry_mode() const {
    return entry_mode_;
}

} // namespace vitte::backend::context
