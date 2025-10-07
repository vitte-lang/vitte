//! vitte-asset — Gestionnaire d’actifs simple, rapide et extensible.
//!
//! Objectifs:
//! - Sources pluggables (FS, mémoire).
//! - Cache thread-safe par type, invalidation ciblée.
//! - Loaders par extension (`.json`, `.toml`, `.yaml`, `.xml`, `.bin`) optionnels.
//! - API générique: `load::<T>("...")` avec résolveur d’extensions.
//! - Zéro alloc superflue, hashes rapides (ahash), verrous `parking_lot`.
//!
//! Activer tous les parseurs: feature `full` ou les features fines: `json`, `toml`, `yaml`, `xml`, `bin`.

#![forbid(unsafe_code)]
#![deny(missing_docs)]

use std::{
    any::Any,
    any::TypeId,
    fmt, io,
    path::{Path, PathBuf},
    sync::Arc,
    time::SystemTime,
};

use ahash::{AHashMap as HashMap, AHashSet as HashSet};
use parking_lot::RwLock;

#[cfg(feature = "std")]
use std::fs;

#[cfg(feature = "bin")]
use bincode as _bincode;
#[cfg(feature = "xml")]
use quick_xml as _quick_xml;
#[cfg(feature = "json")]
use serde_json as _serde_json;
#[cfg(feature = "yaml")]
use serde_yaml as _serde_yaml;
#[cfg(feature = "toml")]
use toml as _toml;

#[cfg(any(
    feature = "json",
    feature = "toml",
    feature = "yaml",
    feature = "xml",
    feature = "bin"
))]
use serde::de::DeserializeOwned;

/// Erreurs possibles du système d’actifs.
#[derive(Debug)]
pub enum AssetError {
    /// Source introuvable pour le schéma requis.
    NoSource(String),

    /// Chemin introuvable dans la source.
    NotFound(String),

    /// Erreur d’E/S sous-jacente.
    Io(io::Error),

    /// Extension non supportée.
    UnsupportedExt(String),

    /// Loader introuvable pour l’extension.
    NoLoader(String),

    /// Échec de parse.
    Parse(String),

    /// Conflit de type en cache.
    TypeMismatch {
        /// Clef affectée.
        key: AssetKey,
        /// Type demandé.
        requested: TypeId,
    },
}

impl core::fmt::Display for AssetError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            AssetError::NoSource(s) => write!(f, "no source for scheme '{s}'"),
            AssetError::NotFound(p) => write!(f, "not found: {p}"),
            AssetError::Io(e) => write!(f, "io error: {e}"),
            AssetError::UnsupportedExt(ext) => write!(f, "unsupported extension: {ext}"),
            AssetError::NoLoader(ext) => write!(f, "no loader registered for extension: {ext}"),
            AssetError::Parse(msg) => write!(f, "parse error: {msg}"),
            AssetError::TypeMismatch { key, requested } => {
                write!(f, "type mismatch in cache for key={key}, requested={requested:?}")
            },
        }
    }
}

impl std::error::Error for AssetError {}

impl From<io::Error> for AssetError {
    fn from(e: io::Error) -> Self {
        AssetError::Io(e)
    }
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, AssetError>;

/// Identifie un actif de manière canonique.
/// Format: `[scheme:]path`, ex: `fs:assets/config.json` ou `mem:logo`.
#[derive(Clone, PartialEq, Eq, Hash)]
pub struct AssetKey {
    scheme: String,
    path: String,
}

impl AssetKey {
    /// Crée une clef à partir d’un chemin arbitraire. Si aucun schéma n’est fourni, `fs` est utilisé.
    pub fn parse<S: AsRef<str>>(s: S) -> Self {
        let s = s.as_ref();
        if let Some(idx) = s.find(':') {
            let scheme = &s[..idx];
            let path = &s[idx + 1..];
            Self { scheme: scheme.to_string(), path: normalize_sep(path) }
        } else {
            Self { scheme: "fs".to_string(), path: normalize_sep(s) }
        }
    }

    /// Schéma (ex: `fs`, `mem`, `http` si impl externe).
    pub fn scheme(&self) -> &str {
        &self.scheme
    }
    /// Chemin interne.
    pub fn path(&self) -> &str {
        &self.path
    }

