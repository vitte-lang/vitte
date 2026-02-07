#include "lower_hir.hpp"
#include "diagnostics_messages.hpp"

namespace vitte::frontend::lower {

using namespace vitte::frontend::ast;

static ir::HirTypeId lower_type(
    const AstContext& ctx,
    TypeId type,
    ir::HirContext& hir_ctx);

static ir::HirExprId lower_expr(
    const AstContext& ctx,
    ExprId expr,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics);

static ir::HirStmtId lower_block(
    const AstContext& ctx,
    StmtId block_id,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics);

static ir::HirPatternId lower_pattern(
    const AstContext& ctx,
    PatternId pattern,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics);

static ir::HirStmtId lower_stmt(
    const AstContext& ctx,
    StmtId stmt,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics);

static ir::HirTypeId lower_type(
    const AstContext& ctx,
    TypeId type,
    ir::HirContext& hir_ctx)
{
    if (type == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    const AstNode& node = ctx.node(type);
    switch (node.kind) {
        case NodeKind::BuiltinType: {
            auto& t = static_cast<const BuiltinType&>(node);
            return hir_ctx.make<ir::HirNamedType>(t.name, t.span);
        }
        case NodeKind::NamedType: {
            auto& t = static_cast<const NamedType&>(node);
            return hir_ctx.make<ir::HirNamedType>(t.ident.name, t.span);
        }
        case NodeKind::GenericType: {
            auto& t = static_cast<const GenericType&>(node);
            std::vector<ir::HirTypeId> args;
            for (auto a : t.type_args) {
                args.push_back(lower_type(ctx, a, hir_ctx));
            }
            return hir_ctx.make<ir::HirGenericType>(
                t.base_ident.name,
                std::move(args),
                t.span);
        }
        default:
            return ir::kInvalidHirId;
    }
}

static ir::HirExprId lower_invoke(
    const AstContext& ctx,
    const InvokeExpr& inv,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics)
{
    ir::HirExprId callee = ir::kInvalidHirId;
    if (inv.callee_expr != kInvalidAstId) {
        callee = lower_expr(ctx, inv.callee_expr, hir_ctx, diagnostics);
    } else if (inv.callee_type != kInvalidAstId) {
        const AstNode& tnode = ctx.node(inv.callee_type);
        if (tnode.kind == NodeKind::BuiltinType) {
            auto& t = static_cast<const BuiltinType&>(tnode);
            callee = hir_ctx.make<ir::HirVarExpr>(t.name, t.span);
        } else if (tnode.kind == NodeKind::NamedType) {
            auto& t = static_cast<const NamedType&>(tnode);
            callee = hir_ctx.make<ir::HirVarExpr>(t.ident.name, t.ident.span);
        } else if (tnode.kind == NodeKind::GenericType) {
            auto& t = static_cast<const GenericType&>(tnode);
            callee = hir_ctx.make<ir::HirVarExpr>(t.base_ident.name, t.base_ident.span);
        }
    }

    std::vector<ir::HirExprId> args;
    for (auto a : inv.args) {
        args.push_back(lower_expr(ctx, a, hir_ctx, diagnostics));
    }

    if (callee == ir::kInvalidHirId) {
        diag::error(diagnostics, diag::DiagId::InvokeHasNoCallee, inv.span);
        callee = hir_ctx.make<ir::HirVarExpr>("<error>", inv.span);
    }

    return hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), inv.span);
}

