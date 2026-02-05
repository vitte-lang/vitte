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

    for (const auto& f : functions) {
        tu.functions.push_back(
            lower_mir_function(f, ctx)
        );
    }

    return tu;
}

} // namespace vitte::backend::lower
