//! vitte-style — thèmes et mise en forme pour CLIs Vitte
//!
//! Objectifs:
//! - Thèmes clairs/sombres avec rôles: primary, success, warn, error, info, muted.
//! - API simple de stylisation: `Style::header`, `ok`, `warn`, `err`, `info`, `muted`.
//! - Bordures/boîtes (ASCII/UTF) et symboles standards (✓, ✗, !, i, →).
//! - Détection thème par env (`VITTE_THEME=dark|light`), override programmatique.
//! - Sans `vitte-ansi`, tout passe-through (aucune couleur).
//! - Zéro `unsafe`.

#![forbid(unsafe_code)]

#[cfg(all(not(feature="std"), not(feature="alloc-only")))]
compile_error!("Enable `std` (default) or `alloc-only`.") ;

#[cfg(feature="alloc-only")]
extern crate alloc;

#[cfg(feature="alloc-only")]
use alloc::{string::String, vec::Vec, boxed::Box, format};

#[cfg(feature="std")]
use std::{string::String, vec::Vec, fmt, env};

#[cfg(feature="serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature="errors")]
use thiserror::Error;

#[cfg(feature="ansi")]
use vitte_ansi as ansi;

/* =============================== Types =============================== */

/// Rôles de couleur utilisés par le thème.
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum Role {
    Primary,
    Success,
    Warn,
    Error,
    Info,
    Muted,
    Invert, // texte inversé pour emphase
}

/// Jeu de symboles standards.
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct Symbols {
    pub ok: &'static str,      // ✓
    pub err: &'static str,     // ✗
    pub warn: &'static str,    // !
    pub info: &'static str,    // i
    pub arrow: &'static str,   // →
    pub bullet: &'static str,  // •
}
impl Default for Symbols {
    fn default() -> Self {
        Self { ok:"✔", err:"✘", warn:"⚠", info:"ℹ", arrow:"→", bullet:"•" }
    }
}

/// Style de bordure (ASCII ou UTF).
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct Border {
    pub tl: &'static str, pub tr: &'static str, pub bl: &'static str, pub br: &'static str,
    pub h:  &'static str, pub v:  &'static str,
}
impl Border {
    pub const ASCII: Border = Border{ tl:"+", tr:"+", bl:"+", br:"+", h:"-", v:"|" };
    pub const UTF:   Border = Border{ tl:"┌", tr:"┐", bl:"└", br:"┘", h:"─", v:"│" };
}

