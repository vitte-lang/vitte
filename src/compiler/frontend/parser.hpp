// ============================================================
// parser.hpp — Vitte Compiler
// Frontend parser interface
// ============================================================

#pragma once

#include <vector>

#include "ast.hpp"
#include "diagnostics.hpp"
#include "keywords.hpp"
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
    Parser(Lexer& lexer, DiagnosticEngine& diagnostics, AstContext& ast_ctx, bool strict_parse);

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
    std::vector<ast::AttributeArg> parse_attribute_args();

    DeclId parse_space_decl();
    DeclId parse_pull_decl();
    DeclId parse_use_decl();
    DeclId parse_share_decl();
    DeclId parse_const_decl();
    DeclId parse_type_alias_decl();
    DeclId parse_macro_decl();
    DeclId parse_form_decl();
    DeclId parse_pick_decl();
    DeclId parse_proc_decl(std::vector<ast::Attribute> attrs);
    DeclId parse_entry_decl();

    ast::FieldDecl parse_field_decl();
    ast::CaseDecl parse_case_decl();
    std::vector<ast::Ident> parse_type_params();

    // Blocks / statements
    StmtId parse_block();
    StmtId parse_stmt();
    StmtId parse_asm_stmt();
    StmtId parse_unsafe_stmt();
    StmtId parse_let_stmt();
    StmtId parse_make_stmt();
    StmtId parse_set_stmt();
    StmtId parse_give_stmt();
    StmtId parse_emit_stmt();
    StmtId parse_if_stmt();
    StmtId parse_loop_stmt();
    StmtId parse_for_stmt();
    StmtId parse_break_stmt();
    StmtId parse_continue_stmt();
    StmtId parse_select_stmt();
    StmtId parse_match_stmt();
    StmtId parse_when_match_stmt();
    StmtId parse_return_stmt();
    StmtId parse_expr_stmt();

    StmtId parse_when_stmt();

    // Expressions
    ExprId parse_expr();
    ExprId parse_unary_expr();
    ExprId parse_primary();
    ExprId parse_postfix_expr(ExprId base);
    ExprId parse_call_expr(ExprId callee);
    ExprId parse_proc_expr();
    ExprId parse_if_expr();
    std::vector<ExprId> parse_arg_list();

    // Patterns
    PatternId parse_pattern();
    std::vector<PatternId> parse_pattern_args();

    // Types
    TypeId parse_type_expr();
    TypeId parse_type_expr_from_base(ast::Ident base);
    TypeId parse_type_primary();
    ast::Ident parse_qualified_ident();

    bool is_expr_start(TokenKind kind) const;
    bool is_unary_start(TokenKind kind) const;
    bool is_binary_op(TokenKind kind) const;
    ast::BinaryOp to_binary_op(TokenKind kind) const;

private:
    Lexer& lexer_;
    DiagnosticEngine& diag_;
    AstContext& ast_ctx_;
    bool strict_;
    Token current_;
    Token previous_;
    std::vector<DeclId> pending_decls_;
};

} // namespace vitte::frontend::parser
