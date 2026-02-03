// ============================================================
// vitte::llvm::emit::object
// Émission LLVM vers fichier objet (.o)
// ============================================================

#include "vitte/llvm/context.hpp"
#include "vitte/llvm/passes.hpp"

#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>

#include <system_error>
#include <memory>

namespace vitte::llvm::emit {


// ------------------------------------------------------------
// Émission objet
// ------------------------------------------------------------

bool emit_object(::llvm::Module& module,
                 vitte::llvm::Context& ctx,
                 const std::string& output_path)
{
    // --------------------------------------------------------
    // Vérification LLVM
    // --------------------------------------------------------

    if (ctx.config().verify_module) {
        if (::llvm::verifyModule(module, &::llvm::errs())) {
            ::llvm::errs() << "[vitte-llvm] invalid LLVM module\n";
            return false;
        }
    }

    // --------------------------------------------------------
    // Passes LLVM (new PM)
    // --------------------------------------------------------

    ::llvm::PassBuilder passBuilder(&ctx.target_machine());

    ::llvm::LoopAnalysisManager loopAM;
    ::llvm::FunctionAnalysisManager functionAM;
    ::llvm::CGSCCAnalysisManager cgsccAM;
    ::llvm::ModuleAnalysisManager moduleAM;

    passBuilder.registerModuleAnalyses(moduleAM);
    passBuilder.registerCGSCCAnalyses(cgsccAM);
    passBuilder.registerFunctionAnalyses(functionAM);
    passBuilder.registerLoopAnalyses(loopAM);
    passBuilder.crossRegisterProxies(
        loopAM, functionAM, cgsccAM, moduleAM
    );

    ::llvm::ModulePassManager modulePM;

    // Vérification minimale
    vitte::llvm::passes::addVerificationPasses(modulePM);

    // Optimisations selon le niveau
    vitte::llvm::passes::addOptimizationPipeline(
        modulePM,
        ctx.target_machine(),
        static_cast<vitte::llvm::passes::OptLevel>(
            ctx.config().opt_level
        )
    );

    // Nettoyage final
    vitte::llvm::passes::addFinalizationPasses(modulePM);

    modulePM.run(module, moduleAM);

    // --------------------------------------------------------
    // Ouverture du fichier de sortie
    // --------------------------------------------------------

    std::error_code ec;
    ::llvm::raw_fd_ostream out(
        output_path,
        ec,
        ::llvm::sys::fs::OF_None
    );

    if (ec) {
        ::llvm::errs()
            << "[vitte-llvm] cannot open output file: "
            << ec.message() << "\n";
        return false;
    }

    // --------------------------------------------------------
    // Émission objet
    // --------------------------------------------------------

    ::llvm::legacy::PassManager codegenPM;

    auto fileType = ::llvm::CodeGenFileType::ObjectFile;

    if (ctx.target_machine().addPassesToEmitFile(
            codegenPM,
            out,
            nullptr,
            fileType
        )) {
        ::llvm::errs()
            << "[vitte-llvm] target does not support object emission\n";
        return false;
    }

    codegenPM.run(module);
    out.flush();

    return true;
}

} // namespace vitte::llvm::emit
