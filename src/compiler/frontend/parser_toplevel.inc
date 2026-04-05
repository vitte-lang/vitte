// ------------------------------------------------------------
// Top-level
// ------------------------------------------------------------

DeclId Parser::parse_toplevel() {
    trace("parse_toplevel");
    if (strict_core_ && tokens::is_forbidden_in_core(current_.kind)) {
        emit_parse_error(diag::DiagId::CoreForbiddenTopLevelSyntax, current_.span);
        return ast::kInvalidAstId;
    }

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
            emit_parse_error(diag::DiagId::ExpectedProcAfterAttribute, current_.span);
            return ast::kInvalidAstId;
        }
        return parse_proc_decl(std::move(attrs));
    }
    if (current_.kind == TokenKind::KwEntry) {
        return parse_entry_decl();
    }

    emit_parse_error(diag::DiagId::ExpectedTopLevelDeclaration, current_.span);
    emit_toplevel_hint(diag_, current_);
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
        emit_parse_error(diag::DiagId::ExpectedIdentifier, current_.span);
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
            emit_parse_error(diag::DiagId::ExpectedAttributeArgument, current_.span);
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

DeclId Parser::parse_global_decl() {
    SourceSpan span = current_.span;
    bool is_mut = false;
    if (current_.kind == TokenKind::KwLet) {
        advance();
    } else {
        expect(TokenKind::KwMake, "expected 'make'");
        is_mut = true;
    }
    Ident name = parse_ident();
    TypeId ty = ast::kInvalidAstId;
    if (match(TokenKind::Colon)) {
        ty = parse_type_expr();
    } else if (match(TokenKind::KwAs)) {
        ty = parse_type_expr();
    }
    expect(TokenKind::Equal, "expected '='");
    ExprId value = parse_expr();
    span.end = value != ast::kInvalidAstId
        ? ast_ctx_.get<Expr>(value).span.end
        : previous_.span.end;
    return ast_ctx_.make<GlobalDecl>(std::move(name), ty, value, is_mut, span);
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
    expect(TokenKind::KwForm, "expected 'form'");
    Ident name = parse_ident();
    auto type_params = parse_type_params();
    std::vector<FieldDecl> fields;
    expect(TokenKind::LBrace, "expected '{'");
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
    return ast_ctx_.make<FormDecl>(std::move(name), std::move(type_params), std::move(fields), span);
}

DeclId Parser::parse_pick_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwPick, "expected 'pick'");
    Ident name = parse_ident();
    auto type_params = parse_type_params();
    std::vector<CaseDecl> cases;
    expect(TokenKind::LBrace, "expected '{'");
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        if (match(TokenKind::KwCase)) {
            // optional in core brace form
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
