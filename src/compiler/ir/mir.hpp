#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace vitte::ir {

/* =================================================
 * MIR – Mid-level Intermediate Representation
 * =================================================
 *
 * Représentation linéaire, simple, indépendante
 * du backend, prête pour lowering codegen.
 */

/* ---------------------------------------------
 * MIR value (SSA-like handle)
 * --------------------------------------------- */
struct MirValue {
    std::string name;
};

/* ---------------------------------------------
 * MIR instruction
 * --------------------------------------------- */
struct MirInstr {
    enum class Kind {
        ConstI32,   // dst = imm
        Add,        // dst = lhs + rhs
        Return      // return dst
    };

    Kind kind;

    /* Operands */
    MirValue dst;
    MirValue lhs;
    MirValue rhs;

    /* Immediate */
    std::int32_t imm = 0;
};

/* ---------------------------------------------
 * MIR function
 * --------------------------------------------- */
struct MirFunction {
    std::string name;
    std::vector<MirInstr> instrs;
};

} // namespace vitte::ir
