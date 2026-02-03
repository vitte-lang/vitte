// ============================================================
// vitte_codegen_llvm::passes
// Implémentation des pipelines de passes LLVM
// ============================================================

#include "vitte/codegen/llvm/passes.hpp"

#include "llvm/IR/Verifier.h"
#include "llvm/IR/PassManager.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Utils.h"

namespace vitte::codegen::llvm_backend::passes {


// ------------------------------------------------------------
// Helpers internes
// ------------------------------------------------------------

static llvm::OptimizationLevel toLlvmOptLevel(OptLevel level)
{
    switch (level) {
        case OptLevel::O0: return llvm::OptimizationLevel::O0;
        case OptLevel::O1: return llvm::OptimizationLevel::O1;
        case OptLevel::O2: return llvm::OptimizationLevel::O2;
        case OptLevel::O3: return llvm::OptimizationLevel::O3;
        case OptLevel::Os: return llvm::OptimizationLevel::Os;
        case OptLevel::Oz: return llvm::OptimizationLevel::Oz;
        default:           return llvm::OptimizationLevel::O0;
    }
}


// ------------------------------------------------------------
// Passes de vérification
// ------------------------------------------------------------

void addVerificationPasses(llvm::ModulePassManager& pm)
{
    // Vérifie la validité IR après lowering
    pm.addPass(llvm::VerifierPass());
}


// ------------------------------------------------------------
// Pipeline d’optimisation Module
// ------------------------------------------------------------

void addOptimizationPipeline(llvm::ModulePassManager& pm,
                             llvm::TargetMachine& tm,
                             OptLevel level)
{
    llvm::PassBuilder pb(&tm);

    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cam;
    llvm::ModuleAnalysisManager mam;

    // Instrumentation standard (debug / remarks)
    llvm::StandardInstrumentations si(
        pb.getContext(),
        /*DebugLogging=*/false
    );
    si.registerCallbacks(pb);

    // Enregistrement des analyses
    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(
        lam, fam, cam, mam
    );

    // Niveau d’optimisation LLVM
    auto llvmLevel = toLlvmOptLevel(level);

    // Pipeline standard LLVM
    pm.addPass(
        pb.buildPerModuleDefaultPipeline(
            llvmLevel
        )
    );
}


// ------------------------------------------------------------
// Passes finales (cleanup avant emission)
// ------------------------------------------------------------

void addFinalizationPasses(llvm::ModulePassManager& pm)
{
    // Nettoyage final, simplifications tardives
    pm.addPass(llvm::GlobalDCEPass());
    pm.addPass(llvm::StripDeadDebugInfoPass());
}


// ------------------------------------------------------------
// Pipeline Function
// ------------------------------------------------------------

void addFunctionPipeline(llvm::FunctionPassManager& pm,
                         OptLevel level)
{
    if (level == OptLevel::O0) {
        // À O0, uniquement passes de correction
        pm.addPass(llvm::PromotePass()); // mem2reg léger
        return;
    }

    // Optimisations locales SSA
    pm.addPass(llvm::PromotePass());            // mem2reg
    pm.addPass(llvm::InstCombinePass());        // combine
    pm.addPass(llvm::ReassociatePass());        // reassoc
    pm.addPass(llvm::GVNPass());                 // value numbering
    pm.addPass(llvm::SimplifyCFGPass());        // CFG cleanup
}


// ------------------------------------------------------------
// Construction pipeline complet
// ------------------------------------------------------------

std::unique_ptr<llvm::ModulePassManager>
buildDefaultPipeline(OptLevel level,
                     llvm::TargetMachine& tm)
{
    auto pm =
        std::make_unique<llvm::ModulePassManager>();

    // 1) Vérification post-lowering
    addVerificationPasses(*pm);

    // 2) Optimisations (si != O0)
    if (level != OptLevel::O0) {
        addOptimizationPipeline(*pm, tm, level);
    }

    // 3) Nettoyage final
    addFinalizationPasses(*pm);

    return pm;
}

} // namespace vitte::codegen::llvm_backend::passes
