//! vitte-sys — primitives bas niveau multi-plateforme pour Vitte
//!
//! Fonctions disponibles avec `std` (par défaut) :
//! - Infos OS: nom, version, architecture, hôte
//! - Processus: PID, spawn, statut, kill, sleep
//! - Utilisateur: UID/GID, HOME, SHELL, USER, TMP
//! - Environnement: get/set/unset, liste
//! - Système: CPU/Mémoire/Disques (si feature `sysinfo`)
//! - Horloges: temps réel, monotone
//! - Chemins: cwd, chdir, temp_dir
//!
//! Avec `libc` (Unix) et `windows-sys` (Windows): wrappers ciblés.
//! Avec `alloc-only`: stubs minimales sans I/O ni OS réels.
//!
//! Erreurs jolies si `errors` activée.

#![forbid(unsafe_code)]

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.");

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{string::String, vec::Vec};

#[cfg(feature = "std")]
use std::{
    env,
    path::{Path, PathBuf},
    process::{Child, Command, ExitStatus, Stdio},
    thread,
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};

#[cfg(feature = "errors")]
pub use error::Error;

#[cfg(feature = "errors")]
mod error {
    #[cfg(feature = "std")]
    use thiserror::Error as ThisError;

    #[cfg_attr(feature = "std", derive(ThisError))]
    #[derive(Debug)]
    pub enum Error {
        #[cfg(feature = "std")]
        #[error("I/O error: {0}")]
        Io(#[from] std::io::Error),

        #[cfg(feature = "std")]
        #[error("Env error: {0}")]
        Var(#[from] std::env::VarError),

        #[error("Unsupported on this platform/feature set")]
        Unsupported,

        #[error("Invalid argument: {0}")]
        InvalidArg(&'static str),

        #[error("Other: {0}")]
        Other(&'static str),
    }

    impl From<&'static str> for Error {
        fn from(s: &'static str) -> Self {
            Error::Other(s)
        }
    }
}

/// Alias résultat conditionné par la feature `errors`.
#[cfg(feature = "errors")]
pub type Result<T> = core::result::Result<T, Error>;

#[cfg(not(feature = "errors"))]
pub type Result<T> = core::result::Result<T, &'static str>;

/* =======================================================================
SECTION: TYPES DE DONNÉES
======================================================================= */

/// Informations système de base.
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct OsInfo {
    pub family: String, // "unix", "windows", "unknown"
    pub os: String,     // "linux", "macos", "windows", ...
    pub version: Option<String>,
    pub arch: String, // "x86_64", "aarch64", ...
    pub hostname: Option<String>,
}

/// Informations CPU/Mémoire (si `sysinfo`).
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug, PartialEq)]
pub struct SysMetrics {
    pub total_memory: u64,
    pub used_memory: u64,
    pub total_swap: u64,
    pub used_swap: u64,
    pub cpus: usize,
    pub avg_cpu_usage: f32, // 0..100
}

/* =======================================================================
SECTION: ALLOC-ONLY STUBS
======================================================================= */

#[cfg(feature = "alloc-only")]
mod stubs {
    use super::*;

    pub fn unsupported<T>() -> Result<T> {
        #[cfg(feature = "errors")]
        {
            Err(Error::Unsupported)
        }
        #[cfg(not(feature = "errors"))]
        {
            Err("unsupported")
        }
    }

    pub fn os_info() -> Result<OsInfo> {
        Ok(OsInfo {
            family: "unknown".into(),
            os: "unknown".into(),
            version: None,
            arch: "unknown".into(),
            hostname: None,
        })
    }
}

/* =======================================================================
SECTION: UTILITAIRES D’ERREURS
======================================================================= */

// Map std::io::Error -> type d'erreur du crate
#[cfg(all(feature = "std", feature = "errors"))]
#[inline]
fn map_io() -> impl FnOnce(std::io::Error) -> Error {
    |e| Error::from(e)
}

#[cfg(all(feature = "std", not(feature = "errors")))]
#[inline]
fn map_io() -> impl FnOnce(std::io::Error) -> &'static str {
    |_e| "io error"
}

