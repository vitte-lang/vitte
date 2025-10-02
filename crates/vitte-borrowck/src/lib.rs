//! vitte-borrock — experimental borrow checker stub.
//!
//! This crate used to host prototypes for the Vitte borrow checker. To keep the
//! workspace compiling while the real engine is being rewritten, we expose a
//! minimal placeholder that retains the top-level types and functions but the
//! implementations simply report that the functionality is unavailable.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::{string::String, sync::Arc, vec::Vec};

/// Result alias for the borrow-checker stub.
pub type Result<T, E = BorrockError> = core::result::Result<T, E>;

/// Errors returned by this crate.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BorrockError {
    /// Placeholder error when a feature is not implemented yet.
    Unsupported(&'static str),
}

impl core::fmt::Display for BorrockError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            BorrockError::Unsupported(msg) => write!(f, "borrow checker unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for BorrockError {}

/// Minimal diagnostic severity enumeration.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Severity {
    /// Informational notice.
    Info,
    /// Warning.
    Warning,
    /// Error.
    Error,
}

/// Span placeholder.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct Span;

/// Diagnostic message returned by the stub.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Diagnostic {
    /// Severity of the diagnostic.
    pub severity: Severity,
    /// Human-readable message.
    pub message: String,
    /// Optional notes.
    pub notes: Vec<String>,
}

impl Default for Diagnostic {
    fn default() -> Self {
        Self { severity: Severity::Info, message: String::new(), notes: Vec::new() }
    }
}

/// Trait implemented by diagnostic emitters.
pub trait Emitter: Send + Sync {
    /// Emits a diagnostic. The stub does nothing.
    fn emit(&self, _diag: &Diagnostic) {}
}

/// Minimal configuration structure used by callers.
#[derive(Debug, Clone, Default)]
pub struct BorrockConfig {
    /// Entry point to evaluate (unused in the stub).
    pub entry: Option<String>,
}

/// Loads a configuration file. Currently returns the default configuration.
#[cfg(feature = "std")]
pub fn load_config(_path: &std::path::Path) -> Result<BorrockConfig> {
    Ok(BorrockConfig::default())
}

/// IR module placeholder.
#[derive(Debug, Clone, Default)]
pub struct Module;

/// Parses source into a module (stub).
pub fn parse_str_stub(_src: &str) -> Result<Module> {
    Err(BorrockError::Unsupported("parser not implemented"))
}

/// Borrow checker driver stub.
#[derive(Debug, Default)]
pub struct Driver {
    config: BorrockConfig,
}

impl Driver {
    /// Creates a new driver with the provided configuration.
    pub fn new(config: Option<BorrockConfig>) -> Self {
        Self { config: config.unwrap_or_default() }
    }

    /// Sets a diagnostic emitter (ignored in the stub).
    pub fn emitter(self, _emitter: Arc<dyn Emitter>) -> Self {
        self
    }

    /// Runs the borrow-checking pipeline on the given module.
    pub fn run(&mut self, _module: &Module) -> Result<()> {
        let _ = &self.config;
        Err(BorrockError::Unsupported("borrow checker not implemented"))
    }
}

/// Convenience function mirroring the old pipeline entry point.
pub fn run(config: Option<BorrockConfig>, module: &Module) -> Result<()> {
    let mut driver = Driver::new(config);
    driver.run(module)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn borrowck_stub() {
        let module = Module::default();
        let err = run(None, &module).unwrap_err();
        matches!(err, BorrockError::Unsupported(_));
    }
}