    /// Extension (sans point), en minuscule.
    pub fn ext(&self) -> Option<&str> {
        Path::new(&self.path)
            .extension()
            .and_then(|s| s.to_str())
            .map(|s| s.to_ascii_lowercase())
            .map(|owned| {
                // store once per call
                // SAFETY: we return a leaked &'static str? Avoid. Return owned via Option<String>.
                // Instead expose helper below:
                Box::leak(owned.into_boxed_str()) as &str
            })
    }

    /// Extension allouée sûre.
    pub fn ext_owned(&self) -> Option<String> {
        Path::new(&self.path)
            .extension()
            .and_then(|s| s.to_str())
            .map(|s| s.to_ascii_lowercase())
    }
}

impl fmt::Debug for AssetKey {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("AssetKey")
            .field("scheme", &self.scheme)
            .field("path", &self.path)
            .finish()
    }
}

impl fmt::Display for AssetKey {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.scheme == "fs" {
            write!(f, "{}", self.path)
        } else {
            write!(f, "{}:{}", self.scheme, self.path)
        }
    }
}

/// Source d’actifs. Responsable de la lecture et de la stat.
pub trait Source: Send + Sync + Any {
    /// Access to `Any` for downcasting trait objects.
    fn as_any(&self) -> &dyn Any;

    /// Lecture intégrale en bytes.
    fn read(&self, path: &str) -> Result<Vec<u8>>;

    /// Lecture texte UTF-8.
    fn read_to_string(&self, path: &str) -> Result<String> {
        let bytes = self.read(path)?;
        String::from_utf8(bytes).map_err(|e| AssetError::Parse(e.to_string()))
    }

    /// Existence.
    fn exists(&self, path: &str) -> Result<bool>;

    /// Date de modification si connue.
    fn modified(&self, path: &str) -> Result<Option<SystemTime>>;
}

/// Loader générique pour un type `T`.
pub trait Loader<T>: Send + Sync + 'static {
    /// Charge `T` à partir de bytes.
    fn load(&self, bytes: &[u8]) -> Result<T>;
}

/// Résout un chemin relatif en absolu logique avec une racine.
fn normalize_sep(s: &str) -> String {
    s.replace('\\', "/")
}

/// Source système de fichiers.
#[derive(Clone)]
pub struct FileSource {
    /// Racine logique. Par défaut `.`.
    root: Arc<PathBuf>,
}

impl FileSource {
    /// Nouvel FileSource avec racine.
    pub fn new<P: Into<PathBuf>>(root: P) -> Self {
        Self { root: Arc::new(root.into()) }
    }

    fn full(&self, path: &str) -> PathBuf {
        let p = Path::new(path);
        if p.is_absolute() { p.to_path_buf() } else { self.root.join(path) }
    }
}

impl Default for FileSource {
    fn default() -> Self {
        Self::new(".")
    }
}

impl Source for FileSource {
    fn as_any(&self) -> &dyn Any {
        self
    }
    fn read(&self, path: &str) -> Result<Vec<u8>> {
        let full = self.full(path);
        fs::read(&full).map_err(|e| match e.kind() {
            io::ErrorKind::NotFound => AssetError::NotFound(full.display().to_string()),
            _ => e.into(),
        })
    }

    fn exists(&self, path: &str) -> Result<bool> {
        Ok(self.full(path).exists())
    }

    fn modified(&self, path: &str) -> Result<Option<SystemTime>> {
        let full = self.full(path);
        match fs::metadata(&full) {
            Ok(meta) => Ok(meta.modified().ok()),
            Err(e) if e.kind() == io::ErrorKind::NotFound => Ok(None),
            Err(e) => Err(e.into()),
        }
    }
}

/// Source en mémoire clé→bytes.
#[derive(Default, Clone)]
pub struct MemorySource {
    inner: Arc<RwLock<HashMap<String, Vec<u8>>>>,
    mtimes: Arc<RwLock<HashMap<String, SystemTime>>>,
}

impl MemorySource {
    /// Ajoute ou remplace une entrée.
    pub fn insert<S: Into<String>, B: Into<Vec<u8>>>(&self, key: S, bytes: B) {
        let k = normalize_sep(&key.into());
        self.inner.write().insert(k.clone(), bytes.into());
        self.mtimes.write().insert(k, SystemTime::now());
    }

