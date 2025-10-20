#![cfg_attr(not(feature = "std"), no_std)]
//! vitte-hal — **Hardware/Host Abstraction Layer** pour Vitte
//!
//! But : fournir une couche d'abstraction **portable, testable, et modulaire**
//! pour l'environnement d'exécution (fichiers, horloge, aléa, console, réseau, process…).
//!
//! Design :
//! - Des **traits fins** (\[Fs\], \[Clock\], \[Entropy\], \[Console\], \[Net\], \[Process\])
//! - Un conteneur \[Hal\] composable (avec builder) + un **singleton global** optionnel
//! - Implémentations de base : \[StdFs\], \[StdClock\], \[StdConsole\], \[MemFs\] (RAM), \[NullFs\]
//! - `no_std` friendly (par défaut `std` activé) et `serde` optionnelle
//! - Modules optionnels : `net`, `process` — laissés derrière des features
//!
//! # Exemple rapide
//! ```ignore
//! use vitte_hal as hal;
//! let hal = hal::Hal::builder()
//!     .with_std_clock()
//!     .with_std_console()
//!     .with_std_fs()
//!     .build();
//! let now = hal.clock.now_mono();
//! hal.console.eprintln(&format!("hello @ {now:?}"));
//! ```

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{borrow::ToOwned, boxed::Box, format, string::String, vec::Vec};
#[cfg(feature = "std")]
use std::{boxed::Box, string::String, vec::Vec};

use core::{fmt, time::Duration};

// Re-exports utiles
pub use clock::*;
pub use console::*;
pub use entropy::*;
pub use fs::*;
#[cfg(feature = "net")]
pub use net::*;
#[cfg(feature = "process")]
pub use process::*;

// ============================== Erreurs ==================================

#[derive(Debug, Clone)]
pub enum HalError {
    Unsupported(&'static str),
    Invalid(&'static str),
    Timeout,
    Io(String),
    Fs(FsError),
    #[cfg(feature = "net")]
    Net(String),
}

impl fmt::Display for HalError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            HalError::Unsupported(s) => write!(f, "non supporté: {s}"),
            HalError::Invalid(s) => write!(f, "invalide: {s}"),
            HalError::Timeout => write!(f, "timeout"),
            HalError::Io(s) => write!(f, "io: {s}"),
            HalError::Fs(e) => write!(f, "fs: {e}"),
            #[cfg(feature = "net")]
            HalError::Net(s) => write!(f, "net: {s}"),
        }
    }
}

pub type Result<T> = core::result::Result<T, HalError>;

// ============================ Conteneur HAL ===============================

/// Ensemble de services d’environnement.
pub struct Hal {
    pub clock: Box<dyn Clock>,
    pub console: Box<dyn Console>,
    pub fs: Box<dyn Fs>,
    #[cfg(feature = "net")]
    pub net: Box<dyn Net>,
    #[cfg(feature = "process")]
    pub process: Box<dyn Process>,
    pub entropy: Box<dyn Entropy>,
}

impl Hal {
    pub fn builder() -> HalBuilder {
        HalBuilder::default()
    }

    /// (Optionnel) Accès à un HAL global initialisé via \[set_global\].
    #[cfg(feature = "std")]
    pub fn global() -> &'static Hal {
        GLOBAL.get().expect("Hal global non initialisé")
    }
}

/// Builder fluide pour composer un HAL.
#[derive(Default)]
pub struct HalBuilder {
    clock: Option<Box<dyn Clock>>,
    console: Option<Box<dyn Console>>,
    fs: Option<Box<dyn Fs>>,
    #[cfg(feature = "net")]
    net: Option<Box<dyn Net>>,
    #[cfg(feature = "process")]
    process: Option<Box<dyn Process>>,
    entropy: Option<Box<dyn Entropy>>,
}

