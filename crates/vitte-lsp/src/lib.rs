//! vitte-lsp — Language Server Protocol pour Vitte
//!
//! - Transport: stdio (via tower-lsp)
//! - Diagnostics: basés sur le lexer (`vitte-lexer`)
//! - Hover: info sur le token sous le curseur
//! - Completion: mots-clés + snippets usuels
//! - Document symbols: extraction via AST (fn)
//! - Formatting: trailing spaces trim + newline final
//!
//! Pour lancer en binaire, créez un `src/main.rs` qui appelle `start_stdio().await`.

#![deny(missing_docs)]

use std::collections::HashMap;
use std::sync::Arc;

use tokio::sync::RwLock;
use tower_lsp::jsonrpc;
use tower_lsp::lsp_types as lsp;
use tower_lsp::{Client, LanguageServer, LspService, Server};

use vitte_core::{Pos, SourceId, Span};
use vitte_lexer::{Lexer, LexerOptions, Token, TokenKind};
use vitte_syntax::{IncrementalParser, Item, ParseDelta, SyntaxModule, TextEdit};

/// Démarre le serveur LSP Vitte sur STDIN/STDOUT (à utiliser depuis un binaire).
pub async fn start_stdio() -> anyhow::Result<()> {
    let (service, socket) = LspService::new(|client| Backend::new(client));
    Server::new(tokio::io::stdin(), tokio::io::stdout(), socket).serve(service).await;
    Ok(())
}

/* ─────────────────────────── Backend ─────────────────────────── */

/// Configuration utilisateur (extensible si besoin).
#[derive(Debug, Clone)]
pub struct Config {
    /// Activer/afficher les diagnostics du lexer.
    pub diagnostics: bool,
    /// Activer les complétions mots-clés.
    pub completions: bool,
}

impl Default for Config {
    fn default() -> Self {
        Self { diagnostics: true, completions: true }
    }
}

/// Un document ouvert côté serveur.
#[derive(Debug, Clone)]
struct Document {
    uri: lsp::Url,
    version: i32,
    parser: IncrementalParser,
    line_starts: Vec<usize>,
    last_delta: ParseDelta,
}

impl Document {
    fn new(uri: lsp::Url, text: String, version: i32) -> Self {
        let parser = IncrementalParser::new(text);
        let line_starts = compute_line_starts(parser.source());
        let has_errors = !parser.module().errors.is_empty();
        let last_delta =
            ParseDelta { full_reparse: true, errors_changed: has_errors, ..ParseDelta::default() };
        Self { uri, version, parser, line_starts, last_delta }
    }

    fn text(&self) -> &str {
        self.parser.source()
    }

    fn module(&self) -> &SyntaxModule {
        self.parser.module()
    }

    fn apply_change(&mut self, change: &lsp::TextDocumentContentChangeEvent) -> ParseDelta {
        let delta = match (&change.range, &change.range_length) {
            (None, None) => {
                self.parser = IncrementalParser::new(change.text.clone());
                ParseDelta { full_reparse: true, errors_changed: true, ..ParseDelta::default() }
            }
            (Some(range), _) => {
                let start_off = position_to_offset(self.text(), &self.line_starts, range.start);
                let end_off = position_to_offset(self.text(), &self.line_starts, range.end);
                if start_off <= end_off && end_off <= self.text().len() {
                    let edit =
                        TextEdit { range: start_off..end_off, replacement: change.text.clone() };
                    self.parser.apply_edit(edit)
                } else {
                    ParseDelta::default()
                }
            }
            _ => ParseDelta::default(),
        };
        self.line_starts = compute_line_starts(self.text());
        self.last_delta = delta.clone();
        delta
    }
}

#[derive(Debug)]
struct Backend {
    client: Client,
    docs: Arc<RwLock<HashMap<lsp::Url, Document>>>,
    config: Arc<RwLock<Config>>,
}

impl Backend {
    fn new(client: Client) -> Self {
        Self {
            client,
            docs: Arc::new(RwLock::new(HashMap::new())),
            config: Arc::new(RwLock::new(Config::default())),
        }
    }

    async fn publish_diagnostics(&self, doc: &Document, force: bool) {
        if !self.config.read().await.diagnostics {
            // vider si désactivé
            let _ =
                self.client.publish_diagnostics(doc.uri.clone(), vec![], Some(doc.version)).await;
            return;
        }
        if !force && !doc.last_delta.errors_changed {
            return;
        }
        let diags = compute_document_diagnostics(doc);
        let _ = self.client.publish_diagnostics(doc.uri.clone(), diags, Some(doc.version)).await;
    }
}

