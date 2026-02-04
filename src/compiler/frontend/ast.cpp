// ============================================================
// ast.cpp — Vitte Compiler
// Frontend AST implementation
// ============================================================

#include "ast.hpp"

#include <cassert>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace vitte::frontend::ast {

// ------------------------------------------------------------
// SourceSpan
// ------------------------------------------------------------

SourceSpan::SourceSpan() : file(nullptr), start(0), end(0) {}

SourceSpan::SourceSpan(const SourceFile* f, std::size_t s, std::size_t e)
    : file(f), start(s), end(e) {}

bool SourceSpan::is_valid() const {
    return file != nullptr && start <= end;
}

std::size_t SourceSpan::length() const {
    return end >= start ? end - start : 0;
}

// ------------------------------------------------------------
// AstNode
// ------------------------------------------------------------

AstNode::AstNode(NodeKind k, SourceSpan sp)
    : kind(k), span(sp) {}

AstNode::~AstNode() = default;

// ------------------------------------------------------------
// Ident
// ------------------------------------------------------------

Ident::Ident(std::string n, SourceSpan sp)
    : AstNode(NodeKind::Ident, sp), name(std::move(n)) {}

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

TypeNode::TypeNode(NodeKind k, SourceSpan sp)
    : AstNode(k, sp) {}

NamedType::NamedType(Ident id, SourceSpan sp)
    : TypeNode(NodeKind::NamedType, sp), ident(std::move(id)) {}

GenericType::GenericType(
    Ident base,
    std::vector<TypePtr> args,
    SourceSpan sp)
    : TypeNode(NodeKind::GenericType, sp),
      base_ident(std::move(base)),
      type_args(std::move(args)) {}

// ------------------------------------------------------------
// Expressions
// ------------------------------------------------------------

Expr::Expr(NodeKind k, SourceSpan sp)
    : AstNode(k, sp) {}

LiteralExpr::LiteralExpr(LiteralKind k, std::string v, SourceSpan sp)
    : Expr(NodeKind::LiteralExpr, sp),
      lit_kind(k),
      value(std::move(v)) {}

IdentExpr::IdentExpr(Ident id, SourceSpan sp)
    : Expr(NodeKind::IdentExpr, sp), ident(std::move(id)) {}

BinaryExpr::BinaryExpr(
    BinaryOp o,
    ExprPtr l,
    ExprPtr r,
    SourceSpan sp)
    : Expr(NodeKind::BinaryExpr, sp),
      op(o),
      lhs(std::move(l)),
      rhs(std::move(r)) {
    assert(lhs && rhs);
}

UnaryExpr::UnaryExpr(
    UnaryOp o,
    ExprPtr e,
    SourceSpan sp)
    : Expr(NodeKind::UnaryExpr, sp),
      op(o),
      expr(std::move(e)) {
    assert(expr);
}

CallExpr::CallExpr(
    ExprPtr cal,
    std::vector<ExprPtr> a,
    SourceSpan sp)
    : Expr(NodeKind::CallExpr, sp),
      callee(std::move(cal)),
      args(std::move(a)) {
    assert(callee);
}

// ------------------------------------------------------------
// Statements
// ------------------------------------------------------------

Stmt::Stmt(NodeKind k, SourceSpan sp)
    : AstNode(k, sp) {}

LetStmt::LetStmt(
    Ident id,
    TypePtr ty,
    ExprPtr init,
    SourceSpan sp)
    : Stmt(NodeKind::LetStmt, sp),
      ident(std::move(id)),
      type(std::move(ty)),
      initializer(std::move(init)) {}

ExprStmt::ExprStmt(ExprPtr e, SourceSpan sp)
    : Stmt(NodeKind::ExprStmt, sp), expr(std::move(e)) {
    assert(expr);
}

ReturnStmt::ReturnStmt(ExprPtr e, SourceSpan sp)
    : Stmt(NodeKind::ReturnStmt, sp), expr(std::move(e)) {}

BlockStmt::BlockStmt(
    std::vector<StmtPtr> s,
    SourceSpan sp)
    : Stmt(NodeKind::BlockStmt, sp),
      stmts(std::move(s)) {}

