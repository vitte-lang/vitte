#![deny(missing_docs)]
//! vitte-loader — chargeur dynamique et gestionnaire de modules pour Vitte.
//!
//! Objectifs:
//! - Chargement cross‑platform (.so/.dylib/.dll) avec résolution de noms portable.
//! - Récupération de symboles typés, avec enveloppe sûre.
//! - Convention d’ABI minimaliste pour plugins Vitte (init, desc, shutdown).
//! - Cache léger, métriques (heure de chargement), et helpers de recherche.
//! - API sans `unsafe` en surface pour les usages courants. Les appels aux
//!   symboles restent `unsafe` par nature et sont cloisonnés.
//!
//! Limitations:
//! - `libloading::Library` n’est ni `Send` ni `Sync`. `Loader` non Send/Sync.
//! - L’unloading a lieu à `Drop`. Évitez de garder des symboles vivants après drop.
//!
//! Exemple basique:
//! ```no_run
//! use vitte_loader as vl;
//! let path = vl::resolve_name("myplugin", None);
//! let mut ld = vl::Loader::load(&path).expect("load");
//! // Symboles classiques
//! let sym: vl::Symbol<'_, unsafe extern "C" fn(i32) -> i32> = unsafe { ld.get("my_add") }.unwrap();
//! let out = unsafe { sym(40) }; assert_eq!(out, 42);
//! // Plugin ABI
//! let desc = unsafe { ld.load_plugin_desc().unwrap() };
//! println!("plugin {} v{}", desc.name, desc.version);
//! ```

use std::ffi::{CStr, CString, OsStr};
use std::fmt;
use std::path::{Path, PathBuf};
use std::time::{Duration, Instant};

use libloading::{Library, Symbol as LibSym};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Erreurs du chargeur.
#[derive(Debug)]
pub enum LoaderError {
    /// Fichier introuvable.
    NotFound(PathBuf),
    /// Échec de chargement dynamique.
    Load(String),
    /// Symbole manquant.
    MissingSymbol(&'static str),
    /// Conversion de chaîne / encodage.
    Encoding(String),
    /// ABI plugin incompatible.
    BadAbi(&'static str),
}

impl fmt::Display for LoaderError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            LoaderError::NotFound(p) => write!(f, "not found: {}", p.display()),
            LoaderError::Load(e) => write!(f, "load error: {e}"),
            LoaderError::MissingSymbol(s) => write!(f, "missing symbol: {s}"),
            LoaderError::Encoding(e) => write!(f, "encoding: {e}"),
            LoaderError::BadAbi(e) => write!(f, "bad abi: {e}"),
        }
    }
}

impl std::error::Error for LoaderError {}

/// Wrapper de symbole typé retourné par le loader.
///
/// Note: appellez la fonction via `(*sym)(...)` ou coercion Fonction.
pub struct Symbol<'lib, T>(LibSym<'lib, T>);

impl<'lib, T> std::ops::Deref for Symbol<'lib, T> {
    type Target = T;
    fn deref(&self) -> &Self::Target { &*self.0 }
}

/// Métadonnées sur une bibliothèque chargée.
#[derive(Debug, Clone)]
pub struct LibraryInfo {
    /// Chemin absolu résolu.
    pub path: PathBuf,
    /// Instant de chargement.
    pub loaded_at: Instant,
    /// Durée entre `new` et chargement effectif.
    pub load_time: Duration,
}

/// Chargeur d’une bibliothèque partagée. Non `Send`/`Sync`.
pub struct Loader {
    lib: Library,
    info: LibraryInfo,
}

impl Loader {
    /// Charge une librairie depuis `path`.
    pub fn load<P: AsRef<Path>>(path: P) -> Result<Self, LoaderError> {
        let start = Instant::now();
        let path = path.as_ref().to_path_buf();
        if !path.exists() {
            return Err(LoaderError::NotFound(path));
        }
        // Safety: libloading gère le comptage de refs OS. Invariant: on garde `lib` en vie.
        let lib = unsafe { Library::new(&path).map_err(|e| LoaderError::Load(e.to_string()))? };
        let info = LibraryInfo { path, loaded_at: Instant::now(), load_time: start.elapsed() };
        Ok(Self { lib, info })
    }