impl HalBuilder {
    pub fn with_clock(mut self, c: impl Clock + 'static) -> Self {
        self.clock = Some(Box::new(c));
        self
    }
    pub fn with_console(mut self, c: impl Console + 'static) -> Self {
        self.console = Some(Box::new(c));
        self
    }
    pub fn with_fs(mut self, f: impl Fs + 'static) -> Self {
        self.fs = Some(Box::new(f));
        self
    }
    pub fn with_entropy(mut self, e: impl Entropy + 'static) -> Self {
        self.entropy = Some(Box::new(e));
        self
    }

    #[cfg(feature = "net")]
    pub fn with_net(mut self, n: impl Net + 'static) -> Self {
        self.net = Some(Box::new(n));
        self
    }
    #[cfg(feature = "process")]
    pub fn with_process(mut self, p: impl Process + 'static) -> Self {
        self.process = Some(Box::new(p));
        self
    }

    /// Ajoute les implémentations basées sur `std` (horloge/console/fs/entropy).
    #[cfg(feature = "std")]
    pub fn with_std(mut self) -> Self {
        if self.clock.is_none() {
            self.clock = Some(Box::new(StdClock));
        }
        if self.console.is_none() {
            self.console = Some(Box::new(StdConsole));
        }
        if self.fs.is_none() {
            self.fs = Some(Box::new(StdFs));
        }
        if self.entropy.is_none() {
            self.entropy = Some(Box::new(FastEntropy::seeded_auto()));
        }
        self
    }

    #[cfg(feature = "std")]
    pub fn with_std_clock(mut self) -> Self {
        self.clock = Some(Box::new(StdClock));
        self
    }
    #[cfg(feature = "std")]
    pub fn with_std_console(mut self) -> Self {
        self.console = Some(Box::new(StdConsole));
        self
    }
    #[cfg(feature = "std")]
    pub fn with_std_fs(mut self) -> Self {
        self.fs = Some(Box::new(StdFs));
        self
    }

    pub fn with_mem_fs(mut self) -> Self {
        self.fs = Some(Box::new(MemFs::default()));
        self
    }

    /// Construit le HAL — fallback sur impls minimales si manquantes.
    pub fn build(self) -> Hal {
        Hal {
            clock: self.clock.unwrap_or_else(|| Box::new(FixedClock::now_zero())),
            console: self.console.unwrap_or_else(|| Box::new(NullConsole)),
            fs: self.fs.unwrap_or_else(|| Box::new(NullFs)),
            #[cfg(feature = "net")]
            net: self.net.unwrap_or_else(|| Box::new(NullNet)),
            #[cfg(feature = "process")]
            process: self.process.unwrap_or_else(|| Box::new(NullProcess)),
            entropy: self
                .entropy
                .unwrap_or_else(|| Box::new(FastEntropy::seeded(0xC0FEBABE_D15EA5E))),
        }
    }
}

/// Enregistre un HAL global accessible via \[Hal::global\]. Non réinitialisable.
#[cfg(feature = "std")]
pub fn set_global(hal: Hal) -> Result<()> {
    GLOBAL.set(hal).map_err(|_| HalError::Invalid("HAL global déjà initialisé"))
}

#[cfg(feature = "std")]
static GLOBAL: std::sync::OnceLock<Hal> = std::sync::OnceLock::new();

// =============================== CLOCK ===================================

mod clock {
    use super::*;

    /// Horloge monotone + utilitaires de temporisation.
    pub trait Clock: Send + Sync {
        /// Timestamp monotone arbitraire (nanosecondes depuis un point fixe).
        fn now_mono(&self) -> u128;
        /// Attente bloquante.
        fn sleep(&self, _d: Duration) {
            let _ = _d; /* par défaut: no-op */
        }
    }

    /// Horloge fixe (tests/déterminisme).
    #[derive(Clone, Copy)]
    pub struct FixedClock {
        base: u128,
    }
    impl FixedClock {
        pub fn now_zero() -> Self {
            Self { base: 0 }
        }
    }
    impl Clock for FixedClock {
        fn now_mono(&self) -> u128 {
            self.base
        }
    }

