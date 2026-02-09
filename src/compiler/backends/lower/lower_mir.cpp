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
    if (auto* t = ctx.resolve_type(mir_name)) {
        return t;
    }
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
    if (mir_name == "VitteAny") return builtin_type(ctx, "void*");
    if (mir_name == "unknown") return builtin_type(ctx, "int32_t");
    if (mir_name == "Unit" || mir_name == "unit" || mir_name == "void") {
        return builtin_type(ctx, "void");
    }
    if (mir_name.size() >= 4 && mir_name.compare(mir_name.size() - 4, 4, "Unit") == 0) {
        return builtin_type(ctx, "void");
    }
    auto* ty = new CppType(CppType::user(mir_name, CppTypeKind::Struct));
    ctx.register_type(mir_name, ty);
    return ty;
}

static CppType* map_type(CppContext& ctx, const vitte::ir::MirTypePtr& type) {
    if (!type) {
        return map_type(ctx, "i32");
    }
    if (type->kind == vitte::ir::MirKind::NamedType) {
        return map_type(ctx, static_cast<const vitte::ir::MirNamedType&>(*type).name);
    }
    if (type->kind == vitte::ir::MirKind::ProcType) {
        const auto& t = static_cast<const vitte::ir::MirProcType&>(*type);
        std::vector<CppType*> params;
        params.reserve(t.params.size());
        for (const auto& p : t.params) {
            params.push_back(map_type(ctx, p));
        }
        auto* ret = map_type(ctx, t.ret);
        return new CppType(CppType::function(ret, params));
    }
    return map_type(ctx, "i32");
}

