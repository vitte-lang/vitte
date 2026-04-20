// ============================================================
// parser.hpp — Vitte Compiler
// Frontend parser interface
// ============================================================

#pragma once

#include <vector>
#include <string_view>

#include "ast.hpp"
#include "diagnostics.hpp"
#include "diagnostics_messages.hpp"
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
    struct ParseMetrics {
        int emitted_errors = 0;
        int emitted_notes = 0;
        int recoveries = 0;
        int lookahead_snapshots = 0;
        int lookahead_restores = 0;
    };

    Parser(Lexer& lexer, DiagnosticEngine& diagnostics, AstContext& ast_ctx, bool strict_parse);
    Parser(Lexer& lexer, DiagnosticEngine& diagnostics, AstContext& ast_ctx, bool strict_parse, bool strict_core);
    Parser(Lexer& lexer,
           DiagnosticEngine& diagnostics,
           AstContext& ast_ctx,
           bool strict_parse,
           bool strict_core,
           bool trace_parse,
           int panic_budget,
           int panic_budget_notes = 0,
           bool syntax_strict = false);

    ast::ModuleId parse_module();
    const ParseMetrics& metrics() const { return metrics_; }

private:
    struct State {
        Lexer::State lexer;
        Token current;
        Token previous;
    };

    const Token& current() const;
    const Token& previous() const;
    void advance();
    bool match(TokenKind kind);
    bool expect(TokenKind kind, const char* message);
    State snapshot() const;
    void restore(State state);
    void sync_to_toplevel_boundary();
    void sync_to_stmt_boundary();
    void sync_to_match_arm_boundary();
    void trace(std::string_view event) const;
    void note_parse(std::string msg, ast::SourceSpan span);
    bool can_emit_parse_error() const;
    bool can_emit_parse_note() const;
    bool emit_parse_error(::vitte::frontend::diag::DiagId id, ast::SourceSpan span);

    // Top-level
    DeclId parse_toplevel();
    ast::ModulePath parse_module_path();
    ast::Ident parse_ident();
    ast::Attribute parse_attribute();
    std::vector<ast::AttributeArg> parse_attribute_args();

    DeclId parse_space_decl();
    DeclId parse_from_import_decl();
    DeclId parse_pull_decl();
    DeclId parse_use_decl();
    DeclId parse_share_decl();
    DeclId parse_const_decl();
    DeclId parse_global_decl();
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
    StmtId parse_while_stmt();
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
    bool try_recover_empty_generic_call_suffix(ExprId& expr);
    ExprId try_parse_generic_ctor_call_expr(ExprId base);
    ExprId try_parse_generic_proc_call_expr(ExprId base);
    ExprId parse_call_expr(ExprId callee);
    ExprId parse_proc_expr();
    ExprId parse_if_expr();
    ExprId parse_interpolated_string_expr(const std::string& value, ast::SourceSpan span);
    ExprId parse_interpolation_path_expr(std::string_view raw, ast::SourceSpan span);
    std::vector<ExprId> parse_arg_list();
    std::vector<ast::InvokeArg> parse_call_arg_list();

    // Patterns
    PatternId parse_pattern();
    std::vector<PatternId> parse_pattern_args();

    // Types
    TypeId parse_type_expr();
    TypeId parse_type_expr_from_base(ast::Ident base);
    TypeId parse_type_primary();
    ast::Ident parse_qualified_ident();
    bool looks_like_type_constructor_head(ExprId expr) const;
    bool looks_like_type_expr_start(TokenKind kind) const;
    bool is_unambiguous_type_expr(TypeId type) const;

    bool is_expr_start(TokenKind kind) const;
    bool is_unary_start(TokenKind kind) const;
    bool is_binary_op(TokenKind kind) const;
    ast::BinaryOp to_binary_op(TokenKind kind) const;

private:
    Lexer& lexer_;
    DiagnosticEngine& diag_;
    AstContext& ast_ctx_;
    bool strict_;
    bool strict_core_;
    bool syntax_strict_ = false;
    bool trace_parse_ = false;
    int panic_budget_ = 0;
    int panic_budget_notes_ = 0;
    int parse_error_count_ = 0;
    int parse_note_count_ = 0;
    ParseMetrics metrics_{};
    Token current_;
    Token previous_;
    std::vector<DeclId> pending_decls_;
};

} // namespace vitte::frontend::parser
