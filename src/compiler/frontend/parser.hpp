// ============================================================
// parser.hpp — Vitte Compiler
// Frontend parser interface
// ============================================================

#pragma once

#include <vector>

#include "ast.hpp"
#include "diagnostics.hpp"
#include "lexer.hpp"

namespace vitte::frontend::parser {

using vitte::frontend::ast::DeclId;
using vitte::frontend::ast::ExprId;
using vitte::frontend::ast::PatternId;
using vitte::frontend::ast::StmtId;
using vitte::frontend::ast::TypeId;
using vitte::frontend::ast::ModuleId;
using vitte::frontend::ast::AstContext;
using vitte::frontend::diag::DiagnosticEngine;

// ------------------------------------------------------------
// Parser
// ------------------------------------------------------------

class Parser {
public:
    Parser(Lexer& lexer, DiagnosticEngine& diagnostics, AstContext& ast_ctx);

    ast::ModuleId parse_module();

private:
    const Token& current() const;
    const Token& previous() const;
    void advance();
    bool match(TokenKind kind);
    bool expect(TokenKind kind, const char* message);

    // Top-level
    DeclId parse_toplevel();
    ast::ModulePath parse_module_path();
    ast::Ident parse_ident();
    ast::Attribute parse_attribute();

    DeclId parse_space_decl();
    DeclId parse_pull_decl();
    DeclId parse_share_decl();
    DeclId parse_form_decl();
    DeclId parse_pick_decl();
    DeclId parse_proc_decl(std::vector<ast::Attribute> attrs);
    DeclId parse_entry_decl();

    ast::FieldDecl parse_field_decl();
    ast::CaseDecl parse_case_decl();

    // Blocks / statements
    StmtId parse_block();
    StmtId parse_stmt();
    StmtId parse_make_stmt();
    StmtId parse_set_stmt();
    StmtId parse_give_stmt();
    StmtId parse_emit_stmt();
    StmtId parse_if_stmt();
    StmtId parse_select_stmt();
    StmtId parse_return_stmt();
    StmtId parse_expr_stmt();

    StmtId parse_when_stmt();

    // Expressions
    ExprId parse_expr();
    ExprId parse_unary_expr();
    ExprId parse_primary();
    ExprId parse_call_expr(ExprId callee);
    std::vector<ExprId> parse_arg_list();

    // Patterns
    PatternId parse_pattern();
    std::vector<PatternId> parse_pattern_args();

    // Types
    TypeId parse_type_expr();
    TypeId parse_type_expr_from_base(ast::Ident base);

    bool is_expr_start(TokenKind kind) const;
    bool is_unary_start(TokenKind kind) const;
    bool is_binary_op(TokenKind kind) const;
    ast::BinaryOp to_binary_op(TokenKind kind) const;

private:
    Lexer& lexer_;
    DiagnosticEngine& diag_;
    AstContext& ast_ctx_;
    Token current_;
    Token previous_;
};

} // namespace vitte::frontend::parser
