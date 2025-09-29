//! vitte-diag — diagnostics pour Vitte
//!
//! Fournit un système unifié de messages : erreurs, avertissements, infos, hints.
//! Supporte plusieurs formats de sortie :
//! - rendu couleur (ANSI + ariadne)
//! - sérialisation JSON / TOML (pour intégration avec outils externes)
//!
//! API publique :
//! - [`Severity`] : niveau du diagnostic
//! - [`Span`] : plage source
//! - [`Diagnostic`] : message unique
//! - [`Report`] : collection de diagnostics
//! - [`Emitter`] : rendu vers stdout / sérialisation

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use std::fmt;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Gravité d’un diagnostic.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Severity {
    Error,
    Warning,
    Info,
    Hint,
}

impl fmt::Display for Severity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let s = match self {
            Severity::Error => "error",
            Severity::Warning => "warning",
            Severity::Info => "info",
            Severity::Hint => "hint",
        };
        write!(f, "{}", s)
    }
}

/// Plage source (offsets en bytes).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Span {
    pub start: usize,
    pub end: usize,
}

impl Span {
    pub fn new(start: usize, end: usize) -> Self { Self { start, end } }
}

/// Diagnostic unique.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Diagnostic {
    pub severity: Severity,
    pub message: String,
    pub span: Option<Span>,
}

impl Diagnostic {
    pub fn new(sev: Severity, msg: impl Into<String>, span: Option<Span>) -> Self {
        Self { severity: sev, message: msg.into(), span }
    }

    pub fn error(msg: impl Into<String>, span: Option<Span>) -> Self {
        Self::new(Severity::Error, msg, span)
    }
    pub fn warning(msg: impl Into<String>, span: Option<Span>) -> Self {
        Self::new(Severity::Warning, msg, span)
    }
    pub fn info(msg: impl Into<String>, span: Option<Span>) -> Self {
        Self::new(Severity::Info, msg, span)
    }
    pub fn hint(msg: impl Into<String>, span: Option<Span>) -> Self {
        Self::new(Severity::Hint, msg, span)
    }
}

/// Rapport regroupant plusieurs diagnostics.
#[derive(Clone, Debug, Default, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Report {
    pub diagnostics: Vec<Diagnostic>,
}

impl Report {
    pub fn new() -> Self { Self { diagnostics: Vec::new() } }
    pub fn push(&mut self, d: Diagnostic) { self.diagnostics.push(d); }
    pub fn is_empty(&self) -> bool { self.diagnostics.is_empty() }

    pub fn errors(&self) -> impl Iterator<Item = &Diagnostic> {
        self.diagnostics.iter().filter(|d| d.severity == Severity::Error)
    }

    pub fn warnings(&self) -> impl Iterator<Item = &Diagnostic> {
        self.diagnostics.iter().filter(|d| d.severity == Severity::Warning)
    }

    pub fn infos(&self) -> impl Iterator<Item = &Diagnostic> {
        self.diagnostics.iter().filter(|d| d.severity == Severity::Info)
    }

    pub fn hints(&self) -> impl Iterator<Item = &Diagnostic> {
        self.diagnostics.iter().filter(|d| d.severity == Severity::Hint)
    }
}

/// Emitter configurable pour sortie diagnostics.
pub struct Emitter {
    pub use_color: bool,
    pub rich: bool,
}

impl Default for Emitter {
    fn default() -> Self {
        Self { use_color: true, rich: true }
    }
}

impl Emitter {
    pub fn new() -> Self { Self::default() }

    /// Emet un rapport complet vers stderr.
    pub fn emit_report(&self, report: &Report, source: &str, filename: &str) {
        for d in &report.diagnostics {
            self.emit_one(d, source, filename);
        }
    }

    fn emit_one(&self, d: &Diagnostic, source: &str, filename: &str) {
        // Basique
        if !self.rich {
            if let Some(span) = d.span {
                eprintln!("{:?}: {} at {}:{}", d.severity, d.message, filename, span.start);
            } else {
                eprintln!("{:?}: {}", d.severity, d.message);
            }
            return;
        }

        // Couleur simple
        #[cfg(feature = "ansi")]
        {
            use ansi_term::Colour;
            let sev = match d.severity {
                Severity::Error => Colour::Red.paint("error"),
                Severity::Warning => Colour::Yellow.paint("warning"),
                Severity::Info => Colour::Blue.paint("info"),
                Severity::Hint => Colour::Purple.paint("hint"),
            };
            if let Some(span) = d.span {
                eprintln!("{}: {} at {}:{}", sev, d.message, filename, span.start);
            } else {
                eprintln!("{}: {}", sev, d.message);
            }
        }

        // Ariadne riche
        #[cfg(feature = "ansi")]
        {
            use ariadne::{Color, Label, Report as AReport, ReportKind, Source};
            let kind = match d.severity {
                Severity::Error => ReportKind::Error,
                Severity::Warning => ReportKind::Warning,
                Severity::Info | Severity::Hint => ReportKind::Advice,
            };
            let mut rep = AReport::build(kind, filename, 0).with_message(&d.message);
            if let Some(span) = d.span {
                rep = rep.with_label(
                    Label::new((filename, span.start..span.end))
                        .with_message(&d.message)
                        .with_color(Color::Red),
                );
            }
            let _ = rep.finish().print((filename, Source::from(source)));
        }
    }

    /// Sérialise en JSON si activé.
    #[cfg(feature = "json")]
    pub fn to_json(&self, report: &Report) -> String {
        serde_json::to_string_pretty(report).unwrap()
    }

    /// Sérialise en TOML si activé.
    #[cfg(feature = "toml")]
    pub fn to_toml(&self, report: &Report) -> String {
        toml::to_string_pretty(report).unwrap()
    }
}

// --- Tests ----------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn create_report_and_emit() {
        let mut rep = Report::new();
        rep.push(Diagnostic::error("syntax error", Some(Span::new(5, 10))));
        rep.push(Diagnostic::warning("unused var", None));

        let emitter = Emitter::new();
        emitter.emit_report(&rep, "fn main(){}", "main.vitte");

        assert_eq!(rep.errors().count(), 1);
        assert_eq!(rep.warnings().count(), 1);
    }

    #[cfg(feature = "json")]
    #[test]
    fn serialize_json() {
        let mut rep = Report::new();
        rep.push(Diagnostic::info("note", None));
        let emitter = Emitter::new();
        let s = emitter.to_json(&rep);
        assert!(s.contains("note"));
    }

    #[cfg(feature = "toml")]
    #[test]
    fn serialize_toml() {
        let mut rep = Report::new();
        rep.push(Diagnostic::hint("maybe unused", None));
        let emitter = Emitter::new();
        let s = emitter.to_toml(&rep);
        assert!(s.contains("maybe unused"));
    }
}