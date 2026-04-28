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

static bool is_callconv_token(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwSysv64:
        case TokenKind::KwWin64:
        case TokenKind::KwInterrupt:
        case TokenKind::KwNaked:
            return true;
        default:
            return false;
    }
}

static bool is_proc_prefix_token(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwPub:
        case TokenKind::KwPriv:
        case TokenKind::KwInternal:
        case TokenKind::KwAsync:
        case TokenKind::KwUnsafe:
        case TokenKind::KwConst:
        case TokenKind::KwInline:
        case TokenKind::KwNoinline:
        case TokenKind::KwExtern:
            return true;
        default:
            return false;
    }
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

    auto looks_like_proc_decl = [&]() -> bool {
        State state = snapshot();
        while (true) {
            if (current_.kind == TokenKind::KwProc || current_.kind == TokenKind::KwDef) {
                restore(std::move(state));
                return true;
            }
            if (current_.kind == TokenKind::KwExtern) {
                advance();
                if (current_.kind == TokenKind::StringLit || current_.kind == TokenKind::Ident ||
                    is_callconv_token(current_.kind)) {
                    advance();
                }
                continue;
            }
            if (is_proc_prefix_token(current_.kind)) {
                advance();
                continue;
            }
            restore(std::move(state));
            return false;
        }
    };

    if (current_.kind == TokenKind::KwUnsafe) {
        State state = snapshot();
        advance();
        if (current_.kind == TokenKind::KwTrait) {
            restore(std::move(state));
            return parse_trait_decl(std::move(attrs));
        }
        restore(std::move(state));
    }

    if (current_.kind == TokenKind::KwSpace) {
        return parse_space_decl();
    }
    if (current_.kind == TokenKind::KwPackage) {
        return parse_package_decl();
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
    if (current_.kind == TokenKind::KwForeign) {
        advance();
        if (current_.kind == TokenKind::KwType) {
            return parse_foreign_type_decl();
        }
        if (current_.kind == TokenKind::KwProc || current_.kind == TokenKind::KwExtern ||
            current_.kind == TokenKind::StringLit || current_.kind == TokenKind::Ident ||
            is_callconv_token(current_.kind)) {
            return parse_foreign_proc_decl();
        }
        expect(TokenKind::KwType, "expected 'type' or 'proc' after 'foreign'");
        return ast::kInvalidAstId;
    }
    if (current_.kind == TokenKind::KwExport) {
        return parse_export_decl();
    }
    if (current_.kind == TokenKind::KwShare) {
        return parse_share_decl();
    }
    if (current_.kind == TokenKind::KwGlobal) {
        return parse_global_decl();
    }
    if (current_.kind == TokenKind::KwConst) {
        return parse_const_decl();
    }
    if (current_.kind == TokenKind::KwStatic) {
        return parse_static_decl();
    }
    if (current_.kind == TokenKind::KwType) {
        return parse_type_alias_decl();
    }
    if (current_.kind == TokenKind::KwForm) {
        return parse_form_decl();
    }
    if (current_.kind == TokenKind::KwClass) {
        return parse_class_decl();
    }
    if (current_.kind == TokenKind::KwUnion) {
        return parse_union_decl();
    }
    if (current_.kind == TokenKind::KwTrait) {
        return parse_trait_decl(std::move(attrs));
    }
    if (current_.kind == TokenKind::KwImpl) {
        return parse_impl_decl(std::move(attrs));
    }
    bool proc_like = looks_like_proc_decl();
    if (proc_like) {
        return parse_proc_decl(std::move(attrs));
    }
    if (current_.kind == TokenKind::KwFlags) {
        return parse_flags_decl();
    }
    if (current_.kind == TokenKind::KwPick) {
        return parse_pick_decl();
    }
    if (current_.kind == TokenKind::KwComptime) {
        return parse_comptime_decl();
    }
    if (current_.kind == TokenKind::KwStaticAssert) {
        return parse_static_assert_decl();
    }
    if (current_.kind == TokenKind::KwBench) {
        return parse_bench_decl();
    }
    if (current_.kind == TokenKind::KwAsync || current_.kind == TokenKind::KwExtern) {
        return parse_proc_decl(std::move(attrs));
    }
    if (!attrs.empty() && !proc_like && current_.kind != TokenKind::KwProc &&
        current_.kind != TokenKind::KwUnsafe) {
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

    auto make_import = [&](const Ident& name, std::optional<Ident> alias = std::nullopt, bool glob = false) -> DeclId {
        std::vector<Ident> joined(path.parts.begin(), path.parts.end());
        joined.push_back(name);
        SourceSpan item_span = alias.has_value() ? alias->span : name.span;
        if (path.span.is_valid()) {
            item_span.start = path.span.start;
        }
        ModulePath item_path(std::move(joined), path.relative_depth, item_span);
        return ast_ctx_.make<UseDecl>(std::move(item_path), std::move(alias), glob, item_span);
    };

    std::vector<DeclId> decls;
    auto parse_import_item = [&]() -> DeclId {
        if (match(TokenKind::Star)) {
            return ast_ctx_.make<UseDecl>(
                ModulePath(std::vector<Ident>(path.parts.begin(), path.parts.end()), path.relative_depth, path.span),
                std::nullopt,
                true,
                span);
        }
        Ident name = parse_ident();
        std::optional<Ident> alias;
        if (match(TokenKind::KwAs)) {
            alias = parse_ident();
        }
        return make_import(std::move(name), std::move(alias), false);
    };

    if (match(TokenKind::LBrace)) {
        if (current_.kind != TokenKind::RBrace) {
            decls.push_back(parse_import_item());
            while (match(TokenKind::Comma)) {
                if (current_.kind == TokenKind::RBrace) {
                    break;
                }
                decls.push_back(parse_import_item());
            }
        }
        expect(TokenKind::RBrace, "expected '}' after import list");
    } else if (current_.kind == TokenKind::Star) {
        decls.push_back(parse_import_item());
    } else {
        decls.push_back(parse_import_item());
        while (match(TokenKind::Comma)) {
            if (current_.kind == TokenKind::Eof || current_.kind == TokenKind::RBrace) {
                break;
            }
            decls.push_back(parse_import_item());
        }
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

DeclId Parser::parse_export_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwExport, "expected 'export'");
    bool share_all = false;
    std::vector<Ident> names;
    if (match(TokenKind::Star)) {
        share_all = true;
    } else if (match(TokenKind::LBrace)) {
        if (current_.kind != TokenKind::RBrace) {
            names.push_back(parse_ident());
            while (match(TokenKind::Comma)) {
                if (current_.kind == TokenKind::RBrace) {
                    break;
                }
                names.push_back(parse_ident());
            }
        }
        expect(TokenKind::RBrace, "expected '}' after export list");
    } else {
        names.push_back(parse_ident());
        while (match(TokenKind::Comma)) {
            if (current_.kind == TokenKind::Eof || current_.kind == TokenKind::RBrace) {
                break;
            }
            names.push_back(parse_ident());
        }
    }
    span.end = previous_.span.end;
    return ast_ctx_.make<ShareDecl>(share_all, std::move(names), span);
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

DeclId Parser::parse_static_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwStatic, "expected 'static'");
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

DeclId Parser::parse_package_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwPackage, "expected 'package'");
    ModulePath path = parse_module_path();
    if (match(TokenKind::LBrace)) {
        while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
            Ident item_name = parse_ident();
            if (match(TokenKind::Colon)) {
                (void)item_name;
                (void)parse_expr();
            } else {
                emit_parse_error(diag::DiagId::ExpectedIdentifier, current_.span);
            }
            if (!match(TokenKind::Comma)) {
                break;
            }
        }
        expect(TokenKind::RBrace, "expected '}'");
    }
    span.end = path.span.end;
    return ast_ctx_.make<SpaceDecl>(std::move(path), span);
}