static std::unique_ptr<CppExpr> emit_value(CppContext& ctx, const vitte::ir::MirValue& v) {
    if (v.kind == MirKind::Local) {
        const auto& l = static_cast<const vitte::ir::MirLocal&>(v);
        if (l.name.find("::") != std::string::npos) {
            return std::make_unique<CppVar>(l.name);
        }
        return std::make_unique<CppVar>(ctx.safe_ident(l.name));
    }
    if (v.kind == MirKind::Member) {
        const auto& m = static_cast<const vitte::ir::MirMember&>(v);
        auto base = emit_value(ctx, *m.base);
        return std::make_unique<CppMember>(std::move(base), ctx.safe_ident(m.member), m.pointer);
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

static bool is_unknown_type_name(const std::string& name) {
    return name == "unknown";
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

static CppType* map_field_type(CppContext& ctx, const vitte::ir::MirFieldType& t) {
    if (t.kind == vitte::ir::MirFieldType::Kind::Named) {
        return map_type(ctx, t.name);
    }
    std::vector<CppType*> params;
    for (const auto& p : t.params) {
        params.push_back(map_type(ctx, p));
    }
    auto* ret = map_type(ctx, t.ret);
    auto* ty = new CppType(CppType::function(ret, params));
    return ty;
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
    if (type->kind == vitte::ir::MirKind::ProcType) {
        return "proc";
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

    std::unordered_set<std::string> defined_structs;
    std::unordered_set<std::string> defined_enums;

    for (const auto& e : module.enums) {
        if (defined_enums.insert(e.name).second) {
            CppEnum en;
            en.name = e.name;
            for (const auto& it : e.items) {
                en.items.push_back({it, std::nullopt});
            }
            tu.enums.push_back(std::move(en));
            auto* ty = new CppType(CppType::user(e.name, CppTypeKind::Enum));
            ctx.register_type(e.name, ty);
        }
    }

    for (const auto& s : module.structs) {
        if (defined_structs.insert(s.name).second) {
            CppStruct st;
            st.name = s.name;
            for (const auto& f : s.fields) {
                st.fields.push_back({map_field_type(ctx, f.type), ctx.safe_ident(f.name)});
            }
            tu.structs.push_back(std::move(st));
            auto* ty = new CppType(CppType::user(s.name, CppTypeKind::Struct));
            ctx.register_type(s.name, ty);
        }
    }

    for (const auto& p : module.picks) {
        if (p.enum_like) {
            if (defined_enums.insert(p.name).second) {
                CppEnum en;
                en.name = p.name;
                for (const auto& c : p.cases) {
                    en.items.push_back({c.name, std::nullopt});
                }
                tu.enums.push_back(std::move(en));
                auto* ty = new CppType(CppType::user(p.name, CppTypeKind::Enum));
                ctx.register_type(p.name, ty);
            }
            continue;
        }
        if (defined_structs.insert(p.name).second) {
            CppStruct st;
            st.name = p.name;
            st.fields.push_back({builtin_type(ctx, "uint8_t"), "__tag"});
            std::unordered_set<std::string> field_names;
            for (const auto& c : p.cases) {
                for (const auto& f : c.fields) {
                    if (field_names.insert(f.name).second) {
                        st.fields.push_back({map_field_type(ctx, f.type), ctx.safe_ident(f.name)});
                    }
                }
            }
            tu.structs.push_back(std::move(st));
            auto* ty = new CppType(CppType::user(p.name, CppTypeKind::Struct));
            ctx.register_type(p.name, ty);
        }

        std::size_t tag = 0;
        for (const auto& c : p.cases) {
            CppFunction ctor;
            ctor.name = ctx.mangle(p.name + "__" + c.name);
            ctor.return_type = map_type(ctx, p.name);
            for (const auto& f : c.fields) {
                ctor.params.push_back({map_field_type(ctx, f.type), ctx.safe_ident(f.name)});
            }

            auto decl = std::make_unique<CppVarDecl>(map_type(ctx, p.name), "_v");
            ctor.body.push_back(std::move(decl));

            ctor.body.push_back(std::make_unique<CppAssign>(
                std::make_unique<CppMember>(std::make_unique<CppVar>("_v"), "__tag"),
                std::make_unique<CppLiteral>(std::to_string(tag))));

            for (const auto& f : c.fields) {
                ctor.body.push_back(std::make_unique<CppAssign>(
                    std::make_unique<CppMember>(std::make_unique<CppVar>("_v"), ctx.safe_ident(f.name)),
                    std::make_unique<CppVar>(ctx.safe_ident(f.name))));
            }

            ctor.body.push_back(std::make_unique<CppReturn>(std::make_unique<CppVar>("_v")));
            tu.functions.push_back(std::move(ctor));

            if (c.fields.empty()) {
                CppGlobal g;
                g.is_const = true;
                g.name = p.name + "__" + c.name + "__value";
                g.type = map_type(ctx, p.name);
                g.init = std::make_unique<CppCall>(ctx.mangle(p.name + "__" + c.name));
                tu.globals.push_back(std::move(g));
            }

            ++tag;
        }
    }

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
        out.return_type = map_type(ctx, fn.return_type);
        out.is_extern = is_extern;
        if (is_extern) {
            out.abi = "C";
        }
        if (fn.name == "main") {
            out.abi = "C";
        }

        std::unordered_set<std::string> param_names;
        for (const auto& p : fn.params) {
            auto ty = map_type(ctx, p.type);
            auto pname = ctx.safe_ident(p.name);
            out.params.push_back({ty, pname});
            param_names.insert(pname);
        }

        std::unordered_set<std::string> declared;
        for (const auto* local : ordered_locals(fn, ctx.repro_strict())) {
            const auto lname = ctx.safe_ident(local->name);
            if (param_names.count(lname) > 0) {
                continue;
            }
            if (is_unknown_type_name(type_name(local->type))) {
                continue;
            }
            if (declared.insert(lname).second) {
                auto decl = std::make_unique<CppVarDecl>(
                    map_type(ctx, local->type),
                    lname
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
                        auto dname = ctx.safe_ident(dst.name);
                        if (declared.insert(dname).second) {
                            auto decl_type = map_type(ctx, dst.type);
                            if (is_unknown_type_name(type_name(dst.type))) {
                                decl_type = builtin_type(ctx, "auto");
                            }
                            auto decl = std::make_unique<CppVarDecl>(
                                decl_type,
                                dname
                            );
                            decl->init = emit_value(ctx, *ins.value);
                            out.body.push_back(std::move(decl));
                        } else {
                            out.body.push_back(std::make_unique<CppAssign>(
                                std::make_unique<CppVar>(dname),
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
                        auto dname = ctx.safe_ident(dst.name);
                        if (declared.insert(dname).second) {
                            auto decl = std::make_unique<CppVarDecl>(
                                map_type(ctx, dst.type),
                                dname
                            );
                            decl->init = std::move(expr);
                            out.body.push_back(std::move(decl));
                        } else {
                            out.body.push_back(std::make_unique<CppAssign>(
                                std::make_unique<CppVar>(dname),
                                std::move(expr)));
                        }
                        break;
                    }
                    case MirKind::Call: {
                        auto& ins = static_cast<const vitte::ir::MirCall&>(*instr);
                        std::string callee_name;
                        if (ins.callee == "builtin.trap") {
                            callee_name = "vitte_builtin_trap";
                        } else if (externs.count(ins.callee) > 0) {
                            callee_name = ins.callee;
                        } else {
                            callee_name = ctx.mangle(ins.callee);
                        }
                        auto call = std::make_unique<CppCall>(callee_name);
                        for (const auto& a : ins.args) {
                            if (a) {
                                call->args.push_back(emit_value(ctx, *a));
                            }
                        }
                        if (ins.result) {
                            const auto& dst = static_cast<const vitte::ir::MirLocal&>(*ins.result);
                            auto dname = ctx.safe_ident(dst.name);
                            if (declared.insert(dname).second) {
                                auto decl_type = map_type(ctx, dst.type);
                                if (is_unknown_type_name(type_name(dst.type))) {
                                    decl_type = builtin_type(ctx, "auto");
                                }
                                auto decl = std::make_unique<CppVarDecl>(
                                    decl_type,
                                    dname
                                );
                                decl->init = std::move(call);
                                out.body.push_back(std::move(decl));
                            } else {
                                out.body.push_back(std::make_unique<CppAssign>(
                                    std::make_unique<CppVar>(dname),
                                    std::move(call)));
                            }
                        } else {
                            out.body.push_back(std::make_unique<CppExprStmt>(std::move(call)));
                        }
                        break;
                    }
                    case MirKind::CallIndirect: {
                        auto& ins = static_cast<const vitte::ir::MirCallIndirect&>(*instr);
                        auto call = std::make_unique<CppCall>(emit_value(ctx, *ins.callee));
                        for (const auto& a : ins.args) {
                            if (a) {
                                call->args.push_back(emit_value(ctx, *a));
                            }
                        }
                        if (ins.result) {
                            const auto& dst = static_cast<const vitte::ir::MirLocal&>(*ins.result);
                            auto dname = ctx.safe_ident(dst.name);
                            if (declared.insert(dname).second) {
                                auto decl_type = map_type(ctx, dst.type);
                                if (is_unknown_type_name(type_name(dst.type))) {
                                    decl_type = builtin_type(ctx, "auto");
                                }
                                auto decl = std::make_unique<CppVarDecl>(
                                    decl_type,
                                    dname
                                );
                                decl->init = std::move(call);
                                out.body.push_back(std::move(decl));
                            } else {
                                out.body.push_back(std::make_unique<CppAssign>(
                                    std::make_unique<CppVar>(dname),
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
