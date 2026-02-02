#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include "vitte/llvm/context.hpp"

using namespace llvm;

namespace vitte::llvm {

void emitObject(Context& ctx, const std::string& path) {
    legacy::PassManager pm;
    std::error_code ec;
    raw_fd_ostream os(path, ec);
    ctx.tm().addPassesToEmitFile(pm, os, nullptr, CGFT_ObjectFile);
    pm.run(ctx.mod());
}

}
