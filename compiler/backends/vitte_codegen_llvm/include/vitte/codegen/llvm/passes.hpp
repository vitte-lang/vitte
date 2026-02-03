// ============================================================
// vitte_codegen_llvm::passes
// Déclaration des pipelines de passes LLVM
// ============================================================

#pragma once

#include <memory>

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

namespace llvm {
class TargetMachine;
}

namespace vitte::codegen::llvm_backend::passes {


// ------------------------------------------------------------
// Niveaux d’optimisation
// ------------------------------------------------------------

enum class OptLevel {
    O0,   // pas d’optimisation
    O1,
    O2,
    O3,
    Os,   // size
    Oz    // min size
};


// ------------------------------------------------------------
// Pipelines Module
// ------------------------------------------------------------

/**
 * Ajoute les passes de vérification minimales.
 * À utiliser juste après génération IR.
 */
void addVerificationPasses(llvm::ModulePassManager& pm);


/**
 * Ajoute un pipeline d’optimisation standard Vitte.
 *
 * @param pm     ModulePassManager
 * @param tm     TargetMachine LLVM
 * @param level  Niveau d’optimisation
 */
void addOptimizationPipeline(llvm::ModulePassManager& pm,
                             llvm::TargetMachine& tm,
                             OptLevel level);


/**
 * Ajoute les passes finales avant émission (cleanup).
 */
void addFinalizationPasses(llvm::ModulePassManager& pm);


// ------------------------------------------------------------
// Pipelines Function
// ------------------------------------------------------------

/**
 * Ajoute les passes locales à une fonction
 * (inlining léger, simplifications SSA, etc.)
 */
void addFunctionPipeline(llvm::FunctionPassManager& pm,
                         OptLevel level);


// ------------------------------------------------------------
// Helpers de construction
// ------------------------------------------------------------

/**
 * Construit un pipeline complet module + fonctions.
 *
 * @param level  Niveau d’optimisation
 * @param tm     TargetMachine LLVM
 */
std::unique_ptr<llvm::ModulePassManager>
buildDefaultPipeline(OptLevel level,
                     llvm::TargetMachine& tm);

} // namespace vitte::codegen::llvm_backend::passes
