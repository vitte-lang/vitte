// vitte-ansi/src/caps.rs
//! Détection ultra-complète des capacités ANSI du terminal.
//!
//! Couvre :
//! - TTY in/out
//! - ANSI SGR basique (CSI m)
//! - Palette 8/16, 256 couleurs, Truecolor 24-bit
//! - Hyperliens (OSC 8) par heuristique
//! - ConPTY sous Windows et activation VT (via `windows.rs`)
//! - Heuristiques TERM / COLORTERM / TERM_PROGRAM / NO_COLOR / FORCE_COLOR
//! - Options doubles largeurs et sélection de police (exposées mais rarement sûres)
//!
//! Zéro dépendance externe.

use std::env;
use std::io::{stdin, stdout, IsTerminal};

/// Drapeaux de capacités.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Capability {
    AnsiOut = 0,
    AnsiIn  = 1,
    Color8  = 2,
    Color256= 3,
    Truecolor = 4,
    Hyperlink = 5,   // OSC 8
    AnsiOsc   = 6,   // OSC support générique (titres, liens…)
    DoubleWidth = 7, // ESC #3/#4/#6 (rare)
    FontSelect = 8,  // SGR 10..19 (rare)
    ConPTY = 9,      // Windows pseudo console
}

/// Résultat agrégé.
#[derive(Clone, Debug)]
pub struct Capabilities {
    pub is_tty_out: bool,
    pub is_tty_in: bool,
    pub term: Option<String>,
    pub term_program: Option<String>,
    pub colorterm: Option<String>,
    pub flags: u32,
}

impl Capabilities {
    pub fn new() -> Self {
        Self {
            is_tty_out: false,
            is_tty_in: false,
            term: env::var("TERM").ok(),
            term_program: env::var("TERM_PROGRAM").ok(),
            colorterm: env::var("COLORTERM").ok(),
            flags: 0,
        }
    }
    #[inline]
    pub fn set(&mut self, c: Capability) { self.flags |= 1 << (c as u32); }
    #[inline]
    pub fn clear(&mut self, c: Capability) { self.flags &= !(1 << (c as u32)); }
    #[inline]
    pub fn has(&self, c: Capability) -> bool { (self.flags & (1 << (c as u32))) != 0 }

    /// Couleurs disponibles: 0, 8/16, 256, 16777216.
    pub fn color_level(&self) -> u32 {
        if self.has(Capability::Truecolor) { 1 << 24 }
        else if self.has(Capability::Color256) { 256 }
        else if self.has(Capability::Color8) { 16 } // inclut versions "bright"
        else { 0 }
    }
}

/// Détection principale multiplateforme.
/// Sous Windows, délègue l’activation VT/ConPTY à `windows.rs`, puis complète heuristiques.
#[cfg(windows)]
pub fn detect_capabilities() -> Capabilities {
    let mut caps = crate::windows::detect_capabilities_win();

    // Si pas TTY, limiter.
    caps.is_tty_out = stdout().is_terminal();
    caps.is_tty_in  = stdin().is_terminal();

    // Heuristiques Windows supplémentaires.
    let term = caps.term.clone().unwrap_or_default().to_lowercase();
    let tp = caps.term_program.clone().unwrap_or_default().to_lowercase();

    // Si VT activé (AnsiOut), on considère au moins 16 couleurs.
    if caps.has(Capability::AnsiOut) && caps.is_tty_out {
        caps.set(Capability::Color8);
        caps.set(Capability::AnsiOsc); // Windows Terminal supporte OSC
        // 256 ou truecolor si Windows Terminal ou TERM=xterm-256color
        if env_has("WT_SESSION") || term.contains("256") {
            caps.set(Capability::Color256);
        }
        if env_contains("COLORTERM", "truecolor") || env_contains("COLORTERM", "24bit") || env_has("WT_SESSION") {
            caps.set(Capability::Truecolor);
        }
        // Hyperliens OSC 8: Windows Terminal oui, ConEmu variable.
        if env_has("WT_SESSION") {
            caps.set(Capability::Hyperlink);
        }
    }

    apply_global_overrides(&mut caps);
    caps
}

