// ------------------------------------------------------------
// Blocks / statements
// ------------------------------------------------------------

static void emit_removed_loop_syntax(DiagnosticEngine& diag, const SourceSpan& span) {
    diag::Diagnostic deprec(
        diag::Severity::Warning,
        "W1101",
        "legacy syntax: 'loop' is deprecated; use 'while true'",
        span
    );
    deprec.add_note("canonical loop form is 'while true { ... }'");
    deprec.add_fix("rewrite loop to canonical while form", "while true { ... }", span);
    diag.emit(std::move(deprec));
}

static void emit_removed_otherwise_syntax(DiagnosticEngine& diag, const SourceSpan& span) {
    diag::Diagnostic deprec(
        diag::Severity::Warning,
        "W1102",
        "legacy syntax: 'otherwise' is deprecated; use 'else'",
        span
    );
    deprec.add_note("canonical fallback keyword is 'else'");
    deprec.add_fix("rewrite fallback keyword", "else", span);
    diag.emit(std::move(deprec));
}

static void emit_removed_return_syntax(DiagnosticEngine& diag, const SourceSpan& span, bool has_expr) {
    diag::Diagnostic deprec(
        diag::Severity::Warning,
        "W1103",
        "legacy syntax: 'return' is deprecated; use 'give'",
        span
    );
    deprec.add_note("canonical return keyword is 'give'");
    deprec.add_fix("rewrite return keyword", has_expr ? "give <expr>" : "give", span);
    diag.emit(std::move(deprec));
}

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
        case TokenKind::KwAsm: return parse_asm_stmt();
        case TokenKind::KwUnsafe: return parse_unsafe_stmt();
        case TokenKind::KwLet: return parse_let_stmt();
        case TokenKind::KwMake: return parse_make_stmt();
        case TokenKind::KwSet: return parse_set_stmt();
        case TokenKind::KwGive: return parse_give_stmt();
        case TokenKind::KwEmit: return parse_emit_stmt();
        case TokenKind::KwIf: return parse_if_stmt();
        case TokenKind::KwLoop: return parse_loop_stmt();
        case TokenKind::KwWhile: return parse_while_stmt();
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
    bool is_mut = match(TokenKind::KwMut);
    if (current_.kind == TokenKind::Ident) {
        Ident first = parse_ident();
        std::vector<Ident> names;
        names.push_back(std::move(first));
        while (match(TokenKind::Comma)) {
            if (current_.kind != TokenKind::Ident) {
                emit_parse_error(diag::DiagId::ExpectedIdentifier, current_.span);
                break;
            }
            names.push_back(parse_ident());
        }
        if (names.size() > 1) {
            TypeId ty = ast::kInvalidAstId;
            if (match(TokenKind::Colon)) {
                ty = parse_type_expr();
            }
            expect(TokenKind::Equal, "expected '='");
            ExprId value = parse_expr();
            span.end = value != ast::kInvalidAstId
                ? ast_ctx_.get<Expr>(value).span.end
                : previous_.span.end;
            std::vector<LetBinding> bindings;
            bindings.reserve(names.size());
            for (auto& n : names) {
                bindings.emplace_back(std::move(n));
            }
            return ast_ctx_.make<LetStmt>(
                std::move(bindings),
                ty,
                value,
                is_mut,
                span);
        }
        Ident name = std::move(names.front());
        TypeId ty = ast::kInvalidAstId;
        if (match(TokenKind::Colon)) {
            ty = parse_type_expr();
        }
        expect(TokenKind::Equal, "expected '='");
        ExprId value = parse_expr();
        span.end = value != ast::kInvalidAstId
            ? ast_ctx_.get<Expr>(value).span.end
            : previous_.span.end;
        if (is_mut) {
            return ast_ctx_.make<MakeStmt>(std::move(name), ty, value, span);
        }
        return ast_ctx_.make<LetStmt>(std::move(name), ty, value, span);
    }
    if (current_.kind == TokenKind::LBracket) {
        expect(TokenKind::LBracket, "expected '[' after 'let'");
        std::function<std::optional<LetBinding>()> parse_binding = [&]() -> std::optional<LetBinding> {
            if (current_.kind == TokenKind::Ident) {
                return LetBinding(parse_ident());
            }
            if (current_.kind == TokenKind::LBracket) {
                expect(TokenKind::LBracket, "expected '['");
                std::vector<LetBinding> children;
                if (current_.kind != TokenKind::RBracket) {
                    while (true) {
                        auto child = parse_binding();
                        if (child.has_value()) {
                            children.push_back(std::move(*child));
                        }
                        if (!match(TokenKind::Comma)) {
                            break;
                        }
                        if (current_.kind == TokenKind::RBracket) {
                            break;
                        }
                    }
                }
                expect(TokenKind::RBracket, "expected ']'");
                return LetBinding(std::move(children), span);
            }
            if (current_.kind == TokenKind::Ident && current_.text == "_") {
                return LetBinding(parse_ident());
            }
            emit_parse_error(diag::DiagId::ExpectedIdentifier, current_.span);
            return std::nullopt;
        };
        std::vector<LetBinding> bindings;
        if (current_.kind != TokenKind::RBracket) {
            while (true) {
                auto binding = parse_binding();
                if (binding.has_value()) {
                    bindings.push_back(std::move(*binding));
                }
                if (!match(TokenKind::Comma)) {
                    break;
                }
                if (current_.kind == TokenKind::RBracket) {
                    break;
                }
            }
        }
        expect(TokenKind::RBracket, "expected ']'");
        TypeId ty = ast::kInvalidAstId;
        if (match(TokenKind::Colon)) {
            ty = parse_type_expr();
        }
        expect(TokenKind::Equal, "expected '='");
        ExprId value = parse_expr();
        span.end = value != ast::kInvalidAstId
            ? ast_ctx_.get<Expr>(value).span.end
            : previous_.span.end;
        return ast_ctx_.make<LetStmt>(
            std::move(bindings),
            ty,
            value,
            is_mut,
            span);
    }
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
    if (is_mut) {
        return ast_ctx_.make<MakeStmt>(std::move(name), ty, value, span);
    }
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
    ExprId target = parse_primary();
    std::vector<ExprId> target_items;
    bool has_multi_target = false;
    if (match(TokenKind::Comma)) {
        has_multi_target = true;
        target_items.push_back(target);
        while (true) {
            target_items.push_back(parse_primary());
            if (!match(TokenKind::Comma)) {
                break;
            }
        }
    }
    TokenKind assign_op = TokenKind::Equal;
    if (match(TokenKind::PlusEqual)) {
        assign_op = TokenKind::PlusEqual;
    } else if (match(TokenKind::MinusEqual)) {
        assign_op = TokenKind::MinusEqual;
    } else if (match(TokenKind::StarEqual)) {
        assign_op = TokenKind::StarEqual;
    } else if (match(TokenKind::SlashEqual)) {
        assign_op = TokenKind::SlashEqual;
    } else if (match(TokenKind::PercentEqual)) {
        assign_op = TokenKind::PercentEqual;
    } else {
        expect(TokenKind::Equal, "expected '='");
    }
    ExprId value = parse_expr();
    if (has_multi_target) {
        std::vector<ExprId> values;
        values.push_back(value);
        while (match(TokenKind::Comma)) {
            values.push_back(parse_expr());
        }
        ExprId target_list = ast_ctx_.make<ListExpr>(std::move(target_items), ast_ctx_.get<Expr>(target_items.front()).span);
        ExprId value_expr = value;
        if (values.size() > 1) {
            value_expr = ast_ctx_.make<ListExpr>(std::move(values), ast_ctx_.get<Expr>(values.front()).span);
        }
        span.end = ast_ctx_.get<Expr>(value_expr).span.end;
        return ast_ctx_.make<SetStmt>(target_list, value_expr, span);
    }
    BinaryOp compound_op = BinaryOp::Add;
    bool has_compound = false;
    switch (assign_op) {
        case TokenKind::PlusEqual:
            compound_op = BinaryOp::Add;
            has_compound = true;
            break;
        case TokenKind::MinusEqual:
            compound_op = BinaryOp::Sub;
            has_compound = true;
            break;
        case TokenKind::StarEqual:
            compound_op = BinaryOp::Mul;
            has_compound = true;
            break;
        case TokenKind::SlashEqual:
            compound_op = BinaryOp::Div;
            has_compound = true;
            break;
        case TokenKind::PercentEqual:
            compound_op = BinaryOp::Mod;
            has_compound = true;
            break;
        default:
            break;
    }
    auto build_value = [&](ExprId lhs, ExprId rhs, SourceSpan value_span) -> ExprId {
        if (!has_compound) {
            return rhs;
        }
        return ast_ctx_.make<BinaryExpr>(compound_op, lhs, rhs, value_span);
    };
    auto make_slice_index = [&](ExprId start_expr, ExprId step_expr, std::size_t offset, SourceSpan value_span) -> ExprId {
        ExprId start_value = start_expr;
        if (start_value == ast::kInvalidAstId) {
            start_value = ast_ctx_.make<LiteralExpr>(LiteralKind::Int, "0", value_span);
        }
        ExprId step_value = step_expr;
        if (step_value == ast::kInvalidAstId) {
            step_value = ast_ctx_.make<LiteralExpr>(LiteralKind::Int, "1", value_span);
        }
        ExprId offset_expr = ast_ctx_.make<LiteralExpr>(LiteralKind::Int, std::to_string(offset), value_span);
        ExprId scaled_offset = ast_ctx_.make<BinaryExpr>(BinaryOp::Mul, step_value, offset_expr, value_span);
        return ast_ctx_.make<BinaryExpr>(BinaryOp::Add, start_value, scaled_offset, value_span);
    };
    std::function<StmtId(ExprId, ExprId, SourceSpan)> emit_target =
        [&](ExprId t, ExprId v, SourceSpan tspan) -> StmtId {
            if (t == ast::kInvalidAstId) {
                return ast::kInvalidAstId;
            }
            const auto& tnode = ast_ctx_.node(t);
            if (tnode.kind == NodeKind::ListExpr) {
                const auto& target_list = ast_ctx_.get<ListExpr>(t);
                if (v == ast::kInvalidAstId || ast_ctx_.node(v).kind != NodeKind::ListExpr) {
                    return ast_ctx_.make<SetStmt>(t, v, tspan);
                }
                const auto& value_list = ast_ctx_.get<ListExpr>(v);
                std::vector<StmtId> stmts;
                std::size_t count = std::min(target_list.items.size(), value_list.items.size());
                for (std::size_t i = 0; i < count; ++i) {
                    ExprId item = target_list.items[i];
                    ExprId rhs_item = value_list.items[i];
                    SourceSpan item_span = item != ast::kInvalidAstId
                        ? ast_ctx_.get<Expr>(item).span
                        : tspan;
                    stmts.push_back(emit_target(item, rhs_item, item_span));
                }
                return ast_ctx_.make<BlockStmt>(std::move(stmts), tspan);
            }
            if (tnode.kind == NodeKind::IndexExpr) {
                const auto& idx = ast_ctx_.get<IndexExpr>(t);
                if (idx.index != ast::kInvalidAstId && ast_ctx_.node(idx.index).kind == NodeKind::ListExpr) {
                    if (v == ast::kInvalidAstId || ast_ctx_.node(v).kind != NodeKind::ListExpr) {
                        return ast_ctx_.make<SetStmt>(t, v, tspan);
                    }
                    const auto& slice_idx = ast_ctx_.get<ListExpr>(idx.index);
                    const auto& value_list = ast_ctx_.get<ListExpr>(v);
                    ExprId start_expr = ast::kInvalidAstId;
                    ExprId step_expr = ast::kInvalidAstId;
                    if (!slice_idx.items.empty()) {
                        start_expr = slice_idx.items[0];
                    }
                    if (slice_idx.items.size() >= 3) {
                        step_expr = slice_idx.items[2];
                    }
                    std::vector<StmtId> stmts;
                    for (std::size_t i = 0; i < value_list.items.size(); ++i) {
                        SourceSpan item_span = value_list.items[i] != ast::kInvalidAstId
                            ? ast_ctx_.get<Expr>(value_list.items[i]).span
                            : tspan;
                        ExprId index_expr = make_slice_index(start_expr, step_expr, i, item_span);
                        ExprId item_target = ast_ctx_.make<IndexExpr>(idx.base, index_expr, item_span);
                        ExprId item_value = build_value(item_target, value_list.items[i], item_span);
                        stmts.push_back(ast_ctx_.make<SetStmt>(item_target, item_value, item_span));
                    }
                    return ast_ctx_.make<BlockStmt>(std::move(stmts), tspan);
                }
            }
            ExprId final_value = build_value(t, v, tspan);
            return ast_ctx_.make<SetStmt>(t, final_value, tspan);
        };
    if (target != ast::kInvalidAstId && ast_ctx_.node(target).kind == NodeKind::ListExpr) {
        if (value != ast::kInvalidAstId && ast_ctx_.node(value).kind == NodeKind::ListExpr) {
            std::vector<StmtId> stmts;
            const auto& target_list = ast_ctx_.get<ListExpr>(target);
            const auto& value_list = ast_ctx_.get<ListExpr>(value);
            std::size_t count = std::min(target_list.items.size(), value_list.items.size());
            for (std::size_t i = 0; i < count; ++i) {
                ExprId item = target_list.items[i];
                ExprId rhs_item = value_list.items[i];
                SourceSpan item_span = item != ast::kInvalidAstId
                    ? ast_ctx_.get<Expr>(item).span
                    : span;
                stmts.push_back(emit_target(item, rhs_item, item_span));
            }
            span.end = value != ast::kInvalidAstId
                ? ast_ctx_.get<Expr>(value).span.end
                : previous_.span.end;
            return ast_ctx_.make<BlockStmt>(std::move(stmts), span);
        }
        return emit_target(target, value, span);
    }
    if (target != ast::kInvalidAstId && ast_ctx_.node(target).kind == NodeKind::IndexExpr) {
        const auto& idx = ast_ctx_.get<IndexExpr>(target);
        if (idx.index != ast::kInvalidAstId && ast_ctx_.node(idx.index).kind == NodeKind::ListExpr) {
            return emit_target(target, value, span);
        }
    }
    if (has_compound) {
        SourceSpan target_span = target != ast::kInvalidAstId
            ? ast_ctx_.get<Expr>(target).span
            : span;
        SourceSpan add_span = target_span;
        if (value != ast::kInvalidAstId) {
            add_span.end = ast_ctx_.get<Expr>(value).span.end;
        }
        value = ast_ctx_.make<BinaryExpr>(compound_op, target, value, add_span);
    }
    span.end = value != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(value).span.end
        : previous_.span.end;
    return ast_ctx_.make<SetStmt>(
        target,
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
    if (match(TokenKind::KwElse)) {
        if (current_.kind == TokenKind::KwIf) {
            auto nested_if = parse_if_stmt();
            std::vector<StmtId> stmts;
            stmts.push_back(nested_if);
            SourceSpan block_span = ast_ctx_.get<Stmt>(nested_if).span;
            else_block = ast_ctx_.make<BlockStmt>(std::move(stmts), block_span);
        } else {
            else_block = parse_block();
        }
    } else if (match(TokenKind::KwOtherwise)) {
        emit_removed_otherwise_syntax(diag_, previous_.span);
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
    emit_removed_loop_syntax(diag_, previous_.span);
    StmtId body = parse_block();
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<LoopStmt>(body, span);
}

StmtId Parser::parse_while_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwWhile, "expected 'while'");
    ExprId cond = parse_expr();
    StmtId body = parse_block();

    const auto& body_block = ast_ctx_.get<BlockStmt>(body);
    SourceSpan cond_span = cond != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(cond).span
        : span;
    ExprId negated = ast_ctx_.make<UnaryExpr>(UnaryOp::Not, cond, cond_span);

    StmtId break_stmt = ast_ctx_.make<BreakStmt>(cond_span);
    std::vector<StmtId> break_stmts;
    break_stmts.push_back(break_stmt);
    StmtId break_block = ast_ctx_.make<BlockStmt>(std::move(break_stmts), cond_span);
    StmtId guard_if = ast_ctx_.make<IfStmt>(negated, break_block, ast::kInvalidAstId, cond_span);

    std::vector<StmtId> loop_stmts;
    loop_stmts.reserve(body_block.stmts.size() + 1);
    loop_stmts.push_back(guard_if);
    for (auto stmt_id : body_block.stmts) {
        loop_stmts.push_back(stmt_id);
    }

    StmtId loop_body = ast_ctx_.make<BlockStmt>(std::move(loop_stmts), body_block.span);
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<LoopStmt>(loop_body, span);
}

