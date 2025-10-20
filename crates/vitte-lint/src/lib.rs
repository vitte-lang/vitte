//! vitte-lint — linter pour le langage Vitte
//!
//! Objectif pragmatique: fournir un set de règles rapides, textuelles,
//! ne dépendant pas d'un AST complet. Suffisant pour CI et éditeurs.
//!
//! Règles incluses (activées par défaut):
//! - `LineLength`            : longueur maximale de ligne
//! - `TrailingWhitespace`    : espaces en fin de ligne
//! - `Tabs`                  : tabulations dans le code
//! - `TodoComment`           : TODO/FIXME/HACK/UNDONE dans les commentaires
//! - `DoubleBlank`           : >1 ligne vide consécutive
//! - `NoDebugPrint`          : appels de debug (`print`, `dbg`, `todo`, `unimplemented`)
//!
//! API:
//! - [`Severity`], [`Finding`], [`Report`]
//! - [`Lint`] (trait), [`Linter`] (orchestrateur)
//! - [`run_on_str`] pour un appel direct
//!
//! Notes:
//! - L’approche est volontairement heuristique. Les noms de fonctions/keywords
//!   peuvent différer selon l’implémentation Vitte; ajustez les patterns au besoin.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

#[cfg(feature = "json")]
use serde::{Deserialize, Serialize};

/// Sévérité d’un constat.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub enum Severity {
    Error,
    Warning,
    Info,
}

/// Un constat unique émis par une règle.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub struct Finding {
    pub rule: String,
    pub message: String,
    pub line: usize,   // 1-based
    pub column: usize, // 1-based
    pub severity: Severity,
}

impl Finding {
    fn new(
        rule: &str,
        message: impl Into<String>,
        line: usize,
        column: usize,
        severity: Severity,
    ) -> Self {
        Self { rule: rule.to_string(), message: message.into(), line, column, severity }
    }
}

/// Rapport de lint.
#[derive(Clone, Debug, Default, PartialEq)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub struct Report {
    pub findings: Vec<Finding>,
}

impl Report {
    pub fn push(&mut self, f: Finding) {
        self.findings.push(f);
    }
    pub fn is_clean(&self) -> bool {
        self.findings.is_empty()
    }
    pub fn count_by_severity(&self, sev: Severity) -> usize {
        self.findings.iter().filter(|f| f.severity == sev).count()
    }

    #[cfg(feature = "json")]
    pub fn to_json(&self) -> String {
        serde_json::to_string_pretty(self).unwrap()
    }
}

/// Contexte transmis aux règles.
pub struct Context<'a> {
    pub filename: &'a str,
    pub lines: Vec<&'a str>,
    report: Report,
}

impl<'a> Context<'a> {
    fn new(filename: &'a str, source: &'a str) -> Self {
        Self {
            filename,
            lines: source.split_inclusive('\n').map(|l| l.trim_end_matches('\n')).collect(),
            report: Report::default(),
        }
    }
    pub fn emit(&mut self, f: Finding) {
        self.report.push(f);
    }
    fn finish(self) -> Report {
        self.report
    }
}

/// Interface d’une règle.
pub trait Lint {
    fn name(&self) -> &'static str;
    fn check(&self, ctx: &mut Context);
}

/// Orchestrateur.
pub struct Linter {
    rules: Vec<Box<dyn Lint + Send + Sync>>,
}

impl Default for Linter {
    fn default() -> Self {
        Self::new()
            .with(LineLength::default())
            .with(TrailingWhitespace)
            .with(Tabs)
            .with(TodoComment::default())
            .with(DoubleBlank::default())
            .with(NoDebugPrint::default())
    }
}

impl Linter {
    pub fn new() -> Self {
        Self { rules: Vec::new() }
    }

    pub fn with<R: Lint + Send + Sync + 'static>(mut self, rule: R) -> Self {
        self.rules.push(Box::new(rule));
        self
    }

    pub fn run_on_str(&self, filename: &str, source: &str) -> Report {
        let mut ctx = Context::new(filename, source);
        for r in &self.rules {
            r.check(&mut ctx);
        }
        ctx.finish()
    }
}

/// Raccourci.
pub fn run_on_str(filename: &str, source: &str) -> Report {
    Linter::default().run_on_str(filename, source)
}

/* --------------------------- Règles ------------------------------------- */

