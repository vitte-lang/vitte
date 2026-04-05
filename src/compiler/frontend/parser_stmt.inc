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
            sync_to_stmt_boundary();
            if (current_.kind != TokenKind::Eof &&
                current_.kind != TokenKind::RBrace &&
                !is_stmt_sync_kind(current_.kind)) {
                advance();
            }
        }
    }
    bool closed = match(TokenKind::RBrace);
    if (!closed) {
        if (emit_parse_error(diag::DiagId::ExpectedRightBrace, current_.span)) {
            note_parse("block opened here", span);
            note_parse("parser will resume after '}'", current_.span);
        }
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<BlockStmt>(std::move(stmts), span);
}

StmtId Parser::parse_stmt() {
    trace("parse_stmt");
    if (strict_core_ && tokens::is_forbidden_in_core(current_.kind)) {
        emit_parse_error(diag::DiagId::CoreForbiddenStatementSyntax, current_.span);
        return ast::kInvalidAstId;
    }

    switch (current_.kind) {
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
        emit_parse_error(diag::DiagId::ExpectedAsmStringLiteral, current_.span);
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
        emit_parse_error(diag::DiagId::ExpectedMatchArm, current_.span);
        sync_to_match_arm_boundary();
        if (current_.kind != TokenKind::Eof &&
            current_.kind != TokenKind::RBrace &&
            current_.kind != TokenKind::KwCase &&
            current_.kind != TokenKind::KwElse &&
            current_.kind != TokenKind::KwOtherwise) {
            advance();
        }
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
