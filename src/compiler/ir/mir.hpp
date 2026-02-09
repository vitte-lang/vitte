// ============================================================
// mir.hpp — Vitte Compiler
// Mid-level Intermediate Representation (MIR)
// ============================================================

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../frontend/ast.hpp" // SourceSpan

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
struct MirGlobal;
struct MirStructDecl;
struct MirEnumDecl;
struct MirPickDecl;

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
    ProcType,

    // values
    Local,
    Const,
    Member,

    // instructions
    Assign,
    BinaryOp,
    Call,
    CallIndirect,
    Asm,
    UnsafeBegin,
    UnsafeEnd,
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

struct MirProcType : MirType {
    std::vector<std::string> params;
    std::string ret;

    MirProcType(std::vector<std::string> params,
                std::string ret,
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

struct MirMember : MirValue {
    MirValuePtr base;
    std::string member;
    bool pointer;

    MirMember(MirValuePtr base,
              std::string member,
              bool pointer,
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

struct MirCallIndirect : MirInstr {
    MirValuePtr callee;
    std::vector<MirValuePtr> args;
    MirLocalPtr result; // may be null

    MirCallIndirect(MirValuePtr callee,
                    std::vector<MirValuePtr> args,
                    MirLocalPtr result,
                    vitte::frontend::ast::SourceSpan span);
};

struct MirAsm : MirInstr {
    std::string code;
    bool is_volatile;

    MirAsm(std::string code,
           bool is_volatile,
           vitte::frontend::ast::SourceSpan span);
};

struct MirUnsafeBegin : MirInstr {
    MirUnsafeBegin(vitte::frontend::ast::SourceSpan span);
};

struct MirUnsafeEnd : MirInstr {
    MirUnsafeEnd(vitte::frontend::ast::SourceSpan span);
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

struct MirParam {
    std::string name;
    MirTypePtr type;

    MirParam(std::string name,
             MirTypePtr type);
};

// ------------------------------------------------------------
// Function
// ------------------------------------------------------------

struct MirFunction {
    std::string name;
    std::vector<MirParam> params;
    MirTypePtr return_type;
    std::vector<MirLocalPtr> locals;
    std::vector<MirBasicBlock> blocks;
    MirBlockId entry_block;
    vitte::frontend::ast::SourceSpan span;

    MirFunction(std::string name,
                std::vector<MirParam> params,
                MirTypePtr return_type,
                std::vector<MirLocalPtr> locals,
                std::vector<MirBasicBlock> blocks,
                MirBlockId entry_block,
                vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Global variable
// ------------------------------------------------------------

struct MirGlobal {
    std::string name;
    std::string type_name;
    bool is_mut;
    bool has_init;
    MirConstKind init_kind;
    std::string init_value;
    vitte::frontend::ast::SourceSpan span;

    MirGlobal(std::string name,
              std::string type_name,
              bool is_mut,
              bool has_init,
              MirConstKind init_kind,
              std::string init_value,
              vitte::frontend::ast::SourceSpan span);
};

// ------------------------------------------------------------
// Type declarations
// ------------------------------------------------------------

struct MirFieldType {
    enum class Kind {
        Named,
        Proc
    };

    Kind kind = Kind::Named;
    std::string name;
    std::vector<std::string> params;
    std::string ret;
};

struct MirField {
    std::string name;
    MirFieldType type;

    MirField(std::string name, MirFieldType type);
};

struct MirStructDecl {
    std::string name;
    std::vector<MirField> fields;

    MirStructDecl(std::string name, std::vector<MirField> fields);
};

struct MirEnumDecl {
    std::string name;
    std::vector<std::string> items;

    MirEnumDecl(std::string name, std::vector<std::string> items);
};

struct MirPickCase {
    std::string name;
    std::vector<MirField> fields;

    MirPickCase(std::string name, std::vector<MirField> fields);
};

struct MirPickDecl {
    std::string name;
    bool enum_like = false;
    std::vector<MirPickCase> cases;

    MirPickDecl(std::string name, bool enum_like, std::vector<MirPickCase> cases);
};

// ------------------------------------------------------------
// Module
// ------------------------------------------------------------

struct MirModule {
    std::vector<MirStructDecl> structs;
    std::vector<MirEnumDecl> enums;
    std::vector<MirPickDecl> picks;
    std::vector<MirGlobal> globals;
    std::vector<MirFunction> functions;
    vitte::frontend::ast::SourceSpan span;

    MirModule(std::vector<MirStructDecl> structs,
              std::vector<MirEnumDecl> enums,
              std::vector<MirPickDecl> picks,
              std::vector<MirGlobal> globals,
              std::vector<MirFunction> functions,
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
