//! vitte-cache — système de cache multi-stratégies (mémoire, disque, distribué stub)
//!
//! Points clés
//! - API unifiée [`Cache`]: get/set/remove/clear + TTL optionnel.
//! - Mémoire: `MemoryCache` (TTL, limite de taille, éviction approximative LRU).
//! - Disque (feature `disk`): `DiskCache` sérialisé en JSON, répertoires namespacés.
//! - Chaînage: `ChainCache` agrège plusieurs backends (read-through + write-through).
//! - Aide: `memoize` (sync) avec clé dérivée d’un input.
//!
//! Exemple rapide
//! ```ignore
//! use vitte_cache::*;
//! let mem = MemoryCache::builder().capacity(10_000).default_ttl_ms(5_000).build();
//! let mut cache = ChainCache::new(vec![Box::new(mem)]);
//! cache.set("ns:key", "value".to_string(), None)?;
//! assert_eq!(cache.get::<String>("ns:key")?.as_deref(), Some("value"));
//! ```

#![forbid(unsafe_code)]

use ahash::{AHashMap, AHasher};
use parking_lot::{Mutex, RwLock};
use std::any::type_name;
use std::fmt;
use std::hash::{Hash, Hasher};
use std::path::{Path, PathBuf};
use std::sync::Arc;
use std::time::{Duration, Instant};
use thiserror::Error;

#[cfg(feature = "disk")]
use serde::{de::DeserializeOwned, Deserialize, Serialize};

#[cfg(not(feature = "disk"))]
use serde::{de::DeserializeOwned, Deserialize};

