// ============================================================
// mir.hpp — Vitte Compiler
// Mid-level Intermediate Representation (MIR)
// ============================================================

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "ast.hpp" // SourceSpan

namespace vitte::ir {

// ------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------

struct MirNode;
struct MirType;
struct MirValue;
struct MirInstr;
struct MirTerminator;
struct MirBasicBlock;
struct MirFunction;
struct MirModule;

// ------------------------------------------------------------
// Pointer aliases
// ------------------------------------------------------------

using MirNodePtr       = std::unique_ptr<MirNode>;
using MirTypePtr       = std::unique_ptr<MirType>;
using MirValuePtr      = std::unique_ptr<MirValue>;
using MirInstrPtr      = std::unique_ptr<MirInstr>;
using MirTerminatorPtr = std::unique_ptr<MirTerminator>;
using MirLocalPtr      = std::unique_ptr<struct MirLocal>;

// ------------------------------------------------------------
// IDs
// ------------------------------------------------------------

using MirBlockId = std::size_t;

// ------------------------------------------------------------
// MIR Kind
// ------------------------------------------------------------

enum class MirKind {
    // types
    NamedType,

    // values
    Local,
    Const,

    // instructions
    Assign,
    BinaryOp,
    Call,
    Return,

    // control flow
    Goto,
    CondGoto,
};

// ------------------------------------------------------------
// Base node
// ------------------------------------------------------------

struct MirNode {
    MirKind kind;
    vitte::frontend::ast::SourceSpan span;

    explicit MirNode(MirKind kind,
                     vitte::frontend::ast::SourceSpan span);
    virtual ~MirNode();
};

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

struct MirType : MirNode {
    explicit MirType(MirKind kind,
                     vitte::frontend::ast::SourceSpan span);
};

struct MirNamedType : MirType {
    std::string name;

    MirNamedType(std::string name,
                 vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Values
// ------------------------------------------------------------

enum class MirConstKind {
    Bool,
    Int,
    String,
};

struct MirValue : MirNode {
    explicit MirValue(MirKind kind,
                      vitte::frontend::ast::SourceSpan span);
};

struct MirLocal : MirValue {
    std::string name;
    MirTypePtr type;

    MirLocal(std::string name,
             MirTypePtr type,
             vitte::frontend::ast::SourceSpan span);
};

struct MirConst : MirValue {
    MirConstKind const_kind;
    std::string value;

    MirConst(MirConstKind kind,
             std::string value,
             vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Instructions
// ------------------------------------------------------------

struct MirInstr : MirNode {
    explicit MirInstr(MirKind kind,
                      vitte::frontend::ast::SourceSpan span);
};

enum class MirBinOp {
    Add,
    Sub,
    Mul,
    Div,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or,
};

struct MirAssign : MirInstr {
    MirLocalPtr dest;
    MirValuePtr value;

    MirAssign(MirLocalPtr dest,
              MirValuePtr value,
              vitte::frontend::ast::SourceSpan span);
};

struct MirBinaryOp : MirInstr {
    MirBinOp op;
    MirLocalPtr dest;
    MirValuePtr left;
    MirValuePtr right;

    MirBinaryOp(MirBinOp op,
                MirLocalPtr dest,
                MirValuePtr lhs,
                MirValuePtr rhs,
                vitte::frontend::ast::SourceSpan span);
};

struct MirCall : MirInstr {
    std::string callee;
    std::vector<MirValuePtr> args;
    MirLocalPtr result; // may be null

    MirCall(std::string callee,
            std::vector<MirValuePtr> args,
            MirLocalPtr result,
            vitte::frontend::ast::SourceSpan span);
};

struct MirReturn : MirInstr {
    MirValuePtr value; // may be null

    MirReturn(MirValuePtr value,
              vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Control flow
// ------------------------------------------------------------

struct MirTerminator : MirNode {
    explicit MirTerminator(MirKind kind,
                           vitte::frontend::ast::SourceSpan span);
};

struct MirGoto : MirTerminator {
    MirBlockId target;

    MirGoto(MirBlockId target,
            vitte::frontend::ast::SourceSpan span);
};

struct MirCondGoto : MirTerminator {
    MirValuePtr cond;
    MirBlockId then_block;
    MirBlockId else_block;

    MirCondGoto(MirValuePtr cond,
                MirBlockId then_block,
                MirBlockId else_block,
                vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Basic block
// ------------------------------------------------------------

struct MirBasicBlock {
    MirBlockId id;
    vitte::frontend::ast::SourceSpan span;

    std::vector<MirInstrPtr> instructions;
    MirTerminatorPtr terminator;

    MirBasicBlock(MirBlockId id,
                  vitte::frontend::ast::SourceSpan span);

    void set_terminator(MirTerminatorPtr term);
};

// ------------------------------------------------------------
// Function
// ------------------------------------------------------------

struct MirFunction {
    std::string name;
    std::vector<MirLocalPtr> locals;
    std::vector<MirBasicBlock> blocks;
    MirBlockId entry_block;
    vitte::frontend::ast::SourceSpan span;

    MirFunction(std::string name,
                std::vector<MirLocalPtr> locals,
                std::vector<MirBasicBlock> blocks,
                MirBlockId entry_block,
                vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Module
// ------------------------------------------------------------

struct MirModule {
    std::vector<MirFunction> functions;
    vitte::frontend::ast::SourceSpan span;

    MirModule(std::vector<MirFunction> functions,
              vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Debug helpers
// ------------------------------------------------------------

const char* to_string(MirKind kind);

void dump(const MirModule& module,
          std::ostream& os);

std::string dump_to_string(const MirModule& module);

} // namespace vitte::ir