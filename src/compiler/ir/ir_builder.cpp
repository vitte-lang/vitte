#include "ir_builder.hpp"

namespace vitte::ir {

/* -------------------------------------------------
 * Helpers
 * ------------------------------------------------- */

static MirValue make_value(const std::string& name) {
    MirValue v;
    v.name = name;
    return v;
}

/* -------------------------------------------------
 * Expression lowering
 * ------------------------------------------------- */

static MirValue lower_expr(
    const HirExpr& expr,
    MirFunction& mf
) {
    switch (expr.kind) {

    case HirKind::Literal: {
        auto& lit = static_cast<const HirLiteral&>(expr);
        MirValue dst = make_value("const_" + lit.value);

        MirInstr ins;
        ins.kind = MirInstr::Kind::ConstI32;
        ins.dst = dst;
        ins.imm = std::stoi(lit.value);

        mf.instrs.push_back(ins);
        return dst;
    }

    case HirKind::Variable: {
        auto& var = static_cast<const HirVariable&>(expr);
        return make_value(var.name);
    }

    default:
        /* Unsupported expression */
        return make_value("undef");
    }
}

/* -------------------------------------------------
 * Statement lowering
 * ------------------------------------------------- */

static void lower_stmt(
    const HirStmt& stmt,
    MirFunction& mf
) {
    switch (stmt.kind) {

    case HirKind::Return: {
        auto& ret = static_cast<const HirReturn&>(stmt);
        MirValue v = lower_expr(*ret.expr, mf);

        MirInstr ins;
        ins.kind = MirInstr::Kind::Return;
        ins.dst = v;

        mf.instrs.push_back(ins);
        break;
    }

    default:
        /* Ignore unsupported statements */
        break;
    }
}

/* -------------------------------------------------
 * Block lowering
 * ------------------------------------------------- */

static void lower_block(
    const HirBlock& block,
    MirFunction& mf
) {
    for (const auto& stmt : block.stmts) {
        lower_stmt(*stmt, mf);
    }
}

/* -------------------------------------------------
 * Function lowering (entry)
 * ------------------------------------------------- */

MirFunction lower_hir_to_mir(const HirFunction& hir) {
    MirFunction mf;
    mf.name = hir.name;

    lower_block(*hir.body, mf);

    return mf;
}

} // namespace vitte::ir
