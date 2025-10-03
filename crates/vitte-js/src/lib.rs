//! vitte-js — JavaScript bindings stub for the Vitte runtime.
//!
//! The actual crate exposes FFI bindings to the Vitte VM for JavaScript/wasm.
//! Until that code is ported, this module exposes minimal placeholder
//! structures. Calling any of the functions will return
//! [`JsError::Unsupported`].

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::string::String;

/// Result alias for the JS bridge.
pub type JsResult<T> = Result<T, JsError>;

/// Errors returned by the JS bridge stub.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum JsError {
    /// Functionality not available yet.
    Unsupported(&'static str),
}

impl core::fmt::Display for JsError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            JsError::Unsupported(msg) => write!(f, "js bridge unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for JsError {}

/// Placeholder handle returned by `compile`.
#[derive(Debug, Default)]
pub struct JsModule;

/// Compiles source code into a module (stub).
pub fn compile(_src: &str) -> JsResult<JsModule> {
    Err(JsError::Unsupported("JS compiler not implemented"))
}

/// Runs the given module (stub).
pub fn run_module(_module: &JsModule) -> JsResult<String> {
    Err(JsError::Unsupported("JS runtime not implemented"))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stub_js() {
        assert!(matches!(compile("print"), Err(JsError::Unsupported(_))));
    }
}
