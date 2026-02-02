#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include "vitte/codegen/llvm/context.hpp"
#include "vitte/codegen/llvm/emit.hpp"

using namespace llvm;

namespace vitte::codegen::llvm_backend {

void emitIR(CodegenContext& ctx, const std::string& path) {
    std::error_code ec;
    raw_fd_ostream os(path, ec);
    ctx.module().print(os, nullptr);
}

void emitObject(CodegenContext& ctx, const std::string& path) {
    legacy::PassManager pm;
    std::error_code ec;
    raw_fd_ostream os(path, ec);
    ctx.targetMachine().addPassesToEmitFile(
        pm, os, nullptr, CGFT_ObjectFile);
    pm.run(ctx.module());
}

void emitAssembly(CodegenContext& ctx, const std::string& path) {
    legacy::PassManager pm;
    std::error_code ec;
    raw_fd_ostream os(path, ec);
    ctx.targetMachine().addPassesToEmitFile(
        pm, os, nullptr, CGFT_AssemblyFile);
    pm.run(ctx.module());
}

}
