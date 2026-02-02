#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include "vitte/llvm/context.hpp"
#include "vitte/llvm/ssa/ssa.hpp"

namespace vitte::llvm {

void lowerSSA(Context&, const ssa::Function&) {
    // SSA -> LLVM IR
}

}
