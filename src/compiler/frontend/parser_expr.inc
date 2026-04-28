// ------------------------------------------------------------
// Expressions
// ------------------------------------------------------------

static int precedence(TokenKind kind) {
    return tokens::binary_precedence(kind);
}

static void emit_symbolic_not_deprecation(DiagnosticEngine& diag, const SourceSpan& span, bool syntax_strict) {
    diag::Diagnostic deprec(
        syntax_strict ? diag::Severity::Error : diag::Severity::Warning,
        syntax_strict ? "E1106" : "W1106",
        syntax_strict
            ? "deprecated syntax: symbolic '!' is forbidden in --syntax-strict mode; use 'not'"
            : "deprecated syntax: symbolic '!' is deprecated; prefer 'not'",
        span
    );
    deprec.add_note("canonical boolean negation operator is 'not'");
    deprec.add_fix("rewrite symbolic negation", "not <expr>", span);
    diag.emit(std::move(deprec));
}

static void emit_symbolic_and_deprecation(DiagnosticEngine& diag, const SourceSpan& span, bool syntax_strict) {
    diag::Diagnostic deprec(
        syntax_strict ? diag::Severity::Error : diag::Severity::Warning,
        syntax_strict ? "E1107" : "W1107",
        syntax_strict
            ? "deprecated syntax: symbolic '&&' is forbidden in --syntax-strict mode; use 'and'"
            : "deprecated syntax: symbolic '&&' is deprecated; prefer 'and'",
        span
    );
    deprec.add_note("canonical boolean conjunction operator is 'and'");
    deprec.add_fix("rewrite symbolic conjunction", "and", span);
    diag.emit(std::move(deprec));
}

static void emit_symbolic_or_deprecation(DiagnosticEngine& diag, const SourceSpan& span, bool syntax_strict) {
    diag::Diagnostic deprec(
        syntax_strict ? diag::Severity::Error : diag::Severity::Warning,
        syntax_strict ? "E1108" : "W1108",
        syntax_strict
            ? "deprecated syntax: symbolic '||' is forbidden in --syntax-strict mode; use 'or'"
            : "deprecated syntax: symbolic '||' is deprecated; prefer 'or'",
        span
    );
    deprec.add_note("canonical boolean disjunction operator is 'or'");
    deprec.add_fix("rewrite symbolic disjunction", "or", span);
    diag.emit(std::move(deprec));
}

static bool is_assignment_op(TokenKind kind) {
    switch (kind) {
        case TokenKind::Equal:
        case TokenKind::PlusEqual:
        case TokenKind::MinusEqual:
        case TokenKind::StarEqual:
        case TokenKind::SlashEqual:
        case TokenKind::PercentEqual:
            return true;
        default:
            return false;
    }
}

static bool compound_assignment_binary_op(TokenKind kind, BinaryOp& out) {
    switch (kind) {
        case TokenKind::PlusEqual:
            out = BinaryOp::Add;
            return true;
        case TokenKind::MinusEqual:
            out = BinaryOp::Sub;
            return true;
        case TokenKind::StarEqual:
            out = BinaryOp::Mul;
            return true;
        case TokenKind::SlashEqual:
            out = BinaryOp::Div;
            return true;
        case TokenKind::PercentEqual:
            out = BinaryOp::Mod;
            return true;
        default:
            return false;
    }
}

static bool is_toplevel_sync_kind(TokenKind kind) {
    switch (kind) {
        case TokenKind::AtSign:
        case TokenKind::KwSpace:
        case TokenKind::KwFrom:
        case TokenKind::KwPackage:
        case TokenKind::KwComptime:
        case TokenKind::KwAsync:
        case TokenKind::KwForeign:
        case TokenKind::KwImpl:
        case TokenKind::KwUnion:
        case TokenKind::KwExtern:
        case TokenKind::KwPull:
        case TokenKind::KwUse:
        case TokenKind::KwExport:
        case TokenKind::KwShare:
        case TokenKind::KwConst:
        case TokenKind::KwStatic:
        case TokenKind::KwGlobal:
        case TokenKind::KwType:
        case TokenKind::KwForm:
        case TokenKind::KwClass:
        case TokenKind::KwTrait:
        case TokenKind::KwFlags:
        case TokenKind::KwBench:
        case TokenKind::KwDef:
        case TokenKind::KwStaticAssert:
        case TokenKind::KwWhere:
        case TokenKind::KwRequires:
        case TokenKind::KwTry:
        case TokenKind::KwExcept:
        case TokenKind::KwFinally:
        case TokenKind::KwRaise:
        case TokenKind::KwDefer:
        case TokenKind::KwWith:
        case TokenKind::KwCritical:
        case TokenKind::KwAtomic:
        case TokenKind::KwVolatile:
        case TokenKind::KwGoto:
        case TokenKind::KwPreempt:
        case TokenKind::KwIrq:
        case TokenKind::KwPick:
        case TokenKind::KwProc:
        case TokenKind::KwEntry:
        case TokenKind::AttrStart:
        case TokenKind::Eof:
            return true;
        default:
            return false;
    }
}