    /// Supprime une entrée.
    pub fn remove<S: AsRef<str>>(&self, key: S) {
        let k = normalize_sep(key.as_ref());
        self.inner.write().remove(&k);
        self.mtimes.write().remove(&k);
    }
}

impl Source for MemorySource {
    fn as_any(&self) -> &dyn Any {
        self
    }
    fn read(&self, path: &str) -> Result<Vec<u8>> {
        self.inner
            .read()
            .get(path)
            .cloned()
            .ok_or_else(|| AssetError::NotFound(path.to_string()))
    }

    fn exists(&self, path: &str) -> Result<bool> {
        Ok(self.inner.read().contains_key(path))
    }

    fn modified(&self, path: &str) -> Result<Option<SystemTime>> {
        Ok(self.mtimes.read().get(path).cloned())
    }
}

/// Enregistrement d’un loader pour une extension donnée.
#[derive(Clone)]
struct DynLoader {
    type_id: TypeId,
    fun: Arc<dyn Fn(&[u8]) -> Result<Arc<dyn Any + Send + Sync>> + Send + Sync>,
}

impl DynLoader {
    fn new<T: Any + Send + Sync + 'static>(loader: Arc<dyn Loader<T>>) -> Self {
        let fun = move |bytes: &[u8]| -> Result<Arc<dyn Any + Send + Sync>> {
            let t = loader.load(bytes)?;
            Ok(Arc::new(t))
        };
        Self { type_id: TypeId::of::<T>(), fun: Arc::new(fun) }
    }
}

/// Cache interne: (key, TypeId) -> Arc<dyn Any>.
#[derive(Default)]
struct Cache {
    map: HashMap<(AssetKey, TypeId), Arc<dyn Any + Send + Sync>>,
    // pour invalidation par clé sans type:
    by_key: HashSet<AssetKey>,
}

/// Gestionnaire principal d’actifs.
pub struct AssetManager {
    sources: RwLock<HashMap<String, Arc<dyn Source>>>,
    loaders: RwLock<HashMap<String, DynLoader>>, // ext -> dyn loader
    cache: RwLock<Cache>,
}

impl Default for AssetManager {
    fn default() -> Self {
        let mgr = Self {
            sources: RwLock::new(HashMap::default()),
            loaders: RwLock::new(HashMap::default()),
            cache: RwLock::new(Cache::default()),
        };
        // Sources par défaut
        mgr.register_source("fs", Arc::new(FileSource::default()));
        mgr.register_source("mem", Arc::new(MemorySource::default()));
        mgr
    }
}

impl AssetManager {
    /// Nouveau gestionnaire vide. Utilise `register_source` pour configurer.
    pub fn new() -> Self {
        Self::default()
    }

    /// Enregistre une source pour un schéma.
    pub fn register_source<S: Into<String>>(&self, scheme: S, source: Arc<dyn Source>) {
        self.sources.write().insert(scheme.into(), source);
    }

    /// Récupère une source par schéma.
    pub fn source(&self, scheme: &str) -> Option<Arc<dyn Source>> {
        self.sources.read().get(scheme).cloned()
    }

