#include "module_loader.hpp"

#include "lexer.hpp"
#include "parser.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace vitte::frontend::modules {

using namespace vitte::frontend::ast;

static std::string join_path(const ModulePath& path, const char* sep) {
    std::string out;
    for (std::size_t i = 0; i < path.parts.size(); ++i) {
        if (i) out += sep;
        out += path.parts[i].name;
    }
    return out;
}

std::string module_prefix(const ModulePath& path) {
    std::string prefix = join_path(path, "__");
    if (!prefix.empty()) {
        prefix += "__";
    }
    return prefix;
}

static std::string module_path_key(const ModulePath& path) {
    return join_path(path, "/");
}

static bool is_std_or_core_root(const ModulePath& path) {
    if (path.parts.empty()) {
        return false;
    }
    const auto& head = path.parts.front().name;
    return head == "std" || head == "core";
}

static bool is_vitte_package_root(const ModulePath& path) {
    if (path.parts.empty()) {
        return false;
    }
    return path.parts.front().name == "vitte";
}

static std::size_t vitte_package_trim_count(const ModulePath& path) {
    return is_vitte_package_root(path) ? 1 : 0;
}

std::string normalized_stdlib_path(const ModulePath& path) {
    if (path.parts.empty()) {
        return {};
    }
    if (path.parts.front().name == "std") {
        return module_path_key(path);
    }
    if (path.parts.front().name == "core") {
        std::string out = "std/core";
        for (std::size_t i = 1; i < path.parts.size(); ++i) {
            out += "/";
            out += path.parts[i].name;
        }
        return out;
    }
    return {};
}

bool is_valid_stdlib_profile(const std::string& profile) {
    return profile == "minimal" ||
           profile == "full" ||
           profile == "kernel" ||
           profile == "arduino";
}

static bool has_prefix(std::string_view value, std::string_view prefix) {
    return value.substr(0, prefix.size()) == prefix;
}

static std::size_t count_lines(const std::string& source) {
    if (source.empty()) {
        return 0;
    }
    std::size_t lines = 1;
    for (char c : source) {
        if (c == '\n') {
            ++lines;
        }
    }
    return lines;
}

static bool is_experimental_module_key(std::string_view key) {
    return key == "experimental" ||
           has_prefix(key, "experimental/") ||
           key.find("/experimental/") != std::string_view::npos ||
           has_prefix(key, "std/experimental/") ||
           has_prefix(key, "core/experimental/");
}

static bool is_internal_module_key(std::string_view key) {
    return key == "internal" ||
           has_prefix(key, "internal/") ||
           key.find("/internal/") != std::string_view::npos;
}

static std::string internal_owner_namespace(std::string_view key) {
    const auto pos = key.find("/internal/");
    if (pos == std::string_view::npos) {
        if (has_prefix(key, "internal/")) {
            return {};
        }
        return std::string(key);
    }
    return std::string(key.substr(0, pos));
}

bool is_stdlib_path_allowed(const ModulePath& path, const std::string& profile) {
    const std::string normalized = normalized_stdlib_path(path);
    if (normalized.empty()) {
        return true;
    }
    if (profile == "full") {
        return true;
    }
    if (profile == "minimal") {
        return normalized == "std/core" || has_prefix(normalized, "std/core/");
    }
    if (profile == "kernel") {
        return normalized == "std/core" || has_prefix(normalized, "std/core/") ||
               normalized == "std/kernel" || has_prefix(normalized, "std/kernel/");
    }
    if (profile == "arduino") {
        return normalized == "std/core" || has_prefix(normalized, "std/core/") ||
               normalized == "std/arduino" || has_prefix(normalized, "std/arduino/");
    }
    return true;
}

static std::filesystem::path resolve_module_file(
    const ModulePath& path,
    const std::filesystem::path& base_dir,
    const std::filesystem::path& repo_root
) {
    auto module_file_candidates = [&](const ModulePath& target_path) {
        std::string rel_str = join_path(target_path, "/");
        if (is_std_or_core_root(target_path) || is_vitte_package_root(target_path)) {
            ModulePath trimmed = target_path;
            const std::size_t trim_count = is_std_or_core_root(target_path) ? 1 : vitte_package_trim_count(target_path);
            trimmed.parts.erase(trimmed.parts.begin(), trimmed.parts.begin() + static_cast<std::ptrdiff_t>(trim_count));
            rel_str = join_path(trimmed, "/");
        }
        std::filesystem::path rel(rel_str);
        std::vector<std::filesystem::path> candidates;

        if (is_std_or_core_root(target_path)) {
            std::filesystem::path std_root = repo_root / "src/vitte/std";
            if (target_path.parts.front().name == "core") {
                std::filesystem::path core_root = std_root / "core";
                candidates.push_back(core_root / (rel.string() + ".vit"));
                candidates.push_back(core_root / rel / "mod.vit");
            }
            candidates.push_back(std_root / (rel.string() + ".vit"));
            candidates.push_back(std_root / rel / "mod.vit");
        } else if (is_vitte_package_root(target_path)) {
            std::filesystem::path pkg_root = repo_root / "src/vitte/packages";
            candidates.push_back(pkg_root / (rel.string() + ".vit"));
            candidates.push_back(pkg_root / rel / "mod.vit");
        } else {
            candidates.push_back(base_dir / (rel.string() + ".vit"));
            candidates.push_back(base_dir / rel / "mod.vit");
        }
        return candidates;
    };

    for (const auto& c : module_file_candidates(path)) {
        if (std::filesystem::exists(c)) {
            return c;
        }
    }

    return {};
}

static std::vector<std::filesystem::path> existing_module_files(
    const ModulePath& path,
    const std::filesystem::path& base_dir,
    const std::filesystem::path& repo_root
) {
    std::string rel_str = join_path(path, "/");
    if (is_std_or_core_root(path) || is_vitte_package_root(path)) {
        ModulePath trimmed = path;
        const std::size_t trim_count = is_std_or_core_root(path) ? 1 : vitte_package_trim_count(path);
        trimmed.parts.erase(trimmed.parts.begin(), trimmed.parts.begin() + static_cast<std::ptrdiff_t>(trim_count));
        rel_str = join_path(trimmed, "/");
    }
    std::filesystem::path rel(rel_str);
    std::vector<std::filesystem::path> candidates;
    if (is_std_or_core_root(path)) {
        std::filesystem::path std_root = repo_root / "src/vitte/std";
        if (path.parts.front().name == "core") {
            std::filesystem::path core_root = std_root / "core";
            candidates.push_back(core_root / (rel.string() + ".vit"));
            candidates.push_back(core_root / rel / "mod.vit");
        }
        candidates.push_back(std_root / (rel.string() + ".vit"));
        candidates.push_back(std_root / rel / "mod.vit");
    } else if (is_vitte_package_root(path)) {
        std::filesystem::path pkg_root = repo_root / "src/vitte/packages";
        candidates.push_back(pkg_root / (rel.string() + ".vit"));
        candidates.push_back(pkg_root / rel / "mod.vit");
    } else {
        candidates.push_back(base_dir / (rel.string() + ".vit"));
        candidates.push_back(base_dir / rel / "mod.vit");
    }
    std::vector<std::filesystem::path> found;
    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) {
            found.push_back(c);
        }
    }
    return found;
}

