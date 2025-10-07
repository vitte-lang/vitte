#![deny(missing_docs)]
//! vitte-matlab — Interop Matlab pour Vitte
//!
//! Objectifs :
//! - Fournir des abstractions sûres pour piloter Matlab (Engine API) et écrire des MEX.
//! - Conversions Rust <-> Matlab (stubs extensibles).
//! - API compilable sans Matlab présent (features désactivées).
//!
//! Features:
//! - `engine` : wrappers du Matlab Engine (lancement, évaluation de code, transfert simple).
//! - `mex`    : aides pour écrire un module MEX (points d'entrée, conversions de base).
//! - `serde`  : conversions JSON optionnelles.

use thiserror::Error;

/// Erreurs d'interop Matlab.
#[derive(Debug, Error)]
pub enum MatlabError {
    /// Le moteur Matlab (Engine) n'est pas disponible sur le système.
    #[error("engine unavailable")]
    EngineUnavailable,
    /// Erreur retournée par l'API Matlab Engine (message brut inclus).
    #[error("engine error: {0}")]
    Engine(String),
    /// Erreur liée à l'exécution d'un module MEX (message brut inclus).
    #[error("mex error: {0}")]
    Mex(String),
    /// Erreur de conversion entre types Rust et représentations Matlab.
    #[error("conversion error: {0}")]
    Conversion(String),
    /// Déréférencement de pointeur nul détecté côté FFI Matlab.
    #[error("null pointer")]
    NullPtr,
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, MatlabError>;

/// Informations runtime simplifiées.
#[derive(Debug, Clone)]
pub struct MatlabInfo {
    /// Version détectée ou fournie.
    pub version: String,
}

// ================================================================================================
// Engine API (feature `engine`)
// ================================================================================================
#[cfg(feature = "engine")]
pub mod engine {
    use super::*;
    use std::ffi::{CStr, CString};
    use std::os::raw::{c_char, c_int, c_void};

    // Liaison FFI minimale. En pratique il faut lier contre libeng / libmx fournis par Matlab.
    // Ici, seules les signatures suffisent pour un scaffolding compilable si link assuré côté build.rs.
    #[allow(non_camel_case_types)]
    type Engine = c_void;

    extern "C" {
        fn engOpen(startcmd: *const c_char) -> *mut Engine;
        fn engClose(ep: *mut Engine) -> c_int;
        fn engEvalString(ep: *mut Engine, cmd: *const c_char) -> c_int;
    }

    /// Contexte de moteur Matlab.
    pub struct EngineCtx {
        ep: *mut Engine,
    }

    unsafe impl Send for EngineCtx {}
    unsafe impl Sync for EngineCtx {}

    impl EngineCtx {
        /// Ouvre un engine Matlab (Matlab doit être installé).
        pub fn open(cmd: Option<&str>) -> Result<Self> {
            let c = match cmd {
                Some(s) => CString::new(s).map_err(|e| MatlabError::Engine(e.to_string()))?,
                None => CString::new("").unwrap(),
            };
            let ep = unsafe { engOpen(c.as_ptr()) };
            if ep.is_null() {
                return Err(MatlabError::EngineUnavailable);
            }
            Ok(Self { ep })
        }

        /// Évalue une commande Matlab, retourne `Ok(())` si succès.
        pub fn eval(&self, code: &str) -> Result<()> {
            let c = CString::new(code).map_err(|e| MatlabError::Engine(e.to_string()))?;
            let rc = unsafe { engEvalString(self.ep, c.as_ptr()) };
            if rc != 0 {
                return Err(MatlabError::Engine(format!("eval rc={rc}")));
            }
            Ok(())
        }

        /// Ferme le moteur.
        pub fn close(mut self) -> Result<()> {
            let rc = unsafe { engClose(self.ep) };
            self.ep = std::ptr::null_mut();
            if rc != 0 {
                return Err(MatlabError::Engine(format!("close rc={rc}")));
            }
            Ok(())
        }

        /// Accès brut au pointeur Engine.
        pub fn as_ptr(&self) -> *mut Engine {
            self.ep
        }
    }

    impl Drop for EngineCtx {
        fn drop(&mut self) {
            if !self.ep.is_null() {
                // best-effort
                unsafe { engClose(self.ep) };
                self.ep = std::ptr::null_mut();
            }
        }
    }
}

// ================================================================================================
// MEX helpers (feature `mex`)
// ================================================================================================
#[cfg(feature = "mex")]
pub mod mex {
    //! Aides pour écrire des MEX en Rust.
    use super::*;
    use std::ffi::{CStr, CString};
    use std::os::raw::{c_char, c_int, c_void};

    // Types minimaux (en vrai, mxArray et API sont bien plus vastes).
    #[repr(C)]
    pub struct mxArray {
        _private: [u8; 0],
    }

    extern "C" {
        fn mexErrMsgIdAndTxt(id: *const c_char, msg: *const c_char);
    }

    /// Déclenche une erreur MEX avec identifiant.
    pub fn mex_error(id: &str, msg: &str) -> ! {
        unsafe {
            let id_c = CString::new(id).unwrap();
            let msg_c = CString::new(msg).unwrap();
            mexErrMsgIdAndTxt(id_c.as_ptr(), msg_c.as_ptr());
            std::hint::unreachable_unchecked()
        }
    }

    /// Exemple de conversion rust -> message MEX.
    pub fn require_nargs(n: usize, required: usize) -> Result<()> {
        if n != required {
            return Err(MatlabError::Mex(format!("expected {required} args, got {n}")));
        }
        Ok(())
    }

    /// Exemple de point d'entrée MEX.
    ///
    /// Pour l'activer, exposez une fonction `mexFunction` de la forme suivante dans votre crate:
    /// ```ignore
    /// #[no_mangle]
    /// pub extern "C" fn mexFunction(nlhs: i32, plhs: *mut *mut mxArray, nrhs: i32, prhs: *const *const mxArray) {
    ///     // ...
    /// }
    /// ```
    #[allow(dead_code)]
    pub extern "C" fn mex_stub() {}
}

// ================================================================================================
// Conversions génériques (stubs extensibles)
// ================================================================================================
/// Conversions utilitaires entre types Rust et représentations Matlab.
pub mod convert {
    use super::*;

    /// Encode un bool rust en entier Matlab (0/1).
    pub fn bool_to_mat(b: bool) -> i32 {
        if b { 1 } else { 0 }
    }

    /// Conversion en chaîne C compatible API Matlab.
    pub fn to_cstring(s: &str) -> Result<std::ffi::CString> {
        std::ffi::CString::new(s).map_err(|e| MatlabError::Conversion(e.to_string()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_fmt() {
        let e = MatlabError::Conversion("bad".into());
        assert!(format!("{e}").contains("bad"));
    }

    #[cfg(feature = "engine")]
    #[test]
    fn engine_open_close_stub() {
        // Ce test valide seulement la présence des symboles et compile-time.
        // L'exécution requiert une installation Matlab avec libeng disponible.
        assert!(true);
    }
}
