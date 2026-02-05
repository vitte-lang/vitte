// ============================================================
// parser.cpp — Vitte Compiler
// Frontend parser implementation
// ============================================================

#include "parser.hpp"

#include <functional>
#include <utility>

namespace vitte::frontend::parser {

using namespace vitte::frontend::ast;

Parser::Parser(Lexer& lexer, DiagnosticEngine& diag, AstContext& ast_ctx)
    : lexer_(lexer), diag_(diag), ast_ctx_(ast_ctx) {
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
    return false;
}

// ------------------------------------------------------------
// Entry
// ------------------------------------------------------------

ast::ModuleId Parser::parse_module() {
    std::vector<DeclId> decls;

    while (current_.kind != TokenKind::Eof) {
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
    if (current_.kind == TokenKind::KwShare) {
        return parse_share_decl();
    }
    if (current_.kind == TokenKind::KwForm) {
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
            diag_.error("expected proc after attribute", current_.span);
            return ast::kInvalidAstId;
        }
        return parse_proc_decl(std::move(attrs));
    }
    if (current_.kind == TokenKind::KwEntry) {
        return parse_entry_decl();
    }

    diag_.error("expected top-level declaration", current_.span);
    return ast::kInvalidAstId;
}

ModulePath Parser::parse_module_path() {
    std::vector<Ident> parts;
    parts.push_back(parse_ident());
    while (match(TokenKind::Slash)) {
        parts.push_back(parse_ident());
    }
    SourceSpan span = parts.front().span;
    if (!parts.empty()) {
        span.end = parts.back().span.end;
    }
    return ModulePath(std::move(parts), span);
}

Ident Parser::parse_ident() {
    if (current_.kind != TokenKind::Ident) {
        diag_.error("expected identifier", current_.span);
        return Ident("<error>", current_.span);
    }
    Token tok = current_;
    advance();
    return Ident(tok.text, tok.span);
}

Attribute Parser::parse_attribute() {
    SourceSpan span = current_.span;
    expect(TokenKind::AttrStart, "expected attribute start");
    Ident name = parse_ident();
    expect(TokenKind::RBracket, "expected ']' after attribute");
    span.end = previous_.span.end;
    return Attribute(std::move(name), span);
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

DeclId Parser::parse_form_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwForm, "expected 'form'");
    Ident name = parse_ident();
    std::vector<FieldDecl> fields;
    while (current_.kind == TokenKind::KwField) {
        fields.push_back(parse_field_decl());
    }
    expect(TokenKind::Dot, "expected '.end'");
    if (!(current_.kind == TokenKind::Ident && current_.text == "end")) {
        diag_.error("expected 'end'", current_.span);
    } else {
        advance();
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<FormDecl>(std::move(name), std::move(fields), span);
}

DeclId Parser::parse_pick_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwPick, "expected 'pick'");
    Ident name = parse_ident();
    std::vector<CaseDecl> cases;
    while (current_.kind == TokenKind::KwCase) {
        cases.push_back(parse_case_decl());
    }
    expect(TokenKind::Dot, "expected '.end'");
    if (!(current_.kind == TokenKind::Ident && current_.text == "end")) {
        diag_.error("expected 'end'", current_.span);
    } else {
        advance();
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<PickDecl>(std::move(name), std::move(cases), span);
}

