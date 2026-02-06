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
    FloatLit,
    CharLit,
    StringLit,

    // keywords
    KwSpace,
    KwPull,
    KwUse,
    KwShare,
    KwForm,
    KwField,
    KwPick,
    KwCase,
    KwTrait,
    KwType,
    KwConst,
    KwMacro,
    KwProc,
    KwEntry,
    KwAt,
    KwAsm,
    KwUnsafe,
    KwMatch,
    KwLet,
    KwMake,
    KwSet,
    KwGive,
    KwEmit,
    KwIf,
    KwElse,
    KwOtherwise,
    KwSelect,
    KwWhen,
    KwIs,
    KwLoop,
    KwFor,
    KwIn,
    KwBreak,
    KwContinue,
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
    Percent,
    Amp,
    Pipe,
    Caret,
    Bang,
    AmpAmp,
    PipePipe,
    Shl,
    Shr,
    Equal,
    EqEq,
    NotEq,
    Arrow,
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
    explicit Lexer(const std::string& source, std::string path = "<input>");

    Token next();

private:
    const std::string& source_;
    ast::SourceFile source_file_;
    std::size_t index_ = 0;
};

/* -------------------------------------------------
 * Lexer entry point
 * ------------------------------------------------- */
std::vector<Token> lex(const std::string& source);

} // namespace vitte::frontend
