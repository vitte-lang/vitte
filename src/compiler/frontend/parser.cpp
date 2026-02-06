// ============================================================
// parser.cpp — Vitte Compiler
// Frontend parser implementation
// ============================================================

#include "parser.hpp"
#include "diagnostics_messages.hpp"

#include <functional>
#include <algorithm>
#include <utility>
#include <vector>
#include <string_view>

namespace vitte::frontend::parser {

using namespace vitte::frontend::ast;

static int precedence(TokenKind kind);
static int edit_distance(std::string_view a, std::string_view b);
static const char* keyword_text(TokenKind kind);

Parser::Parser(Lexer& lexer, DiagnosticEngine& diag, AstContext& ast_ctx, bool strict_parse)
    : lexer_(lexer), diag_(diag), ast_ctx_(ast_ctx), strict_(strict_parse) {
    advance();
}

const Token& Parser::current() const {
    return current_;
}

const Token& Parser::previous() const {
    return previous_;
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
    diag_.error(message, current_.span);
    if (kind == TokenKind::Dot && current_.kind == TokenKind::Ident && current_.text == "end") {
        diag_.note("did you mean '.end'?", current_.span);
    }
    if (current_.kind == TokenKind::Ident) {
        const char* kw = keyword_text(kind);
        if (kw != nullptr && edit_distance(current_.text, kw) <= 2) {
            std::string note = "did you mean '";
            note += kw;
            note += "'?";
            diag_.note(std::move(note), current_.span);
        }
    }
    return false;
}

// ------------------------------------------------------------
// Entry
// ------------------------------------------------------------

ast::ModuleId Parser::parse_module() {
    std::vector<DeclId> decls;

    while (current_.kind != TokenKind::Eof) {
        if (!pending_decls_.empty()) {
            decls.push_back(pending_decls_.front());
            pending_decls_.erase(pending_decls_.begin());
            continue;
        }
        auto d = parse_toplevel();
        if (d != ast::kInvalidAstId) {
            decls.push_back(d);
        } else {
            advance();
        }
    }

    return ast_ctx_.make<Module>("<root>", std::move(decls), SourceSpan{});
}

// ------------------------------------------------------------
// Top-level
// ------------------------------------------------------------

DeclId Parser::parse_toplevel() {
    if (current_.kind == TokenKind::KwSpace) {
        return parse_space_decl();
    }
    if (current_.kind == TokenKind::KwPull) {
        return parse_pull_decl();
    }
    if (current_.kind == TokenKind::KwUse) {
        return parse_use_decl();
    }
    if (current_.kind == TokenKind::KwShare) {
        return parse_share_decl();
    }
    if (current_.kind == TokenKind::KwConst) {
        return parse_const_decl();
    }
    if (current_.kind == TokenKind::KwType) {
        return parse_type_alias_decl();
    }
    if (current_.kind == TokenKind::KwMacro) {
        return parse_macro_decl();
    }
    if (current_.kind == TokenKind::KwForm || current_.kind == TokenKind::KwTrait) {
        return parse_form_decl();
    }
    if (current_.kind == TokenKind::KwPick) {
        return parse_pick_decl();
    }
    if (current_.kind == TokenKind::KwProc || current_.kind == TokenKind::AttrStart) {
        std::vector<Attribute> attrs;
        while (current_.kind == TokenKind::AttrStart) {
            attrs.push_back(parse_attribute());
        }
        if (current_.kind != TokenKind::KwProc) {
            diag::error(diag_, diag::DiagId::ExpectedProcAfterAttribute, current_.span);
            return ast::kInvalidAstId;
        }
        return parse_proc_decl(std::move(attrs));
    }
    if (current_.kind == TokenKind::KwEntry) {
        return parse_entry_decl();
    }

    diag::error(diag_, diag::DiagId::ExpectedTopLevelDeclaration, current_.span);
    return ast::kInvalidAstId;
}

ModulePath Parser::parse_module_path() {
    std::size_t relative_depth = 0;
    SourceSpan span = current_.span;
    while (match(TokenKind::Dot)) {
        if (relative_depth == 0) {
            span.start = previous_.span.start;
        }
        relative_depth++;
    }

    std::vector<Ident> parts;
    parts.push_back(parse_ident());
    while (current_.kind == TokenKind::Slash || current_.kind == TokenKind::Dot) {
        advance();
        parts.push_back(parse_ident());
    }
    if (!parts.empty()) {
        span.end = parts.back().span.end;
    }
    return ModulePath(std::move(parts), relative_depth, span);
}

Ident Parser::parse_ident() {
    auto policy = strict_ ? KeywordPolicy::Strict : KeywordPolicy::Permissive;
    if (!is_identifier_token(current_.kind, policy)) {
        diag::error(diag_, diag::DiagId::ExpectedIdentifier, current_.span);
        advance();
        return Ident("<error>", current_.span);
    }
    Token tok = current_;
    advance();
    return Ident(tok.text, tok.span);
}

Ident Parser::parse_qualified_ident() {
    Ident base = parse_ident();
    SourceSpan span = base.span;
    std::string name = base.name;
    while (match(TokenKind::Dot)) {
        Ident part = parse_ident();
        name.append(".").append(part.name);
        span.end = part.span.end;
    }
    return Ident(std::move(name), span);
}

Attribute Parser::parse_attribute() {
    SourceSpan span = current_.span;
    expect(TokenKind::AttrStart, "expected attribute start");
    Ident name = parse_ident();
    std::vector<AttributeArg> args;
    if (match(TokenKind::LParen)) {
        if (current_.kind != TokenKind::RParen) {
            args = parse_attribute_args();
        }
        expect(TokenKind::RParen, "expected ')' after attribute args");
    }
    expect(TokenKind::RBracket, "expected ']' after attribute");
    span.end = previous_.span.end;
    return Attribute(std::move(name), std::move(args), span);
}

std::vector<AttributeArg> Parser::parse_attribute_args() {
    std::vector<AttributeArg> args;
    while (current_.kind != TokenKind::RParen && current_.kind != TokenKind::Eof) {
        if (current_.kind == TokenKind::Ident) {
            Token tok = current_;
            advance();
            args.emplace_back(AttributeArg::Kind::Ident, tok.text);
        } else if (current_.kind == TokenKind::StringLit) {
            Token tok = current_;
            advance();
            args.emplace_back(AttributeArg::Kind::String, tok.text);
        } else if (current_.kind == TokenKind::IntLit) {
            Token tok = current_;
            advance();
            args.emplace_back(AttributeArg::Kind::Int, tok.text);
        } else {
            diag_.error("expected attribute argument", current_.span);
            advance();
        }
        if (!match(TokenKind::Comma)) {
            break;
        }
    }
    return args;
}

DeclId Parser::parse_space_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwSpace, "expected 'space'");
    ModulePath path = parse_module_path();
    span.end = path.span.end;
    return ast_ctx_.make<SpaceDecl>(std::move(path), span);
}

