#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vitte::linker {

/* -------------------------------------------------
 * Symbol kind
 * ------------------------------------------------- */
enum class SymbolKind {
    Defined,
    Undefined
};

/* -------------------------------------------------
 * Symbol information
 * ------------------------------------------------- */
struct Symbol {
    std::string name;
    SymbolKind kind;
    std::string object; // object file where it appears
};

/* -------------------------------------------------
 * Symbol table
 * ------------------------------------------------- */
class SymbolTable {
public:
    /* Register a defined symbol */
    void define(
        const std::string& name,
        const std::string& object
    );

    /* Register an undefined symbol */
    void declare(
        const std::string& name,
        const std::string& object
    );

    /* Check for unresolved symbols */
    bool has_unresolved() const;

    /* Return list of unresolved symbol names */
    std::vector<std::string> unresolved() const;

    /* Diagnostics dump */
    void dump() const;

private:
    std::unordered_map<std::string, Symbol> defined;
    std::unordered_set<std::string> undefined;
};

} // namespace vitte::linker
