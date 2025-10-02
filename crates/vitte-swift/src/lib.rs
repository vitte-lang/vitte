

#![deny(missing_docs)]
//! vitte-swift — interop Swift pour Vitte
//!
//! Fournit :
//! - ABI C stable: fonctions `vitte_swift_sum`, `vitte_swift_greet`.
//! - Passage tampon binaire via `VitteSwiftBuf`.
//! - Intégration optionnelle `swift-bridge` pour générer du Swift idiomatique.
//! - Compatible cbindgen pour header C automatique.
//!
//! Générer un en-tête C (si `cbindgen` en build-dep et `--features cbindgen` via build-script):
//! ```text
//! cbindgen --crate vitte-swift --output vitte_swift.h
//! ```

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_uint};

/// Erreurs interop Swift.
#[derive(Debug, thiserror::Error)]
pub enum SwiftError {
    #[error("utf8: {0}")] Utf8(#[from] std::str::Utf8Error),
    #[error("nul: {0}")] Nul(#[from] std::ffi::NulError),
    #[error("other: {0}")] Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, SwiftError>;

// ================================================================================================
// ABI C minimale (consommable côté Swift via bridge C)
// ================================================================================================

/// Tampon binaire passé par FFI (read-only). Non-owning du point de vue Rust.
#[repr(C)]
pub struct VitteSwiftBuf {
    /// Pointeur vers les octets
    pub data: *const u8,
    /// Longueur
    pub len: usize,
}

impl VitteSwiftBuf {
    /// Vue slice sûre si `data` non nul.
    pub unsafe fn as_slice<'a>(&self) -> &'a [u8] {
        if self.data.is_null() { &[] } else { std::slice::from_raw_parts(self.data, self.len) }
    }
}

/// Additionne deux entiers 32 bits (exemple ABI C).
#[no_mangle]
pub extern "C" fn vitte_swift_sum(a: c_int, b: c_int) -> c_int { a.saturating_add(b) }

/// Construit une salutation. Retourne une chaîne C allouée que l'appelant doit libérer via
/// `vitte_swift_string_free`.
#[no_mangle]
pub extern "C" fn vitte_swift_greet(name: *const c_char) -> *mut c_char {
    // Sécurité: vérifie pointeur
    if name.is_null() {
        return CString::new("Hello from Vitte/Rust").unwrap().into_raw();
    }
    let cname = unsafe { CStr::from_ptr(name) };
    let n = match cname.to_str() { Ok(s) => s, Err(_) => "unknown" };
    let s = format!("Hello, {n} from Vitte/Rust");
    CString::new(s).unwrap().into_raw()
}

/// Libère une chaîne allouée par Rust et retournée à Swift.
#[no_mangle]
pub extern "C" fn vitte_swift_string_free(s: *mut c_char) {
    if s.is_null() { return; }
    unsafe { let _ = CString::from_raw(s); }
}

/// Exemple de traitement d'un tampon d'entrée et retour de longueur (démonstration FFI).
#[no_mangle]
pub extern "C" fn vitte_swift_buf_len(buf: VitteSwiftBuf) -> usize { buf.len }

// ================================================================================================
// swift-bridge (facultatif)
// ================================================================================================
#[cfg(feature = "swift-bridge")]
pub mod bridge {
    use super::*;

    /// API idiomatique Swift <-> Rust via `swift-bridge`.
    #[swift_bridge::bridge]
    mod ffi {
        extern "Rust" {
            fn sb_sum(a: i32, b: i32) -> i32;
            fn sb_greet(name: String) -> String;
        }
    }

    /// Addition pour swift-bridge.
    pub fn sb_sum(a: i32, b: i32) -> i32 { a.saturating_add(b) }
    /// Salutation pour swift-bridge.
    pub fn sb_greet(name: String) -> String { format!("Hello, {name} from Vitte/Rust") }
}

// ================================================================================================
// Tests
// ================================================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sum_and_greet() {
        assert_eq!(unsafe { vitte_swift_sum(2, 40) }, 42);
        let c = CString::new("Swift").unwrap();
        let p = vitte_swift_greet(c.as_ptr());
        let out = unsafe { CStr::from_ptr(p) }.to_str().unwrap().to_string();
        vitte_swift_string_free(p);
        assert!(out.contains("Swift"));
    }

    #[test]
    fn buf_len() {
        let data = b"abc";
        let b = VitteSwiftBuf { data: data.as_ptr(), len: data.len() };
        assert_eq!(unsafe { vitte_swift_buf_len(b) }, 3);
    }
}