// ------------------------------------------------------------
// Top-level
// ------------------------------------------------------------

static bool is_triple_quoted_docstring(const Token& tok) {
    if (tok.kind != TokenKind::StringLit || tok.span.file == nullptr) {
        return false;
    }
    if (tok.span.end <= tok.span.start) {
        return false;
    }
    const std::string& content = tok.span.file->content;
    if (tok.span.end > content.size() || tok.span.start + 3 > content.size()) {
        return false;
    }
    if (tok.span.end - tok.span.start < 6) {
        return false;
    }
    std::string_view raw(content.data() + tok.span.start, tok.span.end - tok.span.start);
    return raw.starts_with("\"\"\"") && raw.ends_with("\"\"\"");
}

static Attribute make_docstring_attribute(const Token& tok) {
    std::vector<AttributeArg> args;
    args.emplace_back(AttributeArg::Kind::String, tok.text);
    return Attribute(Ident("doc", tok.span), std::move(args), tok.span);
}

static Attribute make_shorthand_attribute(const Token& tok, Ident name) {
    return Attribute(std::move(name), {}, tok.span);
}

static void emit_pull_deprecation(DiagnosticEngine& diag, const SourceSpan& span, bool syntax_strict) {
    diag::Diagnostic deprec(
        syntax_strict ? diag::Severity::Error : diag::Severity::Warning,
        syntax_strict ? "E1104" : "W1104",
        syntax_strict
            ? "legacy syntax: 'pull' was removed from the stable grammar; use 'use'"
            : "legacy syntax: 'pull' is deprecated; use 'use'",
        span
    );
    deprec.add_note("canonical import keyword is 'use'");
    deprec.add_fix("rewrite import keyword", "use", span);
    diag.emit(std::move(deprec));
}

static void emit_share_deprecation(DiagnosticEngine& diag, const SourceSpan& span, bool syntax_strict) {
    diag::Diagnostic deprec(
        syntax_strict ? diag::Severity::Error : diag::Severity::Warning,
        syntax_strict ? "E1105" : "W1105",
        syntax_strict
            ? "legacy syntax: 'share' was removed from the stable grammar"
            : "legacy syntax: 'share' is deprecated",
        span
    );
    deprec.add_note("canonical module surface uses explicit 'use ... as ...' imports and implicit exports");
    deprec.add_fix("remove share declaration", "", span);
    diag.emit(std::move(deprec));
}

DeclId Parser::parse_toplevel() {
    trace("parse_toplevel");
    if (strict_core_ && tokens::is_forbidden_in_core(current_.kind)) {
        emit_parse_error(diag::DiagId::CoreForbiddenTopLevelSyntax, current_.span);
        return ast::kInvalidAstId;
    }

    std::vector<Attribute> attrs;
    while (true) {
        if (current_.kind == TokenKind::AttrStart) {
            attrs.push_back(parse_attribute());
            continue;
        }
        if (current_.kind == TokenKind::AtSign) {
            Token at = current_;
            advance();
            attrs.push_back(make_shorthand_attribute(at, parse_ident()));
            continue;
        }
        if (is_triple_quoted_docstring(current_)) {
            Token doc = current_;
            advance();
            attrs.push_back(make_docstring_attribute(doc));
            continue;
        }
        break;
    }

    if (current_.kind == TokenKind::KwSpace) {
        return parse_space_decl();
    }
    if (current_.kind == TokenKind::KwFrom) {
        return parse_from_import_decl();
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
    if (!attrs.empty() && current_.kind != TokenKind::KwProc) {
        emit_parse_error(diag::DiagId::ExpectedProcAfterAttribute, current_.span);
        return ast::kInvalidAstId;
    }
    if (current_.kind == TokenKind::KwProc) {
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
        if (current_.kind == TokenKind::RParen) {
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

DeclId Parser::parse_from_import_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwFrom, "expected 'from'");
    ModulePath path = parse_module_path();
    expect(TokenKind::KwImport, "expected 'import'");

    auto make_import = [&](const Ident& name) -> DeclId {
        std::vector<Ident> joined(path.parts.begin(), path.parts.end());
        joined.push_back(name);
        SourceSpan item_span = name.span;
        if (path.span.is_valid()) {
            item_span.start = path.span.start;
        }
        ModulePath item_path(std::move(joined), path.relative_depth, item_span);
        return ast_ctx_.make<UseDecl>(std::move(item_path), std::nullopt, false, item_span);
    };

    std::vector<DeclId> decls;
    decls.push_back(make_import(parse_ident()));
    while (match(TokenKind::Comma)) {
        if (current_.kind == TokenKind::Eof || current_.kind == TokenKind::RBrace) {
            break;
        }
        decls.push_back(make_import(parse_ident()));
    }

    span.end = previous_.span.end;
    for (std::size_t i = 1; i < decls.size(); ++i) {
        pending_decls_.push_back(decls[i]);
    }
    return decls.front();
}

DeclId Parser::parse_pull_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwPull, "expected 'pull'");
    emit_pull_deprecation(diag_, previous_.span, syntax_strict_);
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
    emit_share_deprecation(diag_, previous_.span, syntax_strict_);
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
            if (current_.kind == TokenKind::RBrace) {
                break;
            }
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
        ExprId default_value = ast::kInvalidAstId;
        if (match(TokenKind::Equal)) {
            default_value = parse_expr();
        }
        params.emplace_back(std::move(param_name), param_type, default_value);
        while (match(TokenKind::Comma)) {
            if (current_.kind == TokenKind::RParen) {
                break;
            }
            Ident next_name = parse_ident();
            TypeId next_type = ast::kInvalidAstId;
            if (match(TokenKind::Colon)) {
                next_type = parse_type_expr();
            }
            ExprId next_default = ast::kInvalidAstId;
            if (match(TokenKind::Equal)) {
                next_default = parse_expr();
            }
            params.emplace_back(std::move(next_name), next_type, next_default);
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
                    if (current_.kind == TokenKind::RParen) {
                        break;
                    }
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
            if (current_.kind == TokenKind::RBracket) {
                break;
            }
            params.push_back(parse_ident());
        }
    }
    expect(TokenKind::RBracket, "expected ']'");
    return params;
}
