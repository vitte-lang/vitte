// ============================================================
// ast.cpp — Vitte Compiler
// Frontend AST implementation
// ============================================================

#include "ast.hpp"

#include <cassert>
#include <functional>
#include <sstream>
#include <unordered_set>
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

BuiltinExpr::BuiltinExpr(
    Kind kind_in,
    TypeId type_in,
    ExprId expr_in,
    ModulePath path_in,
    Ident member_in,
    SourceSpan sp)
    : Expr(NodeKind::BuiltinExpr, sp),
      kind(kind_in),
      type(type_in),
      expr(expr_in),
      path(std::move(path_in)),
      member(std::move(member_in)) {}

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
    std::vector<TypeId> type_args_in,
    std::vector<InvokeArg> args_in,
    SourceSpan sp)
    : Expr(NodeKind::InvokeExpr, sp),
      callee_expr(callee_expr_in),
      callee_type(callee_type_in),
      type_args(std::move(type_args_in)),
      args(std::move(args_in)) {}

ListExpr::ListExpr(std::vector<ExprId> items, SourceSpan sp)
    : Expr(NodeKind::ListExpr, sp), items(std::move(items)) {}

ListCompExpr::ListCompExpr(
    Kind kind_in,
    ExprId key_in,
    ExprId v,
    std::optional<Ident> idx,
    Ident id,
    ExprId it,
    ExprId cond,
    SourceSpan sp)
    : Expr(NodeKind::ListCompExpr, sp),
      kind(kind_in),
      key(key_in),
      value(v),
      index_ident(std::move(idx)),
      ident(std::move(id)),
      iterable(it),
      condition(cond) {}

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

WildcardPattern::WildcardPattern(SourceSpan sp)
    : Pattern(NodeKind::WildcardPattern, sp) {}

// ------------------------------------------------------------
// Statements
// ------------------------------------------------------------

Stmt::Stmt(NodeKind k, SourceSpan sp)
    : AstNode(k, sp) {}

LetBinding::LetBinding(Ident id)
    : ident(std::move(id)), children() {}

LetBinding::LetBinding(std::vector<LetBinding> ch, SourceSpan sp)
    : ident(Ident("", sp)), children(std::move(ch)) {}

bool LetBinding::is_nested() const {
    return !children.empty();
}

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
      is_destructuring(false),
      is_mutable(false),
      bindings(),
      type(ty),
      initializer(init) {}

LetStmt::LetStmt(
    std::vector<LetBinding> bs,
    TypeId ty,
    ExprId init,
    bool mut,
    SourceSpan sp)
    : Stmt(NodeKind::LetStmt, sp),
      ident(Ident("", sp)),
      is_destructuring(true),
      is_mutable(mut),
      bindings(std::move(bs)),
      type(ty),
      initializer(init) {}

MakeStmt::MakeStmt(Ident id, TypeId ty, ExprId v, SourceSpan sp)
    : Stmt(NodeKind::MakeStmt, sp),
      ident(std::move(id)),
      type(ty),
      value(v) {}

SetStmt::SetStmt(ExprId t, ExprId v, SourceSpan sp)
    : Stmt(NodeKind::SetStmt, sp),
      target(t),
      value(v) {}

GiveStmt::GiveStmt(ExprId v, SourceSpan sp)
    : Stmt(NodeKind::GiveStmt, sp), value(v) {}

EmitStmt::EmitStmt(ExprId v, SourceSpan sp)
    : Stmt(NodeKind::EmitStmt, sp), value(v) {}

ExprStmt::ExprStmt(ExprId e, SourceSpan sp)
    : Stmt(NodeKind::ExprStmt, sp), expr(e) {}

ReturnStmt::ReturnStmt(ExprId e, SourceSpan sp)
    : Stmt(NodeKind::ReturnStmt, sp), expr(e) {}

TryStmt::TryStmt(StmtId b, StmtId e, StmtId f, SourceSpan sp)
    : Stmt(NodeKind::TryStmt, sp),
      body(b),
      except_body(e),
      finally_body(f) {}

RaiseStmt::RaiseStmt(ExprId e, SourceSpan sp)
    : Stmt(NodeKind::RaiseStmt, sp), expr(e) {}

DeferStmt::DeferStmt(StmtId b, SourceSpan sp)
    : Stmt(NodeKind::DeferStmt, sp), body(b) {}

WithStmt::WithStmt(ExprId e, std::optional<PatternId> p, StmtId b, SourceSpan sp)
    : Stmt(NodeKind::WithStmt, sp),
      expr(e),
      pattern(std::move(p)),
      body(b) {}

CriticalStmt::CriticalStmt(StmtId b, SourceSpan sp)
    : Stmt(NodeKind::CriticalStmt, sp), body(b) {}

AtomicStmt::AtomicStmt(StmtId b, SourceSpan sp)
    : Stmt(NodeKind::AtomicStmt, sp), body(b) {}

VolatileStmt::VolatileStmt(StmtId b, SourceSpan sp)
    : Stmt(NodeKind::VolatileStmt, sp), body(b) {}

GotoStmt::GotoStmt(Ident t, SourceSpan sp)
    : Stmt(NodeKind::GotoStmt, sp), target(std::move(t)) {}

PreemptStmt::PreemptStmt(bool e, SourceSpan sp)
    : Stmt(NodeKind::PreemptStmt, sp), enabled(e) {}

