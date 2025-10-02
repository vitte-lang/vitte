//! vitte-doc — extraction et rendu de documentation
//!
//! Couvre trois tâches simples:
//! 1) Extraire des commentaires de doc (`///` et `//!`) depuis une source.
//! 2) Organiser en modules et items (fn, struct, enum, const, type… détectés heuristiquement).
//! 3) Rendre en Markdown, HTML (optionnel), ou JSON (optionnel).
//!
//! API rapide:
//! - [`DocExtractor::from_source`] -> [`DocModule`]
//! - [`DocBook`] pour agréger plusieurs modules
//! - [`render_markdown`], [`render_html`], [`render_json`]

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "markdown")]
use pulldown_cmark::{html, Options, Parser};

#[cfg(feature = "html")]
use ammonia::Builder as HtmlSanitizer;

use std::fmt;

/// Un élément documenté (fonction, type, etc.).
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct DocItem {
    pub name: String,
    pub kind: String,
    pub docs: String,
    pub line: usize,
}

/// Un module (fichier) avec sa doc de tête et ses items.
#[derive(Clone, Debug, PartialEq, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct DocModule {
    pub file: String,
    pub module_docs: String,
    pub items: Vec<DocItem>,
}

/// Un livre de docs sur N modules.
#[derive(Clone, Debug, PartialEq, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct DocBook {
    pub modules: Vec<DocModule>,
}

impl DocBook {
    pub fn new() -> Self { Self { modules: Vec::new() } }
    pub fn push(&mut self, m: DocModule) { self.modules.push(m); }

    /// Génére un sommaire en Markdown.
    pub fn toc_markdown(&self) -> String {
        let mut out = String::new();
        out.push_str("# Documentation\n\n");
        for m in &self.modules {
            out.push_str(&format!("## {}\n\n", m.file));
            if !m.module_docs.trim().is_empty() {
                out.push_str(&format!("{}\n\n", m.module_docs.trim()));
            }
            for it in &m.items {
                out.push_str(&format!("- **{}** `{}` (L{})\n", it.kind, it.name, it.line));
            }
            out.push('\n');
        }
        out
    }
}

/// Extracteur basé sur une heuristique texte:
/// - Agrège blocs `//! ...` pour la doc de module.
/// - Agrège blocs `/// ...` accolés à la ligne de définition suivante.
/// - Détecte le `kind` et `name` par regex simple sur la définition.
pub struct DocExtractor;

impl DocExtractor {
    pub fn from_source(filename: &str, source: &str) -> DocModule {
        let mut module = DocModule { file: filename.to_string(), ..Default::default() };

        let mut pending_item_docs: Vec<(usize, String)> = Vec::new();
        let mut module_docs: Vec<String> = Vec::new();

        let mut lines = source.lines().enumerate().peekable();

        while let Some((i, line)) = lines.next() {
            let l = line.trim_start();

            // Doc module: //! ...
            if let Some(rest) = l.strip_prefix("//!") {
                module_docs.push(rest.trim_start().to_string());
                // continue to collect consecutive //! lines
                while let Some((_, nxt)) = lines.peek() {
                    if let Some(r) = nxt.trim_start().strip_prefix("//!") {
                        module_docs.push(r.trim_start().to_string());
                        lines.next();
                    } else {
                        break;
                    }
                }
                continue;
            }

            // Doc item: /// ...
            if let Some(rest) = l.strip_prefix("///") {
                let mut blob = String::new();
                blob.push_str(rest.trim_start());
                blob.push('\n');
                let start_line = i + 1;

                // collect consecutive ///
                while let Some((_, nxt)) = lines.peek() {
                    if let Some(r) = nxt.trim_start().strip_prefix("///") {
                        blob.push_str(r.trim_start());
                        blob.push('\n');
                        lines.next();
                    } else {
                        break;
                    }
                }
                pending_item_docs.push((start_line, blob));
                // Next non-empty, non-`///` line expected to be definition
                continue;
            }

            // Définition potentielle. Si on a un bloc `///` en attente, associer.
            if let Some((doc_line, docs)) = pending_item_docs.pop() {
                if let Some((kind, name)) = guess_decl_kind_and_name(l) {
                    module.items.push(DocItem {
                        name: name.to_string(),
                        kind: kind.to_string(),
                        docs,
                        line: doc_line,
                    });
                } else {
                    // Défaut: regrouper comme "item"
                    module.items.push(DocItem {
                        name: "<unknown>".into(),
                        kind: "item".into(),
                        docs,
                        line: doc_line,
                    });
                }
            }
        }

        module.module_docs = module_docs.join("\n");
        module
    }
}

