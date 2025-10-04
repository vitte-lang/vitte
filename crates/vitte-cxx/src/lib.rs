//! vitte-cxx — interop C++ pour Vitte
//!
//! But
//! - Pont idiomatique Rust↔C++ via `cxx::bridge`.
//! - Types sûrs (slices, strings, vectors) et zero-copy quand possible.
//! - API minimale prête à l’emploi + points d’extension côté C++.
//!
//! Faits
//! - Nécessite `feature="cxx-bridge"` et `std`.
//! - Le code C++ (implémentations extern "C++") est fourni par l’application
//!   via `build.rs` (compilation avec `cc` ou CMake) et des `.cc/.hpp`.
//!
//! Exemple `build.rs` (résumé) :
//! ```no_run
//! fn main() {
//!     // Génère le pont et compile le(s) .cc/.cpp qui implémentent la partie C++
//!     cxx_build::bridge("src/lib.rs")
//!         .file("cxx/vitte_cpp.cc")
//!         .flag_if_supported("-std=c++17")
//!         .compile("vitte_cxx_bridge");
//!     println!("cargo:rerun-if-changed=src/lib.rs");
//!     println!("cargo:rerun-if-changed=cxx/vitte_cpp.cc");
//!     println!("cargo:rerun-if-changed=cxx/vitte_cpp.hpp");
//! }
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![forbid(unsafe_op_in_unsafe_fn)]
#![deny(missing_docs)]

#[cfg(not(feature = "std"))]
use alloc::string::String;
#[cfg(feature = "std")]
use std::string::String;

/// Résultat standard.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Erreurs interop.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Fonctionnalité absente.
    #[error("feature désactivée: {0}")]
    Unsupported(&'static str),
    /// Entrée invalide.
    #[error("invalide: {0}")]
    Invalid(&'static str),
}

/* ------------------------------------------------------------------------- */
/* Pont Rust↔C++                                                             */
/* ------------------------------------------------------------------------- */

/// Module FFI généré par `cxx::bridge`.
#[cfg(all(feature = "cxx-bridge", feature = "std"))]
pub mod ffi {
    use super::*;

    /// Pont CXX.
    ///
    /// - `extern "Rust"` expose des fonctions Rust que le C++ peut appeler.
    /// - `extern "C++"` déclare des symboles implémentés côté C++.
    #[cxx::bridge(namespace = "vitte")]
    mod bridge {
        // Types partagés
        unsafe extern "C++" {
            include!("vitte_cpp.hpp"); // à fournir dans votre projet

            /// Version côté C++ (implémentation fournie par l’appli).
            fn cpp_version() -> String;

            /// Somme d'un buffer côté C++ (exemple basique).
            fn cpp_sum(buf: &[u8]) -> u64;

            /// Log côté C++ (stdout/stderr, ou routeur custom).
            fn cpp_log(msg: &str);
        }

        // Fonctions Rust appelables depuis C++
        extern "Rust" {
            /// Addition 64-bit sûre, exposée au C++.
            fn rust_sum(a: u64, b: u64) -> u64;

            /// Hachage simple non-cryptographique pour démo (FNV-1a 64).
            fn rust_fnv1a64(buf: &[u8]) -> u64;

            /// Echo UTF-8 pour tests d’interop string.
            fn rust_echo(s: &str) -> String;
        }
    }

    /* ------------------- Implémentations Rust exportées ------------------- */

    /// Addition 64-bit.
    pub fn rust_sum(a: u64, b: u64) -> u64 { a.wrapping_add(b) }

    /// FNV-1a 64 (démo, non crypto).
    pub fn rust_fnv1a64(buf: &[u8]) -> u64 {
        const OFF: u64 = 0xcbf29ce484222325;
        const PRM: u64 = 0x100000001b3;
        let mut h = OFF;
        for &b in buf { h ^= b as u64; h = h.wrapping_mul(PRM); }
        h
    }

    /// Echo string → String.
    pub fn rust_echo(s: &str) -> String { s.to_owned() }

    /* -------------------- Helpers Rust appelant le C++ -------------------- */

    /// Retourne la version côté C++ si liée.
    pub fn version_cpp() -> String { self::bridge::cpp_version() }

    /// Démo: somme calculée côté C++.
    pub fn sum_cpp(buf: &[u8]) -> u64 { self::bridge::cpp_sum(buf) }

    /// Démo: log côté C++.
    pub fn log_cpp(msg: &str) { self::bridge::cpp_log(msg) }
}

/* ------------------------------------------------------------------------- */
/* API neutre quand `cxx-bridge` n’est pas activée                           */
/* ------------------------------------------------------------------------- */

#[cfg(not(all(feature = "cxx-bridge", feature = "std")))]
pub mod ffi {
    use super::*;
    /// Stub: version C++ indisponible.
    pub fn version_cpp() -> String { "<cxx disabled>".into() }
    /// Stub: somme C++ indisponible.
    pub fn sum_cpp(_buf: &[u8]) -> u64 { 0 }
    /// Stub: log C++ indisponible.
    pub fn log_cpp(_msg: &str) {}
    /// Stub: Rust callable par C++ non exposé.
    pub fn rust_sum(a: u64, b: u64) -> u64 { a.wrapping_add(b) }
    /// Stub: FNV.
    pub fn rust_fnv1a64(buf: &[u8]) -> u64 {
        const OFF: u64 = 0xcbf29ce484222325;
        const PRM: u64 = 0x100000001b3;
        let mut h = OFF;
        for &b in buf { h ^= b as u64; h = h.wrapping_mul(PRM); }
        h
    }
    /// Stub: echo.
    pub fn rust_echo(s: &str) -> String { s.into() }
}

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */

#[cfg(all(test, feature = "std"))]
mod tests {
    use super::ffi::*;

    #[test]
    fn fnv_matches_known() {
        let h = rust_fnv1a64(b"hello");
        // Valeur de référence FNV-1a 64 pour "hello"
        assert_eq!(h, 0xa430d84680aabd0b);
    }

    #[test]
    fn sum_wraps() {
        assert_eq!(rust_sum(u64::MAX, 1), 0);
    }

    #[test]
    fn echo_roundtrip() {
        assert_eq!(rust_echo("épreuve"), "épreuve");
    }
}