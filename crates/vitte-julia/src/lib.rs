#![deny(missing_docs)]
//! vitte-julia — Interop Julia pour Vitte.
//!
//! Fournit des abstractions pour interagir avec Julia via son runtime C (`libjulia`):
//! - Initialiser/arrêter la runtime Julia.
//! - Appeler des fonctions Julia depuis Rust.
//! - Convertir types Rust <-> Julia (stubs extensibles).
//! - Gérer les exceptions Julia comme `JuliaError`.
//!
//! # Features
//! - `runtime`: active les FFI vers libjulia.
//! - `serde`: helpers JSON pour conversions.
//!
//! # Exemple
//! ```no_run
//! use vitte_julia::runtime;
//! runtime::init();
//! let v = runtime::version();
//! println!("Julia version: {v}");
//! runtime::shutdown(0);
//! ```

use thiserror::Error;

/// Erreurs liées à l'Interop Julia et au runtime.
#[derive(Debug, Error)]
pub enum JuliaError {
    /// La runtime Julia n'est pas disponible ou non initialisée.
    #[error("Julia runtime not available")]
    RuntimeUnavailable,
    /// Échec d'appel JNI/FFI ou interaction runtime (message brut).
    #[error("Interop error: {0}")]
    Interop(String),
    /// Échec de conversion de types Rust ⇄ Julia.
    #[error("Conversion error: {0}")]
    Conversion(String),
    /// Erreur lors de l'évaluation d'une expression Julia.
    #[error("Evaluation error: {0}")]
    Eval(String),
    /// Un pointeur nul a été renvoyé par l'API Julia.
    #[error("Null pointer received from Julia")]
    NullPointer,
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, JuliaError>;

/// Informations sur la runtime Julia.
#[derive(Debug, Clone)]
pub struct JuliaInfo {
    /// Version de Julia.
    pub version: String,
    /// Dossier d'installation.
    pub home: String,
}

/// Conversions Rust <-> Julia (stubs extensibles).
pub mod convert {
    use super::*;

    /// Convertit un bool Rust en représentation Julia simple (stub).
    pub fn to_julia_bool(v: bool) -> i32 {
        if v {
            1
        } else {
            0
        }
    }

    /// Convertit un entier Rust en entier Julia (stub).
    pub fn to_julia_int(v: i64) -> i64 {
        v
    }

    /// Convertit un pointeur Julia en bool Rust (stub).
    pub fn from_julia_bool(ptr: *mut std::os::raw::c_void) -> Result<bool> {
        if ptr.is_null() {
            return Err(JuliaError::NullPointer);
        }
        // stub: treat non-null pointer as true
        Ok(true)
    }
}

#[cfg(feature = "runtime")]
pub mod runtime {
    use super::*;
    use std::ffi::{CStr, CString};
    use std::os::raw::c_char;

    #[link(name = "julia")]
    extern "C" {
        fn jl_init();
        fn jl_atexit_hook(status: i32);
        fn jl_eval_string(s: *const c_char) -> *mut std::os::raw::c_void;
        fn jl_ver_string() -> *const c_char;
    }

    static mut INITIALIZED: bool = false;

    /// Initialise la runtime Julia (idempotent).
    pub fn init() {
        unsafe {
            if !INITIALIZED {
                jl_init();
                INITIALIZED = true;
            }
        }
    }

    /// Arrête la runtime Julia.
    pub fn shutdown(status: i32) {
        unsafe {
            if INITIALIZED {
                jl_atexit_hook(status);
                INITIALIZED = false;
            }
        }
    }

    /// Retourne la version de Julia.
    pub fn version() -> String {
        unsafe {
            let ptr = jl_ver_string();
            if ptr.is_null() {
                return "unknown".into();
            }
            let cstr = CStr::from_ptr(ptr);
            cstr.to_string_lossy().into_owned()
        }
    }

    /// Évalue une expression Julia et retourne un pointeur brut.
    pub fn eval_raw(expr: &str) -> Result<*mut std::os::raw::c_void> {
        let cexpr = CString::new(expr).map_err(|e| JuliaError::Interop(e.to_string()))?;
        let res = unsafe { jl_eval_string(cexpr.as_ptr()) };
        if res.is_null() {
            Err(JuliaError::Eval(expr.into()))
        } else {
            Ok(res)
        }
    }

    /// Évalue une expression Julia et essaie de convertir en String (si possible).
    pub fn eval_string(expr: &str) -> Result<String> {
        let _ptr = eval_raw(expr)?;
        // Stub: in a full impl, we'd use Julia API to convert object to string.
        Ok(format!("Evaluated: {expr}"))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_fmt() {
        let e = JuliaError::Conversion("bad".into());
        assert!(format!("{e}").contains("bad"));
    }

    #[test]
    fn convert_bool() {
        assert_eq!(convert::to_julia_bool(true), 1);
        assert_eq!(convert::to_julia_bool(false), 0);
    }

    #[cfg(feature = "runtime")]
    #[test]
    fn version_non_empty() {
        let v = crate::runtime::version();
        assert!(!v.is_empty());
    }
}
