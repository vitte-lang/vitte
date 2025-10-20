//! vitte-banner — génération de bannières ASCII/Unicode pour CLI.
//!
//! - Polices intégrées: Simple, Block, Outline, Shadow
//! - Alignement: Left, Center, Right
//! - Spécification complète via `BannerSpec` (builder):
//!   width, margin, padding, fill, border, title, separator
//! - Calcul de largeur Unicode si feature `unicode-width` activée
//! - Zéro unsafe, zéro dépendance obligatoire
//!
//! Exemple rapide:
//! ```no_run
//! use vitte_banner::{BannerSpec, Font, Align, BorderStyle};
//! let spec = BannerSpec::new("Vitte")
//!     .font(Font::Block)
//!     .align(Align::Center)
//!     .width(48)
//!     .padding(1, 2)
//!     .border(BorderStyle::Rounded)
//!     .title("Demo");
//! println!("{}", spec.render());
//! ```

#![forbid(unsafe_code)]

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.");

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{string::String, vec::Vec};

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "errors")]
use thiserror::Error;

#[cfg(feature = "unicode-width")]
use unicode_width::UnicodeWidthStr;

/* =============================== TYPES =============================== */

#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Align {
    Left,
    Center,
    Right,
}

#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Font {
    Simple,
    Block,
    Outline,
    Shadow,
}

#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BorderStyle {
    None,
    Plain,
    Thick,
    Rounded,
    Double,
}

#[cfg(feature = "errors")]
#[derive(Debug, Error, PartialEq, Eq)]
pub enum BannerError {
    #[error("invalid width (must be >= 1)")]
    InvalidWidth,
    #[error("content wider than width")]
    ContentWiderThanWidth,
}
#[cfg(not(feature = "errors"))]
#[derive(Debug, PartialEq, Eq)]
pub enum BannerError {
    InvalidWidth,
    ContentWiderThanWidth,
}

#[cfg(feature = "errors")]
pub type Result<T> = core::result::Result<T, BannerError>;
#[cfg(not(feature = "errors"))]
pub type Result<T> = core::result::Result<T, &'static str>;

#[inline]
fn err<T>(e: BannerError) -> Result<T> {
    #[cfg(feature = "errors")]
    {
        Err(e)
    }
    #[cfg(not(feature = "errors"))]
    {
        let msg: &'static str = match e {
            BannerError::InvalidWidth => "invalid width (must be >= 1)",
            BannerError::ContentWiderThanWidth => "content wider than width",
        };
        Err(msg)
    }
}

#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct BannerSpec {
    text: String,
    font: Font,
    align: Align,
    width: usize,
    pad_v: usize,
    pad_h: usize,
    margin_v: usize,
    margin_h: usize,
    fill: char,
    border: BorderStyle,
    title: Option<String>,
    separator: bool,
}

impl BannerSpec {
    pub fn new<T: Into<String>>(text: T) -> Self {
        Self {
            text: text.into(),
            font: Font::Simple,
            align: Align::Left,
            width: 0, // 0 => auto
            pad_v: 0,
            pad_h: 0,
            margin_v: 0,
            margin_h: 0,
            fill: ' ',
            border: BorderStyle::None,
            title: None,
            separator: false,
        }
    }
    pub fn font(mut self, f: Font) -> Self {
        self.font = f;
        self
    }
    pub fn align(mut self, a: Align) -> Self {
        self.align = a;
        self
    }
    pub fn width(mut self, w: usize) -> Self {
        self.width = w;
        self
    }
    pub fn padding(mut self, v: usize, h: usize) -> Self {
        self.pad_v = v;
        self.pad_h = h;
        self
    }
    pub fn margin(mut self, v: usize, h: usize) -> Self {
        self.margin_v = v;
        self.margin_h = h;
        self
    }
    pub fn fill(mut self, ch: char) -> Self {
        self.fill = ch;
        self
    }
    pub fn border(mut self, b: BorderStyle) -> Self {
        self.border = b;
        self
    }
    pub fn title<T: Into<String>>(mut self, t: T) -> Self {
        self.title = Some(t.into());
        self
    }
    pub fn separator(mut self, on: bool) -> Self {
        self.separator = on;
        self
    }

    /// Rend la bannière complète.
    pub fn render(&self) -> String {
        self.render_result().unwrap_or_else(|_| String::new())
    }

