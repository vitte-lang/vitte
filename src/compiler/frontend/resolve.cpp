#include "resolve.hpp"
#include "diagnostics_messages.hpp"

#include <algorithm>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace vitte::frontend::resolve {

using namespace vitte::frontend::ast;

static int edit_distance(std::string_view a, std::string_view b) {
    const std::size_t n = a.size();
    const std::size_t m = b.size();
    if (n == 0) return static_cast<int>(m);
    if (m == 0) return static_cast<int>(n);
    std::vector<int> prev(m + 1), cur(m + 1);
    for (std::size_t j = 0; j <= m; ++j) {
        prev[j] = static_cast<int>(j);
    }
    for (std::size_t i = 1; i <= n; ++i) {
        cur[0] = static_cast<int>(i);
        for (std::size_t j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
        }
        prev.swap(cur);
    }
    return prev[m];
}

static std::string suggest_closest(
    const std::string& name,
    const std::vector<std::string>& candidates
) {
    if (candidates.empty()) {
        return {};
    }
    int max_dist = name.size() <= 3 ? 1 : 2;
    std::string best;
    int best_dist = max_dist + 1;
    for (const auto& cand : candidates) {
        if (cand == name) {
            continue;
        }
        int dist = edit_distance(name, cand);
        if (dist < best_dist) {
            best_dist = dist;
            best = cand;
        }
    }
    return best_dist <= max_dist ? best : std::string{};
}

static const std::unordered_map<std::string, std::string>& std_ident_suggestions() {
    static const std::unordered_map<std::string, std::string> kMap = {
        {"print", "use std/io/print.print"},
        {"println", "use std/io/print.println"},
        {"eprint", "use std/io/print.eprint"},
        {"eprintln", "use std/io/print.eprintln"},
        {"print_or_panic", "use std/io/print.print_or_panic"},
        {"println_or_panic", "use std/io/print.println_or_panic"},
        {"read_all", "use std/io/read.read_all"},
        {"read_fd", "use std/io/read.read_fd"},
        {"reader_from_fd", "use std/io/read.reader_from_fd"},
        {"reader_as_reader", "use std/io/read.reader_as_reader"},
        {"read_exact", "use std/io/read.read_exact"},
        {"read_to_end", "use std/io/read.read_to_end"},
        {"read_some", "use std/io/stdin.read_some"},
        {"read_exact_all", "use std/io/stdin.read_exact_all"},
        {"read_stdin", "use std/io/stdin.read_stdin"},
        {"stdin", "use std/io/stdin.stdin"},
        {"as_reader", "use std/io/stdin.as_reader"},
        {"write_all", "use std/io/write.write_all"},
        {"write_fd", "use std/io/write.write_fd"},
        {"writer_from_fd", "use std/io/write.writer_from_fd"},
        {"writer_as_writer", "use std/io/write.writer_as_writer"},
        {"write_string", "use std/io/write.write_string"},
        {"flush", "use std/io/write.flush"},
        {"write", "use std/io/stdout.write"},
        {"writeln", "use std/io/stdout.writeln"},
        {"write_or_panic", "use std/io/stdout.write_or_panic"},
        {"writeln_or_panic", "use std/io/stdout.writeln_or_panic"},
        {"path", "use std/io/path"},
        {"fs", "use std/io/fs"},
        {"Option", "use std/core/option.Option"},
        {"Result", "use std/core/result.Result"},
        {"Unit", "use std/core/types.Unit"},
        {"IoError", "use std/io/error.IoError"},
        {"IoResult", "use std/io/error.IoResult"},
    };
    return kMap;
}

static const std::unordered_map<std::string, std::string>& std_type_suggestions() {
    static const std::unordered_map<std::string, std::string> kMap = {
        {"Option", "use std/core/option.Option"},
        {"Result", "use std/core/result.Result"},
        {"Unit", "use std/core/types.Unit"},
        {"IoError", "use std/io/error.IoError"},
        {"IoResult", "use std/io/error.IoResult"},
    };
    return kMap;
}

static bool is_known_import_type(std::string_view name) {
    static const std::unordered_set<std::string_view> kTypes = {
        "bool", "string", "int",
        "i8", "i16", "i32", "i64", "i128",
        "u8", "u16", "u32", "u64", "u128",
        "isize", "usize",
        "f32", "f64",
        "Unit", "Never",
    };
    return kTypes.count(name) > 0;
}

static bool path_mentions_types(const ast::ModulePath& path) {
    for (const auto& part : path.parts) {
        if (part.name == "types") {
            return true;
        }
    }
    return false;
}

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

