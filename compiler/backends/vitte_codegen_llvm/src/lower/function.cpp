#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include "vitte/codegen/llvm/context.hpp"
#include "vitte/codegen/llvm/lower.hpp"

namespace vitte::codegen::llvm_backend {

void lowerFunction(CodegenContext& ctx, const std::string& fn_ir) {
    // Create llvm::Function, entry block, ABI lowering
    (void)ctx; (void)fn_ir;
}

}
