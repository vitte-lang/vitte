#pragma once
#include <vector>

#include "../ast/cpp_decl.hpp"
#include "../context/cpp_context.hpp"

namespace vitte::backend::lower {

/* -------------------------------------------------
 * Forward MIR placeholders
 * -------------------------------------------------
 * Ces structures sont volontairement minimales ici.
 * Elles doivent être remplacées ou adaptées pour
 * matcher TON vrai MIR Vitte.
 * ------------------------------------------------- */

struct MirValue {
    std::string name;
};

struct MirInstr {
    enum class Kind {
        ConstI32,
        Add,
        Return
    };

    Kind kind;
    MirValue dst;
    MirValue lhs;
    MirValue rhs;
    int32_t imm = 0;
};

struct MirFunction {
    std::string name;
    std::vector<MirInstr> instrs;
};

/* -------------------------------------------------
 * Lowering API
 * ------------------------------------------------- */

/* Lower a single MIR function to a C++ AST function */
ast::cpp::CppFunction lower_mir_function(
    const MirFunction& mf,
    context::CppContext& ctx
);

/* Lower a full MIR module to a C++ translation unit */
ast::cpp::CppTranslationUnit lower_mir(
    const std::vector<MirFunction>& functions,
    context::CppContext& ctx
);

} // namespace vitte::backend::lower
