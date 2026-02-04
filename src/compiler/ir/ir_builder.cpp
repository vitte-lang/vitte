// ============================================================
// ir_builder.cpp — Vitte Compiler
// AST → HIR builder / lowering entry
// ============================================================

#include "ir_builder.hpp"

#include <cassert>

namespace vitte::ir {

using namespace vitte::frontend::ast;

// ------------------------------------------------------------
// IRBuilder
// ------------------------------------------------------------

IrBuilder::IrBuilder() = default;

// ------------------------------------------------------------
// Entry
// ------------------------------------------------------------

HirModule IrBuilder::build_module(const Module& ast_module) {
    std::vector<HirDeclPtr> decls;

    for (const auto& d : ast_module.decls) {
        if (auto hd = build_decl(*d)) {
            decls.push_back(std::move(hd));
        }
    }

    return HirModule(
        ast_module.name,
        std::move(decls),
        ast_module.span);
}

// ------------------------------------------------------------
// Declarations
// ------------------------------------------------------------

HirDeclPtr IrBuilder::build_decl(const Decl& decl) {
    switch (decl.kind) {
        case NodeKind::FnDecl:
            return build_fn(static_cast<const FnDecl&>(decl));
        default:
            return nullptr;
    }
}

HirDeclPtr IrBuilder::build_fn(const FnDecl& fn) {
    std::vector<HirParam> params;
    for (const auto& p : fn.params) {
        params.emplace_back(
            p.ident.name,
            build_type(p.type.get()));
    }

    HirTypePtr ret_type = nullptr;
    if (fn.return_type) {
        ret_type = build_type(fn.return_type.get());
    }

    HirBlock body = build_block(fn.body);

    return std::make_unique<HirFnDecl>(
        fn.name.name,
        std::move(params),
        std::move(ret_type),
        std::move(body),
        fn.span);
}

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

HirTypePtr IrBuilder::build_type(const TypeNode* ty) {
    if (!ty) {
        return nullptr;
    }

    switch (ty->kind) {
        case NodeKind::NamedType: {
            auto& nt = static_cast<const NamedType&>(*ty);
            return std::make_unique<HirNamedType>(
                nt.ident.name,
                nt.span);
        }
        case NodeKind::GenericType: {
            auto& gt = static_cast<const GenericType&>(*ty);
            std::vector<HirTypePtr> args;
            for (const auto& a : gt.type_args) {
                args.push_back(build_type(a.get()));
            }
            return std::make_unique<HirGenericType>(
                gt.base_ident.name,
                std::move(args),
                gt.span);
        }
        default:
            return nullptr;
    }
}

// ------------------------------------------------------------
// Blocks / statements
// ------------------------------------------------------------

HirBlock IrBuilder::build_block(const BlockStmt& block) {
    std::vector<HirStmtPtr> stmts;
    for (const auto& s : block.stmts) {
        if (auto hs = build_stmt(*s)) {
            stmts.push_back(std::move(hs));
        }
    }
    return HirBlock(std::move(stmts), block.span);
}

HirStmtPtr IrBuilder::build_stmt(const Stmt& stmt) {
    switch (stmt.kind) {
        case NodeKind::LetStmt:
            return build_let(static_cast<const LetStmt&>(stmt));
        case NodeKind::ReturnStmt:
            return build_return(static_cast<const ReturnStmt&>(stmt));
        case NodeKind::ExprStmt:
            return build_expr_stmt(static_cast<const ExprStmt&>(stmt));
        default:
            return nullptr;
    }
}

HirStmtPtr IrBuilder::build_let(const LetStmt& let) {
    return std::make_unique<HirLetStmt>(
        let.ident.name,
        build_type(let.type.get()),
        build_expr(let.initializer.get()),
        let.span);
}

HirStmtPtr IrBuilder::build_return(const ReturnStmt& ret) {
    return std::make_unique<HirReturnStmt>(
        build_expr(ret.expr.get()),
        ret.span);
}

HirStmtPtr IrBuilder::build_expr_stmt(const ExprStmt& stmt) {
    return std::make_unique<HirExprStmt>(
        build_expr(stmt.expr.get()),
        stmt.span);
}

// ------------------------------------------------------------
// Expressions
// ------------------------------------------------------------

HirExprPtr IrBuilder::build_expr(const Expr* expr) {
    if (!expr) {
        return nullptr;
    }

    switch (expr->kind) {
        case NodeKind::LiteralExpr:
            return build_literal(static_cast<const LiteralExpr&>(*expr));
        case NodeKind::IdentExpr:
            return build_ident(static_cast<const IdentExpr&>(*expr));
        case NodeKind::UnaryExpr:
            return build_unary(static_cast<const UnaryExpr&>(*expr));
        case NodeKind::BinaryExpr:
            return build_binary(static_cast<const BinaryExpr&>(*expr));
        case NodeKind::CallExpr:
            return build_call(static_cast<const CallExpr&>(*expr));
        default:
            return nullptr;
    }
}

HirExprPtr IrBuilder::build_literal(const LiteralExpr& lit) {
    HirLiteralKind kind;
    switch (lit.lit_kind) {
        case LiteralKind::Bool:   kind = HirLiteralKind::Bool;   break;
        case LiteralKind::Int:    kind = HirLiteralKind::Int;    break;
        case LiteralKind::String: kind = HirLiteralKind::String; break;
        default:
            kind = HirLiteralKind::Int;
            break;
    }

    return std::make_unique<HirLiteralExpr>(
        kind,
        lit.value,
        lit.span);
}

HirExprPtr IrBuilder::build_ident(const IdentExpr& id) {
    return std::make_unique<HirVarExpr>(
        id.ident.name,
        id.span);
}

HirExprPtr IrBuilder::build_unary(const UnaryExpr& un) {
    HirUnaryOp op =
        un.op == UnaryOp::Not ? HirUnaryOp::Not : HirUnaryOp::Neg;

    return std::make_unique<HirUnaryExpr>(
        op,
        build_expr(un.expr.get()),
        un.span);
}

HirExprPtr IrBuilder::build_binary(const BinaryExpr& bin) {
    HirBinaryOp op;
    switch (bin.op) {
        case BinaryOp::Add: op = HirBinaryOp::Add; break;
        case BinaryOp::Sub: op = HirBinaryOp::Sub; break;
        case BinaryOp::Mul: op = HirBinaryOp::Mul; break;
        case BinaryOp::Div: op = HirBinaryOp::Div; break;
        case BinaryOp::Eq:  op = HirBinaryOp::Eq;  break;
        case BinaryOp::Ne:  op = HirBinaryOp::Ne;  break;
        case BinaryOp::Lt:  op = HirBinaryOp::Lt;  break;
        case BinaryOp::Le:  op = HirBinaryOp::Le;  break;
        case BinaryOp::Gt:  op = HirBinaryOp::Gt;  break;
        case BinaryOp::Ge:  op = HirBinaryOp::Ge;  break;
        case BinaryOp::And: op = HirBinaryOp::And; break;
        case BinaryOp::Or:  op = HirBinaryOp::Or;  break;
        default:
            op = HirBinaryOp::Add;
            break;
    }

    return std::make_unique<HirBinaryExpr>(
        op,
        build_expr(bin.lhs.get()),
        build_expr(bin.rhs.get()),
        bin.span);
}

HirExprPtr IrBuilder::build_call(const CallExpr& call) {
    std::vector<HirExprPtr> args;
    for (const auto& a : call.args) {
        args.push_back(build_expr(a.get()));
    }

    return std::make_unique<HirCallExpr>(
        build_expr(call.callee.get()),
        std::move(args),
        call.span);
}

} // namespace vitte::ir