static ir::HirExprId lower_expr(
    const AstContext& ctx,
    ExprId expr,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics)
{
    if (expr == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    const AstNode& node = ctx.node(expr);
    switch (node.kind) {
        case NodeKind::LiteralExpr: {
            auto& e = static_cast<const LiteralExpr&>(node);
            ir::HirLiteralKind kind = ir::HirLiteralKind::Int;
            switch (e.lit_kind) {
                case LiteralKind::Bool: kind = ir::HirLiteralKind::Bool; break;
                case LiteralKind::Int: kind = ir::HirLiteralKind::Int; break;
                case LiteralKind::String: kind = ir::HirLiteralKind::String; break;
                case LiteralKind::Float: kind = ir::HirLiteralKind::Int; break;
                case LiteralKind::Char: kind = ir::HirLiteralKind::Int; break;
            }
            return hir_ctx.make<ir::HirLiteralExpr>(kind, e.value, e.span);
        }
        case NodeKind::IdentExpr: {
            auto& e = static_cast<const IdentExpr&>(node);
            return hir_ctx.make<ir::HirVarExpr>(e.ident.name, e.span);
        }
        case NodeKind::UnaryExpr: {
            auto& e = static_cast<const UnaryExpr&>(node);
            auto rhs = lower_expr(ctx, e.expr, hir_ctx, diagnostics);
            return hir_ctx.make<ir::HirUnaryExpr>(
                ir::HirUnaryOp::Not,
                rhs,
                e.span);
        }
        case NodeKind::BinaryExpr: {
            auto& e = static_cast<const BinaryExpr&>(node);
            ir::HirBinaryOp op = ir::HirBinaryOp::Add;
            switch (e.op) {
                case BinaryOp::Add: op = ir::HirBinaryOp::Add; break;
                case BinaryOp::Sub: op = ir::HirBinaryOp::Sub; break;
                case BinaryOp::Mul: op = ir::HirBinaryOp::Mul; break;
                case BinaryOp::Div: op = ir::HirBinaryOp::Div; break;
                case BinaryOp::Eq: op = ir::HirBinaryOp::Eq; break;
                case BinaryOp::Ne: op = ir::HirBinaryOp::Ne; break;
                case BinaryOp::Lt: op = ir::HirBinaryOp::Lt; break;
                case BinaryOp::Le: op = ir::HirBinaryOp::Le; break;
                case BinaryOp::Gt: op = ir::HirBinaryOp::Gt; break;
                case BinaryOp::Ge: op = ir::HirBinaryOp::Ge; break;
                case BinaryOp::And: op = ir::HirBinaryOp::And; break;
                case BinaryOp::Or: op = ir::HirBinaryOp::Or; break;
                default: break;
            }
            return hir_ctx.make<ir::HirBinaryExpr>(
                op,
                lower_expr(ctx, e.lhs, hir_ctx, diagnostics),
                lower_expr(ctx, e.rhs, hir_ctx, diagnostics),
                e.span);
        }
        case NodeKind::InvokeExpr:
            return lower_invoke(
                ctx,
                static_cast<const InvokeExpr&>(node),
                hir_ctx,
                diagnostics);
        case NodeKind::CallNoParenExpr: {
            auto& e = static_cast<const CallNoParenExpr&>(node);
            std::vector<ir::HirExprId> args;
            args.push_back(lower_expr(ctx, e.arg, hir_ctx, diagnostics));
            auto callee = hir_ctx.make<ir::HirVarExpr>(e.callee.name, e.callee.span);
            return hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), e.span);
        }
        case NodeKind::ListExpr: {
            auto& e = static_cast<const ListExpr&>(node);
            std::vector<ir::HirExprId> args;
            for (auto item : e.items) {
                args.push_back(lower_expr(ctx, item, hir_ctx, diagnostics));
            }
            auto callee = hir_ctx.make<ir::HirVarExpr>("list", e.span);
            return hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), e.span);
        }
        default:
            diag::error(diagnostics, diag::DiagId::UnsupportedExpressionInHir, node.span);
            return ir::kInvalidHirId;
    }
}

static std::string pattern_type_name(const AstContext& ctx, TypeId type) {
    if (type == kInvalidAstId) {
        return "<unknown>";
    }
    const AstNode& node = ctx.node(type);
    switch (node.kind) {
        case NodeKind::BuiltinType: {
            auto& t = static_cast<const BuiltinType&>(node);
            return t.name;
        }
        case NodeKind::NamedType: {
            auto& t = static_cast<const NamedType&>(node);
            return t.ident.name;
        }
        case NodeKind::GenericType: {
            auto& t = static_cast<const GenericType&>(node);
            return t.base_ident.name;
        }
        default:
            return "<unknown>";
    }
}