    /// Impl `std`.
    #[cfg(feature = "std")]
    pub struct StdClock;
    #[cfg(feature = "std")]
    impl Clock for StdClock {
        fn now_mono(&self) -> u128 {
            std::time::Instant::now().elapsed().as_nanos()
        }
        fn sleep(&self, d: Duration) {
            std::thread::sleep(d);
        }
    }
}

// ============================== CONSOLE ==================================

mod console {
    use super::*;

    pub trait Console: Send + Sync {
        fn print(&self, s: &str);
        fn eprint(&self, s: &str);
        fn println(&self, s: &str) {
            self.print(s);
            self.print("\n");
        }
        fn eprintln(&self, s: &str) {
            self.eprint(s);
            self.eprint("\n");
        }
        /// Lecture *best-effort* d'une ligne (peut être non supportée).
        fn read_line(&self, _buf: &mut String) -> Result<()> {
            Err(HalError::Unsupported("read_line"))
        }
    }

    /// Console nulle (absorbe tout).
    pub struct NullConsole;
    impl Console for NullConsole {
        fn print(&self, _s: &str) {}
        fn eprint(&self, _s: &str) {}
    }

    /// Console stdio.
    #[cfg(feature = "std")]
    pub struct StdConsole;
    #[cfg(feature = "std")]
    impl Console for StdConsole {
        fn print(&self, s: &str) {
            let _ = std::io::Write::write_all(&mut std::io::stdout(), s.as_bytes());
        }
        fn eprint(&self, s: &str) {
            let _ = std::io::Write::write_all(&mut std::io::stderr(), s.as_bytes());
        }
        fn read_line(&self, buf: &mut String) -> Result<()> {
            use std::io::{self, Read};
            buf.clear();
            let mut stdin = io::stdin();
            let mut tmp = String::new();
            stdin.read_to_string(&mut tmp).map_err(|e| HalError::Io(e.to_string()))?;
            // ne lit qu'une ligne
            if let Some((line, _)) = tmp.split_once('\n') {
                buf.push_str(line);
            } else {
                buf.push_str(&tmp);
            }
            Ok(())
        }
    }
}

// ============================== ENTROPY ==================================

mod entropy {
    use super::*;

    /// Source d'aléa.
    pub trait Entropy: Send + Sync {
        /// Remplit le buffer (non nécessairement cryptographiquement sûr).
        fn fill(&self, buf: &mut [u8]) -> Result<()>;
        /// Un `u64` pratique.
        fn next_u64(&self) -> u64 {
            let mut b = [0u8; 8];
            let _ = self.fill(&mut b);
            u64::from_le_bytes(b)
        }
    }

    /// XorShift64* très rapide (pas CSPRNG) — utile pour tests/seed rapide.
    #[derive(Clone, Copy)]
    pub struct FastEntropy {
        state: u64,
    }
    impl FastEntropy {
        pub fn seeded(seed: u64) -> Self {
            Self { state: seed.max(1) }
        }
        #[cfg(feature = "std")]
        pub fn seeded_auto() -> Self {
            let t = std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .map(|d| d.as_nanos())
                .unwrap_or(0) as u64;
            let a = (&t as *const u64 as usize as u64) ^ t.rotate_left(13);
            Self::seeded(0x9E3779B97F4A7C15u64 ^ a)
        }
        fn step(&mut self) -> u64 {
            let mut x = self.state;
            x ^= x >> 12;
            x ^= x << 25;
            x ^= x >> 27;
            self.state = x;
            x.wrapping_mul(0x2545F4914F6CDD1D)
        }
    }
    impl Entropy for FastEntropy {
        fn fill(&self, buf: &mut [u8]) -> Result<()> {
            let mut s = *self; // copie
            let mut i = 0;
            while i < buf.len() {
                let r = s.step();
                let n = core::cmp::min(8, buf.len() - i);
                buf[i..i + n].copy_from_slice(&r.to_le_bytes()[..n]);
                i += n;
            }
            Ok(())
        }
    }

    /// Entropy via `getrandom` si disponible.
    #[cfg(feature = "getrandom")]
    pub struct SysEntropy;
    #[cfg(feature = "getrandom")]
    impl Entropy for SysEntropy {
        fn fill(&self, buf: &mut [u8]) -> Result<()> {
            getrandom::getrandom(buf).map_err(|e| HalError::Io(e.to_string()))
        }
    }
}

