//! vitte-cross — cross-compilation utilities for Vitte
//!
//! Objectifs
//! - Détection de toolchains (CC/CXX/ar/ranlib/ld/strip) par cible.
//! - Résolution des triples, sysroots, et variables d’environnement Cargo.
//! - Exécution sous qemu ou dans un conteneur docker (optionnel).
//! - API simple: `detect()`, `env_for_cargo()`, `runner_for()`.
//!
//! Contraintes
//! - Requiert `std` pour l’I/O et les processus.
//! - Les intégrations qemu/docker sont activées par features.
//!
//! Exemple
//! ```no_run
//! use vitte_cross::*;
//! fn main() -> Result<()> {
//!     let target = TargetTriple::from_str("x86_64-unknown-linux-gnu")?;
//!     let tc = detect(&target)?;
//!     println!("cc: {}", tc.cc);
//!     let env = env_for_cargo(&target, &tc, None);
//!     for (k,v) in env { println!("{k}={v}"); }
//!     Ok(())
//! }
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]
#![forbid(unsafe_op_in_unsafe_fn)]

extern crate alloc;

use alloc::{borrow::ToOwned, string::String, vec, vec::Vec};

use camino::{Utf8Path, Utf8PathBuf};

#[cfg(feature = "std")]
use std::{env, fs, process::Command};

/// Résultat.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Erreurs.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Données invalides.
    #[error("invalid: {0}")]
    Invalid(String),
    /// Outil introuvable.
    #[error("tool not found: {0}")]
    NotFound(String),
    /// I/O.
    #[error("io: {0}")]
    Io(String),
    /// Processus a échoué.
    #[error("process `{tool}` failed: {code:?}\n{stderr}")]
    Proc {
        /// Outil.
        tool: String,
        /// Code retour.
        code: Option<i32>,
        /// Stderr tronqué.
        stderr: String,
    },
    /// Feature désactivée.
    #[error("unsupported: {0}")]
    Unsupported(&'static str),
}

/* ------------------------------- Target -------------------------------- */

/// Triple cible Rust (simplifié).
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct TargetTriple {
    /// Texte du triple (`aarch64-unknown-linux-gnu`, etc.)
    pub triple: String,
}

impl TargetTriple {
    /// Crée depuis un str.
    pub fn from_str(s: &str) -> Result<Self> {
        if s.trim().is_empty() {
            return Err(Error::Invalid("empty target".into()));
        }
        Ok(Self { triple: s.to_owned() })
    }
    /// Heuristiques OS.
    pub fn os(&self) -> &'static str {
        if self.triple.contains("windows") { "windows" }
        else if self.triple.contains("darwin") || self.triple.contains("apple") { "macos" }
        else if self.triple.contains("linux") { "linux" }
        else if self.triple.contains("wasi") { "wasi" }
        else { "unknown" }
    }
    /// Heuristique arch.
    pub fn arch(&self) -> &'static str {
        if self.triple.starts_with("x86_64") { "x86_64" }
        else if self.triple.starts_with("aarch64") { "aarch64" }
        else if self.triple.starts_with("arm") { "arm" }
        else if self.triple.starts_with("riscv64") { "riscv64" }
        else if self.triple.starts_with("wasm32") { "wasm32" }
        else if self.triple.starts_with("wasm64") { "wasm64" }
        else { "unknown" }
    }
}

/* ------------------------------ Toolchain ------------------------------- */

/// Emplacement d’un outil (binaire) avec son chemin.
#[derive(Debug, Clone)]
pub struct Tool {
    /// Nom logique (ex: "cc").
    pub name: String,
    /// Chemin absolu.
    pub path: Utf8PathBuf,
}

