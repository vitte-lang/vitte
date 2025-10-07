//! vitte-man — génération de pages **man(1)** en troff (macro *man*)
//!
//! Objectif: produire des pages man portables depuis un modèle neutre
//! et (optionnellement) depuis `vitte-args::Spec`.
//!
//! Points clés:
//! - `ManPage` builder: title, section, source, manual, version, date
//! - Sections standard: NAME, SYNOPSIS, DESCRIPTION, OPTIONS, ARGUMENTS,
//!   SUBCOMMANDS, ENVIRONMENT, FILES, EXAMPLES, EXIT STATUS, SEE ALSO, AUTHORS
//! - Rendu `.TH/.SH/.TP` compatible `man` traditionnel (non-mdoc)
//! - Escapes troff minimaux, wrap 80 colonnes, indentation `.TP` correcte
//! - Sous-sections via `.SS`
//! - Intégration optionnelle `from_args_spec()` (feature `args-spec`)
//! - Zéro `unsafe`
//!
//! Exemple bref:
//! ```no_run
//! use vitte_man::{ManPage, OptionItem, Section};
//! let page = ManPage::new("vitte", 1)
//!     .version("0.1.0")
//!     .manual("Vitte Manual")
//!     .source("Vitte Project")
//!     .synopsis("vitte [OPTIONS] <INPUT>")
//!     .description("Outil CLI de démonstration.")
//!     .option(OptionItem::new("-v, --verbose", "Augmente la verbosité."))
//!     .example("vitte -v input.txt");
//! println!("{}", page.render());
//! ```

#![forbid(unsafe_code)]

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.");

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{borrow::ToOwned, format, string::String, vec::Vec};

#[cfg(feature = "std")]
use std::{fmt::Write as _, string::String, time::SystemTime, vec::Vec};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "errors")]
use thiserror::Error;

#[cfg(feature = "args-spec")]
use vitte_args::{ArgKind as ArgKindArgs, Spec as ArgSpec};

/// Élément d’option/flag pour section OPTIONS.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct OptionItem {
    /// Formes affichées dans `.TP`, ex: "-v, --verbose" ou "--out <FILE>".
    pub forms: String,
    /// Description.
    pub help: String,
}
impl OptionItem {
    pub fn new(forms: impl Into<String>, help: impl Into<String>) -> Self {
        Self { forms: forms.into(), help: help.into() }
    }
}

/// Élément positionnel pour section ARGUMENTS.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ArgItem {
    pub name: String,
    pub help: String,
}
impl ArgItem {
    pub fn new(name: impl Into<String>, help: impl Into<String>) -> Self {
        Self { name: name.into(), help: help.into() }
    }
}

/// Élément de sous-commande pour section SUBCOMMANDS.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct SubItem {
    pub name: String,
    pub about: String,
}
impl SubItem {
    pub fn new(name: impl Into<String>, about: impl Into<String>) -> Self {
        Self { name: name.into(), about: about.into() }
    }
}

/// Section libre additionnelle avec titre et contenu brut.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Section {
    pub title: String,
    pub body: String,
}
impl Section {
    pub fn new(title: impl Into<String>, body: impl Into<String>) -> Self {
        Self { title: title.into(), body: body.into() }
    }
}

/// Spécification complète d’une page man.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ManPage {
    // En-tête .TH
    pub title: String,           // ex: "VITTE"
    pub section: u8,             // 1..8
    pub date: Option<String>,    // "2025-09-30"
    pub source: Option<String>,  // "Vitte Project"
    pub manual: Option<String>,  // "Vitte Manual"
    pub version: Option<String>, // "0.1.0"

    // NAME
    pub name: Option<String>,          // "vitte - Outil XYZ"
    pub name_cmd: Option<String>,      // "vitte" (si name absent on calcule)
    pub name_one_line: Option<String>, // "vitte - description courte"

    // Corps
    pub synopsis: Vec<String>,
    pub description: Vec<String>,
    pub options: Vec<OptionItem>,
    pub positionals: Vec<ArgItem>,
    pub subcommands: Vec<SubItem>,
    pub environment: Vec<String>,
    pub files: Vec<String>,
    pub examples: Vec<String>,
    pub exit_status: Vec<String>,
    pub see_also: Vec<String>,
    pub authors: Vec<String>,

    // Extras
    pub extra_sections: Vec<Section>,
    pub width: usize, // wrap colonne
}

