// ============================================================
// parser.cpp — Vitte Compiler
// Frontend parser implementation
// ============================================================

#include "parser.hpp"

#include <cassert>
#include <utility>

namespace vitte::frontend::parser {

using namespace vitte::frontend::ast;
using diag::DiagnosticEngine;

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static bool is_binary_op(TokenKind k) {
    switch (k) {
        case TokenKind::Plus:
        case TokenKind::Minus:
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::EqEq:
        case TokenKind::NotEq:
        case TokenKind::Lt:
        case TokenKind::Le:
        case TokenKind::Gt:
        case TokenKind::Ge:
        case TokenKind::And:
        case TokenKind::Or:
            return true;
        default:
            return false;
    }
}

static BinaryOp to_binary_op(TokenKind k) {
    switch (k) {
        case TokenKind::Plus:  return BinaryOp::Add;
        case TokenKind::Minus: return BinaryOp::Sub;
        case TokenKind::Star:  return BinaryOp::Mul;
        case TokenKind::Slash: return BinaryOp::Div;
        case TokenKind::EqEq:  return BinaryOp::Eq;
        case TokenKind::NotEq: return BinaryOp::Ne;
        case TokenKind::Lt:    return BinaryOp::Lt;
        case TokenKind::Le:    return BinaryOp::Le;
        case TokenKind::Gt:    return BinaryOp::Gt;
        case TokenKind::Ge:    return BinaryOp::Ge;
        case TokenKind::And:   return BinaryOp::And;
        case TokenKind::Or:    return BinaryOp::Or;
        default:
            assert(false && "invalid binary operator");
            return BinaryOp::Add;
    }
}

// ------------------------------------------------------------
// Parser
// ------------------------------------------------------------

Parser::Parser(
    Lexer& lexer,
    DiagnosticEngine& diag)
    : lexer_(lexer),
      diag_(diag)
{
    advance();
}

const Token& Parser::current() const {
    return current_;
}

void Parser::advance() {
    current_ = lexer_.next();
}

bool Parser::expect(TokenKind kind) {
    if (current_.kind == kind) {
        advance();
        return true;
    }

    diag_.error(
        "unexpected token",
        current_.span);
    return false;
}

// ------------------------------------------------------------
// Entry
// ------------------------------------------------------------

Module Parser::parse_module() {
    std::vector<DeclPtr> decls;

    while (current_.kind != TokenKind::Eof) {
        if (auto d = parse_decl()) {
            decls.push_back(std::move(d));
        } else {
            advance();
        }
    }

    return Module(
        "<root>",
        std::move(decls),
        {});
}

// ------------------------------------------------------------
// Declarations
// ------------------------------------------------------------

DeclPtr Parser::parse_decl() {
    switch (current_.kind) {
        case TokenKind::Fn:
            return parse_fn_decl();
        case TokenKind::Type:
            return parse_type_decl();
        default:
            diag_.error(
                "expected declaration",
                current_.span);
            return nullptr;
    }
}

DeclPtr Parser::parse_fn_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::Fn);

    if (current_.kind != TokenKind::Ident) {
        diag_.error("expected function name", current_.span);
        return nullptr;
    }

    Ident name(current_.text, current_.span);
    advance();

    expect(TokenKind::LParen);
    auto params = parse_params();
    expect(TokenKind::RParen);

    TypePtr ret_type = nullptr;
    if (current_.kind == TokenKind::Arrow) {
        advance();
        ret_type = parse_type();
    }

    auto body = parse_block();

    return std::make_unique<FnDecl>(
        std::move(name),
        std::move(params),
        std::move(ret_type),
        std::move(body),
        span);
}

DeclPtr Parser::parse_type_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::Type);

    if (current_.kind != TokenKind::Ident) {
        diag_.error("expected type name", current_.span);
        return nullptr;
    }

    Ident name(current_.text, current_.span);
    advance();

    expect(TokenKind::LBrace);

    std::vector<FieldDecl> fields;
    while (current_.kind != TokenKind::RBrace &&
           current_.kind != TokenKind::Eof) {
        if (current_.kind != TokenKind::Ident) {
            diag_.error("expected field name", current_.span);
            break;
        }

        Ident field_name(current_.text, current_.span);
        advance();

        expect(TokenKind::Colon);
        auto ty = parse_type();

        fields.emplace_back(
            std::move(field_name),
            std::move(ty));
    }

    expect(TokenKind::RBrace);

    return std::make_unique<TypeDecl>(
        std::move(name),
        std::move(fields),
        span);
}

