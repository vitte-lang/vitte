// vitte/rust/crates/vitte_lsp/src/main.rs
//
// vitte-lsp — Language Server Protocol server for Vitte
//
// Goals (phase 1):
//   - Provide basic LSP plumbing: initialize, shutdown, text sync
//   - Diagnostics: basic UTF-8 + trivial syntax checks (placeholder)
//   - Formatting: delegate to vitte-fmt engine (later shared crate)
//   - Hover/completion: stubs for now
//
// Strategy:
//   - Use tower-lsp for robust JSON-RPC/LSP wiring
//   - Maintain in-memory document store keyed by URI
//   - On change/save: validate + publish diagnostics
//
// Notes:
//   - This is a "max" bootstrap, production-oriented skeleton.
//   - Real parser integration should replace the placeholder validator.
//   - Keep it fast: avoid heavy clones, use Arc/RwLock.
//
// Build/run:
//   - cargo run -p vitte_lsp
//   - Editor config: point to the binary `vitte-lsp`
//
// Crate deps (recommended in Cargo.toml):
//   tower-lsp = "0.20"
//   tokio = { version = "1", features=["rt-multi-thread","macros","io-std"] }
//   url = "2"
//   vitte_common = { path="../vitte_common" }

use std::collections::HashMap;
use std::sync::Arc;

use tokio::sync::RwLock;
use tower_lsp::jsonrpc::Result as LspResult;
use tower_lsp::lsp_types::*;
use tower_lsp::{Client, LanguageServer, LspService, Server};

use vitte_common::err::{ErrCode, VitteError};

#[tokio::main(flavor = "multi_thread")]
async fn main() {
    let stdin = tokio::io::stdin();
    let stdout = tokio::io::stdout();

    let (service, socket) = LspService::new(|client| Backend::new(client));
    Server::new(stdin, stdout, socket).serve(service).await;
}

// =============================================================================
// Document store
// =============================================================================

#[derive(Debug, Clone)]
struct Document {
    version: i32,
    text: String,
}

#[derive(Default)]
struct Docs {
    map: HashMap<Url, Document>,
}

impl Docs {
    fn open(&mut self, uri: Url, version: i32, text: String) {
        self.map.insert(uri, Document { version, text });
    }

    fn close(&mut self, uri: &Url) {
        self.map.remove(uri);
    }

    fn update(&mut self, uri: &Url, version: i32, text: String) {
        self.map.insert(uri.clone(), Document { version, text });
    }

    fn get(&self, uri: &Url) -> Option<&Document> {
        self.map.get(uri)
    }
}

// =============================================================================
// Backend
// =============================================================================

struct Backend {
    client: Client,
    docs: Arc<RwLock<Docs>>,
}

impl Backend {
    fn new(client: Client) -> Self {
        Self {
            client,
            docs: Arc::new(RwLock::new(Docs::default())),
        }
    }

    async fn publish_diagnostics_for(&self, uri: &Url) {
        let doc = {
            let docs = self.docs.read().await;
            docs.get(uri).cloned()
        };

        let Some(doc) = doc else {
            self.client.publish_diagnostics(uri.clone(), vec![], None).await;
            return;
        };

        let diags = validate_vitte_source(&doc.text);
        self.client
            .publish_diagnostics(uri.clone(), diags, Some(doc.version))
            .await;
    }
}

#[tower_lsp::async_trait]
impl LanguageServer for Backend {
    async fn initialize(&self, params: InitializeParams) -> LspResult<InitializeResult> {
        let caps = ServerCapabilities {
            text_document_sync: Some(TextDocumentSyncCapability::Options(TextDocumentSyncOptions {
                open_close: Some(true),
                change: Some(TextDocumentSyncKind::FULL),
                save: Some(TextDocumentSyncSaveOptions::Supported(true)),
                ..Default::default()
            })),
            document_formatting_provider: Some(OneOf::Left(true)),
            hover_provider: Some(HoverProviderCapability::Simple(true)),
            completion_provider: Some(CompletionOptions {
                resolve_provider: Some(false),
                trigger_characters: Some(vec![".".into(), "/".into(), ":".into()]),
                ..Default::default()
            }),
            ..Default::default()
        };

        // Log a bit of environment info
        if let Some(info) = params.client_info {
            self.client
                .log_message(MessageType::INFO, format!("client: {} {}", info.name, info.version.unwrap_or_default()))
                .await;
        }
        self.client
            .log_message(MessageType::INFO, "vitte-lsp initialized")
            .await;

        Ok(InitializeResult {
            capabilities: caps,
            server_info: Some(ServerInfo {
                name: "vitte-lsp".into(),
                version: Some(env!("CARGO_PKG_VERSION").into()),
            }),
        })
    }

    async fn initialized(&self, _: InitializedParams) {
        self.client
            .log_message(MessageType::INFO, "vitte-lsp ready")
            .await;
    }