DeclId Parser::parse_pull_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwPull, "expected 'pull'");
    ModulePath path = parse_module_path();
    std::optional<Ident> alias;
    if (match(TokenKind::KwAs)) {
        alias = parse_ident();
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<PullDecl>(std::move(path), std::move(alias), span);
}

DeclId Parser::parse_use_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwUse, "expected 'use'");
    std::size_t relative_depth = 0;
    SourceSpan path_span = current_.span;
    while (match(TokenKind::Dot)) {
        if (relative_depth == 0) {
            path_span.start = previous_.span.start;
        }
        relative_depth++;
    }
    std::vector<Ident> parts;
    parts.push_back(parse_ident());
    bool is_glob = false;
    bool has_group = false;
    std::vector<Ident> group_items;
    while (current_.kind == TokenKind::Slash || current_.kind == TokenKind::Dot) {
        bool is_dot = current_.kind == TokenKind::Dot;
        advance();
        if (is_dot && match(TokenKind::LBrace)) {
            has_group = true;
            if (current_.kind != TokenKind::RBrace) {
                if (match(TokenKind::Star)) {
                    is_glob = true;
                } else {
                    group_items.push_back(parse_ident());
                    while (match(TokenKind::Comma)) {
                        if (match(TokenKind::Star)) {
                            is_glob = true;
                            break;
                        }
                        group_items.push_back(parse_ident());
                    }
                }
            }
            expect(TokenKind::RBrace, "expected '}' after use group");
            break;
        }
        if (current_.kind == TokenKind::Star) {
            is_glob = true;
            advance();
            break;
        }
        parts.push_back(parse_ident());
    }
    if (!parts.empty()) {
        path_span.end = parts.back().span.end;
    }
    if (has_group) {
        std::vector<DeclId> decls;
        if (is_glob) {
            ModulePath path(std::vector<Ident>(parts.begin(), parts.end()), relative_depth, path_span);
            decls.push_back(ast_ctx_.make<UseDecl>(std::move(path), std::nullopt, true, span));
        }
        for (const auto& item : group_items) {
            std::vector<Ident> joined(parts.begin(), parts.end());
            joined.push_back(item);
            SourceSpan item_span = item.span;
            ModulePath path(std::move(joined), relative_depth, item_span);
            decls.push_back(ast_ctx_.make<UseDecl>(std::move(path), std::nullopt, false, span));
        }
        if (!decls.empty()) {
            for (std::size_t i = 1; i < decls.size(); ++i) {
                pending_decls_.push_back(decls[i]);
            }
            return decls[0];
        }
    }

    ModulePath path(std::move(parts), relative_depth, path_span);
    std::optional<Ident> alias;
    if (match(TokenKind::KwAs)) {
        alias = parse_ident();
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<UseDecl>(std::move(path), std::move(alias), is_glob, span);
}

DeclId Parser::parse_share_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwShare, "expected 'share'");
    bool share_all = false;
    std::vector<Ident> names;
    if (match(TokenKind::KwAll)) {
        share_all = true;
    } else {
        names.push_back(parse_ident());
        while (match(TokenKind::Comma)) {
            names.push_back(parse_ident());
        }
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<ShareDecl>(share_all, std::move(names), span);
}

DeclId Parser::parse_const_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwConst, "expected 'const'");
    Ident name = parse_ident();
    TypeId ty = ast::kInvalidAstId;
    if (match(TokenKind::Colon)) {
        ty = parse_type_expr();
    }
    expect(TokenKind::Equal, "expected '='");
    ExprId value = parse_expr();
    span.end = value != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(value).span.end
        : previous_.span.end;
    return ast_ctx_.make<ConstDecl>(std::move(name), ty, value, span);
}

DeclId Parser::parse_type_alias_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwType, "expected 'type'");
    Ident name = parse_ident();
    auto type_params = parse_type_params();
    expect(TokenKind::Equal, "expected '='");
    TypeId target = parse_type_expr();
    span.end = previous_.span.end;
    return ast_ctx_.make<TypeAliasDecl>(std::move(name), std::move(type_params), target, span);
}

DeclId Parser::parse_macro_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwMacro, "expected 'macro'");
    Ident name = parse_ident();
    expect(TokenKind::LParen, "expected '('");
    std::vector<Ident> params;
    if (current_.kind != TokenKind::RParen) {
        params.push_back(parse_ident());
        while (match(TokenKind::Comma)) {
            params.push_back(parse_ident());
        }
    }
    expect(TokenKind::RParen, "expected ')'");
    StmtId body = parse_block();
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<MacroDecl>(std::move(name), std::move(params), body, span);
}

DeclId Parser::parse_form_decl() {
    SourceSpan span = current_.span;
    if (current_.kind == TokenKind::KwTrait) {
        advance();
    } else {
        expect(TokenKind::KwForm, "expected 'form'");
    }
    Ident name = parse_ident();
    auto type_params = parse_type_params();
    std::vector<FieldDecl> fields;
    if (match(TokenKind::LBrace)) {
        while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
            Ident field_name = parse_ident();
            expect(TokenKind::Colon, "expected ':'");
            TypeId ty = parse_type_expr();
            fields.emplace_back(std::move(field_name), ty);
            if (match(TokenKind::Comma)) {
                continue;
            }
        }
        expect(TokenKind::RBrace, "expected '}'");
        span.end = previous_.span.end;
    } else {
        while (current_.kind == TokenKind::KwField) {
            fields.push_back(parse_field_decl());
        }
        expect(TokenKind::Dot, "expected '.end'");
        if (!(current_.kind == TokenKind::Ident && current_.text == "end")) {
            diag::error(diag_, diag::DiagId::ExpectedEnd, current_.span);
            diag_.note("did you mean '.end'?", current_.span);
            diag_.note("block opened here", span);
            diag_.note("parser will resume after '.end'", current_.span);
        } else {
            advance();
        }
        span.end = previous_.span.end;
    }
    return ast_ctx_.make<FormDecl>(std::move(name), std::move(type_params), std::move(fields), span);
}

