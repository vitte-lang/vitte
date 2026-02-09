#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "../ast/cpp_type.hpp"

namespace vitte::backend::context {

/* -------------------------------------------------
 * C++ backend global context
 * ------------------------------------------------- */
class CppContext {
public:
    CppContext();

    /* ---------------------------------------------
     * Namespace stack (module path → C++)
     * --------------------------------------------- */
    void push_namespace(const std::string& ns);
    void pop_namespace();
    std::string current_namespace() const;

    /* ---------------------------------------------
     * Symbol table (Vitte → C++)
     * --------------------------------------------- */
    void register_symbol(
        const std::string& vitte_name,
        const std::string& cpp_name
    );

    bool has_symbol(const std::string& vitte_name) const;

    std::string resolve_symbol(
        const std::string& vitte_name
    ) const;

    /* ---------------------------------------------
     * Type registry (Vitte → CppType)
     * --------------------------------------------- */
    void register_type(
        const std::string& vitte_name,
        ast::cpp::CppType* cpp_type
    );

    ast::cpp::CppType* resolve_type(
        const std::string& vitte_name
    ) const;

    /* ---------------------------------------------
     * Name mangling
     * --------------------------------------------- */
    std::string mangle(
        const std::string& base,
        const std::vector<std::string>& params = {}
    ) const;

    /* ---------------------------------------------
     * Include management
     * --------------------------------------------- */
    void add_include(const std::string& include);
    const std::vector<std::string>& get_includes() const;

    /* ---------------------------------------------
     * Emission options
     * --------------------------------------------- */
    void set_debug(bool v);
    void set_optimize(bool v);

    bool is_debug() const;
    bool is_optimize() const;

    /* Deterministic lowering */
    void set_repro_strict(bool v);
    bool repro_strict() const;

    /* ---------------------------------------------
     * Entry point mode
     * --------------------------------------------- */
    enum class EntryMode {
        Native,
        Arduino,
        Freestanding
    };

    void set_entry_mode(EntryMode mode);
    EntryMode entry_mode() const;

private:
    /* Namespace stack */
    std::vector<std::string> namespace_stack;

    /* Symbol table */
    std::unordered_map<std::string, std::string> symbols;

    /* Type registry */
    std::unordered_map<std::string, ast::cpp::CppType*> types;

    /* Includes */
    std::vector<std::string> includes;

    /* Options */
    bool debug = false;
    bool optimize = false;
    bool repro_strict_ = false;
    EntryMode entry_mode_ = EntryMode::Native;
};

} // namespace vitte::backend::context
