//! vitte-tree — arbre pour CLIs **ultra ultra complet**
//!
//! Capacités:
//! - Modèle `Node` (étiquette + enfants + meta optionnels).
//! - Rendu ASCII/UTF avec branches, derniers nœuds, indentation, icônes.
//! - Options: afficher la racine, profondeur max, largeur clamp, tri personnalisable.
//! - Nœuds repliés (`collapsed`) ou exclus par prédicat.
//! - Coloration optionnelle (vitte-style) par rôle logique.
//! - Export JSON/YAML si features activées.
//! - Construction fluide: `Node::new("root").push(Node::leaf("x"))`.
//! - Zéro `unsafe`.

#![forbid(unsafe_code)]
#![cfg_attr(docsrs, feature(doc_cfg))]

/* ============================== imports ============================== */

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable `std` (default) or `alloc-only`.");

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{boxed::Box, format, string::String, vec, vec::Vec};

#[cfg(feature = "std")]
use std::{fmt, string::String, vec::Vec};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "errors")]
use thiserror::Error;

#[cfg(feature = "style")]
use vitte_style as vstyle;

/* ============================== erreurs ============================== */

#[cfg(feature = "errors")]
#[derive(Debug, Error)]
pub enum TreeError {
    #[error("serialize: {0}")]
    Ser(String),
}
#[cfg(not(feature = "errors"))]
#[derive(Debug)]
pub enum TreeError {
    Ser(String),
}

pub type Result<T> = core::result::Result<T, TreeError>;

/* ============================== modèle ============================== */

/// Rôle logique facultatif pour coloration.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum RoleLike {
    Primary,
    Success,
    Warn,
    Error,
    Info,
    Muted,
}

/// Métadonnées optionnelles.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, Default)]
pub struct Meta {
    pub role: Option<RoleLike>,
    pub icon: Option<String>, // ex: "📁"
    pub collapsed: bool,      // si true, n’affiche pas les enfants
    pub data: Option<String>, // libre (chemin, id, etc.)
}

#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, Default)]
pub struct Node {
    pub label: String,
    pub children: Vec<Node>,
    pub meta: Meta,
}

impl Node {
    pub fn new<S: Into<String>>(label: S) -> Self {
        Self { label: label.into(), children: Vec::new(), meta: Meta::default() }
    }
    pub fn with_meta(mut self, meta: Meta) -> Self {
        self.meta = meta;
        self
    }
    pub fn with_role(mut self, r: RoleLike) -> Self {
        self.meta.role = Some(r);
        self
    }
    pub fn with_icon(mut self, s: impl Into<String>) -> Self {
        self.meta.icon = Some(s.into());
        self
    }
    pub fn collapsed(mut self, on: bool) -> Self {
        self.meta.collapsed = on;
        self
    }
    pub fn data(mut self, d: impl Into<String>) -> Self {
        self.meta.data = Some(d.into());
        self
    }
    pub fn push(mut self, child: Node) -> Self {
        self.children.push(child);
        self
    }
    pub fn add(&mut self, child: Node) {
        self.children.push(child);
    }
    pub fn leaf<S: Into<String>>(label: S) -> Self {
        Self::new(label)
    }

    /// Parcours profondeur d’abord. `f(node, depth)`.
    pub fn walk<F: FnMut(&Node, usize)>(&self, mut f: F) {
        walk_impl(self, 0, &mut f);
    }

    /// Tri récursif des enfants par comparateur.
    pub fn sort_by<F: FnMut(&Node, &Node) -> core::cmp::Ordering>(&mut self, mut cmp: F) {
        self.children.sort_by(|a, b| cmp(a, b));
        for c in &mut self.children {
            c.sort_by(&mut cmp);
        }
    }
}

fn walk_impl<F: FnMut(&Node, usize)>(n: &Node, d: usize, f: &mut F) {
    f(n, d);
    for c in &n.children {
        walk_impl(c, d + 1, f);
    }
}

/* ============================== rendu ============================== */

/// Jeu de caractères de branche.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum Charset {
    Ascii,
    Utf,
}

impl Default for Charset {
    fn default() -> Self {
        Charset::Utf
    }
}

impl Charset {
    fn branch(&self, last: bool) -> &'static str {
        match (self, last) {
            (Charset::Utf, false) => "├── ",
            (Charset::Utf, true) => "└── ",
            (Charset::Ascii, false) => "|-- ",
            (Charset::Ascii, true) => "`-- ",
        }
    }
    fn pipe(&self) -> &'static str {
        match self {
            Charset::Utf => "│   ",
            Charset::Ascii => "|   ",
        }
    }
    fn space(&self) -> &'static str {
        "    "
    }
}

