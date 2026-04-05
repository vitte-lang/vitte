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
        if (emit_parse_error(diag::DiagId::ExpectedType, current_.span)) {
            note_parse("expected type (e.g. int, i32, i64, i128, u32, u64, u128, string, bool, Option[T])", current_.span);
            note_parse("parser will resume after the next token", current_.span);
        }
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
