#![deny(missing_docs)]
//! vitte-r — Interop R pour Vitte
//!
//! Fournit :
//! - Intégration `extendr` optionnelle pour écrire des fonctions exportées vers R.
//! - Stubs d'embed de l'interpréteur R (option `r-embed`).
//! - Conversions JSON optionnelles (`serde`).
//!
//! Features:
//! - `extendr` : active l'intégration via le crate `extendr-api`.
//! - `r-embed` : expose des FFI minimales de l'API C de R (nécessite `libR`).
//! - `serde`   : conversions JSON.

use thiserror::Error;

/// Erreurs interop R.
#[derive(Debug, Error)]
pub enum RError {
    /// Erreur renvoyée par l'intégration extendr (bindings R) avec un message détaillé.
    #[error("extendr error: {0}")]
    Extendr(String),
    /// Erreur bas niveau via FFI vers l'API C de R (par ex. initialisation/évaluation).
    #[error("FFI error: {0}")]
    Ffi(String),
    /// Problème de conversion (R <-> Rust ou JSON) avec description.
    #[error("conversion error: {0}")]
    Conversion(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, RError>;

// ================================================================================================
// extendr: exports vers R
// ================================================================================================
#[cfg(feature = "extendr")]
pub mod extendr_exports {
    use super::*;
    use extendr_api::prelude::*;

    /// Retourne un message de salutation.
    #[extendr]
    pub fn r_greet(name: &str) -> String {
        format!("Hello, {name} from Vitte/R")
    }

    /// Sérialise une valeur R en JSON si `serde`, sinon `as.character`.
    #[extendr]
    pub fn r_to_json(x: Robj) -> Robj {
        #[cfg(feature = "serde")]
        {
            match extendr_api::serde::from_robj::<serde_json::Value>(&x) {
                Ok(v) => {
                    extendr_api::serde::to_robj(&serde_json::to_string(&v).unwrap_or_default())
                        .unwrap_or(R_NilValue)
                }
                Err(_) => R_NilValue,
            }
        }
        #[cfg(not(feature = "serde"))]
        {
            if let Ok(s) = x.as_str() {
                s.into()
            } else {
                R_NilValue
            }
        }
    }

    /// Désérialise un JSON en objet R (list) si `serde`, sinon renvoie la chaîne d'entrée.
    #[extendr]
    pub fn r_from_json(s: &str) -> Robj {
        #[cfg(feature = "serde")]
        {
            let v: serde_json::Value = serde_json::from_str(s).unwrap_or(serde_json::Value::Null);
            extendr_api::serde::to_robj(&v).unwrap_or(R_NilValue)
        }
        #[cfg(not(feature = "serde"))]
        {
            s.into()
        }
    }

    /// Somme d'un numeric vector.
    #[extendr]
    pub fn r_sum(x: Doubles) -> f64 {
        x.iter().map(|v| v.unwrap_or(0.0)).sum()
    }

    /// Enregistre le module côté R.
    extendr_module! {
        mod vitte_r;
        fn r_greet;
        fn r_to_json;
        fn r_from_json;
        fn r_sum;
    }
}

// ================================================================================================
// Embed R (FFI minimal) — nécessite linkage avec libR, activé par `r-embed`.
// ================================================================================================
#[cfg(feature = "r-embed")]
pub mod embed {
    use super::*;
    use std::ffi::CString;
    use std::os::raw::{c_char, c_int};

    extern "C" {
        fn Rf_initEmbeddedR(argc: c_int, argv: *mut *mut c_char) -> c_int;
        fn Rf_endEmbeddedR(fatal: c_int);
        fn Rf_eval(expr: SEXP, env: SEXP) -> SEXP;
        fn Rf_protect(s: SEXP) -> SEXP;
        fn Rf_unprotect(n: c_int);
        static mut R_GlobalEnv: SEXP;
        fn Rf_mkString(s: *const c_char) -> SEXP;
        fn Rf_lang2(s: SEXP, t: SEXP) -> SEXP;
        fn Rf_install(s: *const c_char) -> SEXP;
    }

    /// Opacité d'un SEXP (type central R). Ici, on le déclare comme pointeur opaque.
    #[repr(C)]
    pub struct SEXPREC {
        _private: [u8; 0],
    }
    /// Alias de pointeur.
    pub type SEXP = *mut SEXPREC;

    /// Démarre un interpréteur R embarqué.
    pub fn start_r() -> Result<()> {
        let args = vec![CString::new("R").unwrap(), CString::new("--quiet").unwrap()];
        let mut c_args: Vec<*mut c_char> = args.into_iter().map(|c| c.into_raw()).collect();
        let rc = unsafe { Rf_initEmbeddedR(c_args.len() as c_int, c_args.as_mut_ptr()) };
        if rc != 0 {
            return Err(RError::Ffi(format!("Rf_initEmbeddedR rc={rc}")));
        }
        Ok(())
    }

    /// Arrête l'interpréteur.
    pub fn end_r() {
        unsafe { Rf_endEmbeddedR(0) }
    }

    /// Évalue une expression R simple `print(<text>)` pour test.
    pub fn eval_print(text: &str) -> Result<()> {
        let s = std::ffi::CString::new(text).map_err(|e| RError::Conversion(e.to_string()))?;
        unsafe {
            let sym_print = Rf_install(CString::new("print").unwrap().as_ptr());
            let str_sexp = Rf_mkString(s.as_ptr());
            let call = Rf_lang2(sym_print, str_sexp);
            Rf_protect(call);
            let _ = Rf_eval(call, R_GlobalEnv);
            Rf_unprotect(1);
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_fmt() {
        let e = RError::Conversion("bad".into());
        assert!(format!("{e}").contains("bad"));
    }

    #[cfg(feature = "extendr")]
    #[test]
    fn extendr_sum_works() {
        use extendr_api::prelude::*;
        let v = Doubles::from_values([1.0, 2.5, 3.5]);
        assert_eq!(extendr_exports::r_sum(v), 7.0);
    }
}
