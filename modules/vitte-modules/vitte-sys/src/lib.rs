#![cfg_attr(not(feature = "std"), no_std)]
//! vitte-sys — primitives **système** bas-niveau pour Vitte (cross‑platform)
//!
//! Objectifs :
//! - Mémoire virtuelle page‑alignée (reserve/commit/decommit/protect) via backends OS (feature `os`) ou fallback mémoire (safe, tests)
//! - Mappage de fichiers (feature `memmap` → `memmap2`)
//! - Chargement dynamique de librairies (feature `dl` → `libloading`)
//! - Infos système : taille de page, CPU count, horloge monotone
//! - Threads utilitaires (sleep, yield, nommage) et (optionnel) priorité (`thread-prio`)
//! - Hooks de page de garde / crash (feature `guard`) — stubs sûrs, plateformes prêtes
//!
//! Par défaut, le crate est **`no_std`‑friendly** (via `alloc`) avec un
//! fallback sans appels OS. Active `std` + features pour les backends natifs.

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{format, string::String, vec::Vec};
#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

use core::fmt;

// ───────────────────────────── Erreur / Result ────────────────────────────

#[derive(Debug, Clone)]
pub enum SysError {
    Unsupported(&'static str),
    Invalid(&'static str),
    Oom,
    Io(String),
}
impl fmt::Display for SysError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Unsupported(s) => write!(f, "unsupported:{s}"),
            Self::Invalid(s) => write!(f, "invalid:{s}"),
            Self::Oom => write!(f, "oom"),
            Self::Io(s) => write!(f, "io:{s}"),
        }
    }
}

pub type Result<T> = core::result::Result<T, SysError>;

// ───────────────────────────── Utils communs ──────────────────────────────

#[cfg(feature = "std")]
pub fn cpu_count() -> usize {
    std::thread::available_parallelism().map(|n| n.get()).unwrap_or(1)
}
#[cfg(not(feature = "std"))]
pub fn cpu_count() -> usize {
    1
}

#[cfg(feature = "std")]
pub fn now_nanos() -> u128 {
    std::time::Instant::now().elapsed().as_nanos()
}
#[cfg(not(feature = "std"))]
pub fn now_nanos() -> u128 {
    0
}

/// Taille de page la plus probable (4K fallback si inconnue).
#[cfg(feature = "std")]
pub fn page_size() -> usize { 4096 }
#[cfg(not(feature = "std"))]
pub fn page_size() -> usize {
    4096
}

// ───────────────────────────── Mémoire virtuelle ─────────────────────────

bitflags::bitflags! {
    /// Droits/protections de pages.
    pub struct Prot: u8 { const READ=1; const WRITE=2; const EXEC=4; }
}

/// Pages réservées.
#[derive(Debug)]
pub struct Pages {
    ptr: *mut u8,
    size: usize,
    #[cfg(feature = "os")]
    os: OsPages,
    #[cfg(not(feature = "os"))]
    _buf: Vec<u8>,
}


impl Pages {
    /// Réserve un espace *page-aligné*. Avec feature `os`, appelle l’OS.
    pub fn reserve(len: usize) -> Result<Self> {
        let ps = page_size();
        let size = round_up(len, ps);
        #[cfg(feature = "os")]
        {
            return OsPages::reserve(size).map(|os| Self { ptr: os.ptr(), size, os });
        }
        #[cfg(not(feature = "os"))]
        {
            let mut buf = vec![0u8; size];
            let ptr = buf.as_mut_ptr();
            Ok(Self { ptr, size, _buf: buf })
        }
    }
    pub fn as_ptr(&self) -> *mut u8 {
        self.ptr
    }
    pub fn len(&self) -> usize {
        self.size
    }
}

impl Drop for Pages {
    fn drop(&mut self) {
        #[cfg(feature = "os")]
        {
            let _ = self.os.free();
        }
        #[cfg(not(feature = "os"))]
        { /* Vec libéré automatiquement */ }
    }
}

/// Commit/Decommit/Protect API (no-op en fallback sans OS).
pub fn commit(p: &mut Pages, offset: usize, len: usize, prot: Prot) -> Result<()> {
    ensure_range(p, offset, len)?;
    #[cfg(feature = "os")]
    {
        return p.os.commit(offset, len, prot);
    }
    let _ = prot;
    Ok(())
}
pub fn decommit(p: &mut Pages, offset: usize, len: usize) -> Result<()> {
    ensure_range(p, offset, len)?;
    #[cfg(feature = "os")]
    {
        return p.os.decommit(offset, len);
    }
    Ok(())
}
pub fn protect(p: &mut Pages, offset: usize, len: usize, prot: Prot) -> Result<()> {
    ensure_range(p, offset, len)?;
    #[cfg(feature = "os")]
    {
        return p.os.protect(offset, len, prot);
    }
    let _ = prot;
    Ok(())
}

