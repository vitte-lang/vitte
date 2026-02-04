// ============================================================
// parser.hpp — Vitte Compiler
// Frontend parser interface
// ============================================================

#pragma once

#include <memory>
#include <vector>

#include "ast.hpp"
#include "diagnostics.hpp"
#include "lexer.hpp"

namespace vitte::frontend::parser {

using vitte::frontend::ast::DeclPtr;
using vitte::frontend::ast::ExprPtr;
using vitte::frontend::ast::StmtPtr;
using vitte::frontend::ast::TypePtr;
using vitte::frontend::ast::BlockStmt;
using vitte::frontend::ast::FnParam;
using vitte::frontend::ast::Module;

using vitte::frontend::diag::DiagnosticEngine;

// ------------------------------------------------------------
// Parser
// ------------------------------------------------------------

class Parser {
public:
    Parser(Lexer& lexer, DiagnosticEngine& diagnostics);

    // Entry point
    Module parse_module();

private:
    // --------------------------------------------------------
    // Core lexer interaction
    // --------------------------------------------------------

    const Token& current() const;
    void advance();
    bool expect(TokenKind kind);

    // --------------------------------------------------------
    // Declarations
    // --------------------------------------------------------

    DeclPtr parse_decl();
    DeclPtr parse_fn_decl();
    DeclPtr parse_type_decl();

    // --------------------------------------------------------
    // Parameters & types
    // --------------------------------------------------------

    std::vector<FnParam> parse_params();
    TypePtr parse_type();

    // --------------------------------------------------------
    // Statements
    // --------------------------------------------------------

    BlockStmt parse_block();
    StmtPtr parse_stmt();
    StmtPtr parse_let_stmt();
    StmtPtr parse_return_stmt();
    StmtPtr parse_expr_stmt();

    // --------------------------------------------------------
    // Expressions
    // --------------------------------------------------------

    ExprPtr parse_expr();
    ExprPtr parse_primary();

private:
    Lexer& lexer_;
    DiagnosticEngine& diag_;
    Token current_;
};

} // namespace vitte::frontend::parser