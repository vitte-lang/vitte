//! vitte-repl — interactive shell placeholder for Vitte.
//!
//! The original REPL depended on the full compiler stack and `rustyline`.
//! Until those pieces are ready, this crate exposes a minimal API that
//! compiles but simply reports that the REPL is unavailable.

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]

extern crate alloc;

use alloc::string::String;
use thiserror::Error;

/// Errors returned by the REPL stub.
#[derive(Debug, Error)]
pub enum ReplError {
    /// The interactive shell is not implemented yet.
    #[error("repl unavailable: {0}")]
    Unsupported(&'static str),
}

/// Result alias.
pub type Result<T> = core::result::Result<T, ReplError>;

/// Configuration options for the REPL placeholder.
#[derive(Debug, Clone, Default)]
pub struct ReplOptions {
    /// Optional startup banner (unused).
    pub banner: Option<String>,
}

/// Minimal REPL handle.
#[derive(Debug, Default)]
pub struct Repl {
    opts: ReplOptions,
}

impl Repl {
    /// Constructs a new REPL using the given options.
    pub fn new(opts: ReplOptions) -> Self {
        Self { opts }
    }

    /// Runs the interactive loop (stub).
    pub fn run(&mut self) -> Result<()> {
        let _ = &self.opts;
        Err(ReplError::Unsupported("interactive shell not implemented"))
    }

    /// Evaluates a single line.
    pub fn eval(&mut self, _line: &str) -> Result<String> {
        Err(ReplError::Unsupported("line evaluation not implemented"))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn repl_stub() {
        let mut repl = Repl::new(ReplOptions::default());
        assert!(matches!(repl.run(), Err(ReplError::Unsupported(_))));
        assert!(matches!(repl.eval("print"), Err(ReplError::Unsupported(_))));
    }
}
