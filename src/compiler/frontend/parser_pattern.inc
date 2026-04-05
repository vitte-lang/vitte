// ------------------------------------------------------------
// Patterns
// ------------------------------------------------------------

PatternId Parser::parse_pattern() {
    SourceSpan span = current_.span;

    if (current_.kind == TokenKind::Ident) {
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

    emit_parse_error(diag::DiagId::ExpectedPattern, current_.span);
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