DeclId Parser::parse_flags_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwFlags, "expected 'flags'");
    Ident name = parse_ident();
    std::vector<FieldDecl> fields;
    if (match(TokenKind::Colon)) {
        (void)parse_type_expr();
    }
    expect(TokenKind::LBrace, "expected '{'");
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        Ident flag_name = parse_ident();
        if (match(TokenKind::Equal)) {
            (void)parse_expr();
        }
        fields.emplace_back(std::move(flag_name), ast::kInvalidAstId);
        if (!match(TokenKind::Comma)) {
            break;
        }
    }
    expect(TokenKind::RBrace, "expected '}'");
    span.end = previous_.span.end;
    return ast_ctx_.make<FormDecl>(std::move(name), std::vector<Ident>{}, std::move(fields), span);
}

DeclId Parser::parse_bench_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwBench, "expected 'bench'");
    if (current_.kind == TokenKind::StringLit) {
        advance();
    } else {
        emit_parse_error(diag::DiagId::ExpectedExpression, current_.span);
    }
    StmtId body = parse_block_body();
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<ComptimeDecl>(body, span);
}

DeclId Parser::parse_static_assert_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwStaticAssert, "expected 'static_assert'");
    expect(TokenKind::LParen, "expected '('");
    ExprId cond = parse_expr();
    if (match(TokenKind::Comma)) {
        (void)parse_expr();
    }
    expect(TokenKind::RParen, "expected ')'");
    ExprId message = ast::kInvalidAstId;
    if (cond != ast::kInvalidAstId) {
        message = ast_ctx_.make<LiteralExpr>(
            LiteralKind::String,
            "",
            ast_ctx_.get<Expr>(cond).span);
    }
    SourceSpan call_span = cond != ast::kInvalidAstId ? ast_ctx_.get<Expr>(cond).span : span;
    if (message != ast::kInvalidAstId) {
        call_span.end = ast_ctx_.get<Expr>(message).span.end;
    }
    Ident callee("assert_true", span);
    ExprId callee_expr = ast_ctx_.make<IdentExpr>(std::move(callee), span);
    std::vector<InvokeArg> args;
    args.emplace_back(cond);
    args.emplace_back(message);
    ExprId invoke = ast_ctx_.make<InvokeExpr>(
        callee_expr,
        ast::kInvalidAstId,
        std::vector<TypeId>{},
        std::move(args),
        call_span);
    std::vector<StmtId> stmts;
    stmts.push_back(ast_ctx_.make<ExprStmt>(invoke, call_span));
    StmtId body = ast_ctx_.make<BlockStmt>(std::move(stmts), span);
    return ast_ctx_.make<ComptimeDecl>(body, span);
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
        fields.push_back(parse_field_decl());
        if (match(TokenKind::Comma)) {
            if (current_.kind == TokenKind::RBrace) {
                break;
            }
            continue;
        }
    }
    expect(TokenKind::RBrace, "expected '}'");
    span.end = previous_.span.end;
    return ast_ctx_.make<FormDecl>(std::move(name), std::move(type_params), std::vector<FieldDecl>{}, span);
}