IfStmt::IfStmt(
    ExprPtr c,
    StmtPtr t,
    StmtPtr e,
    SourceSpan sp)
    : Stmt(NodeKind::IfStmt, sp),
      cond(std::move(c)),
      then_branch(std::move(t)),
      else_branch(std::move(e)) {
    assert(cond && then_branch);
}

// ------------------------------------------------------------
// Declarations
// ------------------------------------------------------------

Decl::Decl(NodeKind k, SourceSpan sp)
    : AstNode(k, sp) {}

FnParam::FnParam(Ident id, TypePtr ty)
    : ident(std::move(id)), type(std::move(ty)) {}

FnDecl::FnDecl(
    Ident n,
    std::vector<FnParam> p,
    TypePtr ret,
    BlockStmt body,
    SourceSpan sp)
    : Decl(NodeKind::FnDecl, sp),
      name(std::move(n)),
      params(std::move(p)),
      return_type(std::move(ret)),
      body(std::move(body)) {}

TypeDecl::TypeDecl(
    Ident n,
    std::vector<FieldDecl> f,
    SourceSpan sp)
    : Decl(NodeKind::TypeDecl, sp),
      name(std::move(n)),
      fields(std::move(f)) {}

FieldDecl::FieldDecl(Ident id, TypePtr ty)
    : ident(std::move(id)), type(std::move(ty)) {}

// ------------------------------------------------------------
// Module
// ------------------------------------------------------------

Module::Module(
    std::string n,
    std::vector<DeclPtr> d,
    SourceSpan sp)
    : AstNode(NodeKind::Module, sp),
      name(std::move(n)),
      decls(std::move(d)) {}

// ------------------------------------------------------------
// AST Visitor
// ------------------------------------------------------------

void AstVisitor::visit(AstNode& n) {
    switch (n.kind) {
        case NodeKind::Module:
            visit_module(static_cast<Module&>(n));
            break;
        case NodeKind::FnDecl:
            visit_fn(static_cast<FnDecl&>(n));
            break;
        case NodeKind::TypeDecl:
            visit_type(static_cast<TypeDecl&>(n));
            break;
        case NodeKind::BlockStmt:
            visit_block(static_cast<BlockStmt&>(n));
            break;
        case NodeKind::LetStmt:
            visit_let(static_cast<LetStmt&>(n));
            break;
        case NodeKind::IfStmt:
            visit_if(static_cast<IfStmt&>(n));
            break;
        case NodeKind::ReturnStmt:
            visit_return(static_cast<ReturnStmt&>(n));
            break;
        case NodeKind::ExprStmt:
            visit_expr_stmt(static_cast<ExprStmt&>(n));
            break;
        case NodeKind::BinaryExpr:
            visit_binary(static_cast<BinaryExpr&>(n));
            break;
        case NodeKind::UnaryExpr:
            visit_unary(static_cast<UnaryExpr&>(n));
            break;
        case NodeKind::CallExpr:
            visit_call(static_cast<CallExpr&>(n));
            break;
        case NodeKind::IdentExpr:
            visit_ident_expr(static_cast<IdentExpr&>(n));
            break;
        case NodeKind::LiteralExpr:
            visit_literal(static_cast<LiteralExpr&>(n));
            break;
        default:
            break;
    }
}

// ------------------------------------------------------------
// Debug dump
// ------------------------------------------------------------

static void indent(std::ostream& os, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        os << "  ";
    }
}

void dump(const AstNode& n, std::ostream& os, std::size_t depth) {
    indent(os, depth);
    os << to_string(n.kind);

    if (n.span.is_valid()) {
        os << " [" << n.span.start << ".." << n.span.end << "]";
    }

    os << "\n";

    switch (n.kind) {
        case NodeKind::Module: {
            auto& m = static_cast<const Module&>(n);
            for (auto& d : m.decls) {
                dump(*d, os, depth + 1);
            }
            break;
        }
        case NodeKind::FnDecl: {
            auto& f = static_cast<const FnDecl&>(n);
            dump(f.body, os, depth + 1);
            break;
        }
        case NodeKind::BlockStmt: {
            auto& b = static_cast<const BlockStmt&>(n);
            for (auto& s : b.stmts) {
                dump(*s, os, depth + 1);
            }
            break;
        }
        default:
            break;
    }
}

std::string dump_to_string(const AstNode& n) {
    std::ostringstream ss;
    dump(n, ss, 0);
    return ss.str();
}

} // namespace vitte::frontend::ast