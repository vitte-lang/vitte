//! vitte-install — placeholder for the Vitte toolchain installer.
//!
//! The real implementation coordinates downloads, archive extraction, and
//! manifest management. This stub keeps the public API while returning
//! `Unsupported` so the rest of the workspace can compile without networking
//! or filesystem side effects.

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]

extern crate alloc;

use alloc::string::String;
use alloc::vec::Vec;
/// Convenience result type.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Errors emitted by the installer stub.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Error {
    /// Indicates that the requested functionality is not yet implemented.
    Unsupported(&'static str),
}

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            Error::Unsupported(msg) => write!(f, "installer unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
#[allow(dead_code)]
fn main() {
    // Binary stub: keep workspace building even when installer is not implemented.
    eprintln!("installer unavailable: toolchain installation not implemented");
    std::process::exit(1);
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}

/// Minimal representation of a toolchain manifest.
#[derive(Debug, Clone, Default)]
pub struct Manifest {
    /// Human-readable name.
    pub name: String,
    /// Version string.
    pub version: String,
    /// Components included in the toolchain.
    pub components: Vec<String>,
}

/// Installation options accepted by the stub.
#[derive(Debug, Clone, Default)]
pub struct InstallOptions {
    /// Manifest to install.
    pub manifest: Manifest,
    /// Destination directory (unused in the stub).
    pub destination: Option<String>,
}

/// Installs a toolchain described by `opts`. Always returns `Unsupported`.
pub fn install(_opts: &InstallOptions) -> Result<()> {
    Err(Error::Unsupported("toolchain installation not implemented"))
}

/// Lists the toolchains currently installed (always empty).
pub fn list_installed() -> Result<Vec<Manifest>> {
    Err(Error::Unsupported("listing toolchains not implemented"))
}

/// Removes the given toolchain (unsupported).
pub fn uninstall(_name: &str) -> Result<()> {
    Err(Error::Unsupported("uninstall not implemented"))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn install_stub() {
        let opts = InstallOptions::default();
        assert!(matches!(install(&opts), Err(Error::Unsupported(_))));
    }
}
