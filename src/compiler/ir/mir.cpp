// ============================================================
// mir.cpp — Vitte Compiler
// Mid-level Intermediate Representation (MIR)
// ============================================================

#include "mir.hpp"

#include <cassert>
#include <ostream>
#include <sstream>

namespace vitte::ir {

using ::vitte::frontend::ast::SourceSpan;

// ------------------------------------------------------------
// MIR Node
// ------------------------------------------------------------

MirNode::MirNode(MirKind k, SourceSpan sp)
    : kind(k), span(sp) {}

MirNode::~MirNode() = default;

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

MirType::MirType(MirKind k, SourceSpan sp)
    : MirNode(k, sp) {}

MirNamedType::MirNamedType(
    std::string n,
    SourceSpan sp)
    : MirType(MirKind::NamedType, sp),
      name(std::move(n)) {}

MirProcType::MirProcType(
    std::vector<std::string> params,
    std::string ret,
    SourceSpan sp)
    : MirType(MirKind::ProcType, sp),
      params(std::move(params)),
      ret(std::move(ret)) {}

// ------------------------------------------------------------
// Values
// ------------------------------------------------------------

MirValue::MirValue(MirKind k, SourceSpan sp)
    : MirNode(k, sp) {}

MirLocal::MirLocal(
    std::string n,
    MirTypePtr t,
    SourceSpan sp)
    : MirValue(MirKind::Local, sp),
      name(std::move(n)),
      type(std::move(t)) {}

MirConst::MirConst(
    MirConstKind k,
    std::string v,
    SourceSpan sp)
    : MirValue(MirKind::Const, sp),
      const_kind(k),
      value(std::move(v)) {}

MirMember::MirMember(
    MirValuePtr b,
    std::string m,
    bool ptr,
    SourceSpan sp)
    : MirValue(MirKind::Member, sp),
      base(std::move(b)),
      member(std::move(m)),
      pointer(ptr) {
    assert(base);
}

// ------------------------------------------------------------
// Instructions
// ------------------------------------------------------------

MirInstr::MirInstr(MirKind k, SourceSpan sp)
    : MirNode(k, sp) {}

MirAssign::MirAssign(
    MirLocalPtr dst,
    MirValuePtr src,
    SourceSpan sp)
    : MirInstr(MirKind::Assign, sp),
      dest(std::move(dst)),
      value(std::move(src)) {
    assert(dest && value);
}

MirBinaryOp::MirBinaryOp(
    MirBinOp op,
    MirLocalPtr dst,
    MirValuePtr lhs,
    MirValuePtr rhs,
    SourceSpan sp)
    : MirInstr(MirKind::BinaryOp, sp),
      op(op),
      dest(std::move(dst)),
      left(std::move(lhs)),
      right(std::move(rhs)) {
    assert(dest && left && right);
}

MirCall::MirCall(
    std::string callee,
    std::vector<MirValuePtr> args,
    MirLocalPtr result,
    SourceSpan sp)
    : MirInstr(MirKind::Call, sp),
      callee(std::move(callee)),
      args(std::move(args)),
      result(std::move(result)) {}

MirCallIndirect::MirCallIndirect(
    MirValuePtr callee,
    std::vector<MirValuePtr> args,
    MirLocalPtr result,
    SourceSpan sp)
    : MirInstr(MirKind::CallIndirect, sp),
      callee(std::move(callee)),
      args(std::move(args)),
      result(std::move(result)) {
    assert(this->callee);
}

MirAsm::MirAsm(
    std::string c,
    bool vol,
    SourceSpan sp)
    : MirInstr(MirKind::Asm, sp),
      code(std::move(c)),
      is_volatile(vol) {}

MirUnsafeBegin::MirUnsafeBegin(SourceSpan sp)
    : MirInstr(MirKind::UnsafeBegin, sp) {}

MirUnsafeEnd::MirUnsafeEnd(SourceSpan sp)
    : MirInstr(MirKind::UnsafeEnd, sp) {}

MirReturn::MirReturn(
    MirValuePtr v,
    SourceSpan sp)
    : MirInstr(MirKind::Return, sp),
      value(std::move(v)) {}

// ------------------------------------------------------------
// Control Flow
// ------------------------------------------------------------

MirTerminator::MirTerminator(
    MirKind k,
    SourceSpan sp)
    : MirNode(k, sp) {}

MirGoto::MirGoto(
    MirBlockId target,
    SourceSpan sp)
    : MirTerminator(MirKind::Goto, sp),
      target(target) {}

MirCondGoto::MirCondGoto(
    MirValuePtr cond,
    MirBlockId then_bb,
    MirBlockId else_bb,
    SourceSpan sp)
    : MirTerminator(MirKind::CondGoto, sp),
      cond(std::move(cond)),
      then_block(then_bb),
      else_block(else_bb) {
    assert(this->cond);
}

// ------------------------------------------------------------
// Basic block
// ------------------------------------------------------------

MirBasicBlock::MirBasicBlock(
    MirBlockId id,
    SourceSpan sp)
    : id(id),
      span(sp),
      terminator(nullptr) {}

void MirBasicBlock::set_terminator(MirTerminatorPtr term) {
    assert(!terminator && "terminator already set");
    terminator = std::move(term);
}

// ------------------------------------------------------------
// Function
// ------------------------------------------------------------

MirParam::MirParam(std::string n, MirTypePtr t)
    : name(std::move(n)),
      type(std::move(t)) {}

MirFunction::MirFunction(
    std::string n,
    std::vector<MirParam> p,
    MirTypePtr ret,
    std::vector<MirLocalPtr> locals,
    std::vector<MirBasicBlock> blocks,
    MirBlockId entry,
    SourceSpan sp)
    : name(std::move(n)),
      params(std::move(p)),
      return_type(std::move(ret)),
      locals(std::move(locals)),
      blocks(std::move(blocks)),
      entry_block(entry),
      span(sp) {}

// ------------------------------------------------------------
// Global variable
// ------------------------------------------------------------

MirGlobal::MirGlobal(
    std::string n,
    std::string tn,
    bool mut,
    bool has_init,
    MirConstKind k,
    std::string v,
    SourceSpan sp)
    : name(std::move(n)),
      type_name(std::move(tn)),
      is_mut(mut),
      has_init(has_init),
      init_kind(k),
      init_value(std::move(v)),
      span(sp) {}

// ------------------------------------------------------------
// Type declarations
// ------------------------------------------------------------

MirField::MirField(std::string n, MirFieldType t)
    : name(std::move(n)),
      type(std::move(t)) {}

MirStructDecl::MirStructDecl(std::string n, std::vector<MirField> f)
    : name(std::move(n)),
      fields(std::move(f)) {}

MirEnumDecl::MirEnumDecl(std::string n, std::vector<std::string> items_in)
    : name(std::move(n)),
      items(std::move(items_in)) {}

MirPickCase::MirPickCase(std::string n, std::vector<MirField> f)
    : name(std::move(n)),
      fields(std::move(f)) {}

MirPickDecl::MirPickDecl(std::string n, bool is_enum, std::vector<MirPickCase> c)
    : name(std::move(n)),
      enum_like(is_enum),
      cases(std::move(c)) {}

// ------------------------------------------------------------
// Module
// ------------------------------------------------------------

MirModule::MirModule(
    std::vector<MirStructDecl> structs,
    std::vector<MirEnumDecl> enums,
    std::vector<MirPickDecl> picks,
    std::vector<MirGlobal> globals,
    std::vector<MirFunction> funcs,
    SourceSpan sp)
    : structs(std::move(structs)),
      enums(std::move(enums)),
      picks(std::move(picks)),
      globals(std::move(globals)),
      functions(std::move(funcs)),
      span(sp) {}

// ------------------------------------------------------------
// Debug / dump
// ------------------------------------------------------------

static void indent(std::ostream& os, std::size_t depth) {
    for (std::size_t i = 0; i < depth; ++i) {
        os << "  ";
    }
}

void dump(const MirModule& m, std::ostream& os) {
    os << "MIR Module\n";

    for (const auto& g : m.globals) {
        os << "global " << g.name << " : " << g.type_name;
        if (g.is_mut) {
            os << " (mut)";
        }
        if (g.has_init) {
            os << " = " << g.init_value;
        }
        os << "\n";
    }

    for (const auto& fn : m.functions) {
        os << "fn " << fn.name << " {\n";

        for (const auto& bb : fn.blocks) {
            os << "  bb" << bb.id << ":\n";

            for (const auto& instr : bb.instructions) {
                indent(os, 2);
                os << to_string(instr->kind) << "\n";
            }

            if (bb.terminator) {
                indent(os, 2);
                os << to_string(bb.terminator->kind) << "\n";
            }
        }

        os << "}\n";
    }
}

std::string dump_to_string(const MirModule& m) {
    std::ostringstream ss;
    dump(m, ss);
    return ss.str();
}

const char* to_string(MirKind kind) {
    switch (kind) {
        case MirKind::NamedType: return "NamedType";
        case MirKind::ProcType: return "ProcType";
        case MirKind::Local: return "Local";
        case MirKind::Const: return "Const";
        case MirKind::Member: return "Member";
        case MirKind::Assign: return "Assign";
        case MirKind::BinaryOp: return "BinaryOp";
        case MirKind::Call: return "Call";
        case MirKind::CallIndirect: return "CallIndirect";
        case MirKind::Asm: return "Asm";
        case MirKind::UnsafeBegin: return "UnsafeBegin";
        case MirKind::UnsafeEnd: return "UnsafeEnd";
        case MirKind::Return: return "Return";
        case MirKind::Goto: return "Goto";
        case MirKind::CondGoto: return "CondGoto";
        default:
            return "Unknown";
    }
}

} // namespace vitte::ir
