#include "lower_hir.hpp"
#include "diagnostics_messages.hpp"

#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace vitte::frontend::lower {

using namespace vitte::frontend::ast;

static ir::HirTypeId lower_type(
    const AstContext& ctx,
    TypeId type,
    ir::HirContext& hir_ctx);

static ir::HirExprId lower_expr(
    const AstContext& ctx,
    ExprId expr,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics,
    const std::unordered_set<std::string>& type_names,
    const std::unordered_map<std::string, bool>& enum_like);

static ir::HirStmtId lower_block(
    const AstContext& ctx,
    StmtId block_id,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics,
    const std::unordered_set<std::string>& type_names,
    const std::unordered_map<std::string, bool>& enum_like,
    std::size_t& tmp_counter);

static ir::HirPatternId lower_pattern(
    const AstContext& ctx,
    PatternId pattern,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics);

static ir::HirStmtId lower_stmt(
    const AstContext& ctx,
    StmtId stmt,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics,
    const std::unordered_set<std::string>& type_names,
    const std::unordered_map<std::string, bool>& enum_like,
    std::size_t& tmp_counter);

static const std::unordered_map<std::string, std::vector<std::string>>* g_ctor_fields = nullptr;

static ir::HirTypeId lower_type(
    const AstContext& ctx,
    TypeId type,
    ir::HirContext& hir_ctx)
{
    if (type == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    const AstNode& node = ctx.node(type);
    switch (node.kind) {
        case NodeKind::BuiltinType: {
            auto& t = static_cast<const BuiltinType&>(node);
            return hir_ctx.make<ir::HirNamedType>(t.name, t.span);
        }
        case NodeKind::NamedType: {
            auto& t = static_cast<const NamedType&>(node);
            return hir_ctx.make<ir::HirNamedType>(t.ident.name, t.span);
        }
        case NodeKind::GenericType: {
            auto& t = static_cast<const GenericType&>(node);
            std::vector<ir::HirTypeId> args;
            for (auto a : t.type_args) {
                args.push_back(lower_type(ctx, a, hir_ctx));
            }
            return hir_ctx.make<ir::HirGenericType>(
                t.base_ident.name,
                std::move(args),
                t.span);
        }
        case NodeKind::SliceType: {
            auto& t = static_cast<const SliceType&>(node);
            std::vector<ir::HirTypeId> args;
            args.push_back(lower_type(ctx, t.element, hir_ctx));
            return hir_ctx.make<ir::HirGenericType>(
                "slice",
                std::move(args),
                t.span);
        }
        case NodeKind::PointerType: {
            auto& t = static_cast<const PointerType&>(node);
            std::vector<ir::HirTypeId> args;
            args.push_back(lower_type(ctx, t.pointee, hir_ctx));
            return hir_ctx.make<ir::HirGenericType>(
                "ptr",
                std::move(args),
                t.span);
        }
        case NodeKind::ProcType: {
            auto& t = static_cast<const ProcType&>(node);
            std::vector<ir::HirTypeId> params;
            for (auto p : t.params) {
                params.push_back(lower_type(ctx, p, hir_ctx));
            }
            auto ret = lower_type(ctx, t.return_type, hir_ctx);
            return hir_ctx.make<ir::HirProcType>(std::move(params), ret, t.span);
        }
        default:
            return ir::kInvalidHirId;
    }
}

static ir::HirExprId lower_invoke(
    const AstContext& ctx,
    const InvokeExpr& inv,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics,
    const std::unordered_set<std::string>& type_names,
    const std::unordered_map<std::string, bool>& enum_like)
{
    ir::HirExprId callee = ir::kInvalidHirId;
    std::vector<ir::HirTypeId> type_args;
    if (inv.callee_expr != kInvalidAstId) {
        callee = lower_expr(ctx, inv.callee_expr, hir_ctx, diagnostics, type_names, enum_like);
        type_args.reserve(inv.type_args.size());
        for (auto arg : inv.type_args) {
            type_args.push_back(lower_type(ctx, arg, hir_ctx));
        }
    } else if (inv.callee_type != kInvalidAstId) {
        const AstNode& tnode = ctx.node(inv.callee_type);
        if (tnode.kind == NodeKind::BuiltinType) {
            auto& t = static_cast<const BuiltinType&>(tnode);
            callee = hir_ctx.make<ir::HirVarExpr>(t.name, t.span);
        } else if (tnode.kind == NodeKind::NamedType) {
            auto& t = static_cast<const NamedType&>(tnode);
            callee = hir_ctx.make<ir::HirVarExpr>(t.ident.name, t.ident.span);
        } else if (tnode.kind == NodeKind::GenericType) {
            auto& t = static_cast<const GenericType&>(tnode);
            callee = hir_ctx.make<ir::HirVarExpr>(t.base_ident.name, t.base_ident.span);
            type_args.reserve(t.type_args.size());
            for (auto arg : t.type_args) {
                type_args.push_back(lower_type(ctx, arg, hir_ctx));
            }
        }
    }

    std::vector<ir::HirExprId> args;
    if (inv.invoke_kind == InvokeExpr::Kind::Ctor && g_ctor_fields != nullptr) {
        std::string ctor_name;
        if (inv.callee_type != kInvalidAstId) {
            const AstNode& tnode = ctx.node(inv.callee_type);
            if (tnode.kind == NodeKind::NamedType) {
                ctor_name = static_cast<const NamedType&>(tnode).ident.name;
            } else if (tnode.kind == NodeKind::GenericType) {
                ctor_name = static_cast<const GenericType&>(tnode).base_ident.name;
            } else if (tnode.kind == NodeKind::BuiltinType) {
                ctor_name = static_cast<const BuiltinType&>(tnode).name;
            }
        }
        if (!ctor_name.empty()) {
            auto field_it = g_ctor_fields->find(ctor_name);
            if (field_it != g_ctor_fields->end() && !field_it->second.empty()) {
                const auto& field_order = field_it->second;
                std::vector<ir::HirExprId> ordered(field_order.size(), ir::kInvalidHirId);
                std::size_t next_pos = 0;
                for (const auto& a : inv.args) {
                    if (!a.name.has_value()) {
                        continue;
                    }
                    for (std::size_t i = 0; i < field_order.size(); ++i) {
                        if (field_order[i] == a.name->name) {
                            ordered[i] = lower_expr(ctx, a.value, hir_ctx, diagnostics, type_names, enum_like);
                            break;
                        }
                    }
                }
                for (const auto& a : inv.args) {
                    if (a.name.has_value()) {
                        continue;
                    }
                    while (next_pos < ordered.size() && ordered[next_pos] != ir::kInvalidHirId) {
                        ++next_pos;
                    }
                    if (next_pos >= ordered.size()) {
                        break;
                    }
                    ordered[next_pos] = lower_expr(ctx, a.value, hir_ctx, diagnostics, type_names, enum_like);
                    ++next_pos;
                }
                for (auto& arg : ordered) {
                    if (arg != ir::kInvalidHirId) {
                        args.push_back(std::move(arg));
                    }
                }
            }
        }
    }
    if (args.empty()) {
        for (const auto& a : inv.args) {
            args.push_back(lower_expr(ctx, a.value, hir_ctx, diagnostics, type_names, enum_like));
        }
    }

    if (callee == ir::kInvalidHirId) {
        diag::error(diagnostics, diag::DiagId::InvokeHasNoCallee, inv.span);
        callee = hir_ctx.make<ir::HirVarExpr>("<error>", inv.span);
    }

    return hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), std::move(type_args), inv.span);
}

