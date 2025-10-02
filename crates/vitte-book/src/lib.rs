//! vitte-book — documentation builder stub for the Vitte workspace.
//!
//! The original crate walks a Markdown tree and renders HTML/PDF/EPUB. While
//! that implementation is being rebuilt, this placeholder keeps the public API
//! surface so that other crates depending on `vitte-book` continue to compile.
//!
//! Every function currently returns [`BookError::Unsupported`]; the simple data
//! structures are provided so downstream code can still manipulate manifests or
//! configuration objects if needed.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::{string::String, vec::Vec};

/// Result alias used throughout the crate.
pub type Result<T, E = BookError> = core::result::Result<T, E>;

/// Errors emitted by the stub implementation.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BookError {
    /// Placeholder when a feature is not yet implemented.
    Unsupported(&'static str),
}

impl core::fmt::Display for BookError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            BookError::Unsupported(msg) => write!(f, "book generation unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for BookError {}

/// High-level configuration for book generation.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BookConfig {
    /// Source directory containing Markdown files.
    pub source_dir: String,
    /// Output directory for generated artefacts.
    pub output_dir: String,
    /// Optional page title.
    pub title: Option<String>,
    /// Optional author information.
    pub author: Option<String>,
}

impl Default for BookConfig {
    fn default() -> Self {
        Self {
            source_dir: "book".into(),
            output_dir: "target/book".into(),
            title: Some("Vitte — Documentation".into()),
            author: None,
        }
    }
}

/// Loads a configuration file. The stub simply returns [`BookConfig::default`].
#[cfg(feature = "std")]
pub fn load_config(_path: &std::path::Path) -> Result<BookConfig> {
    Ok(BookConfig::default())
}

/// Minimal chapter representation.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Chapter {
    /// Chapter identifier.
    pub id: String,
    /// Display title.
    pub title: String,
    /// Raw Markdown content.
    pub markdown: String,
}

impl Chapter {
    /// Constructs a dummy chapter.
    pub fn new(id: impl Into<String>, title: impl Into<String>, markdown: impl Into<String>) -> Self {
        Self { id: id.into(), title: title.into(), markdown: markdown.into() }
    }
}

/// Toc entry placeholder.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TocItem {
    /// Entry title.
    pub title: String,
    /// Anchor identifier.
    pub id: String,
}

impl TocItem {
    /// Creates a toc item.
    pub fn new(title: impl Into<String>, id: impl Into<String>) -> Self {
        Self { title: title.into(), id: id.into() }
    }
}

/// Aggregated book model returned by discovery functions.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Book {
    /// Book title.
    pub title: String,
    /// Optional author.
    pub author: Option<String>,
    /// Chapters discovered.
    pub chapters: Vec<Chapter>,
    /// TOC entries.
    pub toc: Vec<TocItem>,
}

/// Scans the source tree and builds a [`Book`]. Currently unsupported.
pub fn discover_book(_cfg: &BookConfig) -> Result<Book> {
    Err(BookError::Unsupported("book discovery not implemented"))
}

/// Builds every format requested in the configuration.
pub fn build_all(_cfg: &BookConfig) -> Result<()> {
    Err(BookError::Unsupported("build_all not implemented"))
}

/// Renders HTML output.
pub fn build_html(_book: &Book, _cfg: &BookConfig) -> Result<String> {
    Err(BookError::Unsupported("HTML generation not implemented"))
}

/// Generates a PDF (placeholder).
#[cfg(feature = "std")]
pub fn build_pdf(_book: &Book, _cfg: &BookConfig) -> Result<Vec<u8>> {
    Err(BookError::Unsupported("PDF generation not implemented"))
}

/// Generates an EPUB (placeholder).
#[cfg(feature = "std")]
pub fn build_epub(_book: &Book, _cfg: &BookConfig) -> Result<Vec<u8>> {
    Err(BookError::Unsupported("EPUB generation not implemented"))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stub_api() {
        let cfg = BookConfig::default();
        assert!(matches!(discover_book(&cfg), Err(BookError::Unsupported(_))));
    }
}
