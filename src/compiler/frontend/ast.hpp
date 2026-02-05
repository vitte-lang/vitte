// ============================================================
// ast.hpp — Vitte Compiler
// Frontend AST definitions
// ============================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../support/arena.hpp"

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
struct Pattern;
struct FnParam;

// ------------------------------------------------------------
// Handle aliases
// ------------------------------------------------------------

using AstId = std::uint32_t;
static constexpr AstId kInvalidAstId = static_cast<AstId>(-1);

using TypeId = AstId;
using ExprId = AstId;
using StmtId = AstId;
using DeclId = AstId;
using PatternId = AstId;
using ModuleId = AstId;

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

    // attributes
    Attribute,

    // module path
    ModulePath,

    // types (legacy + Vitte 1.0)
    NamedType,
    GenericType,
    BuiltinType,
    PointerType,
    SliceType,
    ProcType,

    // expressions
    LiteralExpr,
    IdentExpr,
    UnaryExpr,
    BinaryExpr,
    MemberExpr,
    IndexExpr,
    ProcExpr,
    IfExpr,
    IsExpr,
    AsExpr,
    CallNoParenExpr,
    InvokeExpr,
    ListExpr,

    // patterns
    IdentPattern,
    CtorPattern,

    // statements
    BlockStmt,
    LetStmt,
    ExprStmt,
    ReturnStmt,
    IfStmt,
    LoopStmt,
    BreakStmt,
    ContinueStmt,
    ForStmt,
    MakeStmt,
    SetStmt,
    GiveStmt,
    EmitStmt,
    SelectStmt,
    WhenStmt,

    // declarations (legacy + Vitte 1.0)
    FnDecl,
    TypeDecl,
    TypeAliasDecl,
    SpaceDecl,
    PullDecl,
    UseDecl,
    ShareDecl,
    ConstDecl,
    MacroDecl,
    FormDecl,
    PickDecl,
    ProcDecl,
    EntryDecl,
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
// Attribute
// ------------------------------------------------------------

struct Attribute : AstNode {
    Ident name;

    explicit Attribute(Ident name, SourceSpan span);
};

// ------------------------------------------------------------
// Module path
// ------------------------------------------------------------

struct ModulePath : AstNode {
    std::vector<Ident> parts;
    std::size_t relative_depth;

    explicit ModulePath(std::vector<Ident> parts, std::size_t relative_depth, SourceSpan span);
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
    std::vector<TypeId> type_args;

    GenericType(
        Ident base_ident,
        std::vector<TypeId> type_args,
        SourceSpan span);
};

struct BuiltinType : TypeNode {
    std::string name;

    BuiltinType(std::string name, SourceSpan span);
};

// ------------------------------------------------------------
// Function parameters
// ------------------------------------------------------------

struct FnParam {
    Ident ident;
    TypeId type;

    FnParam(Ident ident, TypeId type);
};

struct PointerType : TypeNode {
    TypeId pointee;

    PointerType(TypeId pointee, SourceSpan span);
};

struct SliceType : TypeNode {
    TypeId element;

    SliceType(TypeId element, SourceSpan span);
};

struct ProcType : TypeNode {
    std::vector<TypeId> params;
    TypeId return_type;

    ProcType(std::vector<TypeId> params, TypeId return_type, SourceSpan span);
};

// ------------------------------------------------------------
// Expressions
// ------------------------------------------------------------

enum class LiteralKind {
    Bool,
    Int,
    String,
    Float,
    Char,
};

enum class UnaryOp {
    Not,
    Neg,
    Addr,
    Deref,
};

enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    BitAnd,
    BitOr,
    BitXor,
    Shl,
    Shr,
    And,
    Or,
    Assign,
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
    ExprId expr;

    UnaryExpr(UnaryOp op, ExprId expr, SourceSpan span);
};

struct BinaryExpr : Expr {
    BinaryOp op;
    ExprId lhs;
    ExprId rhs;

    BinaryExpr(BinaryOp op, ExprId lhs, ExprId rhs, SourceSpan span);
};

struct ProcExpr : Expr {
    std::vector<FnParam> params;
    TypeId return_type;
    StmtId body;

    ProcExpr(std::vector<FnParam> params, TypeId return_type, StmtId body, SourceSpan span);
};

struct MemberExpr : Expr {
    ExprId base;
    Ident member;

    MemberExpr(ExprId base, Ident member, SourceSpan span);
};

struct IndexExpr : Expr {
    ExprId base;
    ExprId index;

    IndexExpr(ExprId base, ExprId index, SourceSpan span);
};