static bool is_stmt_sync_kind(TokenKind kind) {
    switch (kind) {
        case TokenKind::RBrace:
        case TokenKind::KwCase:
        case TokenKind::KwElse:
        case TokenKind::KwOtherwise:
        case TokenKind::KwLet:
        case TokenKind::KwMake:
        case TokenKind::KwSet:
        case TokenKind::KwGive:
        case TokenKind::KwEmit:
        case TokenKind::KwAsm:
        case TokenKind::KwUnsafe:
        case TokenKind::KwAssert:
        case TokenKind::KwPanic:
        case TokenKind::KwUnreachable:
        case TokenKind::KwDefer:
        case TokenKind::KwWith:
        case TokenKind::KwCritical:
        case TokenKind::KwAtomic:
        case TokenKind::KwVolatile:
        case TokenKind::KwGoto:
        case TokenKind::KwPreempt:
        case TokenKind::KwIrq:
        case TokenKind::KwIf:
        case TokenKind::KwElif:
        case TokenKind::KwTry:
        case TokenKind::KwExcept:
        case TokenKind::KwFinally:
        case TokenKind::KwRaise:
        case TokenKind::KwLoop:
        case TokenKind::KwWhile:
        case TokenKind::KwFor:
        case TokenKind::KwBreak:
        case TokenKind::KwContinue:
        case TokenKind::KwSelect:
        case TokenKind::KwMatch:
        case TokenKind::KwWhen:
        case TokenKind::KwReturn:
        case TokenKind::KwOn:
        case TokenKind::KwOff:
        case TokenKind::KwEnable:
        case TokenKind::KwDisable:
        case TokenKind::Eof:
            return true;
        default:
            return is_toplevel_sync_kind(kind);
    }
}

static bool is_match_arm_sync_kind(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwCase:
        case TokenKind::KwElse:
        case TokenKind::KwOtherwise:
        case TokenKind::KwOn:
        case TokenKind::KwOff:
        case TokenKind::KwEnable:
        case TokenKind::KwDisable:
        case TokenKind::RBrace:
        case TokenKind::Eof:
            return true;
        default:
            return false;
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
    return tokens::keyword_text(kind);
}

static bool is_ctor_body_start(TokenKind kind);
static bool is_builtin_expr_start(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwSizeof:
        case TokenKind::KwAlignof:
        case TokenKind::KwOffsetof:
        case TokenKind::KwTypeof:
        case TokenKind::KwNameof:
            return true;
        default:
            return false;
    }
}

