// ============================================================
// vitte_codegen_llvm::lower::module
// Orchestration MIR -> LLVM au niveau module
// ============================================================

#include "vitte/codegen/llvm/context.hpp"
#include "vitte/codegen/llvm/passes.hpp"
#include "vitte/codegen/llvm/emit.hpp"

#include "llvm/IR/Verifier.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include <cassert>
#include <memory>

namespace vitte::codegen::llvm_backend::lower {

using namespace vitte::mir;


// ------------------------------------------------------------
// Forward declaration
// ------------------------------------------------------------

void lowerFunction(CodegenContext& cg,
                   const Function& fn);


// ------------------------------------------------------------
// Déclaration préalable des fonctions (signatures)
// ------------------------------------------------------------

static void declareFunctions(CodegenContext& cg,
                             const Program& program)
{
    for (const auto& fn : program.functions) {
        // Le type exact sera reconstruit dans lowerFunction,
        // ici on réserve juste le symbole si nécessaire.
        // (optionnel selon l’architecture)
        (void)fn;
    }
}


// ------------------------------------------------------------
// Lowering principal du module
// ------------------------------------------------------------

void lowerModule(CodegenContext& cg,
                 const Program& program)
{
    // --------------------------------------------------------
    // Phase 0 — Déclarations
    // --------------------------------------------------------

    declareFunctions(cg, program);

    // --------------------------------------------------------
    // Phase 1 — Lowering des fonctions
    // --------------------------------------------------------

    for (const auto& fn : program.functions) {
        lowerFunction(cg, fn);
    }

    // --------------------------------------------------------
    // Vérification du module LLVM
    // --------------------------------------------------------

    assert(
        !llvm::verifyModule(
            cg.getModule(),
            &llvm::errs()
        ) && "LLVM module verification failed"
    );
}


// ------------------------------------------------------------
// Pipeline complet + émission (helper)
// ------------------------------------------------------------

bool compileModule(const Program& program,
                   const std::string& moduleName,
                   const std::string& outputPath,
                   emit::OutputKind outputKind,
                   passes::OptLevel optLevel,
                   const std::string& triple)
{
    // --------------------------------------------------------
    // Contexte LLVM
    // --------------------------------------------------------

    CodegenContext cg(moduleName);

    // --------------------------------------------------------
    // Lowering MIR -> LLVM IR
    // --------------------------------------------------------

    lowerModule(cg, program);

    // --------------------------------------------------------
    // Initialisation LLVM target
    // --------------------------------------------------------

    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

    std::string error;
    const llvm::Target* target =
        llvm::TargetRegistry::lookupTarget(triple, error);

    if (!target) {
        llvm::errs() << "[vitte-llvm] target error: "
                     << error << "\n";
        return false;
    }

    llvm::TargetOptions options;
    auto tm = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(
            triple,
            "generic",
            "",
            options,
            std::nullopt
        )
    );

    cg.getModule().setTargetTriple(triple);
    cg.getModule().setDataLayout(
        tm->createDataLayout()
    );

    // --------------------------------------------------------
    // Passes LLVM
    // --------------------------------------------------------

    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cam;
    llvm::ModuleAnalysisManager mam;

    llvm::PassBuilder pb(tm.get());

    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(
        lam, fam, cam, mam
    );

    auto pipeline =
        passes::buildDefaultPipeline(
            optLevel,
            *tm
        );

    pipeline->run(
        cg.getModule(),
        mam
    );

    // --------------------------------------------------------
    // Emission
    // --------------------------------------------------------

    return emit::emitModule(
        cg.getModule(),
        outputPath,
        outputKind,
        triple
    );
}

} // namespace vitte::codegen::llvm_backend::lower