static ir::HirExprId lower_expr(
    const AstContext& ctx,
    ExprId expr,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics,
    const std::unordered_set<std::string>& type_names,
    const std::unordered_map<std::string, bool>& enum_like)
{
    if (expr == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    const AstNode& node = ctx.node(expr);
    switch (node.kind) {
        case NodeKind::LiteralExpr: {
            auto& e = static_cast<const LiteralExpr&>(node);
            ir::HirLiteralKind kind = ir::HirLiteralKind::Int;
            switch (e.lit_kind) {
                case LiteralKind::Bool: kind = ir::HirLiteralKind::Bool; break;
                case LiteralKind::Int: kind = ir::HirLiteralKind::Int; break;
                case LiteralKind::String: kind = ir::HirLiteralKind::String; break;
                case LiteralKind::Float: kind = ir::HirLiteralKind::Int; break;
                case LiteralKind::Char: kind = ir::HirLiteralKind::Int; break;
            }
            return hir_ctx.make<ir::HirLiteralExpr>(kind, e.value, e.span);
        }
        case NodeKind::IdentExpr: {
            auto& e = static_cast<const IdentExpr&>(node);
            return hir_ctx.make<ir::HirVarExpr>(e.ident.name, e.span);
        }
        case NodeKind::UnaryExpr: {
            auto& e = static_cast<const UnaryExpr&>(node);
            auto rhs = lower_expr(ctx, e.expr, hir_ctx, diagnostics, type_names, enum_like);
            return hir_ctx.make<ir::HirUnaryExpr>(
                ir::HirUnaryOp::Not,
                rhs,
                e.span);
        }
        case NodeKind::BinaryExpr: {
            auto& e = static_cast<const BinaryExpr&>(node);
            ir::HirBinaryOp op = ir::HirBinaryOp::Add;
            switch (e.op) {
                case BinaryOp::Add: op = ir::HirBinaryOp::Add; break;
                case BinaryOp::Sub: op = ir::HirBinaryOp::Sub; break;
                case BinaryOp::Mul: op = ir::HirBinaryOp::Mul; break;
                case BinaryOp::Div: op = ir::HirBinaryOp::Div; break;
                case BinaryOp::Eq: op = ir::HirBinaryOp::Eq; break;
                case BinaryOp::Ne: op = ir::HirBinaryOp::Ne; break;
                case BinaryOp::Lt: op = ir::HirBinaryOp::Lt; break;
                case BinaryOp::Le: op = ir::HirBinaryOp::Le; break;
                case BinaryOp::Gt: op = ir::HirBinaryOp::Gt; break;
                case BinaryOp::Ge: op = ir::HirBinaryOp::Ge; break;
                case BinaryOp::And: op = ir::HirBinaryOp::And; break;
                case BinaryOp::Or: op = ir::HirBinaryOp::Or; break;
                case BinaryOp::Assign: op = ir::HirBinaryOp::Assign; break;
                default: break;
            }
            return hir_ctx.make<ir::HirBinaryExpr>(
                op,
                lower_expr(ctx, e.lhs, hir_ctx, diagnostics, type_names, enum_like),
                lower_expr(ctx, e.rhs, hir_ctx, diagnostics, type_names, enum_like),
                e.span);
        }
        case NodeKind::AsExpr: {
            auto& e = static_cast<const AsExpr&>(node);
            return hir_ctx.make<ir::HirCastExpr>(
                lower_expr(ctx, e.value, hir_ctx, diagnostics, type_names, enum_like),
                lower_type(ctx, e.type, hir_ctx),
                e.span);
        }
        case NodeKind::IsExpr: {
            auto& e = static_cast<const IsExpr&>(node);
            return hir_ctx.make<ir::HirPatternTestExpr>(
                lower_expr(ctx, e.value, hir_ctx, diagnostics, type_names, enum_like),
                lower_pattern(ctx, e.pattern, hir_ctx, diagnostics),
                e.span);
        }
        case NodeKind::InvokeExpr:
            return lower_invoke(
                ctx,
                static_cast<const InvokeExpr&>(node),
                hir_ctx,
                diagnostics,
                type_names,
                enum_like);
        case NodeKind::MemberExpr: {
            auto& e = static_cast<const MemberExpr&>(node);
            auto base = lower_expr(ctx, e.base, hir_ctx, diagnostics, type_names, enum_like);
            bool base_is_type = false;
            bool is_enum = false;
            if (e.base != kInvalidAstId) {
                const auto& base_node = ctx.get<Expr>(e.base);
                if (base_node.kind == NodeKind::IdentExpr) {
                    const auto& id = static_cast<const IdentExpr&>(base_node);
                    base_is_type = type_names.count(id.ident.name) > 0;
                    auto it = enum_like.find(id.ident.name);
                    if (it != enum_like.end()) {
                        is_enum = it->second;
                    }
                }
            }
            return hir_ctx.make<ir::HirMemberExpr>(
                base,
                e.member.name,
                false,
                base_is_type,
                is_enum,
                e.span);
        }
        case NodeKind::IndexExpr: {
            auto& e = static_cast<const IndexExpr&>(node);
            return hir_ctx.make<ir::HirIndexExpr>(
                lower_expr(ctx, e.base, hir_ctx, diagnostics, type_names, enum_like),
                lower_expr(ctx, e.index, hir_ctx, diagnostics, type_names, enum_like),
                e.span);
        }
        case NodeKind::CallNoParenExpr: {
            auto& e = static_cast<const CallNoParenExpr&>(node);
            std::vector<ir::HirExprId> args;
            args.push_back(lower_expr(ctx, e.arg, hir_ctx, diagnostics, type_names, enum_like));
            auto callee = hir_ctx.make<ir::HirVarExpr>(e.callee.name, e.callee.span);
            return hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), std::vector<ir::HirTypeId>{}, e.span);
        }
        case NodeKind::ListExpr: {
            auto& e = static_cast<const ListExpr&>(node);
            std::vector<ir::HirExprId> args;
            for (auto item : e.items) {
                args.push_back(lower_expr(ctx, item, hir_ctx, diagnostics, type_names, enum_like));
            }
            auto callee = hir_ctx.make<ir::HirVarExpr>("list", e.span);
            return hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), std::vector<ir::HirTypeId>{}, e.span);
        }
        case NodeKind::ListCompExpr: {
            auto& e = static_cast<const ListCompExpr&>(node);
            return hir_ctx.make<ir::HirListCompExpr>(
                static_cast<ir::HirListCompExpr::Kind>(e.kind == ListCompExpr::Kind::List
                    ? ir::HirListCompExpr::Kind::List
                    : (e.kind == ListCompExpr::Kind::Set
                        ? ir::HirListCompExpr::Kind::Set
                        : ir::HirListCompExpr::Kind::Dict)),
                e.key != kInvalidAstId
                    ? lower_expr(ctx, e.key, hir_ctx, diagnostics, type_names, enum_like)
                    : ir::kInvalidHirId,
                lower_expr(ctx, e.value, hir_ctx, diagnostics, type_names, enum_like),
                e.index_ident.has_value() ? std::optional<std::string>(e.index_ident->name) : std::optional<std::string>{},
                e.ident.name,
                lower_expr(ctx, e.iterable, hir_ctx, diagnostics, type_names, enum_like),
                e.condition != kInvalidAstId
                    ? lower_expr(ctx, e.condition, hir_ctx, diagnostics, type_names, enum_like)
                    : ir::kInvalidHirId,
                e.span);
        }
        default:
            diag::error(diagnostics, diag::DiagId::UnsupportedExpressionInHir, node.span);
            return ir::kInvalidHirId;
    }
}