DeclId Parser::parse_class_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwClass, "expected 'class'");
    Ident name = parse_ident();
    auto type_params = parse_type_params();
    std::vector<FieldDecl> fields;
    expect(TokenKind::LBrace, "expected '{'");
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        if (current_.kind == TokenKind::KwProc || current_.kind == TokenKind::KwDef) {
            DeclId method_id = parse_proc_decl({});
            if (method_id != ast::kInvalidAstId) {
                auto& method = ast_ctx_.get<ProcDecl>(method_id);
                std::string mangled = name.name + "__" + method.name.name;
                method.name = Ident(std::move(mangled), method.name.span);

                std::vector<FnParam> params = std::move(method.params);
                if (params.empty() || params.front().ident.name != "self") {
                    TypeId self_ty = ast_ctx_.make<NamedType>(Ident(name.name, name.span), name.span);
                    params.insert(
                        params.begin(),
                        FnParam(Ident("self", name.span), self_ty, ast::kInvalidAstId, std::nullopt, true));
                } else if (params.front().type == ast::kInvalidAstId) {
                    params.front().type = ast_ctx_.make<NamedType>(Ident(name.name, name.span), name.span);
                }
                method.params = std::move(params);
                pending_decls_.push_back(method_id);
            }
        } else {
            fields.push_back(parse_field_decl());
            if (match(TokenKind::Comma)) {
                if (current_.kind == TokenKind::RBrace) {
                    break;
                }
                continue;
            }
        }
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

