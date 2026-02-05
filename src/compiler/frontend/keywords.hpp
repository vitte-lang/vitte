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
        case TokenKind::KwBool:
        case TokenKind::KwString:
        case TokenKind::KwInt:
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
        || kind == TokenKind::KwInt;
}

} // namespace vitte::frontend
