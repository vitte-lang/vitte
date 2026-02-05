#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"
#include "types.hpp"

#include <string>
#include <ostream>
#include <unordered_map>
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

private:
    std::vector<Symbol> symbols_;
    std::vector<std::unordered_map<std::string, SymbolId>> scopes_;
};

class Resolver {
public:
    explicit Resolver(diag::DiagnosticEngine& diagnostics);

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
    std::unordered_map<ast::TypeId, types::TypeId> resolved_types_;
};

const char* to_string(SymbolKind kind);
void dump_symbols(const SymbolTable& table, std::ostream& os);

} // namespace vitte::frontend::resolve