DeclId Parser::parse_union_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwUnion, "expected 'union'");
    Ident name = parse_ident();
    auto type_params = parse_type_params();
    std::vector<FieldDecl> fields;
    expect(TokenKind::LBrace, "expected '{'");
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        fields.push_back(parse_field_decl());
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

DeclId Parser::parse_trait_decl(std::vector<Attribute> attrs) {
    SourceSpan span = current_.span;
    bool is_unsafe = false;
    if (current_.kind == TokenKind::KwUnsafe) {
        is_unsafe = true;
        advance();
    }
    expect(TokenKind::KwTrait, "expected 'trait'");
    Ident name = parse_ident();
    auto type_params = parse_type_params();
    std::vector<std::pair<TypeId, TypeId>> where_bounds;
    if (match(TokenKind::KwWhere)) {
        while (true) {
            TypeId lhs = parse_type_expr();
            expect(TokenKind::Colon, "expected ':' in where clause");
            TypeId rhs = parse_type_expr();
            while (match(TokenKind::Plus)) {
                (void)parse_type_expr();
            }
            where_bounds.emplace_back(lhs, rhs);
            if (!match(TokenKind::Comma)) {
                break;
            }
            if (current_.kind == TokenKind::RBrace || current_.kind == TokenKind::Eof) {
                break;
            }
        }
    }
    expect(TokenKind::LBrace, "expected '{'");
    std::vector<DeclId> items;
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        std::vector<Attribute> item_attrs;
        while (current_.kind == TokenKind::AttrStart || current_.kind == TokenKind::AtSign) {
            if (current_.kind == TokenKind::AttrStart) {
                item_attrs.push_back(parse_attribute());
            } else {
                Token at = current_;
                advance();
                item_attrs.push_back(make_shorthand_attribute(at, parse_ident()));
            }
        }
        DeclId item = ast::kInvalidAstId;
        if (current_.kind == TokenKind::KwConst) {
            (void)item_attrs;
            item = parse_const_decl();
        } else if (current_.kind == TokenKind::KwType) {
            (void)item_attrs;
            item = parse_type_alias_decl();
        } else if (current_.kind == TokenKind::KwProc || current_.kind == TokenKind::KwAsync ||
                   current_.kind == TokenKind::KwUnsafe || current_.kind == TokenKind::KwExtern ||
                   current_.kind == TokenKind::KwPub || current_.kind == TokenKind::KwPriv ||
                   current_.kind == TokenKind::KwInternal || current_.kind == TokenKind::KwInline ||
                   current_.kind == TokenKind::KwNoinline || current_.kind == TokenKind::KwDef) {
            item = parse_proc_decl(std::move(item_attrs), false);
        } else {
            emit_parse_error(diag::DiagId::ExpectedTopLevelDeclaration, current_.span);
            sync_to_toplevel_boundary();
        }
        if (item != ast::kInvalidAstId) {
            items.push_back(item);
            pending_decls_.push_back(item);
        }
        if (match(TokenKind::Comma)) {
            if (current_.kind == TokenKind::RBrace) {
                break;
            }
            continue;
        }
    }
    expect(TokenKind::RBrace, "expected '}'");
    span.end = previous_.span.end;
    return ast_ctx_.make<TraitDecl>(
        std::move(attrs),
        std::move(name),
        std::move(type_params),
        is_unsafe,
        std::move(where_bounds),
        std::move(items),
        span);
}

