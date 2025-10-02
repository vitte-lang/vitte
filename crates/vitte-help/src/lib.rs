//! vitte-help — rendu d’aide CLI **ultra complet**
//!
//! Objectif: générer un help lisible, compact, et stable en largeur.
//!
//! Points clés:
//! - `HelpSpec` builder (nom, version, about, usage, options, positionnels, sous-commandes, exemples).
//! - Rendu texte brut ou colorisé (feature `colors` via `vitte-ansi`).
//! - Largeur contrôlée (`with_width`), auto à partir de `COLUMNS` si `std`.
//! - Colonnes alignées, wrap intelligent, sections localisées (fr/en).
//! - Intégration optionnelle avec `vitte-args::Spec` (feature `args-spec`).
//! - Zéro `unsafe`.
//!
//! Exemple court:
//! ```no_run
//! use vitte_help::{HelpSpec, Lang};
//! let help = HelpSpec::new("vitte")
//!     .version("0.1.0")
//!     .about("Vitte CLI")
//!     .usage("vitte [OPTIONS] <INPUT>")
//!     .option(["-v","--verbose"], "Augmente la verbosité.")
//!     .pos("INPUT", "Fichier d'entrée.")
//!     .example("vitte -v input.txt")
//!     .lang(Lang::Fr)
//!     .with_width(100)
//!     .render(true);
//! println!("{}", help);
//! ```

#![forbid(unsafe_code)]

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.") ;

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{string::String, vec::Vec, borrow::ToOwned, format};
#[cfg(feature = "std")]
use std::{string::String, vec::Vec, env};

#[cfg(feature = "serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature = "errors")]
use thiserror::Error;

#[cfg(feature = "colors")]
use vitte_ansi as ansi;

#[cfg(feature = "args-spec")]
use vitte_args::{Spec as ArgSpec, ArgKind as ArgKindArgs};

/// Langue de l’affichage.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Lang { En, Fr }

impl Default for Lang { fn default() -> Self { Lang::Fr } }

#[cfg(feature = "errors")]
#[derive(Debug, Error, PartialEq, Eq)]
pub enum HelpError {
    #[error("invalid width (must be >= 40)")]
    WidthTooSmall,
}
#[cfg(not(feature = "errors"))]
#[derive(Debug, PartialEq, Eq)]
pub enum HelpError { WidthTooSmall }

#[cfg(feature = "errors")]
pub type Result<T> = core::result::Result<T, HelpError>;
#[cfg(not(feature = "errors"))]
pub type Result<T> = core::result::Result<T, &'static str>;

/// Une option/flag à afficher.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct OptLine {
    /// Alias, ex: ["-v","--verbose"] ou ["--level <LVL>"].
    pub forms: Vec<String>,
    /// Description courte.
    pub help: String,
    /// Masquer du help.
    pub hidden: bool,
}
impl OptLine {
    pub fn new<I, S>(forms: I, help: S) -> Self
    where I: IntoIterator<Item = S>, S: Into<String> + Clone {
        Self { forms: forms.into_iter().map(|s| s.into()).collect(), help: help.clone().into(), hidden: false }
    }
    pub fn hidden(mut self, h: bool) -> Self { self.hidden = h; self }
}

/// Un argument positionnel.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct PosLine {
    pub name: String,
    pub help: String,
}
impl PosLine { pub fn new(name: &str, help: &str) -> Self { Self { name: name.to_owned(), help: help.to_owned() } } }

/// Une sous-commande visible dans l’aide.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct SubLine {
    pub name: String,
    pub about: String,
}
impl SubLine { pub fn new(name: &str, about: &str) -> Self { Self { name: name.to_owned(), about: about.to_owned() } } }

/// Spécification d’aide complète.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct HelpSpec {
    pub bin: String,
    pub version: Option<String>,
    pub about: Option<String>,
    pub usage: Option<String>,
    pub options: Vec<OptLine>,
    pub positionals: Vec<PosLine>,
    pub subs: Vec<SubLine>,
    pub examples: Vec<String>,
    pub notes: Vec<String>,
    pub lang: Lang,
    pub width: usize,   // largeur totale; 0 → auto
    pub gutter: usize,  // espaces entre colonnes
    pub colors: bool,   // activer les couleurs si supportées
}

impl HelpSpec {
    pub fn new(bin: &str) -> Self {
        Self {
            bin: bin.to_owned(),
            version: None,
            about: None,
            usage: None,
            options: Vec::new(),
            positionals: Vec::new(),
            subs: Vec::new(),
            examples: Vec::new(),
            notes: Vec::new(),
            lang: Lang::default(),
            width: 0,
            gutter: 2,
            colors: true,
        }
    }