    async fn shutdown(&self) -> LspResult<()> {
        Ok(())
    }

    async fn did_open(&self, params: DidOpenTextDocumentParams) {
        let td = params.text_document;
        let uri = td.uri;
        let version = td.version;
        let text = td.text;

        {
            let mut docs = self.docs.write().await;
            docs.open(uri.clone(), version, text);
        }

        self.publish_diagnostics_for(&uri).await;
    }

    async fn did_change(&self, params: DidChangeTextDocumentParams) {
        let uri = params.text_document.uri;
        let version = params.text_document.version;

        // FULL sync: one change contains full text
        let Some(change) = params.content_changes.into_iter().last() else {
            return;
        };

        {
            let mut docs = self.docs.write().await;
            docs.update(&uri, version, change.text);
        }

        self.publish_diagnostics_for(&uri).await;
    }

    async fn did_save(&self, params: DidSaveTextDocumentParams) {
        let uri = params.text_document.uri;
        self.publish_diagnostics_for(&uri).await;
    }

    async fn did_close(&self, params: DidCloseTextDocumentParams) {
        let uri = params.text_document.uri;
        {
            let mut docs = self.docs.write().await;
            docs.close(&uri);
        }
        self.client.publish_diagnostics(uri, vec![], None).await;
    }

    async fn formatting(&self, params: DocumentFormattingParams) -> LspResult<Option<Vec<TextEdit>>> {
        let uri = params.text_document.uri;

        let doc = {
            let docs = self.docs.read().await;
            docs.get(&uri).cloned()
        };

        let Some(doc) = doc else {
            return Ok(Some(vec![]));
        };

        // Placeholder formatter: normalize whitespace + indentation with `.end` heuristic.
        // Replace with shared formatter crate later.
        let formatted = simple_format(&doc.text);

        if formatted == doc.text {
            return Ok(Some(vec![]));
        }

        // Replace whole document
        let range = full_document_range(&doc.text);
        Ok(Some(vec![TextEdit { range, new_text: formatted }]))
    }

    async fn hover(&self, params: HoverParams) -> LspResult<Option<Hover>> {
        let uri = params.text_document_position_params.text_document.uri;

        let doc = {
            let docs = self.docs.read().await;
            docs.get(&uri).cloned()
        };

        let Some(doc) = doc else {
            return Ok(None);
        };

        let pos = params.text_document_position_params.position;
        let word = extract_word_at(&doc.text, pos);

        if word.is_empty() {
            return Ok(None);
        }

        let contents = HoverContents::Markup(MarkupContent {
            kind: MarkupKind::Markdown,
            value: format!("**{}**\n\n`vitte-lsp` stub hover.\n", word),
        });

        Ok(Some(Hover {
            contents,
            range: None,
        }))
    }

    async fn completion(&self, _: CompletionParams) -> LspResult<Option<CompletionResponse>> {
        // Minimal keyword completions
        let items = vec![
            kw("mod"), kw("use"), kw("export"), kw("type"),
            kw("fn"), kw("scn"), kw("prog"),
            kw("let"), kw("const"),
            kw("if"), kw("elif"), kw("else"),
            kw("while"), kw("for"), kw("match"),
            kw("when"), kw("loop"),
            kw(".end"),
            kw("ret"), kw("set"), kw("say"), kw("do"),
        ];
        Ok(Some(CompletionResponse::Array(items)))
    }
}

// =============================================================================
// Diagnostics (placeholder validator)
// =============================================================================

fn validate_vitte_source(text: &str) -> Vec<Diagnostic> {
    // Placeholder checks:
    //   - UTF-8 is always valid in Rust String, but keep for symmetry
    //   - `.end` balance heuristic
    //   - tabs discouraged (example style rule)
    let mut diags = Vec::new();

    // Tabs warning
    for (line_idx, line) in text.lines().enumerate() {
        if let Some(col) = line.find('\t') {
            diags.push(Diagnostic {
                range: Range {
                    start: Position::new(line_idx as u32, col as u32),
                    end: Position::new(line_idx as u32, (col + 1) as u32),
                },
                severity: Some(DiagnosticSeverity::WARNING),
                code: Some(NumberOrString::String("STYLE_TAB".into())),
                code_description: None,
                source: Some("vitte-lsp".into()),
                message: "tab character (prefer spaces)".into(),
                related_information: None,
                tags: None,
                data: None,
            });
        }
    }

    // `.end` balance heuristic
    let mut depth: i32 = 0;
    for (line_idx, line) in text.lines().enumerate() {
        let t = line.trim();
        if t == ".end" {
            depth -= 1;
            if depth < 0 {
                diags.push(Diagnostic {
                    range: Range {
                        start: Position::new(line_idx as u32, 0),
                        end: Position::new(line_idx as u32, 4),
                    },
                    severity: Some(DiagnosticSeverity::ERROR),
                    code: Some(NumberOrString::String("UNBALANCED_END".into())),
                    code_description: None,
                    source: Some("vitte-lsp".into()),
                    message: "unexpected .end (block underflow)".into(),
                    related_information: None,
                    tags: None,
                    data: None,
                });
                depth = 0;
            }
        } else if opens_block_heuristic(t) {
            depth += 1;
        }
    }

    if depth != 0 {
        // Point at EOF
        let last_line = text.lines().count().saturating_sub(1) as u32;
        diags.push(Diagnostic {
            range: Range {
                start: Position::new(last_line, 0),
                end: Position::new(last_line, 0),
            },
            severity: Some(DiagnosticSeverity::ERROR),
            code: Some(NumberOrString::String("MISSING_END".into())),
            code_description: None,
            source: Some("vitte-lsp".into()),
            message: format!("missing .end ({} block(s) not closed)", depth),
            related_information: None,
            tags: None,
            data: None,
        });
    }

    diags
}

