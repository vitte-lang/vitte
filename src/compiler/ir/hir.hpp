// ============================================================
// hir.hpp — Vitte Compiler
// High-level Intermediate Representation (HIR)
// ============================================================

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "ast.hpp"   // SourceSpan

namespace vitte::ir {

// ------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------

struct HirNode;
struct HirType;
struct HirExpr;
struct HirStmt;
struct HirDecl;
struct HirModule;

// ------------------------------------------------------------
// Pointer aliases
// ------------------------------------------------------------

using HirNodePtr = std::unique_ptr<HirNode>;
using HirTypePtr = std::unique_ptr<HirType>;
using HirExprPtr = std::unique_ptr<HirExpr>;
using HirStmtPtr = std::unique_ptr<HirStmt>;
using HirDeclPtr = std::unique_ptr<HirDecl>;

// ------------------------------------------------------------
// HIR Kind
// ------------------------------------------------------------

enum class HirKind {
    // root
    Module,

    // types
    NamedType,
    GenericType,

    // expressions
    LiteralExpr,
    VarExpr,
    UnaryExpr,
    BinaryExpr,
    CallExpr,

    // statements
    LetStmt,
    ExprStmt,
    ReturnStmt,
    Block,

    // declarations
    FnDecl,
};

// ------------------------------------------------------------
// Base node
// ------------------------------------------------------------

struct HirNode {
    HirKind kind;
    vitte::frontend::ast::SourceSpan span;

    explicit HirNode(HirKind kind,
                     vitte::frontend::ast::SourceSpan span);
    virtual ~HirNode();
};

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

struct HirType : HirNode {
    explicit HirType(HirKind kind,
                     vitte::frontend::ast::SourceSpan span);
};

struct HirNamedType : HirType {
    std::string name;

    HirNamedType(std::string name,
                 vitte::frontend::ast::SourceSpan span);
};

struct HirGenericType : HirType {
    std::string base_name;
    std::vector<HirTypePtr> type_args;

    HirGenericType(std::string base_name,
                   std::vector<HirTypePtr> type_args,
                   vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Expressions
// ------------------------------------------------------------

enum class HirLiteralKind {
    Bool,
    Int,
    String,
};

enum class HirUnaryOp {
    Not,
    Neg,
};

enum class HirBinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or,
};

struct HirExpr : HirNode {
    explicit HirExpr(HirKind kind,
                     vitte::frontend::ast::SourceSpan span);
};

struct HirLiteralExpr : HirExpr {
    HirLiteralKind lit_kind;
    std::string value;

    HirLiteralExpr(HirLiteralKind kind,
                   std::string value,
                   vitte::frontend::ast::SourceSpan span);
};

struct HirVarExpr : HirExpr {
    std::string name;

    HirVarExpr(std::string name,
               vitte::frontend::ast::SourceSpan span);
};

struct HirUnaryExpr : HirExpr {
    HirUnaryOp op;
    HirExprPtr expr;

    HirUnaryExpr(HirUnaryOp op,
                 HirExprPtr expr,
                 vitte::frontend::ast::SourceSpan span);
};

struct HirBinaryExpr : HirExpr {
    HirBinaryOp op;
    HirExprPtr lhs;
    HirExprPtr rhs;

    HirBinaryExpr(HirBinaryOp op,
                  HirExprPtr lhs,
                  HirExprPtr rhs,
                  vitte::frontend::ast::SourceSpan span);
};

struct HirCallExpr : HirExpr {
    HirExprPtr callee;
    std::vector<HirExprPtr> args;

    HirCallExpr(HirExprPtr callee,
                std::vector<HirExprPtr> args,
                vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Statements
// ------------------------------------------------------------

struct HirStmt : HirNode {
    explicit HirStmt(HirKind kind,
                     vitte::frontend::ast::SourceSpan span);
};

struct HirLetStmt : HirStmt {
    std::string name;
    HirTypePtr type;
    HirExprPtr init;

    HirLetStmt(std::string name,
               HirTypePtr type,
               HirExprPtr init,
               vitte::frontend::ast::SourceSpan span);
};

struct HirExprStmt : HirStmt {
    HirExprPtr expr;

    HirExprStmt(HirExprPtr expr,
                vitte::frontend::ast::SourceSpan span);
};

struct HirReturnStmt : HirStmt {
    HirExprPtr expr;

    HirReturnStmt(HirExprPtr expr,
                  vitte::frontend::ast::SourceSpan span);
};

struct HirBlock : HirStmt {
    std::vector<HirStmtPtr> stmts;

    HirBlock(std::vector<HirStmtPtr> stmts,
             vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Declarations
// ------------------------------------------------------------

struct HirDecl : HirNode {
    explicit HirDecl(HirKind kind,
                     vitte::frontend::ast::SourceSpan span);
};

struct HirParam {
    std::string name;
    HirTypePtr type;

    HirParam(std::string name,
             HirTypePtr type);
};

struct HirFnDecl : HirDecl {
    std::string name;
    std::vector<HirParam> params;
    HirTypePtr return_type;
    HirBlock body;

    HirFnDecl(std::string name,
              std::vector<HirParam> params,
              HirTypePtr return_type,
              HirBlock body,
              vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Module
// ------------------------------------------------------------

struct HirModule : HirNode {
    std::string name;
    std::vector<HirDeclPtr> decls;

    HirModule(std::string name,
              std::vector<HirDeclPtr> decls,
              vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Debug helpers
// ------------------------------------------------------------

const char* to_string(HirKind kind);

void dump(const HirNode& node,
          std::ostream& os,
          std::size_t depth = 0);

std::string dump_to_string(const HirNode& node);

} // namespace vitte::ir