static bool has_ambiguous_module_file(
    const ModulePath& path,
    const std::filesystem::path& base_dir,
    const std::filesystem::path& repo_root
) {
    return existing_module_files(path, base_dir, repo_root).size() > 1;
}

static std::filesystem::path detect_repo_root(const std::filesystem::path& start_dir) {
    std::filesystem::path cur = start_dir;
    if (cur.empty()) {
        cur = std::filesystem::current_path();
    }
    if (!std::filesystem::exists(cur)) {
        cur = std::filesystem::current_path();
    }

    while (true) {
        const auto candidate = cur / "src/vitte/std";
        if (std::filesystem::exists(candidate)) {
            return cur;
        }
        const auto parent = cur.parent_path();
        if (parent.empty() || parent == cur) {
            break;
        }
        cur = parent;
    }

    return std::filesystem::current_path();
}

static void collect_decl_names(
    const AstContext& ctx,
    ModuleId module_id,
    std::unordered_set<std::string>& names
) {
    const auto& module = ctx.get<Module>(module_id);
    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidAstId) continue;
        const auto& decl = ctx.get<Decl>(decl_id);
        switch (decl.kind) {
            case NodeKind::ProcDecl: {
                const auto& p = static_cast<const ProcDecl&>(decl);
                bool is_extern = false;
                for (const auto& attr : p.attrs) {
                    if (attr.name.name == "extern") {
                        is_extern = true;
                        break;
                    }
                }
                if (!is_extern) {
                    names.insert(p.name.name);
                }
                break;
            }
            case NodeKind::FnDecl:
                names.insert(static_cast<const FnDecl&>(decl).name.name);
                break;
            case NodeKind::ConstDecl:
                names.insert(static_cast<const ConstDecl&>(decl).name.name);
                break;
            case NodeKind::GlobalDecl:
                names.insert(static_cast<const GlobalDecl&>(decl).name.name);
                break;
            case NodeKind::TypeDecl:
                names.insert(static_cast<const TypeDecl&>(decl).name.name);
                break;
            case NodeKind::TypeAliasDecl:
                names.insert(static_cast<const TypeAliasDecl&>(decl).name.name);
                break;
            case NodeKind::FormDecl:
                names.insert(static_cast<const FormDecl&>(decl).name.name);
                break;
            case NodeKind::PickDecl:
                names.insert(static_cast<const PickDecl&>(decl).name.name);
                break;
            case NodeKind::MacroDecl:
                names.insert(static_cast<const MacroDecl&>(decl).name.name);
                break;
            default:
                break;
        }
    }
}

static void qualify_type(AstContext& ctx,
                         TypeId& type_id,
                         const std::unordered_set<std::string>& locals,
                         const std::string& prefix) {
    if (type_id == kInvalidAstId) {
        return;
    }
    auto& node = ctx.node(type_id);
    switch (node.kind) {
        case NodeKind::NamedType: {
            auto& t = static_cast<NamedType&>(node);
            auto dot = t.ident.name.find('.');
            if (dot != std::string::npos) {
                std::string base = t.ident.name.substr(0, dot);
                std::string member = t.ident.name.substr(dot + 1);
                if (locals.count(base)) {
                    t.ident.name = prefix + base + "." + member;
                }
                break;
            }
            if (locals.count(t.ident.name)) {
                t.ident.name = prefix + t.ident.name;
            }
            break;
        }
        case NodeKind::GenericType: {
            auto& t = static_cast<GenericType&>(node);
            auto dot = t.base_ident.name.find('.');
            if (dot != std::string::npos) {
                std::string base = t.base_ident.name.substr(0, dot);
                std::string member = t.base_ident.name.substr(dot + 1);
                if (locals.count(base)) {
                    t.base_ident.name = prefix + base + "." + member;
                }
            } else if (locals.count(t.base_ident.name)) {
                t.base_ident.name = prefix + t.base_ident.name;
            }
            for (auto& arg : t.type_args) {
                qualify_type(ctx, arg, locals, prefix);
            }
            break;
        }
        case NodeKind::PointerType: {
            auto& t = static_cast<PointerType&>(node);
            qualify_type(ctx, t.pointee, locals, prefix);
            break;
        }
        case NodeKind::SliceType: {
            auto& t = static_cast<SliceType&>(node);
            qualify_type(ctx, t.element, locals, prefix);
            break;
        }
        case NodeKind::ProcType: {
            auto& t = static_cast<ProcType&>(node);
            for (auto& p : t.params) {
                qualify_type(ctx, p, locals, prefix);
            }
            qualify_type(ctx, t.return_type, locals, prefix);
            break;
        }
        default:
            break;
    }
}

static void qualify_pattern(AstContext& ctx,
                            PatternId& pattern_id,
                            const std::unordered_set<std::string>& locals,
                            const std::string& prefix) {
    if (pattern_id == kInvalidAstId) {
        return;
    }
    auto& node = ctx.node(pattern_id);
    switch (node.kind) {
        case NodeKind::CtorPattern: {
            auto& p = static_cast<CtorPattern&>(node);
            qualify_type(ctx, p.type, locals, prefix);
            for (auto& arg : p.args) {
                qualify_pattern(ctx, arg, locals, prefix);
            }
            break;
        }
        case NodeKind::IdentPattern:
        default:
            break;
    }
}

static void qualify_expr(AstContext& ctx,
                         ExprId& expr_id,
                         const std::unordered_set<std::string>& locals,
                         const std::string& prefix);

