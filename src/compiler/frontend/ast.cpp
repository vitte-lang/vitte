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

AttributeArg::AttributeArg(Kind k, std::string v)
    : kind(k), value(std::move(v)) {}

Attribute::Attribute(Ident n, std::vector<AttributeArg> a, SourceSpan sp)
    : AstNode(NodeKind::Attribute, sp),
      name(std::move(n)),
      args(std::move(a)) {}

// ------------------------------------------------------------
// Module path
// ------------------------------------------------------------

ModulePath::ModulePath(std::vector<Ident> p, std::size_t rel, SourceSpan sp)
    : AstNode(NodeKind::ModulePath, sp),
      parts(std::move(p)),
      relative_depth(rel) {}

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

PointerType::PointerType(TypeId p, SourceSpan sp)
    : TypeNode(NodeKind::PointerType, sp), pointee(p) {}

SliceType::SliceType(TypeId e, SourceSpan sp)
    : TypeNode(NodeKind::SliceType, sp), element(e) {}

ProcType::ProcType(std::vector<TypeId> p, TypeId r, SourceSpan sp)
    : TypeNode(NodeKind::ProcType, sp),
      params(std::move(p)),
      return_type(r) {}

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

ProcExpr::ProcExpr(std::vector<FnParam> p, TypeId rt, StmtId b, SourceSpan sp)
    : Expr(NodeKind::ProcExpr, sp),
      params(std::move(p)),
      return_type(rt),
      body(b) {}

MemberExpr::MemberExpr(ExprId b, Ident m, SourceSpan sp)
    : Expr(NodeKind::MemberExpr, sp),
      base(b),
      member(std::move(m)) {}

IndexExpr::IndexExpr(ExprId b, ExprId i, SourceSpan sp)
    : Expr(NodeKind::IndexExpr, sp),
      base(b),
      index(i) {}

IfExpr::IfExpr(ExprId c, StmtId t, StmtId e, SourceSpan sp)
    : Expr(NodeKind::IfExpr, sp),
      cond(c),
      then_block(t),
      else_block(e) {}

IsExpr::IsExpr(ExprId v, PatternId p, SourceSpan sp)
    : Expr(NodeKind::IsExpr, sp),
      value(v),
      pattern(p) {}

AsExpr::AsExpr(ExprId v, TypeId t, SourceSpan sp)
    : Expr(NodeKind::AsExpr, sp),
      value(v),
      type(t) {}
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

AsmStmt::AsmStmt(std::string c, SourceSpan sp)
    : Stmt(NodeKind::AsmStmt, sp),
      code(std::move(c)) {}

UnsafeStmt::UnsafeStmt(StmtId b, SourceSpan sp)
    : Stmt(NodeKind::UnsafeStmt, sp),
      body(b) {}

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

LoopStmt::LoopStmt(StmtId b, SourceSpan sp)
    : Stmt(NodeKind::LoopStmt, sp), body(b) {}

BreakStmt::BreakStmt(SourceSpan sp)
    : Stmt(NodeKind::BreakStmt, sp) {}

ContinueStmt::ContinueStmt(SourceSpan sp)
    : Stmt(NodeKind::ContinueStmt, sp) {}

ForStmt::ForStmt(Ident id, ExprId it, StmtId b, SourceSpan sp)
    : Stmt(NodeKind::ForStmt, sp),
      ident(std::move(id)),
      iterable(it),
      body(b) {}
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

TypeAliasDecl::TypeAliasDecl(Ident n, std::vector<Ident> tp, TypeId t, SourceSpan sp)
    : Decl(NodeKind::TypeAliasDecl, sp),
      name(std::move(n)),
      type_params(std::move(tp)),
      target(t) {}

SpaceDecl::SpaceDecl(ModulePath p, SourceSpan sp)
    : Decl(NodeKind::SpaceDecl, sp), path(std::move(p)) {}

PullDecl::PullDecl(ModulePath p, std::optional<Ident> a, SourceSpan sp)
    : Decl(NodeKind::PullDecl, sp),
      path(std::move(p)),
      alias(std::move(a)) {}

UseDecl::UseDecl(ModulePath p, std::optional<Ident> a, bool glob, SourceSpan sp)
    : Decl(NodeKind::UseDecl, sp),
      path(std::move(p)),
      alias(std::move(a)),
      is_glob(glob) {}