/* ─────────────────────────── LanguageServer impl ─────────────────────────── */

#[tower_lsp::async_trait]
impl LanguageServer for Backend {
    async fn initialize(&self, _: lsp::InitializeParams) -> jsonrpc::Result<lsp::InitializeResult> {
        let text_sync =
            lsp::TextDocumentSyncCapability::Kind(lsp::TextDocumentSyncKind::INCREMENTAL);

        let completion = Some(lsp::CompletionOptions {
            resolve_provider: Some(false),
            trigger_characters: Some(vec![".".into(), ":".into(), ">".into(), "=".into()]),
            ..Default::default()
        });

        let hover = Some(lsp::HoverProviderCapability::Simple(true));

        let doc_symbols = Some(lsp::OneOf::Left(true));

        let formatting = Some(lsp::OneOf::Left(true));

        let capabilities = lsp::ServerCapabilities {
            text_document_sync: Some(text_sync),
            completion_provider: completion,
            hover_provider: hover,
            document_symbol_provider: doc_symbols,
            document_formatting_provider: formatting,
            definition_provider: Some(lsp::OneOf::Left(false)),
            ..Default::default()
        };

        Ok(lsp::InitializeResult {
            capabilities,
            server_info: Some(lsp::ServerInfo {
                name: "vitte-lsp".into(),
                version: Some("0.1.0".into()),
            }),
        })
    }

    async fn initialized(&self, _: lsp::InitializedParams) {
        let _ = self.client.log_message(lsp::MessageType::INFO, "vitte-lsp ready ✨").await;
    }

    async fn shutdown(&self) -> jsonrpc::Result<()> {
        Ok(())
    }

    async fn did_open(&self, params: lsp::DidOpenTextDocumentParams) {
        let td = params.text_document;
        let doc = Document::new(td.uri.clone(), td.text, td.version);
        self.docs.write().await.insert(td.uri.clone(), doc);
        if let Some(doc) = self.docs.read().await.get(&td.uri).cloned() {
            self.publish_diagnostics(&doc, true).await;
        }
    }

    async fn did_change(&self, params: lsp::DidChangeTextDocumentParams) {
        let mut docs = self.docs.write().await;
        if let Some(doc) = docs.get_mut(&params.text_document.uri) {
            let mut delta = ParseDelta::default();
            for change in &params.content_changes {
                delta = doc.apply_change(change);
            }
            doc.version = params.text_document.version;
            let doc_clone = doc.clone();
            drop(docs);
            if delta.errors_changed {
                self.publish_diagnostics(&doc_clone, false).await;
            }
            if has_symbol_delta(&delta) {
                let _ = self
                    .client
                    .log_message(
                        lsp::MessageType::INFO,
                        format_changed_functions(&params.text_document.uri, &delta),
                    )
                    .await;
            }
        }
    }

    async fn did_save(&self, params: lsp::DidSaveTextDocumentParams) {
        if let Some(doc) = self.docs.read().await.get(&params.text_document.uri).cloned() {
            self.publish_diagnostics(&doc, true).await;
        }
    }

    async fn did_close(&self, params: lsp::DidCloseTextDocumentParams) {
        // Efface diagnostics
        let _ =
            self.client.publish_diagnostics(params.text_document.uri.clone(), vec![], None).await;
        self.docs.write().await.remove(&params.text_document.uri);
    }

    async fn hover(&self, params: lsp::HoverParams) -> jsonrpc::Result<Option<lsp::Hover>> {
        let uri = match params.text_document_position_params.text_document.uri {
            u => u,
        };
        let docs = self.docs.read().await;
        let doc = match docs.get(&uri) {
            Some(d) => d,
            None => return Ok(None),
        };

        let pos = params.text_document_position_params.position;
        let text = doc.text();
        let off = position_to_offset(text, &doc.line_starts, pos);

        // Token sous curseur via un lexing simple
        let (tok, span) = token_at_offset(text, off)
            .unwrap_or((TokenKind::Eof, Span { source: SourceId(0), start: Pos(0), end: Pos(0) }));

        let (title, body) = hover_contents(&tok);

        let start = byte_offset_to_position(text, &doc.line_starts, span.start.0 as usize);
        let end = byte_offset_to_position(text, &doc.line_starts, span.end.0 as usize);

        let hover = lsp::Hover {
            contents: lsp::HoverContents::Markup(lsp::MarkupContent {
                kind: lsp::MarkupKind::Markdown,
                value: format!("**{}**\n\n{}", title, body),
            }),
            range: Some(lsp::Range { start, end }),
        };
        Ok(Some(hover))
    }