IrqStmt::IrqStmt(bool e, SourceSpan sp)
    : Stmt(NodeKind::IrqStmt, sp), enabled(e) {}

LabelStmt::LabelStmt(Ident n, SourceSpan sp)
    : Stmt(NodeKind::LabelStmt, sp), name(std::move(n)) {}

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

ForStmt::ForStmt(std::optional<Ident> idx, Ident id, ExprId it, StmtId b, bool tuple_destructure_in, SourceSpan sp)
    : Stmt(NodeKind::ForStmt, sp),
      index_ident(std::move(idx)),
      ident(std::move(id)),
      iterable(it),
      body(b),
      tuple_destructure(tuple_destructure_in) {}
WhenStmt::WhenStmt(PatternId p, ExprId g, StmtId b, SourceSpan sp)
    : Stmt(NodeKind::WhenStmt, sp),
      pattern(p),
      guard(g),
      block(b) {}

ComptimeDecl::ComptimeDecl(StmtId b, SourceSpan sp)
    : Decl(NodeKind::ComptimeDecl, sp),
      body(b) {}

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

FieldDecl::FieldDecl(Ident id, TypeId ty, std::optional<std::string> vis)
    : ident(std::move(id)), type(ty), visibility(std::move(vis)) {}

CaseField::CaseField(Ident id, TypeId ty)
    : ident(std::move(id)), type(ty) {}

CaseDecl::CaseDecl(Ident id, std::vector<CaseField> f)
    : ident(std::move(id)), fields(std::move(f)) {}

FnParam::FnParam(
    Ident id,
    TypeId ty,
    ExprId default_value,
    std::optional<ParamMode> mode,
    bool is_self,
    bool is_variadic,
    bool self_is_ref,
    bool self_is_mut)
    : ident(std::move(id)),
      type(ty),
      default_value(default_value),
      mode(mode),
      is_self(is_self),
      is_variadic(is_variadic),
      self_is_ref(self_is_ref),
      self_is_mut(self_is_mut) {}

InvokeArg::InvokeArg(ExprId v)
    : name(std::nullopt), value(v) {}

InvokeArg::InvokeArg()
    : name(std::nullopt), value(kInvalidAstId) {}

InvokeArg::InvokeArg(Ident n, ExprId v)
    : name(std::move(n)), value(v) {}

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

GlobalDecl::GlobalDecl(Ident n, TypeId t, ExprId v, bool mut, SourceSpan sp)
    : Decl(NodeKind::GlobalDecl, sp),
      name(std::move(n)),
      type(t),
      value(v),
      is_mut(mut) {}

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

TraitDecl::TraitDecl(
    std::vector<Attribute> a,
    Ident n,
    std::vector<Ident> tp,
    bool unsafe_trait,
    std::vector<std::pair<TypeId, TypeId>> wb,
    std::vector<DeclId> it,
    SourceSpan sp)
    : Decl(NodeKind::TraitDecl, sp),
      attrs(std::move(a)),
      name(std::move(n)),
      type_params(std::move(tp)),
      is_unsafe(unsafe_trait),
      where_bounds(std::move(wb)),
      items(std::move(it)) {}

