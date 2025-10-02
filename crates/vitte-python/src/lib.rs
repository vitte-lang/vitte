

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

use thiserror::Error;

/// Erreurs interop Python.
#[derive(Debug, Error)]
pub enum PyErrKind {
    #[error("PyO3 error: {0}")] Py(String),
    #[error("conversion error: {0}")] Conversion(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, PyErrKind>;

// ============================================================================
// Embedding helpers (Rust -> Python), disponibles avec `pyo3`.
// ============================================================================
#[cfg(feature = "pyo3")]
pub mod embed {
    use super::*;
    use pyo3::prelude::*;
    use pyo3::{types::{PyDict, PyList, PyString, PyTuple}};

    /// Exécute une chaîne de code Python (sans résultat).
    pub fn py_run(code: &str) -> Result<()> {
        Python::with_gil(|py| {
            py.run(code, None, None).map_err(|e| PyErrKind::Py(e.to_string()))
        })
    }

    /// Évalue une expression Python et renvoie sa représentation `str`.
    pub fn py_eval_str(expr: &str) -> Result<String> {
        Python::with_gil(|py| {
            let v = py.eval(expr, None, None).map_err(|e| PyErrKind::Py(e.to_string()))?;
            Ok(v.str().map_err(|e| PyErrKind::Py(e.to_string()))?.to_str().map_err(|e| PyErrKind::Py(e.to_string()))?.to_owned())
        })
    }

    /// Appelle une fonction d'un module avec arguments texte, renvoie str.
    pub fn call_fn_str(module: &str, func: &str, args: &[&str]) -> Result<String> {
        Python::with_gil(|py| {
            let m = py.import(module).map_err(|e| PyErrKind::Py(e.to_string()))?;
            let tuple = PyTuple::new(py, args.iter().map(|s| PyString::new(py, s)));
            let out = m.getattr(func).and_then(|f| f.call1(tuple)).map_err(|e| PyErrKind::Py(e.to_string()))?;
            Ok(out.str().map_err(|e| PyErrKind::Py(e.to_string()))?.to_str().map_err(|e| PyErrKind::Py(e.to_string()))?.to_owned())
        })
    }

    /// Convertit un slice de i64 en liste Python.
    pub fn to_py_list_i64(data: &[i64]) -> Result<String> {
        Python::with_gil(|py| {
            let list = PyList::new(py, data);
            Ok(list.repr().map_err(|e| PyErrKind::Py(e.to_string()))?.to_str().map_err(|e| PyErrKind::Py(e.to_string()))?.to_owned())
        })
    }
}

// ============================================================================
// Module d'extension CPython (exports) si `pyo3`.
// ============================================================================
#[cfg(feature = "pyo3")]
pub mod ext {
    use super::*;
    use pyo3::prelude::*;
    use pyo3::types::{PyAny, PyList, PyTuple, Py};

    /// `greet(name) -> str`
    #[pyfunction]
    pub fn greet(name: String) -> String { format!("Hello, {name} from Vitte/Python") }

    /// `sum_list(seq) -> int` (somme d'entiers)
    #[pyfunction]
    pub fn sum_list(seq: &PyAny) -> PyResult<i64> {
        let iter = seq.iter()?;
        let mut acc: i64 = 0;
        for item in iter {
            acc += item?.extract::<i64>()?;
        }
        Ok(acc)
    }

    /// `to_json(obj) -> str` si `serde`, sinon `str(obj)`.
    #[pyfunction]
    pub fn to_json(py: Python<'_>, obj: Py<PyAny>) -> PyResult<String> {
        #[cfg(feature = "serde")]
        {
            let v = obj.as_ref(py);
            let any = v.extract::<serde_json::Value>()?;
            return Ok(serde_json::to_string(&any).unwrap_or_default());
        }
        #[cfg(not(feature = "serde"))]
        {
            let v = obj.as_ref(py);
            Ok(v.str()?.to_str()?.to_owned())
        }
    }

    /// `from_json(s) -> object` si `serde`, sinon `s`.
    #[pyfunction]
    pub fn from_json(py: Python<'_>, s: String) -> PyResult<Py<PyAny>> {
        #[cfg(feature = "serde")]
        {
            let v: serde_json::Value = serde_json::from_str(&s).unwrap_or(serde_json::Value::Null);
            return v.into_py(py).extract(py);
        }
        #[cfg(not(feature = "serde"))]
        {
            Ok(s.into_py(py))
        }
    }

    /// Module `vitte_python`.
    #[pymodule]
    pub fn vitte_python(py: Python, m: &PyModule) -> PyResult<()> {
        m.add_function(wrap_pyfunction!(greet, m)?)?;
        m.add_function(wrap_pyfunction!(sum_list, m)?)?;
        m.add_function(wrap_pyfunction!(to_json, m)?)?;
        m.add_function(wrap_pyfunction!(from_json, m)?)?;
        // Constante de version
        m.add("__version__", env!("CARGO_PKG_VERSION"))?;
        // Exemple: expose la liste [1,2,3]
        let l = PyList::new(py, [1,2,3]);
        m.add("demo_list", l)?;
        Ok(())
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

    #[cfg(feature = "pyo3")]
    #[test]
    fn eval_and_run() {
        assert!(embed::py_run("x=1+1").is_ok());
        let s = embed::py_eval_str("1+2").unwrap();
        assert!(s.contains("3"));
    }
}