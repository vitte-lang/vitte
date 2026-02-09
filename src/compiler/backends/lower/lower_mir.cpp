#include "lower_mir.hpp"

#include "../ast/cpp_expr.hpp"
#include "../ast/cpp_stmt.hpp"
#include "../ast/cpp_decl.hpp"
#include "../ast/cpp_type.hpp"
#include "../context/cpp_context.hpp"

#include <algorithm>
#include <unordered_set>

namespace vitte::backend::lower {

using namespace vitte::backend::ast::cpp;
using vitte::backend::context::CppContext;
using vitte::ir::MirKind;

namespace {

static CppType* builtin_type(CppContext& ctx, const std::string& name) {
    if (auto* t = ctx.resolve_type(name)) {
        return t;
    }
    auto* ty = new CppType(CppType::builtin(name));
    ctx.register_type(name, ty);
    return ty;
}

static CppType* map_type(CppContext& ctx, const std::string& mir_name) {
    if (mir_name == "i32") return builtin_type(ctx, "int32_t");
    if (mir_name == "i64") return builtin_type(ctx, "int64_t");
    if (mir_name == "i16") return builtin_type(ctx, "int16_t");
    if (mir_name == "i8") return builtin_type(ctx, "int8_t");
    if (mir_name == "u64") return builtin_type(ctx, "uint64_t");
    if (mir_name == "u32") return builtin_type(ctx, "uint32_t");
    if (mir_name == "u16") return builtin_type(ctx, "uint16_t");
    if (mir_name == "u8") return builtin_type(ctx, "uint8_t");
    if (mir_name == "usize") return builtin_type(ctx, "size_t");
    if (mir_name == "isize") return builtin_type(ctx, "ptrdiff_t");
    if (mir_name == "bool") return builtin_type(ctx, "bool");
    if (mir_name == "string") return builtin_type(ctx, "VitteString");
    if (mir_name == "Unit" || mir_name == "unit" || mir_name == "void") {
        return builtin_type(ctx, "void");
    }
    if (mir_name.size() >= 4 && mir_name.compare(mir_name.size() - 4, 4, "Unit") == 0) {
        return builtin_type(ctx, "void");
    }
    return builtin_type(ctx, "int32_t");
}

static std::unique_ptr<CppExpr> emit_value(CppContext& ctx, const vitte::ir::MirValue& v) {
    (void)ctx;
    if (v.kind == MirKind::Local) {
        const auto& l = static_cast<const vitte::ir::MirLocal&>(v);
        return std::make_unique<CppVar>(l.name);
    }
    if (v.kind == MirKind::Const) {
        const auto& c = static_cast<const vitte::ir::MirConst&>(v);
        switch (c.const_kind) {
            case vitte::ir::MirConstKind::Bool:
                if (c.value == "true" || c.value == "false") {
                    return std::make_unique<CppLiteral>(c.value);
                }
                return std::make_unique<CppLiteral>(c.value == "0" ? "false" : "true");
            case vitte::ir::MirConstKind::Int:
                return std::make_unique<CppLiteral>(c.value);
            case vitte::ir::MirConstKind::String: {
                std::string lit = "\"";
                for (char ch : c.value) {
                    switch (ch) {
                        case '\\': lit += "\\\\"; break;
                        case '"': lit += "\\\""; break;
                        case '\n': lit += "\\n"; break;
                        case '\r': lit += "\\r"; break;
                        case '\t': lit += "\\t"; break;
                        default: lit += ch; break;
                    }
                }
                lit += "\"";
                std::string out = "VitteString{" + lit + ", " + std::to_string(c.value.size()) + "}";
                return std::make_unique<CppLiteral>(out);
            }
        }
    }
    return std::make_unique<CppLiteral>("0");
}

static std::unique_ptr<CppExpr> emit_const_expr(const vitte::ir::MirConstKind kind, const std::string& value) {
    switch (kind) {
        case vitte::ir::MirConstKind::Bool:
            if (value == "true" || value == "false") {
                return std::make_unique<CppLiteral>(value);
            }
            return std::make_unique<CppLiteral>(value == "0" ? "false" : "true");
        case vitte::ir::MirConstKind::Int:
            return std::make_unique<CppLiteral>(value);
        case vitte::ir::MirConstKind::String: {
            std::string lit = "\"";
            for (char ch : value) {
                switch (ch) {
                    case '\\': lit += "\\\\"; break;
                    case '"': lit += "\\\""; break;
                    case '\n': lit += "\\n"; break;
                    case '\r': lit += "\\r"; break;
                    case '\t': lit += "\\t"; break;
                    default: lit += ch; break;
                }
            }
            lit += "\"";
            std::string out = "VitteString{" + lit + ", " + std::to_string(value.size()) + "}";
            return std::make_unique<CppLiteral>(out);
        }
    }
    return std::make_unique<CppLiteral>("0");
}
static std::string binop_to_cpp(vitte::ir::MirBinOp op) {
    switch (op) {
        case vitte::ir::MirBinOp::Add: return "+";
        case vitte::ir::MirBinOp::Sub: return "-";
        case vitte::ir::MirBinOp::Mul: return "*";
        case vitte::ir::MirBinOp::Div: return "/";
        case vitte::ir::MirBinOp::Eq: return "==";
        case vitte::ir::MirBinOp::Ne: return "!=";
        case vitte::ir::MirBinOp::Lt: return "<";
        case vitte::ir::MirBinOp::Le: return "<=";
        case vitte::ir::MirBinOp::Gt: return ">";
        case vitte::ir::MirBinOp::Ge: return ">=";
        case vitte::ir::MirBinOp::And: return "&&";
        case vitte::ir::MirBinOp::Or: return "||";
        default: return "+";
    }
}

static std::string label_for(std::size_t fn_index, std::size_t block_id) {
    return "bb_" + std::to_string(fn_index) + "_" + std::to_string(block_id);
}

static std::string type_name(const vitte::ir::MirTypePtr& type) {
    if (!type) {
        return "i32";
    }
    if (type->kind == vitte::ir::MirKind::NamedType) {
        return static_cast<const vitte::ir::MirNamedType&>(*type).name;
    }
    return "i32";
}

static bool is_extern_fn(const vitte::ir::MirFunction& fn) {
    for (const auto& bb : fn.blocks) {
        if (!bb.instructions.empty() || bb.terminator) {
            return false;
        }
    }
    return true;
}

static std::vector<const vitte::ir::MirBasicBlock*> ordered_blocks(
    const vitte::ir::MirFunction& fn,
    bool repro_strict
) {
    std::vector<const vitte::ir::MirBasicBlock*> blocks;
    blocks.reserve(fn.blocks.size());
    for (const auto& bb : fn.blocks) {
        blocks.push_back(&bb);
    }
    if (repro_strict) {
        std::stable_sort(
            blocks.begin(),
            blocks.end(),
            [](const auto* a, const auto* b) { return a->id < b->id; }
        );
    }
    return blocks;
}

static std::vector<const vitte::ir::MirLocal*> ordered_locals(
    const vitte::ir::MirFunction& fn,
    bool repro_strict
) {
    std::vector<const vitte::ir::MirLocal*> locals;
    locals.reserve(fn.locals.size());
    for (const auto& local : fn.locals) {
        if (!local) continue;
        locals.push_back(local.get());
    }
    if (repro_strict) {
        std::stable_sort(
            locals.begin(),
            locals.end(),
            [](const auto* a, const auto* b) { return a->name < b->name; }
        );
    }
    return locals;
}

} // namespace

ast::cpp::CppTranslationUnit lower_mir(
    const vitte::ir::MirModule& module,
    CppContext& ctx
) {
    CppTranslationUnit tu;
    ctx.add_include("<cstdint>");
    ctx.add_include("<cstddef>");

    bool has_entry = false;
    std::string entry_mangled;

    for (const auto& g : module.globals) {
        CppGlobal glob;
        glob.name = g.name;
        glob.type = map_type(ctx, g.type_name);
        glob.is_const = !g.is_mut;
        if (g.has_init) {
            glob.init = emit_const_expr(g.init_kind, g.init_value);
        }
        tu.globals.push_back(std::move(glob));
    }

    std::unordered_set<std::string> externs;
    for (const auto& fn : module.functions) {
        if (is_extern_fn(fn)) {
            externs.insert(fn.name);
        }
    }

    std::size_t fn_index = 0;
    for (const auto& fn : module.functions) {
        CppFunction out;
        bool is_extern = externs.count(fn.name) > 0;
        out.name = is_extern ? fn.name : ctx.mangle(fn.name);
        out.return_type = map_type(ctx, type_name(fn.return_type));
        out.is_extern = is_extern;
        if (is_extern) {
            out.abi = "C";
        }
        if (fn.name == "main") {
            out.abi = "C";
        }

        std::unordered_set<std::string> param_names;
        for (const auto& p : fn.params) {
            auto ty = map_type(ctx, type_name(p.type));
            out.params.push_back({ty, p.name});
            param_names.insert(p.name);
        }

        std::unordered_set<std::string> declared;
        for (const auto* local : ordered_locals(fn, ctx.repro_strict())) {
            if (param_names.count(local->name) > 0) {
                continue;
            }
            if (declared.insert(local->name).second) {
                auto decl = std::make_unique<CppVarDecl>(
                    map_type(ctx, type_name(local->type)),
                    local->name
                );
                out.body.push_back(std::move(decl));
            }
        }

        if (is_extern) {
            tu.functions.push_back(std::move(out));
            ++fn_index;
            continue;
        }

        for (const auto* bb : ordered_blocks(fn, ctx.repro_strict())) {
            out.body.push_back(std::make_unique<CppLabel>(label_for(fn_index, bb->id)));

            for (const auto& instr : bb->instructions) {
                switch (instr->kind) {
                    case MirKind::Assign: {
                        auto& ins = static_cast<const vitte::ir::MirAssign&>(*instr);
                        const auto& dst = static_cast<const vitte::ir::MirLocal&>(*ins.dest);
                        if (declared.insert(dst.name).second) {
                            auto decl = std::make_unique<CppVarDecl>(
                                map_type(ctx, type_name(dst.type)),
                                dst.name
                            );
                            decl->init = emit_value(ctx, *ins.value);
                            out.body.push_back(std::move(decl));
                        } else {
                            out.body.push_back(std::make_unique<CppAssign>(
                                std::make_unique<CppVar>(dst.name),
                                emit_value(ctx, *ins.value)));
                        }
                        break;
                    }
                    case MirKind::BinaryOp: {
                        auto& ins = static_cast<const vitte::ir::MirBinaryOp&>(*instr);
                        const auto& dst = static_cast<const vitte::ir::MirLocal&>(*ins.dest);
                        auto expr = std::make_unique<CppBinary>(
                            binop_to_cpp(ins.op),
                            emit_value(ctx, *ins.left),
                            emit_value(ctx, *ins.right));
                        if (declared.insert(dst.name).second) {
                            auto decl = std::make_unique<CppVarDecl>(
                                map_type(ctx, type_name(dst.type)),
                                dst.name
                            );
                            decl->init = std::move(expr);
                            out.body.push_back(std::move(decl));
                        } else {
                            out.body.push_back(std::make_unique<CppAssign>(
                                std::make_unique<CppVar>(dst.name),
                                std::move(expr)));
                        }
                        break;
                    }
                    case MirKind::Call: {
                        auto& ins = static_cast<const vitte::ir::MirCall&>(*instr);
                        std::string callee_name = externs.count(ins.callee) > 0
                            ? ins.callee
                            : ctx.mangle(ins.callee);
                        auto call = std::make_unique<CppCall>(callee_name);
                        for (const auto& a : ins.args) {
                            if (a) {
                                call->args.push_back(emit_value(ctx, *a));
                            }
                        }
                        if (ins.result) {
                            const auto& dst = static_cast<const vitte::ir::MirLocal&>(*ins.result);
                            if (declared.insert(dst.name).second) {
                                auto decl = std::make_unique<CppVarDecl>(
                                    map_type(ctx, type_name(dst.type)),
                                    dst.name
                                );
                                decl->init = std::move(call);
                                out.body.push_back(std::move(decl));
                            } else {
                                out.body.push_back(std::make_unique<CppAssign>(
                                    std::make_unique<CppVar>(dst.name),
                                    std::move(call)));
                            }
                        } else {
                            out.body.push_back(std::make_unique<CppExprStmt>(std::move(call)));
                        }
                        break;
                    }
                    case MirKind::Asm: {
                        auto& ins = static_cast<const vitte::ir::MirAsm&>(*instr);
                        out.body.push_back(std::make_unique<CppAsm>(ins.code, ins.is_volatile));
                        break;
                    }
                    case MirKind::UnsafeBegin: {
                        out.body.push_back(std::make_unique<CppExprStmt>(
                            std::make_unique<CppLiteral>("/* unsafe begin */")));
                        break;
                    }
                    case MirKind::UnsafeEnd: {
                        out.body.push_back(std::make_unique<CppExprStmt>(
                            std::make_unique<CppLiteral>("/* unsafe end */")));
                        break;
                    }
                    case MirKind::Return: {
                        auto& ins = static_cast<const vitte::ir::MirReturn&>(*instr);
                        if (ins.value) {
                            out.body.push_back(std::make_unique<CppReturn>(
                                emit_value(ctx, *ins.value)));
                        } else {
                            out.body.push_back(std::make_unique<CppReturn>());
                        }
                        break;
                    }
                    default:
                        out.body.push_back(std::make_unique<CppExprStmt>(
                            std::make_unique<CppLiteral>("/* unsupported MIR */")));
                        break;
                }
            }

            if (bb->terminator) {
                switch (bb->terminator->kind) {
                    case MirKind::Goto: {
                        auto& term = static_cast<const vitte::ir::MirGoto&>(*bb->terminator);
                        out.body.push_back(std::make_unique<CppGoto>(label_for(fn_index, term.target)));
                        break;
                    }
                    case MirKind::CondGoto: {
                        auto& term = static_cast<const vitte::ir::MirCondGoto&>(*bb->terminator);
                        auto cond = emit_value(ctx, *term.cond);
                        auto if_stmt = std::make_unique<CppIf>(std::move(cond));
                        if_stmt->then_body.push_back(std::make_unique<CppGoto>(label_for(fn_index, term.then_block)));
                        if_stmt->else_body.push_back(std::make_unique<CppGoto>(label_for(fn_index, term.else_block)));
                        out.body.push_back(std::move(if_stmt));
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        tu.functions.push_back(std::move(out));
        if (fn.name == "main") {
            has_entry = true;
            entry_mangled = ctx.mangle(fn.name);
        }
        fn_index++;
    }

    if (has_entry) {
        if (ctx.entry_mode() == CppContext::EntryMode::Freestanding) {
            return tu;
        }
        if (ctx.entry_mode() == CppContext::EntryMode::Arduino) {
            CppFunction setup;
            setup.name = "setup";
            setup.return_type = builtin_type(ctx, "void");
            auto call = std::make_unique<CppCall>(entry_mangled);
            setup.body.push_back(std::make_unique<CppExprStmt>(std::move(call)));
            tu.functions.push_back(std::move(setup));

            CppFunction loop;
            loop.name = "loop";
            loop.return_type = builtin_type(ctx, "void");
            tu.functions.push_back(std::move(loop));
        } else {
            CppFunction wrapper;
            wrapper.name = "main";
            wrapper.return_type = map_type(ctx, "i32");
            wrapper.params.push_back({map_type(ctx, "i32"), "argc"});
            wrapper.params.push_back({builtin_type(ctx, "const char**"), "argv"});

            auto set_args = std::make_unique<CppCall>("vitte_set_args");
            set_args->args.push_back(std::make_unique<CppVar>("argc"));
            set_args->args.push_back(std::make_unique<CppVar>("argv"));
            wrapper.body.push_back(std::make_unique<CppExprStmt>(std::move(set_args)));

            auto call = std::make_unique<CppCall>(entry_mangled);
            wrapper.body.push_back(std::make_unique<CppReturn>(std::move(call)));

            tu.functions.push_back(std::move(wrapper));
        }
    }

    return tu;
}

} // namespace vitte::backend::lower
