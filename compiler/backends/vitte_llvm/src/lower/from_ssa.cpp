// ============================================================
// vitte::llvm::ssa::from_ssa
// Lowering SSA Vitte -> LLVM IR
// ============================================================

#include "vitte/llvm/ssa/ssa.hpp"
#include "vitte/llvm/context.hpp"

#include <llvm/IR/Verifier.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>

#include <cassert>

namespace vitte::llvm::ssa {


// ------------------------------------------------------------
// Constructeur
// ------------------------------------------------------------

SsaToLlvm::SsaToLlvm(::llvm::LLVMContext& ctx)
    : llvm_ctx(ctx),
      llvm_module(nullptr),
      builder(ctx)
{
}


// ------------------------------------------------------------
// Entrée principale
// ------------------------------------------------------------

std::unique_ptr<::llvm::Module>
SsaToLlvm::lower_module(const SsaModule& module)
{
    llvm_module =
        std::make_unique<::llvm::Module>(
            module.name,
            llvm_ctx
        );

    // --------------------------------------------------------
    // Déclaration des fonctions
    // --------------------------------------------------------

    for (const auto& [fid, fn] : module.functions) {
        // Types paramètres
        std::vector<::llvm::Type*> params;
        for (const auto& p : fn.params) {
            params.push_back(
                lower_type(p.type)
            );
        }

        auto* retTy =
            lower_type(fn.return_type);

        auto* fnTy =
            ::llvm::FunctionType::get(
                retTy,
                params,
                false
            );

        auto* llvmFn =
            ::llvm::Function::Create(
                fnTy,
                ::llvm::Function::ExternalLinkage,
                fn.name,
                llvm_module.get()
            );

        function_map[fid] = llvmFn;

        // Nom des paramètres
        unsigned idx = 0;
        for (auto& arg : llvmFn->args()) {
            arg.setName(fn.params[idx].name);
            value_map[fn.params[idx].id] = &arg;
            ++idx;
        }
    }

    // --------------------------------------------------------
    // Définition des fonctions
    // --------------------------------------------------------

    for (const auto& [fid, fn] : module.functions) {
        lower_function(fn);
    }

    return std::move(llvm_module);
}


// ------------------------------------------------------------
// Lowering fonction SSA
// ------------------------------------------------------------

void SsaToLlvm::lower_function(const SsaFunction& fn)
{
    auto* llvmFn = function_map.at(fn.id);

    // --------------------------------------------------------
    // Création des basic blocks
    // --------------------------------------------------------

    for (const auto& [bid, block] : fn.blocks) {
        auto* bb =
            ::llvm::BasicBlock::Create(
                llvm_ctx,
                block.label,
                llvmFn
            );
        block_map[bid] = bb;
    }

    // --------------------------------------------------------
    // Corps des blocs
    // --------------------------------------------------------

    for (const auto& [bid, block] : fn.blocks) {
        lower_block(fn, block);
    }

    // Vérification locale (optionnelle)
#ifndef NDEBUG
    assert(!::llvm::verifyFunction(*llvmFn, &::llvm::errs()));
#endif
}


// ------------------------------------------------------------
// Lowering bloc SSA
// ------------------------------------------------------------

void SsaToLlvm::lower_block(const SsaFunction& fn,
                            const SsaBlock& block)
{
    auto* bb = block_map.at(block.id);
    builder.SetInsertPoint(bb);

    // --------------------------------------------------------
    // Instructions
    // --------------------------------------------------------

    for (const auto& inst : block.instructions) {
        lower_instruction(inst);
    }

    // --------------------------------------------------------
    // Terminator
    // --------------------------------------------------------

    lower_terminator(block.terminator);
}


// ------------------------------------------------------------
// Lowering instruction SSA
// ------------------------------------------------------------

void SsaToLlvm::lower_instruction(const SsaInstruction& inst)
{
    ::llvm::Value* result = nullptr;

    auto get = [&](ValueId id) -> ::llvm::Value* {
        return value_map.at(id);
    };

    switch (inst.opcode) {

    case SsaOpcode::Add:
        result = builder.CreateAdd(
            get(inst.operands[0]),
            get(inst.operands[1])
        );
        break;

    case SsaOpcode::Sub:
        result = builder.CreateSub(
            get(inst.operands[0]),
            get(inst.operands[1])
        );
        break;

    case SsaOpcode::Mul:
        result = builder.CreateMul(
            get(inst.operands[0]),
            get(inst.operands[1])
        );
        break;

    case SsaOpcode::Div:
        result = builder.CreateSDiv(
            get(inst.operands[0]),
            get(inst.operands[1])
        );
        break;

    case SsaOpcode::And:
        result = builder.CreateAnd(
            get(inst.operands[0]),
            get(inst.operands[1])
        );
        break;

    case SsaOpcode::Or:
        result = builder.CreateOr(
            get(inst.operands[0]),
            get(inst.operands[1])
        );
        break;

    case SsaOpcode::Xor:
        result = builder.CreateXor(
            get(inst.operands[0]),
            get(inst.operands[1])
        );
        break;

    case SsaOpcode::Call: {
        auto* callee =
            llvm_module->getFunction(
                inst.operands[0] == 0
                    ? ""
                    : ""
            );
        // NOTE: mapping réel via table symboles ultérieure
        std::vector<::llvm::Value*> args;
        for (size_t i = 1; i < inst.operands.size(); ++i) {
            args.push_back(get(inst.operands[i]));
        }
        result = builder.CreateCall(callee, args);
        break;
    }

    default:
        // opcode non encore supporté
        break;
    }

    if (inst.result.has_value() && result) {
        value_map[inst.result.value()] = result;
    }
}


// ------------------------------------------------------------
// Lowering terminator SSA
// ------------------------------------------------------------

void SsaToLlvm::lower_terminator(const SsaTerminator& term)
{
    switch (term.kind) {

    case SsaTerminatorKind::Return:
        if (term.value.has_value()) {
            builder.CreateRet(
                value_map.at(term.value.value())
            );
        } else {
            builder.CreateRetVoid();
        }
        break;

    case SsaTerminatorKind::Jump:
        builder.CreateBr(
            block_map.at(term.target)
        );
        break;

    case SsaTerminatorKind::Branch:
        builder.CreateCondBr(
            value_map.at(term.value.value()),
            block_map.at(term.then_block),
            block_map.at(term.else_block)
        );
        break;

    case SsaTerminatorKind::Unreachable:
        builder.CreateUnreachable();
        break;
    }
}


// ------------------------------------------------------------
// Lowering type SSA -> LLVM
// ------------------------------------------------------------

::llvm::Type*
SsaToLlvm::lower_type(const SsaType& ty)
{
    switch (ty.kind) {
    case SsaTypeKind::Void:
        return ::llvm::Type::getVoidTy(llvm_ctx);
    case SsaTypeKind::Bool:
        return ::llvm::Type::getInt1Ty(llvm_ctx);
    case SsaTypeKind::I32:
        return ::llvm::Type::getInt32Ty(llvm_ctx);
    case SsaTypeKind::I64:
        return ::llvm::Type::getInt64Ty(llvm_ctx);
    case SsaTypeKind::Ptr:
        return ::llvm::Type::getInt8PtrTy(llvm_ctx);
    default:
        return ::llvm::Type::getVoidTy(llvm_ctx);
    }
}

} // namespace vitte::llvm::ssa
