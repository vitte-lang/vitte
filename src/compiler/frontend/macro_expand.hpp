#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"

namespace vitte::frontend::passes {

void expand_macros(ast::AstContext& ctx, ast::ModuleId module, diag::DiagnosticEngine& diagnostics);

} // namespace vitte::frontend::passes