fn ensure_range(p: &Pages, offset: usize, len: usize) -> Result<()> {
    let end = offset.checked_add(len).ok_or(SysError::Invalid("overflow"))?;
    if end > p.size {
        return Err(SysError::Invalid("range"));
    }
    Ok(())
}
fn round_up(x: usize, a: usize) -> usize {
    (x + (a - 1)) & !(a - 1)
}

// ───────────────────────────── Backend OS (optionnel) ────────────────────

#[cfg(feature = "os")]
mod os_pages {
    use super::*;
    use core::ptr;
    #[cfg(unix)]
    use libc;
    #[cfg(windows)]
    use windows_sys::Win32::System::Memory::*;

    #[derive(Debug)]
    pub struct OsPages {
        base: *mut u8,
        len: usize,
    }
    impl OsPages {
        pub fn ptr(&self) -> *mut u8 {
            self.base
        }
    }

    impl OsPages {
        pub fn reserve(len: usize) -> Result<Self> {
            #[cfg(unix)]
            unsafe {
                let ptr = libc::mmap(
                    ptr::null_mut(),
                    len,
                    libc::PROT_NONE,
                    libc::MAP_PRIVATE | libc::MAP_ANON,
                    -1,
                    0,
                );
                if ptr == libc::MAP_FAILED {
                    return Err(SysError::Io("mmap".into()));
                }
                Ok(Self { base: ptr as *mut u8, len })
            }
            #[cfg(windows)]
            unsafe {
                let p = VirtualAlloc(ptr::null_mut(), len, MEM_RESERVE, PAGE_NOACCESS);
                if p.is_null() {
                    return Err(SysError::Io("VirtualAlloc reserve".into()));
                }
                Ok(Self { base: p as *mut u8, len })
            }
        }
        pub fn free(&mut self) -> Result<()> {
            #[cfg(unix)]
            unsafe {
                if libc::munmap(self.base as *mut _, self.len) != 0 {
                    return Err(SysError::Io("munmap".into()));
                }
            }
            #[cfg(windows)]
            unsafe {
                if VirtualFree(self.base as *mut _, 0, MEM_RELEASE) == 0 {
                    return Err(SysError::Io("VirtualFree".into()));
                }
            }
            Ok(())
        }
        pub fn commit(&self, offset: usize, len: usize, prot: Prot) -> Result<()> {
            #[cfg(unix)]
            unsafe {
                let p = self.base.add(offset) as *mut _;
                let mut pf = 0;
                if prot.contains(Prot::READ) {
                    pf |= libc::PROT_READ;
                }
                if prot.contains(Prot::WRITE) {
                    pf |= libc::PROT_WRITE;
                }
                if prot.contains(Prot::EXEC) {
                    pf |= libc::PROT_EXEC;
                }
                // sur mmap anonyme, mprotect suffit à "commiter" sous Linux
                if libc::mprotect(p, len, pf) != 0 {
                    return Err(SysError::Io("mprotect".into()));
                }
                Ok(())
            }
            #[cfg(windows)]
            unsafe {
                let p = self.base.add(offset) as *mut _;
                let fl = prot_to_win(prot);
                let mut old = 0u32;
                if VirtualAlloc(p, len, MEM_COMMIT, fl).is_null() {
                    return Err(SysError::Io("VirtualAlloc commit".into()));
                }
                let _ = old;
                Ok(())
            }
        }
        pub fn decommit(&self, offset: usize, len: usize) -> Result<()> {
            #[cfg(unix)]
            {
                let _ = offset;
                let _ = len;
                Ok(()) /* pas de decommit portable garanti */
            }
            #[cfg(windows)]
            unsafe {
                let p = self.base.add(offset) as *mut _;
                if VirtualFree(p, len, MEM_DECOMMIT) == 0 {
                    return Err(SysError::Io("VirtualFree decommit".into()));
                }
                Ok(())
            }
        }
        pub fn protect(&self, offset: usize, len: usize, prot: Prot) -> Result<()> {
            #[cfg(unix)]
            unsafe {
                let p = self.base.add(offset) as *mut _;
                let mut pf = 0;
                if prot.contains(Prot::READ) {
                    pf |= libc::PROT_READ;
                }
                if prot.contains(Prot::WRITE) {
                    pf |= libc::PROT_WRITE;
                }
                if prot.contains(Prot::EXEC) {
                    pf |= libc::PROT_EXEC;
                }
                if libc::mprotect(p, len, pf) != 0 {
                    return Err(SysError::Io("mprotect".into()));
                }
                Ok(())
            }
            #[cfg(windows)]
            unsafe {
                let p = self.base.add(offset) as *mut _;
                let fl = prot_to_win(prot);
                let mut old = 0u32;
                if VirtualProtect(p, len, fl, &mut old) == 0 {
                    return Err(SysError::Io("VirtualProtect".into()));
                }
                Ok(())
            }
        }
    }

