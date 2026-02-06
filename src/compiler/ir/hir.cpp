#include "hir.hpp"

#include <sstream>

namespace vitte::ir {

// ------------------------------------------------------------
// Base nodes
// ------------------------------------------------------------

HirNode::HirNode(HirKind k, vitte::frontend::ast::SourceSpan sp)
    : kind(k), span(sp) {}

HirNode::~HirNode() = default;

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

HirType::HirType(HirKind k, vitte::frontend::ast::SourceSpan sp)
    : HirNode(k, sp) {}

HirNamedType::HirNamedType(std::string n, vitte::frontend::ast::SourceSpan sp)
    : HirType(HirKind::NamedType, sp), name(std::move(n)) {}

HirGenericType::HirGenericType(
    std::string base,
    std::vector<HirTypeId> args,
    vitte::frontend::ast::SourceSpan sp)
    : HirType(HirKind::GenericType, sp),
      base_name(std::move(base)),
      type_args(std::move(args)) {}

// ------------------------------------------------------------
// Expressions
// ------------------------------------------------------------

HirExpr::HirExpr(HirKind k, vitte::frontend::ast::SourceSpan sp)
    : HirNode(k, sp) {}

HirLiteralExpr::HirLiteralExpr(
    HirLiteralKind k,
    std::string v,
    vitte::frontend::ast::SourceSpan sp)
    : HirExpr(HirKind::LiteralExpr, sp),
      lit_kind(k),
      value(std::move(v)) {}

HirVarExpr::HirVarExpr(std::string n, vitte::frontend::ast::SourceSpan sp)
    : HirExpr(HirKind::VarExpr, sp), name(std::move(n)) {}

HirUnaryExpr::HirUnaryExpr(
    HirUnaryOp o,
    HirExprId e,
    vitte::frontend::ast::SourceSpan sp)
    : HirExpr(HirKind::UnaryExpr, sp),
      op(o),
      expr(e) {}

HirBinaryExpr::HirBinaryExpr(
    HirBinaryOp o,
    HirExprId l,
    HirExprId r,
    vitte::frontend::ast::SourceSpan sp)
    : HirExpr(HirKind::BinaryExpr, sp),
      op(o),
      lhs(l),
      rhs(r) {}

HirCallExpr::HirCallExpr(
    HirExprId c,
    std::vector<HirExprId> a,
    vitte::frontend::ast::SourceSpan sp)
    : HirExpr(HirKind::CallExpr, sp),
      callee(c),
      args(std::move(a)) {}

// ------------------------------------------------------------
// Statements
// ------------------------------------------------------------

HirStmt::HirStmt(HirKind k, vitte::frontend::ast::SourceSpan sp)
    : HirNode(k, sp) {}

HirLetStmt::HirLetStmt(
    std::string n,
    HirTypeId t,
    HirExprId i,
    vitte::frontend::ast::SourceSpan sp)
    : HirStmt(HirKind::LetStmt, sp),
      name(std::move(n)),
      type(t),
      init(i) {}

HirExprStmt::HirExprStmt(HirExprId e, vitte::frontend::ast::SourceSpan sp)
    : HirStmt(HirKind::ExprStmt, sp),
      expr(e) {}

HirReturnStmt::HirReturnStmt(HirExprId e, vitte::frontend::ast::SourceSpan sp)
    : HirStmt(HirKind::ReturnStmt, sp),
      expr(e) {}

HirBlock::HirBlock(std::vector<HirStmtId> s, vitte::frontend::ast::SourceSpan sp)
    : HirStmt(HirKind::Block, sp),
      stmts(std::move(s)) {}

HirIf::HirIf(
    HirExprId c,
    HirStmtId t,
    HirStmtId e,
    vitte::frontend::ast::SourceSpan sp)
    : HirStmt(HirKind::IfStmt, sp),
      cond(c),
      then_block(t),
      else_block(e) {}

HirLoop::HirLoop(HirStmtId b, vitte::frontend::ast::SourceSpan sp)
    : HirStmt(HirKind::LoopStmt, sp),
      body(b) {}

// ------------------------------------------------------------
// Patterns
// ------------------------------------------------------------

HirPattern::HirPattern(HirKind k, vitte::frontend::ast::SourceSpan sp)
    : HirNode(k, sp) {}

HirIdentPattern::HirIdentPattern(std::string n, vitte::frontend::ast::SourceSpan sp)
    : HirPattern(HirKind::PatternIdent, sp), name(std::move(n)) {}

HirCtorPattern::HirCtorPattern(
    std::string n,
    std::vector<HirPatternId> a,
    vitte::frontend::ast::SourceSpan sp)
    : HirPattern(HirKind::PatternCtor, sp),
      name(std::move(n)),
      args(std::move(a)) {}

HirWhen::HirWhen(
    HirPatternId p,
    HirStmtId b,
    vitte::frontend::ast::SourceSpan sp)
    : HirStmt(HirKind::WhenStmt, sp),
      pattern(p),
      block(b) {}

HirSelect::HirSelect(
    HirExprId e,
    std::vector<HirStmtId> w,
    HirStmtId o,
    vitte::frontend::ast::SourceSpan sp)
    : HirStmt(HirKind::SelectStmt, sp),
      expr(e),
      whens(std::move(w)),
      otherwise_block(o) {}

// ------------------------------------------------------------
// Declarations
// ------------------------------------------------------------

HirDecl::HirDecl(HirKind k, vitte::frontend::ast::SourceSpan sp)
    : HirNode(k, sp) {}

HirParam::HirParam(std::string n, HirTypeId t)
    : name(std::move(n)), type(t) {}

HirFnDecl::HirFnDecl(
    std::string n,
    std::vector<HirParam> p,
    HirTypeId ret,
    HirStmtId b,
    vitte::frontend::ast::SourceSpan sp)
    : HirDecl(HirKind::FnDecl, sp),
      name(std::move(n)),
      params(std::move(p)),
      return_type(ret),
      body(b) {}

HirModule::HirModule(
    std::string n,
    std::vector<HirDeclId> d,
    vitte::frontend::ast::SourceSpan sp)
    : HirNode(HirKind::Module, sp),
      name(std::move(n)),
      decls(std::move(d)) {}

// ------------------------------------------------------------
// Debug helpers
// ------------------------------------------------------------

static const char* to_string(HirUnaryOp op) {
    switch (op) {
        case HirUnaryOp::Not: return "not";
        case HirUnaryOp::Neg: return "neg";
        default:
            return "?";
    }
}

static const char* to_string(HirBinaryOp op) {
    switch (op) {
        case HirBinaryOp::Add: return "+";
        case HirBinaryOp::Sub: return "-";
        case HirBinaryOp::Mul: return "*";
        case HirBinaryOp::Div: return "/";
        case HirBinaryOp::Eq: return "==";
        case HirBinaryOp::Ne: return "!=";
        case HirBinaryOp::Lt: return "<";
        case HirBinaryOp::Le: return "<=";
        case HirBinaryOp::Gt: return ">";
        case HirBinaryOp::Ge: return ">=";
        case HirBinaryOp::And: return "and";
        case HirBinaryOp::Or: return "or";
        default:
            return "?";
    }
}

static void indent(std::ostream& os, std::size_t depth) {
    for (std::size_t i = 0; i < depth; ++i) {
        os << "  ";
    }
}

void dump(const HirContext& ctx, HirId id, std::ostream& os, std::size_t depth) {
    if (id == kInvalidHirId) {
        indent(os, depth);
        os << "<invalid>";
        return;
    }

    const HirNode& n = ctx.node(id);
    indent(os, depth);
    os << to_string(n.kind);

    if (n.kind == HirKind::VarExpr) {
        auto& v = static_cast<const HirVarExpr&>(n);
        os << " " << v.name;
    } else if (n.kind == HirKind::LiteralExpr) {
        auto& l = static_cast<const HirLiteralExpr&>(n);
        os << " " << l.value;
    } else if (n.kind == HirKind::UnaryExpr) {
        auto& u = static_cast<const HirUnaryExpr&>(n);
        os << " " << to_string(u.op);
    } else if (n.kind == HirKind::BinaryExpr) {
        auto& b = static_cast<const HirBinaryExpr&>(n);
        os << " " << to_string(b.op);
    } else if (n.kind == HirKind::PatternIdent) {
        auto& p = static_cast<const HirIdentPattern&>(n);
        os << " " << p.name;
    } else if (n.kind == HirKind::PatternCtor) {
        auto& p = static_cast<const HirCtorPattern&>(n);
        os << " " << p.name;
    }

    os << "\n";

    switch (n.kind) {
        case HirKind::Module: {
            auto& m = static_cast<const HirModule&>(n);
            for (auto d : m.decls) {
                dump(ctx, d, os, depth + 1);
            }
            break;
        }
        case HirKind::FnDecl: {
            auto& f = static_cast<const HirFnDecl&>(n);
            dump(ctx, f.body, os, depth + 1);
            break;
        }
        case HirKind::Block: {
            auto& b = static_cast<const HirBlock&>(n);
            for (auto s : b.stmts) {
                dump(ctx, s, os, depth + 1);
            }
            break;
        }
        case HirKind::IfStmt: {
            auto& i = static_cast<const HirIf&>(n);
            dump(ctx, i.cond, os, depth + 1);
            dump(ctx, i.then_block, os, depth + 1);
            if (i.else_block != kInvalidHirId) {
                dump(ctx, i.else_block, os, depth + 1);
            }
            break;
        }
        case HirKind::LoopStmt: {
            auto& l = static_cast<const HirLoop&>(n);
            dump(ctx, l.body, os, depth + 1);
            break;
        }
        case HirKind::SelectStmt: {
            auto& s = static_cast<const HirSelect&>(n);
            dump(ctx, s.expr, os, depth + 1);
            for (auto w : s.whens) {
                dump(ctx, w, os, depth + 1);
            }
            if (s.otherwise_block != kInvalidHirId) {
                dump(ctx, s.otherwise_block, os, depth + 1);
            }
            break;
        }
        case HirKind::WhenStmt: {
            auto& w = static_cast<const HirWhen&>(n);
            dump(ctx, w.pattern, os, depth + 1);
            dump(ctx, w.block, os, depth + 1);
            break;
        }
        case HirKind::LetStmt: {
            auto& s = static_cast<const HirLetStmt&>(n);
            if (s.init != kInvalidHirId) {
                dump(ctx, s.init, os, depth + 1);
            }
            break;
        }
        case HirKind::ExprStmt: {
            auto& s = static_cast<const HirExprStmt&>(n);
            dump(ctx, s.expr, os, depth + 1);
            break;
        }
        case HirKind::ReturnStmt: {
            auto& s = static_cast<const HirReturnStmt&>(n);
            if (s.expr != kInvalidHirId) {
                dump(ctx, s.expr, os, depth + 1);
            }
            break;
        }
        case HirKind::UnaryExpr: {
            auto& u = static_cast<const HirUnaryExpr&>(n);
            dump(ctx, u.expr, os, depth + 1);
            break;
        }
        case HirKind::BinaryExpr: {
            auto& b = static_cast<const HirBinaryExpr&>(n);
            dump(ctx, b.lhs, os, depth + 1);
            dump(ctx, b.rhs, os, depth + 1);
            break;
        }
        case HirKind::CallExpr: {
            auto& c = static_cast<const HirCallExpr&>(n);
            dump(ctx, c.callee, os, depth + 1);
            for (auto a : c.args) {
                dump(ctx, a, os, depth + 1);
            }
            break;
        }
        case HirKind::PatternCtor: {
            auto& p = static_cast<const HirCtorPattern&>(n);
            for (auto a : p.args) {
                dump(ctx, a, os, depth + 1);
            }
            break;
        }
        default:
            break;
    }
}

std::string dump_to_string(const HirContext& ctx, HirId node) {
    std::ostringstream os;
    dump(ctx, node, os, 0);
    return os.str();
}

static void dump_compact_impl(const HirContext& ctx, HirId id, std::ostream& os) {
    if (id == kInvalidHirId) {
        os << "<invalid>";
        return;
    }

    const HirNode& n = ctx.node(id);
    os << to_string(n.kind);

    if (n.kind == HirKind::VarExpr) {
        auto& v = static_cast<const HirVarExpr&>(n);
        os << "(" << v.name << ")";
    } else if (n.kind == HirKind::LiteralExpr) {
        auto& l = static_cast<const HirLiteralExpr&>(n);
        os << "(" << l.value << ")";
    } else if (n.kind == HirKind::UnaryExpr) {
        auto& u = static_cast<const HirUnaryExpr&>(n);
        os << "(" << to_string(u.op) << ")";
    } else if (n.kind == HirKind::BinaryExpr) {
        auto& b = static_cast<const HirBinaryExpr&>(n);
        os << "(" << to_string(b.op) << ")";
    } else if (n.kind == HirKind::PatternIdent) {
        auto& p = static_cast<const HirIdentPattern&>(n);
        os << "(" << p.name << ")";
    } else if (n.kind == HirKind::PatternCtor) {
        auto& p = static_cast<const HirCtorPattern&>(n);
        os << "(" << p.name << ")";
    }

    std::vector<HirId> children;
    switch (n.kind) {
        case HirKind::Module: {
            auto& m = static_cast<const HirModule&>(n);
            children.insert(children.end(), m.decls.begin(), m.decls.end());
            break;
        }
        case HirKind::FnDecl: {
            auto& f = static_cast<const HirFnDecl&>(n);
            children.push_back(f.body);
            break;
        }
        case HirKind::Block: {
            auto& b = static_cast<const HirBlock&>(n);
            children.insert(children.end(), b.stmts.begin(), b.stmts.end());
            break;
        }
        case HirKind::IfStmt: {
            auto& i = static_cast<const HirIf&>(n);
            children.push_back(i.cond);
            children.push_back(i.then_block);
            if (i.else_block != kInvalidHirId) {
                children.push_back(i.else_block);
            }
            break;
        }
        case HirKind::LoopStmt: {
            auto& l = static_cast<const HirLoop&>(n);
            children.push_back(l.body);
            break;
        }
        case HirKind::SelectStmt: {
            auto& s = static_cast<const HirSelect&>(n);
            children.push_back(s.expr);
            children.insert(children.end(), s.whens.begin(), s.whens.end());
            if (s.otherwise_block != kInvalidHirId) {
                children.push_back(s.otherwise_block);
            }
            break;
        }
        case HirKind::WhenStmt: {
            auto& w = static_cast<const HirWhen&>(n);
            children.push_back(w.pattern);
            children.push_back(w.block);
            break;
        }
        case HirKind::LetStmt: {
            auto& s = static_cast<const HirLetStmt&>(n);
            if (s.init != kInvalidHirId) {
                children.push_back(s.init);
            }
            break;
        }
        case HirKind::ExprStmt: {
            auto& s = static_cast<const HirExprStmt&>(n);
            children.push_back(s.expr);
            break;
        }
        case HirKind::ReturnStmt: {
            auto& s = static_cast<const HirReturnStmt&>(n);
            if (s.expr != kInvalidHirId) {
                children.push_back(s.expr);
            }
            break;
        }
        case HirKind::UnaryExpr: {
            auto& u = static_cast<const HirUnaryExpr&>(n);
            children.push_back(u.expr);
            break;
        }
        case HirKind::BinaryExpr: {
            auto& b = static_cast<const HirBinaryExpr&>(n);
            children.push_back(b.lhs);
            children.push_back(b.rhs);
            break;
        }
        case HirKind::CallExpr: {
            auto& c = static_cast<const HirCallExpr&>(n);
            children.push_back(c.callee);
            children.insert(children.end(), c.args.begin(), c.args.end());
            break;
        }
        case HirKind::PatternCtor: {
            auto& p = static_cast<const HirCtorPattern&>(n);
            children.insert(children.end(), p.args.begin(), p.args.end());
            break;
        }
        default:
            break;
    }

    if (!children.empty()) {
        os << " {";
        for (std::size_t i = 0; i < children.size(); ++i) {
            if (i != 0) {
                os << " ";
            }
            dump_compact_impl(ctx, children[i], os);
        }
        os << "}";
    }
}

void dump_compact(const HirContext& ctx, HirId node, std::ostream& os) {
    dump_compact_impl(ctx, node, os);
}

std::string dump_compact_to_string(const HirContext& ctx, HirId node) {
    std::ostringstream os;
    dump_compact(ctx, node, os);
    return os.str();
}

static void dump_json_impl(const HirContext& ctx, HirId id, std::ostream& os) {
    if (id == kInvalidHirId) {
        os << "{\"kind\":\"Invalid\"}";
        return;
    }

    const HirNode& n = ctx.node(id);

    os << "{\"kind\":\"" << to_string(n.kind) << "\"";

    if (n.kind == HirKind::VarExpr) {
        auto& v = static_cast<const HirVarExpr&>(n);
        os << ",\"name\":\"" << v.name << "\"";
    } else if (n.kind == HirKind::LiteralExpr) {
        auto& l = static_cast<const HirLiteralExpr&>(n);
        os << ",\"value\":\"" << l.value << "\"";
    } else if (n.kind == HirKind::UnaryExpr) {
        auto& u = static_cast<const HirUnaryExpr&>(n);
        os << ",\"op\":\"" << to_string(u.op) << "\"";
    } else if (n.kind == HirKind::BinaryExpr) {
        auto& b = static_cast<const HirBinaryExpr&>(n);
        os << ",\"op\":\"" << to_string(b.op) << "\"";
    } else if (n.kind == HirKind::PatternIdent) {
        auto& p = static_cast<const HirIdentPattern&>(n);
        os << ",\"name\":\"" << p.name << "\"";
    } else if (n.kind == HirKind::PatternCtor) {
        auto& p = static_cast<const HirCtorPattern&>(n);
        os << ",\"name\":\"" << p.name << "\"";
    }

    std::vector<HirId> children;
    switch (n.kind) {
        case HirKind::Module: {
            auto& m = static_cast<const HirModule&>(n);
            children.insert(children.end(), m.decls.begin(), m.decls.end());
            break;
        }
        case HirKind::FnDecl: {
            auto& f = static_cast<const HirFnDecl&>(n);
            children.push_back(f.body);
            break;
        }
        case HirKind::Block: {
            auto& b = static_cast<const HirBlock&>(n);
            children.insert(children.end(), b.stmts.begin(), b.stmts.end());
            break;
        }
        case HirKind::IfStmt: {
            auto& i = static_cast<const HirIf&>(n);
            children.push_back(i.cond);
            children.push_back(i.then_block);
            if (i.else_block != kInvalidHirId) {
                children.push_back(i.else_block);
            }
            break;
        }
        case HirKind::SelectStmt: {
            auto& s = static_cast<const HirSelect&>(n);
            children.push_back(s.expr);
            children.insert(children.end(), s.whens.begin(), s.whens.end());
            if (s.otherwise_block != kInvalidHirId) {
                children.push_back(s.otherwise_block);
            }
            break;
        }
        case HirKind::WhenStmt: {
            auto& w = static_cast<const HirWhen&>(n);
            children.push_back(w.pattern);
            children.push_back(w.block);
            break;
        }
        case HirKind::LetStmt: {
            auto& s = static_cast<const HirLetStmt&>(n);
            if (s.init != kInvalidHirId) {
                children.push_back(s.init);
            }
            break;
        }
        case HirKind::ExprStmt: {
            auto& s = static_cast<const HirExprStmt&>(n);
            children.push_back(s.expr);
            break;
        }
        case HirKind::ReturnStmt: {
            auto& s = static_cast<const HirReturnStmt&>(n);
            if (s.expr != kInvalidHirId) {
                children.push_back(s.expr);
            }
            break;
        }
        case HirKind::UnaryExpr: {
            auto& u = static_cast<const HirUnaryExpr&>(n);
            children.push_back(u.expr);
            break;
        }
        case HirKind::BinaryExpr: {
            auto& b = static_cast<const HirBinaryExpr&>(n);
            children.push_back(b.lhs);
            children.push_back(b.rhs);
            break;
        }
        case HirKind::CallExpr: {
            auto& c = static_cast<const HirCallExpr&>(n);
            children.push_back(c.callee);
            children.insert(children.end(), c.args.begin(), c.args.end());
            break;
        }
        case HirKind::PatternCtor: {
            auto& p = static_cast<const HirCtorPattern&>(n);
            children.insert(children.end(), p.args.begin(), p.args.end());
            break;
        }
        default:
            break;
    }

    if (!children.empty()) {
        os << ",\"children\":[";
        for (std::size_t i = 0; i < children.size(); ++i) {
            if (i != 0) {
                os << ",";
            }
            dump_json_impl(ctx, children[i], os);
        }
        os << "]";
    }

    os << "}";
}

void dump_json(const HirContext& ctx, HirId node, std::ostream& os) {
    dump_json_impl(ctx, node, os);
}

std::string dump_json_to_string(const HirContext& ctx, HirId node) {
    std::ostringstream os;
    dump_json(ctx, node, os);
    return os.str();
}

const char* to_string(HirKind kind) {
    switch (kind) {
        case HirKind::NamedType: return "NamedType";
        case HirKind::GenericType: return "GenericType";
        case HirKind::LiteralExpr: return "LiteralExpr";
        case HirKind::VarExpr: return "VarExpr";
        case HirKind::UnaryExpr: return "UnaryExpr";
        case HirKind::BinaryExpr: return "BinaryExpr";
        case HirKind::CallExpr: return "CallExpr";
        case HirKind::LetStmt: return "LetStmt";
        case HirKind::ExprStmt: return "ExprStmt";
        case HirKind::ReturnStmt: return "ReturnStmt";
        case HirKind::Block: return "Block";
        case HirKind::IfStmt: return "IfStmt";
        case HirKind::LoopStmt: return "LoopStmt";
        case HirKind::SelectStmt: return "SelectStmt";
        case HirKind::WhenStmt: return "WhenStmt";
        case HirKind::FnDecl: return "FnDecl";
        case HirKind::Module: return "Module";
        case HirKind::PatternIdent: return "PatternIdent";
        case HirKind::PatternCtor: return "PatternCtor";
        default:
            return "Unknown";
    }
}

} // namespace vitte::ir