    /* ---------- builder ---------- */
    pub fn version(mut self, v: &str) -> Self { self.version = Some(v.to_owned()); self }
    pub fn about(mut self, a: &str) -> Self { self.about = Some(a.to_owned()); self }
    pub fn usage(mut self, u: &str) -> Self { self.usage = Some(u.to_owned()); self }
    pub fn option<I, S>(mut self, forms: I, help: S) -> Self
    where I: IntoIterator<Item = S>, S: Into<String> + Clone {
        self.options.push(OptLine::new(forms, help)); self
    }
    pub fn pos(mut self, name: &str, help: &str) -> Self { self.positionals.push(PosLine::new(name, help)); self }
    pub fn sub(mut self, name: &str, about: &str) -> Self { self.subs.push(SubLine::new(name, about)); self }
    pub fn example(mut self, ex: &str) -> Self { self.examples.push(ex.to_owned()); self }
    pub fn note(mut self, n: &str) -> Self { self.notes.push(n.to_owned()); self }
    pub fn lang(mut self, l: Lang) -> Self { self.lang = l; self }
    pub fn with_width(mut self, w: usize) -> Self { self.width = w; self }
    pub fn gutter(mut self, g: usize) -> Self { self.gutter = g.max(1); self }
    pub fn colors(mut self, on: bool) -> Self { self.colors = on; self }

    /// Rend en String. `colorize=true` pour activer ANSI si `colors` est vrai.
    pub fn render(&self, colorize: bool) -> String {
        self.render_result(colorize).unwrap_or_else(|_| String::new())
    }

    /// Rend en String avec gestion d’erreurs.
    pub fn render_result(&self, colorize: bool) -> Result<String> {
        let width = effective_width(self.width)?;
        let mut out = String::new();

        // En-tête
        header(&mut out, self, colorize);

        // Usage
        if let Some(u) = &self.usage {
            section_title(&mut out, self.lang, "USAGE", "Utilisation", colorize);
            wrap_lines(&mut out, u, width, 0);
            out.push('\n');
        }

        // Description
        if let Some(a) = &self.about {
            section_title(&mut out, self.lang, "ABOUT", "À propos", colorize);
            wrap_lines(&mut out, a, width, 0);
            out.push('\n');
        }

        // Positionnels
        if !self.positionals.is_empty() {
            section_title(&mut out, self.lang, "ARGS", "Arguments", colorize);
            render_table(&mut out,
                         self.positionals.iter().map(|p| (p.name.as_str(), p.help.as_str())),
                         width, self.gutter, colorize);
            out.push('\n');
        }

        // Options
        let opts_vis: Vec<&OptLine> = self.options.iter().filter(|o| !o.hidden).collect();
        if !opts_vis.is_empty() {
            section_title(&mut out, self.lang, "OPTIONS", "Options", colorize);
            // Prépare la forme jointe type "-v, --verbose"
            let rows = opts_vis.iter().map(|o| (o.forms.join(", "), o.help.clone())).collect::<Vec<_>>();
            render_table_owned(&mut out, rows, width, self.gutter, colorize);
            out.push('\n');
        }

        // Sous-commandes
        if !self.subs.is_empty() {
            section_title(&mut out, self.lang, "SUBCOMMANDS", "Sous-commandes", colorize);
            render_table(&mut out,
                         self.subs.iter().map(|s| (s.name.as_str(), s.about.as_str())),
                         width, self.gutter, colorize);
            out.push('\n');
        }

        // Exemples
        if !self.examples.is_empty() {
            section_title(&mut out, self.lang, "EXAMPLES", "Exemples", colorize);
            for e in &self.examples {
                bullet(&mut out, e, width, 2);
            }
            out.push('\n');
        }

        // Notes
        if !self.notes.is_empty() {
            section_title(&mut out, self.lang, "NOTES", "Notes", colorize);
            for n in &self.notes {
                wrap_lines(&mut out, n, width, 0);
            }
            out.push('\n');
        }

        Ok(out)
    }

    /* ---------- Intégration vitte-args (facultative) ---------- */

