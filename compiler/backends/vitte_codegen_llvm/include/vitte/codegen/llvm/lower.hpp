#pragma once
#include <string>

namespace vitte::codegen::llvm_backend {

class CodegenContext;

void lowerModule(CodegenContext&, const std::string& vitte_ir);
void lowerFunction(CodegenContext&, const std::string& fn_ir);
void lowerExpr(CodegenContext&, const std::string& expr_ir);

}