DeclId Parser::parse_pick_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwPick, "expected 'pick'");
    Ident name = parse_ident();
    auto type_params = parse_type_params();
    std::vector<CaseDecl> cases;
    if (match(TokenKind::LBrace)) {
        while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
            if (match(TokenKind::KwCase)) {
                // optional in brace form
            }
            Ident case_name = parse_ident();
            std::vector<CaseField> fields;
            if (match(TokenKind::LParen)) {
                if (current_.kind != TokenKind::RParen) {
                    Ident field_name = parse_ident();
                    expect(TokenKind::Colon, "expected ':'");
                    fields.emplace_back(std::move(field_name), parse_type_expr());
                    while (match(TokenKind::Comma)) {
                        Ident next_name = parse_ident();
                        expect(TokenKind::Colon, "expected ':'");
                        fields.emplace_back(std::move(next_name), parse_type_expr());
                    }
                }
                expect(TokenKind::RParen, "expected ')'");
            }
            cases.emplace_back(std::move(case_name), std::move(fields));
            if (match(TokenKind::Comma)) {
                continue;
            }
        }
        expect(TokenKind::RBrace, "expected '}'");
        span.end = previous_.span.end;
    } else {
        while (current_.kind == TokenKind::KwCase) {
            cases.push_back(parse_case_decl());
        }
        expect(TokenKind::Dot, "expected '.end'");
        if (!(current_.kind == TokenKind::Ident && current_.text == "end")) {
            diag::error(diag_, diag::DiagId::ExpectedEnd, current_.span);
            diag_.note("did you mean '.end'?", current_.span);
            diag_.note("block opened here", span);
            diag_.note("parser will resume after '.end'", current_.span);
        } else {
            advance();
        }
        span.end = previous_.span.end;
    }
    return ast_ctx_.make<PickDecl>(std::move(name), std::move(type_params), std::move(cases), span);
}

DeclId Parser::parse_proc_decl(std::vector<Attribute> attrs) {
    SourceSpan span = current_.span;
    expect(TokenKind::KwProc, "expected 'proc'");
    Ident name = parse_ident();
    auto type_params = parse_type_params();
    expect(TokenKind::LParen, "expected '('");
    std::vector<FnParam> params;
    if (current_.kind != TokenKind::RParen) {
        Ident param_name = parse_ident();
        TypeId param_type = ast::kInvalidAstId;
        if (match(TokenKind::Colon)) {
            param_type = parse_type_expr();
        }
        params.emplace_back(std::move(param_name), param_type);
        while (match(TokenKind::Comma)) {
            Ident next_name = parse_ident();
            TypeId next_type = ast::kInvalidAstId;
            if (match(TokenKind::Colon)) {
                next_type = parse_type_expr();
            }
            params.emplace_back(std::move(next_name), next_type);
        }
    }
    expect(TokenKind::RParen, "expected ')'");
    TypeId return_type = ast::kInvalidAstId;
    if (match(TokenKind::Arrow)) {
        return_type = parse_type_expr();
    }
    StmtId body = ast::kInvalidAstId;
    if (current_.kind == TokenKind::LBrace) {
        body = parse_block();
        span.end = ast_ctx_.get<Stmt>(body).span.end;
    } else {
        span.end = previous_.span.end;
    }
    return ast_ctx_.make<ProcDecl>(
        std::move(attrs),
        std::move(name),
        std::move(type_params),
        std::move(params),
        return_type,
        body,
        span);
}

DeclId Parser::parse_entry_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwEntry, "expected 'entry'");
    Ident name = parse_ident();
    expect(TokenKind::KwAt, "expected 'at'");
    ModulePath path = parse_module_path();
    StmtId body = parse_block();
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<EntryDecl>(
        std::move(name),
        std::move(path),
        body,
        span);
}

FieldDecl Parser::parse_field_decl() {
    expect(TokenKind::KwField, "expected 'field'");
    Ident name = parse_ident();
    expect(TokenKind::KwAs, "expected 'as'");
    TypeId ty = parse_type_expr();
    return FieldDecl(std::move(name), ty);
}

CaseDecl Parser::parse_case_decl() {
    expect(TokenKind::KwCase, "expected 'case'");
    Ident name = parse_ident();
    std::vector<CaseField> fields;
    if (match(TokenKind::LParen)) {
        if (current_.kind != TokenKind::RParen) {
            Ident field_name = parse_ident();
            expect(TokenKind::KwAs, "expected 'as'");
            fields.emplace_back(std::move(field_name), parse_type_expr());
            while (match(TokenKind::Comma)) {
                Ident next_name = parse_ident();
                expect(TokenKind::KwAs, "expected 'as'");
                fields.emplace_back(std::move(next_name), parse_type_expr());
            }
        }
        expect(TokenKind::RParen, "expected ')'");
    }
    return CaseDecl(std::move(name), std::move(fields));
}

std::vector<Ident> Parser::parse_type_params() {
    std::vector<Ident> params;
    if (!match(TokenKind::LBracket)) {
        return params;
    }
    if (current_.kind != TokenKind::RBracket) {
        params.push_back(parse_ident());
        while (match(TokenKind::Comma)) {
            params.push_back(parse_ident());
        }
    }
    expect(TokenKind::RBracket, "expected ']'");
    return params;
}

// ------------------------------------------------------------
// Blocks / statements
// ------------------------------------------------------------

StmtId Parser::parse_block() {
    SourceSpan span = current_.span;
    expect(TokenKind::LBrace, "expected '{'");
    std::vector<StmtId> stmts;
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        auto s = parse_stmt();
        if (s != ast::kInvalidAstId) {
            stmts.push_back(s);
        } else {
            advance();
        }
    }
    bool closed = expect(TokenKind::RBrace, "expected '}'");
    if (!closed) {
        diag_.note("block opened here", span);
        diag_.note("parser will resume after '}'", current_.span);
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<BlockStmt>(std::move(stmts), span);
}