    /// Construit un `HelpSpec` depuis un `vitte-args::Spec` simplifié.
    #[cfg(feature = "args-spec")]
    pub fn from_args_spec(spec: &ArgSpec) -> Self {
        let mut h = HelpSpec::new(&spec.bin);
        h.about = spec.about.clone();
        h.version = None; // `ArgSpec` peut l’avoir, mais on laisse l’appelant renseigner.
        h.usage = Some(spec.usage());
        // Options + positionnels
        for a in &spec.args {
            match a.kind {
                ArgKindArgs::Flag => {
                    let mut forms = Vec::new();
                    if let Some(c) = a.short { forms.push(format!("-{}", c)); }
                    if let Some(l) = &a.long { forms.push(format!("--{}", l)); }
                    if forms.is_empty() { forms.push(format!("--{}", a.name)); }
                    h.options.push(OptLine {
                        forms,
                        help: a.help.clone().unwrap_or_default(),
                        hidden: false,
                    });
                }
                ArgKindArgs::Opt => {
                    let mut forms = Vec::new();
                    if let Some(c) = a.short {
                        forms.push(format!("-{} <{}>", c, a.value_name.as_deref().unwrap_or("VAL")));
                    } else if let Some(c) = a.short {
                        forms.push(format!("-{}", c));
                    }
                    if let Some(l) = &a.long {
                        forms.push(format!("--{} <{}>", l, a.value_name.as_deref().unwrap_or("VAL")));
                    } else if let Some(l) = &a.long {
                        forms.push(format!("--{}", l));
                    }
                    if forms.is_empty() {
                        forms.push(format!("--{} <{}>", a.name, a.value_name.as_deref().unwrap_or("VAL")));
                    }
                    let mut help = a.help.clone().unwrap_or_default();
                    if let Some(def) = &a.default {
                        if !help.is_empty() { help.push(' '); }
                        help.push_str(&format!("[default: {}]", def));
                    }
                    h.options.push(OptLine { forms, help, hidden: false });
                }
                ArgKindArgs::Pos => {
                    h.positionals.push(PosLine {
                        name: a.value_name.clone().unwrap_or_else(|| a.name.clone()),
                        help: a.help.clone().unwrap_or_default(),
                    });
                }
            }
        }
        // Subcommands
        for sc in &spec.subcommands {
            h.subs.push(SubLine::new(&sc.bin, sc.about.clone().unwrap_or_default().as_str()));
        }
        h
    }
}

/* ============================== RENDERING ============================== */

fn header(out: &mut String, spec: &HelpSpec, colorize: bool) {
    let mut title = spec.bin.clone();
    if let Some(v) = &spec.version {
        title.push(' ');
        title.push_str(v);
    }
    if colorize && spec.colors {
        #[cfg(feature = "colors")]
        {
            out.push_str(&ansi::style(&title).bold().to_string());
            out.push('\n');
        }
        #[cfg(not(feature = "colors"))]
        {
            out.push_str(&title); out.push('\n');
        }
    } else {
        out.push_str(&title); out.push('\n');
    }
}

fn section_title(out: &mut String, lang: Lang, en: &str, fr: &str, colorize: bool) {
    let label = match lang { Lang::En => en, Lang::Fr => fr };
    if colorize {
        #[cfg(feature = "colors")]
        {
            out.push_str(&ansi::style(label).bold().underline().to_string());
            out.push('\n');
        }
        #[cfg(not(feature = "colors"))]
        {
            out.push_str(label); out.push('\n');
        }
    } else {
        out.push_str(label); out.push('\n');
    }
}

/// Table deux colonnes, avec wrap à droite.
/// rows: (col1, col2)
fn render_table<'a, I>(out: &mut String, rows: I, width: usize, gutter: usize, colorize: bool)
where I: IntoIterator<Item = (&'a str, &'a str)> {
    // calc col1 width = min(max form width, width/2), mais pas plus de 40% si étroit.
    let mut left_max = 0usize;
    let mut rows_vec: Vec<(String, String)> = Vec::new();
    for (l, r) in rows {
        left_max = left_max.max(display_width(l));
        rows_vec.push((l.to_owned(), r.to_owned()));
    }
    let cap_left = (width as f32 * 0.45) as usize;
    let left_width = left_max.min(cap_left);
    let right_width = width.saturating_sub(left_width + gutter);

    for (l, r) in rows_vec {
        // col1 (éventuellement colorisée)
        let left = if colorize {
            #[cfg(feature = "colors")]
            { ansi::style(&l).bold().to_string() }
            #[cfg(not(feature = "colors"))]
            { l.clone() }
        } else { l.clone() };

        // imprime line1
        let left_pad = pad_exact(&left, left_width);
        let mut wrapped = wrap(r.as_str(), right_width);
        if wrapped.is_empty() { wrapped.push(String::new()); }
        out.push_str(&left_pad);
        out.push_str(&" ".repeat(gutter));
        out.push_str(&wrapped[0]);
        out.push('\n');

        // suites
        for cont in wrapped.into_iter().skip(1) {
            out.push_str(&" ".repeat(left_width));
            out.push_str(&" ".repeat(gutter));
            out.push_str(&cont);
            out.push('\n');
        }
    }
}

