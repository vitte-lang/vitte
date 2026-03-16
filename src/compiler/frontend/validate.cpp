// ============================================================
// validate.cpp — Vitte Compiler
// AST shape validation
// ============================================================

#include "validate.hpp"
#include "diagnostics_messages.hpp"

#include <unordered_map>
#include <unordered_set>

namespace vitte::frontend::validate {

using namespace vitte::frontend::ast;

static bool has_attr(const std::vector<Attribute>& attrs, const char* name, SourceSpan* span_out) {
    for (const auto& attr : attrs) {
        if (attr.name.name == name) {
            if (span_out) {
                *span_out = attr.span;
            }
            return true;
        }
    }
    return false;
}

static void validate_type_params(
    const std::vector<Ident>& params,
    diag::DiagnosticEngine& diagnostics,
    const char* owner) {
    std::unordered_map<std::string, SourceSpan> seen;
    for (const auto& param : params) {
        auto it = seen.find(param.name);
        if (it != seen.end()) {
            diagnostics.error(
                std::string("duplicate type parameter '") + param.name + "' in " + owner,
                param.span);
            continue;
        }
        seen.emplace(param.name, param.span);
    }
}

static bool stmt_always_returns(const AstContext& ast_ctx, StmtId stmt_id) {
    if (stmt_id == ast::kInvalidAstId) {
        return false;
    }

    const auto& stmt = ast_ctx.get<Stmt>(stmt_id);
    switch (stmt.kind) {
        case NodeKind::GiveStmt:
        case NodeKind::ReturnStmt:
            return true;
        case NodeKind::UnsafeStmt: {
            const auto& s = ast_ctx.get<UnsafeStmt>(stmt_id);
            return stmt_always_returns(ast_ctx, s.body);
        }
        case NodeKind::BlockStmt: {
            const auto& block = ast_ctx.get<BlockStmt>(stmt_id);
            for (StmtId child_id : block.stmts) {
                if (stmt_always_returns(ast_ctx, child_id)) {
                    return true;
                }
            }
            return false;
        }
        case NodeKind::IfStmt: {
            const auto& s = ast_ctx.get<IfStmt>(stmt_id);
            if (s.else_block == ast::kInvalidAstId) {
                return false;
            }
            return stmt_always_returns(ast_ctx, s.then_block) &&
                   stmt_always_returns(ast_ctx, s.else_block);
        }
        case NodeKind::SelectStmt: {
            const auto& s = ast_ctx.get<SelectStmt>(stmt_id);
            if (s.otherwise_block == ast::kInvalidAstId || s.whens.empty()) {
                return false;
            }
            for (StmtId when_id : s.whens) {
                const auto& when_stmt = ast_ctx.get<WhenStmt>(when_id);
                if (!stmt_always_returns(ast_ctx, when_stmt.block)) {
                    return false;
                }
            }
            return stmt_always_returns(ast_ctx, s.otherwise_block);
        }
        default:
            return false;
    }
}

static void validate_proc(const ProcDecl& proc, diag::DiagnosticEngine& diagnostics) {
    SourceSpan extern_span{};
    bool is_extern = has_attr(proc.attrs, "extern", &extern_span);
    bool has_body = proc.body != ast::kInvalidAstId;

    if (is_extern && has_body) {
        diag::error(diagnostics, diag::DiagId::ExternProcCannotHaveBody, extern_span.is_valid() ? extern_span : proc.span);
    }
    if (!is_extern && !has_body) {
        diag::error(diagnostics, diag::DiagId::ProcRequiresBodyUnlessExtern, proc.span);
    }
    validate_type_params(proc.type_params, diagnostics, "proc");
}

static void validate_proc_returns(const AstContext& ast_ctx, const ProcDecl& proc, diag::DiagnosticEngine& diagnostics) {
    if (proc.return_type == ast::kInvalidAstId || proc.body == ast::kInvalidAstId) {
        return;
    }

    SourceSpan extern_span{};
    if (has_attr(proc.attrs, "extern", &extern_span)) {
        return;
    }

    if (!stmt_always_returns(ast_ctx, proc.body)) {
        SourceSpan span = proc.name.span.is_valid() ? proc.name.span : proc.span;
        diag::Diagnostic missing_return(
            diag::Severity::Error,
            "E0013",
            "proc may exit without returning a value",
            span);
        missing_return.add_note("typed procedures with '-> T' must return a value on every path");
        missing_return.add_fix("add explicit fallback return", "give <value>", span);
        diagnostics.emit(std::move(missing_return));
    }
}

static void validate_type_alias(const TypeAliasDecl& alias, diag::DiagnosticEngine& diagnostics) {
    validate_type_params(alias.type_params, diagnostics, "type alias");
    if (alias.target == ast::kInvalidAstId) {
        diag::error(diagnostics, diag::DiagId::TypeAliasRequiresTargetType, alias.span);
    }
}

static void validate_form(const FormDecl& form, diag::DiagnosticEngine& diagnostics) {
    validate_type_params(form.type_params, diagnostics, "form");
}

static void validate_pick(const PickDecl& pick, diag::DiagnosticEngine& diagnostics) {
    validate_type_params(pick.type_params, diagnostics, "pick");
}

static std::unordered_set<std::string> collect_shareable_names_for_file(
    const AstContext& ast_ctx,
    const Module& mod,
    const SourceFile* file
) {
    std::unordered_set<std::string> names;
    for (const auto decl_id : mod.decls) {
        if (decl_id == ast::kInvalidAstId) {
            continue;
        }
        const auto& decl = ast_ctx.get<Decl>(decl_id);
        if (decl.span.file != file) {
            continue;
        }
        switch (decl.kind) {
            case NodeKind::ProcDecl: {
                const auto& proc = ast_ctx.get<ProcDecl>(decl_id);
                bool is_extern = false;
                for (const auto& attr : proc.attrs) {
                    if (attr.name.name == "extern") {
                        is_extern = true;
                        break;
                    }
                }
                if (!is_extern) {
                    names.insert(proc.name.name);
                }
                break;
            }
            case NodeKind::FnDecl:
                names.insert(ast_ctx.get<FnDecl>(decl_id).name.name);
                break;
            case NodeKind::ConstDecl:
                names.insert(ast_ctx.get<ConstDecl>(decl_id).name.name);
                break;
            case NodeKind::GlobalDecl:
                names.insert(ast_ctx.get<GlobalDecl>(decl_id).name.name);
                break;
            case NodeKind::TypeDecl:
                names.insert(ast_ctx.get<TypeDecl>(decl_id).name.name);
                break;
            case NodeKind::TypeAliasDecl:
                names.insert(ast_ctx.get<TypeAliasDecl>(decl_id).name.name);
                break;
            case NodeKind::FormDecl:
                names.insert(ast_ctx.get<FormDecl>(decl_id).name.name);
                break;
            case NodeKind::PickDecl:
                names.insert(ast_ctx.get<PickDecl>(decl_id).name.name);
                break;
            case NodeKind::MacroDecl:
                names.insert(ast_ctx.get<MacroDecl>(decl_id).name.name);
                break;
            case NodeKind::UseDecl: {
                const auto& use = ast_ctx.get<UseDecl>(decl_id);
                if (use.alias.has_value()) {
                    names.insert(use.alias->name);
                } else if (!use.path.parts.empty()) {
                    names.insert(use.path.parts.back().name);
                }
                break;
            }
            case NodeKind::PullDecl: {
                const auto& pull = ast_ctx.get<PullDecl>(decl_id);
                if (pull.alias.has_value()) {
                    names.insert(pull.alias->name);
                } else if (!pull.path.parts.empty()) {
                    names.insert(pull.path.parts.back().name);
                }
                break;
            }
            default:
                break;
        }
    }
    return names;
}

static std::optional<std::pair<std::string, SourceSpan>> import_binding_name(
    const AstContext& ast_ctx,
    DeclId decl_id
) {
    const auto& decl = ast_ctx.get<Decl>(decl_id);
    switch (decl.kind) {
        case NodeKind::UseDecl: {
            const auto& use = ast_ctx.get<UseDecl>(decl_id);
            if (use.alias.has_value()) {
                return std::make_pair(use.alias->name, use.alias->span);
            }
            if (!use.path.parts.empty()) {
                const auto& last = use.path.parts.back();
                return std::make_pair(last.name, last.span);
            }
            return std::nullopt;
        }
        case NodeKind::PullDecl: {
            const auto& pull = ast_ctx.get<PullDecl>(decl_id);
            if (pull.alias.has_value()) {
                return std::make_pair(pull.alias->name, pull.alias->span);
            }
            if (!pull.path.parts.empty()) {
                const auto& last = pull.path.parts.back();
                return std::make_pair(last.name, last.span);
            }
            return std::nullopt;
        }
        default:
            return std::nullopt;
    }
}

static std::optional<std::pair<std::string, SourceSpan>> local_binding_name(
    const AstContext& ast_ctx,
    DeclId decl_id
) {
    const auto& decl = ast_ctx.get<Decl>(decl_id);
    switch (decl.kind) {
        case NodeKind::ProcDecl: {
            const auto& proc = ast_ctx.get<ProcDecl>(decl_id);
            for (const auto& attr : proc.attrs) {
                if (attr.name.name == "extern") {
                    return std::nullopt;
                }
            }
            return std::make_pair(proc.name.name, proc.name.span);
        }
        case NodeKind::FnDecl: {
            const auto& fn = ast_ctx.get<FnDecl>(decl_id);
            return std::make_pair(fn.name.name, fn.name.span);
        }
        case NodeKind::ConstDecl: {
            const auto& c = ast_ctx.get<ConstDecl>(decl_id);
            return std::make_pair(c.name.name, c.name.span);
        }
        case NodeKind::GlobalDecl: {
            const auto& g = ast_ctx.get<GlobalDecl>(decl_id);
            return std::make_pair(g.name.name, g.name.span);
        }
        case NodeKind::TypeDecl: {
            const auto& t = ast_ctx.get<TypeDecl>(decl_id);
            return std::make_pair(t.name.name, t.name.span);
        }
        case NodeKind::TypeAliasDecl: {
            const auto& t = ast_ctx.get<TypeAliasDecl>(decl_id);
            return std::make_pair(t.name.name, t.name.span);
        }
        case NodeKind::FormDecl: {
            const auto& f = ast_ctx.get<FormDecl>(decl_id);
            return std::make_pair(f.name.name, f.name.span);
        }
        case NodeKind::PickDecl: {
            const auto& p = ast_ctx.get<PickDecl>(decl_id);
            return std::make_pair(p.name.name, p.name.span);
        }
        case NodeKind::MacroDecl: {
            const auto& m = ast_ctx.get<MacroDecl>(decl_id);
            return std::make_pair(m.name.name, m.name.span);
        }
        case NodeKind::EntryDecl: {
            const auto& e = ast_ctx.get<EntryDecl>(decl_id);
            return std::make_pair(e.name.name, e.name.span);
        }
        default:
            return std::nullopt;
    }
}

static void validate_entry_decl(
    const EntryDecl& entry,
    std::unordered_map<std::string, SourceSpan>& seen_names,
    diag::DiagnosticEngine& diagnostics
) {
    if (entry.module.relative_depth > 0) {
        diag::Diagnostic invalid_entry(
            diag::Severity::Error,
            "E1021",
            "entry module path must be canonical",
            entry.module.span
        );
        invalid_entry.add_note("remove leading dots from the entry module path");
        invalid_entry.add_fix("use canonical entry path", "entry " + entry.name.name + " at app/core { ... }", entry.module.span);
        diagnostics.emit(std::move(invalid_entry));
    }

    auto [it, inserted] = seen_names.emplace(entry.name.name, entry.name.span);
    if (!inserted) {
        diag::Diagnostic duplicate_entry(
            diag::Severity::Error,
            "E1022",
            "duplicate entry name",
            entry.name.span
        );
        duplicate_entry.add_note("first entry '" + entry.name.name + "' is declared earlier in this module");
        duplicate_entry.add_fix("rename duplicate entry", entry.name.name + "_alt", entry.name.span);
        diagnostics.emit(std::move(duplicate_entry));
    }
}

static void validate_share_decl(
    const ShareDecl& share,
    const std::unordered_set<std::string>& shareable_names,
    SourceSpan* first_share_span,
    diag::DiagnosticEngine& diagnostics
) {
    if (first_share_span->is_valid()) {
        diag::Diagnostic duplicate_share(
            diag::Severity::Error,
            "E1026",
            "duplicate share declaration",
            share.span
        );
        duplicate_share.add_note("first share declaration appears earlier in this module");
        duplicate_share.add_fix("merge share declarations", "share a, b", share.span);
        diagnostics.emit(std::move(duplicate_share));
        return;
    }
    *first_share_span = share.span;

    if (share.share_all) {
        return;
    }

    std::unordered_set<std::string> seen;
    for (const auto& name : share.names) {
        auto [_, inserted] = seen.insert(name.name);
        if (!inserted) {
            diag::Diagnostic duplicate_symbol(
                diag::Severity::Error,
                "E1024",
                "duplicate symbol in share list",
                name.span
            );
            duplicate_symbol.add_note("keep each shared symbol only once");
            duplicate_symbol.add_fix("remove duplicate share symbol", name.name, name.span);
            diagnostics.emit(std::move(duplicate_symbol));
            continue;
        }
        if (shareable_names.count(name.name) == 0) {
            diag::Diagnostic unknown_symbol(
                diag::Severity::Error,
                "E1023",
                "share references unknown symbol",
                name.span
            );
            unknown_symbol.add_note("shared names must match a local declaration or an explicit import alias");
            unknown_symbol.add_fix("share an existing symbol", "share <existing_name>", name.span);
            diagnostics.emit(std::move(unknown_symbol));
        }
    }
}

void validate_module(ast::AstContext& ast_ctx, ast::ModuleId module, diag::DiagnosticEngine& diagnostics) {
    const auto& mod = ast_ctx.get<Module>(module);
    std::unordered_map<const SourceFile*, std::unordered_set<std::string>> shareable_names_by_file;
    std::unordered_map<const SourceFile*, std::unordered_map<std::string, SourceSpan>> seen_entry_names_by_file;
    std::unordered_map<const SourceFile*, std::unordered_map<std::string, SourceSpan>> seen_import_bindings_by_file;
    std::unordered_map<const SourceFile*, std::unordered_map<std::string, SourceSpan>> seen_local_bindings_by_file;
    std::unordered_map<const SourceFile*, SourceSpan> first_share_span_by_file;
    for (const auto& decl_id : mod.decls) {
        const auto& decl = ast_ctx.get<Decl>(decl_id);
        const SourceFile* file = decl.span.file != nullptr ? decl.span.file : mod.span.file;
        auto& seen_entry_names = seen_entry_names_by_file[file];
        auto& seen_import_bindings = seen_import_bindings_by_file[file];
        auto& seen_local_bindings = seen_local_bindings_by_file[file];
        auto& first_share_span = first_share_span_by_file[file];
        if (auto imported = import_binding_name(ast_ctx, decl_id); imported.has_value()) {
            auto [name, span] = *imported;
            auto [it, inserted] = seen_import_bindings.emplace(name, span);
            if (!inserted) {
                diag::Diagnostic duplicate_import(
                    diag::Severity::Error,
                    "E1027",
                    "duplicate import binding",
                    span
                );
                duplicate_import.add_note("first import binding '" + name + "' appears earlier in this module");
                duplicate_import.add_fix("rename duplicate import", name + "_mod", span);
                diagnostics.emit(std::move(duplicate_import));
            }
            auto local_it = seen_local_bindings.find(name);
            if (local_it != seen_local_bindings.end()) {
                diag::Diagnostic conflict(
                    diag::Severity::Error,
                    "E1028",
                    "import binding conflicts with local declaration",
                    span
                );
                conflict.add_note("local declaration '" + name + "' appears earlier in this module");
                conflict.add_fix("rename imported binding", name + "_mod", span);
                diagnostics.emit(std::move(conflict));
            }
        }
        if (auto local = local_binding_name(ast_ctx, decl_id); local.has_value()) {
            auto [name, span] = *local;
            auto [_, inserted] = seen_local_bindings.emplace(name, span);
            if (inserted) {
                auto import_it = seen_import_bindings.find(name);
                if (import_it != seen_import_bindings.end()) {
                    diag::Diagnostic conflict(
                        diag::Severity::Error,
                        "E1028",
                        "import binding conflicts with local declaration",
                        span
                    );
                    conflict.add_note("import binding '" + name + "' appears earlier in this module");
                    conflict.add_fix("rename local declaration", name + "_local", span);
                    diagnostics.emit(std::move(conflict));
                }
            } else {
                diag::Diagnostic duplicate_local(
                    diag::Severity::Error,
                    "E1029",
                    "duplicate local declaration name",
                    span
                );
                duplicate_local.add_note("first local declaration '" + name + "' appears earlier in this module");
                duplicate_local.add_fix("rename duplicate local declaration", name + "_alt", span);
                diagnostics.emit(std::move(duplicate_local));
            }
        }
        switch (decl.kind) {
            case NodeKind::ProcDecl:
                validate_proc(ast_ctx.get<ProcDecl>(decl_id), diagnostics);
                validate_proc_returns(ast_ctx, ast_ctx.get<ProcDecl>(decl_id), diagnostics);
                break;
            case NodeKind::EntryDecl:
                validate_entry_decl(ast_ctx.get<EntryDecl>(decl_id), seen_entry_names, diagnostics);
                break;
            case NodeKind::ShareDecl:
                if (shareable_names_by_file.find(file) == shareable_names_by_file.end()) {
                    shareable_names_by_file.emplace(file, collect_shareable_names_for_file(ast_ctx, mod, file));
                }
                validate_share_decl(
                    ast_ctx.get<ShareDecl>(decl_id),
                    shareable_names_by_file[file],
                    &first_share_span,
                    diagnostics
                );
                break;
            case NodeKind::TypeAliasDecl:
                validate_type_alias(ast_ctx.get<TypeAliasDecl>(decl_id), diagnostics);
                break;
            case NodeKind::FormDecl:
                validate_form(ast_ctx.get<FormDecl>(decl_id), diagnostics);
                break;
            case NodeKind::PickDecl:
                validate_pick(ast_ctx.get<PickDecl>(decl_id), diagnostics);
                break;
            default:
                break;
        }
    }
}

} // namespace vitte::frontend::validate
