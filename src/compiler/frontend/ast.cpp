// ============================================================
// ast.cpp — Vitte Compiler
// Frontend AST implementation
// ============================================================

#include "ast.hpp"

#include <cassert>
#include <sstream>
#include <utility>

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
// Attribute
// ------------------------------------------------------------

Attribute::Attribute(Ident n, SourceSpan sp)
    : AstNode(NodeKind::Attribute, sp), name(std::move(n)) {}

// ------------------------------------------------------------
// Module path
// ------------------------------------------------------------

ModulePath::ModulePath(std::vector<Ident> p, SourceSpan sp)
    : AstNode(NodeKind::ModulePath, sp), parts(std::move(p)) {}

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

TypeNode::TypeNode(NodeKind k, SourceSpan sp)
    : AstNode(k, sp) {}

NamedType::NamedType(Ident id, SourceSpan sp)
    : TypeNode(NodeKind::NamedType, sp), ident(std::move(id)) {}

GenericType::GenericType(
    Ident base,
    std::vector<TypeId> args,
    SourceSpan sp)
    : TypeNode(NodeKind::GenericType, sp),
      base_ident(std::move(base)),
      type_args(std::move(args)) {}

BuiltinType::BuiltinType(std::string n, SourceSpan sp)
    : TypeNode(NodeKind::BuiltinType, sp), name(std::move(n)) {}

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

UnaryExpr::UnaryExpr(UnaryOp o, ExprId e, SourceSpan sp)
    : Expr(NodeKind::UnaryExpr, sp),
      op(o),
      expr(e) {}

BinaryExpr::BinaryExpr(BinaryOp o, ExprId l, ExprId r, SourceSpan sp)
    : Expr(NodeKind::BinaryExpr, sp),
      op(o),
      lhs(l),
      rhs(r) {}

CallNoParenExpr::CallNoParenExpr(Ident c, ExprId a, SourceSpan sp)
    : Expr(NodeKind::CallNoParenExpr, sp),
      callee(std::move(c)),
      arg(a) {}

InvokeExpr::InvokeExpr(
    ExprId callee_expr_in,
    TypeId callee_type_in,
    std::vector<ExprId> args_in,
    SourceSpan sp)
    : Expr(NodeKind::InvokeExpr, sp),
      callee_expr(callee_expr_in),
      callee_type(callee_type_in),
      args(std::move(args_in)) {}

ListExpr::ListExpr(std::vector<ExprId> items, SourceSpan sp)
    : Expr(NodeKind::ListExpr, sp), items(std::move(items)) {}

// ------------------------------------------------------------
// Patterns
// ------------------------------------------------------------

Pattern::Pattern(NodeKind k, SourceSpan sp)
    : AstNode(k, sp) {}

IdentPattern::IdentPattern(Ident id, SourceSpan sp)
    : Pattern(NodeKind::IdentPattern, sp), ident(std::move(id)) {}

CtorPattern::CtorPattern(TypeId t, std::vector<PatternId> a, SourceSpan sp)
    : Pattern(NodeKind::CtorPattern, sp),
      type(t),
      args(std::move(a)) {}

// ------------------------------------------------------------
// Statements
// ------------------------------------------------------------

Stmt::Stmt(NodeKind k, SourceSpan sp)
    : AstNode(k, sp) {}

LetStmt::LetStmt(
    Ident id,
    TypeId ty,
    ExprId init,
    SourceSpan sp)
    : Stmt(NodeKind::LetStmt, sp),
      ident(std::move(id)),
      type(ty),
      initializer(init) {}

MakeStmt::MakeStmt(Ident id, TypeId ty, ExprId v, SourceSpan sp)
    : Stmt(NodeKind::MakeStmt, sp),
      ident(std::move(id)),
      type(ty),
      value(v) {}

SetStmt::SetStmt(Ident id, ExprId v, SourceSpan sp)
    : Stmt(NodeKind::SetStmt, sp),
      ident(std::move(id)),
      value(v) {}

GiveStmt::GiveStmt(ExprId v, SourceSpan sp)
    : Stmt(NodeKind::GiveStmt, sp), value(v) {}

EmitStmt::EmitStmt(ExprId v, SourceSpan sp)
    : Stmt(NodeKind::EmitStmt, sp), value(v) {}

ExprStmt::ExprStmt(ExprId e, SourceSpan sp)
    : Stmt(NodeKind::ExprStmt, sp), expr(e) {}

ReturnStmt::ReturnStmt(ExprId e, SourceSpan sp)
    : Stmt(NodeKind::ReturnStmt, sp), expr(e) {}

BlockStmt::BlockStmt(std::vector<StmtId> s, SourceSpan sp)
    : Stmt(NodeKind::BlockStmt, sp), stmts(std::move(s)) {}

IfStmt::IfStmt(
    ExprId c,
    StmtId t,
    StmtId e,
    SourceSpan sp)
    : Stmt(NodeKind::IfStmt, sp),
      cond(c),
      then_block(t),
      else_block(e) {}

WhenStmt::WhenStmt(PatternId p, StmtId b, SourceSpan sp)
    : Stmt(NodeKind::WhenStmt, sp),
      pattern(p),
      block(b) {}

SelectStmt::SelectStmt(
    ExprId e,
    std::vector<StmtId> w,
    StmtId o,
    SourceSpan sp)
    : Stmt(NodeKind::SelectStmt, sp),
      expr(e),
      whens(std::move(w)),
      otherwise_block(o) {}

// ------------------------------------------------------------
// Declarations
// ------------------------------------------------------------

FieldDecl::FieldDecl(Ident id, TypeId ty)
    : ident(std::move(id)), type(ty) {}