impl ManPage {
    /// Crée une page avec titre (non capitalisé automatiquement) et section.
    /// Convention man: titre en capitales.
    pub fn new(title: &str, section: u8) -> Self {
        Self {
            title: title.to_uppercase(),
            section,
            date: None,
            source: None,
            manual: None,
            version: None,
            name: None,
            name_cmd: None,
            name_one_line: None,
            synopsis: Vec::new(),
            description: Vec::new(),
            options: Vec::new(),
            positionals: Vec::new(),
            subcommands: Vec::new(),
            environment: Vec::new(),
            files: Vec::new(),
            examples: Vec::new(),
            exit_status: Vec::new(),
            see_also: Vec::new(),
            authors: Vec::new(),
            extra_sections: Vec::new(),
            width: 80,
        }
    }

    /* ---------- builder entête ---------- */

    pub fn version(mut self, v: &str) -> Self {
        self.version = Some(v.to_owned());
        self
    }
    pub fn manual(mut self, m: &str) -> Self {
        self.manual = Some(m.to_owned());
        self
    }
    pub fn source(mut self, s: &str) -> Self {
        self.source = Some(s.to_owned());
        self
    }
    pub fn date(mut self, d: &str) -> Self {
        self.date = Some(d.to_owned());
        self
    }
    pub fn with_width(mut self, w: usize) -> Self {
        self.width = w.max(60);
        self
    }

    /// Section NAME. Si non fourni, sera composé à partir de `name_cmd` et `description`.
    pub fn name_line(mut self, one_line: &str) -> Self {
        self.name_one_line = Some(one_line.to_owned());
        self
    }
    pub fn name_cmd(mut self, cmd: &str) -> Self {
        self.name_cmd = Some(cmd.to_owned());
        self
    }

    /* ---------- builder contenu ---------- */

    pub fn synopsis(mut self, syn: &str) -> Self {
        self.synopsis.push(syn.to_owned());
        self
    }
    pub fn description(mut self, txt: &str) -> Self {
        self.description.push(txt.to_owned());
        self
    }
    pub fn option(mut self, it: OptionItem) -> Self {
        self.options.push(it);
        self
    }
    pub fn positional(mut self, it: ArgItem) -> Self {
        self.positionals.push(it);
        self
    }
    pub fn sub(mut self, it: SubItem) -> Self {
        self.subcommands.push(it);
        self
    }
    pub fn env_line(mut self, s: &str) -> Self {
        self.environment.push(s.to_owned());
        self
    }
    pub fn file_line(mut self, s: &str) -> Self {
        self.files.push(s.to_owned());
        self
    }
    pub fn example(mut self, s: &str) -> Self {
        self.examples.push(s.to_owned());
        self
    }
    pub fn exit_line(mut self, s: &str) -> Self {
        self.exit_status.push(s.to_owned());
        self
    }
    pub fn see_also(mut self, s: &str) -> Self {
        self.see_also.push(s.to_owned());
        self
    }
    pub fn author(mut self, s: &str) -> Self {
        self.authors.push(s.to_owned());
        self
    }
    pub fn section(mut self, s: Section) -> Self {
        self.extra_sections.push(s);
        self
    }

    /// Rendu troff (macro *man*). Retourne `String`.
    pub fn render(&self) -> String {
        self.render_result().unwrap_or_else(|_| String::new())
    }

