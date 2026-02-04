#pragma once
#include <string>
#include <vector>
#include <cstddef>

namespace vitte::frontend {

/* -------------------------------------------------
 * Source position
 * ------------------------------------------------- */
struct SourcePos {
    std::size_t line = 1;
    std::size_t column = 1;
};

/* -------------------------------------------------
 * Token kinds
 * ------------------------------------------------- */
enum class TokenKind {
    EndOfFile,

    Identifier,
    Number,
    String,

    KwFn,
    KwReturn,

    LParen,     // (
    RParen,     // )
    LBrace,     // {
    RBrace,     // }
    Semicolon,  // ;
    Comma,      // ,

    Plus,       // +
    Minus,      // -
    Star,       // *
    Slash,      // /
    Equal       // =
};

/* -------------------------------------------------
 * Token
 * ------------------------------------------------- */
struct Token {
    TokenKind kind;
    std::string text;
    SourcePos pos;
};

/* -------------------------------------------------
 * Lexer entry point
 * ------------------------------------------------- */
std::vector<Token> lex(const std::string& source);

} // namespace vitte::frontend
