#pragma once
#include <vector>

#include "../../ir/mir.hpp"
#include "../ast/cpp_decl.hpp"
#include "../context/cpp_context.hpp"

namespace vitte::backend::lower {

/* -------------------------------------------------
 * Lowering API
 * ------------------------------------------------- */

/* Lower a full MIR module to a C++ translation unit */
ast::cpp::CppTranslationUnit lower_mir(
    const vitte::ir::MirModule& module,
    context::CppContext& ctx
);

} // namespace vitte::backend::lower
