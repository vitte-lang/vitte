// ============================================================
// hir.cpp — Vitte Compiler
// High-level Intermediate Representation (HIR)
// ============================================================

#include "hir.hpp"

#include <cassert>
#include <ostream>
#include <sstream>

namespace vitte::ir {

// ------------------------------------------------------------
// HIR Node
// ------------------------------------------------------------

HirNode::HirNode(HirKind k, SourceSpan sp)
    : kind(k), span(sp) {}

HirNode::~HirNode() = default;

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

HirType::HirType(HirKind k, SourceSpan sp)
    : HirNode(k, sp) {}

HirNamedType::HirNamedType(
    std::string n,
    SourceSpan sp)
    : HirType(HirKind::NamedType, sp),
      name(std::move(n)) {}

HirGenericType::HirGenericType(
    std::string base,
    std::vector<HirTypePtr> args,
    SourceSpan sp)
    : HirType(HirKind::GenericType, sp),
      base_name(std::move(base)),
      type_args(std::move(args)) {}

// ------------------------------------------------------------
// Expressions
// ------------------------------------------------------------

HirExpr::HirExpr(HirKind k, SourceSpan sp)
    : HirNode(k, sp) {}

HirLiteralExpr::HirLiteralExpr(
    HirLiteralKind k,
    std::string v,
    SourceSpan sp)
    : HirExpr(HirKind::LiteralExpr, sp),
      lit_kind(k),
      value(std::move(v)) {}

HirVarExpr::HirVarExpr(
    std::string n,
    SourceSpan sp)
    : HirExpr(HirKind::VarExpr, sp),
      name(std::move(n)) {}

HirUnaryExpr::HirUnaryExpr(
    HirUnaryOp o,
    HirExprPtr e,
    SourceSpan sp)
    : HirExpr(HirKind::UnaryExpr, sp),
      op(o),
      expr(std::move(e)) {
    assert(expr);
}

HirBinaryExpr::HirBinaryExpr(
    HirBinaryOp o,
    HirExprPtr l,
    HirExprPtr r,
    SourceSpan sp)
    : HirExpr(HirKind::BinaryExpr, sp),
      op(o),
      lhs(std::move(l)),
      rhs(std::move(r)) {
    assert(lhs && rhs);
}

HirCallExpr::HirCallExpr(
    HirExprPtr callee,
    std::vector<HirExprPtr> args,
    SourceSpan sp)
    : HirExpr(HirKind::CallExpr, sp),
      callee(std::move(callee)),
      args(std::move(args)) {
    assert(this->callee);
}

// ------------------------------------------------------------
// Statements
// ------------------------------------------------------------

HirStmt::HirStmt(HirKind k, SourceSpan sp)
    : HirNode(k, sp) {}

HirLetStmt::HirLetStmt(
    std::string n,
    HirTypePtr t,
    HirExprPtr i,
    SourceSpan sp)
    : HirStmt(HirKind::LetStmt, sp),
      name(std::move(n)),
      type(std::move(t)),
      init(std::move(i)) {}

HirExprStmt::HirExprStmt(
    HirExprPtr e,
    SourceSpan sp)
    : HirStmt(HirKind::ExprStmt, sp),
      expr(std::move(e)) {
    assert(expr);
}

HirReturnStmt::HirReturnStmt(
    HirExprPtr e,
    SourceSpan sp)
    : HirStmt(HirKind::ReturnStmt, sp),
      expr(std::move(e)) {}

HirBlock::HirBlock(
    std::vector<HirStmtPtr> s,
    SourceSpan sp)
    : HirStmt(HirKind::Block, sp),
      stmts(std::move(s)) {}

// ------------------------------------------------------------
// Declarations
// ------------------------------------------------------------

HirDecl::HirDecl(HirKind k, SourceSpan sp)
    : HirNode(k, sp) {}

HirParam::HirParam(
    std::string n,
    HirTypePtr t)
    : name(std::move(n)),
      type(std::move(t)) {}

HirFnDecl::HirFnDecl(
    std::string n,
    std::vector<HirParam> p,
    HirTypePtr ret,
    HirBlock body,
    SourceSpan sp)
    : HirDecl(HirKind::FnDecl, sp),
      name(std::move(n)),
      params(std::move(p)),
      return_type(std::move(ret)),
      body(std::move(body)) {}

// ------------------------------------------------------------
// Module
// ------------------------------------------------------------

HirModule::HirModule(
    std::string n,
    std::vector<HirDeclPtr> d,
    SourceSpan sp)
    : HirNode(HirKind::Module, sp),
      name(std::move(n)),
      decls(std::move(d)) {}

// ------------------------------------------------------------
// Debug / dump
// ------------------------------------------------------------

static void indent(std::ostream& os, std::size_t depth) {
    for (std::size_t i = 0; i < depth; ++i) {
        os << "  ";
    }
}

void dump(const HirNode& n, std::ostream& os, std::size_t depth) {
    indent(os, depth);
    os << to_string(n.kind);

    if (n.span.is_valid()) {
        os << " [" << n.span.start << ".." << n.span.end << "]";
    }
    os << "\n";

    switch (n.kind) {
        case HirKind::Module: {
            auto& m = static_cast<const HirModule&>(n);
            for (auto& d : m.decls) {
                dump(*d, os, depth + 1);
            }
            break;
        }
        case HirKind::FnDecl: {
            auto& f = static_cast<const HirFnDecl&>(n);
            dump(f.body, os, depth + 1);
            break;
        }
        case HirKind::Block: {
            auto& b = static_cast<const HirBlock&>(n);
            for (auto& s : b.stmts) {
                dump(*s, os, depth + 1);
            }
            break;
        }
        default:
            break;
    }
}

std::string dump_to_string(const HirNode& n) {
    std::ostringstream ss;
    dump(n, ss, 0);
    return ss.str();
}

} // namespace vitte::ir