ExprId Parser::parse_expr() {
    trace("parse_expr");
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
            SourceSpan op_span = current_.span;
            advance();
            int next_min_prec = is_assignment_op(op) ? prec : (prec + 1);
            auto rhs = parse_prec(next_min_prec);
            if (rhs == ast::kInvalidAstId) {
                return lhs;
            }
            if (op == TokenKind::AmpAmp) {
                emit_symbolic_and_deprecation(diag_, op_span, syntax_strict_);
            } else if (op == TokenKind::PipePipe) {
                emit_symbolic_or_deprecation(diag_, op_span, syntax_strict_);
            }
            SourceSpan span = ast_ctx_.get<Expr>(lhs).span;
            span.end = ast_ctx_.get<Expr>(rhs).span.end;
            BinaryOp compound_op = BinaryOp::Add;
            if (compound_assignment_binary_op(op, compound_op)) {
                SourceSpan add_span = ast_ctx_.get<Expr>(lhs).span;
                add_span.end = ast_ctx_.get<Expr>(rhs).span.end;
                ExprId add_expr = ast_ctx_.make<BinaryExpr>(
                    compound_op,
                    lhs,
                    rhs,
                    add_span);
                lhs = ast_ctx_.make<BinaryExpr>(
                    BinaryOp::Assign,
                    lhs,
                    add_expr,
                    span);
                continue;
            }
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
    if (match(TokenKind::Bang)) {
        emit_symbolic_not_deprecation(diag_, previous_.span, syntax_strict_);
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
    if (match(TokenKind::Tilde)) {
        auto rhs = parse_unary_expr();
        SourceSpan span = previous_.span;
        if (rhs != ast::kInvalidAstId) {
            span.end = ast_ctx_.get<Expr>(rhs).span.end;
        }
        return ast_ctx_.make<UnaryExpr>(UnaryOp::BitNot, rhs, span);
    }
    if (match(TokenKind::KwAwait)) {
        auto rhs = parse_unary_expr();
        SourceSpan span = previous_.span;
        if (rhs != ast::kInvalidAstId) {
            span.end = ast_ctx_.get<Expr>(rhs).span.end;
        }
        return ast_ctx_.make<UnaryExpr>(UnaryOp::Await, rhs, span);
    }
    return parse_primary();
}

ExprId Parser::parse_primary() {
    SourceSpan span = current_.span;

    if (strict_core_ && (current_.kind == TokenKind::KwIf || current_.kind == TokenKind::KwProc)) {
        emit_parse_error(diag::DiagId::CoreForbiddenExpressionSyntax, current_.span);
        advance();
        return ast::kInvalidAstId;
    }

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
        if (value.find("${") == std::string::npos) {
            return ast_ctx_.make<LiteralExpr>(LiteralKind::String, value, span);
        }
        return parse_interpolated_string_expr(value, span);
    }

    if (current_.kind == TokenKind::BytesLit) {
        std::string value = current_.text;
        advance();
        return ast_ctx_.make<LiteralExpr>(LiteralKind::Bytes, value, span);
    }

    if (current_.kind == TokenKind::LBracket) {
        return parse_list_literal_or_comp_expr();
    }

    if (current_.kind == TokenKind::LBrace) {
        return parse_brace_literal_or_comp_expr();
    }

    if (current_.kind == TokenKind::LParen) {
        advance();
        ExprId expr = parse_expr();
        expect(TokenKind::RParen, "expected ')'");
        return parse_postfix_expr(expr);
    }

    if (is_builtin_expr_start(current_.kind)) {
        TokenKind builtin = current_.kind;
        advance();
        expect(TokenKind::LParen, "expected '(' after builtin expression");
        BuiltinExpr::Kind kind = BuiltinExpr::Kind::Sizeof;
        TypeId type = ast::kInvalidAstId;
        ExprId expr = ast::kInvalidAstId;
        ModulePath path(std::vector<Ident>{}, 0, span);
        Ident member("", span);
        switch (builtin) {
            case TokenKind::KwSizeof: {
                kind = BuiltinExpr::Kind::Sizeof;
                if (current_.kind != TokenKind::RParen) {
                    State try_type = snapshot();
                    TypeId maybe_type = parse_type_expr();
                    if (maybe_type != ast::kInvalidAstId && is_unambiguous_type_expr(maybe_type) && match(TokenKind::RParen)) {
                        type = maybe_type;
                        span.end = previous_.span.end;
                        return ast_ctx_.make<BuiltinExpr>(kind, type, expr, std::move(path), std::move(member), span);
                    }
                    restore(std::move(try_type));
                    expr = parse_expr();
                }
                expect(TokenKind::RParen, "expected ')' after sizeof");
                span.end = previous_.span.end;
                return ast_ctx_.make<BuiltinExpr>(kind, type, expr, std::move(path), std::move(member), span);
            }
            case TokenKind::KwAlignof: {
                kind = BuiltinExpr::Kind::Alignof;
                type = parse_type_expr();
                expect(TokenKind::RParen, "expected ')' after alignof");
                span.end = previous_.span.end;
                return ast_ctx_.make<BuiltinExpr>(kind, type, expr, std::move(path), std::move(member), span);
            }
            case TokenKind::KwOffsetof: {
                kind = BuiltinExpr::Kind::Offsetof;
                type = parse_type_expr();
                expect(TokenKind::Comma, "expected ',' after offsetof type");
                member = parse_ident();
                expect(TokenKind::RParen, "expected ')' after offsetof");
                span.end = previous_.span.end;
                return ast_ctx_.make<BuiltinExpr>(kind, type, expr, std::move(path), std::move(member), span);
            }
            case TokenKind::KwTypeof: {
                kind = BuiltinExpr::Kind::Typeof;
                expr = parse_expr();
                expect(TokenKind::RParen, "expected ')' after typeof");
                span.end = previous_.span.end;
                return ast_ctx_.make<BuiltinExpr>(kind, type, expr, std::move(path), std::move(member), span);
            }
            case TokenKind::KwNameof: {
                kind = BuiltinExpr::Kind::Nameof;
                path = parse_module_path();
                expect(TokenKind::RParen, "expected ')' after nameof");
                span.end = previous_.span.end;
                return ast_ctx_.make<BuiltinExpr>(kind, type, expr, std::move(path), std::move(member), span);
            }
            default:
                break;
        }
    }

    if (current_.kind == TokenKind::Ident) {
        Ident ident = parse_ident();
        auto base = ast_ctx_.make<IdentExpr>(std::move(ident), ident.span);
        return parse_postfix_expr(base);
    }

    if (emit_parse_error(diag::DiagId::ExpectedExpression, current_.span)) {
        note_parse("expected expression (e.g. 1, name, call(), { ... })", current_.span);
        note_parse("parser will resume after the next token", current_.span);
    }
    advance();
    return ast::kInvalidAstId;
}

