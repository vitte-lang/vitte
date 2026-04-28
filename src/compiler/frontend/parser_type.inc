// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

TypeId Parser::parse_type_expr() {
    TypeId ty = parse_type_primary();
    if (current_.kind == TokenKind::Pipe) {
        std::vector<TypeId> items;
        items.push_back(ty);
        while (match(TokenKind::Pipe)) {
            items.push_back(parse_type_primary());
        }
        SourceSpan span = ast_ctx_.get<TypeNode>(items.front()).span;
        if (!items.empty()) {
            span.end = ast_ctx_.get<TypeNode>(items.back()).span.end;
        }
        return ast_ctx_.make<GenericType>(Ident("union", span), std::move(items), span);
    }
    return ty;
}

TypeId Parser::parse_type_primary() {
    SourceSpan span = current_.span;
    if (current_.kind == TokenKind::KwConst || current_.kind == TokenKind::KwVolatile ||
        current_.kind == TokenKind::KwAtomic || current_.kind == TokenKind::KwMut ||
        current_.kind == TokenKind::KwOwned || current_.kind == TokenKind::KwBorrow ||
        current_.kind == TokenKind::KwUser || current_.kind == TokenKind::KwKernel ||
        current_.kind == TokenKind::KwPhys || current_.kind == TokenKind::KwMmio ||
        current_.kind == TokenKind::KwDma) {
        advance();
        return parse_type_expr();
    }
    if (match(TokenKind::Star)) {
        TypeId inner = parse_type_expr();
        span.end = inner != ast::kInvalidAstId
            ? ast_ctx_.get<TypeNode>(inner).span.end
            : previous_.span.end;
        return ast_ctx_.make<PointerType>(inner, span);
    }
    if (match(TokenKind::Amp)) {
        if (current_.kind == TokenKind::KwMut) {
            advance();
        }
        TypeId inner = parse_type_expr();
        span.end = inner != ast::kInvalidAstId
            ? ast_ctx_.get<TypeNode>(inner).span.end
            : previous_.span.end;
        return ast_ctx_.make<PointerType>(inner, span);
    }
    if (match(TokenKind::Question)) {
        TypeId inner = parse_type_expr();
        span.end = inner != ast::kInvalidAstId
            ? ast_ctx_.get<TypeNode>(inner).span.end
            : previous_.span.end;
        std::vector<TypeId> args;
        args.push_back(inner);
        return ast_ctx_.make<GenericType>(Ident("Option", span), std::move(args), span);
    }
    if (match(TokenKind::LBracket)) {
        TypeId elem = parse_type_expr();
        if (match(TokenKind::Semicolon)) {
            (void)parse_expr();
            expect(TokenKind::RBracket, "expected ']'");
            span.end = previous_.span.end;
            return ast_ctx_.make<SliceType>(elem, span);
        }
        expect(TokenKind::RBracket, "expected ']'");
        span.end = previous_.span.end;
        return ast_ctx_.make<SliceType>(elem, span);
    }
    if (match(TokenKind::LParen)) {
        std::vector<TypeId> items;
        if (current_.kind != TokenKind::RParen) {
            items.push_back(parse_type_expr());
            while (match(TokenKind::Comma)) {
                if (current_.kind == TokenKind::RParen) {
                    break;
                }
                items.push_back(parse_type_expr());
            }
        }
        expect(TokenKind::RParen, "expected ')'");
        if (items.size() == 1) {
            return items.front();
        }
        span.end = previous_.span.end;
        return ast_ctx_.make<GenericType>(Ident("tuple", span), std::move(items), span);
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
    if (current_.kind == TokenKind::KwDyn) {
        advance();
        std::vector<TypeId> args;
        args.push_back(parse_type_expr());
        while (match(TokenKind::Plus)) {
            args.push_back(parse_type_expr());
        }
        TypeId inner = args.front();
        span.end = inner != ast::kInvalidAstId
            ? ast_ctx_.get<TypeNode>(inner).span.end
            : previous_.span.end;
        return ast_ctx_.make<GenericType>(Ident("dyn", span), std::move(args), span);
    }
    if (current_.kind == TokenKind::KwImpl) {
        advance();
        std::vector<TypeId> args;
        args.push_back(parse_type_expr());
        while (match(TokenKind::Plus)) {
            args.push_back(parse_type_expr());
        }
        TypeId inner = args.front();
        span.end = inner != ast::kInvalidAstId
            ? ast_ctx_.get<TypeNode>(inner).span.end
            : previous_.span.end;
        return ast_ctx_.make<GenericType>(Ident("impl", span), std::move(args), span);
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
    if (current_.kind == TokenKind::KwBytes) {
        advance();
        return ast_ctx_.make<BuiltinType>("bytes", span);
    }
    if (current_.kind == TokenKind::KwChar) {
        advance();
        return ast_ctx_.make<BuiltinType>("char", span);
    }
    if (current_.kind == TokenKind::KwVoid) {
        advance();
        return ast_ctx_.make<BuiltinType>("void", span);
    }
    if (current_.kind == TokenKind::KwNever) {
        advance();
        return ast_ctx_.make<BuiltinType>("never", span);
    }
    if (current_.kind == TokenKind::KwCChar) {
        advance();
        return ast_ctx_.make<BuiltinType>("c_char", span);
    }
    if (current_.kind == TokenKind::KwCInt) {
        advance();
        return ast_ctx_.make<BuiltinType>("c_int", span);
    }
    if (current_.kind == TokenKind::KwCUInt) {
        advance();
        return ast_ctx_.make<BuiltinType>("c_uint", span);
    }
    if (current_.kind == TokenKind::KwCLong) {
        advance();
        return ast_ctx_.make<BuiltinType>("c_long", span);
    }
    if (current_.kind == TokenKind::KwCULong) {
        advance();
        return ast_ctx_.make<BuiltinType>("c_ulong", span);
    }
    if (current_.kind == TokenKind::KwCVoid) {
        advance();
        return ast_ctx_.make<BuiltinType>("c_void", span);
    }
    if (current_.kind == TokenKind::KwCStr) {
        advance();
        return ast_ctx_.make<BuiltinType>("cstr", span);
    }
    if (current_.kind == TokenKind::KwI8) {
        advance();
        return ast_ctx_.make<BuiltinType>("i8", span);
    }
    if (current_.kind == TokenKind::KwI16) {
        advance();
        return ast_ctx_.make<BuiltinType>("i16", span);
    }
    if (current_.kind == TokenKind::KwU8) {
        advance();
        return ast_ctx_.make<BuiltinType>("u8", span);
    }
    if (current_.kind == TokenKind::KwU16) {
        advance();
        return ast_ctx_.make<BuiltinType>("u16", span);
    }
    if (current_.kind == TokenKind::KwIsize) {
        advance();
        return ast_ctx_.make<BuiltinType>("isize", span);
    }
    if (current_.kind == TokenKind::KwUsize) {
        advance();
        return ast_ctx_.make<BuiltinType>("usize", span);
    }
    if (current_.kind == TokenKind::KwIntptr) {
        advance();
        return ast_ctx_.make<BuiltinType>("intptr", span);
    }
    if (current_.kind == TokenKind::KwUintptr) {
        advance();
        return ast_ctx_.make<BuiltinType>("uintptr", span);
    }
    if (current_.kind == TokenKind::KwF16) {
        advance();
        return ast_ctx_.make<BuiltinType>("f16", span);
    }
    if (current_.kind == TokenKind::KwF32) {
        advance();
        return ast_ctx_.make<BuiltinType>("f32", span);
    }
    if (current_.kind == TokenKind::KwF64) {
        advance();
        return ast_ctx_.make<BuiltinType>("f64", span);
    }
    if (current_.kind == TokenKind::KwF128) {
        advance();
        return ast_ctx_.make<BuiltinType>("f128", span);
    }
    if (current_.kind == TokenKind::KwStr) {
        advance();
        return ast_ctx_.make<BuiltinType>("str", span);
    }
    if (current_.kind == TokenKind::KwUnit) {
        advance();
        return ast_ctx_.make<BuiltinType>("unit", span);
    }
    if (current_.kind == TokenKind::KwRune) {
        advance();
        return ast_ctx_.make<BuiltinType>("rune", span);
    }
    if (current_.kind == TokenKind::KwNull) {
        advance();
        return ast_ctx_.make<BuiltinType>("null", span);
    }

    if (current_.kind != TokenKind::Ident) {
        if (emit_parse_error(diag::DiagId::ExpectedType, current_.span)) {
            note_parse("expected type (e.g. int, bytes, char, void, never, string, bool, c_int, cstr, Option[T])", current_.span);
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
