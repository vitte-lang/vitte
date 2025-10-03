//! vitte-sandbox — minimal placeholder for WASM sandboxing.
//!
//! The original crate wrapped Wasmtime to execute WebAssembly modules with
//! resource limits. For now we provide a lightweight shim so the workspace
//! builds without pulling Wasmtime or requiring async runtimes. The API keeps a
//! similar surface (`Sandbox`, `InstanceHandle`) but every operation simply
//! reports that sandboxing is unavailable.

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]

use thiserror::Error;

/// Errors emitted by the sandbox stub.
#[derive(Debug, Error)]
pub enum SandboxError {
    /// Feature is not enabled or implementation missing.
    #[error("sandboxing unavailable: {0}")]
    Unsupported(&'static str),
}

/// Convenience result alias.
pub type Result<T> = core::result::Result<T, SandboxError>;

/// Placeholder sandbox type.
#[derive(Debug, Default)]
pub struct Sandbox;

impl Sandbox {
    /// Creates a new sandbox instance (always returns [`SandboxError::Unsupported`]).
    pub fn new() -> Result<Self> {
        Err(SandboxError::Unsupported("wasmtime feature disabled"))
    }

    /// Attempts to instantiate a WebAssembly module (always unsupported).
    pub async fn instantiate(&self, _bytes: &[u8]) -> Result<InstanceHandle> {
        Err(SandboxError::Unsupported("wasmtime feature disabled"))
    }
}

/// Placeholder handle for an instantiated module.
#[derive(Debug, Default)]
pub struct InstanceHandle;

impl InstanceHandle {
    /// Returns `None` because function lookup is not supported in the stub.
    pub fn get_func(&self, _name: &str) -> Option<()> {
        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sandbox_stub() {
        let err = Sandbox::new().unwrap_err();
        matches!(err, SandboxError::Unsupported(_));
    }
}
