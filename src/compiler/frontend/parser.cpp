// ============================================================
// parser.cpp — Vitte Compiler
// Frontend parser implementation
// ============================================================

#include "parser.hpp"
#include "diagnostics_messages.hpp"
#include "token_tables.hpp"

#include <functional>
#include <algorithm>
#include <utility>
#include <vector>
#include <string_view>
#include <iostream>

namespace vitte::frontend::parser {

using namespace vitte::frontend::ast;

static int precedence(TokenKind kind);
static int edit_distance(std::string_view a, std::string_view b);
static const char* keyword_text(TokenKind kind);
static const char* closest_toplevel_keyword(std::string_view ident);
static void emit_toplevel_hint(DiagnosticEngine& diag, const Token& tok);
static bool is_toplevel_sync_kind(TokenKind kind);
static bool is_stmt_sync_kind(TokenKind kind);
static bool is_match_arm_sync_kind(TokenKind kind);
static const char* expected_token_fix_hint(TokenKind kind);

Parser::Parser(Lexer& lexer, DiagnosticEngine& diag, AstContext& ast_ctx, bool strict_parse)
    : Parser(lexer, diag, ast_ctx, strict_parse, false, false, 0, 0) {}

Parser::Parser(Lexer& lexer, DiagnosticEngine& diag, AstContext& ast_ctx, bool strict_parse, bool strict_core)
    : Parser(lexer, diag, ast_ctx, strict_parse, strict_core, false, 0, 0) {}

Parser::Parser(Lexer& lexer,
               DiagnosticEngine& diag,
               AstContext& ast_ctx,
               bool strict_parse,
               bool strict_core,
               bool trace_parse,
               int panic_budget,
               int panic_budget_notes)
    : lexer_(lexer),
      diag_(diag),
      ast_ctx_(ast_ctx),
      strict_(strict_parse),
      strict_core_(strict_core),
      trace_parse_(trace_parse),
      panic_budget_(panic_budget),
      panic_budget_notes_(panic_budget_notes) {
    advance();
}

const Token& Parser::current() const {
    return current_;
}

const Token& Parser::previous() const {
    return previous_;
}

Parser::State Parser::snapshot() const {
    trace("lookahead snapshot");
    const_cast<Parser*>(this)->metrics_.lookahead_snapshots++;
    return State{
        lexer_.snapshot(),
        current_,
        previous_,
    };
}

void Parser::restore(State state) {
    trace("lookahead restore");
    metrics_.lookahead_restores++;
    lexer_.restore(state.lexer);
    current_ = std::move(state.current);
    previous_ = std::move(state.previous);
}

void Parser::sync_to_toplevel_boundary() {
    trace("recover sync_to_toplevel_boundary");
    metrics_.recoveries++;
    while (current_.kind != TokenKind::Eof && !is_toplevel_sync_kind(current_.kind)) {
        advance();
    }
}

void Parser::sync_to_stmt_boundary() {
    trace("recover sync_to_stmt_boundary");
    metrics_.recoveries++;
    while (current_.kind != TokenKind::Eof && !is_stmt_sync_kind(current_.kind)) {
        advance();
    }
}

void Parser::sync_to_match_arm_boundary() {
    trace("recover sync_to_match_arm_boundary");
    metrics_.recoveries++;
    while (current_.kind != TokenKind::Eof && !is_match_arm_sync_kind(current_.kind)) {
        advance();
    }
}

void Parser::advance() {
    previous_ = current_;
    current_ = lexer_.next();
}

bool Parser::match(TokenKind kind) {
    if (current_.kind == kind) {
        advance();
        return true;
    }
    return false;
}

bool Parser::expect(TokenKind kind, const char* message) {
    if (current_.kind == kind) {
        advance();
        return true;
    }
    if (!can_emit_parse_error()) {
        return false;
    }
    ++parse_error_count_;
    ++metrics_.emitted_errors;
    const auto token_msg = diag::diag_message(diag::DiagId::ExpectedToken);
    diag_.error_code(token_msg.code, message, current_.span);
    if (kind == TokenKind::Dot && current_.kind == TokenKind::Ident && current_.text == "end") {
        note_parse("did you mean '.end'?", current_.span);
    }
    if (current_.kind == TokenKind::Ident) {
        const char* kw = keyword_text(kind);
        if (kw != nullptr && edit_distance(current_.text, kw) <= 2) {
            std::string note = "did you mean '";
            note += kw;
            note += "'?";
            note_parse(std::move(note), current_.span);
        }
    }
    if (const char* fix = expected_token_fix_hint(kind); fix != nullptr) {
        note_parse(fix, current_.span);
    }
    return false;
}

void Parser::trace(std::string_view event) const {
    if (!trace_parse_) {
        return;
    }
    std::cerr << "[parse-trace] " << event << " tok=" << current_.text << "\n";
}

bool Parser::can_emit_parse_error() const {
    if (panic_budget_ <= 0) {
        return true;
    }
    return parse_error_count_ < panic_budget_;
}

bool Parser::can_emit_parse_note() const {
    if (panic_budget_notes_ <= 0) {
        return true;
    }
    return parse_note_count_ < panic_budget_notes_;
}

void Parser::note_parse(std::string msg, SourceSpan span) {
    if (!can_emit_parse_note()) {
        return;
    }
    ++parse_note_count_;
    ++metrics_.emitted_notes;
    diag_.note(std::move(msg), span);
}

bool Parser::emit_parse_error(diag::DiagId id, SourceSpan span) {
    if (!can_emit_parse_error()) {
        return false;
    }
    ++parse_error_count_;
    ++metrics_.emitted_errors;
    diag::error(diag_, id, span);
    return true;
}

static const char* closest_toplevel_keyword(std::string_view ident) {
    static constexpr const char* kDeclKeywords[] = {
        "space", "pull", "use", "share", "const",
        "type", "form", "pick", "proc", "entry",
    };
    const char* best = nullptr;
    int best_dist = 1000;
    for (const char* kw : kDeclKeywords) {
        const int d = edit_distance(ident, kw);
        if (d < best_dist) {
            best_dist = d;
            best = kw;
        }
    }
    if (best_dist <= 2) {
        return best;
    }
    return nullptr;
}

static void emit_toplevel_hint(DiagnosticEngine& diag, const Token& tok) {
    switch (tok.kind) {
        case TokenKind::RBrace:
            diag.note("stray '}' at top level; remove it or close the matching block earlier", tok.span);
            return;
        case TokenKind::KwIf:
        case TokenKind::KwLoop:
        case TokenKind::KwFor:
        case TokenKind::KwSet:
        case TokenKind::KwGive:
        case TokenKind::KwEmit:
        case TokenKind::KwMatch:
        case TokenKind::KwWhen:
        case TokenKind::KwReturn:
        case TokenKind::KwBreak:
        case TokenKind::KwContinue:
            diag.note("this is a statement; place it inside 'proc { ... }' or 'entry { ... }'", tok.span);
            return;
        case TokenKind::Ident: {
            if (const char* kw = closest_toplevel_keyword(tok.text); kw != nullptr) {
                std::string note = "did you mean top-level keyword '";
                note += kw;
                note += "'?";
                diag.note(std::move(note), tok.span);
            }
            return;
        }
        default:
            return;
    }
}

static const char* expected_token_fix_hint(TokenKind kind) {
    switch (kind) {
        case TokenKind::RParen:
            return "insert ')' to close this parenthesized section";
        case TokenKind::RBracket:
            return "insert ']' to close this bracketed section";
        case TokenKind::RBrace:
            return "insert '}' to close this block";
        case TokenKind::Equal:
            return "add '=' between the target and assigned value";
        case TokenKind::Colon:
            return "add ':' between name and type";
        case TokenKind::KwIn:
            return "use 'for <name> in <iterable> { ... }'";
        case TokenKind::KwAs:
            return "use 'as' to introduce alias or cast target type";
        default:
            return nullptr;
    }
}

// ------------------------------------------------------------
// Entry
// ------------------------------------------------------------

ast::ModuleId Parser::parse_module() {
    std::vector<DeclId> decls;
    trace("enter parse_module");

    while (current_.kind != TokenKind::Eof) {
        if (!can_emit_parse_error()) {
            trace("panic-budget exhausted; stop parse_module");
            note_parse("parse stopped after panic budget limit", current_.span);
            break;
        }
        if (!pending_decls_.empty()) {
            decls.push_back(pending_decls_.front());
            pending_decls_.erase(pending_decls_.begin());
            continue;
        }
        auto d = parse_toplevel();
        if (d != ast::kInvalidAstId) {
            decls.push_back(d);
        } else {
            sync_to_toplevel_boundary();
            if (current_.kind != TokenKind::Eof && !is_toplevel_sync_kind(current_.kind)) {
                advance();
            }
        }
    }

    trace("exit parse_module");
    return ast_ctx_.make<Module>("<root>", std::move(decls), SourceSpan{});
}


#include "parser_toplevel.inc"
#include "parser_stmt.inc"
#include "parser_expr.inc"
#include "parser_pattern.inc"
#include "parser_type.inc"
#include "parser_helpers.inc"

} // namespace vitte::frontend::parser