static std::string pattern_type_name(const AstContext& ctx, TypeId type) {
    if (type == kInvalidAstId) {
        return "<unknown>";
    }
    const AstNode& node = ctx.node(type);
    switch (node.kind) {
        case NodeKind::BuiltinType: {
            auto& t = static_cast<const BuiltinType&>(node);
            return t.name;
        }
        case NodeKind::NamedType: {
            auto& t = static_cast<const NamedType&>(node);
            return t.ident.name;
        }
        case NodeKind::GenericType: {
            auto& t = static_cast<const GenericType&>(node);
            return t.base_ident.name;
        }
        default:
            return "<unknown>";
    }
}

static ir::HirPatternId lower_pattern(
    const AstContext& ctx,
    PatternId pattern,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics)
{
    if (pattern == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    const AstNode& node = ctx.node(pattern);
    switch (node.kind) {
        case NodeKind::IdentPattern: {
            auto& p = static_cast<const IdentPattern&>(node);
            return hir_ctx.make<ir::HirIdentPattern>(p.ident.name, p.span);
        }
        case NodeKind::CtorPattern: {
            auto& p = static_cast<const CtorPattern&>(node);
            std::vector<ir::HirPatternId> args;
            for (auto a : p.args) {
                args.push_back(lower_pattern(ctx, a, hir_ctx, diagnostics));
            }
            return hir_ctx.make<ir::HirCtorPattern>(
                pattern_type_name(ctx, p.type),
                std::move(args),
                p.span);
        }
        case NodeKind::WildcardPattern: {
            auto& p = static_cast<const WildcardPattern&>(node);
            return hir_ctx.make<ir::HirWildcardPattern>(p.span);
        }
        default:
            diag::error(diagnostics, diag::DiagId::UnsupportedPatternInHir, node.span);
            return ir::kInvalidHirId;
    }
}

static ir::HirStmtId lower_block(
    const AstContext& ctx,
    StmtId block_id,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics,
    const std::unordered_set<std::string>& type_names,
    const std::unordered_map<std::string, bool>& enum_like,
    std::size_t& tmp_counter)
{
    if (block_id == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    auto& block = ctx.get<BlockStmt>(block_id);
    std::vector<ir::HirStmtId> out;
    for (auto s : block.stmts) {
        auto hs = lower_stmt(ctx, s, hir_ctx, diagnostics, type_names, enum_like, tmp_counter);
        if (hs != ir::kInvalidHirId) {
            out.push_back(hs);
        }
    }
    return hir_ctx.make<ir::HirBlock>(std::move(out), block.span);
}

static ir::HirStmtId lower_stmt(
    const AstContext& ctx,
    StmtId stmt,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics,
    const std::unordered_set<std::string>& type_names,
    const std::unordered_map<std::string, bool>& enum_like,
    std::size_t& tmp_counter)
{
    if (stmt == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    const AstNode& node = ctx.node(stmt);
    switch (node.kind) {
        case NodeKind::BlockStmt: {
            return lower_block(ctx, stmt, hir_ctx, diagnostics, type_names, enum_like, tmp_counter);
        }
        case NodeKind::UnsafeStmt: {
            auto& s = static_cast<const UnsafeStmt&>(node);
            std::vector<ir::HirStmtId> stmts;
            auto mk_call = [&](const char* name) {
                auto callee = hir_ctx.make<ir::HirVarExpr>(name, s.span);
                std::vector<ir::HirExprId> args;
                auto call = hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), std::vector<ir::HirTypeId>{}, s.span);
                return hir_ctx.make<ir::HirExprStmt>(call, s.span);
            };
            stmts.push_back(mk_call("unsafe_begin"));
            auto inner_block = lower_block(ctx, s.body, hir_ctx, diagnostics, type_names, enum_like, tmp_counter);
            if (inner_block != ir::kInvalidHirId) {
                if (hir_ctx.node(inner_block).kind == ir::HirKind::Block) {
                    auto& b = hir_ctx.get<ir::HirBlock>(inner_block);
                    for (auto st : b.stmts) {
                        stmts.push_back(st);
                    }
                } else {
                    stmts.push_back(inner_block);
                }
            }
            stmts.push_back(mk_call("unsafe_end"));
            return hir_ctx.make<ir::HirBlock>(std::move(stmts), s.span);
        }
        case NodeKind::AsmStmt: {
            auto& s = static_cast<const AsmStmt&>(node);
            auto lit = hir_ctx.make<ir::HirLiteralExpr>(
                ir::HirLiteralKind::String,
                s.code,
                s.span);
            auto callee = hir_ctx.make<ir::HirVarExpr>("asm", s.span);
            std::vector<ir::HirExprId> args;
            args.push_back(lit);
            auto call = hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), std::vector<ir::HirTypeId>{}, s.span);
            return hir_ctx.make<ir::HirExprStmt>(call, s.span);
        }
        case NodeKind::LetStmt: {
            auto& s = static_cast<const LetStmt&>(node);
            if (s.is_destructuring) {
                auto init = lower_expr(ctx, s.initializer, hir_ctx, diagnostics, type_names, enum_like);
                if (!init) {
                    diag::Diagnostic d(
                        diag::Severity::Error,
                        "E2003",
                        "unsupported expression in HIR",
                        s.span
                    );
                    d.add_note("destructuring initializer must lower to a value expression");
                    diagnostics.emit(std::move(d));
                    return ir::kInvalidHirId;
                }
                std::vector<ir::HirStmtId> lowered;
                std::string tmp_name = "__destruct_" + std::to_string(tmp_counter++);
                lowered.push_back(hir_ctx.make<ir::HirLetStmt>(
                    tmp_name,
                    lower_type(ctx, s.type, hir_ctx),
                    init,
                    s.span));
                auto root_var = hir_ctx.make<ir::HirVarExpr>(tmp_name, s.span);
                std::function<void(const LetBinding&, ir::HirExprId)> emit_binding =
                    [&](const LetBinding& binding, ir::HirExprId source) {
                        if (binding.children.empty()) {
                            if (binding.ident.name != "_") {
                                lowered.push_back(hir_ctx.make<ir::HirLetStmt>(
                                    binding.ident.name,
                                    ir::kInvalidHirId,
                                    source,
                                    binding.ident.span));
                            }
                            return;
                        }
                        std::string tmp_name = "__destruct_" + std::to_string(tmp_counter++);
                        lowered.push_back(hir_ctx.make<ir::HirLetStmt>(
                            tmp_name,
                            ir::kInvalidHirId,
                            source,
                            binding.ident.span));
                        auto tmp_var = hir_ctx.make<ir::HirVarExpr>(tmp_name, binding.ident.span);
                        for (std::size_t i = 0; i < binding.children.size(); ++i) {
                            auto idx = hir_ctx.make<ir::HirLiteralExpr>(
                                ir::HirLiteralKind::Int,
                                std::to_string(i),
                                binding.children[i].ident.span);
                            auto item = hir_ctx.make<ir::HirIndexExpr>(tmp_var, idx, binding.children[i].ident.span);
                            emit_binding(binding.children[i], item);
                        }
                    };
                for (std::size_t i = 0; i < s.bindings.size(); ++i) {
                    auto idx = hir_ctx.make<ir::HirLiteralExpr>(
                        ir::HirLiteralKind::Int,
                        std::to_string(i),
                        s.bindings[i].ident.span);
                    auto item = hir_ctx.make<ir::HirIndexExpr>(
                        root_var,
                        idx,
                        s.bindings[i].ident.span);
                    emit_binding(s.bindings[i], item);
                }
                return hir_ctx.make<ir::HirBlock>(std::move(lowered), s.span);
            }
            return hir_ctx.make<ir::HirLetStmt>(
                s.ident.name,
                lower_type(ctx, s.type, hir_ctx),
                lower_expr(ctx, s.initializer, hir_ctx, diagnostics, type_names, enum_like),
                s.span);
        }
        case NodeKind::MakeStmt: {
            auto& s = static_cast<const MakeStmt&>(node);
            return hir_ctx.make<ir::HirLetStmt>(
                s.ident.name,
                lower_type(ctx, s.type, hir_ctx),
                lower_expr(ctx, s.value, hir_ctx, diagnostics, type_names, enum_like),
                s.span);
        }
        case NodeKind::SetStmt: {
            auto& s = static_cast<const SetStmt&>(node);
            if (s.target != kInvalidAstId && ctx.node(s.target).kind == NodeKind::ListExpr) {
                auto rhs = lower_expr(ctx, s.value, hir_ctx, diagnostics, type_names, enum_like);
                if (!rhs) {
                    diag::Diagnostic d(
                        diag::Severity::Error,
                        "E2003",
                        "unsupported expression in HIR",
                        s.span
                    );
                    d.add_note("multi-assignment source must lower to a value expression");
                    diagnostics.emit(std::move(d));
                    return ir::kInvalidHirId;
                }
                std::vector<ir::HirStmtId> lowered;
                std::string tmp_name = "__assign_" + std::to_string(tmp_counter++);
                lowered.push_back(hir_ctx.make<ir::HirLetStmt>(tmp_name, ir::kInvalidHirId, rhs, s.span));
                auto root_var = hir_ctx.make<ir::HirVarExpr>(tmp_name, s.span);

                std::function<void(ExprId, ir::HirExprId)> emit_binding =
                    [&](ExprId target_expr, ir::HirExprId source_expr) {
                        if (target_expr == kInvalidAstId) {
                            return;
                        }
                        const auto& tnode = ctx.node(target_expr);
                        if (tnode.kind == NodeKind::ListExpr) {
                            const auto& target_list = static_cast<const ListExpr&>(tnode);
                            std::string nested_name = "__assign_" + std::to_string(tmp_counter++);
                            lowered.push_back(hir_ctx.make<ir::HirLetStmt>(
                                nested_name,
                                ir::kInvalidHirId,
                                source_expr,
                                tnode.span));
                            auto nested_var = hir_ctx.make<ir::HirVarExpr>(nested_name, tnode.span);
                            for (std::size_t i = 0; i < target_list.items.size(); ++i) {
                                auto idx = hir_ctx.make<ir::HirLiteralExpr>(
                                    ir::HirLiteralKind::Int,
                                    std::to_string(i),
                                    target_list.items[i] != kInvalidAstId
                                        ? ctx.get<Expr>(target_list.items[i]).span
                                        : tnode.span);
                                auto item = hir_ctx.make<ir::HirIndexExpr>(
                                    nested_var,
                                    idx,
                                    target_list.items[i] != kInvalidAstId
                                        ? ctx.get<Expr>(target_list.items[i]).span
                                        : tnode.span);
                                emit_binding(target_list.items[i], item);
                            }
                            return;
                        }
                        auto assign = hir_ctx.make<ir::HirBinaryExpr>(
                            ir::HirBinaryOp::Assign,
                            lower_expr(ctx, target_expr, hir_ctx, diagnostics, type_names, enum_like),
                            source_expr,
                            ctx.node(target_expr).span);
                        lowered.push_back(hir_ctx.make<ir::HirExprStmt>(assign, ctx.node(target_expr).span));
                    };

                auto& target_list = ctx.get<ListExpr>(s.target);
                for (std::size_t i = 0; i < target_list.items.size(); ++i) {
                    auto idx = hir_ctx.make<ir::HirLiteralExpr>(
                        ir::HirLiteralKind::Int,
                        std::to_string(i),
                        target_list.items[i] != kInvalidAstId
                            ? ctx.get<Expr>(target_list.items[i]).span
                            : s.span);
                    auto item = hir_ctx.make<ir::HirIndexExpr>(root_var, idx, s.span);
                    emit_binding(target_list.items[i], item);
                }
                return hir_ctx.make<ir::HirBlock>(std::move(lowered), s.span);
            }
            auto lhs = lower_expr(ctx, s.target, hir_ctx, diagnostics, type_names, enum_like);
            auto rhs = lower_expr(ctx, s.value, hir_ctx, diagnostics, type_names, enum_like);
            auto assign = hir_ctx.make<ir::HirBinaryExpr>(ir::HirBinaryOp::Assign, lhs, rhs, s.span);
            return hir_ctx.make<ir::HirExprStmt>(assign, s.span);
        }
        case NodeKind::GiveStmt: {
            auto& s = static_cast<const GiveStmt&>(node);
            return hir_ctx.make<ir::HirReturnStmt>(
                lower_expr(ctx, s.value, hir_ctx, diagnostics, type_names, enum_like),
                s.span);
        }
        case NodeKind::EmitStmt: {
            auto& s = static_cast<const EmitStmt&>(node);
            std::vector<ir::HirExprId> args;
            args.push_back(lower_expr(ctx, s.value, hir_ctx, diagnostics, type_names, enum_like));
            auto callee = hir_ctx.make<ir::HirVarExpr>("emit", s.span);
            auto call = hir_ctx.make<ir::HirCallExpr>(callee, std::move(args), std::vector<ir::HirTypeId>{}, s.span);
            return hir_ctx.make<ir::HirExprStmt>(call, s.span);
        }
        case NodeKind::ExprStmt: {
            auto& s = static_cast<const ExprStmt&>(node);
            return hir_ctx.make<ir::HirExprStmt>(
                lower_expr(ctx, s.expr, hir_ctx, diagnostics, type_names, enum_like),
                s.span);
        }
        case NodeKind::ReturnStmt: {
            auto& s = static_cast<const ReturnStmt&>(node);
            return hir_ctx.make<ir::HirReturnStmt>(
                lower_expr(ctx, s.expr, hir_ctx, diagnostics, type_names, enum_like),
                s.span);
        }
        case NodeKind::TryStmt: {
            auto& s = static_cast<const TryStmt&>(node);
            return hir_ctx.make<ir::HirTryStmt>(
                lower_block(ctx, s.body, hir_ctx, diagnostics, type_names, enum_like, tmp_counter),
                s.except_body != kInvalidAstId
                    ? lower_block(ctx, s.except_body, hir_ctx, diagnostics, type_names, enum_like, tmp_counter)
                    : ir::kInvalidHirId,
                s.finally_body != kInvalidAstId
                    ? lower_block(ctx, s.finally_body, hir_ctx, diagnostics, type_names, enum_like, tmp_counter)
                    : ir::kInvalidHirId,
                s.span);
        }
        case NodeKind::RaiseStmt: {
            auto& s = static_cast<const RaiseStmt&>(node);
            return hir_ctx.make<ir::HirRaiseStmt>(
                lower_expr(ctx, s.expr, hir_ctx, diagnostics, type_names, enum_like),
                s.span);
        }
        case NodeKind::IfStmt: {
            auto& s = static_cast<const IfStmt&>(node);
            return hir_ctx.make<ir::HirIf>(
                lower_expr(ctx, s.cond, hir_ctx, diagnostics, type_names, enum_like),
                lower_block(ctx, s.then_block, hir_ctx, diagnostics, type_names, enum_like, tmp_counter),
                s.else_block != kInvalidAstId
                    ? lower_block(ctx, s.else_block, hir_ctx, diagnostics, type_names, enum_like, tmp_counter)
                    : ir::kInvalidHirId,
                s.span);
        }
        case NodeKind::LoopStmt: {
            auto& s = static_cast<const LoopStmt&>(node);
            return hir_ctx.make<ir::HirLoop>(
                lower_block(ctx, s.body, hir_ctx, diagnostics, type_names, enum_like, tmp_counter),
                s.span);
        }
        case NodeKind::BreakStmt: {
            auto& s = static_cast<const BreakStmt&>(node);
            return hir_ctx.make<ir::HirBreak>(s.span);
        }
        case NodeKind::ContinueStmt: {
            auto& s = static_cast<const ContinueStmt&>(node);
            return hir_ctx.make<ir::HirContinue>(s.span);
        }
        case NodeKind::ForStmt: {
            auto& s = static_cast<const ForStmt&>(node);
            if (s.iterable != kInvalidAstId) {
                const auto& iterable_node = ctx.node(s.iterable);
                if (iterable_node.kind == NodeKind::LiteralExpr) {
                    const auto& lit = static_cast<const LiteralExpr&>(iterable_node);
                    if (lit.lit_kind != LiteralKind::String) {
                        diag::Diagnostic d(
                            diag::Severity::Error,
                            "E1109",
                            "for-in expects an iterable value (collection or iterable expression), not a scalar literal",
                            lit.span
                        );
                        d.add_note("current for-in lowering expects len/index iteration semantics");
                        d.add_fix("iterate over a list/expression with len and index semantics", "for x in [ ... ] { ... }", lit.span);
                        diagnostics.emit(std::move(d));
                        return ir::kInvalidHirId;
                    }
                }
            }

            const std::string tmp_iter = "__for_iter_" + std::to_string(tmp_counter++);
            const std::string tmp_idx = "__for_idx_" + std::to_string(tmp_counter++);

            std::vector<ir::HirStmtId> lowered;
            lowered.reserve(4);

            auto iter_init = lower_expr(ctx, s.iterable, hir_ctx, diagnostics, type_names, enum_like);
            lowered.push_back(hir_ctx.make<ir::HirLetStmt>(tmp_iter, ir::kInvalidHirId, iter_init, s.span));

            const std::string tmp_seq = "__for_seq_" + std::to_string(tmp_counter++);
            auto iter_source = hir_ctx.make<ir::HirVarExpr>(tmp_iter, s.span);
            auto iter_member = hir_ctx.make<ir::HirMemberExpr>(
                iter_source,
                "__iter__",
                false,
                false,
                false,
                s.span);
            std::vector<ir::HirExprId> iter_args;
            auto iter_call = hir_ctx.make<ir::HirCallExpr>(
                iter_member,
                std::move(iter_args),
                std::vector<ir::HirTypeId>{},
                s.span);
            lowered.push_back(hir_ctx.make<ir::HirLetStmt>(tmp_seq, ir::kInvalidHirId, iter_call, s.span));

            auto zero = hir_ctx.make<ir::HirLiteralExpr>(ir::HirLiteralKind::Int, "0", s.span);
            lowered.push_back(hir_ctx.make<ir::HirLetStmt>(
                tmp_idx,
                hir_ctx.make<ir::HirNamedType>("usize", s.span),
                zero,
                s.span));

            auto idx_var = hir_ctx.make<ir::HirVarExpr>(tmp_idx, s.span);
            auto iter_var_for_len = hir_ctx.make<ir::HirVarExpr>(tmp_seq, s.span);
            auto len_member = hir_ctx.make<ir::HirMemberExpr>(
                iter_var_for_len,
                "len",
                false,
                false,
                false,
                s.span);
            auto cond = hir_ctx.make<ir::HirBinaryExpr>(ir::HirBinaryOp::Lt, idx_var, len_member, s.span);
            auto not_cond = hir_ctx.make<ir::HirUnaryExpr>(ir::HirUnaryOp::Not, cond, s.span);
            auto break_stmt = hir_ctx.make<ir::HirBreak>(s.span);
            std::vector<ir::HirStmtId> break_stmts;
            break_stmts.push_back(break_stmt);
            auto break_block = hir_ctx.make<ir::HirBlock>(std::move(break_stmts), s.span);
            auto guard_if = hir_ctx.make<ir::HirIf>(not_cond, break_block, ir::kInvalidHirId, s.span);

            std::vector<ir::HirStmtId> loop_body_stmts;
            loop_body_stmts.push_back(guard_if);

            auto iter_var_for_item = hir_ctx.make<ir::HirVarExpr>(tmp_seq, s.span);
            auto idx_var_for_item = hir_ctx.make<ir::HirVarExpr>(tmp_idx, s.span);
            auto item_expr = hir_ctx.make<ir::HirIndexExpr>(iter_var_for_item, idx_var_for_item, s.span);
            if (s.tuple_destructure && s.index_ident.has_value()) {
                auto first_item = hir_ctx.make<ir::HirIndexExpr>(item_expr, hir_ctx.make<ir::HirLiteralExpr>(ir::HirLiteralKind::Int, "0", s.span), s.span);
                auto second_item = hir_ctx.make<ir::HirIndexExpr>(item_expr, hir_ctx.make<ir::HirLiteralExpr>(ir::HirLiteralKind::Int, "1", s.span), s.span);
                loop_body_stmts.push_back(hir_ctx.make<ir::HirLetStmt>(
                    s.index_ident->name,
                    ir::kInvalidHirId,
                    first_item,
                    s.index_ident->span));
                loop_body_stmts.push_back(hir_ctx.make<ir::HirLetStmt>(
                    s.ident.name,
                    ir::kInvalidHirId,
                    second_item,
                    s.span));
            } else {
                if (s.index_ident.has_value()) {
                    loop_body_stmts.push_back(hir_ctx.make<ir::HirLetStmt>(
                        s.index_ident->name,
                        ir::kInvalidHirId,
                        idx_var_for_item,
                        s.index_ident->span));
                }
                loop_body_stmts.push_back(hir_ctx.make<ir::HirLetStmt>(s.ident.name, ir::kInvalidHirId, item_expr, s.span));
            }

            auto lowered_user_body = lower_block(ctx, s.body, hir_ctx, diagnostics, type_names, enum_like, tmp_counter);
            if (lowered_user_body != ir::kInvalidHirId) {
                const auto& lowered_node = hir_ctx.node(lowered_user_body);
                if (lowered_node.kind == ir::HirKind::Block) {
                    const auto& lowered_block = hir_ctx.get<ir::HirBlock>(lowered_user_body);
                    loop_body_stmts.insert(loop_body_stmts.end(), lowered_block.stmts.begin(), lowered_block.stmts.end());
                } else {
                    loop_body_stmts.push_back(lowered_user_body);
                }
            }

            auto idx_var_lhs = hir_ctx.make<ir::HirVarExpr>(tmp_idx, s.span);
            auto idx_var_rhs = hir_ctx.make<ir::HirVarExpr>(tmp_idx, s.span);
            auto one = hir_ctx.make<ir::HirLiteralExpr>(ir::HirLiteralKind::Int, "1", s.span);
            auto plus_one = hir_ctx.make<ir::HirBinaryExpr>(ir::HirBinaryOp::Add, idx_var_rhs, one, s.span);
            auto assign_next = hir_ctx.make<ir::HirBinaryExpr>(ir::HirBinaryOp::Assign, idx_var_lhs, plus_one, s.span);
            loop_body_stmts.push_back(hir_ctx.make<ir::HirExprStmt>(assign_next, s.span));

            auto loop_body = hir_ctx.make<ir::HirBlock>(std::move(loop_body_stmts), s.span);
            lowered.push_back(hir_ctx.make<ir::HirLoop>(loop_body, s.span));
            return hir_ctx.make<ir::HirBlock>(std::move(lowered), s.span);
        }
        case NodeKind::SelectStmt: {
            auto& s = static_cast<const SelectStmt&>(node);
            std::vector<ir::HirStmtId> whens;
            for (auto w_id : s.whens) {
                if (w_id == kInvalidAstId) {
                    continue;
                }
                auto& w = static_cast<const WhenStmt&>(ctx.node(w_id));
                whens.push_back(hir_ctx.make<ir::HirWhen>(
                    lower_pattern(ctx, w.pattern, hir_ctx, diagnostics),
                    w.guard != kInvalidAstId
                        ? lower_expr(ctx, w.guard, hir_ctx, diagnostics, type_names, enum_like)
                        : ir::kInvalidHirId,
                    lower_block(ctx, w.block, hir_ctx, diagnostics, type_names, enum_like, tmp_counter),
                    w.span));
            }
            ir::HirStmtId otherwise_block = ir::kInvalidHirId;
            if (s.otherwise_block != kInvalidAstId) {
                otherwise_block = lower_block(ctx, s.otherwise_block, hir_ctx, diagnostics, type_names, enum_like, tmp_counter);
            }
            return hir_ctx.make<ir::HirSelect>(
                lower_expr(ctx, s.expr, hir_ctx, diagnostics, type_names, enum_like),
                std::move(whens),
                otherwise_block,
                s.span);
        }
        case NodeKind::WhenStmt: {
            auto& w = static_cast<const WhenStmt&>(node);
            return hir_ctx.make<ir::HirWhen>(
                lower_pattern(ctx, w.pattern, hir_ctx, diagnostics),
                w.guard != kInvalidAstId
                    ? lower_expr(ctx, w.guard, hir_ctx, diagnostics, type_names, enum_like)
                    : ir::kInvalidHirId,
                lower_block(ctx, w.block, hir_ctx, diagnostics, type_names, enum_like, tmp_counter),
                w.span);
        }
        default:
            diag::error(diagnostics, diag::DiagId::UnsupportedStatementInHir, node.span);
            return ir::kInvalidHirId;
    }
}