ShareDecl::ShareDecl(bool all, std::vector<Ident> n, SourceSpan sp)
    : Decl(NodeKind::ShareDecl, sp),
      share_all(all),
      names(std::move(n)) {}

ConstDecl::ConstDecl(Ident n, TypeId t, ExprId v, SourceSpan sp)
    : Decl(NodeKind::ConstDecl, sp),
      name(std::move(n)),
      type(t),
      value(v) {}

MacroDecl::MacroDecl(Ident n, std::vector<Ident> p, StmtId b, SourceSpan sp)
    : Decl(NodeKind::MacroDecl, sp),
      name(std::move(n)),
      params(std::move(p)),
      body(b) {}

FormDecl::FormDecl(Ident n, std::vector<Ident> tp, std::vector<FieldDecl> f, SourceSpan sp)
    : Decl(NodeKind::FormDecl, sp),
      name(std::move(n)),
      type_params(std::move(tp)),
      fields(std::move(f)) {}

PickDecl::PickDecl(Ident n, std::vector<Ident> tp, std::vector<CaseDecl> c, SourceSpan sp)
    : Decl(NodeKind::PickDecl, sp),
      name(std::move(n)),
      type_params(std::move(tp)),
      cases(std::move(c)) {}

ProcDecl::ProcDecl(
    std::vector<Attribute> a,
    Ident n,
    std::vector<Ident> tp,
    std::vector<FnParam> p,
    TypeId rt,
    StmtId b,
    SourceSpan sp)
    : Decl(NodeKind::ProcDecl, sp),
      attrs(std::move(a)),
      name(std::move(n)),
      type_params(std::move(tp)),
      params(std::move(p)),
      return_type(rt),
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
        case NodeKind::PointerType: return "PointerType";
        case NodeKind::SliceType: return "SliceType";
        case NodeKind::ProcType: return "ProcType";
        case NodeKind::LiteralExpr: return "LiteralExpr";
        case NodeKind::IdentExpr: return "IdentExpr";
        case NodeKind::UnaryExpr: return "UnaryExpr";
        case NodeKind::BinaryExpr: return "BinaryExpr";
        case NodeKind::ProcExpr: return "ProcExpr";
        case NodeKind::MemberExpr: return "MemberExpr";
        case NodeKind::IndexExpr: return "IndexExpr";
        case NodeKind::IfExpr: return "IfExpr";
        case NodeKind::IsExpr: return "IsExpr";
        case NodeKind::AsExpr: return "AsExpr";
        case NodeKind::CallNoParenExpr: return "CallNoParenExpr";
        case NodeKind::InvokeExpr: return "InvokeExpr";
        case NodeKind::ListExpr: return "ListExpr";
        case NodeKind::IdentPattern: return "IdentPattern";
        case NodeKind::CtorPattern: return "CtorPattern";
        case NodeKind::BlockStmt: return "BlockStmt";
        case NodeKind::AsmStmt: return "AsmStmt";
        case NodeKind::UnsafeStmt: return "UnsafeStmt";
        case NodeKind::LetStmt: return "LetStmt";
        case NodeKind::ExprStmt: return "ExprStmt";
        case NodeKind::ReturnStmt: return "ReturnStmt";
        case NodeKind::IfStmt: return "IfStmt";
        case NodeKind::LoopStmt: return "LoopStmt";
        case NodeKind::BreakStmt: return "BreakStmt";
        case NodeKind::ContinueStmt: return "ContinueStmt";
        case NodeKind::ForStmt: return "ForStmt";
        case NodeKind::MakeStmt: return "MakeStmt";
        case NodeKind::SetStmt: return "SetStmt";
        case NodeKind::GiveStmt: return "GiveStmt";
        case NodeKind::EmitStmt: return "EmitStmt";
        case NodeKind::SelectStmt: return "SelectStmt";
        case NodeKind::WhenStmt: return "WhenStmt";
        case NodeKind::FnDecl: return "FnDecl";
        case NodeKind::TypeDecl: return "TypeDecl";
        case NodeKind::TypeAliasDecl: return "TypeAliasDecl";
        case NodeKind::SpaceDecl: return "SpaceDecl";
        case NodeKind::PullDecl: return "PullDecl";
        case NodeKind::UseDecl: return "UseDecl";
        case NodeKind::ShareDecl: return "ShareDecl";
        case NodeKind::ConstDecl: return "ConstDecl";
        case NodeKind::MacroDecl: return "MacroDecl";
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