    /// Rend avec gestion d’erreurs.
    pub fn render_result(&self) -> Result<String> {
        let mut glyph_lines = match self.font {
            Font::Simple => font_simple(&self.text),
            Font::Block => font_block(&self.text),
            Font::Outline => font_outline(&self.text),
            Font::Shadow => font_shadow(&self.text),
        };

        // Calcul largeur intérieure (hors marges et bordures)
        let mut content_width = glyph_lines.iter().map(|l| display_width(l)).max().unwrap_or(0);
        if self.pad_h > 0 {
            content_width += self.pad_h * 2;
        }

        // Largeur extérieure demandée (si width=0 => auto)
        let border_w = border_side_width(self.border) * 2;
        let outer_width = if self.width == 0 {
            content_width + border_w
        } else {
            if self.width < 1 {
                return err(BannerError::InvalidWidth);
            }
            self.width
        };

        // Largeur intérieure réelle
        let inner_width = outer_width.saturating_sub(border_w);
        if content_width > inner_width {
            return err(BannerError::ContentWiderThanWidth);
        }

        // Applique alignement + padding vertical/horizontal
        let mut content: Vec<String> = Vec::new();

        // Titre optionnel (au-dessus)
        if let Some(t) = &self.title {
            let title_line = align_line(t, inner_width, Align::Center);
            content.push(title_line);
            if self.separator {
                content.push(repeat_char('─', inner_width));
            }
        }

        // Padding vertical top
        for _ in 0..self.pad_v {
            content.push(repeat_char(self.fill, inner_width));
        }

        // Lignes de glyphes
        for line in glyph_lines.iter_mut() {
            let mut row = String::new();
            // padding horizontal
            row.push_str(&repeat_char(self.fill, self.pad_h));
            // align interne du texte dans la zone utile
            let avail = inner_width.saturating_sub(self.pad_h * 2);
            row.push_str(&align_line_exact(line, avail, self.align, self.fill));
            row.push_str(&repeat_char(self.fill, self.pad_h));
            // sanity
            content.push(crop_or_pad(&row, inner_width, self.fill));
        }

        // Padding vertical bottom
        for _ in 0..self.pad_v {
            content.push(repeat_char(self.fill, inner_width));
        }

        // Ajoute bordures
        let boxed = add_border(&content, inner_width, self.border);

        // Ajoute marges autour
        let mut out = String::new();
        for _ in 0..self.margin_v {
            out.push('\n');
        }
        let margin_side = repeat_char(' ', self.margin_h);
        for line in boxed {
            out.push_str(&margin_side);
            out.push_str(&line);
            out.push_str(&margin_side);
            out.push('\n');
        }
        for _ in 0..self.margin_v {
            out.push('\n');
        }

        Ok(out)
    }
}

/* ============================== FONTS ============================== */

fn font_simple(text: &str) -> Vec<String> {
    vec![text.to_string()]
}

fn font_outline(text: &str) -> Vec<String> {
    let mut out = Vec::new();
    out.push(format!("+{}+", repeat_char('-', text.len())));
    out.push(format!("|{}|", text));
    out.push(format!("+{}+", repeat_char('-', text.len())));
    out
}

fn font_block(text: &str) -> Vec<String> {
    let mut lines = vec![String::new(); 5];
    for ch in text.chars() {
        let glyph = block_glyph(ch);
        for (i, g) in glyph.iter().enumerate() {
            lines[i].push_str(g);
            lines[i].push(' ');
        }
    }
    lines
}

fn font_shadow(text: &str) -> Vec<String> {
    // Ombre simple à droite et en bas
    let base = font_block(text);
    let mut out: Vec<String> = base.clone();
    // applique ombre
    for (i, l) in base.iter().enumerate() {
        let mut s = String::new();
        s.push_str(l);
        s.push(' '); // décalage horizontal
        s.push_str(&l.replace(|c| c != ' ', "·"));
        // combine partiellement selon la ligne pour simuler ombre basse
        if i + 1 < out.len() {
            out[i + 1].push(' ');
        }
        out[i] = s;
    }
    out
}

/* ========================== BLOCK GLYPHS =========================== */

