#pragma once

namespace vitte::codegen::llvm_backend {

class CodegenContext;

void registerDefaultPasses(CodegenContext&);
void runOptimizations(CodegenContext&, int opt_level);

}
