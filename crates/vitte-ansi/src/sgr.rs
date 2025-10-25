// vitte-ansi/src/sgr.rs
//! SGR/ANSI ultra complet : styles, couleurs 8/256/RGB, polices, doubles largeurs,
//! liens OSC 8, et helpers de composition. Sans dépendances externes.

use std::fmt;

/// Couleur SGR.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Color {
    Default,
    Basic(u8),      // 0..7 (noir..blanc) ou 0..15 (intenses) si désiré
    Ansi256(u8),    // 0..255 (xterm-256)
    Rgb(u8, u8, u8) // 24-bit truecolor
}

impl Color {
    fn to_sgr_fg(self) -> Vec<String> {
        match self {
            Color::Default => vec!["39".into()],
            Color::Basic(n) if n < 8  => vec![(30 + n) .to_string()],
            Color::Basic(n) if n < 16 => vec![(90 + (n - 8)).to_string()],
            Color::Basic(_) => vec!["39".into()],
            Color::Ansi256(n) => vec!["38".into(), "5".into(), n.to_string()],
            Color::Rgb(r,g,b) => vec!["38".into(), "2".into(), r.to_string(), g.to_string(), b.to_string()],
        }
    }
    fn to_sgr_bg(self) -> Vec<String> {
        match self {
            Color::Default => vec!["49".into()],
            Color::Basic(n) if n < 8  => vec![(40 + n) .to_string()],
            Color::Basic(n) if n < 16 => vec![(100 + (n - 8)).to_string()],
            Color::Basic(_) => vec!["49".into()],
            Color::Ansi256(n) => vec!["48".into(), "5".into(), n.to_string()],
            Color::Rgb(r,g,b) => vec!["48".into(), "2".into(), r.to_string(), g.to_string(), b.to_string()],
        }
    }
}

/// Famille de police SGR 10..19 (si supporté, sinon dégradé).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FontFamily {
    Default,
    Alt(u8), // 0..9 => SGR 10+n
}

/// Largeur/hauteur de ligne VT (séquences DEC privées).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum WidthMode {
    Normal,
    DoubleWidth,        // ESC # 6
    DoubleHeightTop,    // ESC # 3
    DoubleHeightBottom, // ESC # 4
}

/// Soulignement.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Underline {
    None,
    Single,     // 4
    Double,     // 21 (ou 4:2 si disponible)
}

/// Style complet.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Style {
    pub bold: bool,          // 1 / 22
    pub dim: bool,           // 2 / 22
    pub italic: bool,        // 3 / 23
    pub underline: Underline,// 4 / 21 / 24
    pub blink: bool,         // 5 / 25
    pub inverse: bool,       // 7 / 27
    pub hidden: bool,        // 8 / 28
    pub strike: bool,        // 9 / 29
    pub fg: Option<Color>,   // 39 si None
    pub bg: Option<Color>,   // 49 si None
    pub font: FontFamily,    // 10..19 si support
    pub width: WidthMode,    // ESC #3/#4/#6 si support
}

impl Default for Style {
    fn default() -> Self {
        Self {
            bold: false, dim: false, italic: false,
            underline: Underline::None, blink: false, inverse: false,
            hidden: false, strike: false,
            fg: None, bg: None,
            font: FontFamily::Default,
            width: WidthMode::Normal,
        }
    }
}

impl Style {
    /// Composition naïve : `self` overridé par `other`.
    pub fn compose(self, other: Style) -> Style {
        Style {
            bold: other.bold, dim: other.dim, italic: other.italic,
            underline: other.underline, blink: other.blink, inverse: other.inverse,
            hidden: other.hidden, strike: other.strike,
            fg: other.fg.or(self.fg),
            bg: other.bg.or(self.bg),
            font: other.font,
            width: other.width,
        }
    }

    /// Helpers builder.
    pub fn fg(mut self, c: Color) -> Self { self.fg = Some(c); self }
    pub fn bg(mut self, c: Color) -> Self { self.bg = Some(c); self }
    pub fn bold(mut self) -> Self { self.bold = true; self }
    pub fn italic(mut self) -> Self { self.italic = true; self }
    pub fn ul(mut self, u: Underline) -> Self { self.underline = u; self }
    pub fn inv(mut self) -> Self { self.inverse = true; self }
    pub fn strike(mut self) -> Self { self.strike = true; self }
    pub fn font(mut self, f: FontFamily) -> Self { self.font = f; self }
    pub fn width(mut self, w: WidthMode) -> Self { self.width = w; self }
}

/// Séquences élevées, y compris OSC 8 pour liens.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum SgrSeq {
    /// Réinitialisation complète (SGR 0).
    Reset,
    /// Applique un style.
    Set(Style),
    /// Lien hypertexte OSC 8: début.
    /// Émet: ESC ] 8 ; ; url ST  puis le texte doit suivre, puis `LinkEnd`.
    LinkStart { url: String, params: Option<String> },
    /// Lien hypertexte OSC 8: fin.
    LinkEnd,
    /// Brute (échappes déjà formées).
    Raw(String),
}

impl SgrSeq {
    /// Rend les échappes ANSI (CSI/OSC) en fonction du support.
    ///
    /// - `support_double` contrôle ESC #3/#4/#6.
    /// - `support_fonts` contrôle SGR 10..19.
    ///
    /// Les couleurs 256/RGB sont toujours émises si présentes. Les implémentations
    /// qui ne supportent pas dégraderont côté terminal (comportement standard).
    pub fn to_ansi(&self, support_double: bool, support_fonts: bool) -> String {
        match self {
            SgrSeq::Reset => "\x1b[0m".to_string(),
            SgrSeq::Set(s) => style_to_ansi(*s, support_double, support_fonts),
            SgrSeq::LinkStart { url, params } => osc8_start(url, params.as_deref()),
            SgrSeq::LinkEnd => osc8_end(),
            SgrSeq::Raw(s) => s.clone(),
        }
    }
}