    /// Infos de chargement.
    pub fn info(&self) -> &LibraryInfo { &self.info }

    /// Récupère un symbole par nom, typé. Appel `unsafe` requis côté appelant pour l’utiliser.
    pub unsafe fn get<T>(&self, name: &str) -> Result<Symbol<'_, T>, LoaderError> {
        let c = CString::new(name).map_err(|e| LoaderError::Encoding(e.to_string()))?;
        let s: LibSym<'_, T> = self.lib.get(c.as_bytes_with_nul()).map_err(|_| LoaderError::MissingSymbol(Box::leak(name.to_owned().into_boxed_str())))?;
        Ok(Symbol(s))
    }

    /// Essaie de récupérer plusieurs symboles d’un coup. Renvoie la première erreur.
    pub unsafe fn get_many<T: Copy, const N: usize>(&self, names: [&str; N]) -> Result<[Symbol<'_, T>; N], LoaderError> {
        let mut out: [std::mem::MaybeUninit<Symbol<'_, T>>; N] = unsafe { std::mem::MaybeUninit::uninit().assume_init() };
        for (i, n) in names.iter().enumerate() {
            out[i].write(self.get::<T>(n)?);
        }
        // Safety: tout écrit ou on a quitté avant.
        Ok(unsafe { std::mem::transmute_copy::<_, [Symbol<'_, T>; N]>(&out) })
    }

    /// Charge les descripteurs de plugin selon l’ABI Vitte.
    ///
    /// Requiert que la lib exporte:
    /// - `vitte_plugin_desc() -> *const VittePluginDesc`
    pub unsafe fn load_plugin_desc(&mut self) -> Result<VittePluginDesc, LoaderError> {
        let sym: LibSym<unsafe extern "C" fn() -> *const VittePluginDesc> =
            self.lib.get(b"vitte_plugin_desc\0").map_err(|_| LoaderError::MissingSymbol("vitte_plugin_desc"))?;
        let ptr = sym();
        if ptr.is_null() { return Err(LoaderError::BadAbi("desc null")); }
        Ok((*ptr).clone())
    }

    /// Appelle la routine d'initialisation du plugin si fournie.
    ///
    /// Exige l’export `vitte_plugin_init() -> i32` qui renvoie 0 si OK.
    pub unsafe fn plugin_init(&mut self) -> Result<(), LoaderError> {
        if let Ok(sym) = self.lib.get::<unsafe extern "C" fn() -> i32>(b"vitte_plugin_init\0") {
            if sym() != 0 { return Err(LoaderError::BadAbi("init non‑zero")); }
        }
        Ok(())
    }

    /// Appelle la routine de shutdown si présente.
    pub unsafe fn plugin_shutdown(&mut self) {
        if let Ok(sym) = self.lib.get::<unsafe extern "C" fn()>(b"vitte_plugin_shutdown\0") {
            sym();
        }
    }
}

/// Décrit un plugin Vitte exporté par une bibliothèque.
#[repr(C)]
#[derive(Clone)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct VittePluginDesc {
    /// Nom logique du plugin.
    pub name: [u8; 64],
    /// Version sémantique "major.minor.patch".
    pub version: [u8; 16],
    /// Auteur ou vendor.
    pub vendor: [u8; 64],
    /// Description courte.
    pub description: [u8; 256],
}

impl VittePluginDesc {
    /// Crée un descripteur depuis champs &str. Tronque si dépassement.
    pub fn from_strs(name: &str, version: &str, vendor: &str, description: &str) -> Result<Self, LoaderError> {
        fn fill<const N: usize>(src: &str) -> Result<[u8; N], LoaderError> {
            let b = src.as_bytes();
            if b.contains(&0) { return Err(LoaderError::Encoding("NUL in string".into())); }
            let mut out = [0u8; N];
            let n = b.len().min(N - 1);
            out[..n].copy_from_slice(&b[..n]);
            Ok(out)
        }
        Ok(Self {
            name: fill::<64>(name)?, version: fill::<16>(version)?, vendor: fill::<64>(vendor)?, description: fill::<256>(description)?,
        })
    }

    /// Accès nom en &str sans allocations.
    pub fn name_str(&self) -> &str { cstr_from_arr(&self.name) }
    /// Accès version.
    pub fn version_str(&self) -> &str { cstr_from_arr(&self.version) }
    /// Accès vendor.
    pub fn vendor_str(&self) -> &str { cstr_from_arr(&self.vendor) }
    /// Accès description.
    pub fn description_str(&self) -> &str { cstr_from_arr(&self.description) }
}

fn cstr_from_arr(buf: &[u8]) -> &str {
    let n = buf.iter().position(|&b| b == 0).unwrap_or(buf.len());
    std::str::from_utf8(&buf[..n]).unwrap_or("")
}

/// Conventions de nommage cross‑platform.
#[derive(Clone, Copy, Debug)]
pub enum LibNaming {
    /// Respecter exactement le chemin fourni.
    Exact,
    /// Appliquer conventions OS (lib + ext).
    Conventional,
}

/// Résout un nom logique de bibliothèque en chemin plausible.
/// `base_dir` si fourni est préfixé.
///
/// - Linux/Unix: lib{name}.so
/// - macOS:      lib{name}.dylib
/// - Windows:    {name}.dll
pub fn resolve_name<S: AsRef<str>>(name: S, base_dir: Option<&Path>) -> PathBuf {
    let name = name.as_ref();
    let filename = if cfg!(target_os = "windows") {
        format!("{name}.dll")
    } else if cfg!(target_os = "macos") {
        format!("lib{name}.dylib")
    } else {
        format!("lib{name}.so")
    };
    match base_dir {
        Some(dir) => dir.join(filename),
        None => PathBuf::from(filename),
    }
}

/// Recherche une bibliothèque par nom dans plusieurs dossiers.
pub fn find_in_dirs<S: AsRef<str>, P: AsRef<Path>>(name: S, dirs: &[P]) -> Option<PathBuf> {
    let rel = resolve_name(name, None);
    for d in dirs {
        let p = d.as_ref().join(&rel);
        if p.exists() { return Some(p); }
    }
    None
}

/// Charge par nom logique en cherchant dans `dirs`, sinon dans le cwd.
pub fn load_from_dirs<S: AsRef<str>, P: AsRef<Path>>(name: S, dirs: &[P]) -> Result<Loader, LoaderError> {
    if let Some(p) = find_in_dirs(&name, dirs) {
        Loader::load(p)
    } else {
        Loader::load(resolve_name(name, None))
    }
}

/// Macro utilitaire côté plugin pour exporter les symboles requis.
///
/// Usage dans le crate du plugin:
/// ```ignore
/// use vitte_loader::{export_vitte_plugin, VittePluginDesc};
/// export_vitte_plugin!(VittePluginDesc::from_strs("demo","0.1.0","acme","example").unwrap());
/// ```
#[macro_export]
macro_rules! export_vitte_plugin {
    ($desc:expr) => {
        #[no_mangle]
        pub extern "C" fn vitte_plugin_desc() -> *const $crate::VittePluginDesc {
            static DESC: $crate::VittePluginDesc = $desc;
            &DESC as *const _
        }
        #[no_mangle]
        pub extern "C" fn vitte_plugin_init() -> i32 { 0 }
        #[no_mangle]
        pub extern "C" fn vitte_plugin_shutdown() {}
    };
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn desc_roundtrip_and_strs() {
        let d = VittePluginDesc::from_strs("demo", "1.2.3", "acme", "plugin de test").unwrap();
        assert_eq!(d.name_str(), "demo");
        assert_eq!(d.version_str(), "1.2.3");
        assert_eq!(d.vendor_str(), "acme");
        assert!(d.description_str().starts_with("plugin"));
    }

    #[test]
    fn resolve_extensions() {
        let p = resolve_name("hello", None);
        let s = p.file_name().unwrap().to_string_lossy().to_string();
        if cfg!(target_os = "windows") {
            assert!(s.ends_with(".dll"));
        } else if cfg!(target_os = "macos") {
            assert!(s.ends_with(".dylib") && s.starts_with("lib"));
        } else {
            assert!(s.ends_with(".so") && s.starts_with("lib"));
        }
    }
}
