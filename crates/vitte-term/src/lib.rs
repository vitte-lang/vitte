//! vitte-term — terminal utilities stub.
//!
//! This placeholder keeps a minimal API for terminal metadata and formatting so
//! that other crates compile without pulling in heavy backends. All runtime
//! operations simply return [`TermError::Unsupported`].

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::string::String;

/// Result alias for terminal operations.
pub type Result<T> = core::result::Result<T, TermError>;

/// Errors produced by the terminal stub.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TermError {
    /// Feature or backend is not available.
    Unsupported(&'static str),
}

impl core::fmt::Display for TermError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            TermError::Unsupported(msg) => write!(f, "terminal backend unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for TermError {}

/// Basic terminal information (stub values).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct TermInfo {
    /// Whether stdout is considered a TTY.
    pub is_tty_stdout: bool,
    /// Whether stderr is considered a TTY.
    pub is_tty_stderr: bool,
    /// Columns.
    pub cols: u16,
    /// Rows.
    pub rows: u16,
}

impl Default for TermInfo {
    fn default() -> Self {
        Self { is_tty_stdout: true, is_tty_stderr: true, cols: 80, rows: 24 }
    }
}

impl TermInfo {
    /// Detects terminal capabilities (stubbed).
    pub fn detect() -> Self {
        Self::default()
    }
}

/// High-level terminal helper.
#[derive(Debug, Default, Clone)]
pub struct Term {
    info: TermInfo,
}

impl Term {
    /// Creates a new terminal helper using detected information.
    pub fn new() -> Self {
        Self { info: TermInfo::detect() }
    }

    /// Returns terminal information.
    pub fn info(&self) -> TermInfo {
        self.info
    }

    /// Clears the screen (stubbed).
    pub fn clear_screen(&self) -> Result<()> {
        Err(TermError::Unsupported("clear_screen not implemented"))
    }

    /// Clears the current line (stubbed).
    pub fn clear_line(&self) -> Result<()> {
        Err(TermError::Unsupported("clear_line not implemented"))
    }

    /// Shows the cursor (stubbed).
    pub fn show_cursor(&self) -> Result<()> {
        Err(TermError::Unsupported("show_cursor not implemented"))
    }

    /// Hides the cursor (stubbed).
    pub fn hide_cursor(&self) -> Result<()> {
        Err(TermError::Unsupported("hide_cursor not implemented"))
    }
}

/// Simple text styling helper (placeholder).
pub fn colorize(_role: &'static str, text: &str) -> String {
    text.to_string()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stub_term() {
        let term = Term::new();
        assert_eq!(term.info().cols, 80);
    }
}