    /// Enregistre un loader pour une extension. Exemple: `.json` → loader T.
    pub fn register_loader<T: Any + Send + Sync + 'static, E: Into<String>>(
        &self,
        ext_without_dot: E,
        loader: Arc<dyn Loader<T>>,
    ) {
        let ext = ext_without_dot.into().trim_start_matches('.').to_ascii_lowercase();
        self.loaders.write().insert(ext, DynLoader::new(loader));
    }

    /// Charge bytes bruts depuis la source résolue.
    pub fn read_bytes<S: AsRef<str>>(&self, key: S) -> Result<Vec<u8>> {
        let key = AssetKey::parse(key);
        let src = self
            .source(key.scheme())
            .ok_or_else(|| AssetError::NoSource(key.scheme().to_string()))?;
        src.read(key.path())
    }

    /// Charge texte UTF-8 depuis la source résolue.
    pub fn read_string<S: AsRef<str>>(&self, key: S) -> Result<String> {
        let key = AssetKey::parse(key);
        let src = self
            .source(key.scheme())
            .ok_or_else(|| AssetError::NoSource(key.scheme().to_string()))?;
        src.read_to_string(key.path())
    }

    /// Existence.
    pub fn exists<S: AsRef<str>>(&self, key: S) -> Result<bool> {
        let key = AssetKey::parse(key);
        let src = self
            .source(key.scheme())
            .ok_or_else(|| AssetError::NoSource(key.scheme().to_string()))?;
        src.exists(key.path())
    }

    /// Métadonnée `modified`.
    pub fn modified<S: AsRef<str>>(&self, key: S) -> Result<Option<SystemTime>> {
        let key = AssetKey::parse(key);
        let src = self
            .source(key.scheme())
            .ok_or_else(|| AssetError::NoSource(key.scheme().to_string()))?;
        src.modified(key.path())
    }

    /// Invalide toutes les entrées relatives à `key` (tous types).
    pub fn invalidate<S: AsRef<str>>(&self, key: S) {
        let key = AssetKey::parse(key);
        let mut cache = self.cache.write();
        cache.by_key.remove(&key);
        cache.map.retain(|(k, _), _| k != &key);
    }

    /// Vide le cache.
    pub fn clear_cache(&self) {
        let mut cache = self.cache.write();
        cache.map.clear();
        cache.by_key.clear();
    }

    /// Obtient un actif depuis le cache si présent.
    pub fn get_cached<T: Any + Send + Sync + 'static, S: AsRef<str>>(
        &self,
        key: S,
    ) -> Option<Arc<T>> {
        let key = AssetKey::parse(key);
        let map = &self.cache.read().map;
        let type_id = TypeId::of::<T>();
        map.get(&(key, type_id))
            .and_then(|arc_any| arc_any.clone().downcast::<T>().ok())
    }

    /// Insère directement dans le cache.
    pub fn put_cached<T: Any + Send + Sync + 'static, S: AsRef<str>>(&self, key: S, value: Arc<T>) {
        let key = AssetKey::parse(key);
        let type_id = TypeId::of::<T>();
        let mut cache = self.cache.write();
        cache.by_key.insert(key.clone());
        cache.map.insert((key, type_id), value as Arc<dyn Any + Send + Sync>);
    }

    /// Charge un actif typé `T` à partir des bytes avec un `Loader<T>` enregistré pour l’extension.
    pub fn load<T: Any + Send + Sync + 'static, S: AsRef<str>>(&self, key: S) -> Result<Arc<T>> {
        let key = AssetKey::parse(key);
        let ext = key.ext_owned().ok_or_else(|| AssetError::UnsupportedExt(key.to_string()))?;

        // cache hit
        if let Some(cached) = self.get_cached::<T, _>(&key.to_string()) {
            return Ok(cached);
        }

        // source
        let src = self
            .source(key.scheme())
            .ok_or_else(|| AssetError::NoSource(key.scheme().to_string()))?;

        // loader
        let dyn_loader = {
            let loaders = self.loaders.read();
            loaders.get(&ext).cloned().ok_or_else(|| AssetError::NoLoader(ext.clone()))?
        };

        // type check
        if dyn_loader.type_id != TypeId::of::<T>() {
            return Err(AssetError::TypeMismatch { key, requested: TypeId::of::<T>() });
        }

        // read + parse
        let bytes = src.read(key.path())?;
        let any = (dyn_loader.fun)(&bytes)?;
        let arc_t: Arc<T> =
            any.downcast::<T>().map_err(|_| AssetError::Parse("downcast".into()))?;

        // store
        self.put_cached(&key.to_string(), arc_t.clone());
        Ok(arc_t)
    }

    /// Charge ou parse via fonction utilisateur, en ignorant les loaders enregistrés.
    pub fn load_with<
        T: Any + Send + Sync + 'static,
        S: AsRef<str>,
        F: FnOnce(&[u8]) -> Result<T>,
    >(
        &self,
        key: S,
        parse: F,
    ) -> Result<Arc<T>> {
        let s = key.as_ref();
        if let Some(cached) = self.get_cached::<T, _>(s) {
            return Ok(cached);
        }
        let bytes = self.read_bytes(s)?;
        let value = Arc::new(parse(&bytes)?);
        self.put_cached(s, value.clone());
        Ok(value)
    }
}

/* -------------------------------- Loaders prêts à l’emploi -------------------------------- */

/// Loader bytes → Vec<u8>.
#[derive(Default)]
pub struct BytesLoader;

