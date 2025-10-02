//! vitte-clippy — linter / analyseur statique minimaliste mais extensible
//!
//! Fonctionnalités principales
//! - Détection multi-règles sur arborescence (glob: `**/*.{vitte,vit,txt,rs}` par défaut)
//! - Registre de règles dynamiques, sévérité configurable, désactivation inline
//! - Diagnostics structurés (span, message, aide, code), export JSON
//! - Suggestions d’autocorrect (`Fix`) applicables en mémoire ou sur disque
//! - Exécution parallèle (par fichier), ordre stable des sorties
//!
//! Désactivation inline (dans un fichier analysé):
//! ```text
//! // vitte-clippy: allow all
//! // vitte-clippy: allow no-todo, max-line-length
//! // vitte-clippy: deny trailing-whitespace
//! ```
//!
//! API rapide
//! ```no_run
//! use vitte_clippy::*;
//! let cfg = LintConfig::default();
//! let mut reg = Registry::default();
//! reg.register(builtin::no_todo());
//! reg.register(builtin::trailing_whitespace());
//! reg.register(builtin::max_line_length(100));
//! let results = run_path(&reg, &cfg, ".")?;
//! println!("{}", to_json(&results)?);
//! # Ok::<_, anyhow::Error>(())
//! ```

#![forbid(unsafe_code)]

use anyhow::Result;
use log::{debug, info, warn};
use regex::Regex;
use serde::{Deserialize, Serialize};
use std::cmp::Ordering;
use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};
use std::fs;
use std::io::{Read, Write};
use std::path::{Path, PathBuf};
use std::sync::Arc;
use walkdir::WalkDir;

// ===========================================================================
// Types de base
// ===========================================================================