#[derive(Debug, Error)]
pub enum CacheError {
    #[error("not found")]
    NotFound,
    #[cfg(feature = "disk")]
    #[error("i/o: {0}")]
    Io(#[from] std::io::Error),
    #[cfg(feature = "disk")]
    #[error("serde: {0}")]
    Serde(#[from] serde_json::Error),
    #[error("type mismatch: stored type `{stored}` vs requested `{requested}`")]
    TypeMismatch { stored: &'static str, requested: &'static str },
    #[error("invalid namespace or key")]
    InvalidKey,
}

pub type Result<T> = std::result::Result<T, CacheError>;

/// Convertit n’importe quelles données en clé hexa stable.
/// Utile pour `memoize`.
pub fn key_hex<T: Hash>(ns: &str, value: &T) -> String {
    let mut h = AHasher::default();
    ns.hash(&mut h);
    value.hash(&mut h);
    format!("{ns}:{:016x}", h.finish())
}

/// Métadonnées d’entrée de cache.
#[derive(Debug, Clone, Copy)]
pub struct Meta {
    pub expires_at: Option<Instant>,
    pub stored_type: &'static str,
}

impl Meta {
    fn expired(&self, now: Instant) -> bool {
        match self.expires_at {
            Some(t) => now >= t,
            None => false,
        }
    }
}

/// Objet sérialisable sur disque.
#[cfg_attr(feature = "disk", derive(Serialize, Deserialize))]
#[derive(Debug, Clone)]
struct DiskEnvelope<T> {
    type_name: String,
    expires_epoch_ms: Option<u128>,
    value: T,
}

// ===========================================================================
// Cache trait
// ===========================================================================

/// Backend de cache. Les implémentations doivent être thread-safe.
pub trait Cache: Send + Sync {
    /// Récupère si présent et non expiré.
    fn get_raw(&self, key: &str) -> Result<(Vec<u8>, Meta)>;

    /// Écrit une valeur sérialisée arbitraire.
    fn set_raw(&self, key: &str, bytes: Vec<u8>, meta: Meta) -> Result<()>;

    /// Supprime une entrée si présente.
    fn remove(&self, key: &str) -> Result<()>;

    /// Vide ce cache.
    fn clear(&self) -> Result<()>;
}

fn ensure_key(key: &str) -> Result<()> {
    if key.is_empty() || !key.as_bytes().iter().all(|&b| b >= 32) || !key.contains(':') {
        return Err(CacheError::InvalidKey);
    }
    Ok(())
}

// Helpers de (dé)sérialisation en mémoire
fn encode_with_typename<T: SerializeOwned>(v: &T, ttl: Option<Duration>) -> (Vec<u8>, Meta)
where
    T: ?Sized,
{
    // Format binaire propriétaire simple:
    // [u8 type_len][type_name bytes][u64 ttl_ms or u64::MAX][payload JSON]
    #[cfg(feature = "disk")]
    let payload = serde_json::to_vec(v).expect("serialize");
    #[cfg(not(feature = "disk"))]
    let payload = b"{}".to_vec(); // pour compilations sans `serde_json` (disk off), mémoire utilise binaire minimal

    let tname = type_name::<T>();
    let mut out = Vec::with_capacity(1 + tname.len() + 8 + payload.len());
    assert!(tname.len() < 255, "type name too long");
    out.push(tname.len() as u8);
    out.extend_from_slice(tname.as_bytes());
    let ttl_ms = ttl.map(|d| d.as_millis() as u64).unwrap_or(u64::MAX);
    out.extend_from_slice(&ttl_ms.to_le_bytes());
    out.extend_from_slice(&payload);

    let expires_at = ttl.map(|d| Instant::now() + d);
    (out, Meta { expires_at, stored_type: tname })
}

fn decode_check_type<'a>(
    now: Instant,
    bytes: &'a [u8],
    expect: &'static str,
) -> Result<(&'a [u8], Meta)> {
    if bytes.len() < 1 + 8 {
        return Err(CacheError::NotFound);
    }
    let tlen = bytes[0] as usize;
    if bytes.len() < 1 + tlen + 8 {
        return Err(CacheError::NotFound);
    }
    let tname = std::str::from_utf8(&bytes[1..1 + tlen]).unwrap_or("");
    let mut ttlb = [0u8; 8];
    ttlb.copy_from_slice(&bytes[1 + tlen..1 + tlen + 8]);
    let ttl_ms = u64::from_le_bytes(ttlb);
    let payload = &bytes[1 + tlen + 8..];

    let meta = Meta {
        expires_at: (ttl_ms != u64::MAX).then(|| now + Duration::from_millis(0)), // expiration évaluée à l’insertion; ici, on ne peut que refuser si déjà expiré en mémoire
        stored_type: tname,
    };
    if meta.expired(now) {
        return Err(CacheError::NotFound);
    }
    if tname != expect {
        return Err(CacheError::TypeMismatch { stored: tname, requested: expect });
    }
    Ok((payload, meta))
}

pub trait SerializeOwned: erased_serde::Serialize {}
impl<T: serde::Serialize> SerializeOwned for T {}

#[cfg(feature = "disk")]
fn serde_from_slice<T: DeserializeOwned>(b: &[u8]) -> Result<T> {
    Ok(serde_json::from_slice(b)?)
}
#[cfg(not(feature = "disk"))]
fn serde_from_slice<T: DeserializeOwned>(_b: &[u8]) -> Result<T> {
    // pas de JSON disponible → le cache mémoire ne peut pas restaurer un type sans `disk`.
    Err(CacheError::TypeMismatch { stored: "unknown", requested: type_name::<T>() })
}

/// API typée.
pub trait TypedCache: Cache {
    fn get<T: DeserializeOwned>(&self, key: &str) -> Result<Option<T>> {
        ensure_key(key)?;
        match self.get_raw(key) {
            Ok((bytes, _m)) => {
                let now = Instant::now();
                let expect = type_name::<T>();
                let (payload, _meta) = decode_check_type(now, &bytes, expect)?;
                let v = serde_from_slice(payload)?;
                Ok(Some(v))
            }
            Err(CacheError::NotFound) => Ok(None),
            Err(e) => Err(e),
        }
    }

    fn set<T: serde::Serialize>(&self, key: &str, value: &T, ttl: Option<Duration>) -> Result<()> {
        ensure_key(key)?;
        let (bytes, meta) = encode_with_typename(value, ttl);
        self.set_raw(key, bytes, meta)
    }
}

impl<T: Cache + ?Sized> TypedCache for T {}

// ===========================================================================
// MemoryCache
// ===========================================================================

#[derive(Debug)]
struct MemEntry {
    bytes: Vec<u8>,
    meta: Meta,
    last_touch: u64,
}

pub struct MemoryCache {
    inner: RwLock<AHashMap<String, MemEntry>>,
    max_items: usize,
    default_ttl: Option<Duration>,
    touch: Mutex<u64>,
}

#[derive(Default)]
pub struct MemoryCacheBuilder {
    capacity: usize,
    default_ttl_ms: Option<u64>,
}
impl MemoryCacheBuilder {
    pub fn capacity(mut self, n: usize) -> Self {
        self.capacity = n.max(1);
        self
    }
    pub fn default_ttl_ms(mut self, ms: u64) -> Self {
        self.default_ttl_ms = Some(ms);
        self
    }
    pub fn build(self) -> MemoryCache {
        MemoryCache {
            inner: RwLock::new(AHashMap::new()),
            max_items: self.capacity.max(1024),
            default_ttl: self.default_ttl_ms.map(|ms| Duration::from_millis(ms)),
            touch: Mutex::new(0),
        }
    }
}
impl MemoryCache {
    pub fn builder() -> MemoryCacheBuilder {
        MemoryCacheBuilder { capacity: 16_384, default_ttl_ms: None }
    }