impl fmt::Display for SgrSeq {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // Par défaut : on suppose pas de double width/polices.
        f.write_str(&self.to_ansi(false, false))
    }
}

/// ---- Internes

fn style_to_ansi(s: Style, support_double: bool, support_fonts: bool) -> String {
    let mut parts: Vec<String> = Vec::with_capacity(16);

    // Reset sélectifs pour états binaires
    if s.bold        { parts.push("1".into()); } else { parts.push("22".into()); }
    if s.dim         { parts.push("2".into()); } else { /* 22 déjà posé */ }
    if s.italic      { parts.push("3".into()); } else { parts.push("23".into()); }
    match s.underline {
        Underline::None   => parts.push("24".into()),
        Underline::Single => parts.push("4".into()),
        Underline::Double => parts.push("21".into()), // 21 = double underline (fallback portable)
    }
    if s.blink       { parts.push("5".into()); } else { parts.push("25".into()); }
    if s.inverse     { parts.push("7".into()); } else { parts.push("27".into()); }
    if s.hidden      { parts.push("8".into()); } else { parts.push("28".into()); }
    if s.strike      { parts.push("9".into()); } else { parts.push("29".into()); }

    // Couleurs
    match s.fg.unwrap_or(Color::Default) {
        Color::Default => parts.push("39".into()),
        c => parts.extend(c.to_sgr_fg()),
    }
    match s.bg.unwrap_or(Color::Default) {
        Color::Default => parts.push("49".into()),
        c => parts.extend(c.to_sgr_bg()),
    }

    // Police
    if support_fonts {
        match s.font {
            FontFamily::Default => parts.push("10".into()),
            FontFamily::Alt(n)  => parts.push((10 + (n as i16).clamp(0,9)) .to_string()),
        }
    }

    // Assemble SGR
    let mut out = String::new();
    if !parts.is_empty() {
        out.push_str("\x1b[");
        out.push_str(&parts.join(";"));
        out.push('m');
    }

    // Largeurs/hauteurs (séquences DEC privées)
    if support_double {
        match s.width {
            WidthMode::Normal             => {}
            WidthMode::DoubleWidth        => out.push_str("\x1b#6"),
            WidthMode::DoubleHeightTop    => out.push_str("\x1b#3"),
            WidthMode::DoubleHeightBottom => out.push_str("\x1b#4"),
        }
    }

    out
}

/// OSC 8 lien début: ESC ] 8 ; params ; url ST
fn osc8_start(url: &str, params: Option<&str>) -> String {
    let p = params.unwrap_or("");
    // Utilise ST = BEL pour compat xterm: ESC ] ... BEL
    // Alternative: ESC \ (ST) si besoin.
    format!("\x1b]8;{};{}\x07", p, url)
}

/// OSC 8 lien fin.
fn osc8_end() -> String {
    "\x1b]8;;\x07".to_string()
}

/// ---- Helpers publics

/// Construit une séquence de style minimale depuis attributs de commodité.
pub fn sgr_style(
    fg: Option<Color>, bg: Option<Color>,
    bold: bool, italic: bool, ul: Underline
) -> SgrSeq {
    SgrSeq::Set(Style {
        fg, bg, bold, italic,
        underline: ul,
        dim: false, blink: false, inverse: false, hidden: false, strike: false,
        font: FontFamily::Default,
        width: WidthMode::Normal,
    })
}

/// Raccourcis couleur.
pub mod colors {
    use super::Color;
    pub const BLACK:  Color = Color::Basic(0);
    pub const RED:    Color = Color::Basic(1);
    pub const GREEN:  Color = Color::Basic(2);
    pub const YELLOW: Color = Color::Basic(3);
    pub const BLUE:   Color = Color::Basic(4);
    pub const MAGENTA:Color = Color::Basic(5);
    pub const CYAN:   Color = Color::Basic(6);
    pub const WHITE:  Color = Color::Basic(7);
}

/// Tests basiques (compilables sans terminal).
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reset_is_csi_0_m() {
        assert_eq!(SgrSeq::Reset.to_ansi(false, false), "\x1b[0m");
    }

    #[test]
    fn rgb_fg_bg_encoded() {
        let st = Style::default()
            .fg(Color::Rgb(1,2,3))
            .bg(Color::Ansi256(200))
            .bold()
            .ul(Underline::Single);
        let s = SgrSeq::Set(st).to_ansi(false, false);
        assert!(s.contains("\x1b["));
        assert!(s.contains("38;2;1;2;3"));
        assert!(s.contains("48;5;200"));
        assert!(s.contains('m'));
    }

    #[test]
    fn font_and_doublewidth_emitted_when_supported() {
        let st = Style::default().font(FontFamily::Alt(2)).width(WidthMode::DoubleWidth);
        let s = SgrSeq::Set(st).to_ansi(true, true);
        assert!(s.contains("\x1b[12m")); // font
        assert!(s.contains("\x1b#6"));   // double width
    }

    #[test]
    fn osc8_link_sequences() {
        let start = SgrSeq::LinkStart { url: "https://example.com".into(), params: None }.to_ansi(false, false);
        let end = SgrSeq::LinkEnd.to_ansi(false, false);
        assert!(start.starts_with("\x1b]8;;https://example.com"));
        assert_eq!(end, "\x1b]8;;\x07");
    }
}
