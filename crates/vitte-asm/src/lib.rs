//! vitte-asm — assembly generator stub for the Vitte toolchain.
//!
//! The original crate emitted architecture-specific assembly. For now we keep a
//! minimal API surface so that dependent crates can compile without pulling in
//! the full backend.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::string::String;
use alloc::vec::Vec;

/// Result alias for the asm stub.
pub type Result<T, E = AsmError> = core::result::Result<T, E>;

/// Errors emitted by the stub implementation.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AsmError {
    /// Assembly generation is not available yet.
    Unsupported(&'static str),
}

impl core::fmt::Display for AsmError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            AsmError::Unsupported(msg) => write!(f, "assembly backend unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for AsmError {}

/// Architecture identifier placeholder.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum TargetArch {
    /// x86_64 backend.
    X86_64,
    /// AArch64 backend.
    Aarch64,
}

impl Default for TargetArch {
    fn default() -> Self { TargetArch::X86_64 }
}

/// Assembly module placeholder.
#[derive(Debug, Clone, Default)]
pub struct AsmModule {
    /// Target architecture.
    pub arch: TargetArch,
    /// Lines of assembly (unused in the stub).
    pub lines: Vec<String>,
}

impl AsmModule {
    /// Emits the module as a string. Always returns an error in the stub.
    pub fn emit(&self) -> Result<String> {
        let _ = self;
        Err(AsmError::Unsupported("assembly emission not implemented"))
    }
}

/// High-level builder API.
pub struct AsmBuilder {
    arch: TargetArch,
}

impl AsmBuilder {
    /// Creates a new builder for the given architecture.
    pub fn new(arch: TargetArch) -> Self {
        Self { arch }
    }

    /// Adds a function to the module (ignored in the stub).
    pub fn add_function(&mut self, _name: &str, _body: &str) {}

    /// Finalises the module and returns the placeholder [`AsmModule`].
    pub fn finish(self) -> AsmModule {
        AsmModule { arch: self.arch, lines: Vec::new() }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn asm_stub() {
        let mut builder = AsmBuilder::new(TargetArch::X86_64);
        builder.add_function("main", "ret");
        let module = builder.finish();
        assert!(matches!(module.emit(), Err(AsmError::Unsupported(_))));
    }
}