/* =======================================================================
SECTION: OS INFO, HOSTNAME, ARCH
======================================================================= */

/// Renvoie une structure `OsInfo` multi-plateforme.
#[cfg(feature = "std")]
pub fn os_info() -> Result<OsInfo> {
    let family = if cfg!(windows) {
        "windows"
    } else if cfg!(unix) {
        "unix"
    } else {
        "unknown"
    }
    .to_string();

    let os = if cfg!(target_os = "linux") {
        "linux"
    } else if cfg!(target_os = "macos") {
        "macos"
    } else if cfg!(target_os = "windows") {
        "windows"
    } else if cfg!(target_os = "freebsd") {
        "freebsd"
    } else if cfg!(target_os = "openbsd") {
        "openbsd"
    } else if cfg!(target_os = "netbsd") {
        "netbsd"
    } else {
        "unknown"
    }
    .to_string();

    let arch = if cfg!(target_arch = "x86_64") {
        "x86_64"
    } else if cfg!(target_arch = "aarch64") {
        "aarch64"
    } else if cfg!(target_arch = "x86") {
        "x86"
    } else if cfg!(target_arch = "arm") {
        "arm"
    } else if cfg!(target_arch = "riscv64") {
        "riscv64"
    } else {
        "unknown"
    }
    .to_string();

    let hostname = hostname().ok();

    // Version best-effort
    let version = os_version();

    Ok(OsInfo { family, os, version, arch, hostname })
}

/// Nom d’hôte.
#[cfg(feature = "std")]
pub fn hostname() -> Result<String> {
    // Linux/macOS: `scutil --get ComputerName` possible, mais /etc/hostname est plus générique
    // On tente d’abord env var puis API standard.
    match hostname_fallback() {
        Some(h) => Ok(h),
        None => {
            // std ne fournit pas direct. On tente via `gethostname` si `libc` activé.
            #[cfg(all(feature = "libc", unix))]
            {
                use std::ffi::CStr;
                let mut buf = [0u8; 256];
                // SAFETY: appel C, taille fixe
                let rc = unsafe { libc::gethostname(buf.as_mut_ptr() as *mut _, buf.len()) };
                if rc == 0 {
                    let cstr = &buf[..buf.iter().position(|&b| b == 0).unwrap_or(buf.len())];
                    let name = CStr::from_bytes_with_nul(&[cstr, &[0]].concat())
                        .ok()
                        .and_then(|cs| cs.to_str().ok())
                        .unwrap_or("unknown")
                        .to_string();
                    return Ok(name);
                }
            }
            // Windows via windows-sys
            #[cfg(all(feature = "winapi", windows))]
            {
                use windows_sys::Win32::System::SystemInformation::GetComputerNameW;
                let mut buf = [0u16; 256];
                let mut len = buf.len() as u32;
                // SAFETY: FFI Windows
                let ok = unsafe { GetComputerNameW(buf.as_mut_ptr(), &mut len) };
                if ok != 0 {
                    let s = String::from_utf16_lossy(&buf[..len as usize]);
                    return Ok(s);
                }
            }
            return err_unsup();
        }
    }
}

#[cfg(feature = "std")]
fn hostname_fallback() -> Option<String> {
    // Essayons env var commune
    for k in ["HOSTNAME", "COMPUTERNAME"] {
        if let Ok(v) = env::var(k) {
            if !v.is_empty() {
                return Some(v);
            }
        }
    }
    // Linux/Unix souvent /etc/hostname
    if cfg!(unix) {
        if let Ok(s) = std::fs::read_to_string("/etc/hostname") {
            let s = s.trim();
            if !s.is_empty() {
                return Some(s.to_string());
            }
        }
    }
    None
}

