#include "macro_expand.hpp"

#include <unordered_map>

namespace vitte::frontend::passes {

using namespace vitte::frontend::ast;

namespace {

struct MacroDef {
    const MacroDecl* decl = nullptr;
};

struct Subst {
    std::string name;
    ExprId expr;
};

static Ident clone_ident(const Ident& id) {
    return Ident(id.name, id.span);
}

static ExprId clone_expr(
    AstContext& ctx,
    ExprId expr,
    const std::vector<Subst>& subst);

static ExprId clone_expr_no_subst(
    AstContext& ctx,
    ExprId expr) {
    std::vector<Subst> empty;
    return clone_expr(ctx, expr, empty);
}

static ExprId clone_expr(
    AstContext& ctx,
    ExprId expr,
    const std::vector<Subst>& subst) {
    if (expr == kInvalidAstId) {
        return kInvalidAstId;
    }

    const AstNode& node = ctx.node(expr);
    switch (node.kind) {
        case NodeKind::LiteralExpr: {
            auto& e = static_cast<const LiteralExpr&>(node);
            return ctx.make<LiteralExpr>(e.lit_kind, e.value, e.span);
        }
        case NodeKind::IdentExpr: {
            auto& e = static_cast<const IdentExpr&>(node);
            for (const auto& s : subst) {
                if (s.name == e.ident.name) {
                    return clone_expr_no_subst(ctx, s.expr);
                }
            }
            return ctx.make<IdentExpr>(clone_ident(e.ident), e.span);
        }
        case NodeKind::UnaryExpr: {
            auto& e = static_cast<const UnaryExpr&>(node);
            ExprId rhs = clone_expr(ctx, e.expr, subst);
            return ctx.make<UnaryExpr>(e.op, rhs, e.span);
        }
        case NodeKind::BinaryExpr: {
            auto& e = static_cast<const BinaryExpr&>(node);
            ExprId lhs = clone_expr(ctx, e.lhs, subst);
            ExprId rhs = clone_expr(ctx, e.rhs, subst);
            return ctx.make<BinaryExpr>(e.op, lhs, rhs, e.span);
        }
        case NodeKind::MemberExpr: {
            auto& e = static_cast<const MemberExpr&>(node);
            ExprId base = clone_expr(ctx, e.base, subst);
            return ctx.make<MemberExpr>(base, clone_ident(e.member), e.span);
        }
        case NodeKind::IndexExpr: {
            auto& e = static_cast<const IndexExpr&>(node);
            ExprId base = clone_expr(ctx, e.base, subst);
            ExprId idx = clone_expr(ctx, e.index, subst);
            return ctx.make<IndexExpr>(base, idx, e.span);
        }
        case NodeKind::ProcExpr: {
            auto& e = static_cast<const ProcExpr&>(node);
            std::vector<FnParam> params;
            params.reserve(e.params.size());
            for (const auto& p : e.params) {
                params.emplace_back(clone_ident(p.ident), p.type);
            }
            StmtId body = e.body;
            return ctx.make<ProcExpr>(std::move(params), e.return_type, body, e.span);
        }
        case NodeKind::IfExpr: {
            auto& e = static_cast<const IfExpr&>(node);
            ExprId cond = clone_expr(ctx, e.cond, subst);
            return ctx.make<IfExpr>(cond, e.then_block, e.else_block, e.span);
        }
        case NodeKind::IsExpr: {
            auto& e = static_cast<const IsExpr&>(node);
            ExprId val = clone_expr(ctx, e.value, subst);
            return ctx.make<IsExpr>(val, e.pattern, e.span);
        }
        case NodeKind::AsExpr: {
            auto& e = static_cast<const AsExpr&>(node);
            ExprId val = clone_expr(ctx, e.value, subst);
            return ctx.make<AsExpr>(val, e.type, e.span);
        }
        case NodeKind::CallNoParenExpr: {
            auto& e = static_cast<const CallNoParenExpr&>(node);
            ExprId arg = clone_expr(ctx, e.arg, subst);
            return ctx.make<CallNoParenExpr>(clone_ident(e.callee), arg, e.span);
        }
        case NodeKind::InvokeExpr: {
            auto& e = static_cast<const InvokeExpr&>(node);
            std::vector<ExprId> args;
            args.reserve(e.args.size());
            for (auto a : e.args) {
                args.push_back(clone_expr(ctx, a, subst));
            }
            ExprId callee = e.callee_expr != kInvalidAstId
                ? clone_expr(ctx, e.callee_expr, subst)
                : kInvalidAstId;
            return ctx.make<InvokeExpr>(callee, e.callee_type, std::move(args), e.span);
        }
        case NodeKind::ListExpr: {
            auto& e = static_cast<const ListExpr&>(node);
            std::vector<ExprId> items;
            items.reserve(e.items.size());
            for (auto it : e.items) {
                items.push_back(clone_expr(ctx, it, subst));
            }
            return ctx.make<ListExpr>(std::move(items), e.span);
        }
        default:
            return expr;
    }
}

static StmtId clone_stmt(
    AstContext& ctx,
    StmtId stmt,
    const std::vector<Subst>& subst);

static std::vector<StmtId> clone_stmt_list(
    AstContext& ctx,
    const std::vector<StmtId>& stmts,
    const std::vector<Subst>& subst) {
    std::vector<StmtId> out;
    out.reserve(stmts.size());
    for (auto s : stmts) {
        out.push_back(clone_stmt(ctx, s, subst));
    }
    return out;
}

static StmtId clone_stmt(
    AstContext& ctx,
    StmtId stmt,
    const std::vector<Subst>& subst) {
    if (stmt == kInvalidAstId) {
        return kInvalidAstId;
    }
    const AstNode& node = ctx.node(stmt);
    switch (node.kind) {
        case NodeKind::BlockStmt: {
            auto& s = static_cast<const BlockStmt&>(node);
            auto stmts = clone_stmt_list(ctx, s.stmts, subst);
            return ctx.make<BlockStmt>(std::move(stmts), s.span);
        }
        case NodeKind::AsmStmt: {
            auto& s = static_cast<const AsmStmt&>(node);
            return ctx.make<AsmStmt>(s.code, s.span);
        }
        case NodeKind::UnsafeStmt: {
            auto& s = static_cast<const UnsafeStmt&>(node);
            StmtId body = clone_stmt(ctx, s.body, subst);
            return ctx.make<UnsafeStmt>(body, s.span);
        }
        case NodeKind::LetStmt: {
            auto& s = static_cast<const LetStmt&>(node);
            ExprId init = clone_expr(ctx, s.initializer, subst);
            return ctx.make<LetStmt>(clone_ident(s.ident), s.type, init, s.span);
        }
        case NodeKind::MakeStmt: {
            auto& s = static_cast<const MakeStmt&>(node);
            ExprId val = clone_expr(ctx, s.value, subst);
            return ctx.make<MakeStmt>(clone_ident(s.ident), s.type, val, s.span);
        }
        case NodeKind::SetStmt: {
            auto& s = static_cast<const SetStmt&>(node);
            ExprId val = clone_expr(ctx, s.value, subst);
            return ctx.make<SetStmt>(clone_ident(s.ident), val, s.span);
        }
        case NodeKind::GiveStmt: {
            auto& s = static_cast<const GiveStmt&>(node);
            ExprId val = clone_expr(ctx, s.value, subst);
            return ctx.make<GiveStmt>(val, s.span);
        }
        case NodeKind::EmitStmt: {
            auto& s = static_cast<const EmitStmt&>(node);
            ExprId val = clone_expr(ctx, s.value, subst);
            return ctx.make<EmitStmt>(val, s.span);
        }
        case NodeKind::ExprStmt: {
            auto& s = static_cast<const ExprStmt&>(node);
            ExprId expr = clone_expr(ctx, s.expr, subst);
            return ctx.make<ExprStmt>(expr, s.span);
        }
        case NodeKind::ReturnStmt: {
            auto& s = static_cast<const ReturnStmt&>(node);
            ExprId expr = clone_expr(ctx, s.expr, subst);
            return ctx.make<ReturnStmt>(expr, s.span);
        }
        case NodeKind::IfStmt: {
            auto& s = static_cast<const IfStmt&>(node);
            ExprId cond = clone_expr(ctx, s.cond, subst);
            StmtId then_block = clone_stmt(ctx, s.then_block, subst);
            StmtId else_block = s.else_block != kInvalidAstId
                ? clone_stmt(ctx, s.else_block, subst)
                : kInvalidAstId;
            return ctx.make<IfStmt>(cond, then_block, else_block, s.span);
        }
        case NodeKind::LoopStmt: {
            auto& s = static_cast<const LoopStmt&>(node);
            StmtId body = clone_stmt(ctx, s.body, subst);
            return ctx.make<LoopStmt>(body, s.span);
        }
        case NodeKind::BreakStmt: {
            auto& s = static_cast<const BreakStmt&>(node);
            return ctx.make<BreakStmt>(s.span);
        }
        case NodeKind::ContinueStmt: {
            auto& s = static_cast<const ContinueStmt&>(node);
            return ctx.make<ContinueStmt>(s.span);
        }
        case NodeKind::ForStmt: {
            auto& s = static_cast<const ForStmt&>(node);
            ExprId it = clone_expr(ctx, s.iterable, subst);
            StmtId body = clone_stmt(ctx, s.body, subst);
            return ctx.make<ForStmt>(clone_ident(s.ident), it, body, s.span);
        }
        case NodeKind::WhenStmt: {
            auto& s = static_cast<const WhenStmt&>(node);
            StmtId block = clone_stmt(ctx, s.block, subst);
            return ctx.make<WhenStmt>(s.pattern, block, s.span);
        }
        case NodeKind::SelectStmt: {
            auto& s = static_cast<const SelectStmt&>(node);
            ExprId expr = clone_expr(ctx, s.expr, subst);
            std::vector<StmtId> whens;
            whens.reserve(s.whens.size());
            for (auto w_id : s.whens) {
                if (w_id == kInvalidAstId) {
                    continue;
                }
                auto& w = static_cast<const WhenStmt&>(ctx.node(w_id));
                StmtId block = clone_stmt(ctx, w.block, subst);
                whens.push_back(ctx.make<WhenStmt>(w.pattern, block, w.span));
            }
            StmtId otherwise_block = s.otherwise_block != kInvalidAstId
                ? clone_stmt(ctx, s.otherwise_block, subst)
                : kInvalidAstId;
            return ctx.make<SelectStmt>(expr, std::move(whens), otherwise_block, s.span);
        }
        default:
            return stmt;
    }
}

static bool extract_macro_call(
    const AstContext& ctx,
    ExprId expr,
    std::string& name,
    std::vector<ExprId>& args) {
    if (expr == kInvalidAstId) {
        return false;
    }

    const AstNode& node = ctx.node(expr);
    if (node.kind == NodeKind::InvokeExpr) {
        auto& e = static_cast<const InvokeExpr&>(node);
        if (e.callee_expr == kInvalidAstId) {
            return false;
        }
        const AstNode& callee = ctx.node(e.callee_expr);
        if (callee.kind != NodeKind::IdentExpr) {
            return false;
        }
        auto& id = static_cast<const IdentExpr&>(callee);
        name = id.ident.name;
        args = e.args;
        return true;
    }
    if (node.kind == NodeKind::CallNoParenExpr) {
        auto& e = static_cast<const CallNoParenExpr&>(node);
        name = e.callee.name;
        args.clear();
        args.push_back(e.arg);
        return true;
    }
    if (node.kind == NodeKind::IdentExpr) {
        auto& e = static_cast<const IdentExpr&>(node);
        name = e.ident.name;
        args.clear();
        return true;
    }
    return false;
}

static std::vector<StmtId> inline_macro(
    AstContext& ctx,
    const MacroDecl& mac,
    const std::vector<ExprId>& args,
    diag::DiagnosticEngine& diagnostics) {
    std::vector<StmtId> out;
    if (mac.body == kInvalidAstId) {
        return out;
    }
    if (mac.body == kInvalidAstId) {
        return out;
    }
    const AstNode& body_node = ctx.node(mac.body);
    if (body_node.kind != NodeKind::BlockStmt) {
        diagnostics.error("macro body must be a block", mac.span);
        return out;
    }

    std::vector<Subst> subst;
    if (mac.params.size() != args.size()) {
        diagnostics.error("macro argument count mismatch", mac.span);
        return out;
    }
    subst.reserve(mac.params.size());
    for (std::size_t i = 0; i < mac.params.size(); ++i) {
        subst.push_back(Subst{mac.params[i].name, args[i]});
    }

    auto& block = static_cast<const BlockStmt&>(body_node);
    for (auto s : block.stmts) {
        out.push_back(clone_stmt(ctx, s, subst));
    }
    return out;
}

static StmtId expand_stmt(
    AstContext& ctx,
    StmtId stmt,
    const std::unordered_map<std::string, MacroDef>& macros,
    diag::DiagnosticEngine& diagnostics);

static StmtId expand_block(
    AstContext& ctx,
    const BlockStmt& block,
    const std::unordered_map<std::string, MacroDef>& macros,
    diag::DiagnosticEngine& diagnostics) {
    std::vector<StmtId> out;
    for (auto s_id : block.stmts) {
        if (s_id == kInvalidAstId) {
            continue;
        }
        const AstNode& s_node = ctx.node(s_id);
        if (s_node.kind == NodeKind::ExprStmt) {
            auto& es = static_cast<const ExprStmt&>(s_node);
            std::string name;
            std::vector<ExprId> args;
            if (extract_macro_call(ctx, es.expr, name, args)) {
                auto it = macros.find(name);
                if (it != macros.end() && it->second.decl) {
                    auto expanded = inline_macro(ctx, *it->second.decl, args, diagnostics);
                    for (auto ex : expanded) {
                        out.push_back(expand_stmt(ctx, ex, macros, diagnostics));
                    }
                    continue;
                }
            }
        }
        out.push_back(expand_stmt(ctx, s_id, macros, diagnostics));
    }
    return ctx.make<BlockStmt>(std::move(out), block.span);
}

static StmtId expand_stmt(
    AstContext& ctx,
    StmtId stmt,
    const std::unordered_map<std::string, MacroDef>& macros,
    diag::DiagnosticEngine& diagnostics) {
    if (stmt == kInvalidAstId) {
        return kInvalidAstId;
    }
    const AstNode& node = ctx.node(stmt);
    switch (node.kind) {
        case NodeKind::BlockStmt: {
            auto& b = static_cast<const BlockStmt&>(node);
            return expand_block(ctx, b, macros, diagnostics);
        }
        case NodeKind::UnsafeStmt: {
            auto& s = static_cast<const UnsafeStmt&>(node);
            StmtId body = expand_stmt(ctx, s.body, macros, diagnostics);
            return ctx.make<UnsafeStmt>(body, s.span);
        }
        case NodeKind::IfStmt: {
            auto& s = static_cast<const IfStmt&>(node);
            StmtId then_block = expand_stmt(ctx, s.then_block, macros, diagnostics);
            StmtId else_block = s.else_block != kInvalidAstId
                ? expand_stmt(ctx, s.else_block, macros, diagnostics)
                : kInvalidAstId;
            return ctx.make<IfStmt>(s.cond, then_block, else_block, s.span);
        }
        case NodeKind::LoopStmt: {
            auto& s = static_cast<const LoopStmt&>(node);
            StmtId body = expand_stmt(ctx, s.body, macros, diagnostics);
            return ctx.make<LoopStmt>(body, s.span);
        }
        case NodeKind::ForStmt: {
            auto& s = static_cast<const ForStmt&>(node);
            StmtId body = expand_stmt(ctx, s.body, macros, diagnostics);
            return ctx.make<ForStmt>(clone_ident(s.ident), s.iterable, body, s.span);
        }
        case NodeKind::SelectStmt: {
            auto& s = static_cast<const SelectStmt&>(node);
            std::vector<StmtId> whens;
            whens.reserve(s.whens.size());
            for (auto w_id : s.whens) {
                if (w_id == kInvalidAstId) {
                    continue;
                }
                auto& w = static_cast<const WhenStmt&>(ctx.node(w_id));
                StmtId block = expand_stmt(ctx, w.block, macros, diagnostics);
                whens.push_back(ctx.make<WhenStmt>(w.pattern, block, w.span));
            }
            StmtId otherwise_block = s.otherwise_block != kInvalidAstId
                ? expand_stmt(ctx, s.otherwise_block, macros, diagnostics)
                : kInvalidAstId;
            return ctx.make<SelectStmt>(s.expr, std::move(whens), otherwise_block, s.span);
        }
        default:
            return stmt;
    }
}

} // namespace

void expand_macros(ast::AstContext& ctx, ast::ModuleId module, diag::DiagnosticEngine& diagnostics) {
    if (module == kInvalidAstId) {
        return;
    }
    auto& mod = ctx.get<Module>(module);
    std::unordered_map<std::string, MacroDef> macros;
    for (auto decl_id : mod.decls) {
        if (decl_id == kInvalidAstId) {
            continue;
        }
        auto& decl = ctx.get<Decl>(decl_id);
        if (decl.kind == NodeKind::MacroDecl) {
            auto& m = static_cast<MacroDecl&>(decl);
            macros[m.name.name] = MacroDef{&m};
        }
    }

    for (auto decl_id : mod.decls) {
        if (decl_id == kInvalidAstId) {
            continue;
        }
        auto& decl = ctx.get<Decl>(decl_id);
        switch (decl.kind) {
            case NodeKind::ProcDecl: {
                auto& d = static_cast<ProcDecl&>(decl);
                if (d.body != kInvalidAstId) {
                    d.body = expand_stmt(ctx, d.body, macros, diagnostics);
                }
                break;
            }
            case NodeKind::EntryDecl: {
                auto& d = static_cast<EntryDecl&>(decl);
                if (d.body != kInvalidAstId) {
                    d.body = expand_stmt(ctx, d.body, macros, diagnostics);
                }
                break;
            }
            case NodeKind::MacroDecl: {
                auto& d = static_cast<MacroDecl&>(decl);
                if (d.body != kInvalidAstId) {
                    d.body = expand_stmt(ctx, d.body, macros, diagnostics);
                }
                break;
            }
            default:
                break;
        }
    }
}

} // namespace vitte::frontend::passes
