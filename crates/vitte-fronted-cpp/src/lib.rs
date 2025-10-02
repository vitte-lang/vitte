//! vitte-frontend-cpp — pont C++ pour le frontend du compilateur Vitte
//!
//! Objectif : exposer lexer + parser + AST du frontend Vitte via C++.
//!
//! Fonctionnalités :
//! - Fonctions FFI `extern "C"` pour parser une source et récupérer l’AST sérialisé.
//! - Si `feature="cxx"` : expose un `cxx::bridge` pour intégration directe C++ idiomatique.
//!
//! Exemple C++ (FFI simple) :
//! ```cpp
//! extern "C" {
//!   const char* vitte_frontend_parse(const char* src);
//!   void vitte_frontend_string_free(const char* s);
//! }
//! // ...
//! const char* json = vitte_frontend_parse("fn main(){42}");
//! printf("%s\n", json);
//! vitte_frontend_string_free(json);
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]

extern crate alloc;
use alloc::{string::String, vec::Vec};

#[cfg(feature="std")]
use std::ffi::{CString, CStr};
#[cfg(feature="std")]
use std::os::raw::c_char;

use vitte_core::Span;
use vitte_lexer::Lexer;
use vitte_parser::Parser;
use vitte_ast::Ast;
use vitte_frontend::{compile_to_ast, Error};

/// Résultat FFI : pointeur vers C string allouée par Rust.
/// Libérer via `vitte_frontend_string_free`.
#[cfg(feature="std")]
#[no_mangle]
pub extern "C" fn vitte_frontend_parse(src: *const c_char) -> *const c_char {
    if src.is_null() {
        return core::ptr::null();
    }
    unsafe {
        match CStr::from_ptr(src).to_str() {
            Ok(s) => {
                match compile_to_ast(s) {
                    Ok(ast) => {
                        // sérialisation JSON basique
                        let json = serde_json::to_string(&ast).unwrap_or_else(|_| "{}".into());
                        let cstr = CString::new(json).unwrap();
                        cstr.into_raw()
                    }
                    Err(e) => {
                        let msg = format!("{e:?}");
                        CString::new(msg).unwrap().into_raw()
                    }
                }
            }
            Err(_) => CString::new("invalid utf-8").unwrap().into_raw(),
        }
    }
}

/// Libère une string allouée par `vitte_frontend_parse`.
#[cfg(feature="std")]
#[no_mangle]
pub extern "C" fn vitte_frontend_string_free(s: *mut c_char) {
    if !s.is_null() {
        unsafe { let _ = CString::from_raw(s); }
    }
}

/// Version C++ idiomatique via cxx::bridge.
#[cfg(feature="cxx")]
pub mod bridge {
    use super::*;
    use cxx::CxxString;

    #[cxx::bridge(namespace = "vitte")]
    mod ffi {
        extern "Rust" {
            /// Parse et renvoie JSON (String).
            fn parse_json(src: &str) -> String;
        }
    }

    /// Parse et sérialise en JSON.
    pub fn parse_json(src: &str) -> String {
        match compile_to_ast(src) {
            Ok(ast) => serde_json::to_string(&ast).unwrap_or_else(|_| "{}".into()),
            Err(e) => format!("{e:?}"),
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */
#[cfg(all(test, feature="std"))]
mod tests {
    use super::*;
    #[test]
    fn parse_ok() {
        let src = CString::new("fn main(){42}").unwrap();
        let p = vitte_frontend_parse(src.as_ptr());
        assert!(!p.is_null());
        unsafe { vitte_frontend_string_free(p as *mut c_char); }
    }
}