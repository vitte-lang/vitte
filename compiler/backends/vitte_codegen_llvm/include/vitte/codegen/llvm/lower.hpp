// ============================================================
// vitte_codegen_llvm::context
// Contexte central du backend LLVM
// ============================================================

#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"

namespace vitte::mir {
struct Function;
struct Block;
struct Expr;
struct Type;

using FnId    = unsigned;
using BlockId = unsigned;
using LocalId = unsigned;
}

namespace vitte::codegen::llvm_backend {


// ------------------------------------------------------------
// Contexte global LLVM (module)
// ------------------------------------------------------------

struct CodegenContext {
    llvm::LLVMContext llvmContext;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;

    // Mapping MIR → LLVM
    std::unordered_map<mir::FnId, llvm::Function*> functions;
    std::unordered_map<mir::FnId, llvm::FunctionType*> functionTypes;

    explicit CodegenContext(const std::string& moduleName)
        : module(std::make_unique<llvm::Module>(moduleName, llvmContext)),
          builder(llvmContext)
    {}

    llvm::Module& getModule() { return *module; }
};


// ------------------------------------------------------------
// Contexte local par fonction
// ------------------------------------------------------------

struct FunctionContext {
    CodegenContext& cg;

    llvm::Function* function;
    llvm::BasicBlock* entryBlock;

    // Locaux MIR → LLVM
    std::unordered_map<mir::LocalId, llvm::Value*> locals;

    // Blocs MIR → LLVM
    std::unordered_map<mir::BlockId, llvm::BasicBlock*> blocks;

    std::string sourceName;

    FunctionContext(CodegenContext& cg,
                    llvm::Function* fn,
                    const std::string& sourceName)
        : cg(cg),
          function(fn),
          sourceName(sourceName)
    {
        entryBlock = llvm::BasicBlock::Create(
            cg.llvmContext,
            "entry",
            function
        );
        cg.builder.SetInsertPoint(entryBlock);
    }

    // --------------------------------------------------------
    // Locaux
    // --------------------------------------------------------

    void declareLocal(mir::LocalId id, llvm::Type* type) {
        auto* allocaInst =
            cg.builder.CreateAlloca(type, nullptr, "local");
        locals[id] = allocaInst;
    }

    llvm::Value* getLocal(mir::LocalId id) const {
        auto it = locals.find(id);
        if (it == locals.end())
            return nullptr;
        return it->second;
    }

    // --------------------------------------------------------
    // Blocs
    // --------------------------------------------------------

    llvm::BasicBlock* getOrCreateBlock(mir::BlockId id) {
        auto it = blocks.find(id);
        if (it != blocks.end())
            return it->second;

        auto* bb = llvm::BasicBlock::Create(
            cg.llvmContext,
            "bb",
            function
        );
        blocks[id] = bb;
        return bb;
    }

    void switchToBlock(mir::BlockId id) {
        cg.builder.SetInsertPoint(
            getOrCreateBlock(id)
        );
    }
};

} // namespace vitte::codegen::llvm_backend