DeclId Parser::parse_impl_decl(std::vector<Attribute> attrs) {
    SourceSpan span = current_.span;
    expect(TokenKind::KwImpl, "expected 'impl'");
    TypeId trait_type = ast::kInvalidAstId;
    TypeId self_type = ast::kInvalidAstId;
    bool has_trait = false;
    TypeId first = parse_type_expr();
    if (match(TokenKind::KwFor)) {
        has_trait = true;
        trait_type = first;
        self_type = parse_type_expr();
    } else {
        self_type = first;
    }
    std::vector<std::pair<TypeId, TypeId>> where_bounds;
    if (match(TokenKind::KwWhere)) {
        while (true) {
            TypeId lhs = parse_type_expr();
            expect(TokenKind::Colon, "expected ':' in where clause");
            TypeId rhs = parse_type_expr();
            while (match(TokenKind::Plus)) {
                (void)parse_type_expr();
            }
            where_bounds.emplace_back(lhs, rhs);
            if (!match(TokenKind::Comma)) {
                break;
            }
            if (current_.kind == TokenKind::RBrace || current_.kind == TokenKind::Eof) {
                break;
            }
        }
    }
    expect(TokenKind::LBrace, "expected '{'");
    std::vector<DeclId> items;
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        std::vector<Attribute> item_attrs;
        while (current_.kind == TokenKind::AttrStart || current_.kind == TokenKind::AtSign) {
            if (current_.kind == TokenKind::AttrStart) {
                item_attrs.push_back(parse_attribute());
            } else {
                Token at = current_;
                advance();
                item_attrs.push_back(make_shorthand_attribute(at, parse_ident()));
            }
        }
        DeclId item = ast::kInvalidAstId;
        if (current_.kind == TokenKind::KwConst) {
            (void)item_attrs;
            item = parse_const_decl();
        } else if (current_.kind == TokenKind::KwType) {
            (void)item_attrs;
            item = parse_type_alias_decl();
        } else if (current_.kind == TokenKind::KwProc || current_.kind == TokenKind::KwAsync ||
                   current_.kind == TokenKind::KwUnsafe || current_.kind == TokenKind::KwExtern ||
                   current_.kind == TokenKind::KwPub || current_.kind == TokenKind::KwPriv ||
                   current_.kind == TokenKind::KwInternal || current_.kind == TokenKind::KwInline ||
                   current_.kind == TokenKind::KwNoinline || current_.kind == TokenKind::KwDef) {
            item = parse_proc_decl(std::move(item_attrs));
        } else {
            emit_parse_error(diag::DiagId::ExpectedTopLevelDeclaration, current_.span);
            sync_to_toplevel_boundary();
        }
        if (item != ast::kInvalidAstId) {
            items.push_back(item);
            pending_decls_.push_back(item);
        }
        if (match(TokenKind::Comma)) {
            if (current_.kind == TokenKind::RBrace) {
                break;
            }
            continue;
        }
    }
    expect(TokenKind::RBrace, "expected '}'");
    span.end = previous_.span.end;
    return ast_ctx_.make<ImplDecl>(
        std::move(attrs),
        trait_type,
        self_type,
        has_trait,
        std::move(where_bounds),
        std::move(items),
        span);
}

DeclId Parser::parse_foreign_type_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwType, "expected 'type'");
    Ident name = parse_ident();
    auto type_params = parse_type_params();
    span.end = previous_.span.end;
    return ast_ctx_.make<TypeDecl>(std::move(name), std::vector<FieldDecl>{}, span);
}

DeclId Parser::parse_foreign_proc_decl() {
    std::optional<std::string> abi;
    if (current_.kind == TokenKind::KwExtern) {
        advance();
    }
    if (current_.kind == TokenKind::StringLit || current_.kind == TokenKind::Ident ||
        is_callconv_token(current_.kind)) {
        abi = current_.text;
        advance();
    }
    if (current_.kind == TokenKind::KwExtern) {
        advance();
    }
    if (current_.kind == TokenKind::KwProc) {
        DeclId proc_id = parse_proc_decl({}, false);
        if (proc_id != ast::kInvalidAstId) {
            auto& proc = ast_ctx_.get<ProcDecl>(proc_id);
            proc.is_extern = true;
            if (abi.has_value()) {
                proc.extern_abi = std::move(abi);
            } else if (!proc.extern_abi.has_value()) {
                proc.extern_abi = "C";
            }
        }
        if (match(TokenKind::Semicolon)) {
            // canonical foreign declaration terminator
        }
        return proc_id;
    }
    expect(TokenKind::KwProc, "expected 'proc'");
    return ast::kInvalidAstId;
}