#[cfg(feature = "std")]
fn os_version() -> Option<String> {
    // Essayons env ou fichiers usuels (best-effort).
    if cfg!(target_os = "linux") {
        if let Ok(s) = std::fs::read_to_string("/etc/os-release") {
            for line in s.lines() {
                if let Some(rest) = line.strip_prefix("PRETTY_NAME=") {
                    return Some(rest.trim_matches('"').to_string());
                }
            }
        }
    } else if cfg!(target_os = "macos") {
        // `sw_vers -productVersion`
        if let Ok(out) = Command::new("sw_vers").arg("-productVersion").output() {
            if out.status.success() {
                return Some(String::from_utf8_lossy(&out.stdout).trim().to_string());
            }
        }
    } else if cfg!(target_os = "windows") {
        // `ver` via cmd
        if let Ok(out) = Command::new("cmd").args(["/C", "ver"]).output() {
            if out.status.success() {
                return Some(String::from_utf8_lossy(&out.stdout).trim().to_string());
            }
        }
    }
    None
}

/* =======================================================================
SECTION: ENVIRONNEMENT
======================================================================= */

/// Récupère une variable d’environnement.
#[cfg(feature = "std")]
pub fn env_get(key: &str) -> Result<Option<String>> {
    match env::var(key) {
        Ok(v) => Ok(Some(v)),
        Err(env::VarError::NotPresent) => Ok(None),
        Err(e) => Err(map_env(e)),
    }
}

/// Définit une variable d’environnement.
#[cfg(feature = "std")]
pub fn env_set(key: &str, val: &str) -> Result<()> {
    env::set_var(key, val);
    Ok(())
}

/// Supprime une variable d’environnement.
#[cfg(feature = "std")]
pub fn env_unset(key: &str) -> Result<()> {
    env::remove_var(key);
    Ok(())
}

/// Liste toutes les variables d’environnement.
#[cfg(feature = "std")]
pub fn env_all() -> Vec<(String, String)> {
    env::vars().collect()
}

#[cfg(all(feature = "std", feature = "errors"))]
#[inline]
fn map_env(e: env::VarError) -> Error {
    Error::from(e)
}

#[cfg(all(feature = "std", not(feature = "errors")))]
#[inline]
fn map_env(_e: env::VarError) -> &'static str {
    "env error"
}

/* =======================================================================
SECTION: UTILISATEUR, CHEMINS, PROCESS INFO
======================================================================= */

/// Répertoire courant.
#[cfg(feature = "std")]
pub fn current_dir() -> Result<PathBuf> {
    std::env::current_dir().map_err(map_io())
}

/// Changer de répertoire courant.
#[cfg(feature = "std")]
pub fn set_current_dir<P: AsRef<Path>>(p: P) -> Result<()> {
    std::env::set_current_dir(p).map_err(map_io())
}

/// Répertoire temporaire du système.
#[cfg(feature = "std")]
pub fn temp_dir() -> PathBuf {
    std::env::temp_dir()
}

/// HOME de l’utilisateur.
#[cfg(feature = "std")]
pub fn home_dir() -> Option<PathBuf> {
    // std n’a pas d’API directe cross-platform stable: on inspecte env
    let candidates =
        if cfg!(windows) { ["USERPROFILE", "HOMEDRIVE", "HOMEPATH"] } else { ["HOME", "", ""] };
    for k in candidates {
        if k.is_empty() {
            continue;
        }
        if let Ok(v) = env::var(k) {
            if !v.is_empty() {
                return Some(PathBuf::from(v));
            }
        }
    }
    None
}

/// SHELL courant (Unix) ou COMSPEC (Windows).
#[cfg(feature = "std")]
pub fn shell() -> Option<PathBuf> {
    if cfg!(windows) {
        env::var_os("COMSPEC").map(PathBuf::from)
    } else {
        env::var_os("SHELL").map(PathBuf::from)
    }
}

/// Nom d’utilisateur si dispo.
#[cfg(feature = "std")]
pub fn username() -> Option<String> {
    if let Ok(u) = env::var("USER") {
        if !u.is_empty() {
            return Some(u);
        }
    }
    if let Ok(u) = env::var("USERNAME") {
        if !u.is_empty() {
            return Some(u);
        }
    }
    None
}

