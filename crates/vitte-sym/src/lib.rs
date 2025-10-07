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
use std::sync::Arc;
use string_interner::Symbol as InternerSymbol;
use string_interner::{StringInterner, backend::DefaultBackend, symbol::SymbolU32};

/// Identifiant de symbole (u32)
pub type SymId = SymbolU32;
type SymBackend = DefaultBackend<SymId>;

/// Erreurs liées aux symboles
#[derive(Debug, thiserror::Error)]
pub enum SymError {
    /// Symbole introuvable (l'identifiant ne correspond à aucune entrée).
    #[error("symbole introuvable: {0:?}")]
    NotFound(SymId),
    /// Autre erreur liée aux symboles.
    #[error("autre: {0}")]
    Other(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, SymError>;

/// Table de symboles (interner)
#[derive(Debug, Clone)]
pub struct SymbolTable {
    inner: Arc<RwLock<StringInterner<SymBackend>>>,
}

impl SymbolTable {
    /// Nouvelle table vide
    pub fn new() -> Self {
        Self { inner: Arc::new(RwLock::new(StringInterner::<SymBackend>::default())) }
    }

    /// Intern une chaîne et retourne son identifiant
    pub fn intern(&mut self, s: &str) -> SymId {
        self.inner.write().get_or_intern(s)
    }

    /// Résout un identifiant en chaîne
    pub fn resolve(&self, id: SymId) -> Result<String> {
        self.inner
            .read()
            .resolve(id)
            .map(|s| s.to_string())
            .ok_or(SymError::NotFound(id))
    }

    /// Nombre de symboles stockés
    pub fn len(&self) -> usize {
        self.inner.read().len()
    }
}

/// Symbole sérialisable
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Symbol(pub SymId);

#[cfg(feature = "serde")]
impl serde::Serialize for Symbol {
    fn serialize<S: serde::Serializer>(
        &self,
        serializer: S,
    ) -> std::result::Result<S::Ok, S::Error> {
        // Serialize as a compact u32 value
        serializer.serialize_u32(InternerSymbol::to_usize(self.0) as u32)
    }
}

#[cfg(feature = "serde")]
impl<'de> serde::Deserialize<'de> for Symbol {
    fn deserialize<D: serde::Deserializer<'de>>(
        deserializer: D,
    ) -> std::result::Result<Self, D::Error> {
        let v = <u32 as serde::Deserialize>::deserialize(deserializer)? as usize;
        InternerSymbol::try_from_usize(v)
            .map(Symbol)
            .ok_or_else(|| serde::de::Error::custom("invalid symbol id"))
    }
}

impl Symbol {
    /// Crée un nouveau symbole
    pub fn new(id: SymId) -> Self {
        Self(id)
    }
    /// Identifiant interne
    pub fn id(&self) -> SymId {
        self.0
    }
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