/// Couleur logique (nominal + éventuellement variante claire/sombre).
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct Color {
    pub name: &'static str,
    pub fg: (u8,u8,u8),
    pub bg: Option<(u8,u8,u8)>,
}
impl Color {
    pub const fn rgb(name:&'static str, r:u8,g:u8,b:u8)->Self{ Self{name, fg:(r,g,b), bg:None} }
    pub const fn rgb_bg(name:&'static str, fr:u8,fg:u8,fb:u8, br:u8,bg:u8,bb:u8)->Self{ Self{name, fg:(fr,fg,fb), bg:Some((br,bg,bb))} }
}

/// Thème: mappage Role -> Color + options.
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct Theme {
    pub name: String,
    pub roles: [(Role, Color); 7],
    pub symbols: Symbols,
    pub border: Border,
    pub bold_headers: bool,
    pub underline_headers: bool,
    pub use_color: bool,
}
impl Theme {
    pub fn color(&self, r: Role) -> &Color {
        self.roles.iter().find(|(rr,_)| *rr==r).map(|(_,c)| c)
            .unwrap_or_else(|| self.roles.iter().find(|(rr,_)| *rr==Role::Primary).map(|(_,c)| c).unwrap())
    }
}

/* =============================== Thèmes prédéfinis =============================== */

pub mod preset {
    use super::*;
    pub fn dark() -> Theme {
        Theme {
            name: "dark".into(),
            roles: [
                (Role::Primary, Color::rgb("primary", 173,216,230)), // lightcyan
                (Role::Success, Color::rgb("success", 80,200,120)),
                (Role::Warn,    Color::rgb("warn",   255,200,87)),
                (Role::Error,   Color::rgb("error",  255,105,97)),
                (Role::Info,    Color::rgb("info",   135,206,250)),
                (Role::Muted,   Color::rgb("muted",  128,128,128)),
                (Role::Invert,  Color::rgb_bg("invert", 0,0,0, 255,255,255)),
            ],
            symbols: Symbols::default(),
            border: Border::UTF,
            bold_headers: true,
            underline_headers: false,
            use_color: true,
        }
    }
    pub fn light() -> Theme {
        Theme {
            name: "light".into(),
            roles: [
                (Role::Primary, Color::rgb("primary", 0,87,146)),
                (Role::Success, Color::rgb("success", 20,128,60)),
                (Role::Warn,    Color::rgb("warn",   170,110,0)),
                (Role::Error,   Color::rgb("error",  176,0,32)),
                (Role::Info,    Color::rgb("info",   0,102,204)),
                (Role::Muted,   Color::rgb("muted",  100,100,100)),
                (Role::Invert,  Color::rgb_bg("invert", 255,255,255, 0,0,0)),
            ],
            symbols: Symbols::default(),
            border: Border::ASCII,
            bold_headers: true,
            underline_headers: true,
            use_color: true,
        }
    }
}

/* =============================== Sélecteur global =============================== */

/// Source simple: `VITTE_THEME=dark|light` sinon `dark`.
pub fn detect_theme() -> Theme {
    match env::var("VITTE_THEME").ok().as_deref() {
        Some("light") => preset::light(),
        _ => preset::dark(),
    }
}

/* =============================== API de mise en forme =============================== */

/// Moteur de style à partir d’un `Theme`.
#[derive(Clone)]
pub struct Style {
    th: Theme,
}

impl Style {
    pub fn new(th: Theme) -> Self { Self { th } }
    pub fn theme(&self) -> &Theme { &self.th }
    pub fn with_symbols(mut self, s: Symbols) -> Self { self.th.symbols = s; self }
    pub fn with_border(mut self, b: Border) -> Self { self.th.border = b; self }
    pub fn use_color(mut self, on: bool) -> Self { self.th.use_color = on; self }

    /* ---- texte simple ---- */
    pub fn primary(&self, s: impl AsRef<str>) -> String { self.paint(Role::Primary, s.as_ref()) }
    pub fn ok(&self, s: impl AsRef<str>)      -> String { format!("{} {}", self.paint(Role::Success, self.th.symbols.ok), s.as_ref()) }
    pub fn warn(&self, s: impl AsRef<str>)    -> String { format!("{} {}", self.paint(Role::Warn, self.th.symbols.warn), s.as_ref()) }
    pub fn err(&self, s: impl AsRef<str>)     -> String { format!("{} {}", self.paint(Role::Error, self.th.symbols.err), s.as_ref()) }
    pub fn info(&self, s: impl AsRef<str>)    -> String { format!("{} {}", self.paint(Role::Info, self.th.symbols.info), s.as_ref()) }
    pub fn muted(&self, s: impl AsRef<str>)   -> String { self.paint(Role::Muted, s.as_ref()) }
    pub fn invert(&self, s: impl AsRef<str>)  -> String { self.paint(Role::Invert, s.as_ref()) }

    /// Titre: gras/underline selon thème.
    pub fn header(&self, s: impl AsRef<str>) -> String {
        let mut out = self.paint(Role::Primary, s.as_ref());
        #[cfg(feature="ansi")]
        if self.th.use_color {
            if self.th.bold_headers { out = ansi::style(out).bold().to_string(); }
            if self.th.underline_headers { out = ansi::style(out).underline().to_string(); }
        }
        out
    }

    /// Puce: "• text" colorée.
    pub fn bullet(&self, s: impl AsRef<str>) -> String {
        format!("{} {}", self.paint(Role::Primary, self.th.symbols.bullet), s.as_ref())
    }

    /// Lien fléché: "→ text".
    pub fn arrow(&self, s: impl AsRef<str>) -> String {
        format!("{} {}", self.paint(Role::Info, self.th.symbols.arrow), s.as_ref())
    }

    /* ---- boîtes ---- */

    /// Encadre un bloc de lignes avec la bordure du thème. Option: rôle de couleur.
    pub fn boxed(&self, lines: &[impl AsRef<str>], role: Option<Role>) -> String {
        let b = self.th.border.clone();
        let width = lines.iter().map(|l| unicode_width(l.as_ref())).max().unwrap_or(0);
        let top    = format!("{}{}{}", b.tl, repeat(b.h, width+2), b.tr);
        let bottom = format!("{}{}{}", b.bl, repeat(b.h, width+2), b.br);
        let mut out = String::new();
        out.push_str(&self.paint_opt(role, &top)); out.push('\n');
        for l in lines {
            let line = l.as_ref();
            let pad = width.saturating_sub(unicode_width(line));
            let row = format!("{} {}{} {}", b.v, line, repeat(" ", pad), b.v);
            out.push_str(&self.paint_opt(role, &row)); out.push('\n');
        }
        out.push_str(&self.paint_opt(role, &bottom));
        out
    }

    /* ---- interne: peinture ---- */

    fn paint(&self, role: Role, s: &str) -> String {
        if !self.th.use_color {
            return s.to_string();
        }
        #[cfg(feature="ansi")]
        {
            let c = self.th.color(role);
            let (r,g,b) = c.fg;
            let mut styled = ansi::rgb(r,g,b, s).to_string();
            if let Some((br,bg,bb)) = c.bg {
                styled = ansi::on_rgb(br,bg,bb, styled).to_string();
            }
            return styled;
        }
        #[cfg(not(feature="ansi"))]
        { s.to_string() }
    }
    fn paint_opt(&self, role: Option<Role>, s: &str) -> String {
        role.map(|r| self.paint(r, s)).unwrap_or_else(|| s.to_string())
    }
}

/* =============================== Helpers =============================== */

fn repeat<S: AsRef<str>>(s: S, n: usize) -> String {
    let mut out = String::with_capacity(s.as_ref().len() * n);
    for _ in 0..n { out.push_str(s.as_ref()); }
    out
}

// largeur naïve (ASCII). Pour vrai support largeurs Unicode, intégrer `unicode-width`.
fn unicode_width(s: &str) -> usize { s.chars().count() }

/* =============================== Erreurs optionnelles =============================== */

#[cfg(feature="errors")]
#[derive(Debug, Error)]
pub enum StyleError {
    #[error("invalid theme")]
    InvalidTheme,
}

/* =============================== Tests =============================== */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn roles_exist() {
        let t = preset::dark();
        assert_eq!(t.roles.len(), 7);
        let s = Style::new(t);
        let _ = s.primary("x");
        let _ = s.ok("done");
        let _ = s.warn("warn");
        let _ = s.err("err");
        let _ = s.info("info");
        let _ = s.muted("muted");
        let _ = s.invert("invert");
    }

    #[test]
    fn box_ascii_and_utf() {
        let s = Style::new(preset::light()).with_border(Border::ASCII);
        let bx = s.boxed(&["hello","world"], Some(Role::Info));
        assert!(bx.contains("+"));
        let s2 = Style::new(preset::dark()).with_border(Border::UTF);
        let bx2 = s2.boxed(&["hello"], Some(Role::Primary));
        assert!(bx2.contains("┌"));
    }
}