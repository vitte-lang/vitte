#include "vitte/codegen/llvm/lower.hpp"
#include "vitte/codegen/llvm/context.hpp"

namespace vitte::codegen::llvm_backend {

void lowerModule(CodegenContext& ctx, const std::string& ir) {
    // Walk Vitte IR module, lower functions and globals
    (void)ctx; (void)ir;
}

}