    async fn completion(
        &self,
        params: lsp::CompletionParams,
    ) -> jsonrpc::Result<Option<lsp::CompletionResponse>> {
        if !self.config.read().await.completions {
            return Ok(None);
        }
        let trigger =
            params.context.as_ref().and_then(|c| c.trigger_character.clone()).unwrap_or_default();

        let mut items = keyword_completions();

        // Petits snippets utiles
        if trigger.is_empty() {
            items.extend_from_slice(&[
                snippet(
                    "fn",
                    "fn ${1:name}(${2}) -> ${3:Void} {\n\t$0\n}",
                    "Déclarer une fonction",
                ),
                snippet("struct", "struct ${1:Name} {\n\t$0\n}", "Déclarer une struct"),
                snippet("if", "if ${1:cond} {\n\t$0\n}", "if"),
                snippet("while", "while ${1:cond} {\n\t$0\n}", "while"),
            ]);
        }

        Ok(Some(lsp::CompletionResponse::Array(items)))
    }

    async fn document_symbol(
        &self,
        params: lsp::DocumentSymbolParams,
    ) -> jsonrpc::Result<Option<lsp::DocumentSymbolResponse>> {
        let docs = self.docs.read().await;
        let doc = match docs.get(&params.text_document.uri) {
            Some(d) => d,
            None => return Ok(None),
        };
        let symbols = extract_symbols(doc.module(), doc.text(), &doc.line_starts, &doc.uri);
        Ok(Some(lsp::DocumentSymbolResponse::Flat(symbols)))
    }

    async fn formatting(
        &self,
        params: lsp::DocumentFormattingParams,
    ) -> jsonrpc::Result<Option<Vec<lsp::TextEdit>>> {
        let docs = self.docs.read().await;
        let doc = match docs.get(&params.text_document.uri) {
            Some(d) => d,
            None => return Ok(None),
        };
        let text = doc.text();
        let formatted = format_document(text);
        if formatted == text {
            return Ok(Some(vec![]));
        }
        let full_range = lsp::Range {
            start: lsp::Position::new(0, 0),
            end: byte_offset_to_position(text, &doc.line_starts, text.len()),
        };
        Ok(Some(vec![lsp::TextEdit { range: full_range, new_text: formatted }]))
    }
}

/* ─────────────────────────── Diagnostics ─────────────────────────── */

fn compute_document_diagnostics(doc: &Document) -> Vec<lsp::Diagnostic> {
    let text = doc.text();
    let mut diags = compute_parser_diagnostics(doc.module(), text, &doc.line_starts);
    diags.extend(compute_lexer_diagnostics(text, &doc.line_starts));
    diags
}

fn compute_parser_diagnostics(
    module: &SyntaxModule,
    text: &str,
    line_starts: &[usize],
) -> Vec<lsp::Diagnostic> {
    module
        .errors
        .iter()
        .map(|err| {
            let range = syntax_span_to_range(text, line_starts, err.span);
            lsp::Diagnostic {
                range,
                severity: Some(lsp::DiagnosticSeverity::ERROR),
                code: None,
                code_description: None,
                source: Some("vitte-parser".into()),
                message: err.message.clone(),
                related_information: None,
                tags: None,
                data: None,
            }
        })
        .collect()
}

fn compute_lexer_diagnostics(text: &str, line_starts: &[usize]) -> Vec<lsp::Diagnostic> {
    let mut diags = Vec::new();
    let mut lx = Lexer::with_options(text, SourceId(0), LexerOptions::default());
    loop {
        match lx.next() {
            Ok(Some(tok)) => {
                if matches!(tok.value, TokenKind::Eof) {
                    break;
                }
            }
            Ok(None) => break,
            Err(e) => {
                // Convertit Span -> Range UTF-16
                let range = span_to_range(text, line_starts, e.span);
                diags.push(lsp::Diagnostic {
                    range,
                    severity: Some(lsp::DiagnosticSeverity::ERROR),
                    code: None,
                    code_description: None,
                    source: Some("vitte-lexer".into()),
                    message: e.to_string(),
                    related_information: None,
                    tags: None,
                    data: None,
                });
                // Le lexer s'arrête à la première erreur. On publie cette erreur et sort.
                break;
            }
        }
    }
    diags
}