ExprId Parser::parse_postfix_expr(ExprId base) {
    ExprId expr = base;
    while (true) {
        if (match(TokenKind::Dot)) {
            if (current_.kind == TokenKind::KwAwait) {
                Token await_tok = current_;
                advance();
                SourceSpan sp = ast_ctx_.get<Expr>(expr).span;
                sp.end = await_tok.span.end;
                expr = ast_ctx_.make<UnaryExpr>(UnaryOp::Await, expr, sp);
                continue;
            }
            Ident member = parse_ident();
            SourceSpan span = ast_ctx_.get<Expr>(expr).span;
            span.end = member.span.end;
            expr = ast_ctx_.make<MemberExpr>(expr, std::move(member), span);
            continue;
        }
        if (current_.kind == TokenKind::LBrace && looks_like_type_constructor_head(expr)) {
            State ctor_state = snapshot();
            advance();
            bool allow_ctor = is_ctor_body_start(current_.kind);
            restore(std::move(ctor_state));
            if (!allow_ctor) {
                break;
            }
            const auto& head = ast_ctx_.get<Expr>(expr);
            TypeId ctor_type = ast::kInvalidAstId;
            if (head.kind == NodeKind::IdentExpr) {
                const auto& ident = static_cast<const IdentExpr&>(head).ident;
                ctor_type = ast_ctx_.make<NamedType>(Ident(ident.name, ident.span), ident.span);
            } else if (head.kind == NodeKind::MemberExpr) {
                const auto& member = static_cast<const MemberExpr&>(head);
                if (member.base != ast::kInvalidAstId) {
                    const auto& base_node = ast_ctx_.get<Expr>(member.base);
                    if (base_node.kind == NodeKind::IdentExpr) {
                        const auto& base = static_cast<const IdentExpr&>(base_node).ident;
                        std::string qualified = base.name + "." + member.member.name;
                        ctor_type = ast_ctx_.make<NamedType>(Ident(std::move(qualified), head.span), head.span);
                    }
                }
            }
            if (ctor_type == ast::kInvalidAstId) {
                break;
            }
            SourceSpan span = ast_ctx_.get<Expr>(expr).span;
            expect(TokenKind::LBrace, "expected '{'");
            std::vector<InvokeArg> args;
            if (current_.kind != TokenKind::RBrace) {
                args = parse_call_arg_list();
            }
            expect(TokenKind::RBrace, "expected '}'");
            span.end = previous_.span.end;
            expr = ast_ctx_.make<InvokeExpr>(
                ast::kInvalidAstId,
                ctor_type,
                std::vector<TypeId>{},
                std::move(args),
                span);
            continue;
        }
        if (current_.kind == TokenKind::LParen) {
            expr = parse_call_expr(expr);
            continue;
        }
        if (current_.kind == TokenKind::LBracket && looks_like_type_constructor_head(expr)) {
            State generic_state = snapshot();
            advance();
            if (looks_like_type_expr_start(current_.kind)) {
                if (current_.kind != TokenKind::RBracket) {
                    parse_type_expr();
                    while (match(TokenKind::Comma)) {
                        if (current_.kind == TokenKind::RBracket) {
                            break;
                        }
                        parse_type_expr();
                    }
                }
                if (match(TokenKind::RBracket)) {
                    if (current_.kind == TokenKind::LParen) {
                        expr = parse_call_expr(expr);
                    }
                    continue;
                }
            }
            restore(std::move(generic_state));
        }
        if (match(TokenKind::LBracket)) {
            ExprId first = ast::kInvalidAstId;
            ExprId second = ast::kInvalidAstId;
            ExprId third = ast::kInvalidAstId;
            bool is_slice = false;
            if (current_.kind != TokenKind::Colon && current_.kind != TokenKind::RBracket) {
                first = parse_expr();
            }
            if (match(TokenKind::Colon)) {
                is_slice = true;
                if (current_.kind != TokenKind::Colon && current_.kind != TokenKind::RBracket) {
                    second = parse_expr();
                }
                if (match(TokenKind::Colon)) {
                    if (current_.kind != TokenKind::RBracket) {
                        third = parse_expr();
                    }
                }
            }
            expect(TokenKind::RBracket, "expected ']'");
            SourceSpan span = ast_ctx_.get<Expr>(expr).span;
            span.end = previous_.span.end;
            if (is_slice) {
                std::vector<ExprId> parts;
                parts.push_back(first);
                parts.push_back(second);
                if (third != ast::kInvalidAstId || current_.kind != TokenKind::RBracket) {
                    parts.push_back(third);
                }
                auto slice_idx = ast_ctx_.make<ListExpr>(std::move(parts), span);
                expr = ast_ctx_.make<IndexExpr>(expr, slice_idx, span);
            } else {
                expr = ast_ctx_.make<IndexExpr>(expr, first, span);
            }
            continue;
        }
        break;
    }
    return expr;
}

bool Parser::try_recover_empty_generic_call_suffix(ExprId& expr) {
    if (expr == ast::kInvalidAstId || current_.kind != TokenKind::LBracket) {
        return false;
    }

    State state = snapshot();
    SourceSpan span = current_.span;
    match(TokenKind::LBracket);
    if (current_.kind != TokenKind::RBracket) {
        restore(std::move(state));
        return false;
    }
    span.end = current_.span.end;
    advance();
    if (current_.kind != TokenKind::LParen) {
        restore(std::move(state));
        return false;
    }

    emit_parse_error(diag::DiagId::GenericTypeRequiresAtLeastOneTypeArgument, span);
    expr = parse_call_expr(expr);
    return true;
}

bool Parser::looks_like_type_constructor_head(ExprId expr) const {
    if (expr == ast::kInvalidAstId) {
        return false;
    }
    const auto& node = ast_ctx_.get<Expr>(expr);
    if (node.kind == NodeKind::IdentExpr) {
        const auto& ident = static_cast<const IdentExpr&>(node).ident.name;
        if (ident.empty()) {
            return false;
        }
        const char c = ident.front();
        return c >= 'A' && c <= 'Z';
    }
    if (node.kind == NodeKind::MemberExpr) {
        const auto& member = static_cast<const MemberExpr&>(node);
        if (member.base == ast::kInvalidAstId) {
            return false;
        }
        const auto& base_node = ast_ctx_.get<Expr>(member.base);
        if (base_node.kind != NodeKind::IdentExpr) {
            return false;
        }
        const auto& member_name = member.member.name;
        if (member_name.empty()) {
            return false;
        }
        const char c = member_name.front();
        return c >= 'A' && c <= 'Z';
    }
    return false;
}