/// PID du processus courant.
#[cfg(feature = "std")]
pub fn current_pid() -> u32 {
    std::process::id()
}

/* =======================================================================
SECTION: HORLOGES
======================================================================= */

/// Temps UNIX epoch en secondes flottantes.
#[cfg(feature = "std")]
pub fn unix_time_secs() -> f64 {
    let now = SystemTime::now();
    let dur = now.duration_since(UNIX_EPOCH).unwrap_or(Duration::from_secs(0));
    dur.as_secs() as f64 + dur.subsec_nanos() as f64 * 1e-9
}

/// Instant monotone (pour mesures).
#[cfg(feature = "std")]
pub fn monotonic_now() -> Instant {
    Instant::now()
}

/* =======================================================================
SECTION: PROCESSUS
======================================================================= */

/// Exécute un programme et attend la fin. Retourne le statut.
#[cfg(feature = "std")]
pub fn run_and_wait<S: AsRef<str>>(program: S, args: &[S]) -> Result<ExitStatus> {
    let mut cmd = Command::new(program.as_ref());
    cmd.args(args.iter().map(|s| s.as_ref()));
    cmd.status().map_err(map_io())
}

/// Exécute en arrière-plan. Retourne `Child`.
#[cfg(feature = "std")]
pub fn spawn_bg<S: AsRef<str>>(program: S, args: &[S]) -> Result<Child> {
    let mut cmd = Command::new(program.as_ref());
    cmd.args(args.iter().map(|s| s.as_ref()));
    cmd.stdin(Stdio::null()).stdout(Stdio::null()).stderr(Stdio::null());
    cmd.spawn().map_err(map_io())
}

/// Dors `ms` millisecondes.
#[cfg(feature = "std")]
pub fn sleep_ms(ms: u64) {
    thread::sleep(Duration::from_millis(ms));
}

/* =======================================================================
SECTION: SIGNAUX, KILL
======================================================================= */

/// Tente de tuer un PID avec le signal approprié.
/// - Unix (`libc`): SIGTERM par défaut
/// - Windows (`windows-sys`): TerminateProcess
#[cfg(feature = "std")]
pub fn kill_process(pid: u32) -> Result<()> {
    // Unix
    #[cfg(all(feature = "libc", unix))]
    {
        // SAFETY: appel FFI, signal 15 (SIGTERM)
        let r = unsafe { libc::kill(pid as i32, libc::SIGTERM) };
        if r == 0 {
            return Ok(());
        } else {
            return Err(map_io()(std::io::Error::from_raw_os_error(r)));
        }
    }
    // Windows
    #[cfg(all(feature = "winapi", windows))]
    {
        use windows_sys::Win32::Foundation::CloseHandle;
        use windows_sys::Win32::System::Threading::{
            OpenProcess, TerminateProcess, PROCESS_TERMINATE,
        };

        // SAFETY: FFI open/terminate
        let h = unsafe { OpenProcess(PROCESS_TERMINATE, 0, pid) };
        if h == 0 {
            return Err(err_unsup());
        }
        let ok = unsafe { TerminateProcess(h, 1) };
        unsafe { CloseHandle(h) };
        if ok != 0 {
            Ok(())
        } else {
            Err(err_unsup())
        }
    }
    // Fallback
    #[cfg(not(any(all(feature = "libc", unix), all(feature = "winapi", windows))))]
    {
        let _ = pid;
        return err_unsup();
    }
}

/* =======================================================================
SECTION: SYSINFO (CPU/MEM/DISKS)
======================================================================= */