    /// Rendu avec gestion d’erreurs basiques.
    pub fn render_result(&self) -> Result<String> {
        if !(1..=8).contains(&self.section) {
            return Err(ManError::InvalidSection);
        }
        let mut out = String::new();

        // .TH title section date source manual
        let date = self.date.clone().unwrap_or_else(today_yyyy_mm_dd);
        let source = self.source.clone().unwrap_or_default();
        let manual = self.manual.clone().unwrap_or_default();
        writeln!(
            &mut out,
            ".TH \"{}\" \"{}\" \"{}\" \"{}\" \"{}\"",
            troff_escape(&self.title),
            self.section,
            troff_escape(&date),
            troff_escape(&source),
            troff_escape(&manual)
        )
        .ok();

        // NAME
        writeln!(&mut out, ".SH NAME").ok();
        let name_line = if let Some(n) = &self.name_one_line {
            n.clone()
        } else {
            // Compose "cmd - desc courte"
            let cmd_owned: String;
            let cmd: &str = if let Some(c) = self.name_cmd.as_deref() {
                c
            } else {
                cmd_owned = self.title.to_lowercase();
                &cmd_owned
            };
            let desc = self.description.get(0).map(|s| s.as_str()).unwrap_or("");
            let short = first_sentence(desc, 60);
            format!("{} - {}", cmd, short)
        };
        writeln!(&mut out, "{}", troff_escape(&name_line)).ok();

        // SYNOPSIS
        if !self.synopsis.is_empty() {
            writeln!(&mut out, ".SH SYNOPSIS").ok();
            for syn in &self.synopsis {
                // Use .nf/.fi to avoid breaking option braces
                writeln!(&mut out, ".nf").ok();
                writeln!(&mut out, "{}", troff_escape(syn)).ok();
                writeln!(&mut out, ".fi").ok();
            }
        }

        // DESCRIPTION
        if !self.description.is_empty() {
            writeln!(&mut out, ".SH DESCRIPTION").ok();
            for p in &self.description {
                write_wrapped(&mut out, p, self.width, 0);
            }
        }

        // ARGUMENTS
        if !self.positionals.is_empty() {
            writeln!(&mut out, ".SH ARGUMENTS").ok();
            for a in &self.positionals {
                writeln!(&mut out, ".TP").ok();
                writeln!(&mut out, "{}", troff_escape(&a.name)).ok();
                write_wrapped(&mut out, &a.help, self.width, 8);
            }
        }

        // OPTIONS
        if !self.options.is_empty() {
            writeln!(&mut out, ".SH OPTIONS").ok();
            for o in &self.options {
                writeln!(&mut out, ".TP").ok();
                writeln!(&mut out, "{}", troff_escape(&o.forms)).ok();
                write_wrapped(&mut out, &o.help, self.width, 8);
            }
        }

        // SUBCOMMANDS
        if !self.subcommands.is_empty() {
            writeln!(&mut out, ".SH SUBCOMMANDS").ok();
            for s in &self.subcommands {
                writeln!(&mut out, ".TP").ok();
                writeln!(&mut out, "{}", troff_escape(&s.name)).ok();
                write_wrapped(&mut out, &s.about, self.width, 8);
            }
        }

        // ENVIRONMENT
        if !self.environment.is_empty() {
            writeln!(&mut out, ".SH ENVIRONMENT").ok();
            for e in &self.environment {
                bullet(&mut out, e, self.width);
            }
        }

        // FILES
        if !self.files.is_empty() {
            writeln!(&mut out, ".SH FILES").ok();
            for f in &self.files {
                bullet(&mut out, f, self.width);
            }
        }

        // EXAMPLES
        if !self.examples.is_empty() {
            writeln!(&mut out, ".SH EXAMPLES").ok();
            for ex in &self.examples {
                writeln!(&mut out, ".nf").ok();
                writeln!(&mut out, "{}", troff_escape(ex)).ok();
                writeln!(&mut out, ".fi").ok();
            }
        }

        // EXIT STATUS
        if !self.exit_status.is_empty() {
            writeln!(&mut out, ".SH EXIT STATUS").ok();
            for l in &self.exit_status {
                bullet(&mut out, l, self.width);
            }
        }

        // SEE ALSO
        if !self.see_also.is_empty() {
            writeln!(&mut out, ".SH SEE ALSO").ok();
            write_wrapped(&mut out, &self.see_also.join(", "), self.width, 0);
        }

        // AUTHORS
        if !self.authors.is_empty() {
            writeln!(&mut out, ".SH AUTHORS").ok();
            write_wrapped(&mut out, &self.authors.join(", "), self.width, 0);
        }

        // Extras
        for sec in &self.extra_sections {
            writeln!(&mut out, ".SH {}", troff_escape(&sec.title)).ok();
            write_wrapped(&mut out, &sec.body, self.width, 0);
        }

        Ok(out)
    }

