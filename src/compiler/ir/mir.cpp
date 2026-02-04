// ============================================================
// mir.cpp — Vitte Compiler
// Mid-level Intermediate Representation (MIR)
// ============================================================

#include "mir.hpp"

#include <cassert>
#include <ostream>
#include <sstream>

namespace vitte::ir {

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

MirFunction::MirFunction(
    std::string n,
    std::vector<MirLocalPtr> locals,
    std::vector<MirBasicBlock> blocks,
    MirBlockId entry,
    SourceSpan sp)
    : name(std::move(n)),
      locals(std::move(locals)),
      blocks(std::move(blocks)),
      entry_block(entry),
      span(sp) {}

// ------------------------------------------------------------
// Module
// ------------------------------------------------------------

MirModule::MirModule(
    std::vector<MirFunction> funcs,
    SourceSpan sp)
    : functions(std::move(funcs)),
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

} // namespace vitte::ir