CaseField::CaseField(Ident id, TypeId ty)
    : ident(std::move(id)), type(ty) {}

CaseDecl::CaseDecl(Ident id, std::vector<CaseField> f)
    : ident(std::move(id)), fields(std::move(f)) {}

FnParam::FnParam(Ident id, TypeId ty)
    : ident(std::move(id)), type(ty) {}

Decl::Decl(NodeKind k, SourceSpan sp)
    : AstNode(k, sp) {}

FnDecl::FnDecl(
    Ident n,
    std::vector<FnParam> p,
    TypeId rt,
    StmtId b,
    SourceSpan sp)
    : Decl(NodeKind::FnDecl, sp),
      name(std::move(n)),
      params(std::move(p)),
      return_type(rt),
      body(b) {}

TypeDecl::TypeDecl(Ident n, std::vector<FieldDecl> f, SourceSpan sp)
    : Decl(NodeKind::TypeDecl, sp),
      name(std::move(n)),
      fields(std::move(f)) {}

SpaceDecl::SpaceDecl(ModulePath p, SourceSpan sp)
    : Decl(NodeKind::SpaceDecl, sp), path(std::move(p)) {}

PullDecl::PullDecl(ModulePath p, std::optional<Ident> a, SourceSpan sp)
    : Decl(NodeKind::PullDecl, sp),
      path(std::move(p)),
      alias(std::move(a)) {}

ShareDecl::ShareDecl(bool all, std::vector<Ident> n, SourceSpan sp)
    : Decl(NodeKind::ShareDecl, sp),
      share_all(all),
      names(std::move(n)) {}

FormDecl::FormDecl(Ident n, std::vector<FieldDecl> f, SourceSpan sp)
    : Decl(NodeKind::FormDecl, sp),
      name(std::move(n)),
      fields(std::move(f)) {}

PickDecl::PickDecl(Ident n, std::vector<CaseDecl> c, SourceSpan sp)
    : Decl(NodeKind::PickDecl, sp),
      name(std::move(n)),
      cases(std::move(c)) {}

ProcDecl::ProcDecl(
    std::vector<Attribute> a,
    Ident n,
    std::vector<Ident> p,
    StmtId b,
    SourceSpan sp)
    : Decl(NodeKind::ProcDecl, sp),
      attrs(std::move(a)),
      name(std::move(n)),
      params(std::move(p)),
      body(b) {}

EntryDecl::EntryDecl(Ident n, ModulePath m, StmtId b, SourceSpan sp)
    : Decl(NodeKind::EntryDecl, sp),
      name(std::move(n)),
      module(std::move(m)),
      body(b) {}

// ------------------------------------------------------------
// Module
// ------------------------------------------------------------

Module::Module(std::string n, std::vector<DeclId> d, SourceSpan sp)
    : AstNode(NodeKind::Module, sp),
      name(std::move(n)),
      decls(std::move(d)) {}

// ------------------------------------------------------------
// Visitor
// ------------------------------------------------------------

void AstVisitor::visit(AstNode& node) {
    (void)node;
}

// ------------------------------------------------------------
// Debug helpers
// ------------------------------------------------------------

const char* to_string(NodeKind kind) {
    switch (kind) {
        case NodeKind::Module: return "Module";
        case NodeKind::Ident: return "Ident";
        case NodeKind::Attribute: return "Attribute";
        case NodeKind::ModulePath: return "ModulePath";
        case NodeKind::NamedType: return "NamedType";
        case NodeKind::GenericType: return "GenericType";
        case NodeKind::BuiltinType: return "BuiltinType";
        case NodeKind::LiteralExpr: return "LiteralExpr";
        case NodeKind::IdentExpr: return "IdentExpr";
        case NodeKind::UnaryExpr: return "UnaryExpr";
        case NodeKind::BinaryExpr: return "BinaryExpr";
        case NodeKind::CallNoParenExpr: return "CallNoParenExpr";
        case NodeKind::InvokeExpr: return "InvokeExpr";
        case NodeKind::ListExpr: return "ListExpr";
        case NodeKind::IdentPattern: return "IdentPattern";
        case NodeKind::CtorPattern: return "CtorPattern";
        case NodeKind::BlockStmt: return "BlockStmt";
        case NodeKind::LetStmt: return "LetStmt";
        case NodeKind::ExprStmt: return "ExprStmt";
        case NodeKind::ReturnStmt: return "ReturnStmt";
        case NodeKind::IfStmt: return "IfStmt";
        case NodeKind::MakeStmt: return "MakeStmt";
        case NodeKind::SetStmt: return "SetStmt";
        case NodeKind::GiveStmt: return "GiveStmt";
        case NodeKind::EmitStmt: return "EmitStmt";
        case NodeKind::SelectStmt: return "SelectStmt";
        case NodeKind::WhenStmt: return "WhenStmt";
        case NodeKind::FnDecl: return "FnDecl";
        case NodeKind::TypeDecl: return "TypeDecl";
        case NodeKind::SpaceDecl: return "SpaceDecl";
        case NodeKind::PullDecl: return "PullDecl";
        case NodeKind::ShareDecl: return "ShareDecl";
        case NodeKind::FormDecl: return "FormDecl";
        case NodeKind::PickDecl: return "PickDecl";
        case NodeKind::ProcDecl: return "ProcDecl";
        case NodeKind::EntryDecl: return "EntryDecl";
        default:
            return "Unknown";
    }
}

std::string dump_to_string(const AstNode& node) {
    std::ostringstream os;
    os << to_string(node.kind);
    return os.str();
}

} // namespace vitte::frontend::ast