DeclId Parser::parse_comptime_decl() {
    SourceSpan span = current_.span;
    expect(TokenKind::KwComptime, "expected 'comptime'");
    StmtId body = parse_block();
    span.end = ast_ctx_.get<Stmt>(body).span.end;
    return ast_ctx_.make<ComptimeDecl>(body, span);
}

FnParam Parser::parse_fn_param() {
    auto parse_mode = [&](TokenKind kind) -> std::optional<ParamMode> {
        switch (kind) {
            case TokenKind::KwMut: return ParamMode::Mut;
            case TokenKind::KwOwned: return ParamMode::Owned;
            case TokenKind::KwBorrow: return ParamMode::Borrow;
            case TokenKind::KwMove: return ParamMode::Move;
            default: return std::nullopt;
        }
    };

    State start = snapshot();

    bool self_is_ref = false;
    bool self_is_mut = false;
    if (match(TokenKind::Amp)) {
        self_is_ref = true;
    }
    if (match(TokenKind::KwMut)) {
        self_is_mut = true;
    }
    if (current_.kind == TokenKind::KwSelf) {
        Token tok = current_;
        advance();
        return FnParam(
            Ident("self", tok.span),
            ast::kInvalidAstId,
            ast::kInvalidAstId,
            std::nullopt,
            true,
            false,
            self_is_ref,
            self_is_mut);
    }
    restore(std::move(start));

    if (current_.kind == TokenKind::Ellipsis) {
        Token tok = current_;
        advance();
        return FnParam(
            Ident("...", tok.span),
            ast::kInvalidAstId,
            ast::kInvalidAstId,
            std::nullopt,
            false,
            true);
    }

    std::optional<ParamMode> mode;
    if (current_.kind == TokenKind::KwMut || current_.kind == TokenKind::KwOwned ||
        current_.kind == TokenKind::KwBorrow || current_.kind == TokenKind::KwMove) {
        mode = parse_mode(current_.kind);
        advance();
    }

    if (current_.kind == TokenKind::Ident && current_.text == "_") {
        Token tok = current_;
        advance();
        return FnParam(
            Ident(tok.text, tok.span),
            ast::kInvalidAstId,
            ast::kInvalidAstId,
            std::move(mode));
    }

    if (current_.kind == TokenKind::Ident || is_identifier_token(current_.kind, KeywordPolicy::Permissive)) {
        Ident name = parse_ident();
        if (match(TokenKind::Colon)) {
            if (match(TokenKind::Ellipsis)) {
                TypeId ty = parse_type_expr();
                return FnParam(
                    std::move(name),
                    ty,
                    ast::kInvalidAstId,
                    std::move(mode),
                    false,
                    true);
            }
            TypeId ty = parse_type_expr();
            ExprId default_value = ast::kInvalidAstId;
            if (match(TokenKind::Equal)) {
                default_value = parse_expr();
            }
            return FnParam(
                std::move(name),
                ty,
                default_value,
                std::move(mode));
        }
        return FnParam(
            std::move(name),
            ast::kInvalidAstId,
            ast::kInvalidAstId,
            std::move(mode));
    }

    emit_parse_error(diag::DiagId::ExpectedIdentifier, current_.span);
    advance();
    return FnParam(Ident("<error>", current_.span), ast::kInvalidAstId);
}

