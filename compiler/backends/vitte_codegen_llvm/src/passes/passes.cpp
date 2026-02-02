#include "vitte/codegen/llvm/passes.hpp"
#include "vitte/codegen/llvm/context.hpp"

namespace vitte::codegen::llvm_backend {

void registerDefaultPasses(CodegenContext&) {}
void runOptimizations(CodegenContext&, int) {}

}
