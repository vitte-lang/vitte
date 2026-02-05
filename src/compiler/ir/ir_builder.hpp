// ============================================================
// ir_builder.hpp — Vitte Compiler
// AST → HIR builder interface
// ============================================================

#pragma once

#include "../frontend/ast.hpp"
#include "../frontend/diagnostics.hpp"
#include "../frontend/lower_hir.hpp"
#include "hir.hpp"

namespace vitte::ir {

// ------------------------------------------------------------
// IR Builder
// ------------------------------------------------------------
//
// Rôle :
//  - transformer l'AST frontend en HIR canonique
//  - aucune logique de typage finale
//  - aucune dépendance backend
//  - point d'entrée unique pour le lowering AST → HIR
//

class IrBuilder {
public:
    explicit IrBuilder(ir::HirContext& ctx);

    // --------------------------------------------------------
    // Entry
    // --------------------------------------------------------

    HirModuleId build_module(
        const vitte::frontend::ast::AstContext& ast_ctx,
        vitte::frontend::ast::ModuleId module,
        vitte::frontend::diag::DiagnosticEngine& diagnostics);

private:
    ir::HirContext& hir_ctx_;
};

} // namespace vitte::ir