/// Récupère des métriques système si `sysinfo` est activée.
#[cfg(all(feature = "std", feature = "sysinfo"))]
pub fn sys_metrics() -> Result<SysMetrics> {
    use sysinfo::{CpuExt, System, SystemExt};

    let mut sys = System::new_all();
    sys.refresh_all();

    let total_memory = sys.total_memory();
    let used_memory = sys.used_memory();
    let total_swap = sys.total_swap();
    let used_swap = sys.used_swap();

    let cpus = sys.cpus().len();
    let avg_cpu_usage = if cpus == 0 {
        0.0
    } else {
        let s: f32 = sys.cpus().iter().map(|c| c.cpu_usage()).sum();
        s / cpus as f32
    };

    Ok(SysMetrics { total_memory, used_memory, total_swap, used_swap, cpus, avg_cpu_usage })
}

/* =======================================================================
SECTION: WINDOWS / UNIX SPÉCIFIQUE (Wrappers)
======================================================================= */

/// UID courant (Unix) si `libc`.
#[cfg(all(feature = "std", feature = "libc", unix))]
pub fn unix_uid() -> u32 {
    // SAFETY: FFI `getuid` sans arg
    unsafe { libc::getuid() as u32 }
}

/// GID courant (Unix) si `libc`.
#[cfg(all(feature = "std", feature = "libc", unix))]
pub fn unix_gid() -> u32 {
    // SAFETY: FFI `getgid` sans arg
    unsafe { libc::getgid() as u32 }
}

/// Nom d’utilisateur (Unix) si `libc`, best-effort.
#[cfg(all(feature = "std", feature = "libc", unix))]
pub fn unix_username() -> Option<String> {
    use std::ffi::CStr;
    // SAFETY: FFI
    let uid = unsafe { libc::getuid() };
    let pwd = unsafe { libc::getpwuid(uid) };
    if pwd.is_null() {
        return None;
    }
    // SAFETY: pwd->pw_name est NUL-terminated
    let name = unsafe { CStr::from_ptr((*pwd).pw_name) }.to_str().ok()?.to_string();
    Some(name)
}

/// Nom d’utilisateur Windows si `windows-sys`.
#[cfg(all(feature = "std", feature = "winapi", windows))]
pub fn windows_username() -> Option<String> {
    use windows_sys::Win32::Security::Authorization::GetUserNameW;
    let mut buf = [0u16; 256];
    let mut len = buf.len() as u32;
    // SAFETY: FFI
    let ok = unsafe { GetUserNameW(buf.as_mut_ptr(), &mut len) };
    if ok != 0 && len > 0 {
        Some(String::from_utf16_lossy(&buf[..(len - 1) as usize]))
    } else {
        None
    }
}

/* =======================================================================
SECTION: HELPERS
======================================================================= */

#[inline]
fn err_unsup<T>() -> Result<T> {
    #[cfg(feature = "errors")]
    {
        Err(Error::Unsupported)
    }
    #[cfg(not(feature = "errors"))]
    {
        Err("unsupported")
    }
}

/* =======================================================================
SECTION: TESTS
======================================================================= */

#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;

    #[test]
    fn info_basic() {
        let info = os_info().unwrap();
        assert!(!info.os.is_empty());
        assert!(!info.arch.is_empty());
    }

    #[test]
    fn env_ops() {
        env_set("VITTE_SYS_TEST", "1").unwrap();
        let v = env_get("VITTE_SYS_TEST").unwrap();
        assert_eq!(v, Some("1".into()));
        env_unset("VITTE_SYS_TEST").unwrap();
        let v = env_get("VITTE_SYS_TEST").unwrap();
        assert!(v.is_none());
    }

    #[test]
    fn time_works() {
        let t = unix_time_secs();
        assert!(t > 0.0);
        let _m = monotonic_now();
    }

    #[test]
    fn cwd_ops() {
        let cwd = current_dir().unwrap();
        assert!(cwd.exists());
        // set_current_dir(cwd) should succeed
        set_current_dir(&cwd).unwrap();
    }

    #[cfg(feature = "sysinfo")]
    #[test]
    fn sysinfo_ok() {
        let m = sys_metrics().unwrap();
        assert!(m.cpus >= 0);
    }
}
