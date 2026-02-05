#include "lexer.hpp"

#include <cctype>
#include <unordered_map>

namespace vitte::frontend {

static ast::SourceSpan make_span(const ast::SourceFile* file, std::size_t start, std::size_t end) {
    return ast::SourceSpan(file, start, end);
}

static Token make_token(
    TokenKind kind,
    std::string text,
    const ast::SourceFile* file,
    std::size_t start,
    std::size_t end)
{
    return Token{kind, std::move(text), make_span(file, start, end)};
}

static bool is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

static bool is_ident_continue(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

static bool is_digit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

static bool is_hex_digit(char c) {
    return std::isxdigit(static_cast<unsigned char>(c)) != 0;
}

static bool is_space(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

static bool is_suffix_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0;
}

static TokenKind keyword_kind(const std::string& ident) {
    static const std::unordered_map<std::string, TokenKind> kKeywords = {
        {"space", TokenKind::KwSpace},
        {"pull", TokenKind::KwPull},
        {"use", TokenKind::KwUse},
        {"share", TokenKind::KwShare},
        {"form", TokenKind::KwForm},
        {"field", TokenKind::KwField},
        {"pick", TokenKind::KwPick},
        {"case", TokenKind::KwCase},
        {"trait", TokenKind::KwTrait},
        {"type", TokenKind::KwType},
        {"const", TokenKind::KwConst},
        {"macro", TokenKind::KwMacro},
        {"proc", TokenKind::KwProc},
        {"entry", TokenKind::KwEntry},
        {"at", TokenKind::KwAt},
        {"let", TokenKind::KwLet},
        {"make", TokenKind::KwMake},
        {"set", TokenKind::KwSet},
        {"give", TokenKind::KwGive},
        {"emit", TokenKind::KwEmit},
        {"if", TokenKind::KwIf},
        {"else", TokenKind::KwElse},
        {"otherwise", TokenKind::KwOtherwise},
        {"select", TokenKind::KwSelect},
        {"when", TokenKind::KwWhen},
        {"is", TokenKind::KwIs},
        {"loop", TokenKind::KwLoop},
        {"for", TokenKind::KwFor},
        {"in", TokenKind::KwIn},
        {"break", TokenKind::KwBreak},
        {"continue", TokenKind::KwContinue},
        {"return", TokenKind::KwReturn},
        {"not", TokenKind::KwNot},
        {"and", TokenKind::KwAnd},
        {"or", TokenKind::KwOr},
        {"as", TokenKind::KwAs},
        {"all", TokenKind::KwAll},
        {"true", TokenKind::KwTrue},
        {"false", TokenKind::KwFalse},
        {"bool", TokenKind::KwBool},
        {"string", TokenKind::KwString},
        {"int", TokenKind::KwInt},
    };

    auto it = kKeywords.find(ident);
    if (it != kKeywords.end()) {
        return it->second;
    }
    return TokenKind::Ident;
}

Lexer::Lexer(const std::string& source, std::string path)
    : source_(source) {
    source_file_.path = std::move(path);
    source_file_.content = source_;
}

Token Lexer::next() {
    auto eof = [&]() {
        return index_ >= source_.size();
    };

    const ast::SourceFile* file = &source_file_;
    auto make = [&](TokenKind kind, std::string text, std::size_t start, std::size_t end) {
        return make_token(kind, std::move(text), file, start, end);
    };

    auto peek = [&](std::size_t offset = 0) {
        std::size_t pos = index_ + offset;
        return pos < source_.size() ? source_[pos] : '\0';
    };

    auto advance = [&]() {
        return eof() ? '\0' : source_[index_++];
    };

    auto skip_zone_comment = [&]() {
        if (peek() == '<' && peek(1) == '<' && peek(2) == '<') {
            index_ += 3;
            while (!eof()) {
                if (peek() == '>' && peek(1) == '>' && peek(2) == '>') {
                    index_ += 3;
                    break;
                }
                advance();
            }
            return true;
        }
        return false;
    };

    while (!eof()) {
        if (skip_zone_comment()) {
            continue;
        }
        if (is_space(peek())) {
            advance();
            continue;
        }
        break;
    }

    if (eof()) {
        return make(TokenKind::Eof, "", index_, index_);
    }

    std::size_t start = index_;
    char c = advance();

    if (is_ident_start(c)) {
        std::string ident(1, c);
        while (!eof() && is_ident_continue(peek())) {
            ident.push_back(advance());
        }
        TokenKind kind = keyword_kind(ident);
        return make(kind, ident, start, index_);
    }

    auto read_number = [&](char first, bool negative) -> Token {
        std::string num;
        if (negative) {
            num.push_back('-');
        }
        num.push_back(first);

        auto read_digits = [&](auto pred) {
            while (!eof() && (pred(peek()) || peek() == '_')) {
                num.push_back(advance());
            }
        };

        if (first == '0' && (peek() == 'x' || peek() == 'X')) {
            num.push_back(advance());
            read_digits(is_hex_digit);
            return make(TokenKind::IntLit, num, start, index_);
        }

        read_digits(is_digit);

        if (peek() == '.' && is_digit(peek(1))) {
            num.push_back(advance());
            read_digits(is_digit);
            return make(TokenKind::FloatLit, num, start, index_);
        }

        while (!eof() && is_suffix_start(peek())) {
            num.push_back(advance());
        }
        return make(TokenKind::IntLit, num, start, index_);
    };

    if (c == '-' && is_digit(peek())) {
        return read_number(advance(), true);
    }

    if (is_digit(c)) {
        return read_number(c, false);
    }

    if (c == '\'') {
        std::string value;
        while (!eof() && peek() != '\'') {
            char ch = advance();
            if (ch == '\\' && !eof()) {
                value.push_back(ch);
                value.push_back(advance());
            } else {
                value.push_back(ch);
            }
        }
        if (!eof()) {
            advance();
        }
        return make(TokenKind::CharLit, value, start, index_);
    }

    if (c == '"') {
        if (peek() == '"' && peek(1) == '"') {
            index_ += 2;
            std::string value;
            while (!eof()) {
                if (peek() == '"' && peek(1) == '"' && peek(2) == '"') {
                    index_ += 3;
                    break;
                }
                value.push_back(advance());
            }
            return make(TokenKind::StringLit, value, start, index_);
        }

        std::string value;
        while (!eof() && peek() != '"') {
            char ch = advance();
            if (ch == '\\' && !eof()) {
                value.push_back(ch);
                value.push_back(advance());
            } else {
                value.push_back(ch);
            }
        }
        if (!eof()) {
            advance();
        }
        return make(TokenKind::StringLit, value, start, index_);
    }

    switch (c) {
        case '#':
            if (peek() == '[') {
                advance();
                return make(TokenKind::AttrStart, "#[", start, index_);
            }
            break;
        case '(':
            return make(TokenKind::LParen, "(", start, index_);
        case ')':
            return make(TokenKind::RParen, ")", start, index_);
        case '{':
            return make(TokenKind::LBrace, "{", start, index_);
        case '}':
            return make(TokenKind::RBrace, "}", start, index_);
        case '[':
            return make(TokenKind::LBracket, "[", start, index_);
        case ']':
            return make(TokenKind::RBracket, "]", start, index_);
        case ',':
            return make(TokenKind::Comma, ",", start, index_);
        case ':':
            return make(TokenKind::Colon, ":", start, index_);
        case '.':
            return make(TokenKind::Dot, ".", start, index_);
        case '/':
            return make(TokenKind::Slash, "/", start, index_);
        case '+':
            return make(TokenKind::Plus, "+", start, index_);
        case '-':
            if (peek() == '>') {
                advance();
                return make(TokenKind::Arrow, "->", start, index_);
            }
            return make(TokenKind::Minus, "-", start, index_);
        case '*':
            return make(TokenKind::Star, "*", start, index_);
        case '%':
            return make(TokenKind::Percent, "%", start, index_);
        case '&':
            if (peek() == '&') {
                advance();
                return make(TokenKind::AmpAmp, "&&", start, index_);
            }
            return make(TokenKind::Amp, "&", start, index_);
        case '|':
            if (peek() == '|') {
                advance();
                return make(TokenKind::PipePipe, "||", start, index_);
            }
            return make(TokenKind::Pipe, "|", start, index_);
        case '^':
            return make(TokenKind::Caret, "^", start, index_);
        case '=':
            if (peek() == '=') {
                advance();
                return make(TokenKind::EqEq, "==", start, index_);
            }
            return make(TokenKind::Equal, "=", start, index_);
        case '!':
            if (peek() == '=') {
                advance();
                return make(TokenKind::NotEq, "!=", start, index_);
            }
            return make(TokenKind::Bang, "!", start, index_);
        case '<':
            if (peek() == '=') {
                advance();
                return make(TokenKind::Le, "<=", start, index_);
            }
            if (peek() == '<') {
                advance();
                return make(TokenKind::Shl, "<<", start, index_);
            }
            return make(TokenKind::Lt, "<", start, index_);
        case '>':
            if (peek() == '=') {
                advance();
                return make(TokenKind::Ge, ">=", start, index_);
            }
            if (peek() == '>') {
                advance();
                return make(TokenKind::Shr, ">>", start, index_);
            }
            return make(TokenKind::Gt, ">", start, index_);
        default:
            break;
    }

    return make(TokenKind::Eof, "", index_, index_);
}

std::vector<Token> lex(const std::string& source) {
    Lexer lexer(source, "<input>");
    std::vector<Token> tokens;
    while (true) {
        Token tok = lexer.next();
        tokens.push_back(tok);
        if (tok.kind == TokenKind::Eof) {
            break;
        }
    }
    return tokens;
}

} // namespace vitte::frontend
