#pragma once

#include "hir.hpp"
#include "mir.hpp"
#include "../frontend/diagnostics.hpp"

namespace vitte::ir::lower {

MirModule lower_to_mir(
    const HirContext& hir_ctx,
    HirModuleId module_id,
    vitte::frontend::diag::DiagnosticEngine& diagnostics);

} // namespace vitte::ir::lower