fn block_glyph(ch: char) -> [&'static str; 5] {
    match ch.to_ascii_uppercase() {
        'A' => ["  A  ", " A A ", "AAAAA", "A   A", "A   A"],
        'B' => ["BBBB ", "B   B", "BBBB ", "B   B", "BBBB "],
        'C' => [" CCC ", "C   C", "C    ", "C   C", " CCC "],
        'D' => ["DDD  ", "D  D ", "D   D", "D  D ", "DDD  "],
        'E' => ["EEEEE", "E    ", "EEE  ", "E    ", "EEEEE"],
        'F' => ["FFFFF", "F    ", "FFF  ", "F    ", "F    "],
        'G' => [" GGG ", "G    ", "G  GG", "G   G", " GGG "],
        'H' => ["H   H", "H   H", "HHHHH", "H   H", "H   H"],
        'I' => ["IIIII", "  I  ", "  I  ", "  I  ", "IIIII"],
        'J' => ["JJJJJ", "    J", "    J", "J   J", " JJJ "],
        'K' => ["K   K", "K  K ", "KKK  ", "K  K ", "K   K"],
        'L' => ["L    ", "L    ", "L    ", "L    ", "LLLLL"],
        'M' => ["M   M", "MM MM", "M M M", "M   M", "M   M"],
        'N' => ["N   N", "NN  N", "N N N", "N  NN", "N   N"],
        'O' => [" OOO ", "O   O", "O   O", "O   O", " OOO "],
        'P' => ["PPPP ", "P   P", "PPPP ", "P    ", "P    "],
        'Q' => [" QQQ ", "Q   Q", "Q   Q", "Q  Q ", " QQ Q"],
        'R' => ["RRRR ", "R   R", "RRRR ", "R R  ", "R  RR"],
        'S' => [" SSS ", "S    ", " SSS ", "    S", " SSS "],
        'T' => ["TTTTT", "  T  ", "  T  ", "  T  ", "  T  "],
        'U' => ["U   U", "U   U", "U   U", "U   U", " UUU "],
        'V' => ["V   V", "V   V", "V   V", " V V ", "  V  "],
        'W' => ["W   W", "W   W", "W W W", "WW WW", "W   W"],
        'X' => ["X   X", " X X ", "  X  ", " X X ", "X   X"],
        'Y' => ["Y   Y", " Y Y ", "  Y  ", "  Y  ", "  Y  "],
        'Z' => ["ZZZZZ", "   Z ", "  Z  ", " Z   ", "ZZZZZ"],
        '0' => [" 000 ", "0  00", "0 0 0", "00  0", " 000 "],
        '1' => ["  1  ", " 11  ", "  1  ", "  1  ", "11111"],
        '2' => [" 222 ", "2   2", "   2 ", "  2  ", "22222"],
        '3' => ["3333 ", "    3", " 333 ", "    3", "3333 "],
        '4' => ["4  4 ", "4  4 ", "44444", "   4 ", "   4 "],
        '5' => ["55555", "5    ", "5555 ", "    5", "5555 "],
        '6' => [" 666 ", "6    ", "6666 ", "6   6", " 666 "],
        '7' => ["77777", "   7 ", "  7  ", " 7   ", "7    "],
        '8' => [" 888 ", "8   8", " 888 ", "8   8", " 888 "],
        '9' => [" 999 ", "9   9", " 9999", "    9", " 999 "],
        ' ' => ["     ", "     ", "     ", "     ", "     "],
        _ => ["?????", "?????", "?????", "?????", "?????"],
    }
}

/* ============================== BORDURES ============================== */

fn border_chars(
    style: BorderStyle,
) -> Option<(&'static str, &'static str, &'static str, &'static str, &'static str, &'static str)> {
    match style {
        BorderStyle::None => None,
        BorderStyle::Plain => Some(("+", "+", "+", "+", "-", "|")),
        BorderStyle::Thick => Some(("┏", "┓", "┗", "┛", "━", "┃")),
        BorderStyle::Rounded => Some(("╭", "╮", "╰", "╯", "─", "│")),
        BorderStyle::Double => Some(("╔", "╗", "╚", "╝", "═", "║")),
    }
}

fn border_side_width(style: BorderStyle) -> usize {
    if matches!(style, BorderStyle::None) {
        0
    } else {
        1
    }
}

