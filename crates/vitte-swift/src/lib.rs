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

#[cfg(feature = "ffi")]
use std::ffi::{CStr, CString};
#[cfg(feature = "ffi")]
use std::os::raw::{c_char, c_int};

/// Erreurs interop Swift.
#[derive(Debug, thiserror::Error)]
pub enum SwiftError {
    /// Erreur de décodage UTF-8 lors de la conversion d'une chaîne C.
    #[error("utf8: {0}")]
    Utf8(#[from] std::str::Utf8Error),
    /// NUL trouvé lors de la construction d'une `CString`.
    #[error("nul: {0}")]
    Nul(#[from] std::ffi::NulError),
    /// Autre erreur interop (message libre).
    #[error("other: {0}")]
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, SwiftError>;

// ================================================================================================
// ABI C minimale (consommable côté Swift via bridge C)
// ================================================================================================

// Les APIs FFI (no_mangle, unsafe) ne sont compilées que si la feature \
// "ffi" est activée afin de respecter les builds sans "unsafe-code".
/// Tampon binaire passé par FFI (read-only). Non-owning du point de vue Rust.
#[repr(C)]
pub struct VitteSwiftBuf {
    /// Pointeur vers les octets
    pub data: *const u8,
    /// Longueur
    pub len: usize,
}

#[cfg(not(feature = "ffi"))]
impl VitteSwiftBuf {
    /// Vue slice non disponible sans la feature `ffi`.
    pub fn as_slice<'a>(&self) -> &'a [u8] {
        &[]
    }
}

#[cfg(feature = "ffi")]
impl VitteSwiftBuf {
    /// Vue slice à partir d'un tampon brut (FFI). **Non safe**: dépend d'un pointeur valide.
    pub unsafe fn as_slice<'a>(&self) -> &'a [u8] {
        if self.data.is_null() {
            &[]
        } else {
            std::slice::from_raw_parts(self.data, self.len)
        }
    }
}

/// [FFI] Additionne deux entiers 32 bits.
#[cfg(feature = "ffi")]
#[no_mangle]
pub extern "C" fn vitte_swift_sum(a: c_int, b: c_int) -> c_int {
    a.saturating_add(b)
}

/// [FFI] Construit une salutation. Retourne une chaîne C (libérer via `vitte_swift_string_free`).
#[cfg(feature = "ffi")]
#[no_mangle]
pub extern "C" fn vitte_swift_greet(name: *const c_char) -> *mut c_char {
    // Sécurité: vérifie pointeur
    if name.is_null() {
        return CString::new("Hello from Vitte/Rust").unwrap().into_raw();
    }
    let cname = unsafe { CStr::from_ptr(name) };
    let n = match cname.to_str() {
        Ok(s) => s,
        Err(_) => "unknown",
    };
    let s = format!("Hello, {n} from Vitte/Rust");
    CString::new(s).unwrap().into_raw()
}

/// [FFI] Libère une chaîne allouée par Rust et retournée à Swift.
#[cfg(feature = "ffi")]
#[no_mangle]
pub extern "C" fn vitte_swift_string_free(s: *mut c_char) {
    if s.is_null() {
        return;
    }
    unsafe {
        let _ = CString::from_raw(s);
    }
}

/// [FFI] Retourne la longueur d'un tampon `VitteSwiftBuf`.
#[cfg(feature = "ffi")]
#[no_mangle]
pub extern "C" fn vitte_swift_buf_len(buf: VitteSwiftBuf) -> usize {
    buf.len
}

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
    pub fn sb_sum(a: i32, b: i32) -> i32 {
        a.saturating_add(b)
    }
    /// Salutation pour swift-bridge.
    pub fn sb_greet(name: String) -> String {
        format!("Hello, {name} from Vitte/Rust")
    }
}

// ================================================================================================
// Tests
// ================================================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[cfg(feature = "ffi")]
    #[test]
    fn sum_and_greet() {
        assert_eq!(unsafe { vitte_swift_sum(2, 40) }, 42);
        let c = CString::new("Swift").unwrap();
        let p = vitte_swift_greet(c.as_ptr());
        let out = unsafe { CStr::from_ptr(p) }.to_str().unwrap().to_string();
        vitte_swift_string_free(p);
        assert!(out.contains("Swift"));
    }

    #[cfg(feature = "ffi")]
    #[test]
    fn buf_len() {
        let data = b"abc";
        let b = VitteSwiftBuf { data: data.as_ptr(), len: data.len() };
        assert_eq!(unsafe { vitte_swift_buf_len(b) }, 3);
    }
}
