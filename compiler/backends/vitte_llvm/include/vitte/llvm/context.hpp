// ============================================================
// vitte::llvm::context
// Contexte central du backend LLVM Vitte
// ============================================================

#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

namespace vitte::llvm {


// ------------------------------------------------------------
// Niveaux d’optimisation LLVM
// ------------------------------------------------------------

enum class OptLevel {
    O0,
    O1,
    O2,
    O3,
    Os,
    Oz
};


// ------------------------------------------------------------
// Format de sortie LLVM
// ------------------------------------------------------------

enum class EmitFormat {
    Object,     // .o
    Assembly,   // .s
    Bitcode     // .bc
};


// ------------------------------------------------------------
// Configuration du backend LLVM
// ------------------------------------------------------------

struct LlvmConfig {
    std::string target_triple;
    std::string cpu;
    std::string features;

    OptLevel opt_level = OptLevel::O2;
    EmitFormat emit_format = EmitFormat::Object;

    bool verify_module = true;
    bool debug_ir = false;
};


// ------------------------------------------------------------
// Contexte LLVM principal
// ------------------------------------------------------------

class Context {
public:
    explicit Context(const LlvmConfig& config);
    ~Context();

    // --------------------------------------------------------
    // Accesseurs LLVM
    // --------------------------------------------------------

    ::llvm::LLVMContext& llvm_context();
    ::llvm::TargetMachine& target_machine();
    const ::llvm::DataLayout& data_layout() const;

    // --------------------------------------------------------
    // Configuration
    // --------------------------------------------------------

    const LlvmConfig& config() const;

    // --------------------------------------------------------
    // Fabrique de modules LLVM
    // --------------------------------------------------------

    std::unique_ptr<::llvm::Module>
    create_module(const std::string& name);

    // --------------------------------------------------------
    // Helpers
    // --------------------------------------------------------

    bool is_optimized() const;
    unsigned llvm_opt_level() const;

private:
    LlvmConfig cfg;

    ::llvm::LLVMContext ctx;
    std::unique_ptr<::llvm::TargetMachine> tm;
    ::llvm::DataLayout layout;

    void init_target();
};


// ------------------------------------------------------------
// Helpers globaux
// ------------------------------------------------------------

::llvm::CodeGenOpt::Level
to_llvm_opt_level(OptLevel level);

std::string
to_string(OptLevel level);

std::string
to_string(EmitFormat fmt);

} // namespace vitte::llvm
