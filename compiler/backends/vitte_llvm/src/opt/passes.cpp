// ============================================================
// vitte::llvm::passes
// Implémentation des pipelines de passes LLVM
// ============================================================

#include "vitte/llvm/passes.hpp"

#include <llvm/IR/Verifier.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

namespace vitte::llvm::passes {


// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static ::llvm::OptimizationLevel
to_llvm_level(OptLevel level)
{
    switch (level) {
    case OptLevel::O0: return ::llvm::OptimizationLevel::O0;
    case OptLevel::O1: return ::llvm::OptimizationLevel::O1;
    case OptLevel::O2: return ::llvm::OptimizationLevel::O2;
    case OptLevel::O3: return ::llvm::OptimizationLevel::O3;
    case OptLevel::Os: return ::llvm::OptimizationLevel::Os;
    case OptLevel::Oz: return ::llvm::OptimizationLevel::Oz;
    }
    return ::llvm::OptimizationLevel::O0;
}


// ------------------------------------------------------------
// Passes de vérification
// ------------------------------------------------------------

void addVerificationPasses(::llvm::ModulePassManager& pm)
{
    pm.addPass(::llvm::VerifierPass());
}


// ------------------------------------------------------------
// Pipeline d’optimisation principal
// ------------------------------------------------------------

void addOptimizationPipeline(::llvm::ModulePassManager& pm,
                             ::llvm::TargetMachine& tm,
                             OptLevel level)
{
    ::llvm::PassBuilder pb(&tm);

    ::llvm::LoopAnalysisManager lam;
    ::llvm::FunctionAnalysisManager fam;
    ::llvm::CGSCCAnalysisManager cgam;
    ::llvm::ModuleAnalysisManager mam;

    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);

    const auto optLevel = to_llvm_level(level);

    // Pipeline standard LLVM
    pm.addPass(
        pb.buildPerModuleDefaultPipeline(optLevel)
    );
}


// ------------------------------------------------------------
// Passes de finalisation
// ------------------------------------------------------------

void addFinalizationPasses(::llvm::ModulePassManager& pm)
{
    // Nettoyage final et vérifications légères
    pm.addPass(::llvm::GlobalDCEPass());
    pm.addPass(::llvm::VerifierPass());
}


// ------------------------------------------------------------
// Pipeline Function (optionnel)
// ------------------------------------------------------------

void addFunctionPipeline(::llvm::FunctionPassManager& pm,
                         OptLevel level)
{
    if (level == OptLevel::O0)
        return;

    pm.addPass(::llvm::InstCombinePass());
    pm.addPass(::llvm::ReassociatePass());
    pm.addPass(::llvm::GVNPass());
    pm.addPass(::llvm::CFGSimplificationPass());
}


// ------------------------------------------------------------
// Fabrique pipeline complet
// ------------------------------------------------------------

std::unique_ptr<::llvm::ModulePassManager>
buildDefaultPipeline(OptLevel level,
                     ::llvm::TargetMachine& tm)
{
    auto pm =
        std::make_unique<::llvm::ModulePassManager>();

    addVerificationPasses(*pm);
    addOptimizationPipeline(*pm, tm, level);
    addFinalizationPasses(*pm);

    return pm;
}

} // namespace vitte::llvm::passes