static bool is_ctor_body_start(TokenKind kind) {
    switch (kind) {
        case TokenKind::Eof:
        case TokenKind::RBrace:
        case TokenKind::KwAsm:
        case TokenKind::KwUnsafe:
        case TokenKind::KwLet:
        case TokenKind::KwMake:
        case TokenKind::KwSet:
        case TokenKind::KwGive:
        case TokenKind::KwEmit:
        case TokenKind::KwIf:
        case TokenKind::KwElif:
        case TokenKind::KwElse:
        case TokenKind::KwOtherwise:
        case TokenKind::KwLoop:
        case TokenKind::KwWhile:
        case TokenKind::KwFor:
        case TokenKind::KwBreak:
        case TokenKind::KwContinue:
        case TokenKind::KwReturn:
        case TokenKind::KwTry:
        case TokenKind::KwRaise:
        case TokenKind::KwSelect:
        case TokenKind::KwMatch:
        case TokenKind::KwWhen:
            return false;
        default:
            return true;
    }
}

bool Parser::looks_like_type_expr_start(TokenKind kind) const {
    switch (kind) {
        case TokenKind::Ident:
        case TokenKind::Star:
        case TokenKind::Amp:
        case TokenKind::Question:
        case TokenKind::LBracket:
        case TokenKind::LParen:
        case TokenKind::KwProc:
        case TokenKind::KwDyn:
        case TokenKind::KwImpl:
        case TokenKind::KwConst:
        case TokenKind::KwVolatile:
        case TokenKind::KwAtomic:
        case TokenKind::KwMut:
        case TokenKind::KwOwned:
        case TokenKind::KwBorrow:
        case TokenKind::KwUser:
        case TokenKind::KwKernel:
        case TokenKind::KwPhys:
        case TokenKind::KwMmio:
        case TokenKind::KwDma:
        case TokenKind::KwBool:
        case TokenKind::KwString:
        case TokenKind::KwInt:
        case TokenKind::KwBytes:
        case TokenKind::KwChar:
        case TokenKind::KwVoid:
        case TokenKind::KwNever:
        case TokenKind::KwCStr:
        case TokenKind::KwCChar:
        case TokenKind::KwCInt:
        case TokenKind::KwCUInt:
        case TokenKind::KwCLong:
        case TokenKind::KwCULong:
        case TokenKind::KwCVoid:
        case TokenKind::KwI8:
        case TokenKind::KwI16:
        case TokenKind::KwU8:
        case TokenKind::KwU16:
        case TokenKind::KwIsize:
        case TokenKind::KwUsize:
        case TokenKind::KwIntptr:
        case TokenKind::KwUintptr:
        case TokenKind::KwF16:
        case TokenKind::KwF32:
        case TokenKind::KwF64:
        case TokenKind::KwF128:
        case TokenKind::KwStr:
        case TokenKind::KwUnit:
        case TokenKind::KwRune:
        case TokenKind::KwNull:
            return true;
        default:
            return false;
    }
}

bool Parser::is_unambiguous_type_expr(TypeId type) const {
    if (type == ast::kInvalidAstId) {
        return false;
    }
    const auto& node = ast_ctx_.get<TypeNode>(type);
    switch (node.kind) {
        case NodeKind::BuiltinType:
            return true;
        case NodeKind::NamedType: {
            const auto& t = static_cast<const NamedType&>(node);
            const auto& name = t.ident.name;
            if (name.find('.') != std::string::npos) {
                return true;
            }
            if (name.empty()) {
                return false;
            }
            const char c = name.front();
            return c >= 'A' && c <= 'Z';
        }
        case NodeKind::GenericType: {
            const auto& t = static_cast<const GenericType&>(node);
            const auto& base = t.base_ident.name;
            bool base_ok = false;
            if (base == "tuple" || base == "dyn" || base == "impl" || base == "union") {
                base_ok = true;
            } else if (base.find('.') != std::string::npos) {
                base_ok = true;
            } else if (!base.empty()) {
                const char c = base.front();
                base_ok = (c >= 'A' && c <= 'Z');
            }
            if (!base_ok) {
                return false;
            }
            for (auto arg : t.type_args) {
                if (!is_unambiguous_type_expr(arg)) {
                    return false;
                }
            }
            return true;
        }
        case NodeKind::PointerType: {
            const auto& t = static_cast<const PointerType&>(node);
            return is_unambiguous_type_expr(t.pointee);
        }
        case NodeKind::SliceType: {
            const auto& t = static_cast<const SliceType&>(node);
            return is_unambiguous_type_expr(t.element);
        }
        case NodeKind::ProcType: {
            const auto& t = static_cast<const ProcType&>(node);
            for (auto p : t.params) {
                if (!is_unambiguous_type_expr(p)) {
                    return false;
                }
            }
            return t.return_type == ast::kInvalidAstId || is_unambiguous_type_expr(t.return_type);
        }
        default:
            return false;
    }
}