ImplDecl::ImplDecl(
    std::vector<Attribute> a,
    TypeId trait_ty,
    TypeId self_ty,
    bool has_trait_in,
    std::vector<std::pair<TypeId, TypeId>> wb,
    std::vector<DeclId> it,
    SourceSpan sp)
    : Decl(NodeKind::ImplDecl, sp),
      attrs(std::move(a)),
      trait_type(trait_ty),
      self_type(self_ty),
      has_trait(has_trait_in),
      where_bounds(std::move(wb)),
      items(std::move(it)) {}

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
    bool async,
    bool extern_,
    bool signature_only,
    std::optional<std::string> extern_abi_in,
    std::vector<std::pair<TypeId, TypeId>> where_bounds_in,
    SourceSpan sp)
    : Decl(NodeKind::ProcDecl, sp),
      attrs(std::move(a)),
      name(std::move(n)),
      type_params(std::move(tp)),
      params(std::move(p)),
      return_type(rt),
      body(b),
      is_async(async),
      is_extern(extern_),
      is_signature_only(signature_only),
      extern_abi(std::move(extern_abi_in)),
      where_bounds(std::move(where_bounds_in)) {}

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
        case NodeKind::BuiltinExpr: return "BuiltinExpr";
        case NodeKind::ProcExpr: return "ProcExpr";
        case NodeKind::MemberExpr: return "MemberExpr";
        case NodeKind::IndexExpr: return "IndexExpr";
        case NodeKind::IfExpr: return "IfExpr";
        case NodeKind::IsExpr: return "IsExpr";
        case NodeKind::AsExpr: return "AsExpr";
        case NodeKind::CallNoParenExpr: return "CallNoParenExpr";
        case NodeKind::InvokeExpr: return "InvokeExpr";
        case NodeKind::ListExpr: return "ListExpr";
        case NodeKind::ListCompExpr: return "ListCompExpr";
        case NodeKind::IdentPattern: return "IdentPattern";
        case NodeKind::CtorPattern: return "CtorPattern";
        case NodeKind::WildcardPattern: return "WildcardPattern";
        case NodeKind::BlockStmt: return "BlockStmt";
        case NodeKind::AsmStmt: return "AsmStmt";
        case NodeKind::UnsafeStmt: return "UnsafeStmt";
        case NodeKind::LetStmt: return "LetStmt";
        case NodeKind::ExprStmt: return "ExprStmt";
        case NodeKind::ReturnStmt: return "ReturnStmt";
        case NodeKind::TryStmt: return "TryStmt";
        case NodeKind::RaiseStmt: return "RaiseStmt";
        case NodeKind::DeferStmt: return "DeferStmt";
        case NodeKind::IfStmt: return "IfStmt";
        case NodeKind::LoopStmt: return "LoopStmt";
        case NodeKind::BreakStmt: return "BreakStmt";
        case NodeKind::ContinueStmt: return "ContinueStmt";
        case NodeKind::ForStmt: return "ForStmt";
        case NodeKind::MakeStmt: return "MakeStmt";
        case NodeKind::SetStmt: return "SetStmt";
        case NodeKind::GiveStmt: return "GiveStmt";
        case NodeKind::EmitStmt: return "EmitStmt";
        case NodeKind::WithStmt: return "WithStmt";
        case NodeKind::CriticalStmt: return "CriticalStmt";
        case NodeKind::AtomicStmt: return "AtomicStmt";
        case NodeKind::VolatileStmt: return "VolatileStmt";
        case NodeKind::GotoStmt: return "GotoStmt";
        case NodeKind::PreemptStmt: return "PreemptStmt";
        case NodeKind::IrqStmt: return "IrqStmt";
        case NodeKind::LabelStmt: return "LabelStmt";
        case NodeKind::SelectStmt: return "SelectStmt";
        case NodeKind::WhenStmt: return "WhenStmt";
        case NodeKind::ComptimeDecl: return "ComptimeDecl";
        case NodeKind::FnDecl: return "FnDecl";
        case NodeKind::TypeDecl: return "TypeDecl";
        case NodeKind::TypeAliasDecl: return "TypeAliasDecl";
        case NodeKind::SpaceDecl: return "SpaceDecl";
        case NodeKind::PullDecl: return "PullDecl";
        case NodeKind::UseDecl: return "UseDecl";
        case NodeKind::ShareDecl: return "ShareDecl";
        case NodeKind::ConstDecl: return "ConstDecl";
        case NodeKind::GlobalDecl: return "GlobalDecl";
        case NodeKind::MacroDecl: return "MacroDecl";
        case NodeKind::FormDecl: return "FormDecl";
        case NodeKind::TraitDecl: return "TraitDecl";
        case NodeKind::ImplDecl: return "ImplDecl";
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

static void append_json_escaped(std::ostringstream& os, const std::string& text) {
    for (char c : text) {
        switch (c) {
            case '\\': os << "\\\\"; break;
            case '"': os << "\\\""; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default: os << c; break;
        }
    }
}

static void append_json_string(std::ostringstream& os, const std::string& text) {
    os << '"';
    append_json_escaped(os, text);
    os << '"';
}

static std::string unary_op_to_string(UnaryOp op) {
    switch (op) {
        case UnaryOp::Not: return "Not";
        case UnaryOp::Neg: return "Neg";
        case UnaryOp::BitNot: return "BitNot";
        case UnaryOp::Addr: return "Addr";
        case UnaryOp::Deref: return "Deref";
        case UnaryOp::Await: return "Await";
    }
    return "Unknown";
}

static std::string builtin_expr_kind_to_string(BuiltinExpr::Kind kind) {
    switch (kind) {
        case BuiltinExpr::Kind::Sizeof: return "Sizeof";
        case BuiltinExpr::Kind::Alignof: return "Alignof";
        case BuiltinExpr::Kind::Offsetof: return "Offsetof";
        case BuiltinExpr::Kind::Typeof: return "Typeof";
        case BuiltinExpr::Kind::Nameof: return "Nameof";
        case BuiltinExpr::Kind::Await: return "Await";
    }
    return "Unknown";
}

static std::string binary_op_to_string(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add: return "Add";
        case BinaryOp::Sub: return "Sub";
        case BinaryOp::Mul: return "Mul";
        case BinaryOp::Div: return "Div";
        case BinaryOp::Mod: return "Mod";
        case BinaryOp::Eq: return "Eq";
        case BinaryOp::Ne: return "Ne";
        case BinaryOp::Lt: return "Lt";
        case BinaryOp::Le: return "Le";
        case BinaryOp::Gt: return "Gt";
        case BinaryOp::Ge: return "Ge";
        case BinaryOp::BitAnd: return "BitAnd";
        case BinaryOp::BitOr: return "BitOr";
        case BinaryOp::BitXor: return "BitXor";
        case BinaryOp::Shl: return "Shl";
        case BinaryOp::Shr: return "Shr";
        case BinaryOp::And: return "And";
        case BinaryOp::Or: return "Or";
        case BinaryOp::Assign: return "Assign";
    }
    return "Unknown";
}

static std::string literal_kind_to_string(LiteralKind kind) {
    switch (kind) {
        case LiteralKind::Bool: return "Bool";
        case LiteralKind::Int: return "Int";
        case LiteralKind::String: return "String";
        case LiteralKind::Bytes: return "Bytes";
        case LiteralKind::Float: return "Float";
        case LiteralKind::Char: return "Char";
    }
    return "Unknown";
}

static std::string invoke_kind_to_string(InvokeExpr::Kind kind) {
    switch (kind) {
        case InvokeExpr::Kind::Unknown: return "Unknown";
        case InvokeExpr::Kind::Call: return "Call";
        case InvokeExpr::Kind::Ctor: return "Ctor";
    }
    return "Unknown";
}

