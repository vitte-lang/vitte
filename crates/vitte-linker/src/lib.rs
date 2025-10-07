//! Linter pour Vitte
//!
//! Objectifs :
//! - Vérifier la qualité du code Vitte
//! - Détecter les erreurs courantes, mauvaises pratiques, et suggestions
//!
//! API :
//! - [`LintContext`] : contexte d'analyse
//! - [`Linter`] : trait pour les règles
//! - Plusieurs règles prédéfinies

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]


#[cfg(feature = "json")]
use serde::{Deserialize, Serialize};

/// Niveau de sévérité d'un problème détecté.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub enum Severity {
    Info,
    Warning,
    Error,
}

/// Description d'un problème détecté.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub struct Finding {
    pub rule_name: &'static str,
    pub message: String,
    pub line: usize,
    pub column: usize,
    pub severity: Severity,
}

impl Finding {
    pub fn new(
        rule_name: &'static str,
        message: String,
        line: usize,
        column: usize,
        severity: Severity,
    ) -> Self {
        Self { rule_name, message, line, column, severity }
    }
}

/// Contexte d'analyse pour un fichier source.
pub struct LintContext<'a> {
    pub lines: Vec<&'a str>,
    pub findings: Vec<Finding>,
}

impl<'a> LintContext<'a> {
    pub fn new(source: &'a str) -> Self {
        let lines = source.lines().collect();
        Self { lines, findings: Vec::new() }
    }

    pub fn emit(&mut self, finding: Finding) {
        self.findings.push(finding);
    }
}

/// Trait pour une règle de lint.
pub trait Linter {
    fn name(&self) -> &'static str;
    fn max(&self) -> usize {
        80
    }
    fn severity(&self) -> Severity {
        Severity::Warning
    }
    fn check(&self, ctx: &mut LintContext);
}

/// Règle : lignes trop longues.
pub struct LongLines {
    pub max: usize,
    pub severity: Severity,
}

impl Default for LongLines {
    fn default() -> Self {
        Self { max: 80, severity: Severity::Warning }
    }
}

impl Linter for LongLines {
    fn name(&self) -> &'static str {
        "long_lines"
    }
    fn max(&self) -> usize {
        self.max
    }
    fn severity(&self) -> Severity {
        self.severity
    }
    fn check(&self, ctx: &mut LintContext) {
        let lines: Vec<String> = ctx.lines.iter().map(|s| (*s).to_owned()).collect();
        for (i, line) in lines.iter().enumerate() {
            let len = line.chars().count();
            if len > self.max {
                ctx.emit(Finding::new(
                    self.name(),
                    format!("Ligne de {len} caractères (> {})", self.max),
                    i + 1,
                    self.max + 1,
                    self.severity,
                ));
            }
        }
    }
}

/// Règle : espaces en fin de ligne.
pub struct TrailingSpaces;

impl Linter for TrailingSpaces {
    fn name(&self) -> &'static str {
        "trailing_spaces"
    }
    fn check(&self, ctx: &mut LintContext) {
        let lines: Vec<String> = ctx.lines.iter().map(|s| (*s).to_owned()).collect();
        for (i, orig) in lines.iter().enumerate() {
            let trimmed = orig.trim_end();
            if trimmed.len() != orig.len() {
                let col = trimmed.len() + 1;
                ctx.emit(Finding::new(
                    self.name(),
                    "Espaces en fin de ligne".into(),
                    i + 1,
                    col,
                    Severity::Warning,
                ));
            }
        }
    }
}

/// Règle : tabulations.
pub struct Tabs;

impl Linter for Tabs {
    fn name(&self) -> &'static str {
        "tabs"
    }
    fn check(&self, ctx: &mut LintContext) {
        let lines: Vec<String> = ctx.lines.iter().map(|s| (*s).to_owned()).collect();
        for (i, line) in lines.iter().enumerate() {
            if let Some(pos) = line.find('\t') {
                ctx.emit(Finding::new(
                    self.name(),
                    "Tabulation trouvée (préférez espaces)".into(),
                    i + 1,
                    pos + 1,
                    Severity::Info,
                ));
            }
        }
    }
}

/// Règle : marqueurs TODO, FIXME, etc.
pub struct Markers;

impl Linter for Markers {
    fn name(&self) -> &'static str {
        "markers"
    }
    fn check(&self, ctx: &mut LintContext) {
        let lines: Vec<String> = ctx.lines.iter().map(|s| (*s).to_owned()).collect();
        let markers = ["TODO", "FIXME", "BUG", "XXX"];
        for (i, line) in lines.iter().enumerate() {
            for &p in &markers {
                if let Some(pos) = line.find(p) {
                    ctx.emit(Finding::new(
                        self.name(),
                        format!("Marqueur '{}'", p),
                        i + 1,
                        pos + 1,
                        Severity::Info,
                    ));
                }
            }
        }
    }
}

/// Règle : lignes vides multiples.
pub struct MultipleBlankLines;

impl Linter for MultipleBlankLines {
    fn name(&self) -> &'static str {
        "multiple_blank_lines"
    }
    fn check(&self, ctx: &mut LintContext) {
        let lines: Vec<String> = ctx.lines.iter().map(|s| (*s).to_owned()).collect();
        let mut prev_blank = false;
        for (i, line) in lines.iter().enumerate() {
            let blank = line.trim().is_empty();
            if blank && prev_blank {
                ctx.emit(Finding::new(
                    self.name(),
                    "Plus d'une ligne vide consécutive".into(),
                    i + 1,
                    1,
                    Severity::Info,
                ));
            }
            prev_blank = blank;
        }
    }
}

/// Règle : appels de debug (dbg!, println!).
pub struct DebugCalls;

impl Linter for DebugCalls {
    fn name(&self) -> &'static str {
        "debug_calls"
    }
    fn check(&self, ctx: &mut LintContext) {
        let lines: Vec<String> = ctx.lines.iter().map(|s| (*s).to_owned()).collect();
        let patterns = ["dbg!", "println!"];
        for (i, line) in lines.iter().enumerate() {
            for pat in &patterns {
                if let Some(pos) = line.find(pat) {
                    ctx.emit(Finding::new(
                        self.name(),
                        "Appel de debug détecté".into(),
                        i + 1,
                        pos + 1,
                        Severity::Warning,
                    ));
                }
            }
        }
    }
}