fn opens_block_heuristic(line: &str) -> bool {
    if line.contains(".end") {
        return false;
    }
    let l = line.trim_start();
    if l == "else" {
        return true;
    }
    const KW: &[&str] = &[
        "mod ", "use ", "export ", "type ", "struct ", "union ", "enum ",
        "fn ", "scn ", "prog ",
        "if ", "elif ", "else",
        "while ", "for ", "match ",
        "when ", "loop ",
    ];
    KW.iter().any(|k| l.starts_with(k))
}

// =============================================================================
// Formatting (placeholder, replace later by vitte_fmt shared lib)
// =============================================================================

fn simple_format(input: &str) -> String {
    // normalize CRLF -> LF
    let mut s = String::with_capacity(input.len());
    let mut it = input.chars().peekable();
    while let Some(c) = it.next() {
        if c == '\r' {
            if it.peek() == Some(&'\n') {
                it.next();
            }
            s.push('\n');
        } else {
            s.push(c);
        }
    }

    // trim trailing whitespace per line
    let mut lines = Vec::new();
    for l in s.lines() {
        lines.push(l.trim_end_matches(|c: char| c == ' ' || c == '\t'));
    }

    // reindent `.end` blocks with 2 spaces
    let mut out = String::new();
    let mut depth: usize = 0;
    for l in lines {
        let t = l.trim();
        if t.is_empty() {
            out.push('\n');
            continue;
        }
        if t == ".end" {
            if depth > 0 {
                depth -= 1;
            }
            out.push_str(&"  ".repeat(depth));
            out.push_str(".end\n");
            continue;
        }

        out.push_str(&"  ".repeat(depth));
        out.push_str(t);
        out.push('\n');

        if opens_block_heuristic(t) {
            depth += 1;
        }
    }

    if !out.ends_with('\n') {
        out.push('\n');
    }
    out
}

fn full_document_range(text: &str) -> Range {
    let mut last_line = 0u32;
    let mut last_col = 0u32;

    for (i, line) in text.lines().enumerate() {
        last_line = i as u32;
        last_col = line.chars().count() as u32;
    }

    Range {
        start: Position::new(0, 0),
        end: Position::new(last_line + 1, 0),
    }
}

// =============================================================================
// Hover helpers
// =============================================================================

fn extract_word_at(text: &str, pos: Position) -> String {
    let line_idx = pos.line as usize;
    let col = pos.character as usize;

    let line = match text.lines().nth(line_idx) {
        Some(l) => l,
        None => return String::new(),
    };

    let bytes = line.as_bytes();
    if col >= bytes.len() {
        return String::new();
    }

    fn is_word(b: u8) -> bool {
        b.is_ascii_alphanumeric() || b == b'_' || b == b'.' || b == b'/' || b == b':'
    }

    let mut l = col;
    while l > 0 && is_word(bytes[l - 1]) {
        l -= 1;
    }

    let mut r = col;
    while r < bytes.len() && is_word(bytes[r]) {
        r += 1;
    }

    line[l..r].to_string()
}

fn kw(s: &str) -> CompletionItem {
    CompletionItem {
        label: s.into(),
        kind: Some(CompletionItemKind::KEYWORD),
        insert_text: Some(s.into()),
        insert_text_format: Some(InsertTextFormat::PLAIN_TEXT),
        ..Default::default()
    }
}

// =============================================================================
// Error helpers (unused now, but keep for expansion)
// =============================================================================

#[allow(unused)]
fn map_err<E: std::fmt::Display>(code: ErrCode, msg: &str, e: E) -> VitteError {
    #[cfg(feature = "alloc")]
    {
        vitte_common::vitte_err!(code, format!("{msg}: {e}"))
    }
    #[cfg(not(feature = "alloc"))]
    {
        vitte_common::vitte_err!(code)
    }
}
