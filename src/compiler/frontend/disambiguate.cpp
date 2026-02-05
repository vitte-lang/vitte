#include "disambiguate.hpp"

namespace vitte::frontend::passes {

using namespace vitte::frontend::ast;

static void disambiguate_expr(AstContext& ctx, ExprId expr_id);
static void disambiguate_stmt(AstContext& ctx, StmtId stmt_id);

static void disambiguate_expr(AstContext& ctx, ExprId expr_id) {
    if (expr_id == kInvalidAstId) {
        return;
    }

    AstNode& node = ctx.node(expr_id);
    switch (node.kind) {
        case NodeKind::UnaryExpr: {
            auto& u = static_cast<UnaryExpr&>(node);
            disambiguate_expr(ctx, u.expr);
            break;
        }
        case NodeKind::BinaryExpr: {
            auto& b = static_cast<BinaryExpr&>(node);
            disambiguate_expr(ctx, b.lhs);
            disambiguate_expr(ctx, b.rhs);
            break;
        }
        case NodeKind::ProcExpr: {
            auto& p = static_cast<ProcExpr&>(node);
            disambiguate_stmt(ctx, p.body);
            break;
        }
        case NodeKind::InvokeExpr: {
            auto& inv = static_cast<InvokeExpr&>(node);
            if (inv.callee_type != kInvalidAstId) {
                inv.invoke_kind = InvokeExpr::Kind::Ctor;
            } else if (inv.callee_expr != kInvalidAstId) {
                inv.invoke_kind = InvokeExpr::Kind::Call;
            }
            if (inv.callee_expr != kInvalidAstId) {
                disambiguate_expr(ctx, inv.callee_expr);
            }
            for (auto arg : inv.args) {
                disambiguate_expr(ctx, arg);
            }
            break;
        }
        case NodeKind::MemberExpr: {
            auto& m = static_cast<MemberExpr&>(node);
            disambiguate_expr(ctx, m.base);
            break;
        }
        case NodeKind::IndexExpr: {
            auto& i = static_cast<IndexExpr&>(node);
            disambiguate_expr(ctx, i.base);
            disambiguate_expr(ctx, i.index);
            break;
        }
        case NodeKind::IfExpr: {
            auto& e = static_cast<IfExpr&>(node);
            disambiguate_expr(ctx, e.cond);
            disambiguate_stmt(ctx, e.then_block);
            if (e.else_block != kInvalidAstId) {
                disambiguate_stmt(ctx, e.else_block);
            }
            break;
        }
        case NodeKind::IsExpr: {
            auto& e = static_cast<IsExpr&>(node);
            disambiguate_expr(ctx, e.value);
            break;
        }
        case NodeKind::AsExpr: {
            auto& e = static_cast<AsExpr&>(node);
            disambiguate_expr(ctx, e.value);
            break;
        }
        case NodeKind::ListExpr: {
            auto& list = static_cast<ListExpr&>(node);
            for (auto item : list.items) {
                disambiguate_expr(ctx, item);
            }
            break;
        }
        case NodeKind::CallNoParenExpr: {
            auto& cnp = static_cast<CallNoParenExpr&>(node);
            disambiguate_expr(ctx, cnp.arg);
            break;
        }
        default:
            break;
    }
}

static void disambiguate_stmt(AstContext& ctx, StmtId stmt_id) {
    if (stmt_id == kInvalidAstId) {
        return;
    }

    AstNode& node = ctx.node(stmt_id);
    switch (node.kind) {
        case NodeKind::BlockStmt: {
            auto& b = static_cast<BlockStmt&>(node);
            for (auto s : b.stmts) {
                disambiguate_stmt(ctx, s);
            }
            break;
        }
        case NodeKind::LetStmt: {
            auto& s = static_cast<LetStmt&>(node);
            disambiguate_expr(ctx, s.initializer);
            break;
        }
        case NodeKind::MakeStmt: {
            auto& s = static_cast<MakeStmt&>(node);
            disambiguate_expr(ctx, s.value);
            break;
        }
        case NodeKind::SetStmt: {
            auto& s = static_cast<SetStmt&>(node);
            disambiguate_expr(ctx, s.value);
            break;
        }
        case NodeKind::GiveStmt: {
            auto& s = static_cast<GiveStmt&>(node);
            disambiguate_expr(ctx, s.value);
            break;
        }
        case NodeKind::EmitStmt: {
            auto& s = static_cast<EmitStmt&>(node);
            disambiguate_expr(ctx, s.value);
            break;
        }
        case NodeKind::ExprStmt: {
            auto& s = static_cast<ExprStmt&>(node);
            disambiguate_expr(ctx, s.expr);
            break;
        }
        case NodeKind::ReturnStmt: {
            auto& s = static_cast<ReturnStmt&>(node);
            disambiguate_expr(ctx, s.expr);
            break;
        }
        case NodeKind::IfStmt: {
            auto& s = static_cast<IfStmt&>(node);
            disambiguate_expr(ctx, s.cond);
            disambiguate_stmt(ctx, s.then_block);
            if (s.else_block != kInvalidAstId) {
                disambiguate_stmt(ctx, s.else_block);
            }
            break;
        }
        case NodeKind::LoopStmt: {
            auto& s = static_cast<LoopStmt&>(node);
            disambiguate_stmt(ctx, s.body);
            break;
        }
        case NodeKind::ForStmt: {
            auto& s = static_cast<ForStmt&>(node);
            disambiguate_expr(ctx, s.iterable);
            disambiguate_stmt(ctx, s.body);
            break;
        }
        case NodeKind::SelectStmt: {
            auto& s = static_cast<SelectStmt&>(node);
            disambiguate_expr(ctx, s.expr);
            for (auto w_id : s.whens) {
                if (w_id == kInvalidAstId) {
                    continue;
                }
                auto& w = static_cast<WhenStmt&>(ctx.node(w_id));
                disambiguate_stmt(ctx, w.block);
            }
            if (s.otherwise_block != kInvalidAstId) {
                disambiguate_stmt(ctx, s.otherwise_block);
            }
            break;
        }
        default:
            break;
    }
}

void disambiguate_invokes(AstContext& ctx, ModuleId module_id) {
    if (module_id == kInvalidAstId) {
        return;
    }
    auto& module = ctx.get<Module>(module_id);
    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidAstId) {
            continue;
        }
        auto& decl = ctx.get<Decl>(decl_id);
        switch (decl.kind) {
            case NodeKind::ProcDecl: {
                auto& d = static_cast<ProcDecl&>(decl);
                if (d.body != kInvalidAstId) {
                    disambiguate_stmt(ctx, d.body);
                }
                break;
            }
            case NodeKind::EntryDecl: {
                auto& d = static_cast<EntryDecl&>(decl);
                if (d.body != kInvalidAstId) {
                    disambiguate_stmt(ctx, d.body);
                }
                break;
            }
            case NodeKind::FnDecl: {
                auto& d = static_cast<FnDecl&>(decl);
                disambiguate_stmt(ctx, d.body);
                break;
            }
            case NodeKind::MacroDecl: {
                auto& d = static_cast<MacroDecl&>(decl);
                disambiguate_stmt(ctx, d.body);
                break;
            }
            default:
                break;
        }
    }
}

} // namespace vitte::frontend::passes
