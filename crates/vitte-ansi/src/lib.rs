#![deny(unsafe_code)]
//! vitte-ansi — utilitaires ANSI (SGR, couleurs, styles, curseur, effacements, OSC).
//!
//! ## Exemples rapides
//! ```rust
//! use vitte_ansi::{paint, Color, Style, cursor, clear, osc};
//! let hello = paint("Hello", Style::new().bold().fgrgb(255, 128, 0));
//! print!("{hello} {}", cursor::goto(1, 1));
//! print!("{}Ready", clear::screen());
//! print!("{}", osc::set_title("Vitte") );
//! ```
//!
//! Hyperlien terminal (iTerm2, kitty, Windows Terminal…):
//! ```rust
//! use vitte_ansi::osc;
//! println!("{}", osc::hyperlink("https://vitte-lang.dev", "Site Vitte"));
//! ```

use core::fmt;

/// CSI / OSC / SGR bas niveau.
pub mod esc {
    /// Escape introducers.
    pub const ESC: &str = "\x1b";
    pub const CSI: &str = "\x1b[";
    pub const OSC: &str = "\x1b]";
    pub const ST: &str = "\x1b\\";
    pub const BEL: &str = "\x07";

    /// Construit une séquence CSI: `CSI <args> <final>`.
    pub fn csi(args: &str, final_byte: char) -> alloc::string::String {
        let mut s = alloc::string::String::with_capacity(2 + args.len() + 1);
        s.push_str(CSI);
        s.push_str(args);
        s.push(final_byte);
        s
    }

    /// Construit une séquence OSC: `OSC <payload> ST/BEL`.
    pub fn osc(payload: &str) -> alloc::string::String {
        let mut s = alloc::string::String::with_capacity(2 + payload.len() + 2);
        s.push_str(OSC);
        s.push_str(payload);
        // De préférence ST, sinon BEL pour compatibilité.
        s.push_str(ST);
        s
    }
}

extern crate alloc;

/// Couleurs de base (8 + 8 brillantes).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[allow(non_camel_case_types)]
pub enum BasicColor {
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,
    BrightBlack,
    BrightRed,
    BrightGreen,
    BrightYellow,
    BrightBlue,
    BrightMagenta,
    BrightCyan,
    BrightWhite,
}

/// Couleur générique.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Color {
    Basic(BasicColor),
    Ansi256(u8),
    Rgb(u8, u8, u8),
}

impl Color {
    #[inline]
    pub fn basic(c: BasicColor) -> Self {
        Self::Basic(c)
    }
    #[inline]
    pub fn ansi256(n: u8) -> Self {
        Self::Ansi256(n)
    }
    #[inline]
    pub fn rgb(r: u8, g: u8, b: u8) -> Self {
        Self::Rgb(r, g, b)
    }
}

/// Style SGR complet.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Style {
    pub bold: bool,
    pub dim: bool,
    pub italic: bool,
    pub underline: bool,
    pub blink: bool,
    pub inverse: bool,
    pub hidden: bool,
    pub strike: bool,
    pub fg: Option<Color>,
    pub bg: Option<Color>,
}

impl Style {
    #[inline]
    pub const fn new() -> Self {
        Self {
            bold: false,
            dim: false,
            italic: false,
            underline: false,
            blink: false,
            inverse: false,
            hidden: false,
            strike: false,
            fg: None,
            bg: None,
        }
    }

    // Flags
    pub fn bold(mut self) -> Self {
        self.bold = true;
        self
    }
    pub fn dim(mut self) -> Self {
        self.dim = true;
        self
    }
    pub fn italic(mut self) -> Self {
        self.italic = true;
        self
    }
    pub fn underline(mut self) -> Self {
        self.underline = true;
        self
    }
    pub fn blink(mut self) -> Self {
        self.blink = true;
        self
    }
    pub fn inverse(mut self) -> Self {
        self.inverse = true;
        self
    }
    pub fn hidden(mut self) -> Self {
        self.hidden = true;
        self
    }
    pub fn strike(mut self) -> Self {
        self.strike = true;
        self
    }