    fn bump_touch(&self) -> u64 {
        let mut t = self.touch.lock();
        *t += 1;
        *t
    }

    fn maybe_evict(&self, guard: &mut AHashMap<String, MemEntry>) {
        if guard.len() <= self.max_items {
            return;
        }
        // Éviction approximative LRU: supprime ~10% des entrées les moins touchées.
        let evict_n = guard.len().saturating_sub(self.max_items) + (self.max_items / 10).max(1);
        let mut items: Vec<(&str, u64)> = guard.iter().map(|(k, v)| (k.as_str(), v.last_touch)).collect();
        items.sort_by_key(|(_, t)| *t);
        for (k, _) in items.into_iter().take(evict_n) {
            guard.remove(k);
        }
    }
}

impl Cache for MemoryCache {
    fn get_raw(&self, key: &str) -> Result<(Vec<u8>, Meta)> {
        ensure_key(key)?;
        let now = Instant::now();
        let mut map = self.inner.write();
        if let Some(e) = map.get_mut(key) {
            if e.meta.expires_at.map(|t| now >= t).unwrap_or(false) {
                map.remove(key);
                return Err(CacheError::NotFound);
            }
            e.last_touch = self.bump_touch();
            return Ok((e.bytes.clone(), e.meta));
        }
        Err(CacheError::NotFound)
    }

    fn set_raw(&self, key: &str, bytes: Vec<u8>, mut meta: Meta) -> Result<()> {
        ensure_key(key)?;
        if meta.expires_at.is_none() {
            if let Some(d) = self.default_ttl {
                meta.expires_at = Some(Instant::now() + d);
            }
        }
        let mut map = self.inner.write();
        map.insert(
            key.to_string(),
            MemEntry { bytes, meta, last_touch: self.bump_touch() },
        );
        self.maybe_evict(&mut map);
        Ok(())
    }

    fn remove(&self, key: &str) -> Result<()> {
        ensure_key(key)?;
        let mut map = self.inner.write();
        map.remove(key);
        Ok(())
    }

    fn clear(&self) -> Result<()> {
        self.inner.write().clear();
        Ok(())
    }
}

// ===========================================================================
// DiskCache (feature `disk`)
// ===========================================================================

#[cfg(feature = "disk")]
pub struct DiskCache {
    root: PathBuf,
    default_ttl: Option<Duration>,
}

#[cfg(feature = "disk")]
impl DiskCache {
    pub fn new<P: AsRef<Path>>(root: P) -> std::io::Result<Self> {
        std::fs::create_dir_all(&root)?;
        Ok(Self { root: root.as_ref().to_path_buf(), default_ttl: None })
    }

    pub fn default_ttl_ms(mut self, ms: u64) -> Self {
        self.default_ttl = Some(Duration::from_millis(ms));
        self
    }

    fn path_for(&self, key: &str) -> Result<PathBuf> {
        ensure_key(key)?;
        // namespaced: "ns:key"
        let (ns, rest) = key.split_once(':').ok_or(CacheError::InvalidKey)?;
        let mut h = AHasher::default();
        rest.hash(&mut h);
        let file = format!("{:016x}.json", h.finish());
        Ok(self.root.join(ns).join(&file))
    }
}

#[cfg(feature = "disk")]
impl Cache for DiskCache {
    fn get_raw(&self, key: &str) -> Result<(Vec<u8>, Meta)> {
        let path = self.path_for(key)?;
        let now_ms = now_epoch_ms();
        let s = std::fs::read_to_string(path)?;
        let env: DiskEnvelope<serde_json::Value> = serde_json::from_str(&s)?;
        if let Some(exp) = env.expires_epoch_ms {
            if now_ms >= exp {
                return Err(CacheError::NotFound);
            }
        }
        // Re-encode le payload avec l’enveloppe binaire mémoire pour cohérence.
        let (bytes, meta) = encode_with_typename(&env.value, env.expires_epoch_ms.map(|e| {
            let left = e.saturating_sub(now_ms);
            Duration::from_millis(left as u64)
        }));
        Ok((bytes, meta))
    }