ExprId Parser::try_parse_generic_ctor_call_expr(ExprId base) {
    if (base == ast::kInvalidAstId || current_.kind != TokenKind::LBracket) {
        return ast::kInvalidAstId;
    }
    const auto& node = ast_ctx_.get<Expr>(base);
    Ident ctor_ident("", node.span);
    SourceSpan ctor_span = node.span;
    if (node.kind == NodeKind::IdentExpr) {
        ctor_ident = static_cast<const IdentExpr&>(node).ident;
    } else if (node.kind == NodeKind::MemberExpr) {
        const auto& member = static_cast<const MemberExpr&>(node);
        if (member.base == ast::kInvalidAstId) {
            return ast::kInvalidAstId;
        }
        const auto& base_node = ast_ctx_.get<Expr>(member.base);
        if (base_node.kind != NodeKind::IdentExpr) {
            return ast::kInvalidAstId;
        }
        const auto& base_ident = static_cast<const IdentExpr&>(base_node).ident;
        ctor_ident = Ident(base_ident.name + "." + member.member.name, ctor_span);
    } else {
        return ast::kInvalidAstId;
    }

    State state = snapshot();
    SourceSpan span = node.span;
    match(TokenKind::LBracket);
    if (!looks_like_type_expr_start(current_.kind)) {
        restore(std::move(state));
        return ast::kInvalidAstId;
    }

    std::vector<TypeId> type_args;
    if (current_.kind != TokenKind::RBracket) {
        type_args.push_back(parse_type_expr());
        while (match(TokenKind::Comma)) {
            if (current_.kind == TokenKind::RBracket) {
                break;
            }
            type_args.push_back(parse_type_expr());
        }
    }
    if (!expect(TokenKind::RBracket, "expected ']'")) {
        restore(std::move(state));
        return ast::kInvalidAstId;
    }
    if (current_.kind != TokenKind::LParen) {
        restore(std::move(state));
        return ast::kInvalidAstId;
    }

    SourceSpan type_span = ctor_span;
    type_span.end = previous_.span.end;
    TypeId callee_type = ast_ctx_.make<GenericType>(
        std::move(ctor_ident),
        std::move(type_args),
        type_span
    );

    expect(TokenKind::LParen, "expected '('");
    std::vector<InvokeArg> args;
    if (current_.kind != TokenKind::RParen) {
        args = parse_call_arg_list();
    }
    expect(TokenKind::RParen, "expected ')'");
    span.end = previous_.span.end;
    return ast_ctx_.make<InvokeExpr>(
        ast::kInvalidAstId,
        callee_type,
        std::vector<TypeId>{},
        std::move(args),
        span
    );
}

ExprId Parser::try_parse_generic_proc_call_expr(ExprId base) {
    if (base == ast::kInvalidAstId || current_.kind != TokenKind::LBracket) {
        return ast::kInvalidAstId;
    }

    const auto& node = ast_ctx_.get<Expr>(base);
    if (node.kind != NodeKind::IdentExpr && node.kind != NodeKind::MemberExpr) {
        return ast::kInvalidAstId;
    }

    State state = snapshot();
    match(TokenKind::LBracket);
    if (!looks_like_type_expr_start(current_.kind)) {
        restore(std::move(state));
        return ast::kInvalidAstId;
    }

    std::vector<TypeId> type_args;
    if (current_.kind != TokenKind::RBracket) {
        type_args.push_back(parse_type_expr());
        while (match(TokenKind::Comma)) {
            type_args.push_back(parse_type_expr());
        }
    }
    if (!expect(TokenKind::RBracket, "expected ']'")) {
        restore(std::move(state));
        return ast::kInvalidAstId;
    }
    if (current_.kind != TokenKind::LParen) {
        restore(std::move(state));
        return ast::kInvalidAstId;
    }
    if (type_args.empty()) {
        restore(std::move(state));
        return ast::kInvalidAstId;
    }
    for (auto arg : type_args) {
        if (!is_unambiguous_type_expr(arg)) {
            restore(std::move(state));
            return ast::kInvalidAstId;
        }
    }

    SourceSpan span = node.span;
    expect(TokenKind::LParen, "expected '('");
    std::vector<InvokeArg> args;
    if (current_.kind != TokenKind::RParen) {
        args = parse_call_arg_list();
    }
    expect(TokenKind::RParen, "expected ')'");
    span.end = previous_.span.end;
    return ast_ctx_.make<InvokeExpr>(
        base,
        ast::kInvalidAstId,
        std::move(type_args),
        std::move(args),
        span
    );
}

ExprId Parser::parse_call_expr(ExprId callee) {
    SourceSpan span = callee != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(callee).span
        : current_.span;
    expect(TokenKind::LParen, "expected '('");
    std::vector<InvokeArg> args;
    if (current_.kind != TokenKind::RParen) {
        args = parse_call_arg_list();
    }
    expect(TokenKind::RParen, "expected ')'");
    span.end = previous_.span.end;
    return ast_ctx_.make<InvokeExpr>(
        callee,
        ast::kInvalidAstId,
        std::vector<TypeId>{},
        std::move(args),
        span);
}