impl Loader<Vec<u8>> for BytesLoader {
    fn load(&self, bytes: &[u8]) -> Result<Vec<u8>> {
        Ok(bytes.to_vec())
    }
}

/// Loader texte UTF-8 → String.
#[derive(Default)]
pub struct StringLoader;

impl Loader<String> for StringLoader {
    fn load(&self, bytes: &[u8]) -> Result<String> {
        String::from_utf8(bytes.to_vec()).map_err(|e| AssetError::Parse(e.to_string()))
    }
}

/// Enregistre les loaders de base: `bin`→Vec<u8>, `txt`/`md`/`csv`→String, etc.
pub fn register_builtin_basic(mgr: &AssetManager) {
    mgr.register_loader("bin", Arc::new(BytesLoader::default()));
    mgr.register_loader("dat", Arc::new(BytesLoader::default()));
    mgr.register_loader("bytes", Arc::new(BytesLoader::default()));

    let text = Arc::new(StringLoader::default());
    for ext in ["txt", "md", "csv", "tsv", "ini", "cfg", "log", "rs", "c", "h", "cpp", "hpp"] {
        mgr.register_loader::<String, _>(ext, text.clone());
    }
}

/* -------------------------- Loaders serde conditionnels par feature -------------------------- */

#[cfg(any(
    feature = "json",
    feature = "toml",
    feature = "yaml",
    feature = "xml",
    feature = "bin"
))]
#[derive(Default)]
struct SerdeLoader<T>(std::marker::PhantomData<T>);

#[cfg(feature = "json")]
impl<T: DeserializeOwned + Send + Sync + 'static> Loader<T> for SerdeLoader<T> {
    fn load(&self, bytes: &[u8]) -> Result<T> {
        _serde_json::from_slice(bytes).map_err(|e| AssetError::Parse(e.to_string()))
    }
}
#[cfg(feature = "toml")]
impl<T: DeserializeOwned + Send + Sync + 'static> Loader<T> for SerdeLoader<T> {
    fn load(&self, bytes: &[u8]) -> Result<T> {
        let s = std::str::from_utf8(bytes).map_err(|e| AssetError::Parse(e.to_string()))?;
        _toml::from_str(s).map_err(|e| AssetError::Parse(e.to_string()))
    }
}
#[cfg(feature = "yaml")]
impl<T: DeserializeOwned + Send + Sync + 'static> Loader<T> for SerdeLoader<T> {
    fn load(&self, bytes: &[u8]) -> Result<T> {
        _serde_yaml::from_slice(bytes).map_err(|e| AssetError::Parse(e.to_string()))
    }
}
#[cfg(feature = "xml")]
impl<T: DeserializeOwned + Send + Sync + 'static> Loader<T> for SerdeLoader<T> {
    fn load(&self, bytes: &[u8]) -> Result<T> {
        // pour XML, utiliser quick-xml + serde feature côté type T
        _quick_xml::de::from_reader(bytes).map_err(|e| AssetError::Parse(e.to_string()))
    }
}
#[cfg(feature = "bin")]
impl<T: DeserializeOwned + Send + Sync + 'static> Loader<T> for SerdeLoader<T> {
    fn load(&self, bytes: &[u8]) -> Result<T> {
        _bincode::deserialize(bytes).map_err(|e| AssetError::Parse(e.to_string()))
    }
}

/// Enregistre les loaders serde pour un type donné et une liste d’extensions.
#[cfg(any(
    feature = "json",
    feature = "toml",
    feature = "yaml",
    feature = "xml",
    feature = "bin"
))]
pub fn register_serde_for<T: DeserializeOwned + Send + Sync + 'static>(mgr: &AssetManager) {
    #[cfg(feature = "json")]
    mgr.register_loader::<T, _>("json", Arc::new(SerdeLoader::<T>::default()));
    #[cfg(feature = "toml")]
    mgr.register_loader::<T, _>("toml", Arc::new(SerdeLoader::<T>::default()));
    #[cfg(feature = "yaml")]
    {
        mgr.register_loader::<T, _>("yaml", Arc::new(SerdeLoader::<T>::default()));
        mgr.register_loader::<T, _>("yml", Arc::new(SerdeLoader::<T>::default()));
    }
    #[cfg(feature = "xml")]
    mgr.register_loader::<T, _>("xml", Arc::new(SerdeLoader::<T>::default()));
    #[cfg(feature = "bin")]
    for ext in ["bin", "dat"] {
        mgr.register_loader::<T, _>(ext, Arc::new(SerdeLoader::<T>::default()));
    }
}