DeclId Parser::parse_proc_decl(std::vector<Attribute> attrs) {
    SourceSpan span = current_.span;
    expect(TokenKind::KwProc, "expected 'proc'");
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
    return ast_ctx_.make<ProcDecl>(
        std::move(attrs),
        std::move(name),
        std::move(params),
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
    expect(TokenKind::RBrace, "expected '}'");
    span.end = previous_.span.end;
    return ast_ctx_.make<BlockStmt>(std::move(stmts), span);
}

StmtId Parser::parse_stmt() {
    switch (current_.kind) {
        case TokenKind::KwMake: return parse_make_stmt();
        case TokenKind::KwSet: return parse_set_stmt();
        case TokenKind::KwGive: return parse_give_stmt();
        case TokenKind::KwEmit: return parse_emit_stmt();
        case TokenKind::KwIf: return parse_if_stmt();
        case TokenKind::KwSelect: return parse_select_stmt();
        case TokenKind::KwReturn: return parse_return_stmt();
        default: return parse_expr_stmt();
    }
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
    if (match(TokenKind::KwOtherwise)) {
        else_block = parse_block();
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<IfStmt>(
        cond,
        then_block,
        else_block,
        span);
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
        case TokenKind::KwOr:
            return 1;
        case TokenKind::KwAnd:
            return 2;
        case TokenKind::EqEq:
        case TokenKind::NotEq:
            return 3;
        case TokenKind::Lt:
        case TokenKind::Le:
        case TokenKind::Gt:
        case TokenKind::Ge:
            return 4;
        case TokenKind::Plus:
        case TokenKind::Minus:
            return 5;
        case TokenKind::Star:
        case TokenKind::Slash:
            return 6;
        default:
            return 0;
    }
}

ExprId Parser::parse_expr() {
    std::function<ExprId(int)> parse_prec = [&](int min_prec) -> ExprId {
        auto lhs = parse_unary_expr();
        if (lhs == ast::kInvalidAstId) {
            return ast::kInvalidAstId;
        }

        while (is_binary_op(current_.kind) &&
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
}

ExprId Parser::parse_unary_expr() {
    if (match(TokenKind::KwNot)) {
        auto rhs = parse_unary_expr();
        SourceSpan span = previous_.span;
        if (rhs != ast::kInvalidAstId) {
            span.end = ast_ctx_.get<Expr>(rhs).span.end;
        }
        return ast_ctx_.make<UnaryExpr>(UnaryOp::Not, rhs, span);
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

    if (current_.kind == TokenKind::StringLit) {
        std::string value = current_.text;
        advance();
        return ast_ctx_.make<LiteralExpr>(LiteralKind::String, value, span);
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
        return expr;
    }

    if (current_.kind == TokenKind::KwBool ||
        current_.kind == TokenKind::KwString ||
        current_.kind == TokenKind::KwInt) {
        TypeId ty = parse_type_expr();
        if (!match(TokenKind::LParen)) {
            diag_.error("expected constructor call", current_.span);
            return ast::kInvalidAstId;
        }
        std::vector<ExprId> args;
        if (current_.kind != TokenKind::RParen) {
            args = parse_arg_list();
        }
        expect(TokenKind::RParen, "expected ')'");
        SourceSpan ctor_span = span;
        ctor_span.end = previous_.span.end;
        return ast_ctx_.make<InvokeExpr>(
            ast::kInvalidAstId,
            ty,
            std::move(args),
            ctor_span);
    }

    if (current_.kind == TokenKind::Ident) {
        Ident ident = parse_ident();

        if (current_.kind == TokenKind::LBracket) {
            TypeId ty = parse_type_expr_from_base(ident);
            if (!match(TokenKind::LParen)) {
                diag_.error("expected constructor call", current_.span);
                return ast::kInvalidAstId;
            }
            std::vector<ExprId> args;
            if (current_.kind != TokenKind::RParen) {
                args = parse_arg_list();
            }
            expect(TokenKind::RParen, "expected ')'");
            SourceSpan ctor_span = ident.span;
            ctor_span.end = previous_.span.end;
            return ast_ctx_.make<InvokeExpr>(
                ast::kInvalidAstId,
                ty,
                std::move(args),
                ctor_span);
        }

        if (current_.kind == TokenKind::LParen) {
            auto callee = ast_ctx_.make<IdentExpr>(ident, ident.span);
            return parse_call_expr(callee);
        }

        if (is_unary_start(current_.kind)) {
            ExprId arg = parse_unary_expr();
            SourceSpan call_span = ident.span;
            if (arg != ast::kInvalidAstId) {
                call_span.end = ast_ctx_.get<Expr>(arg).span.end;
            }
            return ast_ctx_.make<CallNoParenExpr>(
                std::move(ident),
                arg,
                call_span);
        }

        return ast_ctx_.make<IdentExpr>(std::move(ident), ident.span);
    }

    diag_.error("expected expression", current_.span);
    advance();
    return ast::kInvalidAstId;
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

    if (current_.kind == TokenKind::KwBool ||
        current_.kind == TokenKind::KwString ||
        current_.kind == TokenKind::KwInt) {
        TypeId ty = parse_type_expr();
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

    if (current_.kind == TokenKind::Ident) {
        Ident ident = parse_ident();
        if (current_.kind == TokenKind::LParen || current_.kind == TokenKind::LBracket) {
            TypeId ty = ast::kInvalidAstId;
            if (current_.kind == TokenKind::LBracket) {
                ty = parse_type_expr_from_base(ident);
            } else {
                ty = ast_ctx_.make<NamedType>(ident, ident.span);
            }
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
        return ast_ctx_.make<IdentPattern>(std::move(ident), span);
    }

    diag_.error("expected pattern", current_.span);
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
    SourceSpan span = current_.span;
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
        diag_.error("expected type", current_.span);
        return ast::kInvalidAstId;
    }

    Ident base = parse_ident();
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
    switch (kind) {
        case TokenKind::KwNot:
        case TokenKind::KwTrue:
        case TokenKind::KwFalse:
        case TokenKind::IntLit:
        case TokenKind::StringLit:
        case TokenKind::Ident:
        case TokenKind::LParen:
        case TokenKind::LBracket:
        case TokenKind::KwBool:
        case TokenKind::KwString:
        case TokenKind::KwInt:
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
        case TokenKind::EqEq: return BinaryOp::Eq;
        case TokenKind::NotEq: return BinaryOp::Ne;
        case TokenKind::Lt: return BinaryOp::Lt;
        case TokenKind::Le: return BinaryOp::Le;
        case TokenKind::Gt: return BinaryOp::Gt;
        case TokenKind::Ge: return BinaryOp::Ge;
        case TokenKind::KwAnd: return BinaryOp::And;
        case TokenKind::KwOr: return BinaryOp::Or;
        default:
            return BinaryOp::Add;
    }
}

} // namespace vitte::frontend::parser
