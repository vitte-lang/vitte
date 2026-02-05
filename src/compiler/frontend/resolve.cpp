#include "resolve.hpp"
#include "diagnostics_messages.hpp"

namespace vitte::frontend::resolve {

using namespace vitte::frontend::ast;

SymbolId SymbolTable::define(Symbol sym) {
    if (scopes_.empty()) {
        scopes_.emplace_back();
    }
    SymbolId id = symbols_.size();
    symbols_.push_back(std::move(sym));
    scopes_.back()[symbols_.back().name] = id;
    return id;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &symbols_[found->second];
        }
    }
    return nullptr;
}

void SymbolTable::push_scope() {
    scopes_.emplace_back();
}

void SymbolTable::pop_scope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
    }
}

Resolver::Resolver(diag::DiagnosticEngine& diagnostics)
    : diag_(diagnostics) {
    symbols_.push_scope();
    define_builtin_types();
}

void Resolver::define_builtin_types() {
    types_.add_builtin("bool");
    types_.add_builtin("string");
    types_.add_builtin("int");
    symbols_.define({"bool", SymbolKind::Form, {}});
    symbols_.define({"string", SymbolKind::Form, {}});
    symbols_.define({"int", SymbolKind::Form, {}});
}

bool Resolver::resolve_module(ast::AstContext& ctx, ast::ModuleId module_id) {
    if (module_id == kInvalidAstId) {
        return false;
    }
    auto& module = ctx.get<Module>(module_id);
    for (auto decl_id : module.decls) {
        if (decl_id != kInvalidAstId) {
            resolve_decl(ctx, decl_id);
        }
    }
    return !diag_.has_errors();
}

types::TypeId Resolver::type_id(ast::TypeId node) const {
    auto it = resolved_types_.find(node);
    if (it == resolved_types_.end()) {
        return static_cast<types::TypeId>(-1);
    }
    return it->second;
}

types::TypeId Resolver::resolve_type(ast::AstContext& ctx, ast::TypeId type) {
    if (type == kInvalidAstId) {
        return static_cast<types::TypeId>(-1);
    }

    auto it = resolved_types_.find(type);
    if (it != resolved_types_.end()) {
        return it->second;
    }

    AstNode& node = ctx.node(type);
    switch (node.kind) {
        case NodeKind::BuiltinType: {
            auto& t = static_cast<const BuiltinType&>(node);
            auto id = types_.lookup(t.name);
            resolved_types_[type] = id;
            return id;
        }
        case NodeKind::NamedType: {
            auto& t = static_cast<const NamedType&>(node);
            auto id = types_.lookup(t.ident.name);
            if (id == static_cast<types::TypeId>(-1)) {
                diag::error(diag_, diag::DiagId::UnknownType, t.ident.span);
            }
            resolved_types_[type] = id;
            return id;
        }
        case NodeKind::GenericType: {
            auto& t = static_cast<const GenericType&>(node);
            auto id = types_.lookup(t.base_ident.name);
            if (id == static_cast<types::TypeId>(-1)) {
                diag::error(diag_, diag::DiagId::UnknownGenericBaseType, t.base_ident.span);
            }
            if (t.type_args.empty()) {
                diag::error(diag_, diag::DiagId::GenericTypeRequiresAtLeastOneArgument, t.base_ident.span);
            }
            for (auto arg : t.type_args) {
                resolve_type(ctx, arg);
            }
            resolved_types_[type] = id;
            return id;
        }
        case NodeKind::PointerType: {
            auto& t = static_cast<const PointerType&>(node);
            resolve_type(ctx, t.pointee);
            resolved_types_[type] = static_cast<types::TypeId>(-1);
            return static_cast<types::TypeId>(-1);
        }
        case NodeKind::SliceType: {
            auto& t = static_cast<const SliceType&>(node);
            resolve_type(ctx, t.element);
            resolved_types_[type] = static_cast<types::TypeId>(-1);
            return static_cast<types::TypeId>(-1);
        }
        case NodeKind::ProcType: {
            auto& t = static_cast<const ProcType&>(node);
            for (auto p : t.params) {
                resolve_type(ctx, p);
            }
            resolve_type(ctx, t.return_type);
            resolved_types_[type] = static_cast<types::TypeId>(-1);
            return static_cast<types::TypeId>(-1);
        }
        default:
            diag::error(diag_, diag::DiagId::UnsupportedType, node.span);
            return static_cast<types::TypeId>(-1);
    }
}

