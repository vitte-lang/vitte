#include "parser.hpp"
#include "diagnostics.hpp"

namespace vitte::frontend {

/* =================================================
 * Recursive descent parser
 * ================================================= */

class Parser {
public:
    explicit Parser(const std::vector<Token>& t)
        : tokens(t) {}

    AstProgram parse_program() {
        AstProgram program;

        while (!check(TokenKind::EndOfFile)) {
            program.items.push_back(parse_function());
        }

        return program;
    }

private:
    const std::vector<Token>& tokens;
    std::size_t pos = 0;

    /* ---------------------------------------------
     * Helpers
     * --------------------------------------------- */
    const Token& peek() const {
        return tokens[pos];
    }

    const Token& previous() const {
        return tokens[pos - 1];
    }

    bool check(TokenKind kind) const {
        return peek().kind == kind;
    }

    bool match(TokenKind kind) {
        if (check(kind)) {
            advance();
            return true;
        }
        return false;
    }

    const Token& advance() {
        if (!check(TokenKind::EndOfFile))
            ++pos;
        return previous();
    }

    const Token& expect(TokenKind kind, const char* msg) {
        if (check(kind))
            return advance();

        error(msg);
        return advance();
    }

    /* ---------------------------------------------
     * Grammar rules
     * --------------------------------------------- */

    /* function ::= "fn" identifier "(" ")" block */
    std::unique_ptr<AstNode> parse_function() {
        expect(TokenKind::KwFn, "expected 'fn'");

        Token name = expect(
            TokenKind::Identifier,
            "expected function name"
        );

        expect(TokenKind::LParen, "expected '('");
        expect(TokenKind::RParen, "expected ')'");

        auto fn = std::make_unique<AstFunction>(name.text);
        fn->body = parse_block();

        return fn;
    }

    /* block ::= "{" { statement } "}" */
    std::unique_ptr<AstBlock> parse_block() {
        expect(TokenKind::LBrace, "expected '{'");

        auto block = std::make_unique<AstBlock>();

        while (!check(TokenKind::RBrace) &&
               !check(TokenKind::EndOfFile)) {
            block->stmts.push_back(parse_statement());
        }

        expect(TokenKind::RBrace, "expected '}'");
        return block;
    }

    /* statement ::= return_stmt */
    std::unique_ptr<AstNode> parse_statement() {
        if (match(TokenKind::KwReturn))
            return parse_return();

        error("unexpected token in statement");
        advance();
        return std::make_unique<AstLiteral>("0");
    }

    /* return_stmt ::= "return" literal ";" */
    std::unique_ptr<AstNode> parse_return() {
        auto value = parse_expression();
        expect(TokenKind::Semicolon, "expected ';' after return");
        return std::make_unique<AstReturn>(std::move(value));
    }

    /* expression ::= literal | identifier */
    std::unique_ptr<AstNode> parse_expression() {
        if (match(TokenKind::Number)) {
            return std::make_unique<AstLiteral>(
                previous().text
            );
        }

        if (match(TokenKind::Identifier)) {
            return std::make_unique<AstIdentifier>(
                previous().text
            );
        }

        error("expected expression");
        advance();
        return std::make_unique<AstLiteral>("0");
    }
};

/* -------------------------------------------------
 * Public API
 * ------------------------------------------------- */
AstProgram parse(const std::vector<Token>& tokens) {
    Parser p(tokens);
    return p.parse_program();
}

} // namespace vitte::frontend