    // Couleurs
    pub fn fg(mut self, c: Color) -> Self {
        self.fg = Some(c);
        self
    }
    pub fn bg(mut self, c: Color) -> Self {
        self.bg = Some(c);
        self
    }
    pub fn fgbasic(self, c: BasicColor) -> Self {
        self.fg(Color::basic(c))
    }
    pub fn fgn8(self, n: u8) -> Self {
        self.fg(Color::ansi256(n))
    }
    pub fn fgrgb(self, r: u8, g: u8, b: u8) -> Self {
        self.fg(Color::rgb(r, g, b))
    }
    pub fn bgbasic(self, c: BasicColor) -> Self {
        self.bg(Color::basic(c))
    }
    pub fn bgn8(self, n: u8) -> Self {
        self.bg(Color::ansi256(n))
    }
    pub fn bgrgb(self, r: u8, g: u8, b: u8) -> Self {
        self.bg(Color::rgb(r, g, b))
    }

    /// Génère la séquence SGR d’activation.
    pub fn prefix(&self) -> alloc::string::String {
        use BasicColor::*;
        let mut params: alloc::vec::Vec<alloc::string::String> = alloc::vec::Vec::with_capacity(8);
        if self.bold {
            params.push("1".into());
        }
        if self.dim {
            params.push("2".into());
        }
        if self.italic {
            params.push("3".into());
        }
        if self.underline {
            params.push("4".into());
        }
        if self.blink {
            params.push("5".into());
        }
        if self.inverse {
            params.push("7".into());
        }
        if self.hidden {
            params.push("8".into());
        }
        if self.strike {
            params.push("9".into());
        }

        if let Some(fg) = self.fg {
            match fg {
                Color::Basic(c) => {
                    let code = match c {
                        Black => 30,
                        Red => 31,
                        Green => 32,
                        Yellow => 33,
                        Blue => 34,
                        Magenta => 35,
                        Cyan => 36,
                        White => 37,
                        BrightBlack => 90,
                        BrightRed => 91,
                        BrightGreen => 92,
                        BrightYellow => 93,
                        BrightBlue => 94,
                        BrightMagenta => 95,
                        BrightCyan => 96,
                        BrightWhite => 97,
                    };
                    params.push(code.to_string());
                }
                Color::Ansi256(n) => {
                    params.push("38".into());
                    params.push("5".into());
                    params.push(n.to_string());
                }
                Color::Rgb(r, g, b) => {
                    params.push("38".into());
                    params.push("2".into());
                    params.push(r.to_string());
                    params.push(g.to_string());
                    params.push(b.to_string());
                }
            }
        }

        if let Some(bg) = self.bg {
            match bg {
                Color::Basic(c) => {
                    let base = match c {
                        Black => 40,
                        Red => 41,
                        Green => 42,
                        Yellow => 43,
                        Blue => 44,
                        Magenta => 45,
                        Cyan => 46,
                        White => 47,
                        BrightBlack => 100,
                        BrightRed => 101,
                        BrightGreen => 102,
                        BrightYellow => 103,
                        BrightBlue => 104,
                        BrightMagenta => 105,
                        BrightCyan => 106,
                        BrightWhite => 107,
                    };
                    params.push(base.to_string());
                }
                Color::Ansi256(n) => {
                    params.push("48".into());
                    params.push("5".into());
                    params.push(n.to_string());
                }
                Color::Rgb(r, g, b) => {
                    params.push("48".into());
                    params.push("2".into());
                    params.push(r.to_string());
                    params.push(g.to_string());
                    params.push(b.to_string());
                }
            }
        }

        if params.is_empty() {
            return alloc::string::String::from("");
        }
        let mut s = alloc::string::String::new();
        s.push_str(esc::CSI);
        for (i, p) in params.iter().enumerate() {
            if i > 0 {
                s.push(';');
            }
            s.push_str(p);
        }
        s.push('m');
        s
    }

    /// Séquence de reset (SGR 0).
    #[inline]
    pub fn suffix(&self) -> &'static str {
        "\x1b[0m"
    }

    /// Applique le style à un texte.
    pub fn paint<'a>(&self, text: impl Into<alloc::borrow::Cow<'a, str>>) -> alloc::string::String {
        let t = text.into();
        let mut out = alloc::string::String::with_capacity(self.prefix().len() + t.len() + 4);
        out.push_str(&self.prefix());
        out.push_str(&t);
        out.push_str(self.suffix());
        out
    }
}

