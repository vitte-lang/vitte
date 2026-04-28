// ============================================================
// token_tables.cpp — Vitte Compiler
// Centralized keyword/operator tables for lexer+parser+driver
// ============================================================

#include "token_tables.hpp"

#include <algorithm>
#include <unordered_map>

namespace vitte::frontend::tokens {

static constexpr KeywordSpec kKeywords[] = {
    {"space", TokenKind::KwSpace, true},
    {"from", TokenKind::KwFrom, false},
    {"import", TokenKind::KwImport, false},
    {"use", TokenKind::KwUse, true},
    {"export", TokenKind::KwExport, true},
    {"async", TokenKind::KwAsync, true},
    {"foreign", TokenKind::KwForeign, true},
    {"impl", TokenKind::KwImpl, false},
    {"global", TokenKind::KwGlobal, true},
    {"form", TokenKind::KwForm, true},
    {"class", TokenKind::KwClass, false},
    {"pick", TokenKind::KwPick, true},
    {"case", TokenKind::KwCase, true},
    {"trait", TokenKind::KwTrait, false},
    {"union", TokenKind::KwUnion, false},
    {"type", TokenKind::KwType, true},
    {"const", TokenKind::KwConst, true},
    {"macro", TokenKind::KwMacro, false},
    {"comptime", TokenKind::KwComptime, true},
    {"proc", TokenKind::KwProc, true},
    {"entry", TokenKind::KwEntry, true},
    {"at", TokenKind::KwAt, true},
    {"asm", TokenKind::KwAsm, true},
    {"unsafe", TokenKind::KwUnsafe, true},
    {"extern", TokenKind::KwExtern, true},
    {"match", TokenKind::KwMatch, true},
    {"try", TokenKind::KwTry, true},
    {"except", TokenKind::KwExcept, true},
    {"finally", TokenKind::KwFinally, true},
    {"let", TokenKind::KwLet, true},
    {"mut", TokenKind::KwMut, true},
    {"make", TokenKind::KwMake, true},
    {"set", TokenKind::KwSet, true},
    {"give", TokenKind::KwGive, true},
    {"emit", TokenKind::KwEmit, true},
    {"defer", TokenKind::KwDefer, true},
    {"with", TokenKind::KwWith, true},
    {"critical", TokenKind::KwCritical, true},
    {"atomic", TokenKind::KwAtomic, true},
    {"volatile", TokenKind::KwVolatile, true},
    {"where", TokenKind::KwWhere, true},
    {"requires", TokenKind::KwRequires, true},
    {"if", TokenKind::KwIf, true},
    {"elif", TokenKind::KwElif, true},
    {"else", TokenKind::KwElse, true},
    {"select", TokenKind::KwSelect, false},
    {"when", TokenKind::KwWhen, true},
    {"is", TokenKind::KwIs, true},
    {"loop", TokenKind::KwLoop, true},
    {"while", TokenKind::KwWhile, true},
    {"for", TokenKind::KwFor, true},
    {"in", TokenKind::KwIn, true},
    {"break", TokenKind::KwBreak, true},
    {"continue", TokenKind::KwContinue, true},
    {"goto", TokenKind::KwGoto, true},
    {"preempt", TokenKind::KwPreempt, true},
    {"irq", TokenKind::KwIrq, true},
    {"await", TokenKind::KwAwait, true},
    {"on", TokenKind::KwOn, true},
    {"off", TokenKind::KwOff, true},
    {"enable", TokenKind::KwEnable, true},
    {"disable", TokenKind::KwDisable, true},
    {"return", TokenKind::KwReturn, true},
    {"assert", TokenKind::KwAssert, true},
    {"panic", TokenKind::KwPanic, true},
    {"unreachable", TokenKind::KwUnreachable, true},
    {"not", TokenKind::KwNot, true},
    {"and", TokenKind::KwAnd, true},
    {"or", TokenKind::KwOr, true},
    {"as", TokenKind::KwAs, true},
    {"true", TokenKind::KwTrue, true},
    {"false", TokenKind::KwFalse, true},
    {"bool", TokenKind::KwBool, true},
    {"string", TokenKind::KwString, true},
    {"int", TokenKind::KwInt, true},
    {"bytes", TokenKind::KwBytes, true},
    {"char", TokenKind::KwChar, true},
    {"void", TokenKind::KwVoid, true},
    {"never", TokenKind::KwNever, true},
    {"bench", TokenKind::KwBench, false},
    {"block", TokenKind::KwBlock, false},
    {"borrow", TokenKind::KwBorrow, true},
    {"inline", TokenKind::KwInline, true},
    {"internal", TokenKind::KwInternal, true},
    {"interrupt", TokenKind::KwInterrupt, true},
    {"move", TokenKind::KwMove, true},
    {"naked", TokenKind::KwNaked, true},
    {"noinline", TokenKind::KwNoinline, true},
    {"noexcept", TokenKind::KwNoexcept, true},
    {"opaque", TokenKind::KwOpaque, false},
    {"package", TokenKind::KwPackage, false},
    {"static", TokenKind::KwStatic, false},
    {"static_assert", TokenKind::KwStaticAssert, false},
    {"pub", TokenKind::KwPub, true},
    {"priv", TokenKind::KwPriv, true},
    {"self", TokenKind::KwSelf, true},
    {"super", TokenKind::KwSuper, true},
    {"ref", TokenKind::KwRef, true},
    {"owned", TokenKind::KwOwned, true},
    {"null", TokenKind::KwNull, true},
    {"test", TokenKind::KwTest, true},
    {"user", TokenKind::KwUser, true},
    {"kernel", TokenKind::KwKernel, true},
    {"phys", TokenKind::KwPhys, true},
    {"mmio", TokenKind::KwMmio, true},
    {"dma", TokenKind::KwDma, true},
    {"dyn", TokenKind::KwDyn, true},
    {"effects", TokenKind::KwEffects, false},
    {"flags", TokenKind::KwFlags, false},
    {"map", TokenKind::KwMap, true},
    {"resource", TokenKind::KwResource, true},
    {"rune", TokenKind::KwRune, true},
    {"str", TokenKind::KwStr, true},
    {"unit", TokenKind::KwUnit, true},
    {"c_char", TokenKind::KwCChar, true},
    {"c_int", TokenKind::KwCInt, true},
    {"c_uint", TokenKind::KwCUInt, true},
    {"c_long", TokenKind::KwCLong, true},
    {"c_ulong", TokenKind::KwCULong, true},
    {"c_void", TokenKind::KwCVoid, true},
    {"cstr", TokenKind::KwCStr, true},
    {"i8", TokenKind::KwI8, true},
    {"i16", TokenKind::KwI16, true},
    {"u8", TokenKind::KwU8, true},
    {"u16", TokenKind::KwU16, true},
    {"isize", TokenKind::KwIsize, true},
    {"usize", TokenKind::KwUsize, true},
    {"intptr", TokenKind::KwIntptr, true},
    {"uintptr", TokenKind::KwUintptr, true},
    {"f16", TokenKind::KwF16, true},
    {"f32", TokenKind::KwF32, true},
    {"f64", TokenKind::KwF64, true},
    {"f128", TokenKind::KwF128, true},
    {"sysv64", TokenKind::KwSysv64, true},
    {"win64", TokenKind::KwWin64, true},
    {"sizeof", TokenKind::KwSizeof, true},
    {"alignof", TokenKind::KwAlignof, true},
    {"offsetof", TokenKind::KwOffsetof, true},
    {"typeof", TokenKind::KwTypeof, true},
    {"nameof", TokenKind::KwNameof, true},
};

static constexpr BinaryOpSpec kBinaryOps[] = {
#define VITTE_PRECEDENCE_OP(token, lexeme, op, prec, core) {token, lexeme, op, prec, core},
#include "precedence_table.def"
#undef VITTE_PRECEDENCE_OP
};

TokenKind keyword_kind(std::string_view ident) {
    static const std::unordered_map<std::string, TokenKind> table = []() {
        std::unordered_map<std::string, TokenKind> out;
        for (const auto& kw : kKeywords) {
            out.emplace(kw.text, kw.kind);
        }
        return out;
    }();
    const auto it = table.find(std::string(ident));
    if (it == table.end()) {
        return TokenKind::Ident;
    }
    return it->second;
}

const char* keyword_text(TokenKind kind) {
    for (const auto& kw : kKeywords) {
        if (kw.kind == kind) {
            return kw.text;
        }
    }
    return nullptr;
}

bool is_core_keyword(TokenKind kind) {
    for (const auto& kw : kKeywords) {
        if (kw.kind == kind) {
            return kw.core;
        }
    }
    return false;
}

bool is_forbidden_in_core(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwMacro:
        case TokenKind::KwTrait:
        case TokenKind::KwClass:
        case TokenKind::KwDef:
        case TokenKind::KwSelect:
        case TokenKind::KwField:
        case TokenKind::KwPull:
        case TokenKind::KwShare:
        case TokenKind::KwUnsafe:
            return true;
        default:
            return false;
    }
}

