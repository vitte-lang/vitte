// ============================================================
// validate.cpp — Vitte Compiler
// HIR validation
// ============================================================

#include "validate.hpp"
#include "../frontend/diagnostics_messages.hpp"

namespace vitte::ir::validate {

using vitte::frontend::diag::DiagnosticEngine;
using vitte::frontend::ast::SourceSpan;
namespace fdiag = vitte::frontend::diag;

static void error_invalid(DiagnosticEngine& diagnostics, const char* what, SourceSpan span) {
    diagnostics.error(std::string("invalid HIR ") + what, span);
}

static void validate_type(const HirContext& ctx,
                          HirTypeId type,
                          DiagnosticEngine& diagnostics,
                          SourceSpan span,
                          bool required);

static void validate_expr(const HirContext& ctx,
                          HirExprId expr,
                          DiagnosticEngine& diagnostics,
                          SourceSpan span,
                          bool required);

static void validate_stmt(const HirContext& ctx,
                          HirStmtId stmt,
                          DiagnosticEngine& diagnostics,
                          SourceSpan span,
                          bool required);

static void validate_pattern(const HirContext& ctx,
                             HirPatternId pattern,
                             DiagnosticEngine& diagnostics,
                             SourceSpan span,
                             bool required);

static void validate_type(const HirContext& ctx,
                          HirTypeId type,
                          DiagnosticEngine& diagnostics,
                          SourceSpan span,
                          bool required) {
    if (type == kInvalidHirId) {
        if (required) {
            error_invalid(diagnostics, "type", span);
        }
        return;
    }
    const auto& node = ctx.node(type);
    switch (node.kind) {
        case HirKind::NamedType:
            return;
        case HirKind::GenericType: {
            const auto& t = ctx.get<HirGenericType>(type);
            if (t.type_args.empty()) {
                fdiag::error(diagnostics, fdiag::DiagId::GenericTypeRequiresAtLeastOneTypeArgument, t.span);
            }
            for (auto arg : t.type_args) {
                validate_type(ctx, arg, diagnostics, t.span, true);
            }
            return;
        }
        default:
            fdiag::error(diagnostics, fdiag::DiagId::UnexpectedHirTypeKind, node.span);
            return;
    }
}

static void validate_expr(const HirContext& ctx,
                          HirExprId expr,
                          DiagnosticEngine& diagnostics,
                          SourceSpan span,
                          bool required) {
    if (expr == kInvalidHirId) {
        if (required) {
            error_invalid(diagnostics, "expr", span);
        }
        return;
    }
    const auto& node = ctx.node(expr);
    switch (node.kind) {
        case HirKind::LiteralExpr:
        case HirKind::VarExpr:
            return;
        case HirKind::UnaryExpr: {
            const auto& e = ctx.get<HirUnaryExpr>(expr);
            validate_expr(ctx, e.expr, diagnostics, e.span, true);
            return;
        }
        case HirKind::BinaryExpr: {
            const auto& e = ctx.get<HirBinaryExpr>(expr);
            validate_expr(ctx, e.lhs, diagnostics, e.span, true);
            validate_expr(ctx, e.rhs, diagnostics, e.span, true);
            return;
        }
        case HirKind::CallExpr: {
            const auto& e = ctx.get<HirCallExpr>(expr);
            validate_expr(ctx, e.callee, diagnostics, e.span, true);
            for (auto arg : e.args) {
                validate_expr(ctx, arg, diagnostics, e.span, true);
            }
            return;
        }
        default:
            fdiag::error(diagnostics, fdiag::DiagId::UnexpectedHirExprKind, node.span);
            return;
    }
}

static void validate_stmt(const HirContext& ctx,
                          HirStmtId stmt,
                          DiagnosticEngine& diagnostics,
                          SourceSpan span,
                          bool required) {
    if (stmt == kInvalidHirId) {
        if (required) {
            error_invalid(diagnostics, "stmt", span);
        }
        return;
    }
    const auto& node = ctx.node(stmt);
    switch (node.kind) {
        case HirKind::LetStmt: {
            const auto& s = ctx.get<HirLetStmt>(stmt);
            validate_type(ctx, s.type, diagnostics, s.span, false);
            validate_expr(ctx, s.init, diagnostics, s.span, true);
            return;
        }
        case HirKind::ExprStmt: {
            const auto& s = ctx.get<HirExprStmt>(stmt);
            validate_expr(ctx, s.expr, diagnostics, s.span, true);
            return;
        }
        case HirKind::ReturnStmt: {
            const auto& s = ctx.get<HirReturnStmt>(stmt);
            validate_expr(ctx, s.expr, diagnostics, s.span, false);
            return;
        }
        case HirKind::Block: {
            const auto& b = ctx.get<HirBlock>(stmt);
            for (auto s_id : b.stmts) {
                validate_stmt(ctx, s_id, diagnostics, b.span, true);
            }
            return;
        }
        case HirKind::IfStmt: {
            const auto& s = ctx.get<HirIf>(stmt);
            validate_expr(ctx, s.cond, diagnostics, s.span, true);
            validate_stmt(ctx, s.then_block, diagnostics, s.span, true);
            validate_stmt(ctx, s.else_block, diagnostics, s.span, false);
            return;
        }
        case HirKind::LoopStmt: {
            const auto& s = ctx.get<HirLoop>(stmt);
            validate_stmt(ctx, s.body, diagnostics, s.span, true);
            return;
        }
        case HirKind::WhenStmt: {
            const auto& s = ctx.get<HirWhen>(stmt);
            validate_pattern(ctx, s.pattern, diagnostics, s.span, true);
            validate_stmt(ctx, s.block, diagnostics, s.span, true);
            return;
        }
        case HirKind::SelectStmt: {
            const auto& s = ctx.get<HirSelect>(stmt);
            validate_expr(ctx, s.expr, diagnostics, s.span, true);
            if (s.whens.empty()) {
                fdiag::error(diagnostics, fdiag::DiagId::SelectRequiresAtLeastOneWhenBranch, s.span);
                diagnostics.note(
                    "example:\n  select x\n    when T(v) { ... }\n  otherwise { ... }",
                    s.span);
            }
            for (auto w_id : s.whens) {
                if (w_id == kInvalidHirId) {
                    error_invalid(diagnostics, "when", s.span);
                    continue;
                }
                const auto& w_node = ctx.node(w_id);
                if (w_node.kind != HirKind::WhenStmt) {
                    fdiag::error(diagnostics, fdiag::DiagId::SelectBranchMustBeWhenStatement, w_node.span);
                    diagnostics.note(
                        "select branches must be 'when' statements",
                        w_node.span);
                }
                validate_stmt(ctx, w_id, diagnostics, s.span, true);
            }
            validate_stmt(ctx, s.otherwise_block, diagnostics, s.span, false);
            return;
        }
        default:
            fdiag::error(diagnostics, fdiag::DiagId::UnexpectedHirStmtKind, node.span);
            return;
    }
}

static void validate_pattern(const HirContext& ctx,
                             HirPatternId pattern,
                             DiagnosticEngine& diagnostics,
                             SourceSpan span,
                             bool required) {
    if (pattern == kInvalidHirId) {
        if (required) {
            error_invalid(diagnostics, "pattern", span);
        }
        return;
    }
    const auto& node = ctx.node(pattern);
    switch (node.kind) {
        case HirKind::PatternIdent:
            return;
        case HirKind::PatternCtor: {
            const auto& p = ctx.get<HirCtorPattern>(pattern);
            for (auto arg : p.args) {
                validate_pattern(ctx, arg, diagnostics, p.span, true);
            }
            return;
        }
        default:
            fdiag::error(diagnostics, fdiag::DiagId::UnexpectedHirPatternKind, node.span);
            return;
    }
}

void validate_module(const HirContext& ctx,
                     HirModuleId module,
                     DiagnosticEngine& diagnostics) {
    if (module == kInvalidHirId) {
        error_invalid(diagnostics, "module", SourceSpan{});
        return;
    }
    const auto& mod = ctx.get<HirModule>(module);
    for (auto decl_id : mod.decls) {
        if (decl_id == kInvalidHirId) {
            error_invalid(diagnostics, "decl", mod.span);
            continue;
        }
        const auto& decl = ctx.node(decl_id);
        if (decl.kind == HirKind::ConstDecl) {
            const auto& c = ctx.get<HirConstDecl>(decl_id);
            validate_type(ctx, c.type, diagnostics, c.span, false);
            validate_expr(ctx, c.value, diagnostics, c.span, true);
            continue;
        }
        if (decl.kind != HirKind::FnDecl) {
            fdiag::error(diagnostics, fdiag::DiagId::UnexpectedHirDeclKind, decl.span);
            continue;
        }
        const auto& fn = ctx.get<HirFnDecl>(decl_id);
        for (const auto& param : fn.params) {
            validate_type(ctx, param.type, diagnostics, fn.span, false);
        }
        validate_type(ctx, fn.return_type, diagnostics, fn.span, false);
        bool has_body = fn.body != kInvalidHirId;
        validate_stmt(ctx, fn.body, diagnostics, fn.span, has_body);
    }
}

} // namespace vitte::ir::validate