/// Raccourci direct.
#[inline]
pub fn paint<'a>(
    text: impl Into<alloc::borrow::Cow<'a, str>>,
    style: Style,
) -> alloc::string::String {
    style.paint(text)
}

/// Préréglages de styles courants.
pub mod styles {
    use super::{BasicColor, Color, Style};
    #[inline]
    pub fn reset() -> &'static str {
        "\x1b[0m"
    }
    #[inline]
    pub fn bold() -> &'static str {
        "\x1b[1m"
    }
    #[inline]
    pub fn dim() -> &'static str {
        "\x1b[2m"
    }
    #[inline]
    pub fn italic() -> &'static str {
        "\x1b[3m"
    }
    #[inline]
    pub fn underline() -> &'static str {
        "\x1b[4m"
    }
    #[inline]
    pub fn blink() -> &'static str {
        "\x1b[5m"
    }
    #[inline]
    pub fn inverse() -> &'static str {
        "\x1b[7m"
    }
    #[inline]
    pub fn hidden() -> &'static str {
        "\x1b[8m"
    }
    #[inline]
    pub fn strikethrough() -> &'static str {
        "\x1b[9m"
    }

    #[inline]
    pub fn fg(c: BasicColor) -> alloc::string::String {
        Style::new().fg(Color::Basic(c)).prefix()
    }
    #[inline]
    pub fn bg(c: BasicColor) -> alloc::string::String {
        Style::new().bg(Color::Basic(c)).prefix()
    }
    #[inline]
    pub fn fgn8(n: u8) -> alloc::string::String {
        Style::new().fgn8(n).prefix()
    }
    #[inline]
    pub fn bgn8(n: u8) -> alloc::string::String {
        Style::new().bgn8(n).prefix()
    }
    #[inline]
    pub fn fgrgb(r: u8, g: u8, b: u8) -> alloc::string::String {
        Style::new().fgrgb(r, g, b).prefix()
    }
    #[inline]
    pub fn bgrgb(r: u8, g: u8, b: u8) -> alloc::string::String {
        Style::new().bgrgb(r, g, b).prefix()
    }
}

/// Contrôles curseur et effacements.
pub mod cursor {
    use super::esc;

    #[inline]
    pub fn up(n: u32) -> alloc::string::String {
        esc::csi(&n.to_string(), "A".chars().next().unwrap())
    }
    #[inline]
    pub fn down(n: u32) -> alloc::string::String {
        esc::csi(&n.to_string(), 'B')
    }
    #[inline]
    pub fn forward(n: u32) -> alloc::string::String {
        esc::csi(&n.to_string(), 'C')
    }
    #[inline]
    pub fn back(n: u32) -> alloc::string::String {
        esc::csi(&n.to_string(), 'D')
    }
    #[inline]
    pub fn next_line(n: u32) -> alloc::string::String {
        esc::csi(&n.to_string(), 'E')
    }
    #[inline]
    pub fn prev_line(n: u32) -> alloc::string::String {
        esc::csi(&n.to_string(), 'F')
    }
    #[inline]
    pub fn horiz_abs(col: u32) -> alloc::string::String {
        esc::csi(&col.to_string(), 'G')
    }
    #[inline]
    pub fn goto(row: u32, col: u32) -> alloc::string::String {
        let mut a = alloc::string::String::new();
        use core::fmt::Write;
        let _ = write!(&mut a, "{};{}", row, col);
        esc::csi(&a, 'H')
    }
    #[inline]
    pub fn save() -> &'static str {
        "\x1b7"
    } // DECSC
    #[inline]
    pub fn restore() -> &'static str {
        "\x1b8"
    } // DECRC
    #[inline]
    pub fn show() -> &'static str {
        "\x1b[?25h"
    }
    #[inline]
    pub fn hide() -> &'static str {
        "\x1b[?25l"
    }
}