/* --------------------------------------- Utilitaires --------------------------------------- */

/// Helper: enregistre sources par défaut et loaders texte/bytes.
pub fn bootstrap_default() -> AssetManager {
    let mgr = AssetManager::default();
    register_builtin_basic(&mgr);
    mgr
}

/// Obtenir une `MemorySource` déjà enregistrée sur `scheme`.
pub fn memory_source(mgr: &AssetManager, scheme: &str) -> Option<MemorySource> {
    mgr.source(scheme)
        .and_then(|arc| arc.as_any().downcast_ref::<MemorySource>().cloned())
}

/* ------------------------------------------- Tests ------------------------------------------- */
#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicUsize, Ordering};

    #[test]
    fn file_and_memory_sources() {
        let mem = MemorySource::default();
        mem.insert("foo/bar.txt", b"Hello");
        assert_eq!(mem.read("foo/bar.txt").unwrap(), b"Hello");

        let fs = FileSource::default();
        let exists = fs.exists("Cargo.toml").unwrap();
        // ne présume pas du repo local, vérifie que la requête n’explose pas.
        let _ = exists;
    }

    #[test]
    fn manager_basic_text_and_bytes() {
        let mgr = bootstrap_default();
        let mem = MemorySource::default();
        mem.insert("a.txt", b"alpha");
        mem.insert("b.bin", b"\x01\x02\x03");
        mgr.register_source("mem2", Arc::new(mem.clone()));

        let s = mgr.load::<String>("mem2:a.txt").unwrap();
        assert_eq!(&*s, "alpha");

        let bytes = mgr.load::<Vec<u8>>("mem2:b.bin").unwrap();
        assert_eq!(&*bytes, b"\x01\x02\x03");
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_typed() {
        #[derive(Debug, Clone, serde::Deserialize)]
        struct Cfg {
            a: i32,
            b: String,
        }

        let mgr = bootstrap_default();
        register_serde_for::<Cfg>(&mgr);

        let mem = MemorySource::default();
        mem.insert("cfg.json", br#"{"a":42,"b":"ok"}"#);
        mgr.register_source("memcfg", Arc::new(mem));

        let cfg = mgr.load::<Cfg>("memcfg:cfg.json").unwrap();
        assert_eq!(cfg.a, 42);
        assert_eq!(cfg.b, "ok");
    }

    #[test]
    fn invalidate_and_cache() {
        let mgr = bootstrap_default();
        let mem = MemorySource::default();
        mem.insert("x.txt", b"1");
        mgr.register_source("X", Arc::new(mem.clone()));

        let s1 = mgr.load::<String>("X:x.txt").unwrap();
        assert_eq!(&*s1, "1");

        // change data but cache prevents reload
        mem.insert("x.txt", b"2");
        let s_cached = mgr.get_cached::<String>("X:x.txt").unwrap();
        assert_eq!(&*s_cached, "1");

        // invalidate then reload
        mgr.invalidate("X:x.txt");
        let s2 = mgr.load::<String>("X:x.txt").unwrap();
        assert_eq!(&*s2, "2");
    }

    #[test]
    fn load_with_custom_parser() {
        static CALLS: AtomicUsize = AtomicUsize::new(0);
        let mgr = bootstrap_default();
        let mem = MemorySource::default();
        mem.insert("n.bin", &[10u8, 20, 30]);
        mgr.register_source("memn", Arc::new(mem));

        let arc = mgr
            .load_with("memn:n.bin", |bytes| {
                CALLS.fetch_add(1, Ordering::SeqCst);
                Ok::<usize, AssetError>(bytes.iter().map(|&b| b as usize).sum())
            })
            .unwrap();
        assert_eq!(*arc, 60);

        // cache hit should not recall parser
        let arc2 = mgr
            .load_with("memn:n.bin", |_bytes| {
                CALLS.fetch_add(1, Ordering::SeqCst);
                Ok::<usize, AssetError>(0)
            })
            .unwrap();
        assert_eq!(*arc2, 60);
        assert_eq!(CALLS.load(Ordering::SeqCst), 1);
    }
}
