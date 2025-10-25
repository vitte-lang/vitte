//! vitte-traits — traits fondamentaux et stables pour l'écosystème Vitte
//!
//! Ce crate expose des **interfaces minimales** et sans opinion forte pour:
//! - accès registre/manifests (résolution)
//! - stockage d'artefacts (cache C.A.S.)
//! - abstractions système (FS léger, horloge)
//! - journalisation et progression
//!
//! Conçu pour rester `no_std` (avec `alloc`) si nécessaire.

#![forbid(unsafe_code)]
#![deny(rust_2018_idioms, missing_docs, unreachable_pub)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
extern crate alloc;

use core::fmt;
#[cfg(feature = "std")] use std::borrow::Cow;
#[cfg(not(feature = "std"))] use alloc::borrow::Cow;
#[cfg(feature = "std")] use std::string::String;
#[cfg(not(feature = "std"))] use alloc::string::String;
#[cfg(feature = "std")] use std::vec::Vec;
#[cfg(not(feature = "std"))] use alloc::vec::Vec;

/// Re-export volontaire pour signatures de résolution.
pub use semver::{Version, VersionReq};

// ------------------------------- Types --------------------------------------

/// Octets possiblement empruntés.
pub type Bytes = Vec<u8>;

/// Identité canonique d'un paquet.
#[derive(Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Debug)]
pub struct PackageId {
    /// Nom canonique (sans espace, sensible à la casse selon l'implémentation).
    pub name: String,
    /// Version concrète (SemVer).
    pub version: Version,
}

impl PackageId { 
    /// Construction utilitaire.
    pub fn new<N: Into<String>>(name: N, version: Version) -> Self { Self { name: name.into(), version } }
}

impl fmt::Display for PackageId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { write!(f, "{}@{}", self.name, self.version) }
}

/// Dépendance déclarée.
#[derive(Clone, Debug)]
pub struct Dependency {
    /// Nom du paquet requis.
    pub name: String,
    /// Contrainte SemVer.
    pub req: VersionReq,
    /// Dépendance optionnelle.
    pub optional: bool,
    /// Ensemble de features demandées (opaque pour les résolveurs).
    pub features: Vec<String>,
}

impl Dependency {
    /// Dépendance obligatoire.
    pub fn required<N: Into<String>>(name: N, req: VersionReq) -> Self {
        Self { name: name.into(), req, optional: false, features: Vec::new() }
    }
}

/// Manifest minimal lisible par les outils Vitte.
#[derive(Clone, Debug)]
pub struct Manifest {
    /// Identité du paquet.
    pub id: PackageId,
    /// Dépendances directes.
    pub dependencies: Vec<Dependency>,
}

impl Manifest { 
    /// Construction utilitaire.
    pub fn new(id: PackageId, dependencies: Vec<Dependency>) -> Self { Self { id, dependencies } }
}

// ------------------------------ Erreurs -------------------------------------

/// Erreur légère et portable.
#[derive(Debug)]
pub enum VitteError {
    /// Ressource introuvable.
    NotFound(Cow<'static, str>),
    /// Conflit logique ou contrainte impossible.
    Conflict(Cow<'static, str>),
    /// Entrée invalide.
    Invalid(Cow<'static, str>),
    /// Erreur d'E/S ou d'environnement.
    Io(Cow<'static, str>),
    /// Boîte de transport pour erreurs spécifiques d'implémentation.
    Other(Cow<'static, str>),
}

impl fmt::Display for VitteError { fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { write!(f, "{:?}", self) } }
#[cfg(feature = "std")]
impl std::error::Error for VitteError {}

pub type Result<T, E = VitteError> = core::result::Result<T, E>;

// ------------------------------ Registre ------------------------------------

/// Accès abstrait aux versions publiées et aux manifests.
pub trait RegistryProvider {
    /// Liste toutes les versions publiées pour `name`.
    fn available_versions(&self, name: &str) -> Result<Vec<Version>>;
    /// Charge le manifest d'une version précise.
    fn manifest(&self, name: &str, version: &Version) -> Result<Manifest>;
}

// ------------------------------- Cache --------------------------------------

/// Clé de contenu pour stockage adressé par le contenu (CAS).
#[derive(Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Debug)]
pub struct Digest(pub [u8; 32]);

impl Digest {
    /// Hex minuscule.
    pub fn to_hex(&self) -> String { 
        let mut s = String::with_capacity(64);
        for b in &self.0 { use core::fmt::Write as _; let _ = write!(&mut s, "{:02x}", b); }
        s
    }
}

/// Calcul de hachage portable.
pub trait Hasher {
    /// SHA-256 d'un buffer complet.
    fn sha256(&self, data: &[u8]) -> Digest;
}

/// Stockage d'artefacts binaire en CAS.
pub trait ArtifactStore {
    /// Récupère des octets via `digest` si présents.
    fn get(&self, digest: &Digest) -> Result<Option<Bytes>>;
    /// Enregistre des octets et retourne leur digest.
    fn put(&self, data: &[u8]) -> Result<Digest>;
}

// ------------------------------- Système ------------------------------------

/// FS minimaliste utilisé par les outils (remplaçable pour tests/sandbox).
pub trait Fs {
    /// Lit un fichier complet en mémoire.
    fn read(&self, path: &str) -> Result<Bytes>;
    /// Écrit un fichier, crée les dossiers si nécessaire.
    fn write(&self, path: &str, data: &[u8]) -> Result<()>;
    /// Retourne vrai si le chemin existe.
    fn exists(&self, path: &str) -> bool;
}

/// Horloge simple.
pub trait Clock { fn now_unix_ms(&self) -> u128; }

// ------------------------------ Journalisation ------------------------------

/// Niveaux de log.
#[derive(Clone, Copy, Debug, Eq, PartialEq, Ord, PartialOrd)]
pub enum Level { Trace, Debug, Info, Warn, Error }

/// Journalisation minimaliste, sans macros pour rester agnostique.
pub trait Logger {
    /// Envoie un message avec un niveau.
    fn log(&self, level: Level, msg: &str);
}

/// Rapport de progression simple.
pub trait ProgressReporter {
    /// Démarre un item de progression. Retourne un identifiant opaque.
    fn begin(&self, name: &str, total: Option<u64>) -> u64;
    /// Met à jour la position.
    fn set(&self, id: u64, pos: u64);
    /// Termine avec succès.
    fn finish(&self, id: u64);
    /// Termine avec échec.
    fn fail(&self, id: u64, err: &str);
}

// ------------------------------- Prélude ------------------------------------

/// Prélude pratique pour import glob.
pub mod prelude {
    pub use super::{
        ArtifactStore, Clock, Dependency, Digest, Fs, Hasher, Level, Logger, Manifest, PackageId,
        ProgressReporter, RegistryProvider, Result, Version, VersionReq,
    };
}
