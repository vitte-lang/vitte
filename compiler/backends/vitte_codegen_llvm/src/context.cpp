#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>

#include "vitte/codegen/llvm/context.hpp"

using namespace llvm;

namespace vitte::codegen::llvm_backend {

CodegenContext::CodegenContext(Session& sess, const TargetDesc& tgt)
    : session(sess), target(tgt) {

    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();

    ctx = std::make_unique<LLVMContext>();
    mod = std::make_unique<Module>("vitte", *ctx);
    mod->setTargetTriple(target.triple);

    std::string err;
    const Target* T = TargetRegistry::lookupTarget(target.triple, err);
    TargetOptions opts;
    tm.reset(T->createTargetMachine(
        target.triple, target.cpu, target.features,
        opts, Reloc::PIC));

    irb = std::make_unique<IRBuilder<>>(*ctx);
}

CodegenContext::~CodegenContext() = default;

LLVMContext& CodegenContext::llvmContext() { return *ctx; }
Module& CodegenContext::module() { return *mod; }
IRBuilderBase& CodegenContext::builder() { return *irb; }
TargetMachine& CodegenContext::targetMachine() { return *tm; }

void CodegenContext::beginModule(const std::string& name) {
    mod->setModuleIdentifier(name);
}

void CodegenContext::endModule() {}

}