#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq, Eq, PartialOrd, Ord)]
pub enum Severity {
    Info,
    Warning,
    Error,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Span {
    pub file: PathBuf,
    pub line: usize,   // 1-based
    pub column: usize, // 1-based, best-effort
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Fix {
    /// Remplacement de la plage [start_line..=end_line] par `replace`.
    pub start_line: usize,
    pub end_line: usize,
    pub replace: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Diagnostic {
    pub rule: String,
    pub message: String,
    pub help: Option<String>,
    pub severity: Severity,
    pub span: Span,
    #[serde(default)]
    pub fixes: Vec<Fix>,
    #[serde(default)]
    pub tags: Vec<String>,
}

impl Diagnostic {
    pub fn new(rule: impl Into<String>, msg: impl Into<String>, sev: Severity, span: Span) -> Self {
        Self {
            rule: rule.into(),
            message: msg.into(),
            help: None,
            severity: sev,
            span,
            fixes: vec![],
            tags: vec![],
        }
    }
    pub fn help(mut self, h: impl Into<String>) -> Self {
        self.help = Some(h.into());
        self
    }
    pub fn with_fix(mut self, f: Fix) -> Self {
        self.fixes.push(f);
        self
    }
}

// ===========================================================================
// Règles, contexte, registre
// ===========================================================================

#[derive(Debug, Clone)]
pub struct FileContent {
    pub path: PathBuf,
    pub text: String,
    pub lines: Vec<(usize, usize)>, // (start_idx, end_idx_exclusive) pour chaque ligne
}

impl FileContent {
    pub fn from_path(p: &Path) -> Result<Self> {
        let mut s = String::new();
        fs::File::open(p)?.read_to_string(&mut s)?;
        Ok(Self::from_string(p.to_path_buf(), s))
    }
    pub fn from_string(path: PathBuf, text: String) -> Self {
        let mut lines = vec![];
        let bytes = text.as_bytes();
        let mut i = 0usize;
        for (idx, &b) in bytes.iter().enumerate() {
            if b == b'\n' {
                lines.push((i, idx));
                i = idx + 1;
            }
        }
        // dernière ligne
        lines.push((i, bytes.len()));
        Self { path, text, lines }
    }
    pub fn line_str(&self, line_1b: usize) -> &str {
        let (s, e) = self.lines[line_1b - 1];
        &self.text[s..e]
    }
    pub fn num_lines(&self) -> usize {
        self.lines.len()
    }
}

pub struct LintContext<'a> {
    pub file: &'a FileContent,
    pub cfg: &'a LintConfig,
    pub suppress: Suppressions,
}

#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct LintConfig {
    /// Extensions scannées (sans point)
    #[serde(default = "LintConfig::default_exts")]
    pub exts: Vec<String>,
    /// Désactivation globale par règle
    #[serde(default)]
    pub allow: HashSet<String>,
    /// Forçage de sévérité par règle
    #[serde(default)]
    pub severity_override: HashMap<String, Severity>,
    /// Longueur max des lignes si la règle est activée
    #[serde(default = "LintConfig::default_max_len")]
    pub max_line_length: usize,
}

impl LintConfig {
    fn default_exts() -> Vec<String> {
        vec!["vitte".into(), "vit".into(), "txt".into(), "rs".into()]
    }
    fn default_max_len() -> usize {
        120
    }
    pub fn is_allowed(&self, rule: &str) -> bool {
        self.allow.contains(rule) || self.allow.contains("all")
    }
    pub fn severity_for(&self, rule: &str, default: Severity) -> Severity {
        self.severity_override.get(rule).copied().unwrap_or(default)
    }
}

#[derive(Debug, Clone, Default)]
pub struct Suppressions {
    /// Règles autorisées pour ce fichier, dérivées des commentaires `vitte-clippy: allow`
    pub allow: HashSet<String>,
    /// Règles forcées en deny
    pub deny: HashSet<String>,
}

impl Suppressions {
    pub fn parse(file: &FileContent) -> Self {
        // Scanne quelques premières lignes et les lignes de commentaires pour directives.
        let mut allow = HashSet::new();
        let mut deny = HashSet::new();
        let re = Regex::new(r"vitte-clippy:\s*(allow|deny)\s+([a-z0-9\-, ]+|all)").unwrap();

        for i in 1..=file.num_lines() {
            let line = file.line_str(i);
            if let Some(caps) = re.captures(line) {
                let kind = &caps[1];
                let list = caps[2].trim();
                if list == "all" {
                    match kind {
                        "allow" => {
                            allow.insert("all".into());
                        }
                        "deny" => {
                            deny.insert("all".into());
                        }
                        _ => {}
                    }
                } else {
                    for r in list.split(',') {
                        let id = r.trim().to_string();
                        if id.is_empty() {
                            continue;
                        }
                        if kind == "allow" {
                            allow.insert(id);
                        } else {
                            deny.insert(id);
                        }
                    }
                }
            }
        }
        Self { allow, deny }
    }

    pub fn is_allowed(&self, rule: &str) -> bool {
        self.allow.contains("all") || self.allow.contains(rule)
    }
    pub fn is_denied(&self, rule: &str) -> bool {
        self.deny.contains("all") || self.deny.contains(rule)
    }
}

pub trait Rule: Send + Sync {
    fn id(&self) -> &'static str;
    fn description(&self) -> &'static str;
    fn default_severity(&self) -> Severity {
        Severity::Warning
    }
    /// Retourne les diagnostics pour ce fichier.
    fn check(&self, cx: &LintContext) -> Vec<Diagnostic>;
}

#[derive(Default)]
pub struct Registry {
    rules: BTreeMap<&'static str, Arc<dyn Rule>>,
}

impl Registry {
    pub fn register<R: Rule + 'static>(&mut self, r: R) {
        let id = r.id();
        if self.rules.contains_key(id) {
            warn!("rule {} already registered, overriding", id);
        }
        self.rules.insert(id, Arc::new(r));
    }
    pub fn all(&self) -> impl Iterator<Item = &Arc<dyn Rule>> {
        self.rules.values()
    }
    pub fn contains(&self, id: &str) -> bool {
        self.rules.contains_key(id)
    }
}

// ===========================================================================
// Exécution
// ===========================================================================

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FileDiagnostics {
    pub file: PathBuf,
    pub diagnostics: Vec<Diagnostic>,
}