    fn set_raw(&self, key: &str, bytes: Vec<u8>, meta: Meta) -> Result<()> {
        let path = self.path_for(key)?;
        if let Some(dir) = path.parent() {
            std::fs::create_dir_all(dir)?;
        }
        // Décoder pour extraire le type et le payload JSON; puis récrire en JSON
        let expect_any = "";
        let (payload, _) = decode_check_type(Instant::now(), &bytes, expect_any)?;
        let value: serde_json::Value = serde_json::from_slice(payload)?;
        let env = DiskEnvelope {
            type_name: "json".to_string(),
            expires_epoch_ms: meta.expires_at.map(|t| now_epoch_ms() + 1), // +1ms best-effort
            value,
        };
        let s = serde_json::to_string_pretty(&env)?;
        std::fs::write(path, s)?;
        Ok(())
    }

    fn remove(&self, key: &str) -> Result<()> {
        let path = self.path_for(key)?;
        if let Err(e) = std::fs::remove_file(&path) {
            if e.kind() != std::io::ErrorKind::NotFound {
                return Err(e.into());
            }
        }
        Ok(())
    }

    fn clear(&self) -> Result<()> {
        // Supprime récursivement le root (sans l’effacer lui-même)
        for ent in walkdir::WalkDir::new(&self.root).into_iter().flatten() {
            if ent.file_type().is_file() {
                let _ = std::fs::remove_file(ent.path());
            }
        }
        Ok(())
    }
}

#[inline]
fn now_epoch_ms() -> u128 {
    use std::time::{SystemTime, UNIX_EPOCH};
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_millis())
        .unwrap_or(0)
}

// ===========================================================================
// ChainCache
// ===========================================================================

pub struct ChainCache {
    layers: Vec<Box<dyn Cache>>,
}

impl ChainCache {
    pub fn new(layers: Vec<Box<dyn Cache>>) -> Self {
        assert!(!layers.is_empty());
        Self { layers }
    }

    pub fn push_layer<C: Cache + 'static>(&mut self, c: C) {
        self.layers.push(Box::new(c));
    }

    /// get typé avec promotion (write-through) vers les couches supérieures.
    pub fn get<T: DeserializeOwned>(&mut self, key: &str) -> Result<Option<T>> {
        ensure_key(key)?;
        let expect = type_name::<T>();
        let now = Instant::now();
        let mut found: Option<(Vec<u8>, Meta, usize)> = None;
        for (idx, layer) in self.layers.iter().enumerate() {
            match layer.get_raw(key) {
                Ok((bytes, meta)) => {
                    // vérifie type
                    let _ = decode_check_type(now, &bytes, expect)?;
                    found = Some((bytes, meta, idx));
                    break;
                }
                Err(CacheError::NotFound) => continue,
                Err(e) => return Err(e),
            }
        }
        if let Some((bytes, meta, where_idx)) = found {
            // remonte dans les couches plus hautes
            for i in 0..where_idx {
                let _ = self.layers[i].set_raw(key, bytes.clone(), meta);
            }
            let (payload, _) = decode_check_type(now, &bytes, expect)?;
            let v = serde_from_slice::<T>(payload)?;
            Ok(Some(v))
        } else {
            Ok(None)
        }
    }

    pub fn set<T: serde::Serialize>(&mut self, key: &str, value: &T, ttl: Option<Duration>) -> Result<()> {
        ensure_key(key)?;
        let (bytes, meta) = encode_with_typename(value, ttl);
        for l in &self.layers {
            l.set_raw(key, bytes.clone(), meta)?;
        }
        Ok(())
    }
}

impl Cache for ChainCache {
    fn get_raw(&self, key: &str) -> Result<(Vec<u8>, Meta)> {
        ensure_key(key)?;
        let now = Instant::now();
        for layer in &self.layers {
            if let Ok((bytes, meta)) = layer.get_raw(key) {
                // ne remonte pas d’ici pour éviter un &mut self
                // l’appelant peut préférer `get<T>()` qui promeut.
                let _ = now; // meta check est fait par chaque couche
                return Ok((bytes, meta));
            }
        }
        Err(CacheError::NotFound)
    }

