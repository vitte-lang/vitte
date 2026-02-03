// ============================================================
// vitte::llvm::context
// Implémentation du contexte LLVM Vitte
// ============================================================

#include "vitte/llvm/context.hpp"

#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetRegistry.h>
#include <llvm/IR/Verifier.h>

#include <cassert>

namespace vitte::llvm {


// ------------------------------------------------------------
// Helpers internes
// ------------------------------------------------------------

static std::string default_triple()
{
    return ::llvm::sys::getDefaultTargetTriple();
}


static ::llvm::Reloc::Model default_reloc_model()
{
#if defined(_WIN32)
    return ::llvm::Reloc::PIC_;
#else
    return ::llvm::Reloc::PIC_;
#endif
}


// ------------------------------------------------------------
// Conversion OptLevel
// ------------------------------------------------------------

::llvm::CodeGenOpt::Level
to_llvm_opt_level(OptLevel level)
{
    switch (level) {
    case OptLevel::O0: return ::llvm::CodeGenOpt::None;
    case OptLevel::O1: return ::llvm::CodeGenOpt::Less;
    case OptLevel::O2: return ::llvm::CodeGenOpt::Default;
    case OptLevel::O3: return ::llvm::CodeGenOpt::Aggressive;
    case OptLevel::Os: return ::llvm::CodeGenOpt::Default;
    case OptLevel::Oz: return ::llvm::CodeGenOpt::Default;
    }
    return ::llvm::CodeGenOpt::Default;
}


std::string
to_string(OptLevel level)
{
    switch (level) {
    case OptLevel::O0: return "O0";
    case OptLevel::O1: return "O1";
    case OptLevel::O2: return "O2";
    case OptLevel::O3: return "O3";
    case OptLevel::Os: return "Os";
    case OptLevel::Oz: return "Oz";
    }
    return "O2";
}


std::string
to_string(EmitFormat fmt)
{
    switch (fmt) {
    case EmitFormat::Object:   return "object";
    case EmitFormat::Assembly: return "assembly";
    case EmitFormat::Bitcode:  return "bitcode";
    }
    return "object";
}


// ------------------------------------------------------------
// Context
// ------------------------------------------------------------

Context::Context(const LlvmConfig& config)
    : cfg(config),
      ctx(),
      tm(nullptr),
      layout("")
{
    init_target();
}


Context::~Context() = default;


// ------------------------------------------------------------
// Initialisation LLVM TargetMachine
// ------------------------------------------------------------

void Context::init_target()
{
    // Initialisation LLVM globale (idempotente)
    static bool initialized = false;
    if (!initialized) {
        ::llvm::InitializeAllTargets();
        ::llvm::InitializeAllTargetMCs();
        ::llvm::InitializeAllAsmPrinters();
        ::llvm::InitializeAllAsmParsers();
        initialized = true;
    }

    // Triple
    if (cfg.target_triple.empty()) {
        cfg.target_triple = default_triple();
    }

    std::string error;
    const auto* target =
        ::llvm::TargetRegistry::lookupTarget(
            cfg.target_triple,
            error
        );

    if (!target) {
        ::llvm::errs()
            << "[vitte-llvm] unable to find target: "
            << error << "\n";
        std::abort();
    }

    // Options target
    ::llvm::TargetOptions opts;
    opts.RelaxELFRelocations = true;

    // Création TargetMachine
    tm.reset(
        target->createTargetMachine(
            cfg.target_triple,
            cfg.cpu.empty() ? "generic" : cfg.cpu,
            cfg.features,
            opts,
            default_reloc_model(),
            std::nullopt,
            to_llvm_opt_level(cfg.opt_level)
        )
    );

    if (!tm) {
        ::llvm::errs()
            << "[vitte-llvm] failed to create TargetMachine\n";
        std::abort();
    }

    layout = tm->createDataLayout();
}


// ------------------------------------------------------------
// Accesseurs
// ------------------------------------------------------------

::llvm::LLVMContext&
Context::llvm_context()
{
    return ctx;
}


::llvm::TargetMachine&
Context::target_machine()
{
    return *tm;
}


const ::llvm::DataLayout&
Context::data_layout() const
{
    return layout;
}


const LlvmConfig&
Context::config() const
{
    return cfg;
}


// ------------------------------------------------------------
// Fabrique de module LLVM
// ------------------------------------------------------------

std::unique_ptr<::llvm::Module>
Context::create_module(const std::string& name)
{
    auto module =
        std::make_unique<::llvm::Module>(
            name,
            ctx
        );

    module->setTargetTriple(cfg.target_triple);
    module->setDataLayout(layout);

    return module;
}


// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

bool Context::is_optimized() const
{
    return cfg.opt_level != OptLevel::O0;
}


unsigned Context::llvm_opt_level() const
{
    switch (cfg.opt_level) {
    case OptLevel::O0: return 0;
    case OptLevel::O1: return 1;
    case OptLevel::O2: return 2;
    case OptLevel::O3: return 3;
    case OptLevel::Os: return 2;
    case OptLevel::Oz: return 2;
    }
    return 2;
}

} // namespace vitte::llvm
