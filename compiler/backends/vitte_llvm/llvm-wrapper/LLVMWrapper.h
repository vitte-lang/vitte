#ifndef INCLUDED_C_LLVM_LLVMWRAPPER_H
#define INCLUDED_C_LLVM_LLVMWRAPPER_H

#include "SuppressLLVMWarnings.h"

#include "llvm/Config/llvm-config.h"  // LLVM_VERSION_MAJOR, LLVM_VERSION_MINOR
#include "llvm/Support/raw_ostream.h" // llvm::raw_ostream
#include <cstddef>                    // size_t etc
#include <cstdint>                    // uint64_t etc

#define LLVM_VERSION_GE(major, minor)                                          \
  (LLVM_VERSION_MAJOR > (major) ||                                             \
   LLVM_VERSION_MAJOR == (major) && LLVM_VERSION_MINOR >= (minor))

#define LLVM_VERSION_LT(major, minor) (!LLVM_VERSION_GE((major), (minor)))

extern "C" void LLVMSetLastError(const char *);

enum class LLVMResult { Success, Failure };

typedef struct OpaqueString *StringRef;
typedef struct LLVMOpaqueTwine *LLVMTwineRef;
typedef struct LLVMOpaqueSMDiagnostic *LLVMSMDiagnosticRef;

extern "C" void LLVMStringWriteImpl(StringRef buf,
                                        const char *slice_ptr,
                                        size_t slice_len);

class RawStringOstream : public llvm::raw_ostream {
  StringRef Str;
  uint64_t Pos;

  void write_impl(const char *Ptr, size_t Size) override {
    LLVMStringWriteImpl(Str, Ptr, Size);
    Pos += Size;
  }

  uint64_t current_pos() const override { return Pos; }

public:
  explicit RawStringOstream(StringRef Str) : Str(Str), Pos(0) {}

  ~RawStringOstream() {
    // LLVM requires this.
    flush();
  }
};

#endif // INCLUDED_C_LLVM_LLVMWRAPPER_H