    fn set_raw(&self, key: &str, bytes: Vec<u8>, meta: Meta) -> Result<()> {
        for layer in &self.layers {
            layer.set_raw(key, bytes.clone(), meta)?;
        }
        Ok(())
    }

    fn remove(&self, key: &str) -> Result<()> {
        let mut last = Ok(());
        for layer in &self.layers {
            if let Err(e) = layer.remove(key) {
                last = Err(e);
            }
        }
        last
    }

    fn clear(&self) -> Result<()> {
        for layer in &self.layers {
            layer.clear()?;
        }
        Ok(())
    }
}

// ===========================================================================
// Memoization helper
// ===========================================================================

/// Calcule et met en cache si absent.
/// - `key` doit inclure un namespace (`"ns:..."`).
/// - `compute` ne doit pas capturer &mut self de `cache`: utilisez une fermeture pure.
pub fn memoize<T, F>(cache: &dyn Cache, key: &str, ttl: Option<Duration>, compute: F) -> Result<T>
where
    T: DeserializeOwned + serde::Serialize,
    F: FnOnce() -> T,
{
    ensure_key(key)?;
    if let Ok((bytes, _)) = cache.get_raw(key) {
        let now = Instant::now();
        let expect = type_name::<T>();
        if let Ok((payload, _)) = decode_check_type(now, &bytes, expect) {
            if let Ok(v) = serde_from_slice::<T>(payload) {
                return Ok(v);
            }
        }
    }
    let v = compute();
    let (bytes, meta) = encode_with_typename(&v, ttl);
    let _ = cache.set_raw(key, bytes, meta);
    Ok(v)
}

// ===========================================================================
// Re-exports
// ===========================================================================
pub use MemoryCache;

// ===========================================================================
// Tests
// ===========================================================================

#[cfg(test)]
mod tests {
    use super::*;
    use std::thread::sleep;

    #[test]
    fn mem_basic_set_get() {
        let mem = MemoryCache::builder().capacity(128).build();
        mem.set("demo:answer", &42i64, None).unwrap();
        let got: Option<i64> = mem.get("demo:answer").unwrap();
        assert_eq!(got, Some(42));
    }

    #[test]
    fn mem_ttl_expires() {
        let mem = MemoryCache::builder().capacity(16).default_ttl_ms(50).build();
        mem.set("t:one", &1u32, None).unwrap(); // TTL par défaut
        let v: Option<u32> = mem.get("t:one").unwrap();
        assert_eq!(v, Some(1));
        sleep(Duration::from_millis(70));
        let v2: Option<u32> = mem.get("t:one").unwrap();
        assert!(v2.is_none());
    }

    #[test]
    fn chain_promote() {
        let upper = MemoryCache::builder().capacity(8).build();
        let lower = MemoryCache::builder().capacity(8).build();
        let mut chain = ChainCache::new(vec![Box::new(upper), Box::new(lower)]);
        chain.set("ns:k", &"v".to_string(), None).unwrap();

        // remove from upper, keep lower
        chain.layers[0].remove("ns:k").unwrap();
        assert!(chain.layers[0].get::<String>("ns:k").unwrap().is_none());
        // read through should pull from lower and promote
        let g = chain.get::<String>("ns:k").unwrap();
        assert_eq!(g.as_deref(), Some("v"));
        assert!(chain.layers[0].get::<String>("ns:k").unwrap().is_some());
    }

    #[test]
    fn memoize_works() {
        let mem = MemoryCache::builder().capacity(64).build();
        let key = key_hex("fib", &30u32);
        let val: i64 = memoize(&mem, &key, None, || 42);
        assert_eq!(val.unwrap(), 42);
        // subsequent is cached
        let val2: i64 = memoize(&mem, &key, None, || 0).unwrap();
        assert_eq!(val2, 42);
    }

    #[cfg(feature = "disk")]
    #[test]
    fn disk_roundtrip() {
        let dir = tempfile::tempdir().unwrap();
        let disk = DiskCache::new(dir.path()).unwrap();
        disk.set("ns:key", &serde_json::json!({"x":1}), None).unwrap();
        let v: Option<serde_json::Value> = disk.get("ns:key").unwrap();
        assert_eq!(v.unwrap()["x"], 1);
    }
}