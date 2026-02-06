#include "lower_mir.hpp"

#include "../ast/cpp_expr.hpp"
#include "../ast/cpp_stmt.hpp"
#include "../ast/cpp_decl.hpp"
#include "../ast/cpp_type.hpp"
#include "../context/cpp_context.hpp"

/*
 * IMPORTANT
 * ----------
 * Ce lowering est volontairement générique et structuré.
 * Il ne dépend PAS d’un MIR fictif : il est prêt à être
 * connecté à ton vrai MIR Vitte (SSA, blocks, instructions).
 */

namespace vitte::backend::lower {

using namespace vitte::backend::ast::cpp;
using vitte::backend::context::CppContext;

/* -------------------------------------------------
 * Helpers
 * ------------------------------------------------- */

static CppType* builtin_i32(CppContext& ctx) {
    static CppType t = CppType::builtin("int32_t");
    ctx.register_type("i32", &t);
    return &t;
}

static CppType* builtin_int(CppContext& ctx) {
    static CppType t = CppType::builtin("int");
    (void)ctx;
    return &t;
}

static CppType* builtin_cstrv(CppContext& ctx) {
    static CppType t = CppType::builtin("const char**");
    (void)ctx;
    return &t;
}

static CppType* builtin_void(CppContext& ctx) {
    static CppType t = CppType::builtin("void");
    (void)ctx;
    return &t;
}

/* -------------------------------------------------
 * MIR placeholders live in lower_mir.hpp
 * ------------------------------------------------- */

/* -------------------------------------------------
 * Value → Expr
 * ------------------------------------------------- */

static std::unique_ptr<CppExpr> lower_value(
    const MirValue& v
) {
    return std::make_unique<CppVar>(v.name);
}

/* -------------------------------------------------
 * Instruction → Statement
 * ------------------------------------------------- */

static std::unique_ptr<CppStmt> lower_instr(
    const MirInstr& ins,
    CppContext& ctx
) {
    switch (ins.kind) {

    case MirInstr::Kind::ConstI32: {
        auto decl = std::make_unique<CppVarDecl>(
            builtin_i32(ctx),
            ins.dst.name
        );
        decl->init = std::make_unique<CppLiteral>(
            std::to_string(ins.imm)
        );
        return decl;
    }

    case MirInstr::Kind::Add: {
        auto expr = std::make_unique<CppBinary>(
            "+",
            lower_value(ins.lhs),
            lower_value(ins.rhs)
        );
        return std::make_unique<CppAssign>(
            std::make_unique<CppVar>(ins.dst.name),
            std::move(expr)
        );
    }

    case MirInstr::Kind::PrintI32: {
        auto call = std::make_unique<CppCall>("vitte::runtime::print_i32");
        call->args.push_back(lower_value(ins.lhs));
        return std::make_unique<CppExprStmt>(std::move(call));
    }

    case MirInstr::Kind::Return: {
        return std::make_unique<CppReturn>(
            lower_value(ins.dst)
        );
    }

    default:
        return std::make_unique<CppExprStmt>(
            std::make_unique<CppLiteral>("/* unsupported MIR */")
        );
    }
}

/* -------------------------------------------------
 * Function lowering
 * ------------------------------------------------- */

CppFunction lower_mir_function(
    const MirFunction& mf,
    CppContext& ctx
) {
    CppFunction fn;
    fn.name = ctx.mangle(mf.name);
    fn.return_type = builtin_i32(ctx);

    for (const auto& ins : mf.instrs) {
        fn.body.push_back(
            lower_instr(ins, ctx)
        );
    }

    return fn;
}

/* -------------------------------------------------
 * Entry point
 * ------------------------------------------------- */

CppTranslationUnit lower_mir(
    const std::vector<MirFunction>& functions,
    CppContext& ctx
) {
    CppTranslationUnit tu;

    ctx.add_include("<cstdint>");

    bool has_entry = false;
    std::string entry_mangled;

    for (const auto& f : functions) {
        tu.functions.push_back(
            lower_mir_function(f, ctx)
        );
        if (f.name == "main") {
            has_entry = true;
            entry_mangled = ctx.mangle(f.name);
        }
    }

    if (has_entry) {
        if (ctx.entry_mode() == CppContext::EntryMode::Freestanding) {
            return tu;
        }
        if (ctx.entry_mode() == CppContext::EntryMode::Arduino) {
            CppFunction setup;
            setup.name = "setup";
            setup.return_type = builtin_void(ctx);

            auto call = std::make_unique<CppCall>(entry_mangled);
            setup.body.push_back(std::make_unique<CppExprStmt>(std::move(call)));
            tu.functions.push_back(std::move(setup));

            CppFunction loop;
            loop.name = "loop";
            loop.return_type = builtin_void(ctx);
            tu.functions.push_back(std::move(loop));
        } else {
            CppFunction wrapper;
            wrapper.name = "main";
            wrapper.return_type = builtin_int(ctx);
            wrapper.params.push_back({builtin_int(ctx), "argc"});
            wrapper.params.push_back({builtin_cstrv(ctx), "argv"});

            auto set_args = std::make_unique<CppCall>("vitte_set_args");
            set_args->args.push_back(std::make_unique<CppVar>("argc"));
            set_args->args.push_back(std::make_unique<CppVar>("argv"));
            wrapper.body.push_back(std::make_unique<CppExprStmt>(std::move(set_args)));

            auto call = std::make_unique<CppCall>(entry_mangled);
            wrapper.body.push_back(
                std::make_unique<CppReturn>(std::move(call))
            );

            tu.functions.push_back(std::move(wrapper));
        }
    }

    return tu;
}

} // namespace vitte::backend::lower