pub mod clear {
    use super::esc;
    /// 0 = du curseur à la fin, 1 = du début au curseur, 2 = écran entier, 3 = écran + scrollback.
    #[inline]
    pub fn ed(mode: u8) -> alloc::string::String {
        esc::csi(&mode.to_string(), 'J')
    }
    /// 0 = du curseur à fin de ligne, 1 = début→curseur, 2 = ligne entière.
    #[inline]
    pub fn el(mode: u8) -> alloc::string::String {
        esc::csi(&mode.to_string(), 'K')
    }
    #[inline]
    pub fn screen() -> alloc::string::String {
        ed(2)
    }
    #[inline]
    pub fn line() -> alloc::string::String {
        el(2)
    }
    #[inline]
    pub fn scroll_up(n: u32) -> alloc::string::String {
        esc::csi(&n.to_string(), 'S')
    }
    #[inline]
    pub fn scroll_down(n: u32) -> alloc::string::String {
        esc::csi(&n.to_string(), 'T')
    }
}

/// OSC utilitaires (titre, hyperliens…).
pub mod osc {
    use super::esc;

    /// Définit le titre de la fenêtre/onglet (OSC 0).
    pub fn set_title(title: &str) -> alloc::string::String {
        let mut payload = alloc::string::String::from("0;");
        payload.push_str(title);
        // ST de préférence
        esc::osc(&payload)
    }

    /// Hyperlien cliquable: OSC 8 ; params ; URI ST  texte  OSC 8 ;; ST
    pub fn hyperlink(uri: &str, text: &str) -> alloc::string::String {
        let mut open = alloc::string::String::from("8;;");
        open.push_str(uri);
        let mut out = alloc::string::String::new();
        out.push_str(&esc::osc(&open));
        out.push_str(text);
        out.push_str(&esc::osc("8;;"));
        out
    }

    /// Écrit une note/annotation (OSC 9 ; message ST) — support variable selon terminal.
    pub fn note(message: &str) -> alloc::string::String {
        let mut payload = alloc::string::String::from("9;");
        payload.push_str(message);
        esc::osc(&payload)
    }
}

/// Détection d’environnement couleur.
pub mod detect {
    /// `NO_COLOR` désactive les couleurs.
    pub fn no_color() -> bool {
        match std::env::var("NO_COLOR") {
            Ok(v) => !v.is_empty(),
            Err(_) => false,
        }
    }

    /// Truecolor/24-bit support.
    pub fn supports_truecolor() -> bool {
        if no_color() {
            return false;
        }
        let ok = std::env::var("COLORTERM").unwrap_or_default().to_lowercase();
        ok.contains("truecolor") || ok.contains("24bit")
    }
}

/// Retire les séquences ANSI d’une chaîne.
pub fn strip_ansi(input: &str) -> alloc::string::String {
    let mut out = alloc::string::String::with_capacity(input.len());
    let mut chars = input.chars().peekable();

    while let Some(ch) = chars.next() {
        if ch == '\x1b' {
            // CSI, OSC, ou autre
            match chars.peek().copied() {
                Some('[') => {
                    // CSI: ESC [ ... letter
                    chars.next();
                    // Consomme jusqu’à un byte final 0x40–0x7E
                    while let Some(c) = chars.next() {
                        if ('@'..='~').contains(&c) {
                            break;
                        }
                    }
                }
                Some(']') => {
                    // OSC: ESC ] ... BEL ou ST
                    chars.next();
                    // Consomme jusqu’à BEL (\x07) ou ST (ESC \)
                    let mut prev_esc = false;
                    while let Some(c) = chars.next() {
                        if c == '\x07' {
                            break;
                        } // BEL
                        if prev_esc && c == '\\' {
                            break;
                        } // ST
                        prev_esc = c == '\x1b';
                    }
                }
                Some(_) => {
                    // Autre séquence courte: ESC <char>, ESC 7/8, etc.
                    // On ignore le prochain char si présent.
                    let _ = chars.next();
                }
                None => break,
            }
        } else {
            out.push(ch);
        }
    }

    out
}

/// Écrit dans un `fmt::Write`, utile pour buffers alloués.
pub fn write_sgr(mut w: impl fmt::Write, style: &Style, text: &str) -> fmt::Result {
    w.write_str(&style.prefix())?;
    w.write_str(text)?;
    w.write_str(style.suffix())
}