void Resolver::resolve_decl(ast::AstContext& ctx, ast::DeclId decl_id) {
    auto& decl = ctx.get<Decl>(decl_id);
    switch (decl.kind) {
        case NodeKind::FormDecl: {
            auto& d = static_cast<FormDecl&>(decl);
            types_.add_named(d.name.name);
            symbols_.define({d.name.name, SymbolKind::Form, d.span});
            for (const auto& f : d.fields) {
                resolve_type(ctx, f.type);
            }
            break;
        }
        case NodeKind::TypeAliasDecl: {
            auto& d = static_cast<TypeAliasDecl&>(decl);
            types_.add_named(d.name.name);
            symbols_.define({d.name.name, SymbolKind::Form, d.span});
            resolve_type(ctx, d.target);
            break;
        }
        case NodeKind::PickDecl: {
            auto& d = static_cast<PickDecl&>(decl);
            types_.add_named(d.name.name);
            symbols_.define({d.name.name, SymbolKind::Pick, d.span});
            for (const auto& c : d.cases) {
                for (const auto& f : c.fields) {
                    resolve_type(ctx, f.type);
                }
            }
            break;
        }
        case NodeKind::ConstDecl: {
            auto& d = static_cast<ConstDecl&>(decl);
            symbols_.define({d.name.name, SymbolKind::Var, d.span});
            resolve_type(ctx, d.type);
            resolve_expr(ctx, d.value);
            break;
        }
        case NodeKind::MacroDecl: {
            auto& d = static_cast<MacroDecl&>(decl);
            symbols_.define({d.name.name, SymbolKind::Proc, d.span});
            symbols_.push_scope();
            for (const auto& p : d.params) {
                symbols_.define({p.name, SymbolKind::Param, p.span});
            }
            resolve_stmt(ctx, d.body);
            symbols_.pop_scope();
            break;
        }
        case NodeKind::UseDecl:
            break;
        case NodeKind::ProcDecl: {
            auto& d = static_cast<ProcDecl&>(decl);
            symbols_.define({d.name.name, SymbolKind::Proc, d.span});
            symbols_.push_scope();
            for (const auto& p : d.params) {
                symbols_.define({p.ident.name, SymbolKind::Param, p.ident.span});
                resolve_type(ctx, p.type);
            }
            resolve_type(ctx, d.return_type);
            if (d.body != kInvalidAstId) {
                resolve_stmt(ctx, d.body);
            }
            symbols_.pop_scope();
            break;
        }
        case NodeKind::EntryDecl: {
            auto& d = static_cast<EntryDecl&>(decl);
            symbols_.define({d.name.name, SymbolKind::Entry, d.span});
            symbols_.push_scope();
            if (d.body != kInvalidAstId) {
                resolve_stmt(ctx, d.body);
            }
            symbols_.pop_scope();
            break;
        }
        case NodeKind::FnDecl: {
            auto& d = static_cast<FnDecl&>(decl);
            symbols_.define({d.name.name, SymbolKind::Proc, d.span});
            symbols_.push_scope();
            for (const auto& p : d.params) {
                symbols_.define({p.ident.name, SymbolKind::Param, p.ident.span});
                resolve_type(ctx, p.type);
            }
            resolve_type(ctx, d.return_type);
            resolve_stmt(ctx, d.body);
            symbols_.pop_scope();
            break;
        }
        default:
            break;
    }
}