/// Détection principale pour Unix-like.
#[cfg(not(windows))]
pub fn detect_capabilities() -> Capabilities {
    let mut caps = Capabilities::new();

    caps.is_tty_out = stdout().is_terminal();
    caps.is_tty_in  = stdin().is_terminal();

    // Cas non TTY: activer rendu virtuel côté bibliothèque, pas d’ANSI à émettre par défaut.
    if !caps.is_tty_out {
        return caps;
    }

    // TERM heuristics.
    let term_low = caps.term.clone().unwrap_or_default().to_lowercase();
    let tp = caps.term_program.clone().unwrap_or_default().to_lowercase();
    let colorterm_low = caps.colorterm.clone().unwrap_or_default().to_lowercase();

    // Base: si on a un TERM non vide et pas "dumb", activer ANSI in/out.
    if !term_low.is_empty() && term_low != "dumb" {
        caps.set(Capability::AnsiOut);
        caps.set(Capability::AnsiIn);
        caps.set(Capability::AnsiOsc); // la plupart des xterm et dérivés
        caps.set(Capability::Color8);
    }

    // 256 couleurs.
    if term_low.contains("256") || term_low.contains("xterm") || term_low.contains("screen-256color") {
        caps.set(Capability::Color256);
    }

    // Truecolor par heuristique: COLORTERM=truecolor|24bit ou iTerm2, modern xterm-256color + COLORTERM.
    if colorterm_low.contains("truecolor") || colorterm_low.contains("24bit") {
        caps.set(Capability::Truecolor);
    }
    if tp.contains("iterm.app") || env_contains("TERM_PROGRAM", "iTerm.app") {
        caps.set(Capability::Truecolor); // iTerm2 supporte truecolor
        caps.set(Capability::Hyperlink); // iTerm2 supporte OSC 8
    }
    if env_contains("VTE_VERSION", "10000") || env_var_nonempty("VTE_VERSION") {
        // VTE (gnome-terminal, etc.) >= 0.50 ~ hyperliens ; version exacte varie
        caps.set(Capability::Hyperlink);
    }

    // Hyperliens OSC 8: nombreux terminaux modernes. Heuristique large.
    if env_contains("TERM", "xterm")
        || env_contains("TERM", "rxvt")
        || env_contains("TERM", "tmux")
        || env_contains("TERM", "screen")
        || env_contains("TERM_PROGRAM", "Apple_Terminal")
        || env_contains("TERM_PROGRAM", "iTerm")
    {
        caps.set(Capability::Hyperlink);
    }

    // Apple Terminal: couleurs étendues partielles, truecolor approximé via dithering.
    if tp.contains("apple_terminal") {
        caps.set(Capability::Color256);
        // truecolor pas garanti: ne pas activer par défaut
    }

    // DoubleWidth / FontSelect: non fiables. Exposés mais désactivés.
    // Certains xterm/kitty les rendent; vous pouvez activer manuellement via env si besoin.
    if env_flag_true("VITTE_ANSI_DOUBLEWIDTH") { caps.set(Capability::DoubleWidth); }
    if env_flag_true("VITTE_ANSI_FONTSELECT")  { caps.set(Capability::FontSelect); }

    apply_global_overrides(&mut caps);
    caps
}

/// Applique NO_COLOR / FORCE_COLOR et forçages spécifiques.
fn apply_global_overrides(caps: &mut Capabilities) {
    // NO_COLOR: désactive toute couleur sur TTY, mais conserve AnsiOut pour resets basiques si nécessaire.
    // Spécification : https://no-color.org/
    if env_has("NO_COLOR") {
        caps.clear(Capability::Color8);
        caps.clear(Capability::Color256);
        caps.clear(Capability::Truecolor);
    }

    // FORCE_COLOR: force au moins 16 couleurs si TTY.
    // Valeurs: "0" désactive, "1" ou vide active 16, "2"/"3" activent 256 ou truecolor (heuristique).
    if let Ok(v) = env::var("FORCE_COLOR") {
        let v = v.trim();
        if v == "0" {
            caps.clear(Capability::Color8);
            caps.clear(Capability::Color256);
            caps.clear(Capability::Truecolor);
        } else {
            if caps.is_tty_out {
                caps.set(Capability::Color8);
            }
            if v == "2" { caps.set(Capability::Color256); }
            if v == "3" { caps.set(Capability::Truecolor); }
        }
    }

    // Forçages ciblés.
    if env_flag_true("VITTE_ANSI_FORCE_TRUECOLOR") { caps.set(Capability::Truecolor); }
    if env_flag_true("VITTE_ANSI_FORCE_256") { caps.set(Capability::Color256); caps.set(Capability::Color8); }
    if env_flag_true("VITTE_ANSI_DISABLE_OSC") { caps.clear(Capability::Hyperlink); caps.clear(Capability::AnsiOsc); }
}

/// Helpers env.
#[inline] fn env_has(k: &str) -> bool { env::var_os(k).is_some() }
#[inline] fn env_var_nonempty(k: &str) -> bool { env::var(k).map(|s| !s.trim().is_empty()).unwrap_or(false) }
#[inline] fn env_contains(k: &str, needle: &str) -> bool {
    env::var(k).map(|v| v.to_lowercase().contains(&needle.to_lowercase())).unwrap_or(false)
}
#[inline] fn env_flag_true(k: &str) -> bool {
    match env::var(k).ok().as_deref().map(str::trim) {
        Some("1") | Some("true") | Some("yes") | Some("on") | Some("") => true,
        _ => false,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn color_level_orders() {
        let mut c = Capabilities::new();
        assert_eq!(c.color_level(), 0);
        c.set(Capability::Color8);
        assert_eq!(c.color_level(), 16);
        c.set(Capability::Color256);
        assert_eq!(c.color_level(), 256);
        c.set(Capability::Truecolor);
        assert_eq!(c.color_level(), 1 << 24);
    }

    #[test]
    fn flags_set_has_clear() {
        let mut c = Capabilities::new();
        assert!(!c.has(Capability::AnsiOut));
        c.set(Capability::AnsiOut);
        assert!(c.has(Capability::AnsiOut));
        c.clear(Capability::AnsiOut);
        assert!(!c.has(Capability::AnsiOut));
    }
}