StmtId Parser::parse_stmt() {
    switch (current_.kind) {
        case TokenKind::KwAsm: return parse_asm_stmt();
        case TokenKind::KwUnsafe: return parse_unsafe_stmt();
        case TokenKind::KwLet: return parse_let_stmt();
        case TokenKind::KwMake: return parse_make_stmt();
        case TokenKind::KwSet: return parse_set_stmt();
        case TokenKind::KwGive: return parse_give_stmt();
        case TokenKind::KwEmit: return parse_emit_stmt();
        case TokenKind::KwIf: return parse_if_stmt();
        case TokenKind::KwLoop: return parse_loop_stmt();
        case TokenKind::KwFor: return parse_for_stmt();
        case TokenKind::KwBreak: return parse_break_stmt();
        case TokenKind::KwContinue: return parse_continue_stmt();
        case TokenKind::KwSelect: return parse_select_stmt();
        case TokenKind::KwMatch: return parse_match_stmt();
        case TokenKind::KwWhen: return parse_when_match_stmt();
        case TokenKind::KwReturn: return parse_return_stmt();
        default: return parse_expr_stmt();
    }
}

StmtId Parser::parse_asm_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwAsm, "expected 'asm'");
    expect(TokenKind::LParen, "expected '(' after 'asm'");
    std::string code;
    if (current_.kind == TokenKind::StringLit) {
        code = current_.text;
        advance();
    } else {
        diag_.error("expected string literal in asm()", current_.span);
    }
    expect(TokenKind::RParen, "expected ')' after asm");
    span.end = previous_.span.end;
    return ast_ctx_.make<AsmStmt>(std::move(code), span);
}

StmtId Parser::parse_unsafe_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwUnsafe, "expected 'unsafe'");
    StmtId body = parse_block();
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<UnsafeStmt>(body, span);
}

StmtId Parser::parse_let_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwLet, "expected 'let'");
    Ident name = parse_ident();
    TypeId ty = ast::kInvalidAstId;
    if (match(TokenKind::Colon)) {
        ty = parse_type_expr();
    }
    expect(TokenKind::Equal, "expected '='");
    ExprId value = parse_expr();
    span.end = value != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(value).span.end
        : previous_.span.end;
    return ast_ctx_.make<LetStmt>(std::move(name), ty, value, span);
}

StmtId Parser::parse_make_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwMake, "expected 'make'");
    Ident name = parse_ident();
    TypeId ty = ast::kInvalidAstId;
    if (match(TokenKind::KwAs)) {
        ty = parse_type_expr();
    }
    expect(TokenKind::Equal, "expected '='");
    ExprId value = parse_expr();
    span.end = value != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(value).span.end
        : previous_.span.end;
    return ast_ctx_.make<MakeStmt>(
        std::move(name),
        ty,
        value,
        span);
}

StmtId Parser::parse_set_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwSet, "expected 'set'");
    Ident name = parse_ident();
    expect(TokenKind::Equal, "expected '='");
    ExprId value = parse_expr();
    span.end = value != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(value).span.end
        : previous_.span.end;
    return ast_ctx_.make<SetStmt>(
        std::move(name),
        value,
        span);
}

StmtId Parser::parse_give_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwGive, "expected 'give'");
    ExprId value = parse_expr();
    span.end = value != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(value).span.end
        : previous_.span.end;
    return ast_ctx_.make<GiveStmt>(value, span);
}

StmtId Parser::parse_emit_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwEmit, "expected 'emit'");
    ExprId value = parse_expr();
    span.end = value != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(value).span.end
        : previous_.span.end;
    return ast_ctx_.make<EmitStmt>(value, span);
}

StmtId Parser::parse_if_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwIf, "expected 'if'");
    ExprId cond = parse_expr();
    StmtId then_block = parse_block();
    StmtId else_block = ast::kInvalidAstId;
    if (match(TokenKind::KwElse) || match(TokenKind::KwOtherwise)) {
        if (current_.kind == TokenKind::KwIf) {
            auto nested_if = parse_if_stmt();
            std::vector<StmtId> stmts;
            stmts.push_back(nested_if);
            SourceSpan block_span = ast_ctx_.get<Stmt>(nested_if).span;
            else_block = ast_ctx_.make<BlockStmt>(std::move(stmts), block_span);
        } else {
            else_block = parse_block();
        }
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<IfStmt>(
        cond,
        then_block,
        else_block,
        span);
}

StmtId Parser::parse_loop_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwLoop, "expected 'loop'");
    StmtId body = parse_block();
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<LoopStmt>(body, span);
}

StmtId Parser::parse_for_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwFor, "expected 'for'");
    Ident name = parse_ident();
    expect(TokenKind::KwIn, "expected 'in'");
    ExprId iterable = parse_expr();
    StmtId body = parse_block();
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<ForStmt>(std::move(name), iterable, body, span);
}

StmtId Parser::parse_break_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwBreak, "expected 'break'");
    return ast_ctx_.make<BreakStmt>(span);
}

StmtId Parser::parse_continue_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwContinue, "expected 'continue'");
    return ast_ctx_.make<ContinueStmt>(span);
}

StmtId Parser::parse_select_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwSelect, "expected 'select'");
    ExprId expr = parse_expr();
    std::vector<StmtId> whens;
    while (current_.kind == TokenKind::KwWhen) {
        whens.push_back(parse_when_stmt());
    }
    StmtId otherwise_block = ast::kInvalidAstId;
    if (match(TokenKind::KwOtherwise)) {
        otherwise_block = parse_block();
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<SelectStmt>(
        expr,
        std::move(whens),
        otherwise_block,
        span);
}