/// Toolchain C/C++ (ou wrappers) pour une cible.
#[derive(Debug, Clone)]
pub struct Toolchain {
    /// Compilateur C.
    pub cc: Utf8PathBuf,
    /// Compilateur C++ (facultatif).
    pub cxx: Option<Utf8PathBuf>,
    /// Archiver.
    pub ar: Option<Utf8PathBuf>,
    /// ranlib.
    pub ranlib: Option<Utf8PathBuf>,
    /// linker dédié (si non via cc).
    pub ld: Option<Utf8PathBuf>,
    /// strip.
    pub strip: Option<Utf8PathBuf>,
    /// sysroot (si existant).
    pub sysroot: Option<Utf8PathBuf>,
    /// flags supplémentaires.
    pub cflags: Vec<String>,
    /// ldflags supplémentaires.
    pub ldflags: Vec<String>,
}

impl Default for Toolchain {
    fn default() -> Self {
        Self {
            cc: Utf8PathBuf::from("cc"),
            cxx: None,
            ar: None,
            ranlib: None,
            ld: None,
            strip: None,
            sysroot: None,
            cflags: vec![],
            ldflags: vec![],
        }
    }
}

/// Détecte la toolchain pour `target`.
#[cfg(feature = "detect")]
pub fn detect(target: &TargetTriple) -> Result<Toolchain> {
    use which::which;

    let prefix = guess_prefix(target);

    // Stratégie:
    // 1) Chercher `${prefix}-cc` ou `${prefix}-gcc` ou `${prefix}-clang`.
    // 2) Sinon, fallback `clang`/`gcc` avec `--target=` quand supporté.
    // 3) Idem pour cxx/ar/ranlib/strip.
    let mut tc = Toolchain::default();

    let cc_candidates = [
        format!("{prefix}-gcc"),
        format!("{prefix}-clang"),
        format!("{prefix}-cc"),
    ];
    tc.cc = pick_first(&cc_candidates).unwrap_or_else(|| Utf8PathBuf::from("cc"));

    // cxx
    let cxx_candidates = [
        format!("{prefix}-g++"),
        format!("{prefix}-clang++"),
        format!("{prefix}-c++"),
    ];
    tc.cxx = pick_first(&cxx_candidates);

    // binutils
    tc.ar = pick_first(&[format!("{prefix}-ar")]);
    tc.ranlib = pick_first(&[format!("{prefix}-ranlib")]);
    tc.strip = pick_first(&[format!("{prefix}-strip")]);
    tc.ld = pick_first(&[format!("{prefix}-ld")]);

    // sysroot heuristique fréquente
    tc.sysroot = guess_sysroot(target);

    // flags
    if is_clang_path(&tc.cc) {
        tc.cflags.push(format!("--target={}", target.triple));
        if let Some(sr) = &tc.sysroot {
            tc.cflags.push(format!("--sysroot={}", sr));
        }
    }
    if let Some(ld) = &tc.ld {
        // pas d’action obligatoire; ld explicite si utile
        let _ = ld;
    }
    Ok(tc)
}

#[cfg(not(feature = "detect"))]
pub fn detect(_target: &TargetTriple) -> Result<Toolchain> {
    Err(Error::Unsupported("feature `detect` is disabled"))
}

#[cfg(feature = "detect")]
fn pick_first(cands: &[String]) -> Utf8PathBuf {
    for c in cands {
        if let Ok(p) = which::which(c) {
            if let Ok(u) = Utf8PathBuf::from_path_buf(p) { return u; }
        }
    }
    // not found → keep as-is name, useful as hint
    Utf8PathBuf::from(cands.last().unwrap().as_str())
}

#[cfg(feature = "detect")]
fn is_clang_path(p: &Utf8Path) -> bool {
    let s = p.as_str();
    s.contains("clang")
}

#[cfg(feature = "detect")]
fn guess_prefix(target: &TargetTriple) -> String {
    // Quelques préfixes usuels pour gcc/clang croisés
    let t = target.triple.as_str();
    if t == "x86_64-unknown-linux-gnu" { "x86_64-linux-gnu".into() }
    else if t.starts_with("aarch64-") { "aarch64-linux-gnu".into() }
    else if t.starts_with("armv7-") || t.starts_with("arm-") { "arm-linux-gnueabihf".into() }
    else if t.starts_with("riscv64-") { "riscv64-linux-gnu".into() }
    else if t.ends_with("-windows-gnu") { format!("{}-w64-mingw32", target.arch()) }
    else { target.triple.clone() }
}

