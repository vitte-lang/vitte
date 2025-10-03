//! vitte-style — styling utilities stub for the Vitte tooling.
//!
//! The concrete theming system (colors, borders, ANSI integration) is not yet
//! implemented. This placeholder keeps a compatible API while returning plain
//! strings so dependent crates continue to compile.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::{format, string::String, vec::Vec};

/// Logical color role identifiers.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Role {
    /// Primary text color.
    Primary,
    /// Success / positive message.
    Success,
    /// Warning message.
    Warn,
    /// Error message.
    Error,
    /// Informational message.
    Info,
    /// Muted or secondary text.
    Muted,
    /// Inverted style.
    Invert,
}

/// Set of symbols used by helpers (check mark, cross, etc.).
#[derive(Clone, Debug)]
pub struct Symbols {
    /// Success indicator.
    pub ok: &'static str,
    /// Error indicator.
    pub err: &'static str,
    /// Warning indicator.
    pub warn: &'static str,
    /// Info indicator.
    pub info: &'static str,
    /// Arrow used for breadcrumbs.
    pub arrow: &'static str,
    /// Bullet for lists.
    pub bullet: &'static str,
}

impl Default for Symbols {
    fn default() -> Self {
        Self { ok: "✔", err: "✘", warn: "⚠", info: "ℹ", arrow: "→", bullet: "•" }
    }
}

/// Placeholder theme.
#[derive(Clone, Debug)]
pub struct Theme {
    /// Theme name.
    pub name: String,
    /// Whether colors should be applied.
    pub use_color: bool,
    /// Symbols used by helpers.
    pub symbols: Symbols,
}

impl Default for Theme {
    fn default() -> Self {
        Self { name: "default".into(), use_color: false, symbols: Symbols::default() }
    }
}

/// Returns a dark preset theme (stubbed, identical to [`Theme::default`]).
pub fn preset_dark() -> Theme {
    Theme { name: "dark".into(), ..Theme::default() }
}

/// Returns a light preset theme (stubbed, identical to [`Theme::default`]).
pub fn preset_light() -> Theme {
    Theme { name: "light".into(), ..Theme::default() }
}

/// Detects a theme using environment hints (stubbed to dark theme).
pub fn detect_theme() -> Theme {
    preset_dark()
}

/// Lightweight style renderer.
#[derive(Clone, Debug)]
pub struct Style {
    theme: Theme,
}

impl Style {
    /// Creates a new style wrapper.
    pub fn new(theme: Theme) -> Self {
        Self { theme }
    }

    /// Returns a reference to the underlying theme.
    pub fn theme(&self) -> &Theme {
        &self.theme
    }

    /// Overrides symbols.
    pub fn with_symbols(mut self, symbols: Symbols) -> Self {
        self.theme.symbols = symbols;
        self
    }

    /// Enables or disables color usage (no-op in the stub).
    pub fn use_color(mut self, on: bool) -> Self {
        self.theme.use_color = on;
        self
    }

    /// Formats primary text.
    pub fn primary(&self, text: impl AsRef<str>) -> String {
        text.as_ref().to_string()
    }

    /// Formats success text with icon.
    pub fn ok(&self, text: impl AsRef<str>) -> String {
        format!("{} {}", self.theme.symbols.ok, text.as_ref())
    }

    /// Formats warning text with icon.
    pub fn warn(&self, text: impl AsRef<str>) -> String {
        format!("{} {}", self.theme.symbols.warn, text.as_ref())
    }

    /// Formats error text with icon.
    pub fn err(&self, text: impl AsRef<str>) -> String {
        format!("{} {}", self.theme.symbols.err, text.as_ref())
    }

    /// Formats informational text with icon.
    pub fn info(&self, text: impl AsRef<str>) -> String {
        format!("{} {}", self.theme.symbols.info, text.as_ref())
    }

    /// Formats muted text.
    pub fn muted(&self, text: impl AsRef<str>) -> String {
        text.as_ref().to_string()
    }

    /// Formats inverted text (same as input in the stub).
    pub fn invert(&self, text: impl AsRef<str>) -> String {
        text.as_ref().to_string()
    }

    /// Formats a header.
    pub fn header(&self, text: impl AsRef<str>) -> String {
        text.as_ref().to_string()
    }

    /// Formats a bullet list item.
    pub fn bullet(&self, text: impl AsRef<str>) -> String {
        format!("{} {}", self.theme.symbols.bullet, text.as_ref())
    }

    /// Formats an arrow-style breadcrumb entry.
    pub fn arrow(&self, text: impl AsRef<str>) -> String {
        format!("{} {}", self.theme.symbols.arrow, text.as_ref())
    }

    /// Renders a boxed block (ASCII borders). The stub just joins lines.
    pub fn boxed(&self, lines: &[impl AsRef<str>], _role: Option<Role>) -> String {
        lines.iter().map(|l| l.as_ref()).collect::<Vec<_>>().join("\n")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn formatting_stub() {
        let style = Style::new(detect_theme());
        assert!(style.ok("done").contains("done"));
        assert!(style.boxed(&["a", "b"], None).contains("b"));
    }
}
