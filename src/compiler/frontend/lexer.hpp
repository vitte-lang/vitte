#pragma once
#include <cstddef>
#include <memory>
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
    BytesLit,

    // keywords
    KwSpace,
    KwFrom,
    KwImport,
    KwPull,
    KwUse,
    KwExport,
    KwBench,
    KwAsync,
    KwForeign,
    KwImpl,
    KwShare,
    KwBlock,
    KwBorrow,
    KwGlobal,
    KwForm,
    KwClass,
    KwUnion,
    KwDef,
    KwField,
    KwPick,
    KwCase,
    KwTrait,
    KwInline,
    KwInternal,
    KwInterrupt,
    KwMove,
    KwNaked,
    KwNoinline,
    KwNoexcept,
    KwType,
    KwOpaque,
    KwConst,
    KwMacro,
    KwComptime,
    KwProc,
    KwPackage,
    KwEntry,
    KwAt,
    KwAsm,
    KwUnsafe,
    KwMatch,
    KwTry,
    KwExcept,
    KwFinally,
    KwRaise,
    KwDefer,
    KwExtern,
    KwLet,
    KwMut,
    KwMake,
    KwSet,
    KwGive,
    KwEmit,
    KwWith,
    KwCritical,
    KwAtomic,
    KwVolatile,
    KwWhere,
    KwRequires,
    KwStatic,
    KwStaticAssert,
    KwIf,
    KwElif,
    KwElse,
    KwOtherwise,
    KwSelect,
    KwWhen,
    KwIs,
    KwLoop,
    KwWhile,
    KwFor,
    KwIn,
    KwBreak,
    KwContinue,
    KwGoto,
    KwPreempt,
    KwIrq,
    KwAwait,
    KwOn,
    KwOff,
    KwEnable,
    KwDisable,
    KwReturn,
    KwAssert,
    KwPanic,
    KwUnreachable,
    KwNot,
    KwAnd,
    KwOr,
    KwAs,
    KwAll,
    KwTrue,
    KwFalse,
    KwPub,
    KwPriv,
    KwSelf,
    KwSuper,
    KwRef,
    KwOwned,
    KwNull,
    KwTest,
    KwUser,
    KwKernel,
    KwPhys,
    KwMmio,
    KwDma,
    KwDyn,
    KwEffects,
    KwFlags,
    KwMap,
    KwResource,
    KwRune,
    KwStr,
    KwUnit,
    KwCChar,
    KwCInt,
    KwCUInt,
    KwCLong,
    KwCULong,
    KwCVoid,
    KwCStr,
    KwI8,
    KwI16,
    KwU8,
    KwU16,
    KwIsize,
    KwUsize,
    KwIntptr,
    KwUintptr,
    KwF16,
    KwF32,
    KwF64,
    KwF128,
    KwSysv64,
    KwWin64,
    KwBool,
    KwString,
    KwInt,
    KwBytes,
    KwChar,
    KwVoid,
    KwNever,
    KwSizeof,
    KwAlignof,
    KwOffsetof,
    KwTypeof,
    KwNameof,

    // symbols
    AttrStart, // #[
    AtSign,    // @
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Comma,
    Colon,
    Semicolon,
    Dot,
    Ellipsis,
    Slash,
    Plus,
    PlusEqual,
    Minus,
    MinusEqual,
    Star,
    StarEqual,
    Percent,
    PercentEqual,
    SlashEqual,
    Question,
    Amp,
    Pipe,
    Caret,
    Tilde,
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
    struct State {
        std::size_t index = 0;
    };

    explicit Lexer(const std::string& source, std::string path = "<input>");
    const std::shared_ptr<ast::SourceFile>& source_file() const { return source_file_; }

    Token next();
    State snapshot() const;
    void restore(State state);

private:
    const std::string& source_;
    std::shared_ptr<ast::SourceFile> source_file_;
    std::size_t index_ = 0;
};

/* -------------------------------------------------
 * Lexer entry point
 * ------------------------------------------------- */
std::vector<Token> lex(const std::string& source);

} // namespace vitte::frontend