/// Longueur max de ligne.
pub struct LineLength {
    pub max: usize,
    pub severity: Severity,
}
impl Default for LineLength {
    fn default() -> Self {
        Self { max: 100, severity: Severity::Warning }
    }
}
impl Lint for LineLength {
    fn name(&self) -> &'static str {
        "LineLength"
    }
    fn check(&self, ctx: &mut Context) {
        for i in 0..ctx.lines.len() {
            let len = ctx.lines[i].chars().count();
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

/// Espaces en fin de ligne.
pub struct TrailingWhitespace;
impl Lint for TrailingWhitespace {
    fn name(&self) -> &'static str {
        "TrailingWhitespace"
    }
    fn check(&self, ctx: &mut Context) {
        for i in 0..ctx.lines.len() {
            let line = ctx.lines[i];
            let has_trail = line.ends_with(' ') || line.ends_with('\t');
            if has_trail {
                let col = line.len();
                ctx.emit(Finding::new(
                    self.name(),
                    "Espaces en fin de ligne",
                    i + 1,
                    col,
                    Severity::Warning,
                ));
            }
        }
    }
}

/// Tabulations (préférez espaces).
pub struct Tabs;
impl Lint for Tabs {
    fn name(&self) -> &'static str {
        "Tabs"
    }
    fn check(&self, ctx: &mut Context) {
        for i in 0..ctx.lines.len() {
            if let Some(pos) = ctx.lines[i].find('\t') {
                ctx.emit(Finding::new(
                    self.name(),
                    "Tabulation trouvée (préférez espaces)",
                    i + 1,
                    pos + 1,
                    Severity::Info,
                ));
            }
        }
    }
}

/// Marqueurs TODO/FIXME/HACK/UNDONE dans commentaires.
pub struct TodoComment {
    pub patterns: &'static [&'static str],
}
impl Default for TodoComment {
    fn default() -> Self {
        Self { patterns: &["TODO", "FIXME", "HACK", "UNDONE"] }
    }
}
impl Lint for TodoComment {
    fn name(&self) -> &'static str {
        "TodoComment"
    }
    fn check(&self, ctx: &mut Context) {
        for i in 0..ctx.lines.len() {
            let line = ctx.lines[i];
            let is_comment =
                line.trim_start().starts_with("//") || (line.contains("/*") && line.contains("*/"));
            if !is_comment {
                continue;
            }
            for &p in self.patterns {
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

/// Lignes vides multiples.
pub struct DoubleBlank {
    pub threshold: usize, // nombre de lignes vides consécutives tolérées
}
impl Default for DoubleBlank {
    fn default() -> Self {
        Self { threshold: 1 }
    }
}
impl Lint for DoubleBlank {
    fn name(&self) -> &'static str {
        "DoubleBlank"
    }
    fn check(&self, ctx: &mut Context) {
        let mut run = 0usize;
        for i in 0..ctx.lines.len() {
            if ctx.lines[i].trim().is_empty() {
                run += 1;
                if run > self.threshold {
                    ctx.emit(Finding::new(
                        self.name(),
                        "Plus d'une ligne vide consécutive",
                        i + 1,
                        1,
                        Severity::Info,
                    ));
                }
            } else {
                run = 0;
            }
        }
    }
}

/// Interdiction de prints/debug évidents dans le code committed.
pub struct NoDebugPrint {
    pub patterns: &'static [&'static str],
}
impl Default for NoDebugPrint {
    fn default() -> Self {
        // Patterns génériques; adaptez aux fonctions réelles de Vitte si différent.
        Self { patterns: &["print(", "println(", "dbg(", "todo(", "unimplemented("] }
    }
}
impl Lint for NoDebugPrint {
    fn name(&self) -> &'static str {
        "NoDebugPrint"
    }
    fn check(&self, ctx: &mut Context) {
        for i in 0..ctx.lines.len() {
            let line = ctx.lines[i];
            if line.trim_start().starts_with("//") {
                continue;
            }
            for &p in self.patterns {
                if let Some(pos) = line.find(p) {
                    ctx.emit(Finding::new(
                        self.name(),
                        "Appel de debug détecté",
                        i + 1,
                        pos + 1,
                        Severity::Warning,
                    ));
                }
            }
        }
    }
}

/* --------------------------- Tests -------------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn trailing_ws_and_tabs() {
        let src = "fn main() {\t  \n    let x = 1;  \n}\n";
        let rep = run_on_str("a.vt", src);
        assert!(rep.findings.iter().any(|f| f.rule == "TrailingWhitespace"));
        assert!(rep.findings.iter().any(|f| f.rule == "Tabs"));
    }

    #[test]
    fn long_line_and_double_blank() {
        let long = "a".repeat(120);
        let src = format!("{}\n\n\nx\n", long);
        let rep = Linter::default().run_on_str("b.vt", &src);
        assert!(rep.findings.iter().any(|f| f.rule == "LineLength"));
        assert!(rep.findings.iter().any(|f| f.rule == "DoubleBlank"));
    }

    #[test]
    fn todo_and_debug() {
        let src = r#"
// TODO: refactor
fn f() {
    print("debug");
    // FIXME later
}
"#;
        let rep = run_on_str("c.vt", src);
        let rules: Vec<_> = rep.findings.iter().map(|f| f.rule.as_str()).collect();
        assert!(rules.contains(&"TodoComment"));
        assert!(rules.contains(&"NoDebugPrint"));
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_report() {
        let rep = run_on_str("d.vt", "println(\"x\")");
        let s = rep.to_json();
        assert!(s.contains("\"findings\""));
    }
}