static ir::HirPatternId lower_pattern(
    const AstContext& ctx,
    PatternId pattern,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics)
{
    if (pattern == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    const AstNode& node = ctx.node(pattern);
    switch (node.kind) {
        case NodeKind::IdentPattern: {
            auto& p = static_cast<const IdentPattern&>(node);
            return hir_ctx.make<ir::HirIdentPattern>(p.ident.name, p.span);
        }
        case NodeKind::CtorPattern: {
            auto& p = static_cast<const CtorPattern&>(node);
            std::vector<ir::HirPatternId> args;
            for (auto a : p.args) {
                args.push_back(lower_pattern(ctx, a, hir_ctx, diagnostics));
            }
            return hir_ctx.make<ir::HirCtorPattern>(
                pattern_type_name(ctx, p.type),
                std::move(args),
                p.span);
        }
        default:
            diag::error(diagnostics, diag::DiagId::UnsupportedPatternInHir, node.span);
            return ir::kInvalidHirId;
    }
}

static ir::HirStmtId lower_block(
    const AstContext& ctx,
    StmtId block_id,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics)
{
    if (block_id == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    auto& block = ctx.get<BlockStmt>(block_id);
    std::vector<ir::HirStmtId> out;
    for (auto s : block.stmts) {
        auto hs = lower_stmt(ctx, s, hir_ctx, diagnostics);
        if (hs != ir::kInvalidHirId) {
            out.push_back(hs);
        }
    }
    return hir_ctx.make<ir::HirBlock>(std::move(out), block.span);
}

static ir::HirStmtId lower_stmt(
    const AstContext& ctx,
    StmtId stmt,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics)
{
    if (stmt == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    const AstNode& node = ctx.node(stmt);
    switch (node.kind) {
        case NodeKind::BlockStmt: {
            return lower_block(ctx, stmt, hir_ctx, diagnostics);
        }
        case NodeKind::UnsafeStmt: {
            auto& s = static_cast<const UnsafeStmt&>(node);
            std::vector<ir::HirStmtId> stmts;
            auto mk_call = [&](const char* name) {
                auto callee = hir_ctx.make<ir::HirVarExpr>(name, s.span);
                std::vector<ir::HirExprId> args;
                auto call = hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), s.span);
                return hir_ctx.make<ir::HirExprStmt>(call, s.span);
            };
            stmts.push_back(mk_call("unsafe_begin"));
            auto inner_block = lower_block(ctx, s.body, hir_ctx, diagnostics);
            if (inner_block != ir::kInvalidHirId) {
                if (hir_ctx.node(inner_block).kind == ir::HirKind::Block) {
                    auto& b = hir_ctx.get<ir::HirBlock>(inner_block);
                    for (auto st : b.stmts) {
                        stmts.push_back(st);
                    }
                } else {
                    stmts.push_back(inner_block);
                }
            }
            stmts.push_back(mk_call("unsafe_end"));
            return hir_ctx.make<ir::HirBlock>(std::move(stmts), s.span);
        }
        case NodeKind::AsmStmt: {
            auto& s = static_cast<const AsmStmt&>(node);
            auto lit = hir_ctx.make<ir::HirLiteralExpr>(
                ir::HirLiteralKind::String,
                s.code,
                s.span);
            auto callee = hir_ctx.make<ir::HirVarExpr>("asm", s.span);
            std::vector<ir::HirExprId> args;
            args.push_back(lit);
            auto call = hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), s.span);
            return hir_ctx.make<ir::HirExprStmt>(call, s.span);
        }
        case NodeKind::LetStmt: {
            auto& s = static_cast<const LetStmt&>(node);
            return hir_ctx.make<ir::HirLetStmt>(
                s.ident.name,
                lower_type(ctx, s.type, hir_ctx),
                lower_expr(ctx, s.initializer, hir_ctx, diagnostics),
                s.span);
        }
        case NodeKind::MakeStmt: {
            auto& s = static_cast<const MakeStmt&>(node);
            return hir_ctx.make<ir::HirLetStmt>(
                s.ident.name,
                lower_type(ctx, s.type, hir_ctx),
                lower_expr(ctx, s.value, hir_ctx, diagnostics),
                s.span);
        }
        case NodeKind::SetStmt: {
            auto& s = static_cast<const SetStmt&>(node);
            std::vector<ir::HirExprId> args;
            args.push_back(hir_ctx.make<ir::HirVarExpr>(s.ident.name, s.ident.span));
            args.push_back(lower_expr(ctx, s.value, hir_ctx, diagnostics));
            auto callee = hir_ctx.make<ir::HirVarExpr>("set", s.span);
            auto call = hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), s.span);
            return hir_ctx.make<ir::HirExprStmt>(call, s.span);
        }
        case NodeKind::GiveStmt: {
            auto& s = static_cast<const GiveStmt&>(node);
            return hir_ctx.make<ir::HirReturnStmt>(
                lower_expr(ctx, s.value, hir_ctx, diagnostics),
                s.span);
        }
        case NodeKind::EmitStmt: {
            auto& s = static_cast<const EmitStmt&>(node);
            std::vector<ir::HirExprId> args;
            args.push_back(lower_expr(ctx, s.value, hir_ctx, diagnostics));
            auto callee = hir_ctx.make<ir::HirVarExpr>("emit", s.span);
            auto call = hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), s.span);
            return hir_ctx.make<ir::HirExprStmt>(call, s.span);
        }
        case NodeKind::ExprStmt: {
            auto& s = static_cast<const ExprStmt&>(node);
            return hir_ctx.make<ir::HirExprStmt>(
                lower_expr(ctx, s.expr, hir_ctx, diagnostics),
                s.span);
        }
        case NodeKind::ReturnStmt: {
            auto& s = static_cast<const ReturnStmt&>(node);
            return hir_ctx.make<ir::HirReturnStmt>(
                lower_expr(ctx, s.expr, hir_ctx, diagnostics),
                s.span);
        }
        case NodeKind::IfStmt: {
            auto& s = static_cast<const IfStmt&>(node);
            return hir_ctx.make<ir::HirIf>(
                lower_expr(ctx, s.cond, hir_ctx, diagnostics),
                lower_block(ctx, s.then_block, hir_ctx, diagnostics),
                s.else_block != kInvalidAstId
                    ? lower_block(ctx, s.else_block, hir_ctx, diagnostics)
                    : ir::kInvalidHirId,
                s.span);
        }
        case NodeKind::LoopStmt: {
            auto& s = static_cast<const LoopStmt&>(node);
            return hir_ctx.make<ir::HirLoop>(
                lower_block(ctx, s.body, hir_ctx, diagnostics),
                s.span);
        }
        case NodeKind::SelectStmt: {
            auto& s = static_cast<const SelectStmt&>(node);
            std::vector<ir::HirStmtId> whens;
            for (auto w_id : s.whens) {
                if (w_id == kInvalidAstId) {
                    continue;
                }
                auto& w = static_cast<const WhenStmt&>(ctx.node(w_id));
                whens.push_back(hir_ctx.make<ir::HirWhen>(
                    lower_pattern(ctx, w.pattern, hir_ctx, diagnostics),
                    lower_block(ctx, w.block, hir_ctx, diagnostics),
                    w.span));
            }
            ir::HirStmtId otherwise_block = ir::kInvalidHirId;
            if (s.otherwise_block != kInvalidAstId) {
                otherwise_block = lower_block(ctx, s.otherwise_block, hir_ctx, diagnostics);
            }
            return hir_ctx.make<ir::HirSelect>(
                lower_expr(ctx, s.expr, hir_ctx, diagnostics),
                std::move(whens),
                otherwise_block,
                s.span);
        }
        case NodeKind::WhenStmt: {
            auto& w = static_cast<const WhenStmt&>(node);
            return hir_ctx.make<ir::HirWhen>(
                lower_pattern(ctx, w.pattern, hir_ctx, diagnostics),
                lower_block(ctx, w.block, hir_ctx, diagnostics),
                w.span);
        }
        default:
            diag::error(diagnostics, diag::DiagId::UnsupportedStatementInHir, node.span);
            return ir::kInvalidHirId;
    }
}

