#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/MC/TargetRegistry.h>
#include "vitte/llvm/context.hpp"

using namespace llvm;

namespace vitte::llvm {

Context::Context(Session& s, const TargetDesc& t)
    : session(s), target(t) {
    InitializeAllTargets();
    InitializeAllAsmPrinters();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();

    _ctx = std::make_unique<LLVMContext>();
    _mod = std::make_unique<Module>("vitte", *_ctx);
    _mod->setTargetTriple(target.triple);

    std::string err;
    const Target* T = TargetRegistry::lookupTarget(target.triple, err);
    TargetOptions opts;
    _tm.reset(T->createTargetMachine(
        target.triple, target.cpu, target.features, opts, Reloc::PIC));

    _ir = std::make_unique<IRBuilder<>>(*_ctx);
}

Context::~Context() = default;
LLVMContext& Context::ctx() { return *_ctx; }
Module& Context::mod() { return *_mod; }
IRBuilderBase& Context::ir() { return *_ir; }
TargetMachine& Context::tm() { return *_tm; }

}
