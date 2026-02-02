#pragma once
#include <string>

namespace vitte::codegen::llvm_backend {

class CodegenContext;

void emitIR(CodegenContext&, const std::string& path);
void emitObject(CodegenContext&, const std::string& path);
void emitAssembly(CodegenContext&, const std::string& path);

}
