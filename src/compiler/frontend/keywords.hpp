// ============================================================
// keywords.hpp — Vitte Compiler
// Keyword policy helpers
// ============================================================

#pragma once

#include "lexer.hpp"

namespace vitte::frontend {

enum class KeywordPolicy {
    Strict,
    Permissive,
};

inline bool is_identifier_keyword(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwAnd:
        case TokenKind::KwOr:
        case TokenKind::KwNot:
        case TokenKind::KwAll:
        case TokenKind::KwDefer:
        case TokenKind::KwWith:
        case TokenKind::KwCritical:
        case TokenKind::KwAtomic:
        case TokenKind::KwVolatile:
        case TokenKind::KwGoto:
        case TokenKind::KwPreempt:
        case TokenKind::KwIrq:
        case TokenKind::KwOn:
        case TokenKind::KwOff:
        case TokenKind::KwEnable:
        case TokenKind::KwDisable:
        case TokenKind::KwBool:
        case TokenKind::KwString:
        case TokenKind::KwInt:
        case TokenKind::KwBytes:
        case TokenKind::KwChar:
        case TokenKind::KwVoid:
        case TokenKind::KwNever:
        case TokenKind::KwBench:
        case TokenKind::KwBlock:
        case TokenKind::KwBorrow:
        case TokenKind::KwInline:
        case TokenKind::KwInternal:
        case TokenKind::KwInterrupt:
        case TokenKind::KwMove:
        case TokenKind::KwNaked:
        case TokenKind::KwNoinline:
        case TokenKind::KwNoexcept:
        case TokenKind::KwOpaque:
        case TokenKind::KwPackage:
        case TokenKind::KwStatic:
        case TokenKind::KwStaticAssert:
        case TokenKind::KwPub:
        case TokenKind::KwPriv:
        case TokenKind::KwSelf:
        case TokenKind::KwSuper:
        case TokenKind::KwRef:
        case TokenKind::KwOwned:
        case TokenKind::KwNull:
        case TokenKind::KwTest:
        case TokenKind::KwUser:
        case TokenKind::KwKernel:
        case TokenKind::KwPhys:
        case TokenKind::KwMmio:
        case TokenKind::KwDma:
        case TokenKind::KwDyn:
        case TokenKind::KwEffects:
        case TokenKind::KwFlags:
        case TokenKind::KwMap:
        case TokenKind::KwResource:
        case TokenKind::KwRune:
        case TokenKind::KwStr:
        case TokenKind::KwUnit:
        case TokenKind::KwCChar:
        case TokenKind::KwCInt:
        case TokenKind::KwCUInt:
        case TokenKind::KwCLong:
        case TokenKind::KwCULong:
        case TokenKind::KwCVoid:
        case TokenKind::KwCStr:
        case TokenKind::KwI8:
        case TokenKind::KwI16:
        case TokenKind::KwU8:
        case TokenKind::KwU16:
        case TokenKind::KwIsize:
        case TokenKind::KwUsize:
        case TokenKind::KwIntptr:
        case TokenKind::KwUintptr:
        case TokenKind::KwF16:
        case TokenKind::KwF32:
        case TokenKind::KwF64:
        case TokenKind::KwF128:
        case TokenKind::KwSysv64:
        case TokenKind::KwWin64:
        case TokenKind::KwAsync:
        case TokenKind::KwForeign:
        case TokenKind::KwImpl:
        case TokenKind::KwUnion:
        case TokenKind::KwComptime:
        case TokenKind::KwExtern:
        case TokenKind::KwWhere:
        case TokenKind::KwRequires:
        case TokenKind::KwAwait:
        case TokenKind::KwSizeof:
        case TokenKind::KwAlignof:
        case TokenKind::KwOffsetof:
        case TokenKind::KwTypeof:
        case TokenKind::KwNameof:
            return true;
        default:
            return false;
    }
}

inline bool is_identifier_token(TokenKind kind, KeywordPolicy policy) {
    if (kind == TokenKind::Ident) {
        return true;
    }
    return policy == KeywordPolicy::Permissive && is_identifier_keyword(kind);
}

inline bool is_type_keyword(TokenKind kind) {
    return kind == TokenKind::KwBool
        || kind == TokenKind::KwString
        || kind == TokenKind::KwInt
        || kind == TokenKind::KwBytes
        || kind == TokenKind::KwChar
        || kind == TokenKind::KwVoid
        || kind == TokenKind::KwNever
        || kind == TokenKind::KwCChar
        || kind == TokenKind::KwCInt
        || kind == TokenKind::KwCUInt
        || kind == TokenKind::KwCLong
        || kind == TokenKind::KwCULong
        || kind == TokenKind::KwCVoid
        || kind == TokenKind::KwCStr
        || kind == TokenKind::KwI8
        || kind == TokenKind::KwI16
        || kind == TokenKind::KwU8
        || kind == TokenKind::KwU16
        || kind == TokenKind::KwIsize
        || kind == TokenKind::KwUsize
        || kind == TokenKind::KwIntptr
        || kind == TokenKind::KwUintptr
        || kind == TokenKind::KwF16
        || kind == TokenKind::KwF32
        || kind == TokenKind::KwF64
        || kind == TokenKind::KwF128
        || kind == TokenKind::KwSysv64
        || kind == TokenKind::KwWin64
        || kind == TokenKind::KwStr
        || kind == TokenKind::KwUnit
        || kind == TokenKind::KwRune;
}

} // namespace vitte::frontend
