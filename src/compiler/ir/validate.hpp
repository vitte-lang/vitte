// ============================================================
// validate.hpp — Vitte Compiler
// HIR validation
// ============================================================

#pragma once

#include "hir.hpp"
#include "../frontend/diagnostics.hpp"

namespace vitte::ir::validate {

void validate_module(const HirContext& ctx,
                     HirModuleId module,
                     frontend::diag::DiagnosticEngine& diagnostics);

} // namespace vitte::ir::validate