struct IfExpr : Expr {
    ExprId cond;
    StmtId then_block;
    StmtId else_block;

    IfExpr(ExprId cond, StmtId then_block, StmtId else_block, SourceSpan span);
};

struct IsExpr : Expr {
    ExprId value;
    PatternId pattern;

    IsExpr(ExprId value, PatternId pattern, SourceSpan span);
};

struct AsExpr : Expr {
    ExprId value;
    TypeId type;

    AsExpr(ExprId value, TypeId type, SourceSpan span);
};

struct CallNoParenExpr : Expr {
    Ident callee;
    ExprId arg;

    CallNoParenExpr(Ident callee, ExprId arg, SourceSpan span);
};

struct InvokeExpr : Expr {
    enum class Kind {
        Unknown,
        Call,
        Ctor
    };

    ExprId callee_expr;
    TypeId callee_type;
    std::vector<ExprId> args;
    Kind invoke_kind = Kind::Unknown;

    InvokeExpr(
        ExprId callee_expr,
        TypeId callee_type,
        std::vector<ExprId> args,
        SourceSpan span);
};

struct ListExpr : Expr {
    std::vector<ExprId> items;

    explicit ListExpr(std::vector<ExprId> items, SourceSpan span);
};

// ------------------------------------------------------------
// Patterns
// ------------------------------------------------------------

struct Pattern : AstNode {
    explicit Pattern(NodeKind kind, SourceSpan span);
};

struct IdentPattern : Pattern {
    Ident ident;

    IdentPattern(Ident ident, SourceSpan span);
};

struct CtorPattern : Pattern {
    TypeId type;
    std::vector<PatternId> args;

    CtorPattern(TypeId type, std::vector<PatternId> args, SourceSpan span);
};

// ------------------------------------------------------------
// Statements
// ------------------------------------------------------------

struct Stmt : AstNode {
    explicit Stmt(NodeKind kind, SourceSpan span);
};

struct LetStmt : Stmt {
    Ident ident;
    TypeId type;
    ExprId initializer;

    LetStmt(
        Ident ident,
        TypeId type,
        ExprId initializer,
        SourceSpan span);
};

struct MakeStmt : Stmt {
    Ident ident;
    TypeId type;
    ExprId value;

    MakeStmt(Ident ident, TypeId type, ExprId value, SourceSpan span);
};

struct SetStmt : Stmt {
    Ident ident;
    ExprId value;

    SetStmt(Ident ident, ExprId value, SourceSpan span);
};

struct GiveStmt : Stmt {
    ExprId value;

    GiveStmt(ExprId value, SourceSpan span);
};

struct EmitStmt : Stmt {
    ExprId value;

    EmitStmt(ExprId value, SourceSpan span);
};

struct ExprStmt : Stmt {
    ExprId expr;

    ExprStmt(ExprId expr, SourceSpan span);
};

struct ReturnStmt : Stmt {
    ExprId expr;

    explicit ReturnStmt(ExprId expr, SourceSpan span);
};

struct BlockStmt : Stmt {
    std::vector<StmtId> stmts;

    explicit BlockStmt(std::vector<StmtId> stmts, SourceSpan span);
};

struct IfStmt : Stmt {
    ExprId cond;
    StmtId then_block;
    StmtId else_block;

    IfStmt(
        ExprId cond,
        StmtId then_block,
        StmtId else_block,
        SourceSpan span);
};

struct LoopStmt : Stmt {
    StmtId body;

    LoopStmt(StmtId body, SourceSpan span);
};

struct BreakStmt : Stmt {
    explicit BreakStmt(SourceSpan span);
};

struct ContinueStmt : Stmt {
    explicit ContinueStmt(SourceSpan span);
};

struct ForStmt : Stmt {
    Ident ident;
    ExprId iterable;
    StmtId body;

    ForStmt(Ident ident, ExprId iterable, StmtId body, SourceSpan span);
};

struct WhenStmt : Stmt {
    PatternId pattern;
    StmtId block;

    WhenStmt(PatternId pattern, StmtId block, SourceSpan span);
};

struct SelectStmt : Stmt {
    ExprId expr;
    std::vector<StmtId> whens;
    StmtId otherwise_block;

    SelectStmt(
        ExprId expr,
        std::vector<StmtId> whens,
        StmtId otherwise_block,
        SourceSpan span);
};

// ------------------------------------------------------------
// Declarations
// ------------------------------------------------------------

struct FieldDecl {
    Ident ident;
    TypeId type;

    FieldDecl(Ident ident, TypeId type);
};