StmtId Parser::parse_match_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwMatch, "expected 'match'");
    ExprId expr = parse_expr();
    expect(TokenKind::LBrace, "expected '{' after match");
    std::vector<StmtId> whens;
    StmtId otherwise_block = ast::kInvalidAstId;
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        if (match(TokenKind::KwCase)) {
            PatternId pat = parse_pattern();
            StmtId block = parse_block();
            whens.push_back(ast_ctx_.make<WhenStmt>(pat, block, ast_ctx_.get<Stmt>(block).span));
            continue;
        }
        if (match(TokenKind::KwOtherwise) || match(TokenKind::KwElse)) {
            otherwise_block = parse_block();
            continue;
        }
        diag_.error("expected 'case' or 'otherwise' in match", current_.span);
        advance();
    }
    expect(TokenKind::RBrace, "expected '}' after match");
    span.end = previous_.span.end;
    return ast_ctx_.make<SelectStmt>(
        expr,
        std::move(whens),
        otherwise_block,
        span);
}

StmtId Parser::parse_when_match_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwWhen, "expected 'when'");

    // Parse value expression without consuming a trailing 'is'.
    auto parse_value = [&]() -> ExprId {
        std::function<ExprId(int)> parse_prec = [&](int min_prec) -> ExprId {
            auto lhs = parse_unary_expr();
            if (lhs == ast::kInvalidAstId) {
                return ast::kInvalidAstId;
            }
            while (is_binary_op(current_.kind) &&
                   current_.kind != TokenKind::KwIs &&
                   precedence(current_.kind) >= min_prec) {
                TokenKind op = current_.kind;
                int prec = precedence(op);
                advance();
                auto rhs = parse_prec(prec + 1);
                if (rhs == ast::kInvalidAstId) {
                    return lhs;
                }
                SourceSpan span = ast_ctx_.get<Expr>(lhs).span;
                span.end = ast_ctx_.get<Expr>(rhs).span.end;
                lhs = ast_ctx_.make<BinaryExpr>(
                    to_binary_op(op),
                    lhs,
                    rhs,
                    span);
            }
            return lhs;
        };

        return parse_prec(1);
    };

    ExprId value = parse_value();
    expect(TokenKind::KwIs, "expected 'is'");
    PatternId pat = parse_pattern();
    StmtId block = parse_block();
    auto when_id = ast_ctx_.make<WhenStmt>(pat, block, ast_ctx_.get<Stmt>(block).span);
    std::vector<StmtId> whens;
    whens.push_back(when_id);
    span.end = ast_ctx_.get<Stmt>(block).span.end;
    return ast_ctx_.make<SelectStmt>(value, std::move(whens), ast::kInvalidAstId, span);
}

StmtId Parser::parse_return_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwReturn, "expected 'return'");
    ExprId expr = ast::kInvalidAstId;
    if (is_expr_start(current_.kind)) {
        expr = parse_expr();
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<ReturnStmt>(expr, span);
}

StmtId Parser::parse_expr_stmt() {
    ExprId expr = parse_expr();
    SourceSpan span = expr != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(expr).span
        : current_.span;
    return ast_ctx_.make<ExprStmt>(expr, span);
}

StmtId Parser::parse_when_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwWhen, "expected 'when'");
    PatternId pat = parse_pattern();
    StmtId block = parse_block();
    span.end = ast_ctx_.get<Stmt>(block).span.end;
    return ast_ctx_.make<WhenStmt>(pat, block, span);
}

// ------------------------------------------------------------
// Expressions
// ------------------------------------------------------------

static int precedence(TokenKind kind) {
    switch (kind) {
        case TokenKind::Equal:
            return 1;
        case TokenKind::KwOr:
        case TokenKind::PipePipe:
            return 2;
        case TokenKind::KwAnd:
        case TokenKind::AmpAmp:
            return 3;
        case TokenKind::Pipe:
            return 4;
        case TokenKind::Caret:
            return 5;
        case TokenKind::Amp:
            return 6;
        case TokenKind::EqEq:
        case TokenKind::NotEq:
            return 7;
        case TokenKind::Lt:
        case TokenKind::Le:
        case TokenKind::Gt:
        case TokenKind::Ge:
            return 8;
        case TokenKind::Shl:
        case TokenKind::Shr:
            return 9;
        case TokenKind::Plus:
        case TokenKind::Minus:
            return 10;
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:
            return 11;
        default:
            return 0;
    }
}

static int edit_distance(std::string_view a, std::string_view b) {
    const std::size_t n = a.size();
    const std::size_t m = b.size();
    if (n == 0) return static_cast<int>(m);
    if (m == 0) return static_cast<int>(n);
    std::vector<int> prev(m + 1), cur(m + 1);
    for (std::size_t j = 0; j <= m; ++j) {
        prev[j] = static_cast<int>(j);
    }
    for (std::size_t i = 1; i <= n; ++i) {
        cur[0] = static_cast<int>(i);
        for (std::size_t j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
        }
        prev.swap(cur);
    }
    return prev[m];
}

static const char* keyword_text(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwSpace: return "space";
        case TokenKind::KwPull: return "pull";
        case TokenKind::KwUse: return "use";
        case TokenKind::KwShare: return "share";
        case TokenKind::KwForm: return "form";
        case TokenKind::KwField: return "field";
        case TokenKind::KwPick: return "pick";
        case TokenKind::KwCase: return "case";
        case TokenKind::KwTrait: return "trait";
        case TokenKind::KwType: return "type";
        case TokenKind::KwConst: return "const";
        case TokenKind::KwMacro: return "macro";
        case TokenKind::KwProc: return "proc";
        case TokenKind::KwEntry: return "entry";
        case TokenKind::KwAt: return "at";
        case TokenKind::KwAsm: return "asm";
        case TokenKind::KwUnsafe: return "unsafe";
        case TokenKind::KwMatch: return "match";
        case TokenKind::KwLet: return "let";
        case TokenKind::KwMake: return "make";
        case TokenKind::KwSet: return "set";
        case TokenKind::KwGive: return "give";
        case TokenKind::KwEmit: return "emit";
        case TokenKind::KwIf: return "if";
        case TokenKind::KwElse: return "else";
        case TokenKind::KwOtherwise: return "otherwise";
        case TokenKind::KwSelect: return "select";
        case TokenKind::KwWhen: return "when";
        case TokenKind::KwIs: return "is";
        case TokenKind::KwLoop: return "loop";
        case TokenKind::KwFor: return "for";
        case TokenKind::KwIn: return "in";
        case TokenKind::KwBreak: return "break";
        case TokenKind::KwContinue: return "continue";
        case TokenKind::KwReturn: return "return";
        case TokenKind::KwNot: return "not";
        case TokenKind::KwAnd: return "and";
        case TokenKind::KwOr: return "or";
        case TokenKind::KwAs: return "as";
        case TokenKind::KwAll: return "all";
        case TokenKind::KwTrue: return "true";
        case TokenKind::KwFalse: return "false";
        case TokenKind::KwBool: return "bool";
        case TokenKind::KwString: return "string";
        case TokenKind::KwInt: return "int";
        default: return nullptr;
    }
}

