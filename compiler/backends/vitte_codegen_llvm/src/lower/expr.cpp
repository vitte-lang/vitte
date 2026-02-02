#include <llvm/IR/Value.h>
#include "vitte/codegen/llvm/context.hpp"
#include "vitte/codegen/llvm/lower.hpp"

namespace vitte::codegen::llvm_backend {

void lowerExpr(CodegenContext& ctx, const std::string& expr_ir) {
    // Arithmetic, calls, control flow
    (void)ctx; (void)expr_ir;
}

}