// ================================ FS =====================================

mod fs {
    use super::*;
    #[cfg(not(feature = "std"))]
    use alloc::collections::BTreeMap;
    #[cfg(feature = "std")]
    use std::collections::BTreeMap;

    #[derive(Debug, Clone)]
    pub enum FsError {
        NotFound,
        AlreadyExists,
        Denied,
        Io(String),
    }
    impl fmt::Display for FsError {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            match self {
                Self::NotFound => write!(f, "not found"),
                Self::AlreadyExists => write!(f, "exists"),
                Self::Denied => write!(f, "denied"),
                Self::Io(s) => write!(f, "io:{s}"),
            }
        }
    }

    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub struct FileMode {
        pub read: bool,
        pub write: bool,
        pub create: bool,
        pub truncate: bool,
        pub append: bool,
    }
    impl FileMode {
        pub const READ: Self =
            Self { read: true, write: false, create: false, truncate: false, append: false };
        pub const WRITE: Self =
            Self { read: false, write: true, create: true, truncate: true, append: false };
    }

    /// Métadonnées minimalistes.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    pub struct Metadata {
        pub is_dir: bool,
        pub len: u64,
    }

    #[derive(Debug, Clone)]
    pub struct DirEntry {
        pub name: String,
        pub meta: Metadata,
    }

    pub trait File: Send {
        fn read(&mut self, out: &mut Vec<u8>) -> core::result::Result<usize, FsError>;
        fn write(&mut self, data: &[u8]) -> core::result::Result<usize, FsError>;
        fn flush(&mut self) -> core::result::Result<(), FsError>;
    }

    pub trait Fs: Send + Sync {
        fn read(&self, path: &str) -> core::result::Result<Vec<u8>, FsError>;
        fn write(
            &self,
            path: &str,
            data: &[u8],
            create_dirs: bool,
        ) -> core::result::Result<(), FsError>;
        fn metadata(&self, path: &str) -> core::result::Result<Metadata, FsError>;
        fn remove_file(&self, path: &str) -> core::result::Result<(), FsError>;
        fn create_dir_all(&self, path: &str) -> core::result::Result<(), FsError>;
        fn read_dir(&self, path: &str) -> core::result::Result<Vec<DirEntry>, FsError>;
        fn open(
            &self,
            _path: &str,
            _mode: FileMode,
        ) -> core::result::Result<Box<dyn File>, FsError> {
            Err(FsError::Io("open: not implemented".to_owned()))
        }
    }

    /// FS nul (tout échoue poliment).
    pub struct NullFs;
    impl Fs for NullFs {
        fn read(&self, _: &str) -> core::result::Result<Vec<u8>, FsError> {
            Err(FsError::NotFound)
        }
        fn write(&self, _: &str, _: &[u8], _: bool) -> core::result::Result<(), FsError> {
            Err(FsError::Denied)
        }
        fn metadata(&self, _: &str) -> core::result::Result<Metadata, FsError> {
            Err(FsError::NotFound)
        }
        fn remove_file(&self, _: &str) -> core::result::Result<(), FsError> {
            Err(FsError::NotFound)
        }
        fn create_dir_all(&self, _: &str) -> core::result::Result<(), FsError> {
            Ok(())
        }
        fn read_dir(&self, _: &str) -> core::result::Result<Vec<DirEntry>, FsError> {
            Ok(Vec::new())
        }
    }

    /// FS en mémoire pour tests.
    #[derive(Default)]
    pub struct MemFs {
        files: BTreeMap<String, Vec<u8>>,
    }
    impl MemFs {
        fn key(p: &str) -> String {
            p.replace('\\', "/")
        }
    }
    impl Fs for MemFs {
        fn read(&self, path: &str) -> core::result::Result<Vec<u8>, FsError> {
            self.files.get(&Self::key(path)).cloned().ok_or(FsError::NotFound)
        }
        fn write(
            &self,
            path: &str,
            data: &[u8],
            _create_dirs: bool,
        ) -> core::result::Result<(), FsError> {
            let k = Self::key(path);
            let mut v = Vec::with_capacity(data.len());
            v.extend_from_slice(data);
            let _ = self as *const _; // appease Send/Sync
                                      // safety: interior mut via shadow map (simple approach):
            #[cfg(feature = "std")]
            {
                use std::sync::Mutex;
                static MAP: once_cell::sync::Lazy<Mutex<BTreeMap<String, Vec<u8>>>> =
                    once_cell::sync::Lazy::new(|| Mutex::new(BTreeMap::new()));
                let mut m = MAP.lock().unwrap();
                m.insert(k, v);
                return Ok(());
            }
            #[cfg(not(feature = "std"))]
            {
                // en no_std on ne persiste pas entre instances (pas de global mut). On fail-soft.
                return Ok(());
            }
        }
        fn metadata(&self, path: &str) -> core::result::Result<Metadata, FsError> {
            match self.files.get(&Self::key(path)) {
                Some(v) => Ok(Metadata { is_dir: false, len: v.len() as u64 }),
                None => Err(FsError::NotFound),
            }
        }
        fn remove_file(&self, path: &str) -> core::result::Result<(), FsError> {
            let k = Self::key(path);
            if self.files.contains_key(&k) {
                #[cfg(feature = "std")]
                {
                    use std::sync::Mutex;
                    static MAP: once_cell::sync::Lazy<Mutex<BTreeMap<String, Vec<u8>>>> =
                        once_cell::sync::Lazy::new(|| Mutex::new(BTreeMap::new()));
                    let mut m = MAP.lock().unwrap();
                    m.remove(&k);
                }
                return Ok(());
            }
            Err(FsError::NotFound)
        }
        fn create_dir_all(&self, _: &str) -> core::result::Result<(), FsError> {
            Ok(())
        }
        fn read_dir(&self, prefix: &str) -> core::result::Result<Vec<DirEntry>, FsError> {
            let p = Self::key(prefix);
            let mut v = Vec::new();
            for (k, val) in self.files.iter() {
                if k.starts_with(&p) {
                    v.push(DirEntry {
                        name: k.clone(),
                        meta: Metadata { is_dir: false, len: val.len() as u64 },
                    });
                }
            }
            Ok(v)
        }
    }

    /// FS basé sur `std`.
    #[cfg(feature = "std")]
    pub struct StdFs;
    #[cfg(feature = "std")]
    impl Fs for StdFs {
        fn read(&self, path: &str) -> core::result::Result<Vec<u8>, FsError> {
            std::fs::read(path).map_err(|e| FsError::Io(e.to_string()))
        }
        fn write(
            &self,
            path: &str,
            data: &[u8],
            create_dirs: bool,
        ) -> core::result::Result<(), FsError> {
            use std::io::Write;
            if create_dirs {
                if let Some(p) = std::path::Path::new(path).parent() {
                    let _ = std::fs::create_dir_all(p);
                }
            }
            let tmp = format!("{}.tmp{}", path, std::process::id());
            {
                let mut f = std::fs::File::create(&tmp).map_err(|e| FsError::Io(e.to_string()))?;
                f.write_all(data).map_err(|e| FsError::Io(e.to_string()))?;
                f.flush().map_err(|e| FsError::Io(e.to_string()))?;
            }
            std::fs::rename(&tmp, path)
                .or_else(|_| {
                    std::fs::copy(&tmp, path)
                        .map(|_| ())
                        .and_then(|_| std::fs::remove_file(&tmp).map(|_| ()))
                })
                .map_err(|e| FsError::Io(e.to_string()))
        }
        fn metadata(&self, path: &str) -> core::result::Result<Metadata, FsError> {
            let m = std::fs::metadata(path).map_err(|e| FsError::Io(e.to_string()))?;
            Ok(Metadata { is_dir: m.is_dir(), len: m.len() })
        }
        fn remove_file(&self, path: &str) -> core::result::Result<(), FsError> {
            std::fs::remove_file(path).map_err(|e| FsError::Io(e.to_string()))
        }
        fn create_dir_all(&self, path: &str) -> core::result::Result<(), FsError> {
            std::fs::create_dir_all(path).map_err(|e| FsError::Io(e.to_string()))
        }
        fn read_dir(&self, path: &str) -> core::result::Result<Vec<DirEntry>, FsError> {
            let mut v = Vec::new();
            for e in std::fs::read_dir(path).map_err(|e| FsError::Io(e.to_string()))? {
                let e = e.map_err(|e| FsError::Io(e.to_string()))?;
                let name = e.path().to_string_lossy().to_string();
                let m = e.metadata().map_err(|e| FsError::Io(e.to_string()))?;
                v.push(DirEntry { name, meta: Metadata { is_dir: m.is_dir(), len: m.len() } });
            }
            Ok(v)
        }
        fn open(&self, path: &str, mode: FileMode) -> core::result::Result<Box<dyn File>, FsError> {
            let mut opts = std::fs::OpenOptions::new();
            opts.read(mode.read)
                .write(mode.write)
                .create(mode.create)
                .truncate(mode.truncate)
                .append(mode.append);
            let f = opts.open(path).map_err(|e| FsError::Io(e.to_string()))?;
            struct F(std::fs::File);
            impl File for F {
                fn read(&mut self, out: &mut Vec<u8>) -> core::result::Result<usize, FsError> {
                    use std::io::Read;
                    let n = self.0.read_to_end(out).map_err(|e| FsError::Io(e.to_string()))?;
                    Ok(n)
                }
                fn write(&mut self, data: &[u8]) -> core::result::Result<usize, FsError> {
                    use std::io::Write;
                    self.0.write(data).map_err(|e| FsError::Io(e.to_string()))
                }
                fn flush(&mut self) -> core::result::Result<(), FsError> {
                    use std::io::Write;
                    self.0.flush().map_err(|e| FsError::Io(e.to_string()))
                }
            }
            Ok(Box::new(F(f)))
        }
    }
}