ir::HirModuleId lower_to_hir(
    const ast::AstContext& ctx,
    ast::ModuleId module_id,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics)
{
    if (module_id == kInvalidAstId) {
        return ir::kInvalidHirId;
    }

    const auto& module = ctx.get<Module>(module_id);
    std::vector<ir::HirDeclId> decls;
    std::unordered_set<std::string> type_names;
    std::unordered_map<std::string, bool> enum_like;
    std::unordered_map<std::string, std::vector<std::string>> ctor_fields;
    std::size_t tmp_counter = 0;

    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidAstId) {
            continue;
        }
        const auto& decl = ctx.get<Decl>(decl_id);
        if (decl.kind == NodeKind::TypeDecl) {
            type_names.insert(static_cast<const TypeDecl&>(decl).name.name);
        } else if (decl.kind == NodeKind::FormDecl) {
            const auto& form = static_cast<const FormDecl&>(decl);
            type_names.insert(form.name.name);
            std::vector<std::string> fields;
            fields.reserve(form.fields.size());
            for (const auto& field : form.fields) {
                fields.push_back(field.ident.name);
            }
            ctor_fields[form.name.name] = std::move(fields);
        } else if (decl.kind == NodeKind::PickDecl) {
            const auto& p = static_cast<const PickDecl&>(decl);
            type_names.insert(p.name.name);
            bool is_enum = true;
            for (const auto& c : p.cases) {
                if (!c.fields.empty()) {
                    is_enum = false;
                    break;
                }
            }
            enum_like[p.name.name] = is_enum;
        }
    }

    g_ctor_fields = &ctor_fields;

    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidAstId) {
            continue;
        }
        const auto& decl = ctx.get<Decl>(decl_id);
        switch (decl.kind) {
            case NodeKind::ProcDecl: {
                auto& d = static_cast<const ProcDecl&>(decl);
                std::vector<ir::HirParam> params;
                for (const auto& p : d.params) {
                    params.emplace_back(p.ident.name, lower_type(ctx, p.type, hir_ctx));
                }
                decls.push_back(hir_ctx.make<ir::HirFnDecl>(
                    d.name.name,
                    std::move(params),
                    lower_type(ctx, d.return_type, hir_ctx),
                    d.body != kInvalidAstId
                        ? lower_block(ctx, d.body, hir_ctx, diagnostics, type_names, enum_like, tmp_counter)
                        : ir::kInvalidHirId,
                    !d.type_params.empty(),
                    d.span));
                break;
            }
            case NodeKind::MacroDecl: {
                auto& d = static_cast<const MacroDecl&>(decl);
                std::vector<ir::HirParam> params;
                for (const auto& p : d.params) {
                    params.emplace_back(p.name, ir::kInvalidHirId);
                }
                decls.push_back(hir_ctx.make<ir::HirFnDecl>(
                    d.name.name,
                    std::move(params),
                    ir::kInvalidHirId,
                    lower_block(ctx, d.body, hir_ctx, diagnostics, type_names, enum_like, tmp_counter),
                    false,
                    d.span));
                break;
            }
            case NodeKind::ConstDecl: {
                auto& d = static_cast<const ConstDecl&>(decl);
                decls.push_back(hir_ctx.make<ir::HirConstDecl>(
                    d.name.name,
                    lower_type(ctx, d.type, hir_ctx),
                    lower_expr(ctx, d.value, hir_ctx, diagnostics, type_names, enum_like),
                    d.span));
                break;
            }
            case NodeKind::GlobalDecl: {
                auto& d = static_cast<const GlobalDecl&>(decl);
                decls.push_back(hir_ctx.make<ir::HirGlobalDecl>(
                    d.name.name,
                    lower_type(ctx, d.type, hir_ctx),
                    lower_expr(ctx, d.value, hir_ctx, diagnostics, type_names, enum_like),
                    d.is_mut,
                    d.span));
                break;
            }
            case NodeKind::EntryDecl: {
                auto& d = static_cast<const EntryDecl&>(decl);
                std::vector<ir::HirParam> params;
                decls.push_back(hir_ctx.make<ir::HirFnDecl>(
                    d.name.name,
                    std::move(params),
                    ir::kInvalidHirId,
                    lower_block(ctx, d.body, hir_ctx, diagnostics, type_names, enum_like, tmp_counter),
                    false,
                    d.span));
                break;
            }
            case NodeKind::FormDecl: {
                auto& d = static_cast<const FormDecl&>(decl);
                std::vector<ir::HirFieldDecl> fields;
                for (const auto& f : d.fields) {
                    fields.emplace_back(f.ident.name, lower_type(ctx, f.type, hir_ctx));
                }
                decls.push_back(hir_ctx.make<ir::HirFormDecl>(
                    d.name.name,
                    std::move(fields),
                    !d.type_params.empty(),
                    d.span));
                break;
            }
            case NodeKind::PickDecl: {
                auto& d = static_cast<const PickDecl&>(decl);
                std::vector<ir::HirPickCase> cases;
                bool is_enum = true;
                for (const auto& c : d.cases) {
                    std::vector<ir::HirFieldDecl> fields;
                    for (const auto& f : c.fields) {
                        fields.emplace_back(f.ident.name, lower_type(ctx, f.type, hir_ctx));
                    }
                    if (!fields.empty()) {
                        is_enum = false;
                    }
                    cases.emplace_back(c.ident.name, std::move(fields));
                }
                decls.push_back(hir_ctx.make<ir::HirPickDecl>(
                    d.name.name,
                    std::move(cases),
                    is_enum,
                    !d.type_params.empty(),
                    d.span));
                break;
            }
            default:
                break;
        }
    }

    g_ctor_fields = nullptr;

    return hir_ctx.make<ir::HirModule>(module.name, std::move(decls), module.span);
}

} // namespace vitte::frontend::lower