std::vector<std::string> SymbolTable::in_scope_names(std::size_t limit) const {
    std::vector<std::string> out;
    if (scopes_.empty()) {
        return out;
    }
    std::unordered_set<std::string> seen;
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        std::vector<std::string> scope_names;
        scope_names.reserve(it->size());
        for (const auto& kv : *it) {
            scope_names.push_back(kv.first);
        }
        std::sort(scope_names.begin(), scope_names.end());
        for (const auto& name : scope_names) {
            if (seen.insert(name).second) {
                out.push_back(name);
                if (limit > 0 && out.size() >= limit) {
                    return out;
                }
            }
        }
    }
    return out;
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
    types_.add_builtin("builtin.bool");
    types_.add_builtin("builtin.char");
    types_.add_builtin("builtin.i8");
    types_.add_builtin("builtin.i16");
    types_.add_builtin("builtin.i32");
    types_.add_builtin("builtin.i64");
    types_.add_builtin("builtin.i128");
    types_.add_builtin("builtin.u8");
    types_.add_builtin("builtin.u16");
    types_.add_builtin("builtin.u32");
    types_.add_builtin("builtin.u64");
    types_.add_builtin("builtin.u128");
    types_.add_builtin("builtin.isize");
    types_.add_builtin("builtin.usize");
    types_.add_builtin("builtin.f32");
    types_.add_builtin("builtin.f64");
    types_.add_builtin("builtin.string");
    types_.add_builtin("builtin.slice");
    types_.add_builtin("builtin.mut_slice");
    symbols_.define({"bool", SymbolKind::Form, {}});
    symbols_.define({"string", SymbolKind::Form, {}});
    symbols_.define({"int", SymbolKind::Form, {}});
    symbols_.define({"builtin", SymbolKind::Var, {}});
}