/// Options de rendu.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct RenderOptions {
    pub charset: Charset,
    pub show_root: bool,
    pub max_depth: Option<usize>, // depth relative à la racine (0 = racine)
    pub clamp_width: Option<usize>, // tronque le label
    pub sort_children: bool,      // tri alpha simple si true
    pub filter_empty: bool,       // masque nœuds sans label après trim
    pub show_counts: bool,        // affiche " (n)" après label si enfants repliés
    #[cfg(feature = "style")]
    pub styler: Option<vstyle::Style>,
}

impl Default for RenderOptions {
    fn default() -> Self {
        Self {
            charset: Charset::Utf,
            show_root: true,
            max_depth: None,
            clamp_width: None,
            sort_children: false,
            filter_empty: false,
            show_counts: true,
            #[cfg(feature = "style")]
            styler: None,
        }
    }
}

impl RenderOptions {
    pub fn utf() -> Self {
        Self::default()
    }
    pub fn ascii() -> Self {
        Self { charset: Charset::Ascii, ..Self::default() }
    }
}

/// Rendu en chaîne multi-lignes.
pub fn render(root: &Node, opts: RenderOptions) -> String {
    let mut out = String::new();
    let mut path_last = Vec::<bool>::new(); // pile des "dernier enfant?"
    if opts.show_root {
        push_line(&mut out, "", &styled(&root.label, &root.meta, &opts));
    }
    let mut kids = root.children.clone();
    if opts.sort_children {
        kids.sort_by(|a, b| a.label.cmp(&b.label));
    }
    render_children(&kids, &mut out, &mut path_last, 0, &opts);
    out
}

fn render_children(
    children: &[Node],
    out: &mut String,
    path_last: &mut Vec<bool>,
    depth: usize,
    opts: &RenderOptions,
) {
    for (i, child) in children.iter().enumerate() {
        let last = i + 1 == children.len();
        let prefix = line_prefix(opts, path_last, last);
        let mut label = styled(&child.label, &child.meta, opts);

        if let Some(maxw) = opts.clamp_width {
            if display_width(&label) > maxw {
                label = truncate(&label, maxw);
            }
        }
        let mut post = String::new();
        let collapsed =
            child.meta.collapsed || opts.max_depth.map(|m| depth + 1 >= m).unwrap_or(false);
        if collapsed && opts.show_counts && !child.children.is_empty() {
            post.push_str(&format!(" ({})", child.children.len()));
        }
        // icône éventuelle
        if let Some(ic) = &child.meta.icon {
            label = format!("{ic} {label}");
        }
        push_line(out, &prefix, &format!("{label}{post}"));

        if !collapsed {
            let mut grand = child.children.clone();
            if opts.sort_children {
                grand.sort_by(|a, b| a.label.cmp(&b.label));
            }
            path_last.push(last);
            render_children(&grand, out, path_last, depth + 1, opts);
            path_last.pop();
        }
    }
}

fn line_prefix(opts: &RenderOptions, path_last: &[bool], is_last: bool) -> String {
    let mut p = String::new();
    for &last in path_last {
        p.push_str(if last { opts.charset.space() } else { opts.charset.pipe() });
    }
    p.push_str(opts.charset.branch(is_last));
    p
}

fn push_line(out: &mut String, prefix: &str, content: &str) {
    out.push_str(prefix);
    out.push_str(content);
    out.push('\n');
}

/* ============================== style helpers ============================== */

#[cfg(feature = "style")]
fn styled(text: &str, meta: &Meta, opts: &RenderOptions) -> String {
    match (&opts.styler, &meta.role) {
        (Some(st), Some(r)) => paint_role(st, r, text),
        (Some(st), None) => st.primary(text), // défaut léger
        _ => text.to_string(),
    }
}
#[cfg(not(feature = "style"))]
fn styled(text: &str, _meta: &Meta, _opts: &RenderOptions) -> String {
    text.to_string()
}

#[cfg(feature = "style")]
fn paint_role(st: &vstyle::Style, role: &RoleLike, s: &str) -> String {
    use vstyle::Role as R;
    match role {
        RoleLike::Primary => st.primary(s),
        RoleLike::Success => st.ok(s),
        RoleLike::Warn => st.warn(s),
        RoleLike::Error => st.err(s),
        RoleLike::Info => st.info(s),
        RoleLike::Muted => st.muted(s),
    }
}

/* ============================== width / truncate ============================== */

fn display_width(s: &str) -> usize {
    #[cfg(feature = "ansi")]
    {
        // naïf: on retire séquences CSI si déjà stylé
        let re = regex_lite::Regex::new("\x1B\\[[0-9;]*[A-Za-z]").unwrap();
        let plain = re.replace_all(s, "");
        return plain.chars().count();
    }
    #[cfg(not(feature = "ansi"))]
    {
        s.chars().count()
    }
}

