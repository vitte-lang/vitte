#pragma once
#include <vector>

#include "hir.hpp"
#include "mir.hpp"

namespace vitte::ir {

/* =================================================
 * IR Builder
 * =================================================
 *
 * Convertit le HIR (haut niveau, structuré)
 * vers le MIR (linéaire, prêt backend).
 */

/* ---------------------------------------------
 * Lower a HIR function to MIR
 * --------------------------------------------- */
MirFunction lower_hir_to_mir(const HirFunction& hir);

} // namespace vitte::ir
