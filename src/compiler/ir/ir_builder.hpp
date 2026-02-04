// ============================================================
// ir_builder.hpp — Vitte Compiler
// AST → HIR builder interface
// ============================================================

#pragma once

#include <memory>

#include "ast.hpp"
#include "hir.hpp"

namespace vitte::ir {

// ------------------------------------------------------------
// IR Builder
// ------------------------------------------------------------
//
// Rôle :
//  - transformer l'AST frontend en HIR canonique
//  - aucune logique de typage finale
//  - aucune dépendance backend
//  - point d'entrée unique pour le lowering AST → HIR
//

class IrBuilder {
public:
    IrBuilder();

    // --------------------------------------------------------
    // Entry
    // --------------------------------------------------------

    HirModule build_module(const vitte::frontend::ast::Module& module);

private:
    using namespace vitte::frontend::ast;

    // --------------------------------------------------------
    // Declarations
    // --------------------------------------------------------

    HirDeclPtr build_decl(const Decl& decl);
    HirDeclPtr build_fn(const FnDecl& fn);

    // --------------------------------------------------------
    // Types
    // --------------------------------------------------------

    HirTypePtr build_type(const TypeNode* type);

    // --------------------------------------------------------
    // Blocks / statements
    // --------------------------------------------------------

    HirBlock build_block(const BlockStmt& block);

    HirStmtPtr build_stmt(const Stmt& stmt);
    HirStmtPtr build_let(const LetStmt& let);
    HirStmtPtr build_return(const ReturnStmt& ret);
    HirStmtPtr build_expr_stmt(const ExprStmt& stmt);

    // --------------------------------------------------------
    // Expressions
    // --------------------------------------------------------

    HirExprPtr build_expr(const Expr* expr);

    HirExprPtr build_literal(const LiteralExpr& literal);
    HirExprPtr build_ident(const IdentExpr& ident);
    HirExprPtr build_unary(const UnaryExpr& unary);
    HirExprPtr build_binary(const BinaryExpr& binary);
    HirExprPtr build_call(const CallExpr& call);
};

} // namespace vitte::ir