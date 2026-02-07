// ============================================================
// hir.hpp — Vitte Compiler
// High-level Intermediate Representation (HIR)
// ============================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <ostream>
#include <utility>
#include <vector>

#include "../frontend/ast.hpp"   // SourceSpan
#include "../support/arena.hpp"

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
struct HirPattern;

// ------------------------------------------------------------
// Handle aliases
// ------------------------------------------------------------

using HirId = std::uint32_t;
static constexpr HirId kInvalidHirId = static_cast<HirId>(-1);

using HirTypeId = HirId;
using HirExprId = HirId;
using HirStmtId = HirId;
using HirDeclId = HirId;
using HirPatternId = HirId;
using HirModuleId = HirId;

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
    IfStmt,
    LoopStmt,
    SelectStmt,
    WhenStmt,

    // declarations
    FnDecl,
    ConstDecl,

    // patterns
    PatternIdent,
    PatternCtor,
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
    std::vector<HirTypeId> type_args;

    HirGenericType(std::string base_name,
                   std::vector<HirTypeId> type_args,
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
    HirExprId expr;

    HirUnaryExpr(HirUnaryOp op,
                 HirExprId expr,
                 vitte::frontend::ast::SourceSpan span);
};

struct HirBinaryExpr : HirExpr {
    HirBinaryOp op;
    HirExprId lhs;
    HirExprId rhs;

    HirBinaryExpr(HirBinaryOp op,
                  HirExprId lhs,
                  HirExprId rhs,
                  vitte::frontend::ast::SourceSpan span);
};

struct HirCallExpr : HirExpr {
    HirExprId callee;
    std::vector<HirExprId> args;

    HirCallExpr(HirExprId callee,
                std::vector<HirExprId> args,
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
    HirTypeId type;
    HirExprId init;

    HirLetStmt(std::string name,
               HirTypeId type,
               HirExprId init,
               vitte::frontend::ast::SourceSpan span);
};

struct HirExprStmt : HirStmt {
    HirExprId expr;

    HirExprStmt(HirExprId expr,
                vitte::frontend::ast::SourceSpan span);
};

struct HirReturnStmt : HirStmt {
    HirExprId expr;

    HirReturnStmt(HirExprId expr,
                  vitte::frontend::ast::SourceSpan span);
};

struct HirBlock : HirStmt {
    std::vector<HirStmtId> stmts;

    HirBlock(std::vector<HirStmtId> stmts,
             vitte::frontend::ast::SourceSpan span);
};

struct HirIf : HirStmt {
    HirExprId cond;
    HirStmtId then_block;
    HirStmtId else_block;

    HirIf(HirExprId cond,
          HirStmtId then_block,
          HirStmtId else_block,
          vitte::frontend::ast::SourceSpan span);
};

struct HirLoop : HirStmt {
    HirStmtId body;

    HirLoop(HirStmtId body,
            vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Patterns
// ------------------------------------------------------------

struct HirPattern : HirNode {
    explicit HirPattern(HirKind kind,
                        vitte::frontend::ast::SourceSpan span);
};

struct HirIdentPattern : HirPattern {
    std::string name;

    HirIdentPattern(std::string name,
                    vitte::frontend::ast::SourceSpan span);
};

struct HirCtorPattern : HirPattern {
    std::string name;
    std::vector<HirPatternId> args;

    HirCtorPattern(std::string name,
                   std::vector<HirPatternId> args,
                   vitte::frontend::ast::SourceSpan span);
};

struct HirWhen : HirStmt {
    HirPatternId pattern;
    HirStmtId block;

    HirWhen(HirPatternId pattern,
            HirStmtId block,
            vitte::frontend::ast::SourceSpan span);
};

struct HirSelect : HirStmt {
    HirExprId expr;
    std::vector<HirStmtId> whens;
    HirStmtId otherwise_block;

    HirSelect(HirExprId expr,
              std::vector<HirStmtId> whens,
              HirStmtId otherwise_block,
              vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Declarations
// ------------------------------------------------------------

struct HirDecl : HirNode {
    explicit HirDecl(HirKind kind,
                     vitte::frontend::ast::SourceSpan span);
};

struct HirConstDecl : HirDecl {
    std::string name;
    HirTypeId type;
    HirExprId value;

    HirConstDecl(std::string name,
                 HirTypeId type,
                 HirExprId value,
                 vitte::frontend::ast::SourceSpan span);
};

struct HirParam {
    std::string name;
    HirTypeId type;

    HirParam(std::string name,
             HirTypeId type);
};

struct HirFnDecl : HirDecl {
    std::string name;
    std::vector<HirParam> params;
    HirTypeId return_type;
    HirStmtId body;

    HirFnDecl(std::string name,
              std::vector<HirParam> params,
              HirTypeId return_type,
              HirStmtId body,
              vitte::frontend::ast::SourceSpan span);
};

struct HirModule : HirNode {
    std::string name;
    std::vector<HirDeclId> decls;

    HirModule(std::string name,
              std::vector<HirDeclId> decls,
              vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// HIR Context / Arena
// ------------------------------------------------------------

struct HirContext {
    vitte::support::Arena<HirNode, HirId> arena;

    template <typename T, typename... Args>
    HirId make(Args&&... args) {
        return arena.template emplace<T>(std::forward<Args>(args)...);
    }

    HirNode& node(HirId id) {
        return *arena.get(id);
    }

    const HirNode& node(HirId id) const {
        return *arena.get(id);
    }

    template <typename T>
    T& get(HirId id) {
        return *static_cast<T*>(arena.get(id));
    }

    template <typename T>
    const T& get(HirId id) const {
        return *static_cast<const T*>(arena.get(id));
    }
};

// ------------------------------------------------------------
// Debug helpers
// ------------------------------------------------------------

const char* to_string(HirKind kind);

void dump(const HirContext& ctx,
          HirId node,
          std::ostream& os,
          std::size_t depth = 0);

std::string dump_to_string(const HirContext& ctx, HirId node);

void dump_compact(const HirContext& ctx, HirId node, std::ostream& os);
std::string dump_compact_to_string(const HirContext& ctx, HirId node);

void dump_json(const HirContext& ctx, HirId node, std::ostream& os);
std::string dump_json_to_string(const HirContext& ctx, HirId node);

} // namespace vitte::ir