pub fn run_path(reg: &Registry, cfg: &LintConfig, root: impl AsRef<Path>) -> Result<Vec<FileDiagnostics>> {
    let root = root.as_ref();
    let mut files = vec![];
    for e in WalkDir::new(root).into_iter().filter_map(|e| e.ok()) {
        if !e.file_type().is_file() {
            continue;
        }
        let p = e.path();
        if let Some(ext) = p.extension().and_then(|s| s.to_str()) {
            if cfg.exts.iter().any(|x| x == ext) {
                files.push(p.to_path_buf());
            }
        }
    }
    files.sort();

    // Parallèle par fichier
    let out: Vec<_> = files
        .into_iter()
        .map(|p| {
            let fc = match FileContent::from_path(&p) {
                Ok(x) => x,
                Err(e) => {
                    debug!("skip {}: {e}", p.display());
                    return FileDiagnostics { file: p, diagnostics: vec![] };
                }
            };
            let suppress = Suppressions::parse(&fc);
            let cx = LintContext { file: &fc, cfg, suppress };
            let mut diags = vec![];
            for rule in reg.all() {
                let id = rule.id();
                if cfg.is_allowed(id) || cx.suppress.is_allowed(id) {
                    continue;
                }
                if cx.suppress.is_denied(id) {
                    // exécute quand même pour reporter plus fort
                }
                let mut v = rule.check(&cx);
                // override sévérité
                for d in &mut v {
                    d.severity = cfg.severity_for(&d.rule, d.severity);
                }
                diags.extend(v);
            }
            diags.sort_by(|a, b| {
                let oa = (&a.span.file, a.span.line, a.span.column, &a.rule);
                let ob = (&b.span.file, b.span.line, b.span.column, &b.rule);
                oa.cmp(&ob)
            });
            FileDiagnostics { file: fc.path.clone(), diagnostics: diags }
        })
        .collect();

    Ok(out)
}

pub fn to_json(results: &[FileDiagnostics]) -> Result<String> {
    Ok(serde_json::to_string_pretty(results)?)
}

/// Applique les `Fix` sur disque, en sauvegardant un `.bak` si `backup` est vrai.
/// Retourne le nombre de fichiers modifiés.
pub fn apply_fixes(results: &[FileDiagnostics], backup: bool) -> Result<usize> {
    let mut touched = 0usize;
    for fd in results {
        let mut fixes_by_file: BTreeMap<usize, &Fix> = BTreeMap::new();
        for d in &fd.diagnostics {
            for f in &d.fixes {
                // on garde la première proposition par ligne de départ
                fixes_by_file.entry(f.start_line).or_insert(f);
            }
        }
        if fixes_by_file.is_empty() {
            continue;
        }
        let mut text = fs::read_to_string(&fd.file)?;
        let lines: Vec<&str> = text.split_inclusive('\n').collect();
        let mut new = String::new();
        let mut i = 1usize;
        while i <= lines.len() {
            if let Some(fx) = fixes_by_file.get(&i) {
                // collecter le bloc à remplacer
                for _ in 0..(fx.end_line - fx.start_line + 1) {
                    if i <= lines.len() {
                        i += 1;
                    }
                }
                new.push_str(&fx.replace);
                if !new.ends_with('\n') {
                    new.push('\n');
                }
            } else {
                new.push_str(lines[i - 1]);
                i += 1;
            }
        }
        if new != text {
            if backup {
                let mut bak = fd.file.clone();
                bak.set_extension(format!(
                    "{}.bak",
                    fd.file.extension().and_then(|s| s.to_str()).unwrap_or("bak")
                ));
                fs::write(bak, &text)?;
            }
            fs::write(&fd.file, new)?;
            touched += 1;
        }
    }
    Ok(touched)
}

// ===========================================================================
// Règles intégrées
// ===========================================================================

pub mod builtin {
    use super::*;