static void qualify_stmt(AstContext& ctx,
                         StmtId& stmt_id,
                         const std::unordered_set<std::string>& locals,
                         const std::string& prefix) {
    if (stmt_id == kInvalidAstId) return;
    auto& node = ctx.get<Stmt>(stmt_id);
    switch (node.kind) {
        case NodeKind::BlockStmt: {
            auto& s = static_cast<BlockStmt&>(node);
            for (auto& st : s.stmts) {
                qualify_stmt(ctx, st, locals, prefix);
            }
            break;
        }
        case NodeKind::LetStmt: {
            auto& s = static_cast<LetStmt&>(node);
            qualify_type(ctx, s.type, locals, prefix);
            qualify_expr(ctx, s.initializer, locals, prefix);
            break;
        }
        case NodeKind::MakeStmt: {
            auto& s = static_cast<MakeStmt&>(node);
            qualify_type(ctx, s.type, locals, prefix);
            qualify_expr(ctx, s.value, locals, prefix);
            break;
        }
        case NodeKind::SetStmt: {
            auto& s = static_cast<SetStmt&>(node);
            qualify_expr(ctx, s.value, locals, prefix);
            break;
        }
        case NodeKind::GiveStmt: {
            auto& s = static_cast<GiveStmt&>(node);
            qualify_expr(ctx, s.value, locals, prefix);
            break;
        }
        case NodeKind::EmitStmt: {
            auto& s = static_cast<EmitStmt&>(node);
            qualify_expr(ctx, s.value, locals, prefix);
            break;
        }
        case NodeKind::ExprStmt: {
            auto& s = static_cast<ExprStmt&>(node);
            qualify_expr(ctx, s.expr, locals, prefix);
            break;
        }
        case NodeKind::ReturnStmt: {
            auto& s = static_cast<ReturnStmt&>(node);
            qualify_expr(ctx, s.expr, locals, prefix);
            break;
        }
        case NodeKind::IfStmt: {
            auto& s = static_cast<IfStmt&>(node);
            qualify_expr(ctx, s.cond, locals, prefix);
            qualify_stmt(ctx, s.then_block, locals, prefix);
            qualify_stmt(ctx, s.else_block, locals, prefix);
            break;
        }
        case NodeKind::LoopStmt: {
            auto& s = static_cast<LoopStmt&>(node);
            qualify_stmt(ctx, s.body, locals, prefix);
            break;
        }
        case NodeKind::SelectStmt: {
            auto& s = static_cast<SelectStmt&>(node);
            qualify_expr(ctx, s.expr, locals, prefix);
            for (auto& w : s.whens) {
                qualify_stmt(ctx, w, locals, prefix);
            }
            qualify_stmt(ctx, s.otherwise_block, locals, prefix);
            break;
        }
        case NodeKind::WhenStmt: {
            auto& s = static_cast<WhenStmt&>(node);
            qualify_pattern(ctx, s.pattern, locals, prefix);
            qualify_stmt(ctx, s.block, locals, prefix);
            break;
        }
        case NodeKind::ForStmt: {
            auto& s = static_cast<ForStmt&>(node);
            qualify_expr(ctx, s.iterable, locals, prefix);
            qualify_stmt(ctx, s.body, locals, prefix);
            break;
        }
        case NodeKind::AsmStmt:
        case NodeKind::UnsafeStmt:
        case NodeKind::BreakStmt:
        case NodeKind::ContinueStmt:
        default:
            break;
    }
}

static void qualify_expr(AstContext& ctx,
                         ExprId& expr_id,
                         const std::unordered_set<std::string>& locals,
                         const std::string& prefix) {
    if (expr_id == kInvalidAstId) return;
    auto& node = ctx.get<Expr>(expr_id);
    switch (node.kind) {
        case NodeKind::IdentExpr: {
            auto& e = static_cast<IdentExpr&>(node);
            if (locals.count(e.ident.name)) {
                e.ident.name = prefix + e.ident.name;
            }
            break;
        }
        case NodeKind::UnaryExpr: {
            auto& e = static_cast<UnaryExpr&>(node);
            qualify_expr(ctx, e.expr, locals, prefix);
            break;
        }
        case NodeKind::BinaryExpr: {
            auto& e = static_cast<BinaryExpr&>(node);
            qualify_expr(ctx, e.lhs, locals, prefix);
            qualify_expr(ctx, e.rhs, locals, prefix);
            break;
        }
        case NodeKind::ProcExpr: {
            auto& e = static_cast<ProcExpr&>(node);
            for (auto& p : e.params) {
                qualify_type(ctx, p.type, locals, prefix);
            }
            qualify_type(ctx, e.return_type, locals, prefix);
            qualify_stmt(ctx, e.body, locals, prefix);
            break;
        }
        case NodeKind::MemberExpr: {
            auto& e = static_cast<MemberExpr&>(node);
            qualify_expr(ctx, e.base, locals, prefix);
            break;
        }
        case NodeKind::IndexExpr: {
            auto& e = static_cast<IndexExpr&>(node);
            qualify_expr(ctx, e.base, locals, prefix);
            qualify_expr(ctx, e.index, locals, prefix);
            break;
        }
        case NodeKind::IfExpr: {
            auto& e = static_cast<IfExpr&>(node);
            qualify_expr(ctx, e.cond, locals, prefix);
            qualify_stmt(ctx, e.then_block, locals, prefix);
            qualify_stmt(ctx, e.else_block, locals, prefix);
            break;
        }
        case NodeKind::IsExpr: {
            auto& e = static_cast<IsExpr&>(node);
            qualify_expr(ctx, e.value, locals, prefix);
            break;
        }
        case NodeKind::AsExpr: {
            auto& e = static_cast<AsExpr&>(node);
            qualify_expr(ctx, e.value, locals, prefix);
            qualify_type(ctx, e.type, locals, prefix);
            break;
        }
        case NodeKind::InvokeExpr: {
            auto& e = static_cast<InvokeExpr&>(node);
            qualify_expr(ctx, e.callee_expr, locals, prefix);
            for (auto& a : e.args) {
                qualify_expr(ctx, a, locals, prefix);
            }
            break;
        }
        case NodeKind::CallNoParenExpr: {
            auto& e = static_cast<CallNoParenExpr&>(node);
            qualify_expr(ctx, e.arg, locals, prefix);
            break;
        }
        case NodeKind::ListExpr: {
            auto& e = static_cast<ListExpr&>(node);
            for (auto& item : e.items) {
                qualify_expr(ctx, item, locals, prefix);
            }
            break;
        }
        default:
            break;
    }
}