fn truncate(s: &str, w: usize) -> String {
    if display_width(s) <= w {
        return s.to_string();
    }
    let mut out = String::new();
    let mut count = 0usize;
    for ch in s.chars() {
        let cw = 1; // simplifié
        if count + cw > w.saturating_sub(1) {
            break;
        }
        out.push(ch);
        count += cw;
    }
    out.push('…');
    out
}

/* ============================== export ============================== */

/// JSON (feature `json`).
#[cfg(feature = "json")]
pub fn to_json(root: &Node, pretty: bool) -> Result<String> {
    if pretty {
        serde_json::to_string_pretty(root).map_err(|e| TreeError::Ser(e.to_string()))
    } else {
        serde_json::to_string(root).map_err(|e| TreeError::Ser(e.to_string()))
    }
}

/// YAML (feature `yaml`).
#[cfg(feature = "yaml")]
pub fn to_yaml(root: &Node) -> Result<String> {
    serde_yaml::to_string(root).map_err(|e| TreeError::Ser(e.to_string()))
}

/* ============================== Display impl ============================== */

impl fmt::Display for Node {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let s = render(self, RenderOptions::default());
        f.write_str(&s)
    }
}

/* ============================== tests ============================== */

#[cfg(test)]
mod tests {
    use super::*;

    fn sample() -> Node {
        Node::new("root")
            .push(
                Node::new("src")
                    .with_icon("📁")
                    .push(Node::leaf("main.rs"))
                    .push(Node::leaf("lib.rs"))
                    .push(Node::new("bin").collapsed(true).push(Node::leaf("vitte"))),
            )
            .push(Node::new("Cargo.toml").with_role(RoleLike::Info))
    }

    #[test]
    fn render_utf() {
        let t = sample();
        let s = render(&t, RenderOptions::default());
        assert!(s.contains("└──"));
        assert!(s.lines().count() >= 4);
    }

    #[test]
    fn render_ascii_truncate() {
        let t = Node::new("r").push(Node::leaf("very-very-long-name"));
        let mut o = RenderOptions::ascii();
        o.clamp_width = Some(10);
        o.show_root = false;
        let s = render(&t, o);
        assert!(s.contains("…"));
        assert!(s.contains("`--"));
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_ok() {
        let t = sample();
        let j = to_json(&t, true).unwrap();
        assert!(j.contains("root"));
    }
}

/* ============================== deps optionnelles minimales ============================== */

#[cfg(feature = "ansi")]
mod regex_lite {
    // mini façade interne, sans dépendance externe.
    pub use crate::internal_regex::Regex;
}

#[cfg(feature = "ansi")]
mod internal_regex {
    use std::borrow::Cow;

    /// Fausse implémentation minimale suffisante pour `display_width`:
    /// on ignore le motif donné et on retire simplement les séquences ANSI de type CSI (`\x1B[` ... lettre).
    pub struct Regex;

    impl Regex {
        pub fn new(_pat: &str) -> Result<Self, ()> {
            Ok(Regex)
        }

        pub fn replace_all<'a>(&self, s: &'a str, _replacement: &str) -> Cow<'a, str> {
            // Scan sur octets pour conserver l’UTF-8 sans l’endommager.
            let bytes = s.as_bytes();
            let mut i = 0usize;
            let mut last = 0usize;
            let mut out: Option<String> = None;

            while i < bytes.len() {
                // Détecte ESC '['
                if bytes[i] == 0x1B && i + 1 < bytes.len() && bytes[i + 1] == b'[' {
                    // Alloue la sortie au premier besoin et pousse la portion claire.
                    if out.is_none() {
                        out = Some(String::with_capacity(s.len()));
                    }
                    if let Some(ref mut o) = out {
                        // segment clair avant la séquence
                        o.push_str(&s[last..i]);
                    }
                    // saute '\x1B['
                    i += 2;
                    // consomme jusqu’à un caractère ASCII alphabétique (fin des CSI)
                    while i < bytes.len() {
                        let b = bytes[i];
                        i += 1;
                        if (b as char).is_ascii_alphabetic() {
                            break;
                        }
                    }
                    last = i;
                } else {
                    // Avance d’un caractère UTF-8 complet
                    let ch_len = match bytes[i] {
                        0x00..=0x7F => 1,
                        0xC0..=0xDF => 2,
                        0xE0..=0xEF => 3,
                        _ => 4,
                    };
                    i += ch_len;
                }
            }

            if let Some(mut o) = out {
                // Reste final après la dernière séquence
                if last <= s.len() {
                    o.push_str(&s[last..]);
                }
                Cow::Owned(o)
            } else {
                // Pas de séquence trouvée: renvoie l’original
                Cow::Borrowed(s)
            }
        }
    }
}