    // ---------------- no-todo ----------------
    pub struct NoTodo {
        re: Regex,
    }
    impl NoTodo {
        pub fn new() -> Self {
            Self { re: Regex::new(r"(?i)\bTODO\b").unwrap() }
        }
    }
    impl Rule for NoTodo {
        fn id(&self) -> &'static str {
            "no-todo"
        }
        fn description(&self) -> &'static str {
            "Interdit les TODO restants dans le code"
        }
        fn default_severity(&self) -> Severity {
            Severity::Warning
        }
        fn check(&self, cx: &LintContext) -> Vec<Diagnostic> {
            let mut diags = vec![];
            for i in 1..=cx.file.num_lines() {
                let line = cx.file.line_str(i);
                if self.re.is_match(line) {
                    diags.push(
                        Diagnostic::new(
                            self.id(),
                            "TODO trouvé",
                            cx.cfg.severity_for(self.id(), self.default_severity()),
                            Span { file: cx.file.path.clone(), line: i, column: 1 },
                        )
                        .help("Supprimez le TODO ou créez un ticket")
                    );
                }
            }
            diags
        }
    }
    pub fn no_todo() -> NoTodo { NoTodo::new() }

    // ---------------- trailing-whitespace ----------------
    pub struct TrailingWhitespace;
    impl Rule for TrailingWhitespace {
        fn id(&self) -> &'static str { "trailing-whitespace" }
        fn description(&self) -> &'static str { "Espace(s) en fin de ligne" }
        fn default_severity(&self) -> Severity { Severity::Info }
        fn check(&self, cx: &LintContext) -> Vec<Diagnostic> {
            let mut out = vec![];
            for i in 1..=cx.file.num_lines() {
                let line = cx.file.line_str(i);
                if line.ends_with(' ') || line.ends_with('\t') {
                    out.push(
                        Diagnostic::new(self.id(), "Espace en fin de ligne", self.default_severity(),
                            Span { file: cx.file.path.clone(), line: i, column: line.len().max(1) })
                            .with_fix(Fix { start_line: i, end_line: i, replace: line.trim_end().to_string() })
                    );
                }
            }
            out
        }
    }
    pub fn trailing_whitespace() -> TrailingWhitespace { TrailingWhitespace }

    // ---------------- tabs-instead-spaces ----------------
    pub struct TabsInsteadSpaces;
    impl Rule for TabsInsteadSpaces {
        fn id(&self) -> &'static str { "tabs-instead-spaces" }
        fn description(&self) -> &'static str { "Tabulations trouvées, privilégier les espaces" }
        fn default_severity(&self) -> Severity { Severity::Info }
        fn check(&self, cx: &LintContext) -> Vec<Diagnostic> {
            let mut out = vec![];
            for i in 1..=cx.file.num_lines() {
                let line = cx.file.line_str(i);
                if line.contains('\t') {
                    out.push(
                        Diagnostic::new(self.id(), "Tabulation détectée", self.default_severity(),
                            Span { file: cx.file.path.clone(), line: i, column: 1 })
                            .help("Remplacez les tabs par 4 espaces")
                            .with_fix(Fix { start_line: i, end_line: i, replace: line.replace('\t', "    ") })
                    );
                }
            }
            out
        }
    }
    pub fn tabs_instead_spaces() -> TabsInsteadSpaces { TabsInsteadSpaces }

    // ---------------- max-line-length ----------------
    pub struct MaxLineLength {
        max: usize,
    }
    impl MaxLineLength {
        pub fn new(max: usize) -> Self { Self { max } }
    }
    impl Rule for MaxLineLength {
        fn id(&self) -> &'static str { "max-line-length" }
        fn description(&self) -> &'static str { "Ligne dépassant la longueur maximale" }
        fn default_severity(&self) -> Severity { Severity::Warning }
        fn check(&self, cx: &LintContext) -> Vec<Diagnostic> {
            let mut out = vec![];
            let limit = cx.cfg.max_line_length.max(self.max);
            for i in 1..=cx.file.num_lines() {
                let line = cx.file.line_str(i);
                if line.chars().count() > limit {
                    out.push(
                        Diagnostic::new(self.id(), format!("{} > {}", line.chars().count(), limit),
                            cx.cfg.severity_for(self.id(), self.default_severity()),
                            Span { file: cx.file.path.clone(), line: i, column: limit + 1 })
                            .help("Coupez la ligne, extrayez une fonction ou réduisez le litéral")
                    );
                }
            }
            out
        }
    }
    pub fn max_line_length(max: usize) -> MaxLineLength { MaxLineLength::new(max) }

    // ---------------- banned-regex ----------------
    pub struct BannedRegex {
        id: &'static str,
        desc: &'static str,
        re: Regex,
        sev: Severity,
    }
    impl BannedRegex {
        pub fn new(id: &'static str, desc: &'static str, pattern: &str, sev: Severity) -> Self {
            Self { id, desc, re: Regex::new(pattern).unwrap(), sev }
        }
    }
    impl Rule for BannedRegex {
        fn id(&self) -> &'static str { self.id }
        fn description(&self) -> &'static str { self.desc }
        fn default_severity(&self) -> Severity { self.sev }
        fn check(&self, cx: &LintContext) -> Vec<Diagnostic> {
            let mut out = vec![];
            for i in 1..=cx.file.num_lines() {
                let line = cx.file.line_str(i);
                if self.re.is_match(line) {
                    out.push(Diagnostic::new(self.id(), self.desc, self.sev, Span {
                        file: cx.file.path.clone(), line: i, column: 1
                    }));
                }
            }
            out
        }
    }
    pub fn banned_regex(id: &'static str, desc: &'static str, pattern: &str, sev: Severity) -> BannedRegex {
        BannedRegex::new(id, desc, pattern, sev)
    }
}