static std::string module_path_to_string(const ModulePath& path) {
    std::string out;
    for (std::size_t i = 0; i < path.relative_depth; ++i) {
        out += ".";
    }
    for (std::size_t i = 0; i < path.parts.size(); ++i) {
        if (!out.empty() && out.back() != '.') {
            out += "/";
        }
        out += path.parts[i].name;
    }
    return out;
}

static const char* param_mode_to_string(std::optional<ParamMode> mode) {
    if (!mode.has_value()) {
        return "";
    }
    switch (*mode) {
        case ParamMode::Mut: return "mut";
        case ParamMode::Owned: return "owned";
        case ParamMode::Borrow: return "borrow";
        case ParamMode::Move: return "move";
    }
    return "";
}

static void append_int_list(std::ostringstream& os, const std::vector<AstId>& values) {
    os << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) os << ",";
        os << values[i];
    }
    os << "]";
}

static void append_string_list(std::ostringstream& os, const std::vector<std::string>& values) {
    os << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) os << ",";
        append_json_string(os, values[i]);
    }
    os << "]";
}

static void append_node_json(const AstContext& ctx,
                             AstId id,
                             std::ostringstream& os,
                             std::unordered_set<AstId>& stack);

static void append_children_json(const AstContext& ctx,
                                 const std::vector<AstId>& children,
                                 std::ostringstream& os,
                                 std::unordered_set<AstId>& stack) {
    os << "[";
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (i) os << ",";
        append_node_json(ctx, children[i], os, stack);
    }
    os << "]";
}

