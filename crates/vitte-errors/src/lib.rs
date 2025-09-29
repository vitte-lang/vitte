//! vitte-errors — gestion des erreurs communes à Vitte
//!
//! Ce crate centralise :
//! - Types d’erreurs du compilateur, de la VM et des outils
//! - Conversion uniforme vers [`anyhow::Error`]
//! - Sérialisation optionnelle via `serde`
//!
//! API publique :
//! - [`ErrorKind`] : classification stable
//! - [`Error`]     : type principal
//! - [`Result`]    : alias de retour
//!
//! Exemple :
//! ```ignore
//! use vitte_errors::{Error, ErrorKind, Result};
//!
//! fn parse_file() -> Result<()> {
//!     Err(Error::new(ErrorKind::Parse, "unexpected token"))
//! }
//! ```

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use thiserror::Error;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Classification stable des erreurs.
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum ErrorKind {
    Parse,
    Typecheck,
    Codegen,
    Vm,
    Io,
    Config,
    Internal,
}

/// Type d’erreur principal.
#[derive(Debug, Error)]
pub struct Error {
    pub kind: ErrorKind,
    pub message: String,
    #[source]
    pub source: Option<anyhow::Error>,
}

impl Error {
    pub fn new(kind: ErrorKind, msg: impl Into<String>) -> Self {
        Self { kind, message: msg.into(), source: None }
    }

    pub fn with_source<E: Into<anyhow::Error>>(kind: ErrorKind, msg: impl Into<String>, src: E) -> Self {
        Self { kind, message: msg.into(), source: Some(src.into()) }
    }
}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}: {}", self.kind, self.message)
    }
}

impl std::error::Error for Error {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        self.source.as_ref().map(|e| e.as_ref() as _)
    }
}

/// Alias standard.
pub type Result<T, E = Error> = std::result::Result<T, E>;

/// Conversion depuis `anyhow::Error`.
impl From<anyhow::Error> for Error {
    fn from(e: anyhow::Error) -> Self {
        Self { kind: ErrorKind::Internal, message: e.to_string(), source: Some(e) }
    }
}

/// Conversion vers `anyhow::Error`.
impl From<Error> for anyhow::Error {
    fn from(e: Error) -> Self {
        if let Some(src) = e.source {
            src.context(e.message)
        } else {
            anyhow::anyhow!(e.message)
        }
    }
}

// --- Tests ---------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn make_and_display() {
        let e = Error::new(ErrorKind::Parse, "unexpected token");
        assert_eq!(e.kind, ErrorKind::Parse);
        assert!(format!("{}", e).contains("unexpected token"));
    }

    #[test]
    fn with_source_ok() {
        let src = std::io::Error::new(std::io::ErrorKind::Other, "iofail");
        let e = Error::with_source(ErrorKind::Io, "failed", src);
        assert_eq!(e.kind, ErrorKind::Io);
        assert!(e.to_string().contains("failed"));
    }
}