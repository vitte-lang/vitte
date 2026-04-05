// ============================================================
// token_tables.hpp — Vitte Compiler
// Centralized keyword/operator tables for lexer+parser+driver
// ============================================================

#pragma once

#include "ast.hpp"
#include "lexer.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace vitte::frontend::tokens {

struct KeywordSpec {
    const char* text;
    TokenKind kind;
    bool core;
};

struct BinaryOpSpec {
    TokenKind token;
    const char* lexeme;
    ast::BinaryOp op;
    int precedence;
    bool core;
};

TokenKind keyword_kind(std::string_view ident);
const char* keyword_text(TokenKind kind);
bool is_core_keyword(TokenKind kind);
bool is_forbidden_in_core(TokenKind kind);
std::vector<std::string> core_keywords();

int binary_precedence(TokenKind kind);
bool is_binary_operator(TokenKind kind);
ast::BinaryOp to_binary_op(TokenKind kind);
std::vector<std::string> core_binary_operators();

} // namespace vitte::frontend::tokens

