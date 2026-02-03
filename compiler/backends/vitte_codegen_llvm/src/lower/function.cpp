// ============================================================
// vitte_codegen_llvm::lower::function
// Lowering des fonctions MIR -> LLVM IR
// ============================================================

#include "vitte/codegen/llvm/context.hpp"

#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Instructions.h"

#include <cassert>
#include <vector>

namespace vitte::codegen::llvm_backend::lower {

using namespace vitte::mir;


// ------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------

llvm::Value* lowerExpression(FunctionContext& ctx,
                             const Expr& expr);

static void lowerBlock(FunctionContext& ctx,
                       const Block& block);


// ------------------------------------------------------------
// Création du type de fonction LLVM
// ------------------------------------------------------------

static llvm::FunctionType*
lowerFunctionType(CodegenContext& cg,
                  const Function& fn)
{
    std::vector<llvm::Type*> params;

    // Hypothèse simple : tous les types sont déjà normalisés
    for (const auto& p : fn.params) {
        // Placeholder : i64 par défaut
        params.push_back(
            llvm::Type::getInt64Ty(cg.llvmContext)
        );
    }

    llvm::Type* retTy =
        fn.returnsVoid
            ? llvm::Type::getVoidTy(cg.llvmContext)
            : llvm::Type::getInt64Ty(cg.llvmContext);

    return llvm::FunctionType::get(retTy, params, false);
}


// ------------------------------------------------------------
// Entrée principale : lowering d’une fonction
// ------------------------------------------------------------

void lowerFunction(CodegenContext& cg,
                   const Function& fn)
{
    // --------------------------------------------------------
    // Création de la fonction LLVM
    // --------------------------------------------------------

    llvm::FunctionType* fnTy =
        lowerFunctionType(cg, fn);

    llvm::Function* llvmFn =
        llvm::Function::Create(
            fnTy,
            llvm::Function::ExternalLinkage,
            fn.name,
            cg.getModule()
        );

    cg.functions[fn.id] = llvmFn;
    cg.functionTypes[fn.id] = fnTy;

    // --------------------------------------------------------
    // Contexte local par fonction
    // --------------------------------------------------------

    FunctionContext fctx(
        cg,
        llvmFn,
        fn.name
    );

    // --------------------------------------------------------
    // Paramètres
    // --------------------------------------------------------

    unsigned index = 0;
    for (auto& arg : llvmFn->args()) {
        arg.setName("arg" + std::to_string(index));

        // Allocation locale (alloca en entry)
        auto* allocaInst =
            fctx.cg.builder.CreateAlloca(
                arg.getType(),
                nullptr,
                arg.getName()
            );

        // Stockage initial
        fctx.cg.builder.CreateStore(
            &arg,
            allocaInst
        );

        fctx.locals[fn.params[index].localId] =
            allocaInst;

        ++index;
    }

    // --------------------------------------------------------
    // Déclaration des variables locales MIR
    // --------------------------------------------------------

    for (const auto& local : fn.locals) {
        // Placeholder : i64
        fctx.declareLocal(
            local.id,
            llvm::Type::getInt64Ty(
                cg.llvmContext
            )
        );
    }

    // --------------------------------------------------------
    // Lowering des blocs MIR
    // --------------------------------------------------------

    for (const auto& block : fn.blocks) {
        lowerBlock(fctx, block);
    }

    // --------------------------------------------------------
    // Vérification LLVM
    // --------------------------------------------------------

    assert(
        !llvm::verifyFunction(
            *llvmFn,
            &llvm::errs()
        ) && "LLVM function verification failed"
    );
}


// ------------------------------------------------------------
// Lowering d’un bloc MIR
// ------------------------------------------------------------

static void lowerBlock(FunctionContext& ctx,
                       const Block& block)
{
    llvm::BasicBlock* bb =
        ctx.getOrCreateBlock(block.id);

    ctx.cg.builder.SetInsertPoint(bb);

    // --------------------------------------------------------
    // Statements
    // --------------------------------------------------------

    for (const auto& stmt : block.stmts) {
        // Hypothèse : stmt est une expression
        // (en attendant lower/stmt.cpp)
        lowerExpression(ctx, *stmt);
    }

    // --------------------------------------------------------
    // Terminator
    // --------------------------------------------------------

    switch (block.terminator.kind) {
        case TerminatorKind::Return: {
            if (block.terminator.value) {
                llvm::Value* retVal =
                    lowerExpression(
                        ctx,
                        *block.terminator.value
                    );
                ctx.cg.builder.CreateRet(retVal);
            } else {
                ctx.cg.builder.CreateRetVoid();
            }
            break;
        }

        case TerminatorKind::Goto: {
            llvm::BasicBlock* target =
                ctx.getOrCreateBlock(
                    block.terminator.target
                );
            ctx.cg.builder.CreateBr(target);
            break;
        }

        case TerminatorKind::If: {
            llvm::Value* cond =
                lowerExpression(
                    ctx,
                    *block.terminator.cond
                );

            llvm::BasicBlock* thenBB =
                ctx.getOrCreateBlock(
                    block.terminator.thenBlock
                );
            llvm::BasicBlock* elseBB =
                ctx.getOrCreateBlock(
                    block.terminator.elseBlock
                );

            ctx.cg.builder.CreateCondBr(
                cond,
                thenBB,
                elseBB
            );
            break;
        }

        default:
            assert(false && "unsupported MIR terminator");
    }
}

} // namespace vitte::codegen::llvm_backend::lower