static void qualify_module(AstContext& ctx,
                           ModuleId module_id,
                           const std::string& prefix,
                           ModuleIndex& index) {
    std::unordered_set<std::string> locals;
    collect_decl_names(ctx, module_id, locals);

    if (!prefix.empty()) {
        index.exports[prefix].insert(locals.begin(), locals.end());
    }

    auto& module = ctx.get<Module>(module_id);
    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidAstId) continue;
        auto& decl = ctx.get<Decl>(decl_id);
        switch (decl.kind) {
            case NodeKind::ProcDecl: {
                auto& d = static_cast<ProcDecl&>(decl);
                bool is_extern = false;
                for (const auto& attr : d.attrs) {
                    if (attr.name.name == "extern") {
                        is_extern = true;
                        break;
                    }
                }
                if (!is_extern) {
                    d.name.name = prefix + d.name.name;
                }
                for (auto& p : d.params) {
                    qualify_type(ctx, p.type, locals, prefix);
                }
                qualify_type(ctx, d.return_type, locals, prefix);
                qualify_stmt(ctx, d.body, locals, prefix);
                break;
            }
            case NodeKind::FnDecl: {
                auto& d = static_cast<FnDecl&>(decl);
                d.name.name = prefix + d.name.name;
                for (auto& p : d.params) {
                    qualify_type(ctx, p.type, locals, prefix);
                }
                qualify_type(ctx, d.return_type, locals, prefix);
                qualify_stmt(ctx, d.body, locals, prefix);
                break;
            }
            case NodeKind::ConstDecl: {
                auto& d = static_cast<ConstDecl&>(decl);
                d.name.name = prefix + d.name.name;
                qualify_type(ctx, d.type, locals, prefix);
                qualify_expr(ctx, d.value, locals, prefix);
                break;
            }
            case NodeKind::GlobalDecl: {
                auto& d = static_cast<GlobalDecl&>(decl);
                d.name.name = prefix + d.name.name;
                qualify_type(ctx, d.type, locals, prefix);
                qualify_expr(ctx, d.value, locals, prefix);
                break;
            }
            case NodeKind::TypeDecl: {
                auto& d = static_cast<TypeDecl&>(decl);
                d.name.name = prefix + d.name.name;
                for (auto& f : d.fields) {
                    qualify_type(ctx, f.type, locals, prefix);
                }
                break;
            }
            case NodeKind::TypeAliasDecl: {
                auto& d = static_cast<TypeAliasDecl&>(decl);
                d.name.name = prefix + d.name.name;
                qualify_type(ctx, d.target, locals, prefix);
                break;
            }
            case NodeKind::FormDecl: {
                auto& d = static_cast<FormDecl&>(decl);
                d.name.name = prefix + d.name.name;
                for (auto& f : d.fields) {
                    qualify_type(ctx, f.type, locals, prefix);
                }
                break;
            }
            case NodeKind::PickDecl: {
                auto& d = static_cast<PickDecl&>(decl);
                d.name.name = prefix + d.name.name;
                for (auto& c : d.cases) {
                    for (auto& f : c.fields) {
                        qualify_type(ctx, f.type, locals, prefix);
                    }
                }
                break;
            }
            case NodeKind::MacroDecl: {
                auto& d = static_cast<MacroDecl&>(decl);
                d.name.name = prefix + d.name.name;
                qualify_stmt(ctx, d.body, locals, prefix);
                break;
            }
            case NodeKind::EntryDecl:
            default:
                break;
        }
    }
}

struct Loader {
    AstContext& ctx;
    diag::DiagnosticEngine& diagnostics;
    ModuleIndex& index;
    LoadOptions options;
    std::unordered_set<std::string> loaded;
    std::unordered_map<std::string, std::string> file_to_key;
    std::vector<DeclId> collected;
    std::filesystem::path repo_root;

    bool load_recursive(ModuleId root,
                        const std::filesystem::path& base_dir,
                        const std::string& current_module_key) {
        auto& module = ctx.get<Module>(root);

        for (auto decl_id : module.decls) {
            if (decl_id == kInvalidAstId) continue;
            const auto& decl = ctx.get<Decl>(decl_id);
            const ModulePath* path = nullptr;
            if (decl.kind == NodeKind::UseDecl) {
                path = &static_cast<const UseDecl&>(decl).path;
            } else if (decl.kind == NodeKind::PullDecl) {
                path = &static_cast<const PullDecl&>(decl).path;
            }
            if (!path) continue;

            ModulePath module_path = *path;
            const std::string raw_key = module_path_key(module_path);
            const std::string normalized = normalized_stdlib_path(module_path);
            if (is_experimental_module_key(raw_key) && !options.allow_experimental) {
                if (options.warn_experimental) {
                    diagnostics.warning_code(
                        "E1015",
                        "experimental module '" + raw_key + "' is imported in warning mode (--warn-experimental)",
                        (*path).span
                    );
                } else {
                    diagnostics.error_code(
                        "E1015",
                        "experimental module '" + raw_key + "' is forbidden (use --allow-experimental)",
                        (*path).span
                    );
                    continue;
                }
            }
            if (!normalized.empty() && is_experimental_module_key(normalized) && !options.allow_experimental) {
                if (options.warn_experimental) {
                    diagnostics.warning_code(
                        "E1015",
                        "experimental stdlib module '" + normalized + "' is imported in warning mode (--warn-experimental)",
                        (*path).span
                    );
                } else {
                    diagnostics.error_code(
                        "E1015",
                        "experimental stdlib module '" + normalized + "' is forbidden (use --allow-experimental)",
                        (*path).span
                    );
                    continue;
                }
            }
            if (!is_valid_stdlib_profile(options.stdlib_profile)) {
                diagnostics.error("invalid stdlib profile: " + options.stdlib_profile, (*path).span);
                continue;
            }
            if (!normalized.empty() &&
                !is_stdlib_path_allowed(module_path, options.stdlib_profile)) {
                diag::Diagnostic denied(
                    diag::Severity::Error,
                    "E1010",
                    "stdlib module '" + normalized + "' is not allowed in profile '" + options.stdlib_profile + "'",
                    (*path).span
                );
                denied.add_note(
                    "allowed by this profile: core=std/core, system=std/core+std/kernel, arduino=std/core+std/arduino, desktop=all"
                );
                denied.add_note(
                    "fix: switch profile (e.g. --runtime-profile desktop) or replace this import with a module allowed by '" + options.stdlib_profile + "'"
                );
                denied.add_fix("switch runtime profile", "--runtime-profile desktop", (*path).span);
                denied.add_fix("replace denied import", "use std/core/... as core_mod", (*path).span);
                diagnostics.emit(std::move(denied));
                continue;
            }
            if (has_ambiguous_module_file(module_path, base_dir, repo_root)) {
                diag::Diagnostic ambiguous(
                    diag::Severity::Error,
                    "E1018",
                    "ambiguous import path '" + module_path_key(module_path) + "' (both file.vit and mod.vit exist)",
                    (*path).span
                );
                const auto matches = existing_module_files(module_path, base_dir, repo_root);
                if (matches.size() >= 2) {
                    ambiguous.add_note(
                        "candidates: '" + matches[0].filename().string() + "' and '" + matches[1].filename().string() + "'"
                    );
                    ambiguous.add_fix("keep file module form", "remove '" + matches[1].filename().string() + "'", (*path).span);
                    ambiguous.add_fix("keep directory module form", "remove '" + matches[0].filename().string() + "'", (*path).span);
                }
                ambiguous.add_note(
                    "fix: keep only one module layout for this path (prefer directory form '<name>/mod.vit' or file form '<name>.vit')"
                );
                diagnostics.emit(std::move(ambiguous));
                continue;
            }
            std::filesystem::path file = resolve_module_file(module_path, base_dir, repo_root);
            if (file.empty() && module_path.parts.size() > 1) {
                ModulePath parent = module_path;
                parent.parts.pop_back();
                std::filesystem::path parent_file = resolve_module_file(parent, base_dir, repo_root);
                if (!parent_file.empty()) {
                    module_path = std::move(parent);
                    file = std::move(parent_file);
                }
            }
            if (file.empty()) {
                const std::string key = module_path_key(*path);
                const std::string missing_normalized = normalized_stdlib_path(*path);
                if (!missing_normalized.empty()) {
                    diagnostics.error_code(
                        "E1014",
                        "stdlib module not found: " + missing_normalized,
                        (*path).span
                    );
                } else {
                    diagnostics.error("module not found: " + key, (*path).span);
                }
                continue;
            }

            std::string key = module_path_key(module_path);
            std::string file_key = file.string();
            auto canonical = file_to_key.find(file_key);
            if (canonical != file_to_key.end()) {
                key = canonical->second;
            } else {
                file_to_key.emplace(file_key, key);
            }
            if (options.deny_internal && is_internal_module_key(key)) {
                std::string owner = internal_owner_namespace(key);
                if (owner.empty() ||
                    (!has_prefix(current_module_key, owner + "/") && current_module_key != owner)) {
                    diagnostics.error_code(
                        "E1016",
                        "internal module '" + key + "' cannot be imported from '" + current_module_key + "'",
                        (*path).span
                    );
                    diagnostics.note("internal modules are private to their owning namespace", (*path).span);
                    continue;
                }
            }

            index.imports[current_module_key].insert(key);
            std::string prefix = module_prefix(module_path);
            if (!prefix.empty() && index.path_to_prefix.find(key) == index.path_to_prefix.end()) {
                index.path_to_prefix.emplace(key, prefix);
            }
            index.module_files[key] = file.string();

            if (loaded.count(file_key)) {
                continue;
            }
            loaded.insert(file_key);

            std::ifstream in(file);
            if (!in.is_open()) {
                diagnostics.error("failed to open module file: " + file.string(), (*path).span);
                continue;
            }

            std::string source(
                (std::istreambuf_iterator<char>(in)),
                std::istreambuf_iterator<char>()
            );
            index.module_loc[key] = count_lines(source);

            Lexer lexer(source, file.string());
            ctx.sources.push_back(lexer.source_file());
            parser::Parser parser(lexer, diagnostics, ctx, false);
            ModuleId mod_id = parser.parse_module();

            std::filesystem::path mod_dir = file.parent_path();
            load_recursive(mod_id, mod_dir, key);

            qualify_module(ctx, mod_id, prefix, index);

            const auto& mod = ctx.get<Module>(mod_id);
            collected.insert(collected.end(), mod.decls.begin(), mod.decls.end());
        }

        return true;
    }
};

