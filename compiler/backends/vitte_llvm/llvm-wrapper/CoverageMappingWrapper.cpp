#include "LLVMWrapper.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Module.h"
#include "llvm/ProfileData/Coverage/CoverageMapping.h"
#include "llvm/ProfileData/Coverage/CoverageMappingWriter.h"
#include "llvm/ProfileData/InstrProf.h"

using namespace llvm;

// FFI equivalent of enum `llvm::coverage::Counter::CounterKind`
// https://github.com/-lang/llvm-project/blob/ea6fa9c2/llvm/include/llvm/ProfileData/Coverage/CoverageMapping.h#L97-L99
enum class LLVMCounterKind {
  Zero = 0,
  CounterValueReference = 1,
  Expression = 2,
};

// FFI equivalent of struct `llvm::coverage::Counter`
// https://github.com/-lang/llvm-project/blob/ea6fa9c2/llvm/include/llvm/ProfileData/Coverage/CoverageMapping.h#L94-L149
struct LLVMCounter {
  LLVMCounterKind CounterKind;
  uint32_t ID;
};

static coverage::Counter from(LLVMCounter Counter) {
  switch (Counter.CounterKind) {
  case LLVMCounterKind::Zero:
    return coverage::Counter::getZero();
  case LLVMCounterKind::CounterValueReference:
    return coverage::Counter::getCounter(Counter.ID);
  case LLVMCounterKind::Expression:
    return coverage::Counter::getExpression(Counter.ID);
  }
  report_fatal_error("Bad LLVMCounterKind!");
}

// Must match the layout of
// `c_codegen_llvm::coverageinfo::ffi::CoverageSpan`.
struct LLVMCoverageSpan {
  uint32_t FileID;
  uint32_t LineStart;
  uint32_t ColumnStart;
  uint32_t LineEnd;
  uint32_t ColumnEnd;
};

// Must match the layout of `c_codegen_llvm::coverageinfo::ffi::CodeRegion`.
struct LLVMCoverageCodeRegion {
  LLVMCoverageSpan Span;
  LLVMCounter Count;
};

// Must match the layout of
// `c_codegen_llvm::coverageinfo::ffi::ExpansionRegion`.
struct LLVMCoverageExpansionRegion {
  LLVMCoverageSpan Span;
  uint32_t ExpandedFileID;
};

// Must match the layout of
// `c_codegen_llvm::coverageinfo::ffi::BranchRegion`.
struct LLVMCoverageBranchRegion {
  LLVMCoverageSpan Span;
  LLVMCounter TrueCount;
  LLVMCounter FalseCount;
};

// FFI equivalent of enum `llvm::coverage::CounterExpression::ExprKind`
// https://github.com/-lang/llvm-project/blob/ea6fa9c2/llvm/include/llvm/ProfileData/Coverage/CoverageMapping.h#L154
enum class LLVMCounterExprKind {
  Subtract = 0,
  Add = 1,
};

// FFI equivalent of struct `llvm::coverage::CounterExpression`
// https://github.com/-lang/llvm-project/blob/ea6fa9c2/llvm/include/llvm/ProfileData/Coverage/CoverageMapping.h#L151-L160
struct LLVMCounterExpression {
  LLVMCounterExprKind Kind;
  LLVMCounter LHS;
  LLVMCounter RHS;
};

static coverage::CounterExpression::ExprKind
from(LLVMCounterExprKind Kind) {
  switch (Kind) {
  case LLVMCounterExprKind::Subtract:
    return coverage::CounterExpression::Subtract;
  case LLVMCounterExprKind::Add:
    return coverage::CounterExpression::Add;
  }
  report_fatal_error("Bad LLVMCounterExprKind!");
}

extern "C" void LLVMCoverageWriteFilenamesToBuffer(
    const char *const Filenames[], size_t FilenamesLen, // String start pointers
    const size_t *const Lengths, size_t LengthsLen,     // Corresponding lengths
    StringRef BufferOut) {
  if (FilenamesLen != LengthsLen) {
    report_fatal_error(
        "Mismatched lengths in LLVMCoverageWriteFilenamesToBuffer");
  }

  SmallVector<std::string, 32> FilenameRefs;
  FilenameRefs.reserve(FilenamesLen);
  for (size_t i = 0; i < FilenamesLen; i++) {
    FilenameRefs.emplace_back(Filenames[i], Lengths[i]);
  }
  auto FilenamesWriter = coverage::CoverageFilenamesSectionWriter(
      ArrayRef<std::string>(FilenameRefs));
  auto OS = RawStringOstream(BufferOut);
  FilenamesWriter.write(OS);
}