fn render_table_owned(out: &mut String, rows: Vec<(String, String)>, width: usize, gutter: usize, colorize: bool) {
    let it = rows.iter().map(|(l,r)| (l.as_str(), r.as_str()));
    render_table(out, it, width, gutter, colorize);
}

fn bullet(out: &mut String, s: &str, width: usize, indent: usize) {
    let prefix = format!("{}- ", " ".repeat(indent));
    let rest_width = width.saturating_sub(prefix.len());
    let lines = wrap(s, rest_width);
    if lines.is_empty() { out.push_str(&prefix); out.push('\n'); return; }
    out.push_str(&prefix); out.push_str(&lines[0]); out.push('\n');
    for l in lines.into_iter().skip(1) {
        out.push_str(&" ".repeat(prefix.len()));
        out.push_str(&l);
        out.push('\n');
    }
}

fn wrap_lines(out: &mut String, s: &str, width: usize, indent: usize) {
    for line in s.split('\n') {
        for seg in wrap(line, width.saturating_sub(indent)) {
            out.push_str(&" ".repeat(indent));
            out.push_str(&seg);
            out.push('\n');
        }
    }
}

/* =============================== LAYOUT =============================== */

fn effective_width(request: usize) -> Result<usize> {
    let w = if request == 0 {
        env_columns().unwrap_or(80)
    } else {
        request
    };
    if w < 40 {
        #[cfg(feature = "errors")] { return Err(HelpError::WidthTooSmall); }
        #[cfg(not(feature = "errors"))] { return Err("width too small"); }
    }
    Ok(w)
}

#[cfg(feature = "std")]
fn env_columns() -> Option<usize> {
    if let Ok(c) = env::var("COLUMNS") {
        if let Ok(n) = c.parse::<usize>() { return Some(n.max(40)); }
    }
    None
}
#[cfg(not(feature = "std"))]
fn env_columns() -> Option<usize> { None }

fn display_width(s: &str) -> usize { s.chars().count() } // suffisant pour ASCII/CLI

fn pad_exact(s: &str, width: usize) -> String {
    let w = display_width(s);
    if w >= width { return s.to_owned(); }
    let mut out = String::with_capacity(width);
    out.push_str(s);
    out.push_str(&" ".repeat(width - w));
    out
}

fn wrap(s: &str, width: usize) -> Vec<String> {
    if s.is_empty() { return vec![]; }
    if width < 8 { return vec![s.to_owned()]; }
    let mut out = Vec::new();
    let mut line = String::new();
    for word in s.split_whitespace() {
        if line.is_empty() {
            if display_width(word) > width {
                // césure brute
                let mut rest = word;
                while display_width(rest) > width {
                    let (take, r) = split_at_display(rest, width);
                    out.push(take.to_owned());
                    rest = r;
                }
                line.push_str(rest);
            } else {
                line.push_str(word);
            }
        } else {
            if display_width(&line) + 1 + display_width(word) <= width {
                line.push(' ');
                line.push_str(word);
            } else {
                out.push(core::mem::take(&mut line));
                line.push_str(word);
            }
        }
    }
    if !line.is_empty() { out.push(line); }
    out
}

fn split_at_display(s: &str, width: usize) -> (&str, &str) {
    let mut count = 0usize;
    for (idx, ch) in s.char_indices() {
        if count == width { return (&s[..idx], &s[idx..]); }
        count += 1;
    }
    (s, "")
}

/* ================================= TESTS ================================= */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn basic_help_renders() {
        let h = HelpSpec::new("demo")
            .version("1.0")
            .about("demo tool")
            .usage("demo [OPTIONS] <FILE>")
            .option(["-v","--verbose"], "Verbose mode")
            .pos("FILE", "Input file")
            .sub("init", "Initialize")
            .example("demo -v file.txt")
            .note("Use `--help` for details.");
        let s = h.render(false);
        assert!(s.contains("demo 1.0"));
        assert!(s.contains("USAGE") || s.contains("Utilisation"));
        assert!(s.contains("--verbose"));
        assert!(s.contains("init"));
    }

    #[test]
    fn table_wraps() {
        let h = HelpSpec::new("t")
            .with_width(50)
            .option(["--long-option"], "A very very long long sentence that should wrap on the right side properly without breaking the left column.")
            .pos("INPUT", "File to process that can be quite descriptive.");
        let s = h.render(false);
        assert!(s.lines().count() > 3);
    }

    #[test]
    fn bullets_wrap() {
        let h = HelpSpec::new("b")
            .with_width(40)
            .example("This example line is intentionally very long so it should wrap below the bullet.");
        let s = h.render(false);
        assert!(s.contains("- This example line"));
    }
}