bool load_modules(AstContext& ctx,
                  ModuleId root,
                  diag::DiagnosticEngine& diagnostics,
                  const std::string& entry_path,
                  ModuleIndex& index,
                  const LoadOptions& options) {
    std::filesystem::path entry = std::filesystem::absolute(std::filesystem::path(entry_path));
    std::filesystem::path base_dir = entry.has_parent_path()
        ? entry.parent_path()
        : std::filesystem::current_path();
    index.module_files["__root__"] = entry.string();
    std::ifstream root_in(entry);
    if (root_in.is_open()) {
        std::string src(
            (std::istreambuf_iterator<char>(root_in)),
            std::istreambuf_iterator<char>()
        );
        index.module_loc["__root__"] = count_lines(src);
    }

    Loader loader{ctx, diagnostics, index, options, {}, {}, {}, detect_repo_root(base_dir)};
    bool ok = loader.load_recursive(root, base_dir, "__root__");
    if (ok && !loader.collected.empty()) {
        auto& module = ctx.get<Module>(root);
        auto insert_it = module.decls.begin();
        for (; insert_it != module.decls.end(); ++insert_it) {
            if (*insert_it == kInvalidAstId) {
                continue;
            }
            const auto& decl = ctx.get<Decl>(*insert_it);
            if (decl.kind == NodeKind::UseDecl ||
                decl.kind == NodeKind::PullDecl ||
                decl.kind == NodeKind::SpaceDecl ||
                decl.kind == NodeKind::ShareDecl) {
                continue;
            }
            break;
        }
        module.decls.insert(insert_it,
                            loader.collected.begin(),
                            loader.collected.end());
    }
    return ok;
}