StmtId Parser::parse_for_stmt() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwFor, "expected 'for'");
    std::optional<Ident> index_ident;
    Ident name = parse_ident();
    if (match(TokenKind::Comma)) {
        index_ident = std::move(name);
        name = parse_ident();
    }
    expect(TokenKind::KwIn, "expected 'in'");
    ExprId iterable = parse_expr();
    StmtId body = parse_block();
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<ForStmt>(std::move(index_ident), std::move(name), iterable, body, span);
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
        emit_removed_otherwise_syntax(diag_, previous_.span);
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
            ExprId guard = ast::kInvalidAstId;
            if (match(TokenKind::KwIf)) {
                guard = parse_expr();
            }
            if (current_.kind != TokenKind::LBrace) {
                expect(TokenKind::LBrace, "expected '{' after match case pattern");
                sync_to_match_arm_boundary();
                continue;
            }
            StmtId block = parse_block();
            whens.push_back(ast_ctx_.make<WhenStmt>(
                pat,
                guard,
                block,
                ast_ctx_.get<Stmt>(block).span));
            continue;
        }
        if (match(TokenKind::KwOtherwise)) {
            emit_removed_otherwise_syntax(diag_, previous_.span);
            otherwise_block = parse_block();
            continue;
        }
        if (match(TokenKind::KwElse)) {
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
    ExprId guard = ast::kInvalidAstId;
    if (match(TokenKind::KwIf)) {
        guard = parse_expr();
    }
    StmtId block = parse_block();
    auto when_id = ast_ctx_.make<WhenStmt>(
        pat,
        guard,
        block,
        ast_ctx_.get<Stmt>(block).span);
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
    emit_removed_return_syntax(diag_, span, expr != ast::kInvalidAstId);
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
    ExprId guard = ast::kInvalidAstId;
    if (match(TokenKind::KwIf)) {
        guard = parse_expr();
    }
    StmtId block = parse_block();
    span.end = ast_ctx_.get<Stmt>(block).span.end;
    return ast_ctx_.make<WhenStmt>(pat, guard, block, span);
}
