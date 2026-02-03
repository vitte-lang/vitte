// ============================================================
// vitte::llvm::ssa
// Interface SSA <-> LLVM pour le backend LLVM Vitte
// ============================================================

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>

namespace vitte::llvm::ssa {

// ------------------------------------------------------------
// Identifiants SSA
// ------------------------------------------------------------

using ValueId  = uint32_t;
using BlockId  = uint32_t;
using FuncId   = uint32_t;


// ------------------------------------------------------------
// Types SSA (vue côté LLVM)
// ------------------------------------------------------------

enum class SsaTypeKind {
    Void,
    Bool,
    I32,
    I64,
    Ptr
};

struct SsaType {
    SsaTypeKind kind;

    static SsaType void_();
    static SsaType bool_();
    static SsaType i32();
    static SsaType i64();
    static SsaType ptr();
};


// ------------------------------------------------------------
// Valeurs SSA
// ------------------------------------------------------------

enum class SsaValueKind {
    Const,
    Param,
    Temp,
    Phi
};

struct SsaValue {
    ValueId id;
    SsaValueKind kind;
    SsaType type;
    std::string name;
};


// ------------------------------------------------------------
// Instructions SSA
// ------------------------------------------------------------

enum class SsaOpcode {
    Add,
    Sub,
    Mul,
    Div,
    And,
    Or,
    Xor,

    CmpEq,
    CmpNe,
    CmpLt,
    CmpLe,
    CmpGt,
    CmpGe,

    Call,
    Load,
    Store,
    Phi
};

struct SsaInstruction {
    SsaOpcode opcode;
    std::vector<ValueId> operands;
    std::optional<ValueId> result;
};


// ------------------------------------------------------------
// Terminators SSA
// ------------------------------------------------------------

enum class SsaTerminatorKind {
    Return,
    Jump,
    Branch,
    Unreachable
};

struct SsaTerminator {
    SsaTerminatorKind kind;

    // Return
    std::optional<ValueId> value;

    // Jump / Branch
    BlockId target;
    BlockId then_block;
    BlockId else_block;
};


// ------------------------------------------------------------
// Bloc SSA
// ------------------------------------------------------------

struct SsaBlock {
    BlockId id;
    std::string label;

    std::vector<SsaInstruction> instructions;
    SsaTerminator terminator;

    std::vector<BlockId> preds;
    std::vector<BlockId> succs;
};


// ------------------------------------------------------------
// Fonction SSA
// ------------------------------------------------------------

struct SsaParam {
    ValueId id;
    std::string name;
    SsaType type;
};

struct SsaFunction {
    FuncId id;
    std::string name;

    std::vector<SsaParam> params;
    SsaType return_type;

    BlockId entry;
    std::unordered_map<BlockId, SsaBlock> blocks;
};


// ------------------------------------------------------------
// Module SSA
// ------------------------------------------------------------

struct SsaModule {
    std::string name;
    std::unordered_map<FuncId, SsaFunction> functions;
};


// ============================================================
// Conversion SSA -> LLVM
// ============================================================

class SsaToLlvm {
public:
    explicit SsaToLlvm(::llvm::LLVMContext& ctx);

    std::unique_ptr<::llvm::Module>
    lower_module(const SsaModule& module);

private:
    ::llvm::LLVMContext& llvm_ctx;
    std::unique_ptr<::llvm::Module> llvm_module;
    ::llvm::IRBuilder<> builder;

    // Tables SSA -> LLVM
    std::unordered_map<ValueId, ::llvm::Value*> value_map;
    std::unordered_map<BlockId, ::llvm::BasicBlock*> block_map;
    std::unordered_map<FuncId, ::llvm::Function*> function_map;

    // ---- Helpers ----
    ::llvm::Type* lower_type(const SsaType& ty);
    ::llvm::Value* lower_value(ValueId id);
    void lower_function(const SsaFunction& fn);
    void lower_block(const SsaFunction& fn, const SsaBlock& block);
    void lower_instruction(const SsaInstruction& inst);
    void lower_terminator(const SsaTerminator& term);
};


// ============================================================
// Utilitaires debug
// ============================================================

std::string to_string(SsaTypeKind kind);
std::string to_string(SsaOpcode op);
std::string to_string(SsaTerminatorKind kind);

} // namespace vitte::llvm::ssa