ExprId Parser::parse_expr() {
    std::function<ExprId(int)> parse_prec = [&](int min_prec) -> ExprId {
        auto lhs = parse_unary_expr();
        if (lhs == ast::kInvalidAstId) {
            return ast::kInvalidAstId;
        }

        while (true) {
            if (current_.kind == TokenKind::KwAs) {
                advance();
                TypeId ty = parse_type_expr();
                SourceSpan span = ast_ctx_.get<Expr>(lhs).span;
                if (ty != ast::kInvalidAstId) {
                    span.end = ast_ctx_.get<TypeNode>(ty).span.end;
                }
                lhs = ast_ctx_.make<AsExpr>(lhs, ty, span);
                continue;
            }
            if (current_.kind == TokenKind::KwIs &&
                precedence(TokenKind::EqEq) >= min_prec) {
                advance();
                PatternId pat = parse_pattern();
                SourceSpan span = ast_ctx_.get<Expr>(lhs).span;
                if (pat != ast::kInvalidAstId) {
                    span.end = ast_ctx_.get<Pattern>(pat).span.end;
                }
                lhs = ast_ctx_.make<IsExpr>(lhs, pat, span);
                continue;
            }
            if (!is_binary_op(current_.kind) ||
                precedence(current_.kind) < min_prec) {
                break;
            }
            TokenKind op = current_.kind;
            int prec = precedence(op);
            advance();
            int next_min_prec = (op == TokenKind::Equal) ? prec : (prec + 1);
            auto rhs = parse_prec(next_min_prec);
            if (rhs == ast::kInvalidAstId) {
                return lhs;
            }
            SourceSpan span = ast_ctx_.get<Expr>(lhs).span;
            span.end = ast_ctx_.get<Expr>(rhs).span.end;
            lhs = ast_ctx_.make<BinaryExpr>(
                to_binary_op(op),
                lhs,
                rhs,
                span);
        }
        return lhs;
    };

    return parse_prec(1);
}

ExprId Parser::parse_unary_expr() {
    if (match(TokenKind::KwNot) || match(TokenKind::Bang)) {
        auto rhs = parse_unary_expr();
        SourceSpan span = previous_.span;
        if (rhs != ast::kInvalidAstId) {
            span.end = ast_ctx_.get<Expr>(rhs).span.end;
        }
        return ast_ctx_.make<UnaryExpr>(UnaryOp::Not, rhs, span);
    }
    if (match(TokenKind::Minus)) {
        auto rhs = parse_unary_expr();
        SourceSpan span = previous_.span;
        if (rhs != ast::kInvalidAstId) {
            span.end = ast_ctx_.get<Expr>(rhs).span.end;
        }
        return ast_ctx_.make<UnaryExpr>(UnaryOp::Neg, rhs, span);
    }
    if (match(TokenKind::Amp)) {
        auto rhs = parse_unary_expr();
        SourceSpan span = previous_.span;
        if (rhs != ast::kInvalidAstId) {
            span.end = ast_ctx_.get<Expr>(rhs).span.end;
        }
        return ast_ctx_.make<UnaryExpr>(UnaryOp::Addr, rhs, span);
    }
    if (match(TokenKind::Star)) {
        auto rhs = parse_unary_expr();
        SourceSpan span = previous_.span;
        if (rhs != ast::kInvalidAstId) {
            span.end = ast_ctx_.get<Expr>(rhs).span.end;
        }
        return ast_ctx_.make<UnaryExpr>(UnaryOp::Deref, rhs, span);
    }
    return parse_primary();
}

ExprId Parser::parse_primary() {
    SourceSpan span = current_.span;

    if (current_.kind == TokenKind::KwTrue || current_.kind == TokenKind::KwFalse) {
        std::string value = current_.text;
        advance();
        return ast_ctx_.make<LiteralExpr>(LiteralKind::Bool, value, span);
    }

    if (current_.kind == TokenKind::IntLit) {
        std::string value = current_.text;
        advance();
        return ast_ctx_.make<LiteralExpr>(LiteralKind::Int, value, span);
    }

    if (current_.kind == TokenKind::FloatLit) {
        std::string value = current_.text;
        advance();
        return ast_ctx_.make<LiteralExpr>(LiteralKind::Float, value, span);
    }

    if (current_.kind == TokenKind::CharLit) {
        std::string value = current_.text;
        advance();
        return ast_ctx_.make<LiteralExpr>(LiteralKind::Char, value, span);
    }

    if (current_.kind == TokenKind::StringLit) {
        std::string value = current_.text;
        advance();
        return ast_ctx_.make<LiteralExpr>(LiteralKind::String, value, span);
    }

    if (current_.kind == TokenKind::KwIf) {
        return parse_if_expr();
    }

    if (current_.kind == TokenKind::KwProc) {
        return parse_proc_expr();
    }

    if (current_.kind == TokenKind::LBracket) {
        advance();
        std::vector<ExprId> items;
        if (current_.kind != TokenKind::RBracket) {
            items = parse_arg_list();
        }
        expect(TokenKind::RBracket, "expected ']'");
        span.end = previous_.span.end;
        return ast_ctx_.make<ListExpr>(std::move(items), span);
    }

    if (current_.kind == TokenKind::LParen) {
        advance();
        ExprId expr = parse_expr();
        expect(TokenKind::RParen, "expected ')'");
        return parse_postfix_expr(expr);
    }

    if (!strict_ && is_type_keyword(current_.kind)) {
        Ident ident(current_.text, current_.span);
        advance();
        auto base = ast_ctx_.make<IdentExpr>(std::move(ident), ident.span);
        return parse_postfix_expr(base);
    }

    if (current_.kind == TokenKind::Ident) {
        Ident ident = parse_ident();
        auto base = ast_ctx_.make<IdentExpr>(std::move(ident), ident.span);
        return parse_postfix_expr(base);
    }

    diag::error(diag_, diag::DiagId::ExpectedExpression, current_.span);
    diag_.note("expected expression (e.g. 1, name, call(), { ... })", current_.span);
    diag_.note("parser will resume after the next token", current_.span);
    advance();
    return ast::kInvalidAstId;
}

