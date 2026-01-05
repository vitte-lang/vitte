#include "LLVMWrapper.h"

#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/Module.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm;

struct Linker {
  Linker L;
  LLVMContext &Ctx;

  Linker(Module &M) : L(M), Ctx(M.getContext()) {}
};

extern "C" Linker *LLVMLinkerNew(LLVMModuleRef DstRef) {
  Module *Dst = unwrap(DstRef);

  return new Linker(*Dst);
}

extern "C" void LLVMLinkerFree(Linker *L) { delete L; }

extern "C" bool LLVMLinkerAdd(Linker *L, char *BC, size_t Len) {
  std::unique_ptr<MemoryBuffer> Buf =
      MemoryBuffer::getMemBufferCopy(StringRef(BC, Len));

  Expected<std::unique_ptr<Module>> SrcOrError =
      llvm::getLazyBitcodeModule(Buf->getMemBufferRef(), L->Ctx);
  if (!SrcOrError) {
    LLVMSetLastError(toString(SrcOrError.takeError()).c_str());
    return false;
  }

  auto Src = std::move(*SrcOrError);

  if (L->L.linkInModule(std::move(Src))) {
    LLVMSetLastError("");
    return false;
  }
  return true;
}