ExprId Parser::parse_proc_expr() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwProc, "expected 'proc'");
    expect(TokenKind::LParen, "expected '('");
    std::vector<FnParam> params;
    if (current_.kind != TokenKind::RParen) {
        params.emplace_back(parse_fn_param());
        while (match(TokenKind::Comma)) {
            if (current_.kind == TokenKind::RParen) {
                break;
            }
            params.emplace_back(parse_fn_param());
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
    if (!match(TokenKind::KwIf)) {
        expect(TokenKind::KwElif, "expected 'if'");
    }
    ExprId cond = parse_expr();
    StmtId then_block = parse_block();
    StmtId else_block = ast::kInvalidAstId;
    auto wrap_if_branch = [&](ExprId nested_if) -> StmtId {
        SourceSpan expr_span = nested_if != ast::kInvalidAstId
            ? ast_ctx_.get<Expr>(nested_if).span
            : current_.span;
        StmtId expr_stmt = ast_ctx_.make<ExprStmt>(nested_if, expr_span);
        std::vector<StmtId> stmts;
        stmts.push_back(expr_stmt);
        return ast_ctx_.make<BlockStmt>(std::move(stmts), expr_span);
    };
    auto parse_else_branch = [&]() -> StmtId {
        if (current_.kind == TokenKind::KwIf || current_.kind == TokenKind::KwElif) {
            return wrap_if_branch(parse_if_expr());
        }
        return parse_block();
    };
    if (current_.kind == TokenKind::KwElif) {
        else_block = wrap_if_branch(parse_if_expr());
    } else if (match(TokenKind::KwElse) || match(TokenKind::KwOtherwise)) {
        else_block = parse_else_branch();
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<IfExpr>(cond, then_block, else_block, span);
}

ExprId Parser::parse_list_literal_or_comp_expr() {
    SourceSpan span = current_.span;
    expect(TokenKind::LBracket, "expected '['");
    if (current_.kind == TokenKind::RBracket) {
        advance();
        span.end = previous_.span.end;
        return ast_ctx_.make<ListExpr>(std::vector<ExprId>{}, span);
    }

    ExprId first = parse_expr();
    if (current_.kind == TokenKind::KwFor) {
        advance();
        std::optional<Ident> index_ident;
        Ident ident = parse_ident();
        if (match(TokenKind::Comma)) {
            index_ident = std::move(ident);
            ident = parse_ident();
        }
        expect(TokenKind::KwIn, "expected 'in'");
        ExprId iterable = parse_expr();
        ExprId condition = ast::kInvalidAstId;
        if (match(TokenKind::KwIf)) {
            condition = parse_expr();
        }
        expect(TokenKind::RBracket, "expected ']'");
        span.end = previous_.span.end;
        return ast_ctx_.make<ListCompExpr>(
            ListCompExpr::Kind::List,
            ast::kInvalidAstId,
            first,
            std::move(index_ident),
            std::move(ident),
            iterable,
            condition,
            span);
    }

    std::vector<ExprId> items;
    items.push_back(first);
    while (match(TokenKind::Comma)) {
        if (current_.kind == TokenKind::RBracket) {
            break;
        }
        items.push_back(parse_expr());
    }
    expect(TokenKind::RBracket, "expected ']'");
    span.end = previous_.span.end;
    return ast_ctx_.make<ListExpr>(std::move(items), span);
}

ExprId Parser::parse_brace_literal_or_comp_expr() {
    SourceSpan span = current_.span;
    expect(TokenKind::LBrace, "expected '{'");
    if (current_.kind == TokenKind::RBrace) {
        advance();
        span.end = previous_.span.end;
        return ast_ctx_.make<ListExpr>(std::vector<ExprId>{}, span);
    }

    ExprId first = parse_expr();
    if (current_.kind == TokenKind::Colon) {
        advance();
        ExprId value = parse_expr();
        if (current_.kind == TokenKind::KwFor) {
            advance();
            std::optional<Ident> index_ident;
            Ident ident = parse_ident();
            if (match(TokenKind::Comma)) {
                index_ident = std::move(ident);
                ident = parse_ident();
            }
            expect(TokenKind::KwIn, "expected 'in'");
            ExprId iterable = parse_expr();
            ExprId condition = ast::kInvalidAstId;
            if (match(TokenKind::KwIf)) {
                condition = parse_expr();
            }
            expect(TokenKind::RBrace, "expected '}'");
            span.end = previous_.span.end;
            return ast_ctx_.make<ListCompExpr>(
                ListCompExpr::Kind::Dict,
                first,
                value,
                std::move(index_ident),
                std::move(ident),
                iterable,
                condition,
                span);
        }
        std::vector<ExprId> items;
        items.push_back(first);
        items.push_back(value);
        while (match(TokenKind::Comma)) {
            if (current_.kind == TokenKind::RBrace) {
                break;
            }
            items.push_back(parse_expr());
            if (!match(TokenKind::Colon)) {
                break;
            }
            items.push_back(parse_expr());
        }
        expect(TokenKind::RBrace, "expected '}'");
        span.end = previous_.span.end;
        return ast_ctx_.make<ListExpr>(std::move(items), span);
    }
    if (current_.kind == TokenKind::KwFor) {
        advance();
        std::optional<Ident> index_ident;
        Ident ident = parse_ident();
        if (match(TokenKind::Comma)) {
            index_ident = std::move(ident);
            ident = parse_ident();
        }
        expect(TokenKind::KwIn, "expected 'in'");
        ExprId iterable = parse_expr();
        ExprId condition = ast::kInvalidAstId;
        if (match(TokenKind::KwIf)) {
            condition = parse_expr();
        }
        expect(TokenKind::RBrace, "expected '}'");
        span.end = previous_.span.end;
        return ast_ctx_.make<ListCompExpr>(
            ListCompExpr::Kind::Set,
            ast::kInvalidAstId,
            first,
            std::move(index_ident),
            std::move(ident),
            iterable,
            condition,
            span);
    }

    std::vector<ExprId> items;
    items.push_back(first);
    while (match(TokenKind::Comma)) {
        if (current_.kind == TokenKind::RBrace) {
            break;
        }
        items.push_back(parse_expr());
    }
    expect(TokenKind::RBrace, "expected '}'");
    span.end = previous_.span.end;
    return ast_ctx_.make<ListExpr>(std::move(items), span);
}

std::vector<ExprId> Parser::parse_arg_list() {
    std::vector<ExprId> args;
    args.push_back(parse_expr());
    while (match(TokenKind::Comma)) {
        if (current_.kind == TokenKind::RParen || current_.kind == TokenKind::RBracket || current_.kind == TokenKind::RBrace) {
            break;
        }
        args.push_back(parse_expr());
    }
    return args;
}

std::vector<InvokeArg> Parser::parse_call_arg_list() {
    std::vector<InvokeArg> args;
    while (true) {
        if (current_.kind == TokenKind::Ident) {
            State state = snapshot();
            Ident name = parse_ident();
            if (match(TokenKind::Colon)) {
                args.emplace_back(std::move(name), parse_expr());
            } else {
                restore(std::move(state));
                args.emplace_back(parse_expr());
            }
        } else {
            args.emplace_back(parse_expr());
        }
        if (!match(TokenKind::Comma)) {
            break;
        }
        if (current_.kind == TokenKind::RParen || current_.kind == TokenKind::RBracket || current_.kind == TokenKind::RBrace) {
            break;
        }
    }
    return args;
}

ExprId Parser::parse_interpolation_path_expr(std::string_view raw, SourceSpan span) {
    auto valid_start = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    };
    auto valid_continue = [&](char c) {
        return valid_start(c) || (c >= '0' && c <= '9');
    };
    auto trim = [](std::string_view in) {
        std::size_t b = 0;
        while (b < in.size() && (in[b] == ' ' || in[b] == '\t' || in[b] == '\n' || in[b] == '\r')) {
            ++b;
        }
        std::size_t e = in.size();
        while (e > b && (in[e - 1] == ' ' || in[e - 1] == '\t' || in[e - 1] == '\n' || in[e - 1] == '\r')) {
            --e;
        }
        return in.substr(b, e - b);
    };

    std::string_view body = trim(raw);
    if (body.empty()) {
        emit_parse_error(diag::DiagId::ExpectedExpression, span);
        note_parse("empty interpolation; use ${name}", span);
        return ast::kInvalidAstId;
    }

    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        std::size_t dot = body.find('.', start);
        std::string_view part = body.substr(start, dot == std::string_view::npos ? body.size() - start : dot - start);
        if (part.empty() || !valid_start(part[0])) {
            emit_parse_error(diag::DiagId::ExpectedExpression, span);
            note_parse("interpolation supports identifier/member paths like ${x} or ${user.name}", span);
            return ast::kInvalidAstId;
        }
        for (char c : part) {
            if (!valid_continue(c)) {
                emit_parse_error(diag::DiagId::ExpectedExpression, span);
                note_parse("invalid interpolation token; expected identifier/member path", span);
                return ast::kInvalidAstId;
            }
        }
        parts.emplace_back(part);
        if (dot == std::string_view::npos) {
            break;
        }
        start = dot + 1;
    }

    Ident base(parts[0], span);
    ExprId expr = ast_ctx_.make<IdentExpr>(base, span);
    for (std::size_t i = 1; i < parts.size(); ++i) {
        expr = ast_ctx_.make<MemberExpr>(expr, Ident(parts[i], span), span);
    }
    return expr;
}