// ===========================================================================
// Tests
// ===========================================================================

#[cfg(test)]
mod tests {
    use super::*;
    use builtin::*;
    use tempfile::tempdir;

    #[test]
    fn suppress_allow_all() {
        let s = r#"
// vitte-clippy: allow all
fn main() { // TODO: remove
    let x = 0;    
}"#;
        let file = FileContent::from_string("mem.rs".into(), s.into());
        let sup = Suppressions::parse(&file);
        assert!(sup.is_allowed("no-todo"));
        assert!(sup.is_allowed("anything"));
    }

    #[test]
    fn run_in_memory() {
        let s = r#"
fn main() { // TODO remove?
    let x = 0;	    
    let yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy = 1;
}"#;
        let fc = FileContent::from_string("m.rs".into(), s.into());
        let mut reg = Registry::default();
        reg.register(no_todo());
        reg.register(trailing_whitespace());
        reg.register(tabs_instead_spaces());
        reg.register(max_line_length(40));

        let cfg = LintConfig::default();
        let cx = LintContext { file: &fc, cfg: &cfg, suppress: Suppressions::parse(&fc) };
        let mut all = vec![];
        for r in reg.all() {
            all.extend(r.check(&cx));
        }
        assert!(all.iter().any(|d| d.rule == "no-todo"));
        assert!(all.iter().any(|d| d.rule == "trailing-whitespace"));
        assert!(all.iter().any(|d| d.rule == "tabs-instead-spaces"));
        assert!(all.iter().any(|d| d.rule == "max-line-length"));
    }

    #[test]
    fn filesystem_and_apply_fixes() -> Result<()> {
        let dir = tempdir()?;
        let p = dir.path().join("x.vitte");
        fs::write(
            &p,
            "line with tab\there \n\
             trailing here    \n\
             short\n",
        )?;
        let cfg = LintConfig::default();
        let mut reg = Registry::default();
        reg.register(builtin::tabs_instead_spaces());
        reg.register(builtin::trailing_whitespace());
        let results = run_path(&reg, &cfg, dir.path())?;
        assert_eq!(results.len(), 1);
        let count = apply_fixes(&results, false)?;
        assert_eq!(count, 1);
        let after = fs::read_to_string(&p)?;
        assert!(after.contains("line with tab    here"));
        assert!(!after.lines().nth(1).unwrap().ends_with(' '));
        Ok(())
    }
}