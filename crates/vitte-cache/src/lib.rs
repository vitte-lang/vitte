//! vitte-cache — cache mémoire minimal, sans dépendances optionnelles.
//!
//! Ce module fournit un cache typé en mémoire, thread-safe, sans `serde`.
//! Quand la feature `disk` est activée, des stubs sont exposés (non-op).
//!
//! Public API stable minimale : `TypedCache`, `MemoryCache`, `memoize`, `CacheError`.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::string::{String, ToString};
use alloc::sync::Arc;
use core::any::{Any, TypeId};
use core::fmt;

#[cfg(feature = "std")]
use std::collections::HashMap;
#[cfg(feature = "std")]
use std::sync::Mutex;
#[cfg(feature = "std")]
use std::time::{Duration, Instant};

#[cfg(not(feature = "std"))]
compile_error!("`vitte-cache` nécessite la feature `std` dans cette implémentation minimaliste.");

/// Erreurs possibles du cache.
#[derive(Debug)]
pub enum CacheError {
    /// Erreur d'E/S (réservée pour l'extension disque).
    Io(&'static str),
    /// Le type ne correspond pas à la valeur stockée.
    TypeMismatch { requested: &'static str, stored: &'static str },
}

impl fmt::Display for CacheError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            CacheError::Io(m) => write!(f, "io error: {m}"),
            CacheError::TypeMismatch { requested, stored } => {
                write!(f, "type mismatch: requested={requested}, stored={stored}")
            }
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for CacheError {}

/// Résultat spécialisé du cache.
pub type Result<T> = core::result::Result<T, CacheError>;

/// Métadonnées associées à une entrée.
#[derive(Debug, Clone)]
struct Meta {
    #[cfg(feature = "std")]
    expires_at: Option<Instant>,
}

#[derive(Debug)]
struct Entry {
    value: Arc<dyn Any + Send + Sync>,
    meta: Meta,
}

/// Interface d'un cache typé minimal.
pub trait TypedCache {
    /// Récupère une valeur clonée si présente et non expirée.
    fn get_cloned<T: Clone + 'static>(&self, key: &str) -> Option<T>;
    /// Enregistre une valeur, avec TTL optionnel (None = jamais expiré).
    fn set<T: 'static + Send + Sync>(
        &self,
        key: &str,
        value: T,
        ttl: Option<Duration>,
    ) -> Result<()>;
    /// Supprime une entrée.
    fn remove(&self, key: &str);
    /// Vide tout.
    fn clear(&self);
}

/// Cache mémoire thread-safe basé sur Mutex + HashMap.
pub struct MemoryCache {
    map: Mutex<HashMap<String, Entry>>,
}

impl Default for MemoryCache {
    fn default() -> Self {
        Self { map: Mutex::new(HashMap::new()) }
    }
}

impl MemoryCache {
    pub fn new() -> Self {
        Self::default()
    }

    #[inline]
    fn is_expired(meta: &Meta) -> bool {
        #[cfg(feature = "std")]
        {
            if let Some(deadline) = meta.expires_at {
                return Instant::now() >= deadline;
            }
        }
        false
    }
}

impl TypedCache for MemoryCache {
    fn get_cloned<T: Clone + 'static>(&self, key: &str) -> Option<T> {
        let mut guard = self.map.lock().ok()?;
        if let Some(entry) = guard.get(key) {
            if Self::is_expired(&entry.meta) {
                // purge paresseuse
                guard.remove(key);
                return None;
            }
            // Vérifie le type et clone si possible
            if (*entry.value).type_id() == TypeId::of::<T>() {
                if let Some(v) = entry.value.downcast_ref::<T>() {
                    return Some(v.clone());
                }
            }
            return None;
        }
        None
    }

    fn set<T: 'static + Send + Sync>(
        &self,
        key: &str,
        value: T,
        ttl: Option<Duration>,
    ) -> Result<()> {
        let mut guard = self.map.lock().map_err(|_| CacheError::Io("poisoned mutex"))?;
        let meta = Meta {
            #[cfg(feature = "std")]
            expires_at: ttl.map(|d| Instant::now() + d),
        };
        let entry = Entry { value: Arc::new(value), meta };
        guard.insert(key.to_string(), entry);
        Ok(())
    }

    fn remove(&self, key: &str) {
        if let Ok(mut g) = self.map.lock() {
            g.remove(key);
        }
    }

    fn clear(&self) {
        if let Ok(mut g) = self.map.lock() {
            g.clear();
        }
    }
}

/// Calcule et met en cache un résultat clonable.
/// - `key` : identifiant logique.
/// - `ttl` : durée de validité (None = infini).
pub fn memoize<T, F>(
    cache: &impl TypedCache,
    key: &str,
    ttl: Option<Duration>,
    compute: F,
) -> Result<T>
where
    T: Clone + 'static + Send + Sync,
    F: FnOnce() -> T,
{
    if let Some(v) = cache.get_cloned::<T>(key) {
        return Ok(v);
    }
    let v = compute();
    cache.set(key, v.clone(), ttl)?;
    Ok(v)
}

/* ─────────────────────── Stubs liés à la feature `disk` ─────────────────────── */

/// Wrapper d'un cache disque optionnel (non-op par défaut).
#[cfg_attr(not(feature = "disk"), allow(dead_code))]
pub struct DiskCache;

#[cfg(feature = "disk")]
impl DiskCache {
    pub fn new(_dir: &str) -> Result<Self> {
        Ok(Self)
    }
    pub fn purge_expired(&self) -> Result<()> {
        Ok(())
    }
}

#[cfg(not(feature = "disk"))]
impl DiskCache {
    pub fn new(_dir: &str) -> Result<Self> {
        Ok(Self)
    }
    pub fn purge_expired(&self) -> Result<()> {
        Ok(())
    }
}

/* ────────────────────────────────── Tests ────────────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;
    use core::time::Duration;

    #[test]
    fn set_get_roundtrip() {
        let c = MemoryCache::new();
        c.set("a", 42u32, None).unwrap();
        assert_eq!(c.get_cloned::<u32>("a"), Some(42));
        assert_eq!(c.get_cloned::<i32>("a"), None); // type mismatch -> None
    }

    #[test]
    fn ttl_expires() {
        let c = MemoryCache::new();
        c.set("x", String::from("ok"), Some(Duration::from_millis(1))).unwrap();
        // Laisse expirer
        #[cfg(feature = "std")]
        std::thread::sleep(std::time::Duration::from_millis(2));
        assert_eq!(c.get_cloned::<String>("x"), None);
    }

    #[test]
    fn memoize_basic() {
        let c = MemoryCache::new();
        let v1 = memoize(&c, "k", None, || 7u8).unwrap();
        let v2 = memoize(&c, "k", None, || 9u8).unwrap(); // récupère du cache
        assert_eq!(v1, 7);
        assert_eq!(v2, 7);
    }
}