#[cfg(feature = "detect")]
fn guess_sysroot(target: &TargetTriple) -> Option<Utf8PathBuf> {
    // Heuristique debian/ubuntu: /usr/<triplet> ou /usr/<prefix>/
    let candidates = [
        format!("/usr/{}", target.triple),
        format!("/usr/{}", guess_prefix(target)),
        "/opt/sysroots".to_string(),
    ];
    for c in candidates {
        let p = Utf8PathBuf::from(c);
        if p.exists() { return Some(p); }
    }
    None
}

/* ------------------------------ Cargo ENV ------------------------------- */

/// Génère les variables d’environnement pour `cargo` (build.rs, cc-rs, etc.).
/// - `RUSTFLAGS` enrichi (`-C linker=...`, `-C link-arg=...`) si applicable.
/// - `CC_<target>`, `CXX_<target>`, `AR_<target>`.
/// - `PKG_CONFIG_SYSROOT_DIR`, `PKG_CONFIG_PATH` si sysroot connu.
pub fn env_for_cargo(target: &TargetTriple, tc: &Toolchain, mut rustflags: Option<String>) -> Vec<(String, String)> {
    let mut envs = Vec::new();
    let canon = target.triple.replace('-', "_");

    envs.push((format!("CC_{}", canon), tc.cc.as_str().to_owned()));
    if let Some(cxx) = &tc.cxx {
        envs.push((format!("CXX_{}", canon), cxx.as_str().to_owned()));
    }
    if let Some(ar) = &tc.ar {
        envs.push((format!("AR_{}", canon), ar.as_str().to_owned()));
    }

    // RUSTFLAGS
    let mut rf = rustflags.take().unwrap_or_default();
    if let Some(ld) = &tc.ld {
        if !rf.is_empty() { rf.push(' '); }
        rf.push_str(&format!("-C linker={}", ld));
    } else {
        // Beaucoup d’environnements utilisent cc comme linker
        if !rf.is_empty() { rf.push(' '); }
        rf.push_str(&format!("-C linker={}", tc.cc));
    }
    for a in &tc.ldflags {
        rf.push_str(" -C link-arg=");
        rf.push_str(a);
    }
    envs.push(("RUSTFLAGS".into(), rf));

    // pkg-config
    if let Some(sr) = &tc.sysroot {
        envs.push(("PKG_CONFIG_SYSROOT_DIR".into(), sr.as_str().to_owned()));
    }

    envs
}

/* -------------------------------- Runner -------------------------------- */

/// Stratégie d’exécution pour binaires cross.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Runner {
    /// Exécution locale.
    Local,
    /// QEMU utilitaire (user-mode). Exemple: `qemu-aarch64 -L <sysroot>`.
    #[cfg(feature = "qemu")]
    Qemu { qemu: Utf8PathBuf, args: Vec<String> },
    /// Docker: `docker run ... <image> <cmd>`.
    #[cfg(feature = "docker")]
    Docker { image: String, args: Vec<String> },
}

/// Runner par défaut pour la cible (heuristique).
pub fn runner_for(target: &TargetTriple, sysroot: Option<&Utf8Path>) -> Result<Runner> {
    if target.os() == host_os() && target.arch() == host_arch() {
        return Ok(Runner::Local);
    }
    #[cfg(feature = "qemu")]
    {
        if let Some(q) = qemu_for_arch(target.arch()) {
            let mut args = Vec::new();
            if let Some(sr) = sysroot { args.extend(["-L".into(), sr.as_str().into()]); }
            return Ok(Runner::Qemu { qemu: Utf8PathBuf::from(q), args });
        }
    }
    #[cfg(feature = "docker")]
    {
        // Fallback docker: image générique multi-arch si dispo côté utilisateur.
        return Ok(Runner::Docker { image: format!("{}-runner:latest", target.arch()), args: vec![] });
    }
    Err(Error::Unsupported("no runner available for this target (enable features `qemu` or `docker`)"))
}

