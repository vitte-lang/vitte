#![deny(missing_docs)]
//! vitte-python — Interop Python pour Vitte (PyO3)
//!
//! Fournit :
//! - Exports de module Python (si feature `pyo3` et/ou `ext-module`).
//! - Aides d'embedding côté Rust (exécuter du code, appeler des fonctions).
//! - Conversions JSON optionnelles (`serde`).
//!
//! Build extension CPython importable :
//! `cargo build -p vitte-python --features "ext-module,abi3" --release`
//!
//! Embedding simple côté Rust (avec `pyo3`):
//! ```no_run
//! use vitte_python as vp;
//! vp::py_run("print('hello from embedded Python')").unwrap();
//! ```

/// Erreurs interop Python.
#[derive(Debug)]
pub enum PyErrKind {
    /// Erreur générée par PyO3
    Py(String),
    /// Erreur de conversion
    Conversion(String),
}

impl core::fmt::Display for PyErrKind {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            PyErrKind::Py(s) => write!(f, "PyO3 error: {s}"),
            PyErrKind::Conversion(s) => write!(f, "conversion error: {s}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for PyErrKind {}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, PyErrKind>;

// ============================================================================
// Embedding helpers (Rust -> Python), disponibles avec `pyo3`.
// ============================================================================
/// Helpers to run Python from Rust when `pyo3` and `pyo3_compat` are enabled.
#[cfg(all(feature = "pyo3", feature = "pyo3_compat"))]
pub mod embed {
    use super::*;
    use pyo3::prelude::*;
    use pyo3::types::{PyDict, PyList, PyString, PyTuple};

    /// Exécute une chaîne de code Python (sans résultat).
    pub fn py_run(code: &str) -> Result<()> {
        Python::with_gil(|py| py.run(code, None, None).map_err(|e| PyErrKind::Py(e.to_string())))
    }

    /// Évalue une expression Python et renvoie sa représentation `str`.
    pub fn py_eval_str(expr: &str) -> Result<String> {
        Python::with_gil(|py| {
            let v = py.eval(expr, None, None).map_err(|e| PyErrKind::Py(e.to_string()))?;
            Ok(v.str()
                .map_err(|e| PyErrKind::Py(e.to_string()))?
                .to_str()
                .map_err(|e| PyErrKind::Py(e.to_string()))?
                .to_owned())
        })
    }

    /// Appelle une fonction d'un module avec arguments texte, renvoie str.
    pub fn call_fn_str(module: &str, func: &str, args: &[&str]) -> Result<String> {
        Python::with_gil(|py| {
            let m = py.import(module).map_err(|e| PyErrKind::Py(e.to_string()))?;
            let tuple = PyTuple::new(py, args.iter().map(|s| PyString::new(py, s)));
            let out = m
                .getattr(func)
                .and_then(|f| f.call1(tuple))
                .map_err(|e| PyErrKind::Py(e.to_string()))?;
            Ok(out
                .str()
                .map_err(|e| PyErrKind::Py(e.to_string()))?
                .to_str()
                .map_err(|e| PyErrKind::Py(e.to_string()))?
                .to_owned())
        })
    }

    /// Convertit un slice de i64 en liste Python.
    pub fn to_py_list_i64(data: &[i64]) -> Result<String> {
        Python::with_gil(|py| {
            let list = PyList::new(py, data);
            Ok(list
                .repr()
                .map_err(|e| PyErrKind::Py(e.to_string()))?
                .to_str()
                .map_err(|e| PyErrKind::Py(e.to_string()))?
                .to_owned())
        })
    }
}

// Fallback stubs when pyo3_compat is not enabled
/// Stub helpers when `pyo3`/`pyo3_compat` are not enabled.
#[cfg(not(all(feature = "pyo3", feature = "pyo3_compat")))]
pub mod embed {
    use super::*;
    /// Exécute une chaîne de code Python (stub).
    pub fn py_run(_code: &str) -> Result<()> {
        Err(PyErrKind::Py("pyo3 compatibility not enabled".into()))
    }
    /// Évalue une expression Python et renvoie sa représentation `str` (stub).
    pub fn py_eval_str(_expr: &str) -> Result<String> {
        Err(PyErrKind::Py("pyo3 compatibility not enabled".into()))
    }
    /// Appelle une fonction d'un module avec arguments texte, renvoie str (stub).
    pub fn call_fn_str(_module: &str, _func: &str, _args: &[&str]) -> Result<String> {
        Err(PyErrKind::Py("pyo3 compatibility not enabled".into()))
    }
    /// Convertit un slice de i64 en liste Python (stub).
    pub fn to_py_list_i64(_data: &[i64]) -> Result<String> {
        Err(PyErrKind::Py("pyo3 compatibility not enabled".into()))
    }
}

// ============================================================================
// Module d'extension CPython (exports) si `pyo3`.
// ============================================================================
/// CPython extension surface (exports) available when `pyo3` and `pyo3_compat` are enabled.
#[cfg(all(feature = "pyo3", feature = "pyo3_compat"))]
pub mod ext {
    /// Stub extension module. Enable the `pyo3_compat` feature and add real PyO3 macros
    /// in environments where PyO3 macros and API versions are available.
    pub fn enabled() -> bool {
        false
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_fmt() {
        let e = PyErrKind::Conversion("bad".into());
        assert!(format!("{e}").contains("bad"));
    }

    #[cfg(all(feature = "pyo3", feature = "pyo3_compat"))]
    #[test]
    fn eval_and_run() {
        assert!(embed::py_run("x=1+1").is_ok());
        let s = embed::py_eval_str("1+2").unwrap();
        assert!(s.contains("3"));
    }
}
