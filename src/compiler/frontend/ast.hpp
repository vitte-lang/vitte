// ============================================================
// ast.hpp — Vitte Compiler
// Frontend AST definitions
// ============================================================

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace vitte::frontend::ast {

// ------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------

struct AstNode;
struct TypeNode;
struct Expr;
struct Stmt;
struct Decl;
struct Module;

// ------------------------------------------------------------
// Common pointer aliases
// ------------------------------------------------------------

using AstPtr  = std::unique_ptr<AstNode>;
using TypePtr = std::unique_ptr<TypeNode>;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using DeclPtr = std::unique_ptr<Decl>;

// ------------------------------------------------------------
// Source handling
// ------------------------------------------------------------

struct SourceFile {
    std::string path;
    std::string content;
};

struct SourceSpan {
    const SourceFile* file;
    std::size_t start;
    std::size_t end;

    SourceSpan();
    SourceSpan(const SourceFile* file, std::size_t start, std::size_t end);

    bool is_valid() const;
    std::size_t length() const;
};

// ------------------------------------------------------------
// NodeKind
// ------------------------------------------------------------

enum class NodeKind {
    // root
    Module,

    // identifiers
    Ident,

    // types
    NamedType,
    GenericType,

    // expressions
    LiteralExpr,
    IdentExpr,
    UnaryExpr,
    BinaryExpr,
    CallExpr,

    // statements
    BlockStmt,
    LetStmt,
    ExprStmt,
    ReturnStmt,
    IfStmt,

    // declarations
    FnDecl,
    TypeDecl,
};

// ------------------------------------------------------------
// AST base node
// ------------------------------------------------------------

struct AstNode {
    NodeKind kind;
    SourceSpan span;

    explicit AstNode(NodeKind kind, SourceSpan span);
    virtual ~AstNode();
};

// ------------------------------------------------------------
// Ident
// ------------------------------------------------------------

struct Ident : AstNode {
    std::string name;

    explicit Ident(std::string name, SourceSpan span);
};

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

struct TypeNode : AstNode {
    explicit TypeNode(NodeKind kind, SourceSpan span);
};

struct NamedType : TypeNode {
    Ident ident;

    explicit NamedType(Ident ident, SourceSpan span);
};

struct GenericType : TypeNode {
    Ident base_ident;
    std::vector<TypePtr> type_args;

    GenericType(
        Ident base_ident,
        std::vector<TypePtr> type_args,
        SourceSpan span);
};

// ------------------------------------------------------------
// Expressions
// ------------------------------------------------------------

enum class LiteralKind {
    Bool,
    Int,
    String,
};

enum class UnaryOp {
    Not,
    Neg,
};

enum class BinaryOp {
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

struct Expr : AstNode {
    explicit Expr(NodeKind kind, SourceSpan span);
};

struct LiteralExpr : Expr {
    LiteralKind lit_kind;
    std::string value;

    LiteralExpr(LiteralKind kind, std::string value, SourceSpan span);
};

struct IdentExpr : Expr {
    Ident ident;

    IdentExpr(Ident ident, SourceSpan span);
};

struct UnaryExpr : Expr {
    UnaryOp op;
    ExprPtr expr;

    UnaryExpr(UnaryOp op, ExprPtr expr, SourceSpan span);
};

struct BinaryExpr : Expr {
    BinaryOp op;
    ExprPtr lhs;
    ExprPtr rhs;

    BinaryExpr(BinaryOp op, ExprPtr lhs, ExprPtr rhs, SourceSpan span);
};

struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;

    CallExpr(ExprPtr callee, std::vector<ExprPtr> args, SourceSpan span);
};

// ------------------------------------------------------------
// Statements
// ------------------------------------------------------------

struct Stmt : AstNode {
    explicit Stmt(NodeKind kind, SourceSpan span);
};

struct LetStmt : Stmt {
    Ident ident;
    TypePtr type;
    ExprPtr initializer;

    LetStmt(
        Ident ident,
        TypePtr type,
        ExprPtr initializer,
        SourceSpan span);
};

struct ExprStmt : Stmt {
    ExprPtr expr;

    ExprStmt(ExprPtr expr, SourceSpan span);
};

struct ReturnStmt : Stmt {
    ExprPtr expr;

    explicit ReturnStmt(ExprPtr expr, SourceSpan span);
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> stmts;

    explicit BlockStmt(std::vector<StmtPtr> stmts, SourceSpan span);
};

struct IfStmt : Stmt {
    ExprPtr cond;
    StmtPtr then_branch;
    StmtPtr else_branch;

    IfStmt(
        ExprPtr cond,
        StmtPtr then_branch,
        StmtPtr else_branch,
        SourceSpan span);
};

// ------------------------------------------------------------
// Declarations
// ------------------------------------------------------------

struct FieldDecl {
    Ident ident;
    TypePtr type;

    FieldDecl(Ident ident, TypePtr type);
};

struct FnParam {
    Ident ident;
    TypePtr type;

    FnParam(Ident ident, TypePtr type);
};

struct Decl : AstNode {
    explicit Decl(NodeKind kind, SourceSpan span);
};

struct FnDecl : Decl {
    Ident name;
    std::vector<FnParam> params;
    TypePtr return_type;
    BlockStmt body;

    FnDecl(
        Ident name,
        std::vector<FnParam> params,
        TypePtr return_type,
        BlockStmt body,
        SourceSpan span);
};

struct TypeDecl : Decl {
    Ident name;
    std::vector<FieldDecl> fields;

    TypeDecl(
        Ident name,
        std::vector<FieldDecl> fields,
        SourceSpan span);
};

// ------------------------------------------------------------
// Module
// ------------------------------------------------------------

struct Module : AstNode {
    std::string name;
    std::vector<DeclPtr> decls;

    Module(
        std::string name,
        std::vector<DeclPtr> decls,
        SourceSpan span);
};

// ------------------------------------------------------------
// Visitor
// ------------------------------------------------------------

struct AstVisitor {
    virtual ~AstVisitor() = default;

    virtual void visit(AstNode& node);

    virtual void visit_module(Module&) {}
    virtual void visit_fn(FnDecl&) {}
    virtual void visit_type(TypeDecl&) {}

    virtual void visit_block(BlockStmt&) {}
    virtual void visit_let(LetStmt&) {}
    virtual void visit_if(IfStmt&) {}
    virtual void visit_return(ReturnStmt&) {}
    virtual void visit_expr_stmt(ExprStmt&) {}

    virtual void visit_binary(BinaryExpr&) {}
    virtual void visit_unary(UnaryExpr&) {}
    virtual void visit_call(CallExpr&) {}
    virtual void visit_ident_expr(IdentExpr&) {}
    virtual void visit_literal(LiteralExpr&) {}
};

// ------------------------------------------------------------
// Debug helpers
// ------------------------------------------------------------

const char* to_string(NodeKind kind);
std::string dump_to_string(const AstNode& node);

} // namespace vitte::frontend::ast