ExprId Parser::parse_postfix_expr(ExprId base) {
    ExprId expr = base;
    while (true) {
        if (match(TokenKind::Dot)) {
            if (match(TokenKind::Star)) {
                SourceSpan span = ast_ctx_.get<Expr>(expr).span;
                span.end = previous_.span.end;
                expr = ast_ctx_.make<UnaryExpr>(UnaryOp::Deref, expr, span);
            } else {
                Ident member = parse_ident();
                SourceSpan span = ast_ctx_.get<Expr>(expr).span;
                span.end = member.span.end;
                expr = ast_ctx_.make<MemberExpr>(expr, std::move(member), span);
            }
            continue;
        }
        if (current_.kind == TokenKind::LParen) {
            expr = parse_call_expr(expr);
            continue;
        }
        if (match(TokenKind::LBracket)) {
            ExprId index = parse_expr();
            expect(TokenKind::RBracket, "expected ']'");
            SourceSpan span = ast_ctx_.get<Expr>(expr).span;
            span.end = previous_.span.end;
            expr = ast_ctx_.make<IndexExpr>(expr, index, span);
            continue;
        }
        break;
    }
    return expr;
}

ExprId Parser::parse_call_expr(ExprId callee) {
    SourceSpan span = callee != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(callee).span
        : current_.span;
    expect(TokenKind::LParen, "expected '('");
    std::vector<ExprId> args;
    if (current_.kind != TokenKind::RParen) {
        args = parse_arg_list();
    }
    expect(TokenKind::RParen, "expected ')'");
    span.end = previous_.span.end;
    return ast_ctx_.make<InvokeExpr>(
        callee,
        ast::kInvalidAstId,
        std::move(args),
        span);
}

ExprId Parser::parse_proc_expr() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwProc, "expected 'proc'");
    expect(TokenKind::LParen, "expected '('");
    std::vector<FnParam> params;
    if (current_.kind != TokenKind::RParen) {
        Ident param_name = parse_ident();
        TypeId param_type = ast::kInvalidAstId;
        if (match(TokenKind::Colon)) {
            param_type = parse_type_expr();
        }
        params.emplace_back(std::move(param_name), param_type);
        while (match(TokenKind::Comma)) {
            Ident next_name = parse_ident();
            TypeId next_type = ast::kInvalidAstId;
            if (match(TokenKind::Colon)) {
                next_type = parse_type_expr();
            }
            params.emplace_back(std::move(next_name), next_type);
        }
    }
    expect(TokenKind::RParen, "expected ')'");
    TypeId return_type = ast::kInvalidAstId;
    if (match(TokenKind::Arrow)) {
        return_type = parse_type_expr();
    }
    StmtId body = parse_block();
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<ProcExpr>(std::move(params), return_type, body, span);
}

ExprId Parser::parse_if_expr() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwIf, "expected 'if'");
    ExprId cond = parse_expr();
    StmtId then_block = parse_block();
    StmtId else_block = ast::kInvalidAstId;
    if (match(TokenKind::KwElse) || match(TokenKind::KwOtherwise)) {
        if (current_.kind == TokenKind::KwIf) {
            ExprId else_expr = parse_if_expr();
            SourceSpan expr_span = else_expr != ast::kInvalidAstId
                ? ast_ctx_.get<Expr>(else_expr).span
                : current_.span;
            StmtId expr_stmt = ast_ctx_.make<ExprStmt>(else_expr, expr_span);
            std::vector<StmtId> stmts;
            stmts.push_back(expr_stmt);
            else_block = ast_ctx_.make<BlockStmt>(std::move(stmts), expr_span);
        } else {
            else_block = parse_block();
        }
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<IfExpr>(cond, then_block, else_block, span);
}

std::vector<ExprId> Parser::parse_arg_list() {
    std::vector<ExprId> args;
    args.push_back(parse_expr());
    while (match(TokenKind::Comma)) {
        args.push_back(parse_expr());
    }
    return args;
}

// ------------------------------------------------------------
// Patterns
// ------------------------------------------------------------

PatternId Parser::parse_pattern() {
    SourceSpan span = current_.span;

    if (current_.kind == TokenKind::Ident ||
        current_.kind == TokenKind::KwBool ||
        current_.kind == TokenKind::KwString ||
        current_.kind == TokenKind::KwInt ||
        current_.kind == TokenKind::KwAnd ||
        current_.kind == TokenKind::KwOr ||
        current_.kind == TokenKind::KwNot ||
        current_.kind == TokenKind::KwAll) {
        Ident ident = parse_qualified_ident();
        bool has_dot = ident.name.find('.') != std::string::npos;
        if (current_.kind == TokenKind::LParen) {
            TypeId ty = ast_ctx_.make<NamedType>(ident, ident.span);
            expect(TokenKind::LParen, "expected '('");
            std::vector<PatternId> args;
            if (current_.kind != TokenKind::RParen) {
                args = parse_pattern_args();
            }
            expect(TokenKind::RParen, "expected ')'");
            span.end = previous_.span.end;
            return ast_ctx_.make<CtorPattern>(
                ty,
                std::move(args),
                span);
        }
        if (has_dot) {
            TypeId ty = ast_ctx_.make<NamedType>(ident, ident.span);
            span.end = ident.span.end;
            return ast_ctx_.make<CtorPattern>(
                ty,
                std::vector<PatternId>{},
                span);
        }
        return ast_ctx_.make<IdentPattern>(std::move(ident), span);
    }

    diag::error(diag_, diag::DiagId::ExpectedPattern, current_.span);
    advance();
    return ast::kInvalidAstId;
}

std::vector<PatternId> Parser::parse_pattern_args() {
    std::vector<PatternId> args;
    args.push_back(parse_pattern());
    while (match(TokenKind::Comma)) {
        args.push_back(parse_pattern());
    }
    return args;
}

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

