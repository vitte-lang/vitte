#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"

#include "../ir/hir.hpp"

namespace vitte::frontend::lower {

ir::HirModuleId lower_to_hir(
    const ast::AstContext& ctx,
    ast::ModuleId module,
    ir::HirContext& hir_ctx,
    diag::DiagnosticEngine& diagnostics);

} // namespace vitte::frontend::lower