    /* ---------- Intégration avec vitte-args (facultative) ---------- */

    /// Construit une page man minimale à partir d’un `vitte-args::Spec`.
    #[cfg(feature = "args-spec")]
    pub fn from_args_spec(spec: &ArgSpec, section: u8) -> Self {
        let mut page = ManPage::new(&spec.bin, section).name_cmd(&spec.bin).synopsis(&spec.usage());

        if let Some(a) = &spec.about {
            page = page.description(a);
        }

        // Options/Args
        for a in &spec.args {
            match a.kind {
                ArgKindArgs::Flag => {
                    let mut forms = String::new();
                    if let Some(c) = a.short {
                        push_form(&mut forms, &format!("-{}", c));
                    }
                    if let Some(l) = &a.long {
                        push_form(&mut forms, &format!("--{}", l));
                    }
                    if forms.is_empty() {
                        push_form(&mut forms, &format!("--{}", a.name));
                    }
                    let help = a.help.clone().unwrap_or_default();
                    page = page.option(OptionItem::new(forms, help));
                },
                ArgKindArgs::Opt => {
                    let vn = a.value_name.as_deref().unwrap_or("VAL");
                    let mut forms = String::new();
                    if let Some(c) = a.short {
                        push_form(&mut forms, &format!("-{} <{}>", c, vn));
                    }
                    if let Some(l) = &a.long {
                        push_form(&mut forms, &format!("--{} <{}>", l, vn));
                    }
                    if forms.is_empty() {
                        push_form(&mut forms, &format!("--{} <{}>", a.name, vn));
                    }
                    let mut help = a.help.clone().unwrap_or_default();
                    if let Some(def) = &a.default {
                        if !help.is_empty() {
                            help.push(' ');
                        }
                        help.push_str(&format!("[default: {}]", def));
                    }
                    page = page.option(OptionItem::new(forms, help));
                },
                ArgKindArgs::Pos => {
                    let nm = a.value_name.clone().unwrap_or_else(|| a.name.clone());
                    let help = a.help.clone().unwrap_or_default();
                    page = page.positional(ArgItem::new(nm, help));
                },
            }
        }

        // Sous-commandes
        for sc in &spec.subcommands {
            page = page.sub(SubItem::new(&sc.bin, sc.about.clone().unwrap_or_default()));
        }

        page
    }
}

/* ================================ ERREURS ================================ */

#[cfg(feature = "errors")]
#[derive(Debug, Error, PartialEq, Eq)]
pub enum ManError {
    #[error("invalid section")]
    InvalidSection,
}
#[cfg(not(feature = "errors"))]
#[derive(Debug, PartialEq, Eq)]
pub enum ManError {
    InvalidSection,
}

pub type Result<T> = core::result::Result<T, ManError>;

/* ================================ HELPERS ================================ */

fn push_form(dst: &mut String, s: &str) {
    if !dst.is_empty() {
        dst.push_str(", ");
    }
    dst.push_str(s);
}

fn today_yyyy_mm_dd() -> String {
    #[cfg(feature = "std")]
    {
        use std::time::{Duration, UNIX_EPOCH};
        // Très simple: on n’utilise pas chrono pour rester sans dépendances.
        // On renvoie une date ISO fictive si conversion échoue.
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or(Duration::from_secs(0))
            .as_secs();
        // Approximation: 86400s par jour depuis 1970.
        const DAY: u64 = 86_400;
        let days = now / DAY;
        let year = 1970 + (days / 365) as i64;
        let rem = days % 365;
        let month = 1 + (rem / 30) as u32;
        let day = 1 + (rem % 30) as u32;
        return format!("{:04}-{:02}-{:02}", year, month.min(12), day.min(28));
    }
    #[cfg(not(feature = "std"))]
    {
        "1970-01-01".to_string()
    }
}