/* ─────────────────────────── Hover helpers ─────────────────────────── */

fn token_at_offset(text: &str, off: usize) -> Option<(TokenKind<'_>, Span)> {
    let mut lx = Lexer::new(text, SourceId(0));
    while let Ok(Some(Token { value, span })) = lx.next() {
        if (span.start.0 as usize) <= off && off < (span.end.0 as usize) {
            return Some((value, span));
        }
        if matches!(value, TokenKind::Eof) {
            break;
        }
    }
    None
}

fn hover_contents(tok: &TokenKind<'_>) -> (String, String) {
    match tok {
        TokenKind::Kw(k) => ("mot-clé".into(), format!("`{:?}`", k)),
        TokenKind::Ident(s) => ("identifiant".into(), format!("`{s}`")),
        TokenKind::Int(i) => ("entier".into(), format!("`{i}`: i64")),
        TokenKind::Float(f) => ("flottant".into(), format!("`{f}`: f64")),
        TokenKind::Str(s) => ("chaîne".into(), format!("\"{}\"", s.escape_default())),
        TokenKind::Char(c) => ("caractère".into(), format!("`{c}`")),
        TokenKind::LParen
        | TokenKind::RParen
        | TokenKind::LBrace
        | TokenKind::RBrace
        | TokenKind::LBracket
        | TokenKind::RBracket
        | TokenKind::Comma
        | TokenKind::Dot
        | TokenKind::Semi
        | TokenKind::Colon
        | TokenKind::PathSep
        | TokenKind::Arrow
        | TokenKind::FatArrow
        | TokenKind::Plus
        | TokenKind::Minus
        | TokenKind::Star
        | TokenKind::Slash
        | TokenKind::Percent
        | TokenKind::Eq
        | TokenKind::EqEq
        | TokenKind::Ne
        | TokenKind::Lt
        | TokenKind::Le
        | TokenKind::Gt
        | TokenKind::Ge
        | TokenKind::AndAnd
        | TokenKind::OrOr
        | TokenKind::Bang => ("symbole".into(), format!("{tok:?}")),
        TokenKind::Eof => ("fin de fichier".into(), "".into()),
    }
}

/* ─────────────────────────── Completions ─────────────────────────── */

fn keyword_completions() -> Vec<lsp::CompletionItem> {
    use lsp::CompletionItemKind as K;
    let kws = [
        ("fn", K::KEYWORD, "Déclare une fonction"),
        ("let", K::KEYWORD, "Déclare une variable"),
        ("const", K::KEYWORD, "Constante"),
        ("return", K::KEYWORD, "Retourne une valeur"),
        ("if", K::KEYWORD, "Condition"),
        ("else", K::KEYWORD, "Alternative"),
        ("while", K::KEYWORD, "Boucle while"),
        ("for", K::KEYWORD, "Boucle for"),
        ("struct", K::STRUCT, "Structure"),
        ("enum", K::ENUM, "Énumération"),
        ("true", K::VALUE, "Booléen vrai"),
        ("false", K::VALUE, "Booléen faux"),
        ("null", K::VALUE, "Null"),
    ];
    kws.iter()
        .map(|(label, kind, detail)| lsp::CompletionItem {
            label: label.to_string(),
            kind: Some(*kind),
            detail: Some((*detail).into()),
            ..Default::default()
        })
        .collect()
}

fn snippet(label: &str, body: &str, detail: &str) -> lsp::CompletionItem {
    lsp::CompletionItem {
        label: label.into(),
        kind: Some(lsp::CompletionItemKind::SNIPPET),
        insert_text_format: Some(lsp::InsertTextFormat::SNIPPET),
        insert_text: Some(body.into()),
        detail: Some(detail.into()),
        ..Default::default()
    }
}

/* ─────────────────────────── Symbols ─────────────────────────── */