    #[cfg(windows)]
    fn prot_to_win(p: Prot) -> u32 {
        match (p.contains(Prot::READ), p.contains(Prot::WRITE), p.contains(Prot::EXEC)) {
            (false, false, false) => PAGE_NOACCESS,
            (true, false, false) => PAGE_READONLY,
            (true, true, false) => PAGE_READWRITE,
            (true, false, true) => PAGE_EXECUTE_READ,
            (true, true, true) => PAGE_EXECUTE_READWRITE,
            (false, false, true) => PAGE_EXECUTE,
            (false, true, _) => PAGE_READWRITE, // pas de write seul
        }
    }

    pub use OsPages;
}
#[cfg(feature = "os")]
pub use os_pages::OsPages;

// ───────────────────────────── Mappage de fichiers ───────────────────────

/// Mappage mémoire en lecture (feature `memmap` pour OS, sinon fallback lecture copy).
pub struct FileMap {
    ptr: *const u8,
    len: usize,
    #[cfg(feature = "memmap")]
    map: memmap2::Mmap,
    #[cfg(all(feature = "std", not(feature = "memmap")))]
    _buf: Vec<u8>,
}
impl FileMap {
    pub fn as_ptr(&self) -> *const u8 {
        self.ptr
    }
    pub fn len(&self) -> usize {
        self.len
    }
}

#[cfg(all(feature = "std", feature = "memmap"))]
pub fn map_file_read(path: &str) -> Result<FileMap> {
    use std::{fs::File, io::Read};
    let f = File::open(path).map_err(|e| SysError::Io(e.to_string()))?;
    let m =
        unsafe { memmap2::MmapOptions::new().map(&f) }.map_err(|e| SysError::Io(e.to_string()))?;
    let ptr = m.as_ptr();
    let len = m.len();
    Ok(FileMap { ptr, len, map: m })
}

#[cfg(all(feature = "std", not(feature = "memmap")))]
pub fn map_file_read(path: &str) -> Result<FileMap> {
    use std::{fs::File, io::Read};
    let mut f = File::open(path).map_err(|e| SysError::Io(e.to_string()))?;
    let mut buf = Vec::new();
    f.read_to_end(&mut buf).map_err(|e| SysError::Io(e.to_string()))?;
    let ptr = buf.as_ptr();
    let len = buf.len();
    Ok(FileMap { ptr, len, _buf: buf })
}

#[cfg(not(feature = "std"))]
pub fn map_file_read(_path: &str) -> Result<FileMap> {
    Err(SysError::Unsupported("map_file_read"))
}

// ───────────────────────────── Chargement dynamique ──────────────────────

#[cfg(all(feature = "std", feature = "dl"))]
pub struct DynLib(libloading::Library);
#[cfg(all(feature = "std", feature = "dl"))]
impl DynLib {
    pub unsafe fn open(path: &str) -> Result<Self> {
        libloading::Library::new(path)
            .map(DynLib)
            .map_err(|e| SysError::Io(e.to_string()))
    }
    pub unsafe fn symbol<T>(&self, name: &[u8]) -> Result<libloading::Symbol<'_, T>> {
        self.0.get(name).map_err(|e| SysError::Io(e.to_string()))
    }
}

// ───────────────────────────── Threads utilitaires ───────────────────────

#[cfg(feature = "std")]
pub fn sleep_ms(ms: u64) {
    std::thread::sleep(std::time::Duration::from_millis(ms));
}
#[cfg(not(feature = "std"))]
pub fn sleep_ms(_ms: u64) {}

#[cfg(feature = "std")]
pub fn yield_now() {
    std::thread::yield_now();
}
#[cfg(not(feature = "std"))]
pub fn yield_now() {}

#[cfg(feature = "std")]
pub fn set_current_thread_name(name: &str) {
    let _ = name;
}
#[cfg(not(feature = "std"))]
pub fn set_current_thread_name(_: &str) {}

// ───────────────────────────── Guard pages / crash hooks (stubs) ─────────

/// Installe un handler *best-effort* (no-op par défaut). Utile pour VM guard pages.
#[cfg(feature = "guard")]
pub fn install_guard_page_handler() -> Result<()> {
    #[cfg(unix)]
    { /* TODO: sigaltstack + sigaction(SIGSEGV) */ }
    #[cfg(windows)]
    { /* TODO: AddVectoredExceptionHandler */ }
    Ok(())
}

// ───────────────────────────── Tests ─────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn cpu_count_nonzero() {
        assert!(cpu_count() >= 1);
    }

    #[test]
    fn reserve_basic() {
        let mut p = Pages::reserve(10_000).unwrap();
        assert!(p.as_ptr() as usize % page_size() == 0 || cfg!(not(feature = "os"))); // align OS sinon fallback
        let _ = commit(&mut p, 0, 4096, Prot::READ | Prot::WRITE);
        let _ = protect(&mut p, 0, 4096, Prot::READ);
        let _ = decommit(&mut p, 0, 4096);
    }

    #[cfg(all(feature = "std", feature = "memmap"))]
    #[test]
    fn map_nonexistent_fails() {
        assert!(map_file_read("this/does/not/exist").is_err());
    }
}
