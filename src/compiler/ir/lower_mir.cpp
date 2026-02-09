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
    std::unordered_map<std::string, MirProcType> proc_locals;
    const std::unordered_map<std::string, std::string>* fn_returns = nullptr;
    const std::unordered_map<std::string, std::pair<MirConstKind, std::string>>* consts = nullptr;
    const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>>* pick_cases = nullptr;
    const std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>* pick_tags = nullptr;
    const std::unordered_map<std::string, std::unordered_map<std::string, MirFieldType>>* form_fields = nullptr;

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

    void ensure_local(const std::string& name, const std::string& type_name, vitte::frontend::ast::SourceSpan span) {
        if (local_types.find(name) == local_types.end()) {
            local_types[name] = type_name;
            register_local(name, type_name, span);
        }
    }

    std::string type_for_local(const std::string& name) const {
        auto it = local_types.find(name);
        if (it != local_types.end()) {
            return it->second;
        }
        return "unknown";
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
                if (t.name == "int") return "i32";
                if (t.name == "bool") return "bool";
                if (t.name == "string") return "string";
                if (t.name.size() <= 2) {
                    bool all_upper = true;
                    for (char c : t.name) {
                        if (c < 'A' || c > 'Z') {
                            all_upper = false;
                            break;
                        }
                    }
                    if (all_upper) {
                        return "VitteAny";
                    }
                }
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

    MirTypePtr type_from_hir_type(HirTypeId ty, const vitte::frontend::ast::SourceSpan& span) {
        if (ty == kInvalidHirId) {
            return type_named("unknown", span);
        }
        const auto& node = hir.node(ty);
        if (node.kind == HirKind::ProcType) {
            const auto& t = hir.get<HirProcType>(ty);
            std::vector<std::string> params;
            params.reserve(t.params.size());
            for (auto p : t.params) {
                params.push_back(type_from_hir(p, span));
            }
            std::string ret = type_from_hir(t.return_type, span);
            return std::make_unique<MirProcType>(std::move(params), std::move(ret), span);
        }
        return type_named(type_from_hir(ty, span), span);
    }

    bool proc_sig_from_hir(
        HirTypeId ty,
        std::vector<std::string>& params,
        std::string& ret,
        const vitte::frontend::ast::SourceSpan& span
    ) {
        if (ty == kInvalidHirId) {
            return false;
        }
        const auto& node = hir.node(ty);
        if (node.kind != HirKind::ProcType) {
            return false;
        }
        const auto& t = hir.get<HirProcType>(ty);
        params.clear();
        params.reserve(t.params.size());
        for (auto p : t.params) {
            params.push_back(type_from_hir(p, span));
        }
        ret = type_from_hir(t.return_type, span);
        return true;
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
    MirValuePtr emit_call_value(
        const std::string& callee,
        std::vector<MirValuePtr> args,
        const std::string& result_type,
        vitte::frontend::ast::SourceSpan span);
    MirValuePtr emit_call_value_indirect(
        MirValuePtr callee,
        std::vector<MirValuePtr> args,
        const std::string& result_type,
        vitte::frontend::ast::SourceSpan span);
};

struct Binding {
    enum class Kind {
        FromValue,
        FromCtorField
    };
    Kind kind;
    std::string name;
    std::string base_local;
    std::size_t field_index;
    std::string field_name;
    vitte::frontend::ast::SourceSpan span;
};

struct PatternResult {
    MirValuePtr cond;
    std::vector<Binding> bindings;
};

static bool is_unit_type_name(const std::string& name) {
    if (name == "Unit" || name == "unit" || name == "void") {
        return true;
    }
    if (name.size() >= 4 && name.compare(name.size() - 4, 4, "Unit") == 0) {
        return true;
    }
    return false;
}

MirValuePtr Builder::emit_call_value(
    const std::string& callee,
    std::vector<MirValuePtr> args,
    const std::string& result_type,
    vitte::frontend::ast::SourceSpan span) {
    std::string tmp = next_temp();
    auto dest = make_local(tmp, result_type, span);
    MirLocalPtr dest_copy = make_local(tmp, result_type, span);
    emit(std::make_unique<MirCall>(
        callee,
        std::move(args),
        std::move(dest),
        span));
    register_local(tmp, result_type, span);
    return dest_copy;
}

MirValuePtr Builder::emit_call_value_indirect(
    MirValuePtr callee,
    std::vector<MirValuePtr> args,
    const std::string& result_type,
    vitte::frontend::ast::SourceSpan span) {
    std::string tmp = next_temp();
    auto dest = make_local(tmp, result_type, span);
    MirLocalPtr dest_copy = make_local(tmp, result_type, span);
    emit(std::make_unique<MirCallIndirect>(
        std::move(callee),
        std::move(args),
        std::move(dest),
        span));
    register_local(tmp, result_type, span);
    return dest_copy;
}

static PatternResult lower_pattern(
    Builder& b,
    HirPatternId pat_id,
    const std::string& base_local) {
    PatternResult out;
    const auto& pnode = b.hir.node(pat_id);
    if (pnode.kind == HirKind::PatternIdent) {
        auto& pat = b.hir.get<HirIdentPattern>(pat_id);
        out.cond = b.make_const(MirConstKind::Bool, "true", pat.span);
        out.bindings.push_back(Binding{
            Binding::Kind::FromValue,
            pat.name,
            base_local,
            0,
            "",
            pat.span
        });
        return out;
    }
    if (pnode.kind == HirKind::PatternCtor) {
        auto& pat = b.hir.get<HirCtorPattern>(pat_id);
        std::string pick_name = pat.name;
        std::string case_name = pat.name;
        auto dot = pat.name.rfind('.');
        if (dot != std::string::npos) {
            pick_name = pat.name.substr(0, dot);
            case_name = pat.name.substr(dot + 1);
        }
        std::size_t tag_val = 0;
        std::vector<std::string> field_names;
        bool found = false;
        if (b.pick_cases && b.pick_tags) {
            auto pit = b.pick_cases->find(pick_name);
            auto tit = b.pick_tags->find(pick_name);
            if (pit != b.pick_cases->end() && tit != b.pick_tags->end()) {
                auto cit = pit->second.find(case_name);
                auto tag_it = tit->second.find(case_name);
                if (cit != pit->second.end() && tag_it != tit->second.end()) {
                    field_names = cit->second;
                    tag_val = tag_it->second;
                    found = true;
                }
            }
        }
        if (!found || pat.args.size() > field_names.size()) {
            b.diag.error("unknown ctor pattern or field mismatch: " + pat.name, pat.span);
            out.cond = b.make_const(MirConstKind::Bool, "false", pat.span);
            return out;
        }

        auto tag_member = std::make_unique<MirMember>(
            b.make_local_value(base_local, b.type_for_local(base_local), pat.span),
            "__tag",
            false,
            pat.span);
        auto tag_const = b.make_const(MirConstKind::Int, std::to_string(tag_val), pat.span);
        std::string tmp = b.next_temp();
        auto dest = b.make_local(tmp, "bool", pat.span);
        MirLocalPtr dest_copy = b.make_local(tmp, "bool", pat.span);
        b.emit(std::make_unique<MirBinaryOp>(
            MirBinOp::Eq,
            std::move(dest),
            std::move(tag_member),
            std::move(tag_const),
            pat.span));
        b.register_local(tmp, "bool", pat.span);
        out.cond = std::move(dest_copy);

        for (std::size_t i = 0; i < pat.args.size(); ++i) {
            HirPatternId arg_pat = pat.args[i];
            const auto& anode = b.hir.node(arg_pat);
            std::string field_name = i < field_names.size() ? field_names[i] : "";
            if (anode.kind == HirKind::PatternIdent) {
                auto& arg = b.hir.get<HirIdentPattern>(arg_pat);
                out.bindings.push_back(Binding{
                    Binding::Kind::FromCtorField,
                    arg.name,
                    base_local,
                    i,
                    field_name,
                    arg.span
                });
            } else if (anode.kind == HirKind::PatternCtor) {
                std::string field_local = b.next_temp();
                auto field_val = std::make_unique<MirMember>(
                    b.make_local_value(base_local, b.type_for_local(base_local), pat.span),
                    field_name,
                    false,
                    pat.span);
                b.emit(std::make_unique<MirAssign>(
                    b.make_local(field_local, "unknown", pat.span),
                    std::move(field_val),
                    pat.span));
                b.register_local(field_local, "unknown", pat.span);
                PatternResult sub = lower_pattern(b, arg_pat, field_local);
                if (sub.cond) {
                    std::vector<MirValuePtr> and_args;
                    auto left = std::move(out.cond);
                    auto right = std::move(sub.cond);
                    if (!left) {
                        out.cond = std::move(right);
                    } else if (!right) {
                        out.cond = std::move(left);
                    } else {
                        std::string tmp = b.next_temp();
                        auto dest = b.make_local(tmp, "bool", pat.span);
                        MirLocalPtr dest_copy = b.make_local(tmp, "bool", pat.span);
                        b.emit(std::make_unique<MirBinaryOp>(
                            MirBinOp::And,
                            std::move(dest),
                            std::move(left),
                            std::move(right),
                            pat.span));
                        b.register_local(tmp, "bool", pat.span);
                        out.cond = std::move(dest_copy);
                    }
                }
                for (auto& bind : sub.bindings) {
                    out.bindings.push_back(std::move(bind));
                }
            } else {
                b.diag.error("unsupported pattern in ctor (only ident/ctor)", pat.span);
            }
        }
        return out;
    }

    b.diag.error("unsupported pattern in select/match", pnode.span);
    out.cond = b.make_const(MirConstKind::Bool, "false", pnode.span);
    return out;
}

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
            if (consts) {
                auto itc = consts->find(e.name);
                if (itc != consts->end()) {
                    return make_const(itc->second.first, itc->second.second, e.span);
                }
            }
            std::string ty = "unknown";
            auto it = local_types.find(e.name);
            if (it != local_types.end()) {
                ty = it->second;
            }
            return make_local_value(e.name, ty, e.span);
        }
        case HirKind::MemberExpr: {
            const auto& e = hir.get<HirMemberExpr>(expr_id);
            if (e.base_is_type) {
                std::string name = e.type_is_enum
                    ? (hir.get<HirVarExpr>(e.base).name + "::" + e.member)
                    : (hir.get<HirVarExpr>(e.base).name + "__" + e.member + "__value");
                return make_local_value(name, "unknown", e.span);
            }
            auto base = lower_expr(e.base);
            if (!base) {
                diag.error("invalid base for member expression", e.span);
                base = make_const(MirConstKind::Int, "0", e.span);
            }
            return std::make_unique<MirMember>(std::move(base), e.member, e.pointer, e.span);
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
            std::string ctor_return;
            bool is_direct = false;
            if (e.callee != kInvalidHirId) {
                const auto& cnode = hir.node(e.callee);
                if (cnode.kind == HirKind::VarExpr) {
                    callee = hir.get<HirVarExpr>(e.callee).name;
                    if (callee == "asm" || callee == "unsafe_begin" || callee == "unsafe_end") {
                        is_direct = true;
                    } else if (fn_returns && fn_returns->find(callee) != fn_returns->end()) {
                        is_direct = true;
                    } else {
                        is_direct = false;
                    }
                } else if (cnode.kind == HirKind::MemberExpr) {
                    const auto& m = hir.get<HirMemberExpr>(e.callee);
                    if (m.base_is_type && !m.type_is_enum) {
                        const auto& base = hir.get<HirVarExpr>(m.base);
                        callee = base.name + "__" + m.member;
                        ctor_return = base.name;
                        is_direct = true;
                    } else if (!m.base_is_type && m.base != kInvalidHirId) {
                        const auto& base_node = hir.node(m.base);
                        if (base_node.kind == HirKind::VarExpr) {
                            const auto& base = hir.get<HirVarExpr>(m.base);
                            if (base.name == "builtin") {
                                callee = "builtin." + m.member;
                                is_direct = true;
                            }
                        }
                    }
                }
            }
            std::vector<MirValuePtr> args;
            for (auto a : e.args) {
                args.push_back(lower_expr(a));
            }
            if (is_direct && callee == "asm" && !args.empty()) {
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
            if (is_direct && callee == "unsafe_begin") {
                emit(std::make_unique<MirUnsafeBegin>(e.span));
                return nullptr;
            }
            if (is_direct && callee == "unsafe_end") {
                emit(std::make_unique<MirUnsafeEnd>(e.span));
                return nullptr;
            }
            std::string ret_type = "unknown";
            if (fn_returns) {
                if (is_direct) {
                    auto it = fn_returns->find(callee);
                    if (it != fn_returns->end()) {
                        ret_type = it->second;
                    }
                }
            }
            if (is_direct && callee == "builtin.trap") {
                ret_type = "void";
            }
            if (ret_type == "unknown" && !ctor_return.empty()) {
                ret_type = ctor_return;
            }
            if (!is_direct && e.callee != kInvalidHirId) {
                const auto& cnode = hir.node(e.callee);
                if (cnode.kind == HirKind::VarExpr) {
                    const auto& v = hir.get<HirVarExpr>(e.callee);
                    auto it = proc_locals.find(v.name);
                    if (it != proc_locals.end()) {
                        ret_type = it->second.ret;
                    }
                }
            }
            if (!is_direct && e.callee != kInvalidHirId) {
                const auto& cnode = hir.node(e.callee);
                if (cnode.kind == HirKind::MemberExpr) {
                    const auto& m = hir.get<HirMemberExpr>(e.callee);
                    if (!m.base_is_type && m.base != kInvalidHirId && form_fields) {
                        const auto& base_node = hir.node(m.base);
                        if (base_node.kind == HirKind::VarExpr) {
                            const auto& base = hir.get<HirVarExpr>(m.base);
                            auto it_base = local_types.find(base.name);
                            if (it_base != local_types.end()) {
                                auto it_form = form_fields->find(it_base->second);
                                if (it_form != form_fields->end()) {
                                    auto it_field = it_form->second.find(m.member);
                                    if (it_field != it_form->second.end()) {
                                        if (it_field->second.kind == MirFieldType::Kind::Proc) {
                                            ret_type = it_field->second.ret;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (is_unit_type_name(ret_type)) {
                if (is_direct) {
                    emit(std::make_unique<MirCall>(
                        callee,
                        std::move(args),
                        nullptr,
                        e.span));
                } else {
                    auto callee_val = lower_expr(e.callee);
                    if (!callee_val) {
                        diag.error("call target must be a value", e.span);
                        return nullptr;
                    }
                    emit(std::make_unique<MirCallIndirect>(
                        std::move(callee_val),
                        std::move(args),
                        nullptr,
                        e.span));
                }
                return nullptr;
            }
            if (is_direct) {
                return emit_call_value(callee, std::move(args), ret_type, e.span);
            }
            auto callee_val = lower_expr(e.callee);
            if (!callee_val) {
                diag.error("call target must be a value", e.span);
                return nullptr;
            }
            return emit_call_value_indirect(std::move(callee_val), std::move(args), ret_type, e.span);
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
            auto val = lower_expr(s.init);
            std::string ty = type_from_hir(s.type, s.span);
            if (ty == "unknown" && val && val->kind == MirKind::Local) {
                const auto& vlocal = static_cast<const MirLocal&>(*val);
                if (vlocal.type && vlocal.type->kind == MirKind::NamedType) {
                    ty = static_cast<const MirNamedType&>(*vlocal.type).name;
                }
            }
            local_types[s.name] = ty;
            register_local(s.name, ty, s.span);
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
        case HirKind::LoopStmt: {
            const auto& s = hir.get<HirLoop>(stmt_id);
            MirBlockId loop_bb = new_block(s.span);
            MirBlockId cont_bb = new_block(s.span);

            terminate(std::make_unique<MirGoto>(loop_bb, s.span));

            set_current(loop_bb);
            lower_block(s.body);
            if (!terminated) {
                terminate(std::make_unique<MirGoto>(loop_bb, s.span));
            }

            set_current(cont_bb);
            break;
        }
        case HirKind::SelectStmt: {
            const auto& s = hir.get<HirSelect>(stmt_id);
            auto sel_val = lower_expr(s.expr);
            std::string sel_type = "unknown";
            if (sel_val && sel_val->kind == MirKind::Local) {
                const auto& l = static_cast<const MirLocal&>(*sel_val);
                if (l.type && l.type->kind == MirKind::NamedType) {
                    sel_type = static_cast<const MirNamedType&>(*l.type).name;
                }
            }
            std::string sel_tmp = next_temp();
            register_local(sel_tmp, sel_type, s.span);
            auto sel_dst = make_local(sel_tmp, sel_type, s.span);
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

                PatternResult pr = lower_pattern(*this, w.pattern, sel_tmp);
                MirValuePtr cond = std::move(pr.cond);
                if (!cond) {
                    cond = make_const(MirConstKind::Bool, "false", w.span);
                }
                terminate(std::make_unique<MirCondGoto>(std::move(cond), then_bb, else_bb, w.span));

                set_current(then_bb);
                for (const auto& bind : pr.bindings) {
                    if (bind.kind == Binding::Kind::FromValue) {
                        ensure_local(bind.name, type_for_local(bind.base_local), bind.span);
                        emit(std::make_unique<MirAssign>(
                            make_local(bind.name, type_for_local(bind.base_local), bind.span),
                            make_local_value(bind.base_local, type_for_local(bind.base_local), bind.span),
                            bind.span));
                    } else if (bind.kind == Binding::Kind::FromCtorField) {
                        ensure_local(bind.name, "unknown", bind.span);
                        auto field_val = std::make_unique<MirMember>(
                            make_local_value(bind.base_local, type_for_local(bind.base_local), bind.span),
                            bind.field_name,
                            false,
                            bind.span);
                        emit(std::make_unique<MirAssign>(
                            make_local(bind.name, "unknown", bind.span),
                            std::move(field_val),
                            bind.span));
                    }
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
        return MirModule({}, {}, {}, {}, {}, {});
    }

    const auto& module = hir_ctx.get<HirModule>(module_id);
    std::vector<MirStructDecl> structs;
    std::vector<MirEnumDecl> enums;
    std::vector<MirPickDecl> picks;
    std::vector<MirFunction> funcs;
    std::vector<MirGlobal> globals;

    auto is_type_param = [&](const std::string& name) -> bool {
        if (name.empty() || name.size() > 2) {
            return false;
        }
        for (char c : name) {
            if (c < 'A' || c > 'Z') {
                return false;
            }
        }
        return true;
    };

    auto normalize_type_name = [&](const std::string& name) -> std::string {
        if (is_type_param(name)) {
            return "VitteAny";
        }
        return name;
    };

    auto type_name_from_hir = [&](HirTypeId ty) -> std::string {
        if (ty == kInvalidHirId) {
            return "i32";
        }
        const auto& tnode = hir_ctx.node(ty);
        if (tnode.kind == HirKind::NamedType) {
            std::string ret = hir_ctx.get<HirNamedType>(ty).name;
            if (ret == "int") ret = "i32";
            if (ret == "bool") ret = "bool";
            if (ret == "string") ret = "string";
            return normalize_type_name(ret);
        }
        if (tnode.kind == HirKind::GenericType) {
            return normalize_type_name(hir_ctx.get<HirGenericType>(ty).base_name);
        }
        return "i32";
    };

    auto field_type_from_hir = [&](HirTypeId ty) -> MirFieldType {
        MirFieldType out;
        if (ty == kInvalidHirId) {
            out.kind = MirFieldType::Kind::Named;
            out.name = "i32";
            return out;
        }
        const auto& node = hir_ctx.node(ty);
        if (node.kind == HirKind::ProcType) {
            const auto& t = hir_ctx.get<HirProcType>(ty);
            out.kind = MirFieldType::Kind::Proc;
            out.ret = normalize_type_name(type_name_from_hir(t.return_type));
            for (auto p : t.params) {
                out.params.push_back(normalize_type_name(type_name_from_hir(p)));
            }
            return out;
        }
        out.kind = MirFieldType::Kind::Named;
        out.name = normalize_type_name(type_name_from_hir(ty));
        return out;
    };

    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> pick_cases;
    std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>> pick_tags;
    std::unordered_map<std::string, std::unordered_map<std::string, MirFieldType>> form_fields;

    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidHirId) {
            continue;
        }
        const auto& decl = hir_ctx.get<HirDecl>(decl_id);
        if (decl.kind == HirKind::FormDecl) {
            const auto& f = hir_ctx.get<HirFormDecl>(decl_id);
            std::vector<MirField> fields;
            for (const auto& field : f.fields) {
                auto ftype = field_type_from_hir(field.type);
                form_fields[f.name][field.name] = ftype;
                fields.emplace_back(field.name, std::move(ftype));
            }
            structs.emplace_back(f.name, std::move(fields));
        } else if (decl.kind == HirKind::PickDecl) {
            const auto& p = hir_ctx.get<HirPickDecl>(decl_id);
            std::vector<MirPickCase> cases;
            std::size_t tag = 0;
            for (const auto& c : p.cases) {
                std::vector<MirField> fields;
                std::vector<std::string> field_names;
                for (const auto& field : c.fields) {
                    fields.emplace_back(field.name, field_type_from_hir(field.type));
                    field_names.push_back(field.name);
                }
                pick_cases[p.name][c.name] = field_names;
                pick_tags[p.name][c.name] = tag++;
                cases.emplace_back(c.name, std::move(fields));
            }
            picks.emplace_back(p.name, p.enum_like, std::move(cases));
            if (p.enum_like) {
                std::vector<std::string> items;
                for (const auto& c : p.cases) {
                    items.push_back(c.name);
                }
                enums.emplace_back(p.name, std::move(items));
            }
        }
    }

    std::unordered_map<std::string, std::pair<MirConstKind, std::string>> consts;
    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidHirId) {
            continue;
        }
        const auto& decl = hir_ctx.get<HirDecl>(decl_id);
        if (decl.kind != HirKind::ConstDecl) {
            continue;
        }
        const auto& c = hir_ctx.get<HirConstDecl>(decl_id);
        MirConstKind kind = MirConstKind::Int;
        std::string value = "0";
        if (c.value != kInvalidHirId) {
            const auto& vnode = hir_ctx.node(c.value);
            if (vnode.kind == HirKind::LiteralExpr) {
                const auto& lit = hir_ctx.get<HirLiteralExpr>(c.value);
                switch (lit.lit_kind) {
                    case HirLiteralKind::Bool: kind = MirConstKind::Bool; break;
                    case HirLiteralKind::Int: kind = MirConstKind::Int; break;
                    case HirLiteralKind::String: kind = MirConstKind::String; break;
                }
                value = lit.value;
            } else {
                diagnostics.error("const expressions must be literals", c.span);
            }
        } else {
            diagnostics.error("const declaration missing value", c.span);
        }
        consts[c.name] = std::make_pair(kind, value);
    }

    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidHirId) {
            continue;
        }
        const auto& decl = hir_ctx.get<HirDecl>(decl_id);
        if (decl.kind != HirKind::GlobalDecl) {
            continue;
        }
        const auto& g = hir_ctx.get<HirGlobalDecl>(decl_id);
        MirConstKind kind = MirConstKind::Int;
        std::string value = "0";
        bool has_init = false;
        if (g.value != kInvalidHirId) {
            const auto& vnode = hir_ctx.node(g.value);
            if (vnode.kind == HirKind::LiteralExpr) {
                const auto& lit = hir_ctx.get<HirLiteralExpr>(g.value);
                switch (lit.lit_kind) {
                    case HirLiteralKind::Bool: kind = MirConstKind::Bool; break;
                    case HirLiteralKind::Int: kind = MirConstKind::Int; break;
                    case HirLiteralKind::String: kind = MirConstKind::String; break;
                }
                value = lit.value;
                has_init = true;
            } else {
                diagnostics.error("global initializers must be literals (for now)", g.span);
            }
        }
        globals.emplace_back(
            g.name,
            type_name_from_hir(g.type),
            g.is_mut,
            has_init,
            kind,
            value,
            g.span);
    }

    std::unordered_map<std::string, std::string> fn_returns;
    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidHirId) {
            continue;
        }
        const auto& decl = hir_ctx.get<HirDecl>(decl_id);
        if (decl.kind != HirKind::FnDecl) {
            continue;
        }
        const auto& fn = hir_ctx.get<HirFnDecl>(decl_id);
        std::string ret = "Unit";
        if (fn.return_type != kInvalidHirId) {
            const auto& tnode = hir_ctx.node(fn.return_type);
            if (tnode.kind == HirKind::NamedType) {
                ret = hir_ctx.get<HirNamedType>(fn.return_type).name;
                if (ret == "int") ret = "i32";
                if (ret == "bool") ret = "bool";
                if (ret == "string") ret = "string";
            } else if (tnode.kind == HirKind::GenericType) {
                ret = hir_ctx.get<HirGenericType>(fn.return_type).base_name;
            }
        }
        fn_returns[fn.name] = ret;
    }

    for (auto decl_id : module.decls) {
        if (decl_id == kInvalidHirId) {
            continue;
        }
        const auto& decl = hir_ctx.get<HirDecl>(decl_id);
        if (decl.kind != HirKind::FnDecl) {
            continue;
        }
        const auto& fn = hir_ctx.get<HirFnDecl>(decl_id);

        std::vector<MirParam> params;
        std::vector<MirLocalPtr> locals;
        std::vector<MirBasicBlock> blocks;
        blocks.emplace_back(0, fn.span);

        MirTypePtr ret_type = std::make_unique<MirNamedType>("Unit", fn.span);

        MirFunction mir_fn(
            fn.name,
            std::move(params),
            std::move(ret_type),
            std::move(locals),
            std::move(blocks),
            0,
            fn.span);

        Builder builder(hir_ctx, diagnostics);
        builder.func = &mir_fn;
        builder.fn_returns = &fn_returns;
        builder.consts = &consts;
        builder.pick_cases = &pick_cases;
        builder.pick_tags = &pick_tags;
        builder.form_fields = &form_fields;
        builder.set_current(0);

        std::string ret_name = "Unit";
        if (fn.return_type != kInvalidHirId) {
            ret_name = builder.type_from_hir(fn.return_type, fn.span);
        }
        mir_fn.return_type = std::make_unique<MirNamedType>(ret_name, fn.span);

        for (const auto& p : fn.params) {
            std::string ty_name = builder.type_from_hir(p.type, fn.span);
            mir_fn.params.emplace_back(p.name, builder.type_from_hir_type(p.type, fn.span));
            builder.ensure_local(p.name, ty_name, fn.span);
            std::vector<std::string> proc_params;
            std::string proc_ret;
            if (builder.proc_sig_from_hir(p.type, proc_params, proc_ret, fn.span)) {
                builder.proc_locals.emplace(
                    p.name,
                    MirProcType(std::move(proc_params), std::move(proc_ret), fn.span));
            }
        }

        builder.lower_block(fn.body);

        funcs.push_back(std::move(mir_fn));
    }

    return MirModule(
        std::move(structs),
        std::move(enums),
        std::move(picks),
        std::move(globals),
        std::move(funcs),
        module.span);
}

} // namespace vitte::ir::lower