DeclId Parser::parse_proc_decl(std::vector<Attribute> attrs, bool allow_body) {
    SourceSpan span = current_.span;
    bool is_async = false;
    bool is_unsafe = false;
    bool is_extern = false;
    std::optional<std::string> extern_abi;
    for (const auto& attr : attrs) {
        if (attr.name.name == "extern") {
            is_extern = true;
        }
    }
    if (current_.kind == TokenKind::KwPub || current_.kind == TokenKind::KwPriv ||
        current_.kind == TokenKind::KwInternal) {
        advance();
        if (match(TokenKind::LParen)) {
            while (current_.kind != TokenKind::RParen && current_.kind != TokenKind::Eof) {
                advance();
            }
            expect(TokenKind::RParen, "expected ')' after visibility scope");
        }
    }
    while (current_.kind == TokenKind::KwAsync || current_.kind == TokenKind::KwUnsafe ||
           current_.kind == TokenKind::KwConst || current_.kind == TokenKind::KwInline ||
           current_.kind == TokenKind::KwNoinline || current_.kind == TokenKind::KwExtern) {
        if (match(TokenKind::KwAsync)) {
            is_async = true;
            continue;
        }
        if (match(TokenKind::KwUnsafe)) {
            is_unsafe = true;
            continue;
        }
        if (match(TokenKind::KwConst) || match(TokenKind::KwInline) || match(TokenKind::KwNoinline)) {
            continue;
        }
        if (match(TokenKind::KwExtern)) {
            is_extern = true;
            if (current_.kind == TokenKind::StringLit || current_.kind == TokenKind::Ident ||
                is_callconv_token(current_.kind)) {
                extern_abi = current_.text;
                advance();
            }
            continue;
        }
    }
    (void)is_unsafe;
    if (current_.kind == TokenKind::KwProc || current_.kind == TokenKind::KwDef) {
        advance();
    } else {
        expect(TokenKind::KwProc, "expected 'proc'");
    }
    Ident name = parse_ident();
    auto type_params = parse_type_params();
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
    std::vector<std::pair<TypeId, TypeId>> where_bounds;
    if (match(TokenKind::KwWhere)) {
        while (true) {
            TypeId lhs = parse_type_expr();
            expect(TokenKind::Colon, "expected ':' in where clause");
            TypeId rhs = parse_type_expr();
            while (match(TokenKind::Plus)) {
                (void)parse_type_expr();
            }
            where_bounds.emplace_back(lhs, rhs);
            if (!match(TokenKind::Comma)) {
                break;
            }
            if (current_.kind == TokenKind::RBrace || current_.kind == TokenKind::Eof) {
                break;
            }
        }
    }
    if (match(TokenKind::KwRequires)) {
        if (match(TokenKind::LParen)) {
            int depth = 1;
            while (depth > 0 && current_.kind != TokenKind::Eof) {
                if (current_.kind == TokenKind::LParen) {
                    ++depth;
                } else if (current_.kind == TokenKind::RParen) {
                    --depth;
                    if (depth == 0) {
                        break;
                    }
                }
                advance();
            }
            expect(TokenKind::RParen, "expected ')' after requires");
        }
    }
    if (match(TokenKind::KwEffects)) {
        if (match(TokenKind::LParen)) {
            int depth = 1;
            while (depth > 0 && current_.kind != TokenKind::Eof) {
                if (current_.kind == TokenKind::LParen) {
                    ++depth;
                } else if (current_.kind == TokenKind::RParen) {
                    --depth;
                    if (depth == 0) {
                        break;
                    }
                }
                advance();
            }
            expect(TokenKind::RParen, "expected ')' after effects");
        }
    }
    if (match(TokenKind::KwNoexcept)) {
        // canonical suffix accepted, semantics handled downstream
    }
    StmtId body = ast::kInvalidAstId;
    if (allow_body && current_.kind == TokenKind::LBrace) {
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
        is_async,
        is_extern,
        !allow_body,
        std::move(extern_abi),
        std::move(where_bounds),
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
    std::optional<std::string> visibility;
    if (current_.kind == TokenKind::KwPub || current_.kind == TokenKind::KwPriv || current_.kind == TokenKind::KwInternal) {
        visibility = current_.text;
        advance();
        if (match(TokenKind::LParen)) {
            std::string scope = "unknown";
            if (current_.kind == TokenKind::KwPackage || current_.kind == TokenKind::KwSuper ||
                (current_.kind == TokenKind::Ident && current_.text == "module")) {
                scope = current_.text;
                advance();
            }
            expect(TokenKind::RParen, "expected ')' after visibility scope");
            visibility = *visibility + "(" + scope + ")";
        }
    }
    Ident name = parse_ident();
    if (match(TokenKind::KwAs) || match(TokenKind::Colon)) {
        // canonical field separator
    } else {
        expect(TokenKind::KwAs, "expected 'as' or ':'");
    }
    TypeId ty = parse_type_expr();
    return FieldDecl(std::move(name), ty, std::move(visibility));
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
