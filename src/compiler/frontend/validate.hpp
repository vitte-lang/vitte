// ============================================================
// validate.hpp — Vitte Compiler
// AST shape validation
// ============================================================

#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"

namespace vitte::frontend::validate {

void validate_module(ast::AstContext& ast_ctx, ast::ModuleId module, diag::DiagnosticEngine& diagnostics);

} // namespace vitte::frontend::validate