static void append_node_json(const AstContext& ctx,
                             AstId id,
                             std::ostringstream& os,
                             std::unordered_set<AstId>& stack) {
    if (id == kInvalidAstId) {
        os << "null";
        return;
    }
    if (stack.count(id)) {
        os << "{\"id\":" << id << ",\"cycle\":true}";
        return;
    }

    stack.insert(id);
    const AstNode& node = ctx.node(id);
    std::vector<AstId> children;
    std::vector<std::string> names;

    os << "{";
    os << "\"id\":" << id << ",";
    os << "\"kind\":";
    append_json_string(os, to_string(node.kind));
    os << ",\"span\":{\"start\":" << node.span.start << ",\"end\":" << node.span.end << "},\"data\":{";

    switch (node.kind) {
        case NodeKind::Module: {
            const auto& n = ctx.get<Module>(id);
            os << "\"name\":";
            append_json_string(os, n.name);
            os << ",\"decl_count\":" << n.decls.size();
            children = n.decls;
            break;
        }
        case NodeKind::Ident: {
            const auto& n = ctx.get<Ident>(id);
            os << "\"name\":";
            append_json_string(os, n.name);
            break;
        }
        case NodeKind::NamedType: {
            const auto& n = ctx.get<NamedType>(id);
            os << "\"name\":";
            append_json_string(os, n.ident.name);
            break;
        }
        case NodeKind::GenericType: {
            const auto& n = ctx.get<GenericType>(id);
            os << "\"base\":";
            append_json_string(os, n.base_ident.name);
            os << ",\"type_arg_count\":" << n.type_args.size();
            children = n.type_args;
            break;
        }
        case NodeKind::BuiltinType: {
            const auto& n = ctx.get<BuiltinType>(id);
            os << "\"name\":";
            append_json_string(os, n.name);
            break;
        }
        case NodeKind::PointerType: {
            const auto& n = ctx.get<PointerType>(id);
            children.push_back(n.pointee);
            break;
        }
        case NodeKind::SliceType: {
            const auto& n = ctx.get<SliceType>(id);
            children.push_back(n.element);
            break;
        }
        case NodeKind::ProcType: {
            const auto& n = ctx.get<ProcType>(id);
            os << "\"param_count\":" << n.params.size();
            children = n.params;
            children.push_back(n.return_type);
            break;
        }
        case NodeKind::LiteralExpr: {
            const auto& n = ctx.get<LiteralExpr>(id);
            os << "\"literal_kind\":";
            append_json_string(os, literal_kind_to_string(n.lit_kind));
            os << ",\"value\":";
            append_json_string(os, n.value);
            break;
        }
        case NodeKind::IdentExpr: {
            const auto& n = ctx.get<IdentExpr>(id);
            os << "\"name\":";
            append_json_string(os, n.ident.name);
            break;
        }
        case NodeKind::UnaryExpr: {
            const auto& n = ctx.get<UnaryExpr>(id);
            os << "\"op\":";
            append_json_string(os, unary_op_to_string(n.op));
            children.push_back(n.expr);
            break;
        }
        case NodeKind::BuiltinExpr: {
            const auto& n = ctx.get<BuiltinExpr>(id);
            os << "\"kind\":";
            append_json_string(os, builtin_expr_kind_to_string(n.kind));
            if (n.type != kInvalidAstId) {
                children.push_back(n.type);
            }
            if (n.expr != kInvalidAstId) {
                children.push_back(n.expr);
            }
            if (!n.path.parts.empty()) {
                os << ",\"path\":";
                append_json_string(os, module_path_to_string(n.path));
            }
            if (!n.member.name.empty()) {
                os << ",\"member\":";
                append_json_string(os, n.member.name);
            }
            break;
        }
        case NodeKind::BinaryExpr: {
            const auto& n = ctx.get<BinaryExpr>(id);
            os << "\"op\":";
            append_json_string(os, binary_op_to_string(n.op));
            children.push_back(n.lhs);
            children.push_back(n.rhs);
            break;
        }
        case NodeKind::ProcExpr: {
            const auto& n = ctx.get<ProcExpr>(id);
            os << "\"param_count\":" << n.params.size();
            std::vector<std::string> modes;
            std::vector<std::string> self_flags;
            std::vector<std::string> variadic_flags;
            for (const auto& p : n.params) {
                if (p.type != kInvalidAstId) {
                    children.push_back(p.type);
                }
                names.push_back(p.ident.name);
                children.push_back(p.default_value);
                modes.push_back(param_mode_to_string(p.mode));
                self_flags.push_back(p.is_self ? "true" : "false");
                variadic_flags.push_back(p.is_variadic ? "true" : "false");
            }
            os << ",\"params\":";
            append_string_list(os, names);
            os << ",\"param_modes\":";
            append_string_list(os, modes);
            os << ",\"param_self_flags\":";
            append_string_list(os, self_flags);
            os << ",\"param_variadic_flags\":";
            append_string_list(os, variadic_flags);
            children.push_back(n.return_type);
            children.push_back(n.body);
            break;
        }
        case NodeKind::MemberExpr: {
            const auto& n = ctx.get<MemberExpr>(id);
            os << "\"member\":";
            append_json_string(os, n.member.name);
            children.push_back(n.base);
            break;
        }
        case NodeKind::IndexExpr: {
            const auto& n = ctx.get<IndexExpr>(id);
            children.push_back(n.base);
            children.push_back(n.index);
            break;
        }
        case NodeKind::IfExpr: {
            const auto& n = ctx.get<IfExpr>(id);
            children.push_back(n.cond);
            children.push_back(n.then_block);
            children.push_back(n.else_block);
            break;
        }
        case NodeKind::IsExpr: {
            const auto& n = ctx.get<IsExpr>(id);
            children.push_back(n.value);
            children.push_back(n.pattern);
            break;
        }
        case NodeKind::AsExpr: {
            const auto& n = ctx.get<AsExpr>(id);
            children.push_back(n.value);
            children.push_back(n.type);
            break;
        }
        case NodeKind::CallNoParenExpr: {
            const auto& n = ctx.get<CallNoParenExpr>(id);
            os << "\"callee\":";
            append_json_string(os, n.callee.name);
            children.push_back(n.arg);
            break;
        }
        case NodeKind::InvokeExpr: {
            const auto& n = ctx.get<InvokeExpr>(id);
            os << "\"invoke_kind\":";
            append_json_string(os, invoke_kind_to_string(n.invoke_kind));
            os << ",\"type_arg_count\":" << n.type_args.size() << ",\"arg_count\":" << n.args.size();
            children.push_back(n.callee_expr);
            if (n.callee_type != kInvalidAstId) {
                children.push_back(n.callee_type);
            }
            children.insert(children.end(), n.type_args.begin(), n.type_args.end());
            for (const auto& arg : n.args) {
                names.push_back(arg.name.has_value() ? arg.name->name : "");
                children.push_back(arg.value);
            }
            os << ",\"arg_names\":";
            append_string_list(os, names);
            break;
        }
        case NodeKind::ListExpr: {
            const auto& n = ctx.get<ListExpr>(id);
            os << "\"item_count\":" << n.items.size();
            children = n.items;
            break;
        }
        case NodeKind::ListCompExpr: {
            const auto& n = ctx.get<ListCompExpr>(id);
            os << "\"kind\":";
            append_json_string(os, n.kind == ListCompExpr::Kind::List
                ? "List"
                : (n.kind == ListCompExpr::Kind::Set ? "Set" : "Dict"));
            os << ",\"ident\":";
            append_json_string(os, n.ident.name);
            if (n.index_ident.has_value()) {
                os << ",\"index_ident\":";
                append_json_string(os, n.index_ident->name);
            }
            if (n.key != kInvalidAstId) {
                children.push_back(n.key);
            }
            children.push_back(n.value);
            children.push_back(n.iterable);
            children.push_back(n.condition);
            break;
        }
        case NodeKind::IdentPattern: {
            const auto& n = ctx.get<IdentPattern>(id);
            os << "\"name\":";
            append_json_string(os, n.ident.name);
            break;
        }
        case NodeKind::CtorPattern: {
            const auto& n = ctx.get<CtorPattern>(id);
            children.push_back(n.type);
            children.insert(children.end(), n.args.begin(), n.args.end());
            break;
        }
        case NodeKind::WildcardPattern:
            break;
        case NodeKind::BlockStmt: {
            const auto& n = ctx.get<BlockStmt>(id);
            os << "\"stmt_count\":" << n.stmts.size();
            children = n.stmts;
            break;
        }
        case NodeKind::AsmStmt: {
            const auto& n = ctx.get<AsmStmt>(id);
            os << "\"code\":";
            append_json_string(os, n.code);
            break;
        }
        case NodeKind::UnsafeStmt: {
            const auto& n = ctx.get<UnsafeStmt>(id);
            children.push_back(n.body);
            break;
        }
        case NodeKind::LetStmt: {
            const auto& n = ctx.get<LetStmt>(id);
            os << "\"is_destructuring\":" << (n.is_destructuring ? "true" : "false");
            os << ",\"is_mutable\":" << (n.is_mutable ? "true" : "false");
            if (n.is_destructuring) {
                std::function<void(const LetBinding&)> dump_binding = [&](const LetBinding& binding) {
                    os << "{";
                    os << "\"name\":";
                    append_json_string(os, binding.ident.name);
                    if (!binding.children.empty()) {
                        os << ",\"children\":[";
                        for (std::size_t i = 0; i < binding.children.size(); ++i) {
                            if (i) os << ",";
                            dump_binding(binding.children[i]);
                        }
                        os << "]";
                    }
                    os << "}";
                };
                os << ",\"bindings\":[";
                for (std::size_t i = 0; i < n.bindings.size(); ++i) {
                    if (i) os << ",";
                    dump_binding(n.bindings[i]);
                }
                os << "]";
            } else {
                os << ",\"ident\":";
                append_json_string(os, n.ident.name);
            }
            if (n.type != kInvalidAstId) {
                children.push_back(n.type);
            }
            children.push_back(n.initializer);
            break;
        }
        case NodeKind::ExprStmt: {
            const auto& n = ctx.get<ExprStmt>(id);
            children.push_back(n.expr);
            break;
        }
        case NodeKind::ReturnStmt: {
            const auto& n = ctx.get<ReturnStmt>(id);
            if (n.expr != kInvalidAstId) {
                children.push_back(n.expr);
            }
            break;
        }
        case NodeKind::TryStmt: {
            const auto& n = ctx.get<TryStmt>(id);
            children.push_back(n.body);
            children.push_back(n.except_body);
            children.push_back(n.finally_body);
            break;
        }
        case NodeKind::RaiseStmt: {
            const auto& n = ctx.get<RaiseStmt>(id);
            children.push_back(n.expr);
            break;
        }
        case NodeKind::DeferStmt: {
            const auto& n = ctx.get<DeferStmt>(id);
            children.push_back(n.body);
            break;
        }
        case NodeKind::IfStmt: {
            const auto& n = ctx.get<IfStmt>(id);
            children.push_back(n.cond);
            children.push_back(n.then_block);
            children.push_back(n.else_block);
            break;
        }
        case NodeKind::LoopStmt: {
            const auto& n = ctx.get<LoopStmt>(id);
            children.push_back(n.body);
            break;
        }
        case NodeKind::BreakStmt:
        case NodeKind::ContinueStmt:
            break;
        case NodeKind::ForStmt: {
            const auto& n = ctx.get<ForStmt>(id);
            if (n.index_ident.has_value()) {
                os << "\"index_ident\":";
                append_json_string(os, n.index_ident->name);
            }
            os << ",\"tuple_destructure\":" << (n.tuple_destructure ? "true" : "false");
            os << ",\"ident\":";
            append_json_string(os, n.ident.name);
            children.push_back(n.iterable);
            children.push_back(n.body);
            break;
        }
        case NodeKind::MakeStmt: {
            const auto& n = ctx.get<MakeStmt>(id);
            os << "\"ident\":";
            append_json_string(os, n.ident.name);
            if (n.type != kInvalidAstId) {
                children.push_back(n.type);
            }
            children.push_back(n.value);
            break;
        }
        case NodeKind::SetStmt: {
            const auto& n = ctx.get<SetStmt>(id);
            children.push_back(n.target);
            children.push_back(n.value);
            break;
        }
        case NodeKind::GiveStmt: {
            const auto& n = ctx.get<GiveStmt>(id);
            children.push_back(n.value);
            break;
        }
        case NodeKind::EmitStmt: {
            const auto& n = ctx.get<EmitStmt>(id);
            children.push_back(n.value);
            break;
        }
        case NodeKind::WithStmt: {
            const auto& n = ctx.get<WithStmt>(id);
            children.push_back(n.expr);
            if (n.pattern.has_value()) {
                children.push_back(*n.pattern);
            }
            children.push_back(n.body);
            break;
        }
        case NodeKind::CriticalStmt: {
            const auto& n = ctx.get<CriticalStmt>(id);
            children.push_back(n.body);
            break;
        }
        case NodeKind::AtomicStmt: {
            const auto& n = ctx.get<AtomicStmt>(id);
            children.push_back(n.body);
            break;
        }
        case NodeKind::VolatileStmt: {
            const auto& n = ctx.get<VolatileStmt>(id);
            children.push_back(n.body);
            break;
        }
        case NodeKind::GotoStmt: {
            const auto& n = ctx.get<GotoStmt>(id);
            os << "\"target\":";
            append_json_string(os, n.target.name);
            break;
        }
        case NodeKind::PreemptStmt: {
            const auto& n = ctx.get<PreemptStmt>(id);
            os << "\"enabled\":" << (n.enabled ? "true" : "false");
            break;
        }
        case NodeKind::IrqStmt: {
            const auto& n = ctx.get<IrqStmt>(id);
            os << "\"enabled\":" << (n.enabled ? "true" : "false");
            break;
        }
        case NodeKind::LabelStmt: {
            const auto& n = ctx.get<LabelStmt>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            break;
        }
        case NodeKind::WhenStmt: {
            const auto& n = ctx.get<WhenStmt>(id);
            children.push_back(n.pattern);
            if (n.guard != kInvalidAstId) {
                children.push_back(n.guard);
            }
            children.push_back(n.block);
            break;
        }
        case NodeKind::SelectStmt: {
            const auto& n = ctx.get<SelectStmt>(id);
            os << "\"when_count\":" << n.whens.size();
            children.push_back(n.expr);
            children.insert(children.end(), n.whens.begin(), n.whens.end());
            if (n.otherwise_block != kInvalidAstId) {
                children.push_back(n.otherwise_block);
            }
            break;
        }
        case NodeKind::ComptimeDecl: {
            const auto& n = ctx.get<ComptimeDecl>(id);
            children.push_back(n.body);
            break;
        }
        case NodeKind::FnDecl: {
            const auto& n = ctx.get<FnDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            os << ",\"param_count\":" << n.params.size();
            for (const auto& p : n.params) {
                names.push_back(p.ident.name);
                if (p.type != kInvalidAstId) {
                    children.push_back(p.type);
                }
                children.push_back(p.default_value);
            }
            os << ",\"params\":";
            append_string_list(os, names);
            children.push_back(n.return_type);
            children.push_back(n.body);
            break;
        }
        case NodeKind::TypeDecl: {
            const auto& n = ctx.get<TypeDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            os << ",\"field_count\":" << n.fields.size();
            for (const auto& f : n.fields) {
                names.push_back(f.ident.name);
                if (f.type != kInvalidAstId) {
                    children.push_back(f.type);
                }
            }
            os << ",\"fields\":";
            append_string_list(os, names);
            break;
        }
        case NodeKind::TypeAliasDecl: {
            const auto& n = ctx.get<TypeAliasDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            for (const auto& p : n.type_params) {
                names.push_back(p.name);
            }
            os << ",\"type_params\":";
            append_string_list(os, names);
            children.push_back(n.target);
            break;
        }
        case NodeKind::SpaceDecl: {
            const auto& n = ctx.get<SpaceDecl>(id);
            os << "\"path\":";
            append_json_string(os, module_path_to_string(n.path));
            break;
        }
        case NodeKind::PullDecl: {
            const auto& n = ctx.get<PullDecl>(id);
            os << "\"path\":";
            append_json_string(os, module_path_to_string(n.path));
            os << ",\"alias\":";
            if (n.alias.has_value()) {
                append_json_string(os, n.alias->name);
            } else {
                os << "null";
            }
            break;
        }
        case NodeKind::UseDecl: {
            const auto& n = ctx.get<UseDecl>(id);
            os << "\"path\":";
            append_json_string(os, module_path_to_string(n.path));
            os << ",\"is_glob\":" << (n.is_glob ? "true" : "false") << ",\"alias\":";
            if (n.alias.has_value()) {
                append_json_string(os, n.alias->name);
            } else {
                os << "null";
            }
            break;
        }
        case NodeKind::ShareDecl: {
            const auto& n = ctx.get<ShareDecl>(id);
            os << "\"share_all\":" << (n.share_all ? "true" : "false");
            for (const auto& p : n.names) {
                names.push_back(p.name);
            }
            os << ",\"names\":";
            append_string_list(os, names);
            break;
        }
        case NodeKind::ConstDecl: {
            const auto& n = ctx.get<ConstDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            if (n.type != kInvalidAstId) {
                children.push_back(n.type);
            }
            children.push_back(n.value);
            break;
        }
        case NodeKind::GlobalDecl: {
            const auto& n = ctx.get<GlobalDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            os << ",\"is_mut\":" << (n.is_mut ? "true" : "false");
            if (n.type != kInvalidAstId) {
                children.push_back(n.type);
            }
            children.push_back(n.value);
            break;
        }
        case NodeKind::MacroDecl: {
            const auto& n = ctx.get<MacroDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            for (const auto& p : n.params) {
                names.push_back(p.name);
            }
            os << ",\"params\":";
            append_string_list(os, names);
            children.push_back(n.body);
            break;
        }
        case NodeKind::FormDecl: {
            const auto& n = ctx.get<FormDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            for (const auto& p : n.type_params) {
                names.push_back(p.name);
            }
            os << ",\"type_params\":";
            append_string_list(os, names);
            names.clear();
            for (const auto& f : n.fields) {
                names.push_back(f.ident.name);
                if (f.type != kInvalidAstId) {
                    children.push_back(f.type);
                }
            }
            os << ",\"fields\":";
            append_string_list(os, names);
            break;
        }
        case NodeKind::TraitDecl: {
            const auto& n = ctx.get<TraitDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            os << ",\"is_unsafe\":" << (n.is_unsafe ? "true" : "false")
               << ",\"attr_count\":" << n.attrs.size()
               << ",\"where_bound_count\":" << n.where_bounds.size()
               << ",\"item_count\":" << n.items.size();
            for (const auto& p : n.type_params) {
                names.push_back(p.name);
            }
            os << ",\"type_params\":";
            append_string_list(os, names);
            names.clear();
            for (auto item : n.items) {
                children.push_back(item);
            }
            for (const auto& wb : n.where_bounds) {
                children.push_back(wb.first);
                children.push_back(wb.second);
            }
            break;
        }
        case NodeKind::ImplDecl: {
            const auto& n = ctx.get<ImplDecl>(id);
            os << "\"attr_count\":" << n.attrs.size()
               << ",\"has_trait\":" << (n.has_trait ? "true" : "false")
               << ",\"where_bound_count\":" << n.where_bounds.size()
               << ",\"item_count\":" << n.items.size();
            if (n.trait_type != kInvalidAstId) {
                children.push_back(n.trait_type);
            }
            if (n.self_type != kInvalidAstId) {
                children.push_back(n.self_type);
            }
            for (auto item : n.items) {
                children.push_back(item);
            }
            for (const auto& wb : n.where_bounds) {
                children.push_back(wb.first);
                children.push_back(wb.second);
            }
            break;
        }
        case NodeKind::PickDecl: {
            const auto& n = ctx.get<PickDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            for (const auto& p : n.type_params) {
                names.push_back(p.name);
            }
            os << ",\"type_params\":";
            append_string_list(os, names);
            names.clear();
            for (const auto& c : n.cases) {
                names.push_back(c.ident.name);
                for (const auto& f : c.fields) {
                    if (f.type != kInvalidAstId) {
                        children.push_back(f.type);
                    }
                }
            }
            os << ",\"cases\":";
            append_string_list(os, names);
            break;
        }
        case NodeKind::ProcDecl: {
            const auto& n = ctx.get<ProcDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            os << ",\"attr_count\":" << n.attrs.size()
               << ",\"param_count\":" << n.params.size()
               << ",\"is_async\":" << (n.is_async ? "true" : "false")
               << ",\"is_extern\":" << (n.is_extern ? "true" : "false")
               << ",\"is_signature_only\":" << (n.is_signature_only ? "true" : "false");
            if (n.extern_abi.has_value()) {
                os << ",\"extern_abi\":";
                append_json_string(os, *n.extern_abi);
            }
            os << ",\"where_bound_count\":" << n.where_bounds.size();
            std::vector<std::string> modes;
            std::vector<std::string> self_flags;
            std::vector<std::string> self_ref_flags;
            std::vector<std::string> self_mut_flags;
            std::vector<std::string> variadic_flags;
            for (const auto& p : n.type_params) {
                names.push_back(p.name);
            }
            os << ",\"type_params\":";
            append_string_list(os, names);
            names.clear();
            for (const auto& p : n.params) {
                names.push_back(p.ident.name);
                if (p.type != kInvalidAstId) {
                    children.push_back(p.type);
                }
                children.push_back(p.default_value);
                modes.push_back(param_mode_to_string(p.mode));
                self_flags.push_back(p.is_self ? "true" : "false");
                self_ref_flags.push_back(p.self_is_ref ? "true" : "false");
                self_mut_flags.push_back(p.self_is_mut ? "true" : "false");
                variadic_flags.push_back(p.is_variadic ? "true" : "false");
            }
            os << ",\"params\":";
            append_string_list(os, names);
            os << ",\"param_modes\":";
            append_string_list(os, modes);
            os << ",\"param_self_flags\":";
            append_string_list(os, self_flags);
            os << ",\"param_self_ref_flags\":";
            append_string_list(os, self_ref_flags);
            os << ",\"param_self_mut_flags\":";
            append_string_list(os, self_mut_flags);
            os << ",\"param_variadic_flags\":";
            append_string_list(os, variadic_flags);
            children.push_back(n.return_type);
            if (n.body != kInvalidAstId) {
                children.push_back(n.body);
            }
            for (const auto& wb : n.where_bounds) {
                children.push_back(wb.first);
                children.push_back(wb.second);
            }
            break;
        }
        case NodeKind::EntryDecl: {
            const auto& n = ctx.get<EntryDecl>(id);
            os << "\"name\":";
            append_json_string(os, n.name.name);
            os << ",\"module\":";
            append_json_string(os, module_path_to_string(n.module));
            children.push_back(n.body);
            break;
        }
        case NodeKind::Attribute:
        case NodeKind::ModulePath:
            os << "\"raw\":";
            append_json_string(os, to_string(node.kind));
            break;
    }

    os << "},\"child_ids\":";
    append_int_list(os, children);
    os << ",\"children\":";
    append_children_json(ctx, children, os, stack);
    os << "}";

    stack.erase(id);
}