void dump_stdlib_map(std::ostream& os, const ModuleIndex& index) {
    std::vector<std::pair<std::string, std::string>> module_to_prefix;
    module_to_prefix.reserve(index.path_to_prefix.size());
    for (const auto& kv : index.path_to_prefix) {
        if (has_prefix(kv.first, "std/") || has_prefix(kv.first, "core/")) {
            module_to_prefix.push_back(kv);
        }
    }
    std::sort(module_to_prefix.begin(), module_to_prefix.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    os << "stdlib_map:\n";
    for (const auto& it : module_to_prefix) {
        os << "  " << it.first << ":\n";
        auto exp_it = index.exports.find(it.second);
        if (exp_it == index.exports.end() || exp_it->second.empty()) {
            os << "    - <no-exports>\n";
            continue;
        }
        std::vector<std::string> names(exp_it->second.begin(), exp_it->second.end());
        std::sort(names.begin(), names.end());
        for (const auto& name : names) {
            os << "    - " << name << "\n";
        }
    }
}

void dump_module_index_json(std::ostream& os,
                            const ModuleIndex& index,
                            const std::string& profile,
                            bool allow_experimental) {
    std::vector<std::string> modules;
    modules.reserve(index.module_files.size());
    for (const auto& kv : index.module_files) {
        modules.push_back(kv.first);
    }
    std::sort(modules.begin(), modules.end());

    os << "{\n";
    os << "  \"profile\": \"" << profile << "\",\n";
    os << "  \"allow_experimental\": " << (allow_experimental ? "true" : "false") << ",\n";
    os << "  \"modules\": [\n";
    for (std::size_t i = 0; i < modules.size(); ++i) {
        const auto& key = modules[i];
        os << "    {\n";
        os << "      \"key\": \"" << key << "\",\n";
        auto file_it = index.module_files.find(key);
        os << "      \"file\": \"" << (file_it == index.module_files.end() ? "" : file_it->second) << "\",\n";
        auto loc_it = index.module_loc.find(key);
        os << "      \"loc\": " << (loc_it == index.module_loc.end() ? 0 : loc_it->second) << ",\n";
        std::string level = "public";
        if (is_internal_module_key(key)) {
            level = "internal";
        } else if (is_experimental_module_key(key)) {
            level = "experimental";
        }
        os << "      \"level\": \"" << level << "\",\n";

        os << "      \"imports\": [";
        if (auto imp_it = index.imports.find(key); imp_it != index.imports.end()) {
            std::vector<std::string> imps(imp_it->second.begin(), imp_it->second.end());
            std::sort(imps.begin(), imps.end());
            for (std::size_t j = 0; j < imps.size(); ++j) {
                if (j) os << ", ";
                os << "\"" << imps[j] << "\"";
            }
        }
        os << "],\n";

        os << "      \"exports\": [";
        if (auto p_it = index.path_to_prefix.find(key); p_it != index.path_to_prefix.end()) {
            if (auto e_it = index.exports.find(p_it->second); e_it != index.exports.end()) {
                std::vector<std::string> exps(e_it->second.begin(), e_it->second.end());
                std::sort(exps.begin(), exps.end());
                for (std::size_t j = 0; j < exps.size(); ++j) {
                    if (j) os << ", ";
                    os << "\"" << exps[j] << "\"";
                }
            }
        }
        os << "],\n";
        os << "      \"warnings\": []\n";
        os << "    }";
        if (i + 1 < modules.size()) {
            os << ",";
        }
        os << "\n";
    }
    os << "  ]\n";
    os << "}\n";
}

static void rewrite_stmt_for_alias(
    AstContext& ctx,
    StmtId& stmt_id,
    const std::unordered_map<std::string, std::string>& alias_to_prefix,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& exports,
    const std::unordered_set<std::string>& glob_aliases,
    const std::unordered_map<std::string, std::string>& symbol_imports
);

static void rewrite_type_for_alias(
    AstContext& ctx,
    TypeId& type_id,
    const std::unordered_map<std::string, std::string>& alias_to_prefix,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& exports,
    const std::unordered_set<std::string>& glob_aliases,
    const std::unordered_map<std::string, std::string>& symbol_imports
);

static void rewrite_pattern_for_alias(
    AstContext& ctx,
    PatternId& pattern_id,
    const std::unordered_map<std::string, std::string>& alias_to_prefix,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& exports,
    const std::unordered_set<std::string>& glob_aliases,
    const std::unordered_map<std::string, std::string>& symbol_imports
) {
    if (pattern_id == kInvalidAstId) return;
    auto& node = ctx.node(pattern_id);
    switch (node.kind) {
        case NodeKind::CtorPattern: {
            auto& p = static_cast<CtorPattern&>(node);
            rewrite_type_for_alias(ctx, p.type, alias_to_prefix, exports, glob_aliases, symbol_imports);
            for (auto& arg : p.args) {
                rewrite_pattern_for_alias(ctx, arg, alias_to_prefix, exports, glob_aliases, symbol_imports);
            }
            break;
        }
        case NodeKind::IdentPattern:
        default:
            break;
    }
}

static void rewrite_type_for_alias(
    AstContext& ctx,
    TypeId& type_id,
    const std::unordered_map<std::string, std::string>& alias_to_prefix,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& exports,
    const std::unordered_set<std::string>& glob_aliases,
    const std::unordered_map<std::string, std::string>& symbol_imports
) {
    if (type_id == kInvalidAstId) return;
    auto& node = ctx.node(type_id);
    switch (node.kind) {
        case NodeKind::NamedType: {
            auto& t = static_cast<NamedType&>(node);
            auto dot = t.ident.name.find('.');
            if (dot != std::string::npos) {
                std::string base = t.ident.name.substr(0, dot);
                std::string member = t.ident.name.substr(dot + 1);
                auto it_alias = alias_to_prefix.find(base);
                if (it_alias != alias_to_prefix.end()) {
                    t.ident.name = it_alias->second + member;
                    break;
                }
                auto it_sym = symbol_imports.find(base);
                if (it_sym != symbol_imports.end()) {
                    t.ident.name = it_sym->second + "." + member;
                    break;
                }
            }
            auto it = symbol_imports.find(t.ident.name);
            if (it != symbol_imports.end()) {
                t.ident.name = it->second;
                break;
            }
            for (const auto& alias : glob_aliases) {
                auto pfx = alias_to_prefix.find(alias);
                if (pfx == alias_to_prefix.end()) continue;
                auto exp = exports.find(pfx->second);
                if (exp != exports.end() && exp->second.count(t.ident.name)) {
                    t.ident.name = pfx->second + t.ident.name;
                    break;
                }
            }
            break;
        }
        case NodeKind::GenericType: {
            auto& t = static_cast<GenericType&>(node);
            auto dot = t.base_ident.name.find('.');
            if (dot != std::string::npos) {
                std::string base = t.base_ident.name.substr(0, dot);
                std::string member = t.base_ident.name.substr(dot + 1);
                auto it_alias = alias_to_prefix.find(base);
                if (it_alias != alias_to_prefix.end()) {
                    t.base_ident.name = it_alias->second + member;
                }
                auto it_sym = symbol_imports.find(base);
                if (it_sym != symbol_imports.end()) {
                    t.base_ident.name = it_sym->second + "." + member;
                }
            }
            auto it = symbol_imports.find(t.base_ident.name);
            if (it != symbol_imports.end()) {
                t.base_ident.name = it->second;
            } else {
                for (const auto& alias : glob_aliases) {
                    auto pfx = alias_to_prefix.find(alias);
                    if (pfx == alias_to_prefix.end()) continue;
                    auto exp = exports.find(pfx->second);
                    if (exp != exports.end() && exp->second.count(t.base_ident.name)) {
                        t.base_ident.name = pfx->second + t.base_ident.name;
                        break;
                    }
                }
            }
            for (auto& arg : t.type_args) {
                rewrite_type_for_alias(ctx, arg, alias_to_prefix, exports, glob_aliases, symbol_imports);
            }
            break;
        }
        case NodeKind::PointerType: {
            auto& t = static_cast<PointerType&>(node);
            rewrite_type_for_alias(ctx, t.pointee, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::SliceType: {
            auto& t = static_cast<SliceType&>(node);
            rewrite_type_for_alias(ctx, t.element, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::ProcType: {
            auto& t = static_cast<ProcType&>(node);
            for (auto& p : t.params) {
                rewrite_type_for_alias(ctx, p, alias_to_prefix, exports, glob_aliases, symbol_imports);
            }
            rewrite_type_for_alias(ctx, t.return_type, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        default:
            break;
    }
}

static void rewrite_expr_for_alias(
    AstContext& ctx,
    ExprId& expr_id,
    const std::unordered_map<std::string, std::string>& alias_to_prefix,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& exports,
    const std::unordered_set<std::string>& glob_aliases,
    const std::unordered_map<std::string, std::string>& symbol_imports
) {
    if (expr_id == kInvalidAstId) return;
    auto& node = ctx.get<Expr>(expr_id);
    switch (node.kind) {
        case NodeKind::MemberExpr: {
            auto& e = static_cast<MemberExpr&>(node);
            rewrite_expr_for_alias(ctx, e.base, alias_to_prefix, exports, glob_aliases, symbol_imports);
            if (e.base == kInvalidAstId) {
                return;
            }
            auto& base_node = ctx.get<Expr>(e.base);
            if (base_node.kind == NodeKind::IdentExpr) {
                auto& base = static_cast<IdentExpr&>(base_node);
                auto it = alias_to_prefix.find(base.ident.name);
                if (it != alias_to_prefix.end()) {
                    const auto& prefix = it->second;
                    std::string qual = prefix + e.member.name;
                    expr_id = ctx.make<IdentExpr>(Ident(qual, e.span), e.span);
                }
            }
            break;
        }
        case NodeKind::IdentExpr: {
            auto& e = static_cast<IdentExpr&>(node);
            auto dot = e.ident.name.find('.');
            if (dot != std::string::npos) {
                std::string base = e.ident.name.substr(0, dot);
                std::string member = e.ident.name.substr(dot + 1);
                auto it_alias = alias_to_prefix.find(base);
                if (it_alias != alias_to_prefix.end()) {
                    e.ident.name = it_alias->second + member;
                    break;
                }
                auto it_sym = symbol_imports.find(base);
                if (it_sym != symbol_imports.end()) {
                    e.ident.name = it_sym->second + "." + member;
                    break;
                }
            }
            auto it_sym = symbol_imports.find(e.ident.name);
            if (it_sym != symbol_imports.end()) {
                e.ident.name = it_sym->second;
                break;
            }
            for (const auto& alias : glob_aliases) {
                auto it = alias_to_prefix.find(alias);
                if (it == alias_to_prefix.end()) {
                    continue;
                }
                const auto& prefix = it->second;
                auto exp = exports.find(prefix);
                if (exp == exports.end()) {
                    continue;
                }
                if (exp->second.count(e.ident.name)) {
                    e.ident.name = prefix + e.ident.name;
                    break;
                }
            }
            break;
        }
        case NodeKind::UnaryExpr: {
            auto& e = static_cast<UnaryExpr&>(node);
            rewrite_expr_for_alias(ctx, e.expr, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::BinaryExpr: {
            auto& e = static_cast<BinaryExpr&>(node);
            rewrite_expr_for_alias(ctx, e.lhs, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_expr_for_alias(ctx, e.rhs, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::ProcExpr: {
            auto& e = static_cast<ProcExpr&>(node);
            for (auto& p : e.params) {
                rewrite_type_for_alias(ctx, p.type, alias_to_prefix, exports, glob_aliases, symbol_imports);
            }
            rewrite_type_for_alias(ctx, e.return_type, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_stmt_for_alias(ctx, e.body, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::IndexExpr: {
            auto& e = static_cast<IndexExpr&>(node);
            rewrite_expr_for_alias(ctx, e.base, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_expr_for_alias(ctx, e.index, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::IfExpr: {
            auto& e = static_cast<IfExpr&>(node);
            rewrite_expr_for_alias(ctx, e.cond, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_stmt_for_alias(ctx, e.then_block, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_stmt_for_alias(ctx, e.else_block, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::IsExpr: {
            auto& e = static_cast<IsExpr&>(node);
            rewrite_expr_for_alias(ctx, e.value, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::AsExpr: {
            auto& e = static_cast<AsExpr&>(node);
            rewrite_expr_for_alias(ctx, e.value, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_type_for_alias(ctx, e.type, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::InvokeExpr: {
            auto& e = static_cast<InvokeExpr&>(node);
            rewrite_expr_for_alias(ctx, e.callee_expr, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_type_for_alias(ctx, e.callee_type, alias_to_prefix, exports, glob_aliases, symbol_imports);
            for (auto& a : e.args) {
                rewrite_expr_for_alias(ctx, a, alias_to_prefix, exports, glob_aliases, symbol_imports);
            }
            break;
        }
        case NodeKind::CallNoParenExpr: {
            auto& e = static_cast<CallNoParenExpr&>(node);
            auto dot = e.callee.name.find('.');
            if (dot != std::string::npos) {
                std::string base = e.callee.name.substr(0, dot);
                std::string member = e.callee.name.substr(dot + 1);
                auto it_alias = alias_to_prefix.find(base);
                if (it_alias != alias_to_prefix.end()) {
                    e.callee.name = it_alias->second + member;
                    break;
                }
                auto it_sym = symbol_imports.find(base);
                if (it_sym != symbol_imports.end()) {
                    e.callee.name = it_sym->second + "." + member;
                }
            }
            auto it_sym = symbol_imports.find(e.callee.name);
            if (it_sym != symbol_imports.end()) {
                e.callee.name = it_sym->second;
            } else {
                for (const auto& alias : glob_aliases) {
                    auto it = alias_to_prefix.find(alias);
                    if (it == alias_to_prefix.end()) {
                        continue;
                    }
                    const auto& prefix = it->second;
                    auto exp = exports.find(prefix);
                    if (exp != exports.end() && exp->second.count(e.callee.name)) {
                        e.callee.name = prefix + e.callee.name;
                        break;
                    }
                }
            }
            rewrite_expr_for_alias(ctx, e.arg, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::ListExpr: {
            auto& e = static_cast<ListExpr&>(node);
            for (auto& item : e.items) {
                rewrite_expr_for_alias(ctx, item, alias_to_prefix, exports, glob_aliases, symbol_imports);
            }
            break;
        }
        default:
            break;
    }
}

static void rewrite_stmt_for_alias(
    AstContext& ctx,
    StmtId& stmt_id,
    const std::unordered_map<std::string, std::string>& alias_to_prefix,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& exports,
    const std::unordered_set<std::string>& glob_aliases,
    const std::unordered_map<std::string, std::string>& symbol_imports
) {
    if (stmt_id == kInvalidAstId) return;
    auto& node = ctx.get<Stmt>(stmt_id);
    switch (node.kind) {
        case NodeKind::BlockStmt: {
            auto& s = static_cast<BlockStmt&>(node);
            for (auto& st : s.stmts) {
                rewrite_stmt_for_alias(ctx, st, alias_to_prefix, exports, glob_aliases, symbol_imports);
            }
            break;
        }
        case NodeKind::LetStmt: {
            auto& s = static_cast<LetStmt&>(node);
            rewrite_type_for_alias(ctx, s.type, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_expr_for_alias(ctx, s.initializer, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::MakeStmt: {
            auto& s = static_cast<MakeStmt&>(node);
            rewrite_type_for_alias(ctx, s.type, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_expr_for_alias(ctx, s.value, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::SetStmt: {
            auto& s = static_cast<SetStmt&>(node);
            rewrite_expr_for_alias(ctx, s.value, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::GiveStmt: {
            auto& s = static_cast<GiveStmt&>(node);
            rewrite_expr_for_alias(ctx, s.value, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::EmitStmt: {
            auto& s = static_cast<EmitStmt&>(node);
            rewrite_expr_for_alias(ctx, s.value, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::ExprStmt: {
            auto& s = static_cast<ExprStmt&>(node);
            rewrite_expr_for_alias(ctx, s.expr, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::ReturnStmt: {
            auto& s = static_cast<ReturnStmt&>(node);
            rewrite_expr_for_alias(ctx, s.expr, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::IfStmt: {
            auto& s = static_cast<IfStmt&>(node);
            rewrite_expr_for_alias(ctx, s.cond, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_stmt_for_alias(ctx, s.then_block, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_stmt_for_alias(ctx, s.else_block, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::LoopStmt: {
            auto& s = static_cast<LoopStmt&>(node);
            rewrite_stmt_for_alias(ctx, s.body, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::SelectStmt: {
            auto& s = static_cast<SelectStmt&>(node);
            rewrite_expr_for_alias(ctx, s.expr, alias_to_prefix, exports, glob_aliases, symbol_imports);
            for (auto& w : s.whens) {
                rewrite_stmt_for_alias(ctx, w, alias_to_prefix, exports, glob_aliases, symbol_imports);
            }
            rewrite_stmt_for_alias(ctx, s.otherwise_block, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::WhenStmt: {
            auto& s = static_cast<WhenStmt&>(node);
            rewrite_pattern_for_alias(ctx, s.pattern, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_stmt_for_alias(ctx, s.block, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        case NodeKind::ForStmt: {
            auto& s = static_cast<ForStmt&>(node);
            rewrite_expr_for_alias(ctx, s.iterable, alias_to_prefix, exports, glob_aliases, symbol_imports);
            rewrite_stmt_for_alias(ctx, s.body, alias_to_prefix, exports, glob_aliases, symbol_imports);
            break;
        }
        default:
            break;
    }
}

void rewrite_member_access(ast::AstContext& ctx,
                           ast::ModuleId root,
                           const ModuleIndex& index,
                           diag::DiagnosticEngine* diagnostics) {
    std::unordered_map<std::string, std::string> alias_to_prefix;
    std::unordered_map<std::string, SourceSpan> alias_to_span;
    std::unordered_set<std::string> glob_aliases;
    std::unordered_map<std::string, std::string> symbol_imports;

    const auto& module = ctx.get<Module>(root);
    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidAstId) continue;
        const auto& decl = ctx.get<Decl>(decl_id);
        if (decl.kind != NodeKind::UseDecl) {
            continue;
        }
        const auto& u = static_cast<const UseDecl&>(decl);
        std::string alias = u.alias ? u.alias->name
                                    : (u.path.parts.empty() ? "" : u.path.parts.back().name);
        if (alias.empty()) {
            continue;
        }
        std::string key = module_path_key(u.path);
        auto it = index.path_to_prefix.find(key);
        if (it != index.path_to_prefix.end()) {
            alias_to_prefix[alias] = it->second;
            alias_to_span[alias] = u.span;
            if (u.is_glob) {
                glob_aliases.insert(alias);
            }
            continue;
        }
        if (u.path.parts.size() > 1) {
            ModulePath parent = u.path;
            Ident last = parent.parts.back();
            parent.parts.pop_back();
            std::string parent_key = module_path_key(parent);
            auto pit = index.path_to_prefix.find(parent_key);
            if (pit != index.path_to_prefix.end()) {
                symbol_imports[alias] = pit->second + last.name;
            }
        }
    }

    if (diagnostics != nullptr) {
        std::unordered_map<std::string, std::string> seen_symbol_owner;
        for (const auto& alias : glob_aliases) {
            auto pfx = alias_to_prefix.find(alias);
            if (pfx == alias_to_prefix.end()) {
                continue;
            }
            auto exp = index.exports.find(pfx->second);
            if (exp == index.exports.end()) {
                continue;
            }
            for (const auto& sym : exp->second) {
                auto it = seen_symbol_owner.find(sym);
                if (it != seen_symbol_owner.end() && it->second != alias) {
                    SourceSpan conflict_span{};
                    auto current_span = alias_to_span.find(alias);
                    if (current_span != alias_to_span.end()) {
                        conflict_span = current_span->second;
                    }
                    diag::Diagnostic conflict(
                        diag::Severity::Error,
                        "E1017",
                        "re-export symbol conflict for '" + sym + "' between aliases '" + it->second + "' and '" + alias + "'",
                        conflict_span
                    );
                    auto owner_span = alias_to_span.find(it->second);
                    if (owner_span != alias_to_span.end()) {
                        conflict.add_note(
                            "first conflicting glob alias '" + it->second + "' is declared here"
                        );
                    }
                    conflict.add_note(
                        "fix: replace one glob import with explicit symbols (e.g. use module.{name} as alias_name)"
                    );
                    conflict.add_note(
                        "fix: keep only one glob alias exporting '" + sym + "'"
                    );
                    conflict.add_fix(
                        "replace glob with explicit symbol import",
                        "use module.{" + sym + "} as " + alias + "_" + sym,
                        conflict_span
                    );
                    conflict.add_fix(
                        "remove one conflicting glob alias",
                        "remove alias '" + alias + "' or '" + it->second + "'",
                        conflict_span
                    );
                    diagnostics->emit(std::move(conflict));
                } else {
                    seen_symbol_owner[sym] = alias;
                }
            }
        }
    }

    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidAstId) continue;
        auto& decl = ctx.get<Decl>(decl_id);
        switch (decl.kind) {
            case NodeKind::ProcDecl: {
                auto& d = static_cast<ProcDecl&>(decl);
                for (auto& p : d.params) {
                    rewrite_type_for_alias(ctx, p.type, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                }
                rewrite_type_for_alias(ctx, d.return_type, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                rewrite_stmt_for_alias(ctx, d.body, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                break;
            }
            case NodeKind::FnDecl: {
                auto& d = static_cast<FnDecl&>(decl);
                for (auto& p : d.params) {
                    rewrite_type_for_alias(ctx, p.type, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                }
                rewrite_type_for_alias(ctx, d.return_type, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                rewrite_stmt_for_alias(ctx, d.body, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                break;
            }
            case NodeKind::MacroDecl: {
                auto& d = static_cast<MacroDecl&>(decl);
                rewrite_stmt_for_alias(ctx, d.body, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                break;
            }
            case NodeKind::ConstDecl: {
                auto& d = static_cast<ConstDecl&>(decl);
                rewrite_type_for_alias(ctx, d.type, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                rewrite_expr_for_alias(ctx, d.value, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                break;
            }
            case NodeKind::GlobalDecl: {
                auto& d = static_cast<GlobalDecl&>(decl);
                rewrite_type_for_alias(ctx, d.type, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                rewrite_expr_for_alias(ctx, d.value, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                break;
            }
            case NodeKind::TypeDecl: {
                auto& d = static_cast<TypeDecl&>(decl);
                for (auto& f : d.fields) {
                    rewrite_type_for_alias(ctx, f.type, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                }
                break;
            }
            case NodeKind::TypeAliasDecl: {
                auto& d = static_cast<TypeAliasDecl&>(decl);
                rewrite_type_for_alias(ctx, d.target, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                break;
            }
            case NodeKind::FormDecl: {
                auto& d = static_cast<FormDecl&>(decl);
                for (auto& f : d.fields) {
                    rewrite_type_for_alias(ctx, f.type, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                }
                break;
            }
            case NodeKind::PickDecl: {
                auto& d = static_cast<PickDecl&>(decl);
                for (auto& c : d.cases) {
                    for (auto& f : c.fields) {
                        rewrite_type_for_alias(ctx, f.type, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                    }
                }
                break;
            }
            case NodeKind::EntryDecl: {
                auto& d = static_cast<EntryDecl&>(decl);
                rewrite_stmt_for_alias(ctx, d.body, alias_to_prefix, index.exports, glob_aliases, symbol_imports);
                break;
            }
            default:
                break;
        }
    }
}

} // namespace vitte::frontend::modules
