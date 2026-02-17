#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"
#include "types.hpp"

#include <string>
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vitte::frontend::resolve {

enum class SymbolKind {
    Proc,
    Entry,
    Form,
    Pick,
    Var,
    Param,
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    ast::SourceSpan span;
};

using SymbolId = std::size_t;

class SymbolTable {
public:
    SymbolId define(Symbol sym);
    const Symbol* lookup(const std::string& name) const;

    void push_scope();
    void pop_scope();

    const std::vector<Symbol>& symbols() const { return symbols_; }
    std::vector<std::string> in_scope_names(std::size_t limit = 0) const;

private:
    std::vector<Symbol> symbols_;
    std::vector<std::unordered_map<std::string, SymbolId>> scopes_;
};

class Resolver {
public:
    explicit Resolver(diag::DiagnosticEngine& diagnostics,
                      bool strict_types = false,
                      bool strict_imports = false,
                      bool strict_modules = false);

    bool resolve_module(ast::AstContext& ctx, ast::ModuleId module);
    const SymbolTable& symbols() const { return symbols_; }
    types::TypeId type_id(ast::TypeId node) const;

private:
    void resolve_decl(ast::AstContext& ctx, ast::DeclId decl);
    void resolve_stmt(ast::AstContext& ctx, ast::StmtId stmt);
    void resolve_expr(ast::AstContext& ctx, ast::ExprId expr);

    void define_builtin_types();
    types::TypeId resolve_type(ast::AstContext& ctx, ast::TypeId type);

    SymbolTable symbols_;
    types::TypeTable types_;
    diag::DiagnosticEngine& diag_;
    bool strict_types_ = false;
    bool strict_imports_ = false;
    bool strict_modules_ = false;
    std::unordered_map<ast::TypeId, types::TypeId> resolved_types_;
    std::unordered_map<std::string, ast::SourceSpan> explicit_imports_;
    std::unordered_set<std::string> used_explicit_imports_;
};

const char* to_string(SymbolKind kind);
void dump_symbols(const SymbolTable& table, std::ostream& os);
const std::vector<std::string>& builtin_type_names();
const std::vector<std::string>& canonical_builtin_type_names();

} // namespace vitte::frontend::resolve