ir::HirModuleId lower_to_hir(
    const ast::AstContext& ctx,
    ast::ModuleId module_id,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics)
{
    if (module_id == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    const auto& module = ctx.get<Module>(module_id);
    std::vector<ir::HirDeclId> decls;

    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidAstId) {
            continue;
        }
        const auto& decl = ctx.get<Decl>(decl_id);
        switch (decl.kind) {
            case NodeKind::ProcDecl: {
                auto& d = static_cast<const ProcDecl&>(decl);
                std::vector<ir::HirParam> params;
                for (const auto& p : d.params) {
                    params.emplace_back(p.ident.name, lower_type(ctx, p.type, hir_ctx));
                }
                decls.push_back(hir_ctx.make<ir::HirFnDecl>(
                    d.name.name,
                    std::move(params),
                    lower_type(ctx, d.return_type, hir_ctx),
                    d.body != kInvalidAstId
                        ? lower_block(ctx, d.body, hir_ctx, diagnostics)
                        : ir::kInvalidHirId,
                    d.span));
                break;
            }
            case NodeKind::MacroDecl: {
                auto& d = static_cast<const MacroDecl&>(decl);
                std::vector<ir::HirParam> params;
                for (const auto& p : d.params) {
                    params.emplace_back(p.name, ir::kInvalidHirId);
                }
                decls.push_back(hir_ctx.make<ir::HirFnDecl>(
                    d.name.name,
                    std::move(params),
                    ir::kInvalidHirId,
                    lower_block(ctx, d.body, hir_ctx, diagnostics),
                    d.span));
                break;
            }
            case NodeKind::ConstDecl: {
                auto& d = static_cast<const ConstDecl&>(decl);
                decls.push_back(hir_ctx.make<ir::HirConstDecl>(
                    d.name.name,
                    lower_type(ctx, d.type, hir_ctx),
                    lower_expr(ctx, d.value, hir_ctx, diagnostics),
                    d.span));
                break;
            }
            case NodeKind::GlobalDecl: {
                auto& d = static_cast<const GlobalDecl&>(decl);
                decls.push_back(hir_ctx.make<ir::HirGlobalDecl>(
                    d.name.name,
                    lower_type(ctx, d.type, hir_ctx),
                    lower_expr(ctx, d.value, hir_ctx, diagnostics),
                    d.is_mut,
                    d.span));
                break;
            }
            case NodeKind::EntryDecl: {
                auto& d = static_cast<const EntryDecl&>(decl);
                std::vector<ir::HirParam> params;
                decls.push_back(hir_ctx.make<ir::HirFnDecl>(
                    d.name.name,
                    std::move(params),
                    ir::kInvalidHirId,
                    lower_block(ctx, d.body, hir_ctx, diagnostics),
                    d.span));
                break;
            }
            default:
                break;
        }
    }

    return hir_ctx.make<ir::HirModule>(module.name, std::move(decls), module.span);
}

} // namespace vitte::frontend::lower