/// Helpers de couleurs de base.
pub mod basic {
    use super::{BasicColor::*, Color, Style};
    #[inline]
    pub fn black() -> Style {
        Style::new().fg(Color::basic(Black))
    }
    #[inline]
    pub fn red() -> Style {
        Style::new().fg(Color::basic(Red))
    }
    #[inline]
    pub fn green() -> Style {
        Style::new().fg(Color::basic(Green))
    }
    #[inline]
    pub fn yellow() -> Style {
        Style::new().fg(Color::basic(Yellow))
    }
    #[inline]
    pub fn blue() -> Style {
        Style::new().fg(Color::basic(Blue))
    }
    #[inline]
    pub fn magenta() -> Style {
        Style::new().fg(Color::basic(Magenta))
    }
    #[inline]
    pub fn cyan() -> Style {
        Style::new().fg(Color::basic(Cyan))
    }
    #[inline]
    pub fn white() -> Style {
        Style::new().fg(Color::basic(White))
    }

    #[inline]
    pub fn bright_black() -> Style {
        Style::new().fg(Color::basic(BrightBlack))
    }
    #[inline]
    pub fn bright_red() -> Style {
        Style::new().fg(Color::basic(BrightRed))
    }
    #[inline]
    pub fn bright_green() -> Style {
        Style::new().fg(Color::basic(BrightGreen))
    }
    #[inline]
    pub fn bright_yellow() -> Style {
        Style::new().fg(Color::basic(BrightYellow))
    }
    #[inline]
    pub fn bright_blue() -> Style {
        Style::new().fg(Color::basic(BrightBlue))
    }
    #[inline]
    pub fn bright_magenta() -> Style {
        Style::new().fg(Color::basic(BrightMagenta))
    }
    #[inline]
    pub fn bright_cyan() -> Style {
        Style::new().fg(Color::basic(BrightCyan))
    }
    #[inline]
    pub fn bright_white() -> Style {
        Style::new().fg(Color::basic(BrightWhite))
    }
}

/// API de composition: `paint!(style, "txt {} {}", a, b)`.
#[macro_export]
macro_rules! paintf {
    ($style:expr, $($arg:tt)*) => {{
        let s = alloc::format!($($arg)*);
        $style.paint(s)
    }};
}

#[cfg(test)]
mod tests {
    use super::BasicColor::*;
    use super::*;

    #[test]
    fn style_prefix_basic() {
        let s = Style::new().fg(Color::basic(Red)).bold().underline().prefix();
        assert!(s.starts_with("\x1b[")); // CSI
        assert!(s.ends_with('m'));
        // ordre peut varier selon construction; vérifie présence
        assert!(s.contains("31")); // red
        assert!(s.contains('1')); // bold
        assert!(s.contains('4')); // underline
    }

    #[test]
    fn paint_and_strip() {
        let styled = paint("X", Style::new().bold().fg(Color::rgb(1, 2, 3)));
        assert!(styled.starts_with("\x1b["));
        assert!(styled.ends_with("\x1b[0m"));
        let plain = strip_ansi(&styled);
        assert_eq!(plain, "X");
    }

    #[test]
    fn cursor_codes() {
        assert_eq!(cursor::goto(2, 3), "\x1b[2;3H");
        assert_eq!(cursor::up(5), "\x1b[5A");
        assert_eq!(cursor::hide(), "\x1b[?25l");
    }

    #[test]
    fn clear_codes() {
        assert_eq!(clear::screen(), "\x1b[2J");
        assert_eq!(clear::line(), "\x1b[2K");
    }

    #[test]
    fn osc_title_and_link() {
        let t = osc::set_title("Vitte");
        assert!(t.starts_with("\x1b]0;"));
        assert!(t.ends_with("\x1b\\"));
        let h = osc::hyperlink("https://example.com", "click");
        assert!(h.contains("]8;;https://example.com\x1b\\"));
        assert!(h.ends_with("\x1b]8;;\x1b\\"));
    }

    #[test]
    fn basic_helpers() {
        let s = basic::bright_green().prefix();
        assert!(s.contains("[92m"));
    }

    #[test]
    fn write_sgr_buf() {
        let mut buf = alloc::string::String::new();
        write_sgr(&mut buf, &Style::new().bold(), "ok").unwrap();
        assert!(buf.starts_with("\x1b[1m"));
        assert!(buf.ends_with("\x1b[0m"));
        assert!(strip_ansi(&buf) == "ok");
    }

    #[test]
    fn detect_truecolor_no_panic() {
        // Ne garantit rien sur l'env du runner, juste un appel.
        let _ = detect::supports_truecolor();
    }
}
