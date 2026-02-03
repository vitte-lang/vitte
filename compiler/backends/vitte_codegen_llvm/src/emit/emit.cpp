// ============================================================
// vitte_codegen_llvm::emit
// Emission LLVM IR -> object / asm / bitcode
// ============================================================

#include <string>
#include <memory>
#include <system_error>

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <llvm/MC/TargetRegistry.h>

namespace vitte::codegen::llvm_backend::emit {

// ------------------------------------------------------------
// Format de sortie
// ------------------------------------------------------------

enum class OutputKind {
    Object,
    Assembly,
    Bitcode,
};


// ------------------------------------------------------------
// Initialisation LLVM (idempotente)
// ------------------------------------------------------------

static void initializeLLVM()
{
    static bool initialized = false;
    if (initialized)
        return;

    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllAsmParsers();

    initialized = true;
}


// ------------------------------------------------------------
// Création du TargetMachine
// ------------------------------------------------------------

static std::unique_ptr<llvm::TargetMachine>
createTargetMachine(const std::string& triple,
                    const std::string& cpu,
                    const std::string& features)
{
    initializeLLVM();

    std::string error;
    const llvm::Target* target =
        llvm::TargetRegistry::lookupTarget(triple, error);

    if (!target) {
        llvm::errs() << "[vitte-llvm] target error: " << error << "\n";
        return nullptr;
    }

    llvm::TargetOptions options;
    auto relocModel = std::optional<llvm::Reloc::Model>();
    auto codeModel  = std::optional<llvm::CodeModel::Model>();

    return std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(
            triple,
            cpu,
            features,
            options,
            relocModel,
            codeModel,
            llvm::CodeGenOpt::Aggressive
        )
    );
}


// ------------------------------------------------------------
// Vérification du module LLVM
// ------------------------------------------------------------

static bool verifyModuleOrReport(llvm::Module& module)
{
    if (llvm::verifyModule(module, &llvm::errs())) {
        llvm::errs() << "[vitte-llvm] IR verification failed\n";
        return false;
    }
    return true;
}


// ------------------------------------------------------------
// Emission principale
// ------------------------------------------------------------

bool emitModule(llvm::Module& module,
                const std::string& outputPath,
                OutputKind kind,
                const std::string& triple,
                const std::string& cpu,
                const std::string& features)
{
    // --------------------------------------------------------
    // Vérification IR
    // --------------------------------------------------------

    if (!verifyModuleOrReport(module))
        return false;

    // --------------------------------------------------------
    // Target machine
    // --------------------------------------------------------

    auto tm = createTargetMachine(triple, cpu, features);
    if (!tm)
        return false;

    module.setTargetTriple(triple);
    module.setDataLayout(tm->createDataLayout());

    // --------------------------------------------------------
    // Fichier de sortie
    // --------------------------------------------------------

    std::error_code ec;
    llvm::raw_fd_ostream out(outputPath, ec, llvm::sys::fs::OF_None);

    if (ec) {
        llvm::errs() << "[vitte-llvm] cannot open output file: "
                     << ec.message() << "\n";
        return false;
    }

    // --------------------------------------------------------
    // Emission selon le format
    // --------------------------------------------------------

    if (kind == OutputKind::Bitcode) {
        llvm::WriteBitcodeToFile(module, out);
        out.flush();
        return true;
    }

    llvm::legacy::PassManager pm;
    llvm::CodeGenFileType fileType;

    switch (kind) {
        case OutputKind::Object:
            fileType = llvm::CGFT_ObjectFile;
            break;
        case OutputKind::Assembly:
            fileType = llvm::CGFT_AssemblyFile;
            break;
        default:
            llvm::errs() << "[vitte-llvm] unsupported output kind\n";
            return false;
    }

    if (tm->addPassesToEmitFile(pm, out, nullptr, fileType)) {
        llvm::errs() << "[vitte-llvm] target does not support this file type\n";
        return false;
    }

    pm.run(module);
    out.flush();

    return true;
}


// ------------------------------------------------------------
// Helpers publics
// ------------------------------------------------------------

bool emitObject(llvm::Module& module,
                const std::string& path,
                const std::string& triple)
{
    return emitModule(
        module,
        path,
        OutputKind::Object,
        triple,
        "generic",
        ""
    );
}


bool emitAssembly(llvm::Module& module,
                  const std::string& path,
                  const std::string& triple)
{
    return emitModule(
        module,
        path,
        OutputKind::Assembly,
        triple,
        "generic",
        ""
    );
}


bool emitBitcode(llvm::Module& module,
                 const std::string& path)
{
    return emitModule(
        module,
        path,
        OutputKind::Bitcode,
        module.getTargetTriple(),
        "",
        ""
    );
}

} // namespace vitte::codegen::llvm_backend::emit
