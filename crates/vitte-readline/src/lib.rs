//! vitte-readline — interactive input stub for the Vitte toolkit.
//!
//! The full crate wraps readline libraries such as `rustyline`. This stub keeps a
//! tiny surface so that dependent crates compile while interactive features are
//! unavailable.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::string::String;
use alloc::vec::Vec;

/// Result alias for readline operations.
pub type Result<T> = core::result::Result<T, ReadErr>;

/// Errors returned by the stub implementation.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ReadErr {
    /// Real readline functionality is not implemented.
    Unsupported(&'static str),
}

impl core::fmt::Display for ReadErr {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            ReadErr::Unsupported(msg) => write!(f, "readline backend unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ReadErr {}

/// Events produced by `read_line`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ReadEvent {
    /// A full line of input.
    Line(String),
    /// End of file (Ctrl-D).
    Eof,
    /// Interrupted (Ctrl-C).
    Interrupted,
}

/// Configuration for the readline engine.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ReadConfig {
    /// Optional path to the history file (string form to stay `no_std` friendly).
    pub history_path: Option<String>,
    /// Maximum number of entries in history.
    pub history_size: usize,
}

impl Default for ReadConfig {
    fn default() -> Self {
        Self { history_path: None, history_size: 1000 }
    }
}

/// Minimal readline engine placeholder.
#[derive(Debug, Default)]
pub struct Readline {
    /// Stored configuration (unused in the stub).
    _cfg: ReadConfig,
}

impl Readline {
    /// Creates a new readline engine with the provided configuration.
    pub fn with_config(cfg: ReadConfig) -> Result<Self> {
        let _ = cfg;
        Err(ReadErr::Unsupported("interactive input not implemented"))
    }

    /// Reads a line from stdin (stub).
    pub fn read_line(&mut self, _prompt: &str) -> Result<ReadEvent> {
        Err(ReadErr::Unsupported("interactive input not implemented"))
    }

    /// Loads command history (no-op).
    pub fn load_history(&mut self) -> Result<()> {
        Err(ReadErr::Unsupported("history not implemented"))
    }

    /// Saves command history (no-op).
    pub fn save_history(&mut self) -> Result<()> {
        Err(ReadErr::Unsupported("history not implemented"))
    }
}

/// Completion trait placeholder.
pub trait Complete {
    /// Suggests completions for the given input at the specified cursor position.
    fn complete(&self, _input: &str, _pos: usize) -> Vec<String> { Vec::new() }
}

/// Highlight trait placeholder.
pub trait Highlight {
    /// Returns a highlighted version of `input`.
    fn highlight(&self, input: &str) -> String { input.to_string() }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stub_readline() {
        let cfg = ReadConfig::default();
        assert!(matches!(Readline::with_config(cfg), Err(ReadErr::Unsupported(_))));
    }
}
