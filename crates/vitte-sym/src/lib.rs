

#![deny(missing_docs)]
//! vitte-sym — symboles et interning pour Vitte
//!
//! Fournit :
//! - Interner de chaînes rapide (string-interner).
//! - SymId stable (u32/u64).
//! - Tables globales thread-safe avec `parking_lot::RwLock`.
//! - Sérialisation optionnelle via serde/bincode.
//!
//! Exemple :
//! ```
//! use vitte_sym::{SymbolTable, Symbol};
//! let mut t = SymbolTable::new();
//! let s1 = t.intern("hello");
//! let s2 = t.intern("hello");
//! assert_eq!(s1, s2);
//! assert_eq!(t.resolve(s1).unwrap(), "hello");
//! ```

use parking_lot::RwLock;
use string_interner::{StringInterner, symbol::SymbolU32};
use std::sync::Arc;

#[cfg(feature = "serde")]
use serde::{Serialize, Deserialize};

/// Identifiant de symbole (u32)
pub type SymId = SymbolU32;

/// Erreurs liées aux symboles
#[derive(Debug, thiserror::Error)]
pub enum SymError {
    #[error("symbole introuvable: {0:?}")]
    NotFound(SymId),
    #[error("autre: {0}")]
    Other(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, SymError>;

/// Table de symboles (interner)
#[derive(Debug, Clone)]
pub struct SymbolTable {
    inner: Arc<RwLock<StringInterner<SymId>>>,
}

impl SymbolTable {
    /// Nouvelle table vide
    pub fn new() -> Self {
        Self { inner: Arc::new(RwLock::new(StringInterner::default())) }
    }

    /// Intern une chaîne et retourne son identifiant
    pub fn intern(&mut self, s: &str) -> SymId {
        self.inner.write().get_or_intern(s)
    }

    /// Résout un identifiant en chaîne
    pub fn resolve(&self, id: SymId) -> Result<String> {
        self.inner.read().resolve(id)
            .map(|s| s.to_string())
            .ok_or(SymError::NotFound(id))
    }

    /// Nombre de symboles stockés
    pub fn len(&self) -> usize { self.inner.read().len() }
}

/// Symbole sérialisable
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Symbol(pub SymId);

impl Symbol {
    /// Crée un nouveau symbole
    pub fn new(id: SymId) -> Self { Self(id) }
    /// Identifiant interne
    pub fn id(&self) -> SymId { self.0 }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn intern_and_resolve() {
        let mut t = SymbolTable::new();
        let s1 = t.intern("foo");
        let s2 = t.intern("foo");
        assert_eq!(s1, s2);
        assert_eq!(t.resolve(s1).unwrap(), "foo");
        assert_eq!(t.len(), 1);
    }
}