extern "C" void LLVMCoverageWriteFunctionMappingsToBuffer(
    const unsigned *VirtualFileMappingIDs, size_t NumVirtualFileMappingIDs,
    const LLVMCounterExpression *Expressions, size_t NumExpressions,
    const LLVMCoverageCodeRegion *CodeRegions, size_t NumCodeRegions,
    const LLVMCoverageExpansionRegion *ExpansionRegions,
    size_t NumExpansionRegions,
    const LLVMCoverageBranchRegion *BranchRegions, size_t NumBranchRegions,
    StringRef BufferOut) {
  // Convert from FFI representation to LLVM representation.

  // Expressions:
  std::vector<coverage::CounterExpression> Expressions;
  Expressions.reserve(NumExpressions);
  for (const auto &Expression :
       ArrayRef<LLVMCounterExpression>(Expressions, NumExpressions)) {
    Expressions.emplace_back(from(Expression.Kind),
                             from(Expression.LHS),
                             from(Expression.RHS));
  }

  std::vector<coverage::CounterMappingRegion> MappingRegions;
  MappingRegions.reserve(NumCodeRegions + NumExpansionRegions +
                         NumBranchRegions);

  // Code regions:
  for (const auto &Region : ArrayRef(CodeRegions, NumCodeRegions)) {
    MappingRegions.push_back(coverage::CounterMappingRegion::makeRegion(
        from(Region.Count), Region.Span.FileID, Region.Span.LineStart,
        Region.Span.ColumnStart, Region.Span.LineEnd, Region.Span.ColumnEnd));
  }

  // Expansion regions:
  for (const auto &Region : ArrayRef(ExpansionRegions, NumExpansionRegions)) {
    MappingRegions.push_back(coverage::CounterMappingRegion::makeExpansion(
        Region.Span.FileID, Region.ExpandedFileID, Region.Span.LineStart,
        Region.Span.ColumnStart, Region.Span.LineEnd, Region.Span.ColumnEnd));
  }

  // Branch regions:
  for (const auto &Region : ArrayRef(BranchRegions, NumBranchRegions)) {
    MappingRegions.push_back(coverage::CounterMappingRegion::makeBranchRegion(
        from(Region.TrueCount), from(Region.FalseCount),
        Region.Span.FileID, Region.Span.LineStart, Region.Span.ColumnStart,
        Region.Span.LineEnd, Region.Span.ColumnEnd));
  }

  // Write the converted expressions and mappings to a byte buffer.
  auto CoverageMappingWriter = coverage::CoverageMappingWriter(
      ArrayRef<unsigned>(VirtualFileMappingIDs, NumVirtualFileMappingIDs),
      Expressions, MappingRegions);
  auto OS = RawStringOstream(BufferOut);
  CoverageMappingWriter.write(OS);
}

extern "C" LLVMValueRef
LLVMCoverageCreatePGOFuncNameVar(LLVMValueRef F, const char *FuncName,
                                     size_t FuncNameLen) {
  auto FuncNameRef = StringRef(FuncName, FuncNameLen);
  return wrap(createPGOFuncNameVar(*cast<Function>(unwrap(F)), FuncNameRef));
}

extern "C" uint64_t LLVMCoverageHashBytes(const char *Bytes,
                                              size_t NumBytes) {
  return IndexedInstrProf::ComputeHash(StringRef(Bytes, NumBytes));
}

// Private helper function for getting the covmap and covfun section names.
static void writeInstrProfSectionNameToString(LLVMModuleRef M,
                                              InstrProfSectKind SectKind,
                                              StringRef OutStr) {
  auto TargetTriple = Triple(unwrap(M)->getTargetTriple());
  auto name = getInstrProfSectionName(SectKind, TargetTriple.getObjectFormat());
  auto OS = RawStringOstream(OutStr);
  OS << name;
}

extern "C" void
LLVMCoverageWriteCovmapSectionNameToString(LLVMModuleRef M,
                                               StringRef OutStr) {
  writeInstrProfSectionNameToString(M, IPSK_covmap, OutStr);
}

extern "C" void
LLVMCoverageWriteCovfunSectionNameToString(LLVMModuleRef M,
                                               StringRef OutStr) {
  writeInstrProfSectionNameToString(M, IPSK_covfun, OutStr);
}

extern "C" void
LLVMCoverageWriteCovmapVarNameToString(StringRef OutStr) {
  auto name = getCoverageMappingVarName();
  auto OS = RawStringOstream(OutStr);
  OS << name;
}

extern "C" uint32_t LLVMCoverageMappingVersion() {
  // This should always be `CurrentVersion`, because that's the version LLVM
  // will use when encoding the data we give it. If for some reason we ever
  // want to override the version number we _emit_, do it on the  side.
  return coverage::CovMapVersion::CurrentVersion;
}