#[cfg(feature = "qemu")]
fn qemu_for_arch(arch: &str) -> Option<&'static str> {
    match arch {
        "aarch64" => Some("qemu-aarch64"),
        "arm" => Some("qemu-arm"),
        "riscv64" => Some("qemu-riscv64"),
        "x86_64" => Some("qemu-x86_64"),
        _ => None,
    }
}

#[cfg(feature = "std")]
fn host_os() -> &'static str {
    if cfg!(target_os = "linux") { "linux" }
    else if cfg!(target_os = "macos") { "macos" }
    else if cfg!(target_os = "windows") { "windows" }
    else { "unknown" }
}

#[cfg(feature = "std")]
fn host_arch() -> &'static str {
    if cfg!(target_arch = "x86_64") { "x86_64" }
    else if cfg!(target_arch = "aarch64") { "aarch64" }
    else if cfg!(target_arch = "arm") { "arm" }
    else if cfg!(target_arch = "riscv64") { "riscv64" }
    else { "unknown" }
}

/* ----------------------------- Run helpers ------------------------------ */

/// Exécute un binaire construit pour `target` selon le `runner`.
/// `bin` est le chemin du binaire cible; `args` ses arguments.
/// Retourne stdout si succès.
#[cfg(all(feature = "std"))]
pub fn run_with(runner: &Runner, bin: &Utf8Path, args: &[String]) -> Result<String> {
    match runner {
        Runner::Local => run_cmd(bin.as_str(), args),
        #[cfg(feature = "qemu")]
        Runner::Qemu { qemu, args: qargs } => {
            let mut full = qargs.clone();
            full.push(bin.as_str().into());
            full.extend(args.iter().cloned());
            run_cmd(qemu.as_str(), &full)
        }
        #[cfg(feature = "docker")]
        Runner::Docker { image, args: dargs } => {
            let mut full = vec!["run".to_string(), "--rm".into()];
            full.extend(dargs.clone());
            full.push(image.clone());
            full.push(bin.as_str().into());
            full.extend(args.iter().cloned());
            run_cmd("docker", &full)
        }
    }
}

#[cfg(feature = "std")]
fn run_cmd(tool: &str, args: &[String]) -> Result<String> {
    let out = Command::new(tool).args(args).output().map_err(|e| Error::Io(e.to_string()))?;
    if out.status.success() {
        Ok(String::from_utf8_lossy(&out.stdout).into_owned())
    } else {
        Err(Error::Proc {
            tool: tool.into(),
            code: out.status.code(),
            stderr: truncate(String::from_utf8_lossy(&out.stderr).into_owned()),
        })
    }
}

/* --------------------------------- Utils -------------------------------- */

fn truncate(mut s: String) -> String {
    const MAX: usize = 24_000;
    if s.len() > MAX { s.truncate(MAX); }
    s
}

/* --------------------------------- Tests -------------------------------- */

#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;

    #[test]
    fn triple_parse() {
        let t = TargetTriple::from_str("x86_64-unknown-linux-gnu").unwrap();
        assert_eq!(t.arch(), "x86_64");
        assert_eq!(t.os(), "linux");
    }

    #[test]
    fn env_build() {
        let t = TargetTriple::from_str("aarch64-unknown-linux-gnu").unwrap();
        let mut tc = Toolchain::default();
        tc.cc = Utf8PathBuf::from("aarch64-linux-gnu-gcc");
        let envs = env_for_cargo(&t, &tc, Some("-C opt-level=2".into()));
        assert!(envs.iter().any(|(k, _)| k.starts_with("CC_")));
        assert!(envs.iter().any(|(k, _)| k == "RUSTFLAGS"));
    }

    #[test]
    fn runner_local_or_err() {
        let t = TargetTriple::from_str(if cfg!(target_os = "linux") { "x86_64-unknown-linux-gnu" } else { "aarch64-unknown-linux-gnu" }).unwrap();
        let _ = runner_for(&t, None).ok();
    }
}