TypeId Parser::parse_type_expr() {
    return parse_type_primary();
}

TypeId Parser::parse_type_primary() {
    SourceSpan span = current_.span;
    if (match(TokenKind::Star)) {
        TypeId inner = parse_type_expr();
        span.end = inner != ast::kInvalidAstId
            ? ast_ctx_.get<TypeNode>(inner).span.end
            : previous_.span.end;
        return ast_ctx_.make<PointerType>(inner, span);
    }
    if (match(TokenKind::LBracket)) {
        TypeId elem = parse_type_expr();
        expect(TokenKind::RBracket, "expected ']'");
        span.end = previous_.span.end;
        return ast_ctx_.make<SliceType>(elem, span);
    }
    if (match(TokenKind::KwProc)) {
        expect(TokenKind::LParen, "expected '('");
        std::vector<TypeId> params;
        if (current_.kind != TokenKind::RParen) {
            while (true) {
                if (current_.kind == TokenKind::Ident) {
                    Ident param_name = parse_ident();
                    if (match(TokenKind::Colon)) {
                        params.push_back(parse_type_expr());
                    } else {
                        params.push_back(parse_type_expr_from_base(std::move(param_name)));
                    }
                } else {
                    params.push_back(parse_type_expr());
                }
                if (!match(TokenKind::Comma)) {
                    break;
                }
            }
        }
        expect(TokenKind::RParen, "expected ')'");
        TypeId ret = ast::kInvalidAstId;
        if (match(TokenKind::Arrow)) {
            ret = parse_type_expr();
        }
        span.end = previous_.span.end;
        return ast_ctx_.make<ProcType>(std::move(params), ret, span);
    }
    if (current_.kind == TokenKind::KwBool) {
        advance();
        return ast_ctx_.make<BuiltinType>("bool", span);
    }
    if (current_.kind == TokenKind::KwString) {
        advance();
        return ast_ctx_.make<BuiltinType>("string", span);
    }
    if (current_.kind == TokenKind::KwInt) {
        advance();
        return ast_ctx_.make<BuiltinType>("int", span);
    }

    if (current_.kind != TokenKind::Ident) {
        diag::error(diag_, diag::DiagId::ExpectedType, current_.span);
        diag_.note("expected type (e.g. int, string, bool, Option[T])", current_.span);
        diag_.note("parser will resume after the next token", current_.span);
        return ast::kInvalidAstId;
    }

    Ident base = parse_qualified_ident();
    return parse_type_expr_from_base(std::move(base));
}

TypeId Parser::parse_type_expr_from_base(Ident base) {
    SourceSpan span = base.span;
    if (match(TokenKind::LBracket)) {
        std::vector<TypeId> args;
        if (current_.kind != TokenKind::RBracket) {
            args.push_back(parse_type_expr());
            while (match(TokenKind::Comma)) {
                args.push_back(parse_type_expr());
            }
        }
        expect(TokenKind::RBracket, "expected ']'");
        span.end = previous_.span.end;
        return ast_ctx_.make<GenericType>(
            std::move(base),
            std::move(args),
            span);
    }

    return ast_ctx_.make<NamedType>(std::move(base), span);
}

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

bool Parser::is_expr_start(TokenKind kind) const {
    return is_unary_start(kind);
}

bool Parser::is_unary_start(TokenKind kind) const {
    if (!strict_ && is_type_keyword(kind)) {
        return true;
    }
    switch (kind) {
        case TokenKind::Bang:
        case TokenKind::Minus:
        case TokenKind::Amp:
        case TokenKind::Star:
        case TokenKind::KwNot:
        case TokenKind::KwIf:
        case TokenKind::KwProc:
        case TokenKind::KwTrue:
        case TokenKind::KwFalse:
        case TokenKind::IntLit:
        case TokenKind::FloatLit:
        case TokenKind::CharLit:
        case TokenKind::StringLit:
        case TokenKind::Ident:
        case TokenKind::LParen:
        case TokenKind::LBracket:
            return true;
        default:
            return false;
    }
}

bool Parser::is_binary_op(TokenKind kind) const {
    switch (kind) {
        case TokenKind::Plus:
        case TokenKind::Minus:
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:
        case TokenKind::Amp:
        case TokenKind::Pipe:
        case TokenKind::Caret:
        case TokenKind::AmpAmp:
        case TokenKind::PipePipe:
        case TokenKind::Shl:
        case TokenKind::Shr:
        case TokenKind::Equal:
        case TokenKind::EqEq:
        case TokenKind::NotEq:
        case TokenKind::Lt:
        case TokenKind::Le:
        case TokenKind::Gt:
        case TokenKind::Ge:
        case TokenKind::KwAnd:
        case TokenKind::KwOr:
            return true;
        default:
            return false;
    }
}

BinaryOp Parser::to_binary_op(TokenKind kind) const {
    switch (kind) {
        case TokenKind::Plus: return BinaryOp::Add;
        case TokenKind::Minus: return BinaryOp::Sub;
        case TokenKind::Star: return BinaryOp::Mul;
        case TokenKind::Slash: return BinaryOp::Div;
        case TokenKind::Percent: return BinaryOp::Mod;
        case TokenKind::EqEq: return BinaryOp::Eq;
        case TokenKind::NotEq: return BinaryOp::Ne;
        case TokenKind::Lt: return BinaryOp::Lt;
        case TokenKind::Le: return BinaryOp::Le;
        case TokenKind::Gt: return BinaryOp::Gt;
        case TokenKind::Ge: return BinaryOp::Ge;
        case TokenKind::Amp: return BinaryOp::BitAnd;
        case TokenKind::Pipe: return BinaryOp::BitOr;
        case TokenKind::Caret: return BinaryOp::BitXor;
        case TokenKind::Shl: return BinaryOp::Shl;
        case TokenKind::Shr: return BinaryOp::Shr;
        case TokenKind::Equal: return BinaryOp::Assign;
        case TokenKind::KwAnd: return BinaryOp::And;
        case TokenKind::KwOr: return BinaryOp::Or;
        case TokenKind::AmpAmp: return BinaryOp::And;
        case TokenKind::PipePipe: return BinaryOp::Or;
        default:
            return BinaryOp::Add;
    }
}

} // namespace vitte::frontend::parser
