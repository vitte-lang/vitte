#include "lower_mir.hpp"

#include <unordered_map>

namespace vitte::ir::lower {

using namespace vitte::ir;
using vitte::frontend::diag::DiagnosticEngine;

namespace {

struct Builder {
    const HirContext& hir;
    DiagnosticEngine& diag;
    MirFunction* func = nullptr;
    MirBlockId current = 0;
    bool terminated = false;
    std::size_t temp_index = 0;
    std::unordered_map<std::string, std::string> local_types;

    explicit Builder(const HirContext& h, DiagnosticEngine& d)
        : hir(h), diag(d) {}

    MirBasicBlock& block(MirBlockId id) {
        return func->blocks.at(id);
    }

    MirBlockId new_block(vitte::frontend::ast::SourceSpan span) {
        MirBlockId id = func->blocks.size();
        func->blocks.emplace_back(id, span);
        return id;
    }

    void set_current(MirBlockId id) {
        current = id;
        terminated = false;
    }

    void emit(MirInstrPtr instr) {
        if (terminated) {
            return;
        }
        block(current).instructions.push_back(std::move(instr));
    }

    void terminate(MirTerminatorPtr term) {
        if (terminated) {
            return;
        }
        block(current).set_terminator(std::move(term));
        terminated = true;
    }

    MirTypePtr type_named(const std::string& name, vitte::frontend::ast::SourceSpan span) {
        return std::make_unique<MirNamedType>(name, span);
    }

    std::string next_temp() {
        return "_t" + std::to_string(temp_index++);
    }

    MirLocalPtr make_local(const std::string& name, const std::string& type_name, vitte::frontend::ast::SourceSpan span) {
        return std::make_unique<MirLocal>(name, type_named(type_name, span), span);
    }

    MirValuePtr make_local_value(const std::string& name, const std::string& type_name, vitte::frontend::ast::SourceSpan span) {
        return std::make_unique<MirLocal>(name, type_named(type_name, span), span);
    }

    void register_local(const std::string& name, const std::string& type_name, vitte::frontend::ast::SourceSpan span) {
        func->locals.push_back(make_local(name, type_name, span));
    }

    MirValuePtr make_const(MirConstKind kind, const std::string& value, vitte::frontend::ast::SourceSpan span) {
        return std::make_unique<MirConst>(kind, value, span);
    }

    std::string type_from_hir(HirTypeId ty, const vitte::frontend::ast::SourceSpan& span) {
        if (ty == kInvalidHirId) {
            return "unknown";
        }
        const auto& node = hir.node(ty);
        switch (node.kind) {
            case HirKind::NamedType: {
                const auto& t = hir.get<HirNamedType>(ty);
                return t.name;
            }
            case HirKind::GenericType: {
                const auto& t = hir.get<HirGenericType>(ty);
                return t.base_name;
            }
            default:
                break;
        }
        (void)span;
        return "unknown";
    }

    std::string type_from_literal(HirLiteralKind kind) {
        switch (kind) {
            case HirLiteralKind::Bool: return "bool";
            case HirLiteralKind::Int: return "i32";
            case HirLiteralKind::String: return "string";
        }
        return "unknown";
    }

    std::string type_from_binop(HirBinaryOp op) {
        switch (op) {
            case HirBinaryOp::Eq:
            case HirBinaryOp::Ne:
            case HirBinaryOp::Lt:
            case HirBinaryOp::Le:
            case HirBinaryOp::Gt:
            case HirBinaryOp::Ge:
            case HirBinaryOp::And:
            case HirBinaryOp::Or:
                return "bool";
            default:
                return "i32";
        }
    }

    MirBinOp to_mir_binop(HirBinaryOp op) {
        switch (op) {
            case HirBinaryOp::Add: return MirBinOp::Add;
            case HirBinaryOp::Sub: return MirBinOp::Sub;
            case HirBinaryOp::Mul: return MirBinOp::Mul;
            case HirBinaryOp::Div: return MirBinOp::Div;
            case HirBinaryOp::Eq: return MirBinOp::Eq;
            case HirBinaryOp::Ne: return MirBinOp::Ne;
            case HirBinaryOp::Lt: return MirBinOp::Lt;
            case HirBinaryOp::Le: return MirBinOp::Le;
            case HirBinaryOp::Gt: return MirBinOp::Gt;
            case HirBinaryOp::Ge: return MirBinOp::Ge;
            case HirBinaryOp::And: return MirBinOp::And;
            case HirBinaryOp::Or: return MirBinOp::Or;
            default:
                return MirBinOp::Add;
        }
    }