std::vector<std::string> core_keywords() {
    std::vector<std::string> out;
    out.reserve(std::size(kKeywords));
    for (const auto& kw : kKeywords) {
        if (kw.core) {
            out.emplace_back(kw.text);
        }
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

std::vector<std::string> all_keywords() {
    std::vector<std::string> out;
    out.reserve(std::size(kKeywords));
    for (const auto& kw : kKeywords) {
        out.emplace_back(kw.text);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

int binary_precedence(TokenKind kind) {
    for (const auto& op : kBinaryOps) {
        if (op.token == kind) {
            return op.precedence;
        }
    }
    return 0;
}

bool is_binary_operator(TokenKind kind) {
    return binary_precedence(kind) > 0;
}

ast::BinaryOp to_binary_op(TokenKind kind) {
    for (const auto& op : kBinaryOps) {
        if (op.token == kind) {
            return op.op;
        }
    }
    return ast::BinaryOp::Add;
}

std::vector<std::string> core_binary_operators() {
    std::vector<std::string> out;
    out.reserve(std::size(kBinaryOps));
    for (const auto& op : kBinaryOps) {
        if (op.core) {
            out.emplace_back(op.lexeme);
        }
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

std::vector<std::string> all_binary_operators() {
    std::vector<std::string> out;
    out.reserve(std::size(kBinaryOps));
    for (const auto& op : kBinaryOps) {
        out.emplace_back(op.lexeme);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

} // namespace vitte::frontend::tokens