ExprId Parser::parse_interpolated_string_expr(const std::string& value, SourceSpan span) {
    std::vector<ExprId> pieces;
    std::size_t i = 0;
    while (i < value.size()) {
        std::size_t open = value.find("${", i);
        if (open == std::string::npos) {
            std::string tail = value.substr(i);
            if (!tail.empty()) {
                pieces.push_back(ast_ctx_.make<LiteralExpr>(LiteralKind::String, tail, span));
            }
            break;
        }

        if (open > i) {
            std::string lit = value.substr(i, open - i);
            if (!lit.empty()) {
                pieces.push_back(ast_ctx_.make<LiteralExpr>(LiteralKind::String, lit, span));
            }
        }

        std::size_t close = value.find('}', open + 2);
        if (close == std::string::npos) {
            emit_parse_error(diag::DiagId::ExpectedToken, span);
            note_parse("unterminated interpolation; add '}'", span);
            return ast_ctx_.make<LiteralExpr>(LiteralKind::String, value, span);
        }
        std::string_view raw = std::string_view(value).substr(open + 2, close - (open + 2));
        ExprId expr = parse_interpolation_path_expr(raw, span);
        if (expr == ast::kInvalidAstId) {
            return ast_ctx_.make<LiteralExpr>(LiteralKind::String, value, span);
        }
        pieces.push_back(expr);
        i = close + 1;
    }

    if (pieces.empty()) {
        return ast_ctx_.make<LiteralExpr>(LiteralKind::String, "", span);
    }

    ExprId out = pieces[0];
    for (std::size_t k = 1; k < pieces.size(); ++k) {
        SourceSpan add_span = ast_ctx_.get<Expr>(out).span;
        add_span.end = ast_ctx_.get<Expr>(pieces[k]).span.end;
        out = ast_ctx_.make<BinaryExpr>(BinaryOp::Add, out, pieces[k], add_span);
    }
    return out;
}