    MirValuePtr lower_expr(HirExprId expr_id);
    void lower_stmt(HirStmtId stmt_id);
    void lower_block(HirStmtId block_id);
};

MirValuePtr Builder::lower_expr(HirExprId expr_id) {
    if (expr_id == kInvalidHirId) {
        return nullptr;
    }
    const auto& node = hir.node(expr_id);
    switch (node.kind) {
        case HirKind::LiteralExpr: {
            const auto& e = hir.get<HirLiteralExpr>(expr_id);
            MirConstKind k = MirConstKind::Int;
            switch (e.lit_kind) {
                case HirLiteralKind::Bool: k = MirConstKind::Bool; break;
                case HirLiteralKind::Int: k = MirConstKind::Int; break;
                case HirLiteralKind::String: k = MirConstKind::String; break;
            }
            return make_const(k, e.value, e.span);
        }
        case HirKind::VarExpr: {
            const auto& e = hir.get<HirVarExpr>(expr_id);
            std::string ty = "unknown";
            auto it = local_types.find(e.name);
            if (it != local_types.end()) {
                ty = it->second;
            }
            return make_local_value(e.name, ty, e.span);
        }
        case HirKind::UnaryExpr: {
            const auto& e = hir.get<HirUnaryExpr>(expr_id);
            auto rhs = lower_expr(e.expr);
            if (!rhs) {
                diag.error("invalid operand for unary expression", e.span);
                rhs = make_const(MirConstKind::Int, "0", e.span);
            }
            std::string tmp = next_temp();
            std::string ty = "i32";
            if (rhs && rhs->kind == MirKind::Const) {
                auto* c = static_cast<MirConst*>(rhs.get());
                if (c->const_kind == MirConstKind::Bool) {
                    ty = "bool";
                }
            }
            auto dest = make_local(tmp, ty, e.span);
            MirLocalPtr dest_copy = make_local(tmp, ty, e.span);
            if (e.op == HirUnaryOp::Neg) {
                auto zero = make_const(MirConstKind::Int, "0", e.span);
                emit(std::make_unique<MirBinaryOp>(
                    MirBinOp::Sub,
                    std::move(dest),
                    std::move(zero),
                    std::move(rhs),
                    e.span));
            } else {
                auto zero = make_const(MirConstKind::Int, "0", e.span);
                emit(std::make_unique<MirBinaryOp>(
                    MirBinOp::Eq,
                    std::move(dest),
                    std::move(rhs),
                    std::move(zero),
                    e.span));
            }
            register_local(tmp, ty, e.span);
            return dest_copy;
        }
        case HirKind::BinaryExpr: {
            const auto& e = hir.get<HirBinaryExpr>(expr_id);
            auto lhs = lower_expr(e.lhs);
            auto rhs = lower_expr(e.rhs);
            if (!lhs || !rhs) {
                diag.error("invalid operand for binary expression", e.span);
                if (!lhs) lhs = make_const(MirConstKind::Int, "0", e.span);
                if (!rhs) rhs = make_const(MirConstKind::Int, "0", e.span);
            }
            std::string tmp = next_temp();
            std::string ty = type_from_binop(e.op);
            auto dest = make_local(tmp, ty, e.span);
            MirLocalPtr dest_copy = make_local(tmp, ty, e.span);
            emit(std::make_unique<MirBinaryOp>(
                to_mir_binop(e.op),
                std::move(dest),
                std::move(lhs),
                std::move(rhs),
                e.span));
            register_local(tmp, ty, e.span);
            return dest_copy;
        }
        case HirKind::CallExpr: {
            const auto& e = hir.get<HirCallExpr>(expr_id);
            std::string callee = "<unknown>";
            if (e.callee != kInvalidHirId) {
                const auto& cnode = hir.node(e.callee);
                if (cnode.kind == HirKind::VarExpr) {
                    callee = hir.get<HirVarExpr>(e.callee).name;
                }
            }
            std::vector<MirValuePtr> args;
            for (auto a : e.args) {
                args.push_back(lower_expr(a));
            }
            if (callee == "asm" && !args.empty()) {
                if (args[0] && args[0]->kind == MirKind::Const) {
                    auto* c = static_cast<MirConst*>(args[0].get());
                    if (c->const_kind == MirConstKind::String) {
                        emit(std::make_unique<MirAsm>(c->value, true, e.span));
                        return nullptr;
                    }
                }
                diag.error("asm(...) expects a string literal", e.span);
                return nullptr;
            }
            if (callee == "unsafe_begin") {
                emit(std::make_unique<MirUnsafeBegin>(e.span));
                return nullptr;
            }
            if (callee == "unsafe_end") {
                emit(std::make_unique<MirUnsafeEnd>(e.span));
                return nullptr;
            }
            std::string tmp = next_temp();
            auto dest = make_local(tmp, "unknown", e.span);
            MirLocalPtr dest_copy = make_local(tmp, "unknown", e.span);
            emit(std::make_unique<MirCall>(
                callee,
                std::move(args),
                std::move(dest),
                e.span));
            register_local(tmp, "unknown", e.span);
            return dest_copy;
        }
        default:
            diag.error("unsupported HIR expression in MIR lowering", node.span);
            return nullptr;
    }
}

void Builder::lower_block(HirStmtId block_id) {
    if (block_id == kInvalidHirId) {
        return;
    }
    const auto& node = hir.node(block_id);
    if (node.kind != HirKind::Block) {
        lower_stmt(block_id);
        return;
    }
    const auto& b = hir.get<HirBlock>(block_id);
    for (auto s : b.stmts) {
        lower_stmt(s);
    }
}

void Builder::lower_stmt(HirStmtId stmt_id) {
    if (stmt_id == kInvalidHirId || terminated) {
        return;
    }
    const auto& node = hir.node(stmt_id);
    switch (node.kind) {
        case HirKind::LetStmt: {
            const auto& s = hir.get<HirLetStmt>(stmt_id);
            std::string ty = type_from_hir(s.type, s.span);
            local_types[s.name] = ty;
            register_local(s.name, ty, s.span);
            auto val = lower_expr(s.init);
            if (!val) {
                diag.error("invalid initializer for let (void expression)", s.span);
                val = make_const(MirConstKind::Int, "0", s.span);
            }
            auto dst = make_local(s.name, ty, s.span);
            emit(std::make_unique<MirAssign>(
                std::move(dst),
                std::move(val),
                s.span));
            break;
        }
        case HirKind::ExprStmt: {
            const auto& s = hir.get<HirExprStmt>(stmt_id);
            (void)lower_expr(s.expr);
            break;
        }
        case HirKind::ReturnStmt: {
            const auto& s = hir.get<HirReturnStmt>(stmt_id);
            auto val = lower_expr(s.expr);
            if (!val && s.expr != kInvalidHirId) {
                diag.error("return value must be a value expression", s.span);
                val = make_const(MirConstKind::Int, "0", s.span);
            }
            emit(std::make_unique<MirReturn>(std::move(val), s.span));
            terminated = true;
            break;
        }
        case HirKind::IfStmt: {
            const auto& s = hir.get<HirIf>(stmt_id);
            auto cond = lower_expr(s.cond);
            if (!cond) {
                diag.error("if condition must be a value expression", s.span);
                cond = make_const(MirConstKind::Bool, "false", s.span);
            }
            MirBlockId then_bb = new_block(s.span);
            MirBlockId else_bb = new_block(s.span);
            MirBlockId cont_bb = new_block(s.span);
            terminate(std::make_unique<MirCondGoto>(std::move(cond), then_bb, else_bb, s.span));

            set_current(then_bb);
            lower_block(s.then_block);
            if (!terminated) {
                terminate(std::make_unique<MirGoto>(cont_bb, s.span));
            }

            set_current(else_bb);
            if (s.else_block != kInvalidHirId) {
                lower_block(s.else_block);
            }
            if (!terminated) {
                terminate(std::make_unique<MirGoto>(cont_bb, s.span));
            }

            set_current(cont_bb);
            break;
        }
        case HirKind::SelectStmt: {
            const auto& s = hir.get<HirSelect>(stmt_id);
            auto sel_val = lower_expr(s.expr);
            std::string sel_tmp = next_temp();
            register_local(sel_tmp, "unknown", s.span);
            auto sel_dst = make_local(sel_tmp, "unknown", s.span);
            if (sel_val) {
                emit(std::make_unique<MirAssign>(
                    std::move(sel_dst),
                    std::move(sel_val),
                    s.span));
            }

            MirBlockId merge_bb = new_block(s.span);
            MirBlockId next_bb = new_block(s.span);
            terminate(std::make_unique<MirGoto>(next_bb, s.span));

            for (std::size_t i = 0; i < s.whens.size(); ++i) {
                set_current(next_bb);
                const auto& w = hir.get<HirWhen>(s.whens[i]);
                MirBlockId then_bb = new_block(w.span);
                MirBlockId else_bb = new_block(w.span);

                MirValuePtr cond;
                const auto& pnode = hir.node(w.pattern);
                if (pnode.kind == HirKind::PatternIdent) {
                    auto& pat = hir.get<HirIdentPattern>(w.pattern);
                    cond = make_const(MirConstKind::Bool, "true", w.span);
                } else {
                    diag.error("unsupported pattern in select/match (only ident supported)", w.span);
                    cond = make_const(MirConstKind::Bool, "false", w.span);
                }
                terminate(std::make_unique<MirCondGoto>(std::move(cond), then_bb, else_bb, w.span));

                set_current(then_bb);
                if (pnode.kind == HirKind::PatternIdent) {
                    auto& pat = hir.get<HirIdentPattern>(w.pattern);
                    register_local(pat.name, "unknown", w.span);
                    emit(std::make_unique<MirAssign>(
                        make_local(pat.name, "unknown", w.span),
                        make_local_value(sel_tmp, "unknown", w.span),
                        w.span));
                }
                lower_block(w.block);
                if (!terminated) {
                    terminate(std::make_unique<MirGoto>(merge_bb, w.span));
                }

                next_bb = else_bb;
            }

            set_current(next_bb);
            if (s.otherwise_block != kInvalidHirId) {
                lower_block(s.otherwise_block);
            }
            if (!terminated) {
                terminate(std::make_unique<MirGoto>(merge_bb, s.span));
            }

            set_current(merge_bb);
            break;
        }
        case HirKind::Block: {
            lower_block(stmt_id);
            break;
        }
        default:
            diag.error("unsupported HIR statement in MIR lowering", node.span);
            break;
    }
}

} // namespace

MirModule lower_to_mir(
    const HirContext& hir_ctx,
    HirModuleId module_id,
    DiagnosticEngine& diagnostics) {
    if (module_id == kInvalidHirId) {
        return MirModule({}, {});
    }

    const auto& module = hir_ctx.get<HirModule>(module_id);
    std::vector<MirFunction> funcs;

    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidHirId) {
            continue;
        }
        const auto& decl = hir_ctx.get<HirDecl>(decl_id);
        if (decl.kind != HirKind::FnDecl) {
            continue;
        }
        const auto& fn = hir_ctx.get<HirFnDecl>(decl_id);

        std::vector<MirLocalPtr> locals;
        std::vector<MirBasicBlock> blocks;
        blocks.emplace_back(0, fn.span);

        MirFunction mir_fn(
            fn.name,
            std::move(locals),
            std::move(blocks),
            0,
            fn.span);

        Builder builder(hir_ctx, diagnostics);
        builder.func = &mir_fn;
        builder.set_current(0);

        for (const auto& p : fn.params) {
            std::string ty = builder.type_from_hir(p.type, fn.span);
            builder.local_types[p.name] = ty;
        }

        builder.lower_block(fn.body);

        funcs.push_back(std::move(mir_fn));
    }

    return MirModule(std::move(funcs), module.span);
}

} // namespace vitte::ir::lower
