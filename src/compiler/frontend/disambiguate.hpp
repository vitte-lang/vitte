#pragma once

#include "ast.hpp"

namespace vitte::frontend::passes {

/* -------------------------------------------------
 * Invoke disambiguation (placeholder)
 * -------------------------------------------------
 * Marks InvokeExpr as Call or Ctor when possible.
 * This will later use resolved type info.
 */
void disambiguate_invokes(ast::AstContext& ctx, ast::ModuleId module);

} // namespace vitte::frontend::passes