void Resolver::resolve_stmt(ast::AstContext& ctx, ast::StmtId stmt_id) {
    if (stmt_id == kInvalidAstId) {
        return;
    }

    auto& stmt = ctx.get<Stmt>(stmt_id);
    switch (stmt.kind) {
        case NodeKind::BlockStmt: {
            auto& b = static_cast<BlockStmt&>(stmt);
            symbols_.push_scope();
            for (auto s : b.stmts) {
                if (s != kInvalidAstId) {
                    resolve_stmt(ctx, s);
                }
            }
            symbols_.pop_scope();
            break;
        }
        case NodeKind::MakeStmt: {
            auto& s = static_cast<MakeStmt&>(stmt);
            symbols_.define({s.ident.name, SymbolKind::Var, s.ident.span});
            resolve_type(ctx, s.type);
            resolve_expr(ctx, s.value);
            break;
        }
        case NodeKind::SetStmt: {
            auto& s = static_cast<SetStmt&>(stmt);
            if (!symbols_.lookup(s.ident.name)) {
                diag::error(diag_, diag::DiagId::UnknownIdentifier, s.ident.span);
            }
            resolve_expr(ctx, s.value);
            break;
        }
        case NodeKind::LetStmt: {
            auto& s = static_cast<LetStmt&>(stmt);
            symbols_.define({s.ident.name, SymbolKind::Var, s.ident.span});
            resolve_type(ctx, s.type);
            resolve_expr(ctx, s.initializer);
            break;
        }
        case NodeKind::GiveStmt: {
            auto& s = static_cast<GiveStmt&>(stmt);
            resolve_expr(ctx, s.value);
            break;
        }
        case NodeKind::EmitStmt: {
            auto& s = static_cast<EmitStmt&>(stmt);
            resolve_expr(ctx, s.value);
            break;
        }
        case NodeKind::ExprStmt: {
            auto& s = static_cast<ExprStmt&>(stmt);
            resolve_expr(ctx, s.expr);
            break;
        }
        case NodeKind::ReturnStmt: {
            auto& s = static_cast<ReturnStmt&>(stmt);
            resolve_expr(ctx, s.expr);
            break;
        }
        case NodeKind::IfStmt: {
            auto& s = static_cast<IfStmt&>(stmt);
            resolve_expr(ctx, s.cond);
            resolve_stmt(ctx, s.then_block);
            if (s.else_block != kInvalidAstId) {
                resolve_stmt(ctx, s.else_block);
            }
            break;
        }
        case NodeKind::LoopStmt: {
            auto& s = static_cast<LoopStmt&>(stmt);
            resolve_stmt(ctx, s.body);
            break;
        }
        case NodeKind::ForStmt: {
            auto& s = static_cast<ForStmt&>(stmt);
            resolve_expr(ctx, s.iterable);
            symbols_.push_scope();
            symbols_.define({s.ident.name, SymbolKind::Var, s.ident.span});
            resolve_stmt(ctx, s.body);
            symbols_.pop_scope();
            break;
        }
        case NodeKind::BreakStmt:
        case NodeKind::ContinueStmt:
            break;
        case NodeKind::SelectStmt: {
            auto& s = static_cast<SelectStmt&>(stmt);
            resolve_expr(ctx, s.expr);
            for (auto w_id : s.whens) {
                if (w_id == kInvalidAstId) {
                    continue;
                }
                auto& w = static_cast<WhenStmt&>(ctx.node(w_id));
                symbols_.push_scope();
                std::unordered_map<std::string, bool> seen;
                auto bind_pattern = [&](auto&& self, PatternId p_id) -> void {
                    if (p_id == kInvalidAstId) {
                        return;
                    }
                    auto& pnode = ctx.node(p_id);
                    switch (pnode.kind) {
                        case NodeKind::IdentPattern: {
                            auto& ip = static_cast<const IdentPattern&>(pnode);
                            if (seen.find(ip.ident.name) != seen.end()) {
                                diag::error(diag_, diag::DiagId::DuplicatePatternBinding, ip.ident.span);
                            } else {
                                seen[ip.ident.name] = true;
                                symbols_.define({ip.ident.name, SymbolKind::Var, ip.ident.span});
                            }
                            break;
                        }
                        case NodeKind::CtorPattern: {
                            auto& cp = static_cast<const CtorPattern&>(pnode);
                            resolve_type(ctx, cp.type);
                            for (auto arg : cp.args) {
                                self(self, arg);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                };

                bind_pattern(bind_pattern, w.pattern);
                resolve_stmt(ctx, w.block);
                symbols_.pop_scope();
            }
            if (s.otherwise_block != kInvalidAstId) {
                symbols_.push_scope();
                resolve_stmt(ctx, s.otherwise_block);
                symbols_.pop_scope();
            }
            break;
        }
        default:
            break;
    }
}

void Resolver::resolve_expr(ast::AstContext& ctx, ast::ExprId expr_id) {
    if (expr_id == kInvalidAstId) {
        return;
    }

    auto& expr = ctx.get<Expr>(expr_id);
    switch (expr.kind) {
        case NodeKind::IdentExpr: {
            auto& e = static_cast<IdentExpr&>(expr);
            if (!symbols_.lookup(e.ident.name)) {
                diag::error(diag_, diag::DiagId::UnknownIdentifier, e.ident.span);
            }
            break;
        }
        case NodeKind::UnaryExpr: {
            auto& e = static_cast<UnaryExpr&>(expr);
            resolve_expr(ctx, e.expr);
            break;
        }
        case NodeKind::BinaryExpr: {
            auto& e = static_cast<BinaryExpr&>(expr);
            resolve_expr(ctx, e.lhs);
            resolve_expr(ctx, e.rhs);
            break;
        }
        case NodeKind::ProcExpr: {
            auto& e = static_cast<ProcExpr&>(expr);
            symbols_.push_scope();
            for (const auto& p : e.params) {
                symbols_.define({p.ident.name, SymbolKind::Param, p.ident.span});
                resolve_type(ctx, p.type);
            }
            resolve_type(ctx, e.return_type);
            resolve_stmt(ctx, e.body);
            symbols_.pop_scope();
            break;
        }
        case NodeKind::MemberExpr: {
            auto& e = static_cast<MemberExpr&>(expr);
            resolve_expr(ctx, e.base);
            break;
        }
        case NodeKind::IndexExpr: {
            auto& e = static_cast<IndexExpr&>(expr);
            resolve_expr(ctx, e.base);
            resolve_expr(ctx, e.index);
            break;
        }
        case NodeKind::IfExpr: {
            auto& e = static_cast<IfExpr&>(expr);
            resolve_expr(ctx, e.cond);
            resolve_stmt(ctx, e.then_block);
            if (e.else_block != kInvalidAstId) {
                resolve_stmt(ctx, e.else_block);
            }
            break;
        }
        case NodeKind::IsExpr: {
            auto& e = static_cast<IsExpr&>(expr);
            resolve_expr(ctx, e.value);
            break;
        }
        case NodeKind::AsExpr: {
            auto& e = static_cast<AsExpr&>(expr);
            resolve_expr(ctx, e.value);
            resolve_type(ctx, e.type);
            break;
        }
        case NodeKind::InvokeExpr: {
            auto& e = static_cast<InvokeExpr&>(expr);
            if (e.callee_expr != kInvalidAstId) {
                resolve_expr(ctx, e.callee_expr);
            }
            for (auto arg : e.args) {
                resolve_expr(ctx, arg);
            }
            break;
        }
        case NodeKind::CallNoParenExpr: {
            auto& e = static_cast<CallNoParenExpr&>(expr);
            if (!symbols_.lookup(e.callee.name)) {
                diag::error(diag_, diag::DiagId::UnknownIdentifier, e.callee.span);
            }
            resolve_expr(ctx, e.arg);
            break;
        }
        case NodeKind::ListExpr: {
            auto& e = static_cast<ListExpr&>(expr);
            for (auto item : e.items) {
                resolve_expr(ctx, item);
            }
            break;
        }
        default:
            break;
    }
}

const char* to_string(SymbolKind kind) {
    switch (kind) {
        case SymbolKind::Proc: return "proc";
        case SymbolKind::Entry: return "entry";
        case SymbolKind::Form: return "form";
        case SymbolKind::Pick: return "pick";
        case SymbolKind::Var: return "var";
        case SymbolKind::Param: return "param";
        default:
            return "unknown";
    }
}

void dump_symbols(const SymbolTable& table, std::ostream& os) {
    for (const auto& sym : table.symbols()) {
        os << to_string(sym.kind) << " " << sym.name;
        if (sym.span.is_valid()) {
            os << " [" << sym.span.start << ".." << sym.span.end << "]";
        }
        os << "\n";
    }
}

} // namespace vitte::frontend::resolve