/// Echappe minimalement pour troff (\, ., - au début de ligne, \* etc.)
fn troff_escape(s: &str) -> String {
    let mut out = String::with_capacity(s.len() + 8);
    let mut first = true;
    for line in s.split('\n') {
        if first {
            first = false;
        } else {
            out.push('\n');
        }
        if line.starts_with('.') || line.starts_with('\'') {
            out.push('\\');
        }
        for ch in line.chars() {
            match ch {
                '\\' => {
                    out.push('\\');
                    out.push('\\');
                },
                // Hyphen vs minus: laissons tel quel mais évitons -- en tête
                _ => out.push(ch),
            }
        }
    }
    out
}

/// Wrap paragraphe en lignes <= width. Indentation en espaces.
fn write_wrapped(out: &mut String, text: &str, width: usize, indent: usize) {
    let lines = wrap(text, width.saturating_sub(indent));
    for l in lines {
        if indent > 0 {
            let _ = write!(out, "{}", " ".repeat(indent));
        }
        let _ = writeln!(out, "{}", troff_escape(&l));
    }
}

/// Puces simples via `.IP \(bu 2`.
fn bullet(out: &mut String, text: &str, width: usize) {
    writeln!(out, ".IP \\(bu 2").ok();
    write_wrapped(out, text, width, 0);
}

/// Coupe sur espaces, césure naïve si mot > width.
fn wrap(s: &str, width: usize) -> Vec<String> {
    if s.is_empty() {
        return vec![];
    }
    let w = width.max(20);
    let mut out = Vec::new();
    for raw in s.split('\n') {
        let mut line = String::new();
        for word in raw.split_whitespace() {
            if line.is_empty() {
                if word.len() > w {
                    // césure brute
                    let mut rest = word;
                    while rest.len() > w {
                        out.push(rest[..w].to_string());
                        rest = &rest[w..];
                    }
                    line.push_str(rest);
                } else {
                    line.push_str(word);
                }
            } else {
                if line.len() + 1 + word.len() <= w {
                    line.push(' ');
                    line.push_str(word);
                } else {
                    out.push(core::mem::take(&mut line));
                    line.push_str(word);
                }
            }
        }
        if !line.is_empty() {
            out.push(line);
        }
    }
    out
}

/// Première phrase ou tronqué à n chars, utilisé pour NAME si absent.
fn first_sentence(s: &str, n: usize) -> String {
    let cut = s.find('.').unwrap_or(s.len());
    let take = cut.min(n);
    let mut out = s[..take].trim().to_string();
    if out.is_empty() {
        out.push_str("command");
    }
    out
}

/* ================================= TESTS ================================= */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn render_minimal() {
        let m = ManPage::new("vitte", 1)
            .name_cmd("vitte")
            .synopsis("vitte [OPTIONS] <INPUT>")
            .description("Outil de démonstration.")
            .option(OptionItem::new("-v, --verbose", "Verbosité accrue."))
            .positional(ArgItem::new("INPUT", "Fichier d'entrée."))
            .example("vitte -v file.txt");
        let s = m.render();
        assert!(s.contains(".TH \"VITTE\" \"1\""));
        assert!(s.contains(".SH NAME"));
        assert!(s.contains(".SH SYNOPSIS"));
        assert!(s.contains(".SH OPTIONS"));
        assert!(s.contains("-v, --verbose"));
    }

    #[test]
    fn troff_escape_dots() {
        let txt = ".danger\nsafe";
        let esc = super::troff_escape(txt);
        assert!(esc.starts_with("\\."));
        assert!(esc.contains("safe"));
    }

    #[test]
    fn wrap_works() {
        let lines = super::wrap("a wordthatistoolongforline should wrap", 10);
        assert!(lines.len() >= 3);
    }

    #[test]
    fn bullet_ip() {
        let mut out = String::new();
        super::bullet(&mut out, "item", 80);
        assert!(out.contains(".IP \\(bu 2"));
    }
}
