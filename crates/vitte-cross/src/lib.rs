//! vitte-cross — primitives de cross-compilation pour Vitte
// The detailed implementation that searched for compilers/linkers per target
// is not available yet. This placeholder keeps the public surface (types such
// as [`Toolchain`] and [`Runner`]) while simply returning `Unsupported`.

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]

extern crate alloc;

use alloc::{string::String, vec::Vec};

/// Result alias used across the crate.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Errors returned by the stub.
#[derive(Debug)]
pub enum Error {
    /// The requested operation is currently not implemented.
    Unsupported(&'static str),
}

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            Error::Unsupported(msg) => write!(f, "cross-compilation support unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}

/// Minimal target triple wrapper.
#[derive(Debug, Clone, Default)]
pub struct TargetTriple {
    /// The canonical triple string (e.g., "x86_64-unknown-linux-gnu").
    pub triple: String,
}

impl TargetTriple {
    /// Constructs a triple from any string-like type.
    pub fn new<S: Into<String>>(s: S) -> Self {
        Self { triple: s.into() }
    }
}

/// Toolchain paths (all optional in the stub).
#[derive(Debug, Clone, Default)]
pub struct Toolchain {
    /// C compiler.
    pub cc: Option<String>,
    /// C++ compiler.
    pub cxx: Option<String>,
    /// Archiver.
    pub ar: Option<String>,
    /// Ranlib.
    pub ranlib: Option<String>,
    /// Linker.
    pub ld: Option<String>,
}

/// Outcome of a cross-compilation probe.
#[derive(Debug, Clone, Default)]
pub struct ProbeResult {
    /// Discovered toolchain.
    pub toolchain: Toolchain,
    /// Additional notes or diagnostics.
    pub notes: Vec<String>,
}

/// Attempts to detect a toolchain for the given target (stub).
pub fn probe_toolchain(_target: &TargetTriple) -> Result<ProbeResult> {
    Err(Error::Unsupported("toolchain probing is disabled"))
}

/// Runner wrapper for executing build commands against a toolchain.
#[derive(Debug, Clone, Default)]
pub struct Runner {
    target: TargetTriple,
    toolchain: Toolchain,
}

impl Runner {
    /// Builds a runner from a target/toolchain pair.
    pub fn new(target: TargetTriple, toolchain: Toolchain) -> Self {
        Self { target, toolchain }
    }

    /// Executes a command (stubbed).
    pub fn exec(&self, _cmd: &str, _args: &[String]) -> Result<()> {
        let _ = (&self.target, &self.toolchain);
        Err(Error::Unsupported("runner execution not implemented"))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stub_probe() {
        let triple = TargetTriple::new("x86_64-unknown-linux-gnu");
        assert!(matches!(probe_toolchain(&triple), Err(Error::Unsupported(_))));
    }
}
