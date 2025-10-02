

#![deny(missing_docs)]
//! vitte-php — Interop PHP pour Vitte
//!
//! Objectifs :
//! - Préparer une base pour écrire des extensions PHP en Rust (FFI Zend API).
//! - Fournir des erreurs typées, conversions simples et macro d’export facultative.
//! - Compiler sans dépendre des headers PHP par défaut (aucun lien exigé).
//!
//! ⚠️ Remarque : pour une extension fonctionnelle, il faut lier contre l’API C de PHP
//! (Zend), déclarer `zend_module_entry` et `zend_get_module`. La macro d’export ci‑dessous
//! montre la structure, mais le lien effectif nécessite les en‑têtes/binaires PHP.

use thiserror::Error;

/// Erreurs interop PHP.
#[derive(Debug, Error)]
pub enum PhpError {
    #[error("FFI error: {0}")]
    Ffi(String),
    #[error("conversion error: {0}")]
    Conversion(String),
    #[error("null pointer")]
    NullPtr,
    #[error("other: {0}")]
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, PhpError>;

/// Conversions utilitaires (stubs extensibles).
pub mod convert {
    use super::*;

    /// Encode un bool Rust au format entier PHP (0/1).
    #[inline]
    pub fn bool_to_php(b: bool) -> i64 { if b { 1 } else { 0 } }

    /// Conversion en C string pour API Zend.
    #[inline]
    pub fn to_cstring(s: &str) -> Result<std::ffi::CString> {
        std::ffi::CString::new(s).map_err(|e| PhpError::Conversion(e.to_string()))
    }
}

/// Déclarations minimales pour illustrer la structure d'un module.
/// Ces types ne correspondent pas 1:1 à l'API Zend et servent uniquement de placeholders
/// afin de permettre un scaffolding compilable sans headers PHP.
///
/// Pour un binding réel, utilisez des crates dédiés ou générez via bindgen.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct ZendModuleEntry {
    _private: [u8; 0], // placeholder
}

/// Macro utilitaire pour exposer un module PHP. Elle exporte symboles MINIT/MINFO
/// *vides* et un `zend_get_module` qui renvoie un pointeur `NULL` (placeholder).
///
/// À adapter en implémentation réelle lorsque vous liez contre PHP.
#[macro_export]
macro_rules! export_php_extension {
    ($module_name:expr) => {
        #[no_mangle]
        pub extern "C" fn php_minit() -> i32 { 0 } // MINIT success
        #[no_mangle]
        pub extern "C" fn php_minfo()  {}          // MINFO noop

        /// Placeholder: PHP s'attend à un `zend_get_module` retournant un `*mut zend_module_entry`.
        /// Ici on renvoie null pour permettre la compilation côté Rust sans lier PHP.
        #[no_mangle]
        pub extern "C" fn zend_get_module() -> *mut $crate::ZendModuleEntry {
            core::ptr::null_mut()
        }

        /// Nom logique du module pour traces/tests.
        pub const VITTE_PHP_MODULE_NAME: &str = $module_name;
    };
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_fmt() {
        let e = PhpError::Conversion("bad".into());
        assert!(format!("{e}").contains("bad"));
    }

    #[test]
    fn bool_encoding() {
        assert_eq!(convert::bool_to_php(true), 1);
        assert_eq!(convert::bool_to_php(false), 0);
    }

    #[test]
    fn cstring_ok() {
        let s = convert::to_cstring("ok").unwrap();
        assert_eq!(s.to_str().unwrap(), "ok");
    }
}