fn extract_symbols(
    module: &SyntaxModule,
    text: &str,
    line_starts: &[usize],
    uri: &lsp::Url,
) -> Vec<lsp::SymbolInformation> {
    let mut out = Vec::new();
    for item in &module.items {
        match item {
            Item::Fn(f) => {
                let range = syntax_span_to_range(text, line_starts, f.span);
                #[allow(deprecated)]
                out.push(lsp::SymbolInformation {
                    name: f.name.clone(),
                    kind: lsp::SymbolKind::FUNCTION,
                    tags: None,
                    deprecated: None,
                    location: lsp::Location { uri: uri.clone(), range },
                    container_name: None,
                });
            }
        }
    }

    out
}

fn has_symbol_delta(delta: &ParseDelta) -> bool {
    !(delta.changed_functions.is_empty()
        && delta.inserted_functions.is_empty()
        && delta.removed_functions.is_empty())
}

fn format_changed_functions(uri: &lsp::Url, delta: &ParseDelta) -> String {
    fn summarize(label: &str, items: &[String]) -> Option<String> {
        if items.is_empty() {
            return None;
        }
        let preview_len = items.len().min(3);
        let mut preview = items.iter().take(preview_len).cloned().collect::<Vec<_>>().join(", ");
        if items.len() > preview_len {
            let remaining = items.len() - preview_len;
            preview.push_str(&format!(" (+{} more)", remaining));
        }
        Some(format!("{}{}", label, preview))
    }

    let mut parts = Vec::new();
    if let Some(s) = summarize("~", &delta.changed_functions) {
        parts.push(s);
    }
    if let Some(s) = summarize("+", &delta.inserted_functions) {
        parts.push(s);
    }
    if let Some(s) = summarize("-", &delta.removed_functions) {
        parts.push(s);
    }
    let summary =
        if parts.is_empty() { "(no symbol delta)".to_string() } else { parts.join(" | ") };
    format!("{} → {}", uri, summary)
}

/* ─────────────────────────── Formatting ─────────────────────────── */

fn format_document(text: &str) -> String {
    let mut out = String::with_capacity(text.len());
    for line in text.lines() {
        let trimmed = line.trim_end_matches(|c: char| c == ' ' || c == '\t');
        out.push_str(trimmed);
        out.push('\n');
    }
    if !text.ends_with('\n') {
        // on a déjà ajouté un \n par ligne
    }
    out
}

/* ─────────────────────────── Mapping UTF-16 / Spans ─────────────────────────── */

fn compute_line_starts(text: &str) -> Vec<usize> {
    let mut v = Vec::with_capacity(64);
    v.push(0);
    for (i, b) in text.bytes().enumerate() {
        if b == b'\n' {
            v.push(i + 1);
        }
    }
    v
}

fn position_to_offset(text: &str, line_starts: &[usize], pos: lsp::Position) -> usize {
    let line = (pos.line as usize).min(line_starts.len().saturating_sub(1));
    let line_start = line_starts[line];
    let line_end = if line + 1 < line_starts.len() { line_starts[line + 1] } else { text.len() };
    let slice = &text[line_start..line_end];

    // character est en UTF-16 code units → convertir
    let mut u16_count = 0u32;
    for (byte_off, ch) in slice.char_indices() {
        if u16_count == pos.character {
            return line_start + byte_off;
        }
        u16_count += ch.len_utf16() as u32;
    }
    // Si on dépasse, retourne fin de ligne
    line_end
}

fn byte_offset_to_position(text: &str, line_starts: &[usize], off: usize) -> lsp::Position {
    let off = off.min(text.len());
    // trouver la ligne
    let mut lo = 0usize;
    let mut hi = line_starts.len();
    while lo + 1 < hi {
        let mid = (lo + hi) / 2;
        if line_starts[mid] <= off {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    let line = lo;
    let _col_bytes = off - line_starts[line];
    let slice = &text[line_starts[line]..off];

    // convertir bytes -> utf16 units
    let col16: u32 = slice.chars().map(|c| c.len_utf16() as u32).sum();

    lsp::Position::new(line as u32, col16)
}

fn span_to_range(text: &str, line_starts: &[usize], sp: Span) -> lsp::Range {
    let start = byte_offset_to_position(text, line_starts, sp.start.0 as usize);
    let end = byte_offset_to_position(text, line_starts, sp.end.0 as usize);
    lsp::Range { start, end }
}

fn syntax_span_to_range(text: &str, line_starts: &[usize], sp: vitte_syntax::Span) -> lsp::Range {
    let start = byte_offset_to_position(text, line_starts, sp.start as usize);
    let end = byte_offset_to_position(text, line_starts, sp.end as usize);
    lsp::Range { start, end }
}
