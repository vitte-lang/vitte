#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "ast.hpp"

namespace vitte::frontend {

/* -------------------------------------------------
 * Token kinds
 * ------------------------------------------------- */
enum class TokenKind {
    Eof,

    Ident,
    IntLit,
    StringLit,

    // keywords
    KwSpace,
    KwPull,
    KwShare,
    KwForm,
    KwField,
    KwPick,
    KwCase,
    KwProc,
    KwEntry,
    KwAt,
    KwMake,
    KwSet,
    KwGive,
    KwEmit,
    KwIf,
    KwOtherwise,
    KwSelect,
    KwWhen,
    KwReturn,
    KwNot,
    KwAnd,
    KwOr,
    KwAs,
    KwAll,
    KwTrue,
    KwFalse,
    KwBool,
    KwString,
    KwInt,

    // symbols
    AttrStart, // #[
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Comma,
    Colon,
    Dot,
    Slash,
    Plus,
    Minus,
    Star,
    Equal,
    EqEq,
    NotEq,
    Lt,
    Le,
    Gt,
    Ge,
    EndMarker
};

/* -------------------------------------------------
 * Token
 * ------------------------------------------------- */
struct Token {
    TokenKind kind;
    std::string text;
    ast::SourceSpan span;
};

/* -------------------------------------------------
 * Lexer
 * ------------------------------------------------- */
class Lexer {
public:
    explicit Lexer(const std::string& source);

    Token next();

private:
    const std::string& source_;
    std::size_t index_ = 0;
};

/* -------------------------------------------------
 * Lexer entry point
 * ------------------------------------------------- */
std::vector<Token> lex(const std::string& source);

} // namespace vitte::frontend