// =============================== NET =====================================

#[cfg(feature = "net")]
mod net {
    use super::*;

    /// Réseau minimal.
    pub trait Net: Send + Sync {
        fn tcp_connect(
            &self,
            _host: &str,
            _port: u16,
            _timeout: Duration,
        ) -> Result<Box<dyn TcpStream>> {
            Err(HalError::Unsupported("tcp_connect"))
        }
    }

    pub trait TcpStream: Send {
        fn read(&mut self, _out: &mut [u8]) -> Result<usize> {
            Err(HalError::Unsupported("tcp.read"))
        }
        fn write(&mut self, _data: &[u8]) -> Result<usize> {
            Err(HalError::Unsupported("tcp.write"))
        }
        fn shutdown(&mut self) -> Result<()> {
            Ok(())
        }
    }

    /// Implémentation nulle.
    pub struct NullNet;
    impl Net for NullNet {}
}

// ============================== PROCESS ==================================

#[cfg(feature = "process")]
mod process {
    use super::*;

    pub trait Process: Send + Sync {
        fn spawn(&self, _prog: &str, _args: &[&str]) -> Result<i32> {
            Err(HalError::Unsupported("spawn"))
        }
        fn env_get(&self, _key: &str) -> Option<String> {
            None
        }
    }

    pub struct NullProcess;
    impl Process for NullProcess {}
}

// ================================ Tests ==================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn build_minimal_hal() {
        let hal = Hal::builder().with_mem_fs().build();
        let _t = hal.clock.now_mono();
        let _ = hal.console.println("ok minimal");
    }

    #[cfg(feature = "std")]
    #[test]
    fn std_hal_works() {
        let hal = Hal::builder().with_std().build();
        hal.console.println("hello");
        let _ = hal.fs.write("./target/test.tmp", b"x", true);
        let b = hal.fs.read("./target/test.tmp").unwrap();
        assert_eq!(b, b"x");
    }
}