fn add_border(content: &[String], inner_width: usize, style: BorderStyle) -> Vec<String> {
    if let Some((tl, tr, bl, br, h, v)) = border_chars(style) {
        let mut out = Vec::with_capacity(content.len() + 2);
        out.push(format!(
            "{}{}{}",
            tl,
            repeat_char(h.chars().next().unwrap_or('-'), inner_width),
            tr
        ));
        for line in content {
            out.push(format!("{}{}{}", v, pad_exact(line, inner_width), v));
        }
        out.push(format!(
            "{}{}{}",
            bl,
            repeat_char(h.chars().next().unwrap_or('-'), inner_width),
            br
        ));
        out
    } else {
        content.iter().map(|s| pad_exact(s, inner_width)).collect()
    }
}

/* ============================== HELPERS ============================== */

fn display_width(s: &str) -> usize {
    #[cfg(feature = "unicode-width")]
    {
        UnicodeWidthStr::width(s)
    }
    #[cfg(not(feature = "unicode-width"))]
    {
        s.len()
    }
}

fn repeat_char(ch: char, n: usize) -> String {
    let mut s = String::new();
    for _ in 0..n {
        s.push(ch);
    }
    s
}

fn pad_exact(s: &str, width: usize) -> String {
    let w = display_width(s);
    if w >= width {
        crop_or_pad(s, width, ' ')
    } else {
        let mut out = String::with_capacity(width);
        out.push_str(s);
        out.push_str(&repeat_char(' ', width - w));
        out
    }
}

fn crop_or_pad(s: &str, width: usize, fill: char) -> String {
    let w = display_width(s);
    if w == width {
        return s.to_string();
    }
    if w < width {
        let mut out = String::with_capacity(width);
        out.push_str(s);
        out.push_str(&repeat_char(fill, width - w));
        return out;
    }
    // crop simpliste par bytes; acceptable pour ASCII/Block. Pour Unicode large,
    // activer feature unicode-width et améliorer si besoin.
    s.chars().take(width).collect()
}

fn align_line(s: &str, width: usize, align: Align) -> String {
    align_line_exact(s, width, align, ' ')
}

fn align_line_exact(s: &str, width: usize, align: Align, fill: char) -> String {
    let w = display_width(s);
    if w >= width {
        return crop_or_pad(s, width, fill);
    }
    let pad = width - w;
    match align {
        Align::Left => {
            let mut out = String::with_capacity(width);
            out.push_str(s);
            out.push_str(&repeat_char(fill, pad));
            out
        }
        Align::Right => {
            let mut out = String::with_capacity(width);
            out.push_str(&repeat_char(fill, pad));
            out.push_str(s);
            out
        }
        Align::Center => {
            let left = pad / 2;
            let right = pad - left;
            let mut out = String::with_capacity(width);
            out.push_str(&repeat_char(fill, left));
            out.push_str(s);
            out.push_str(&repeat_char(fill, right));
            out
        }
    }
}

/* ================================ TESTS ================================ */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn simple_auto_width() {
        let out = BannerSpec::new("Vitte").font(Font::Simple).align(Align::Left).render();
        assert!(out.contains("Vitte"));
    }

    #[test]
    fn block_center_box() {
        let spec = BannerSpec::new("VITTE")
            .font(Font::Block)
            .align(Align::Center)
            .width(40)
            .padding(1, 2)
            .border(BorderStyle::Rounded)
            .title("Demo")
            .separator(true);
        let out = spec.render();
        assert!(out.lines().count() > 3);
        assert!(out.contains("╭"));
        assert!(out.contains("╯"));
    }

    #[test]
    fn outline_right_margin() {
        let spec =
            BannerSpec::new("Hi").font(Font::Outline).align(Align::Right).width(20).margin(1, 2);
        let out = spec.render();
        assert!(out.contains("+--+"));
        assert!(out.lines().next().unwrap().starts_with("  "));
    }

    #[test]
    fn shadow_effect() {
        let out = BannerSpec::new("A").font(Font::Shadow).width(20).render();
        assert!(out.contains("·"));
    }

    #[test]
    fn too_narrow_detected() {
        let spec = BannerSpec::new("WIDE").font(Font::Block).width(5);
        let err = spec.render_result().unwrap_err();
        #[cfg(feature = "errors")]
        assert!(matches!(err, BannerError::ContentWiderThanWidth));
    }
}
