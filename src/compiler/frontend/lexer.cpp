#include "lexer.hpp"
#include "diagnostics.hpp"

#include <cctype>

namespace vitte::frontend {

/* -------------------------------------------------
 * Lexer state
 * ------------------------------------------------- */
class Lexer {
public:
    explicit Lexer(const std::string& src)
        : source(src) {}

    std::vector<Token> run() {
        std::vector<Token> tokens;
        while (!eof()) {
            skip_whitespace();
            if (eof()) break;
            tokens.push_back(next_token());
        }

        tokens.push_back(make_token(TokenKind::EndOfFile, ""));
        return tokens;
    }

private:
    const std::string& source;
    std::size_t index = 0;
    SourcePos pos{1, 1};

    /* ---------------------------------------------
     * Helpers
     * --------------------------------------------- */
    bool eof() const {
        return index >= source.size();
    }

    char peek() const {
        return eof() ? '\0' : source[index];
    }

    char advance() {
        char c = peek();
        ++index;
        if (c == '\n') {
            ++pos.line;
            pos.column = 1;
        } else {
            ++pos.column;
        }
        return c;
    }

    void skip_whitespace() {
        while (!eof() && std::isspace(peek()))
            advance();
    }

    Token make_token(TokenKind kind, std::string text) {
        return Token{kind, std::move(text), pos};
    }

    /* ---------------------------------------------
     * Token lexing
     * --------------------------------------------- */
    Token next_token() {
        char c = advance();

        /* Identifiers / keywords */
        if (std::isalpha(c) || c == '_') {
            std::string ident(1, c);
            while (!eof() && (std::isalnum(peek()) || peek() == '_'))
                ident.push_back(advance());

            if (ident == "fn")     return make_token(TokenKind::KwFn, ident);
            if (ident == "return") return make_token(TokenKind::KwReturn, ident);

            return make_token(TokenKind::Identifier, ident);
        }

        /* Numbers */
        if (std::isdigit(c)) {
            std::string num(1, c);
            while (!eof() && std::isdigit(peek()))
                num.push_back(advance());
            return make_token(TokenKind::Number, num);
        }

        /* Strings */
        if (c == '"') {
            std::string value;
            while (!eof() && peek() != '"') {
                value.push_back(advance());
            }
            if (eof()) {
                error("unterminated string literal");
                return make_token(TokenKind::String, value);
            }
            advance(); // closing "
            return make_token(TokenKind::String, value);
        }

        /* Single-character tokens */
        switch (c) {
            case '(': return make_token(TokenKind::LParen, "(");
            case ')': return make_token(TokenKind::RParen, ")");
            case '{': return make_token(TokenKind::LBrace, "{");
            case '}': return make_token(TokenKind::RBrace, "}");
            case ';': return make_token(TokenKind::Semicolon, ";");
            case ',': return make_token(TokenKind::Comma, ",");
            case '+': return make_token(TokenKind::Plus, "+");
            case '-': return make_token(TokenKind::Minus, "-");
            case '*': return make_token(TokenKind::Star, "*");
            case '/': return make_token(TokenKind::Slash, "/");
            case '=': return make_token(TokenKind::Equal, "=");
        }

        error(std::string("unexpected character: '") + c + "'");
        return make_token(TokenKind::EndOfFile, "");
    }
};

/* -------------------------------------------------
 * Public API
 * ------------------------------------------------- */
std::vector<Token> lex(const std::string& source) {
    Lexer l(source);
    return l.run();
}

} // namespace vitte::frontend