struct CaseField {
    Ident ident;
    TypeId type;

    CaseField(Ident ident, TypeId type);
};

struct CaseDecl {
    Ident ident;
    std::vector<CaseField> fields;

    CaseDecl(Ident ident, std::vector<CaseField> fields);
};

struct Decl : AstNode {
    explicit Decl(NodeKind kind, SourceSpan span);
};

struct FnDecl : Decl {
    Ident name;
    std::vector<FnParam> params;
    TypeId return_type;
    StmtId body;

    FnDecl(
        Ident name,
        std::vector<FnParam> params,
        TypeId return_type,
        StmtId body,
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

struct TypeAliasDecl : Decl {
    Ident name;
    std::vector<Ident> type_params;
    TypeId target;

    TypeAliasDecl(Ident name, std::vector<Ident> type_params, TypeId target, SourceSpan span);
};

struct SpaceDecl : Decl {
    ModulePath path;

    SpaceDecl(ModulePath path, SourceSpan span);
};

struct PullDecl : Decl {
    ModulePath path;
    std::optional<Ident> alias;

    PullDecl(ModulePath path, std::optional<Ident> alias, SourceSpan span);
};

struct UseDecl : Decl {
    ModulePath path;
    std::optional<Ident> alias;
    bool is_glob;

    UseDecl(ModulePath path, std::optional<Ident> alias, bool is_glob, SourceSpan span);
};

struct ShareDecl : Decl {
    bool share_all;
    std::vector<Ident> names;

    ShareDecl(bool share_all, std::vector<Ident> names, SourceSpan span);
};

struct ConstDecl : Decl {
    Ident name;
    TypeId type;
    ExprId value;

    ConstDecl(Ident name, TypeId type, ExprId value, SourceSpan span);
};

struct MacroDecl : Decl {
    Ident name;
    std::vector<Ident> params;
    StmtId body;

    MacroDecl(Ident name, std::vector<Ident> params, StmtId body, SourceSpan span);
};

struct FormDecl : Decl {
    Ident name;
    std::vector<Ident> type_params;
    std::vector<FieldDecl> fields;

    FormDecl(Ident name, std::vector<Ident> type_params, std::vector<FieldDecl> fields, SourceSpan span);
};

struct PickDecl : Decl {
    Ident name;
    std::vector<Ident> type_params;
    std::vector<CaseDecl> cases;

    PickDecl(Ident name, std::vector<Ident> type_params, std::vector<CaseDecl> cases, SourceSpan span);
};

struct ProcDecl : Decl {
    std::vector<Attribute> attrs;
    Ident name;
    std::vector<Ident> type_params;
    std::vector<FnParam> params;
    TypeId return_type;
    StmtId body;

    ProcDecl(
        std::vector<Attribute> attrs,
        Ident name,
        std::vector<Ident> type_params,
        std::vector<FnParam> params,
        TypeId return_type,
        StmtId body,
        SourceSpan span);
};

struct EntryDecl : Decl {
    Ident name;
    ModulePath module;
    StmtId body;

    EntryDecl(Ident name, ModulePath module, StmtId body, SourceSpan span);
};

// ------------------------------------------------------------
// Module
// ------------------------------------------------------------

struct Module : AstNode {
    std::string name;
    std::vector<DeclId> decls;

    Module(
        std::string name,
        std::vector<DeclId> decls,
        SourceSpan span);
};

// ------------------------------------------------------------
// AST Context / Arena
// ------------------------------------------------------------

struct AstContext {
    support::Arena<AstNode, AstId> arena;

    template <typename T, typename... Args>
    AstId make(Args&&... args) {
        return arena.template emplace<T>(std::forward<Args>(args)...);
    }

    AstNode& node(AstId id) {
        return *arena.get(id);
    }

    const AstNode& node(AstId id) const {
        return *arena.get(id);
    }

    template <typename T>
    T& get(AstId id) {
        return *static_cast<T*>(arena.get(id));
    }

    template <typename T>
    const T& get(AstId id) const {
        return *static_cast<const T*>(arena.get(id));
    }
};

// ------------------------------------------------------------
// Visitor (legacy)
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
    virtual void visit_invoke(InvokeExpr&) {}
    virtual void visit_ident_expr(IdentExpr&) {}
    virtual void visit_literal(LiteralExpr&) {}
};

// ------------------------------------------------------------
// Debug helpers
// ------------------------------------------------------------

const char* to_string(NodeKind kind);
std::string dump_to_string(const AstNode& node);

} // namespace vitte::frontend::ast