// ------------------------------------------------------------
// Parameters
// ------------------------------------------------------------

std::vector<FnParam> Parser::parse_params() {
    std::vector<FnParam> params;

    if (current_.kind == TokenKind::RParen) {
        return params;
    }

    while (true) {
        if (current_.kind != TokenKind::Ident) {
            diag_.error("expected parameter name", current_.span);
            break;
        }

        Ident name(current_.text, current_.span);
        advance();

        expect(TokenKind::Colon);
        auto ty = parse_type();

        params.emplace_back(
            std::move(name),
            std::move(ty));

        if (current_.kind != TokenKind::Comma) {
            break;
        }
        advance();
    }

    return params;
}

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

TypePtr Parser::parse_type() {
    if (current_.kind != TokenKind::Ident) {
        diag_.error("expected type", current_.span);
        return nullptr;
    }

    Ident id(current_.text, current_.span);
    SourceSpan span = current_.span;
    advance();

    return std::make_unique<NamedType>(
        std::move(id),
        span);
}

// ------------------------------------------------------------
// Statements
// ------------------------------------------------------------

BlockStmt Parser::parse_block() {
    SourceSpan span = current_.span;
    expect(TokenKind::LBrace);

    std::vector<StmtPtr> stmts;
    while (current_.kind != TokenKind::RBrace &&
           current_.kind != TokenKind::Eof) {
        if (auto s = parse_stmt()) {
            stmts.push_back(std::move(s));
        } else {
            advance();
        }
    }

    expect(TokenKind::RBrace);
    return BlockStmt(std::move(stmts), span);
}

StmtPtr Parser::parse_stmt() {
    switch (current_.kind) {
        case TokenKind::Let:
            return parse_let_stmt();
        case TokenKind::Return:
            return parse_return_stmt();
        default:
            return parse_expr_stmt();
    }
}

StmtPtr Parser::parse_let_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::Let);

    if (current_.kind != TokenKind::Ident) {
        diag_.error("expected identifier", current_.span);
        return nullptr;
    }

    Ident id(current_.text, current_.span);
    advance();

    TypePtr ty = nullptr;
    if (current_.kind == TokenKind::Colon) {
        advance();
        ty = parse_type();
    }

    expect(TokenKind::Equal);
    auto init = parse_expr();

    return std::make_unique<LetStmt>(
        std::move(id),
        std::move(ty),
        std::move(init),
        span);
}

StmtPtr Parser::parse_return_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::Return);

    ExprPtr expr = nullptr;
    if (current_.kind != TokenKind::Semicolon &&
        current_.kind != TokenKind::RBrace) {
        expr = parse_expr();
    }

    return std::make_unique<ReturnStmt>(
        std::move(expr),
        span);
}

StmtPtr Parser::parse_expr_stmt() {
    auto expr = parse_expr();
    return std::make_unique<ExprStmt>(
        std::move(expr),
        expr->span);
}

// ------------------------------------------------------------
// Expressions
// ------------------------------------------------------------

ExprPtr Parser::parse_expr() {
    auto lhs = parse_primary();

    while (is_binary_op(current_.kind)) {
        TokenKind op = current_.kind;
        advance();
        auto rhs = parse_primary();

        lhs = std::make_unique<BinaryExpr>(
            to_binary_op(op),
            std::move(lhs),
            std::move(rhs),
            lhs->span);
    }

    return lhs;
}

ExprPtr Parser::parse_primary() {
    SourceSpan span = current_.span;

    switch (current_.kind) {
        case TokenKind::Ident: {
            Ident id(current_.text, span);
            advance();
            return std::make_unique<IdentExpr>(
                std::move(id),
                span);
        }
        case TokenKind::IntLit:
        case TokenKind::StringLit: {
            LiteralKind kind =
                current_.kind == TokenKind::IntLit
                    ? LiteralKind::Int
                    : LiteralKind::String;

            auto value = current_.text;
            advance();

            return std::make_unique<LiteralExpr>(
                kind,
                value,
                span);
        }
        case TokenKind::LParen: {
            advance();
            auto e = parse_expr();
            expect(TokenKind::RParen);
            return e;
        }
        default:
            diag_.error("expected expression", current_.span);
            advance();
            return nullptr;
    }
}

} // namespace vitte::frontend::parser