bool Resolver::resolve_module(ast::AstContext& ctx, ast::ModuleId module_id) {
    if (module_id == kInvalidAstId) {
        return false;
    }
    auto& module = ctx.get<Module>(module_id);
    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidAstId) {
            continue;
        }
        const auto& decl = ctx.get<Decl>(decl_id);
        switch (decl.kind) {
            case NodeKind::TypeDecl: {
                const auto& d = static_cast<const TypeDecl&>(decl);
                types_.add_named(d.name.name);
                break;
            }
            case NodeKind::TypeAliasDecl: {
                const auto& d = static_cast<const TypeAliasDecl&>(decl);
                types_.add_named(d.name.name);
                break;
            }
            case NodeKind::FormDecl: {
                const auto& d = static_cast<const FormDecl&>(decl);
                types_.add_named(d.name.name);
                break;
            }
            case NodeKind::PickDecl: {
                const auto& d = static_cast<const PickDecl&>(decl);
                types_.add_named(d.name.name);
                for (const auto& c : d.cases) {
                    types_.add_named(d.name.name + "." + c.ident.name);
                }
                break;
            }
            case NodeKind::UseDecl: {
                const auto& d = static_cast<const UseDecl&>(decl);
                std::string name;
                if (d.alias.has_value()) {
                    name = d.alias->name;
                } else if (!d.path.parts.empty()) {
                    name = d.path.parts.back().name;
                }
                if (!name.empty() && (is_known_import_type(name) || path_mentions_types(d.path))) {
                    types_.add_named(name);
                }
                break;
            }
            default:
                break;
        }
    }
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
                std::vector<std::string> names;
                names.reserve(types_.all().size());
                for (const auto& info : types_.all()) {
                    names.push_back(info.name);
                }
                if (auto suggestion = suggest_closest(t.ident.name, names); !suggestion.empty()) {
                    diag_.note("did you mean '" + suggestion + "'?", t.ident.span);
                }
                std::vector<std::string> builtins;
                for (const auto& info : types_.all()) {
                    if (info.kind == types::TypeKind::Builtin) {
                        builtins.push_back(info.name);
                    }
                }
                if (!builtins.empty()) {
                    std::string list = builtins.front();
                    for (std::size_t i = 1; i < builtins.size(); ++i) {
                        list += ", " + builtins[i];
                    }
                    diag_.note("built-in types: " + list, t.ident.span);
                }
                auto it = std_type_suggestions().find(t.ident.name);
                if (it != std_type_suggestions().end()) {
                    diag_.note("try: " + it->second, t.ident.span);
                }
            }
            resolved_types_[type] = id;
            return id;
        }
        case NodeKind::GenericType: {
            auto& t = static_cast<const GenericType&>(node);
            auto id = types_.lookup(t.base_ident.name);
            if (id == static_cast<types::TypeId>(-1)) {
                diag::error(diag_, diag::DiagId::UnknownGenericBaseType, t.base_ident.span);
                std::vector<std::string> names;
                names.reserve(types_.all().size());
                for (const auto& info : types_.all()) {
                    names.push_back(info.name);
                }
                if (auto suggestion = suggest_closest(t.base_ident.name, names); !suggestion.empty()) {
                    diag_.note("did you mean '" + suggestion + "'?", t.base_ident.span);
                }
                std::vector<std::string> builtins;
                for (const auto& info : types_.all()) {
                    if (info.kind == types::TypeKind::Builtin) {
                        builtins.push_back(info.name);
                    }
                }
                if (!builtins.empty()) {
                    std::string list = builtins.front();
                    for (std::size_t i = 1; i < builtins.size(); ++i) {
                        list += ", " + builtins[i];
                    }
                    diag_.note("built-in types: " + list, t.base_ident.span);
                }
                auto it = std_type_suggestions().find(t.base_ident.name);
                if (it != std_type_suggestions().end()) {
                    diag_.note("try: " + it->second, t.base_ident.span);
                }
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
            for (const auto& param : d.type_params) {
                types_.add_named(param.name);
            }
            symbols_.define({d.name.name, SymbolKind::Form, d.span});
            for (const auto& f : d.fields) {
                resolve_type(ctx, f.type);
            }
            break;
        }
        case NodeKind::TypeAliasDecl: {
            auto& d = static_cast<TypeAliasDecl&>(decl);
            types_.add_named(d.name.name);
            for (const auto& param : d.type_params) {
                types_.add_named(param.name);
            }
            symbols_.define({d.name.name, SymbolKind::Form, d.span});
            resolve_type(ctx, d.target);
            break;
        }
        case NodeKind::PickDecl: {
            auto& d = static_cast<PickDecl&>(decl);
            types_.add_named(d.name.name);
            for (const auto& param : d.type_params) {
                types_.add_named(param.name);
            }
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
        case NodeKind::GlobalDecl: {
            auto& d = static_cast<GlobalDecl&>(decl);
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
        {
            auto& d = static_cast<UseDecl&>(decl);
            std::string name;
            if (d.alias.has_value()) {
                name = d.alias->name;
            } else if (!d.path.parts.empty()) {
                name = d.path.parts.back().name;
            }
            if (!name.empty()) {
                symbols_.define({name, SymbolKind::Var, d.span});
                if (is_known_import_type(name) || path_mentions_types(d.path)) {
                    types_.add_named(name);
                }
            }
            break;
        }
        case NodeKind::ProcDecl: {
            auto& d = static_cast<ProcDecl&>(decl);
            symbols_.define({d.name.name, SymbolKind::Proc, d.span});
            symbols_.push_scope();
            for (const auto& param : d.type_params) {
                types_.add_named(param.name);
            }
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
        case NodeKind::AsmStmt:
            break;
        case NodeKind::UnsafeStmt: {
            auto& s = static_cast<UnsafeStmt&>(stmt);
            resolve_stmt(ctx, s.body);
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
                auto in_scope = symbols_.in_scope_names();
                if (auto suggestion = suggest_closest(s.ident.name, in_scope); !suggestion.empty()) {
                    diag_.note("did you mean '" + suggestion + "'?", s.ident.span);
                }
                auto top = symbols_.in_scope_names(3);
                if (!top.empty()) {
                    std::string list = top.front();
                    for (std::size_t i = 1; i < top.size(); ++i) {
                        list += ", " + top[i];
                    }
                    diag_.note("in scope: " + list, s.ident.span);
                }
                auto it = std_ident_suggestions().find(s.ident.name);
                if (it != std_ident_suggestions().end()) {
                    diag_.note("try: " + it->second, s.ident.span);
                }
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
                auto in_scope = symbols_.in_scope_names();
                if (auto suggestion = suggest_closest(e.ident.name, in_scope); !suggestion.empty()) {
                    diag_.note("did you mean '" + suggestion + "'?", e.ident.span);
                }
                auto top = symbols_.in_scope_names(3);
                if (!top.empty()) {
                    std::string list = top.front();
                    for (std::size_t i = 1; i < top.size(); ++i) {
                        list += ", " + top[i];
                    }
                    diag_.note("in scope: " + list, e.ident.span);
                }
                auto it = std_ident_suggestions().find(e.ident.name);
                if (it != std_ident_suggestions().end()) {
                    diag_.note("try: " + it->second, e.ident.span);
                }
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
                auto in_scope = symbols_.in_scope_names();
                if (auto suggestion = suggest_closest(e.callee.name, in_scope); !suggestion.empty()) {
                    diag_.note("did you mean '" + suggestion + "'?", e.callee.span);
                }
                auto top = symbols_.in_scope_names(3);
                if (!top.empty()) {
                    std::string list = top.front();
                    for (std::size_t i = 1; i < top.size(); ++i) {
                        list += ", " + top[i];
                    }
                    diag_.note("in scope: " + list, e.callee.span);
                }
                auto it = std_ident_suggestions().find(e.callee.name);
                if (it != std_ident_suggestions().end()) {
                    diag_.note("try: " + it->second, e.callee.span);
                }
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