static std::string pretty_json_compact(std::string_view in) {
    std::string out;
    out.reserve(in.size() + in.size() / 3);
    int indent = 0;
    bool in_string = false;
    bool escape = false;
    auto newline_indent = [&](int n) {
        out.push_back('\n');
        out.append(static_cast<std::size_t>(n * 2), ' ');
    };
    for (char c : in) {
        if (in_string) {
            out.push_back(c);
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }
        switch (c) {
            case '"':
                in_string = true;
                out.push_back(c);
                break;
            case '{':
            case '[':
                out.push_back(c);
                indent++;
                newline_indent(indent);
                break;
            case '}':
            case ']':
                indent = std::max(0, indent - 1);
                newline_indent(indent);
                out.push_back(c);
                break;
            case ',':
                out.push_back(c);
                newline_indent(indent);
                break;
            case ':':
                out.push_back(c);
                out.push_back(' ');
                break;
            default:
                if (c != '\n' && c != '\r' && c != '\t' && c != ' ') {
                    out.push_back(c);
                }
                break;
        }
    }
    out.push_back('\n');
    return out;
}

std::string dump_json_to_string(const AstContext& ast_ctx, AstId root_id, bool pretty) {
    std::ostringstream os;
    std::unordered_set<AstId> stack;
    append_node_json(ast_ctx, root_id, os, stack);
    const std::string compact = os.str();
    if (!pretty) {
        return compact;
    }
    return pretty_json_compact(compact);
}

} // namespace vitte::frontend::ast
