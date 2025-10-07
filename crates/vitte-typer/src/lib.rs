//! vitte-typer — type inference stub for the Vitte compiler.
//!
//! The original crate performed Hindley–Milner style inference on Vitte’s HIR.
//! Until that logic is reintroduced, this module offers a minimal API so other
//! components compile.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::string::String;

/// Result alias for the type checker.
pub type Result<T> = core::result::Result<T, TyperError>;

/// Errors emitted by the typer stub.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TyperError {
    /// Feature not implemented yet.
    Unsupported(&'static str),
}

impl core::fmt::Display for TyperError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            TyperError::Unsupported(msg) => write!(f, "typer unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for TyperError {}

/// Placeholder type representation.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Type {
    /// Human-readable description.
    pub name: String,
}

/// Typing context placeholder.
#[derive(Debug, Default)]
pub struct InferCtx;

impl InferCtx {
    /// Creates a new inference context.
    pub fn new() -> Self { Self }
}

/// Runs type inference on a module (stub).
pub fn infer_module(_ctx: &mut InferCtx) -> Result<()> {
    Err(TyperError::Unsupported("type inference not implemented"))
}

/// Infers the type of an expression (stub).
pub fn infer_expr(_ctx: &mut InferCtx) -> Result<Type> {
    Err(TyperError::Unsupported("type inference not implemented"))
}

/// Utility returning a unit type placeholder.
pub fn unit_type() -> Type {
    Type { name: "()".into() }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn typer_stub() {
        let mut ctx = InferCtx::new();
        assert!(matches!(infer_module(&mut ctx), Err(TyperError::Unsupported(_))));
    }
}