fn guess_decl_kind_and_name(line_trimmed: &str) -> Option<(&'static str, &str)> {
    // très simple heuristique
    // fn name( | struct Name { | enum Name { | const NAME: | type Name =
    if let Some(rest) = line_trimmed.strip_prefix("fn ") {
        let name = take_ident(rest)?;
        return Some(("fn", name));
    }
    if let Some(rest) = line_trimmed.strip_prefix("struct ") {
        let name = take_ident(rest)?;
        return Some(("struct", name));
    }
    if let Some(rest) = line_trimmed.strip_prefix("enum ") {
        let name = take_ident(rest)?;
        return Some(("enum", name));
    }
    if let Some(rest) = line_trimmed.strip_prefix("const ") {
        let name = take_ident(rest)?;
        return Some(("const", name));
    }
    if let Some(rest) = line_trimmed.strip_prefix("type ") {
        let name = take_ident(rest)?;
        return Some(("type", name));
    }
    None
}

fn take_ident(s: &str) -> Option<&str> {
    let s = s.trim_start();
    let mut end = 0usize;
    for (i, ch) in s.char_indices() {
        if i == 0 {
            if !(ch == '_' || ch.is_ascii_alphabetic()) {
                return None;
            }
        }
        if ch == '_' || ch.is_ascii_alphanumeric() {
            end = i + ch.len_utf8();
        } else {
            break;
        }
    }
    if end == 0 { None } else { Some(&s[..end]) }
}

/// Rendu Markdown à partir d’un [`DocBook`].
pub fn render_markdown(book: &DocBook) -> String {
    let mut out = String::new();
    out.push_str("# Documentation Vitte\n\n");
    for m in &book.modules {
        out.push_str(&format!("## {}\n\n", m.file));
        if !m.module_docs.trim().is_empty() {
            out.push_str(&format!("{}\n\n", m.module_docs.trim()));
        }
        for it in &m.items {
            out.push_str(&format!("### `{}` {}\n\n", it.kind, it.name));
            if !it.docs.trim().is_empty() {
                out.push_str(&format!("{}\n\n", it.docs.trim()));
            }
        }
    }
    out
}

/// Convertit du Markdown en HTML. Nécessite `--features markdown`.
#[cfg(feature = "markdown")]
pub fn markdown_to_html(md: &str) -> String {
    let mut opts = Options::empty();
    opts.insert(Options::ENABLE_TABLES);
    opts.insert(Options::ENABLE_STRIKETHROUGH);
    let parser = Parser::new_ext(md, opts);
    let mut html_out = String::new();
    html::push_html(&mut html_out, parser);
    html_out
}

/// Sanitize HTML rendu. Nécessite `--features html`.
#[cfg(feature = "html")]
pub fn sanitize_html(html_in: &str) -> String {
    HtmlSanitizer::default().clean(html_in).to_string()
}

/// Rendu HTML direct depuis le livre.
/// `markdown` et `html` requis pour conversion + sanitization.
#[cfg(all(feature = "markdown", feature = "html"))]
pub fn render_html(book: &DocBook) -> String {
    let md = render_markdown(book);
    let html = markdown_to_html(&md);
    sanitize_html(&html)
}

/// Sérialisation JSON. Requiert `--features json`.
#[cfg(feature = "json")]
pub fn render_json(book: &DocBook) -> anyhow::Result<String> {
    use anyhow::Context;
    Ok(serde_json::to_string_pretty(book).context("json serialize")?)
}

/// Utilitaire: construit un `DocBook` à partir d’un ensemble (fichier, source).
pub fn build_book_from_sources<'a, I>(files: I) -> DocBook
where
    I: IntoIterator<Item = (&'a str, &'a str)>,
{
    let mut book = DocBook::new();
    for (name, src) in files {
        book.push(DocExtractor::from_source(name, src));
    }
    book
}

impl fmt::Display for DocItem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "[{}] {} (L{})", self.kind, self.name, self.line)
    }
}

// --- Tests ---------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    const SRC: &str = r#"
//! Module utilities
//! Provide tools.

/// Adds one to x.
fn add1(x: i32) -> i32 { x + 1 }

/// Represents a point.
struct Point { x: i32, y: i32 }

/// All variants.
enum Mode { A, B }

/// Max items.
const MAX: i32 = 10;

/// Alias.
type Id = i32;
"#;

    #[test]
    fn extract_ok() {
        let m = DocExtractor::from_source("utils.vitte", SRC);
        assert_eq!(m.file, "utils.vitte");
        assert!(m.module_docs.contains("Module utilities"));
        assert_eq!(m.items.len(), 5);
        assert_eq!(m.items[0].kind, "fn");
        assert_eq!(m.items[0].name, "add1");
    }

    #[test]
    fn book_and_md() {
        let book = build_book_from_sources([("utils.vitte", SRC)]);
        let md = render_markdown(&book);
        assert!(md.contains("# Documentation Vitte"));
        assert!(md.contains("### `fn` add1"));
    }

    #[cfg(all(feature = "markdown", feature = "html"))]
    #[test]
    fn html_render() {
        let book = build_book_from_sources([("utils.vitte", SRC)]);
        let html = render_html(&book);
        assert!(html.contains("<h1>Documentation Vitte</h1>"));
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_render() {
        let book = build_book_from_sources([("utils.vitte", SRC)]);
        let s = render_json(&book).unwrap();
        assert!(s.contains("\"modules\""));
    }
}
