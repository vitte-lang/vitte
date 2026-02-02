#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Scalar.h>
#include "vitte/llvm/context.hpp"

namespace vitte::llvm {

void runOptimizations(Context& ctx, int level) {
    llvm::legacy::PassManager pm;
    if (level >= 2) {
        pm.add(llvm::createInstructionCombiningPass());
        pm.add(llvm::createReassociatePass());
        pm.add(llvm::createGVNPass());
        pm.add(llvm::createCFGSimplificationPass());
    }
    pm.run(ctx.mod());
}

}
