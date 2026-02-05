// ============================================================
// ir_builder.cpp — Vitte Compiler
// AST → HIR builder implementation
// ============================================================

#include "ir_builder.hpp"

namespace vitte::ir {

IrBuilder::IrBuilder(ir::HirContext& ctx)
    : hir_ctx_(ctx) {}

HirModuleId IrBuilder::build_module(
    const vitte::frontend::ast::AstContext& ast_ctx,
    vitte::frontend::ast::ModuleId module,
    vitte::frontend::diag::DiagnosticEngine& diagnostics)
{
    return vitte::frontend::lower::lower_to_hir(ast_ctx, module, hir_ctx_, diagnostics);
}

} // namespace vitte::ir
