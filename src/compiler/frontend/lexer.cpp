#include "lexer.hpp"

#include <cctype>
#include <unordered_map>

namespace vitte::frontend {

static ast::SourceSpan make_span(std::size_t start, std::size_t end) {
    return ast::SourceSpan(nullptr, start, end);
}

static Token make_token(
    TokenKind kind,
    std::string text,
    std::size_t start,
    std::size_t end)
{
    return Token{kind, std::move(text), make_span(start, end)};
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
        {"share", TokenKind::KwShare},
        {"form", TokenKind::KwForm},
        {"field", TokenKind::KwField},
        {"pick", TokenKind::KwPick},
        {"case", TokenKind::KwCase},
        {"proc", TokenKind::KwProc},
        {"entry", TokenKind::KwEntry},
        {"at", TokenKind::KwAt},
        {"make", TokenKind::KwMake},
        {"set", TokenKind::KwSet},
        {"give", TokenKind::KwGive},
        {"emit", TokenKind::KwEmit},
        {"if", TokenKind::KwIf},
        {"otherwise", TokenKind::KwOtherwise},
        {"select", TokenKind::KwSelect},
        {"when", TokenKind::KwWhen},
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

Lexer::Lexer(const std::string& source)
    : source_(source) {}

Token Lexer::next() {
    auto eof = [&]() {
        return index_ >= source_.size();
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
        return make_token(TokenKind::Eof, "", index_, index_);
    }

    std::size_t start = index_;
    char c = advance();

    if (is_ident_start(c)) {
        std::string ident(1, c);
        while (!eof() && is_ident_continue(peek())) {
            ident.push_back(advance());
        }
        TokenKind kind = keyword_kind(ident);
        return make_token(kind, ident, start, index_);
    }

    if (c == '-' && is_digit(peek())) {
        std::string num(1, c);
        while (!eof() && is_digit(peek())) {
            num.push_back(advance());
        }
        while (!eof() && is_suffix_start(peek())) {
            num.push_back(advance());
        }
        return make_token(TokenKind::IntLit, num, start, index_);
    }

    if (is_digit(c)) {
        std::string num(1, c);
        while (!eof() && is_digit(peek())) {
            num.push_back(advance());
        }
        while (!eof() && is_suffix_start(peek())) {
            num.push_back(advance());
        }
        return make_token(TokenKind::IntLit, num, start, index_);
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
            return make_token(TokenKind::StringLit, value, start, index_);
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
        return make_token(TokenKind::StringLit, value, start, index_);
    }

    switch (c) {
        case '#':
            if (peek() == '[') {
                advance();
                return make_token(TokenKind::AttrStart, "#[", start, index_);
            }
            break;
        case '(':
            return make_token(TokenKind::LParen, "(", start, index_);
        case ')':
            return make_token(TokenKind::RParen, ")", start, index_);
        case '{':
            return make_token(TokenKind::LBrace, "{", start, index_);
        case '}':
            return make_token(TokenKind::RBrace, "}", start, index_);
        case '[':
            return make_token(TokenKind::LBracket, "[", start, index_);
        case ']':
            return make_token(TokenKind::RBracket, "]", start, index_);
        case ',':
            return make_token(TokenKind::Comma, ",", start, index_);
        case ':':
            return make_token(TokenKind::Colon, ":", start, index_);
        case '.':
            return make_token(TokenKind::Dot, ".", start, index_);
        case '/':
            return make_token(TokenKind::Slash, "/", start, index_);
        case '+':
            return make_token(TokenKind::Plus, "+", start, index_);
        case '-':
            return make_token(TokenKind::Minus, "-", start, index_);
        case '*':
            return make_token(TokenKind::Star, "*", start, index_);
        case '=':
            if (peek() == '=') {
                advance();
                return make_token(TokenKind::EqEq, "==", start, index_);
            }
            return make_token(TokenKind::Equal, "=", start, index_);
        case '!':
            if (peek() == '=') {
                advance();
                return make_token(TokenKind::NotEq, "!=", start, index_);
            }
            break;
        case '<':
            if (peek() == '=') {
                advance();
                return make_token(TokenKind::Le, "<=", start, index_);
            }
            return make_token(TokenKind::Lt, "<", start, index_);
        case '>':
            if (peek() == '=') {
                advance();
                return make_token(TokenKind::Ge, ">=", start, index_);
            }
            return make_token(TokenKind::Gt, ">", start, index_);
        default:
            break;
    }

    return make_token(TokenKind::Eof, "", index_, index_);
}

std::vector<Token> lex(const std::string& source) {
    Lexer lexer(source);
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
