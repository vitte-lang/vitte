#![cfg_attr(not(feature = "std"), no_std)]
//! vitte-mm — **Memory & Handles toolkit** pour Vitte
//!
//! Un set d'outils mémoire minimalistes, rapides et sûrs pour bâtir
//! compilateur/VM/outils :
//!
//! - [`GenArena<T>`] : arène à **handles générationnels** (stables tant que l'élément vit)
//! - [`Bump`] : allocateur **bump** (réinitialisation O(1))
//! - [`Bytes`] : petit **buffer de bytes** resizable + utilitaires d'alignement
//! - [`StrInterner`] : **interneur de chaînes** (Symbol ⇄ &str)
//! - [`Pages`] : pagineur simple pour grands buffers (lecture/écriture par adresse virtuelle)
//!
//! Conçu pour `no_std` (avec `alloc`). Active la feature `std` pour des extras pratiques (I/O tests).
//! La feature `serde` sérialise les handles et quelques conteneurs (quand c'est raisonnable).
//!
//! # Exemple éclair
//! ```ignore
//! use vitte_mm::*;
//! let mut arena = GenArena::<String>::new();
//! let h = arena.insert("hello".to_string());
//! assert_eq!(arena.get(h).unwrap(), "hello");
//! arena.remove(h);
//! assert!(arena.get(h).is_none()); // handle invalidé (génération bumpée)
//! ```

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{
    borrow::ToOwned,
    boxed::Box,
    string::{String, ToString},
    vec::Vec,
};
#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

use core::{
    fmt,
    hash::{Hash, Hasher},
};

// ───────────────────────────── Erreurs ─────────────────────────────

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MmError {
    OutOfMemory,
    InvalidHandle,
    Bounds,
}
impl fmt::Display for MmError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::OutOfMemory => write!(f, "oom"),
            Self::InvalidHandle => write!(f, "bad handle"),
            Self::Bounds => write!(f, "out of bounds"),
        }
    }
}

pub type Result<T> = core::result::Result<T, MmError>;

// ───────────────────────── Handles générationnels ─────────────────────────

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Default)]
pub struct Handle {
    idx: u32,
    gen: u32,
}
impl Handle {
    pub const fn new(idx: u32, gen: u32) -> Self {
        Self { idx, gen }
    }
    pub fn index(self) -> u32 {
        self.idx
    }
    pub fn generation(self) -> u32 {
        self.gen
    }
}
impl fmt::Debug for Handle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "h#{}@{}", self.idx, self.gen)
    }
}
impl PartialEq for Handle {
    fn eq(&self, o: &Self) -> bool {
        self.idx == o.idx && self.gen == o.gen
    }
}
impl Eq for Handle {}
impl Hash for Handle {
    fn hash<H: Hasher>(&self, h: &mut H) {
        self.idx.hash(h);
        self.gen.hash(h);
    }
}

#[derive(Clone, Debug)]
struct Slot<T> {
    gen: u32,
    val: Option<T>,
}

/// Arène à handles générationnels.
#[derive(Clone, Debug, Default)]
pub struct GenArena<T> {
    slots: Vec<Slot<T>>,
    free: Vec<u32>,
    len: u32,
}

impl<T> GenArena<T> {
    pub fn new() -> Self {
        Self { slots: Vec::new(), free: Vec::new(), len: 0 }
    }
    pub fn with_capacity(n: usize) -> Self {
        let mut a = Self::new();
        a.slots.reserve(n);
        a
    }
    pub fn capacity(&self) -> usize {
        self.slots.len()
    }
    pub fn len(&self) -> usize {
        self.len as usize
    }
    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    pub fn insert(&mut self, v: T) -> Handle {
        if let Some(idx) = self.free.pop() {
            let s = &mut self.slots[idx as usize];
            debug_assert!(s.val.is_none());
            s.val = Some(v);
            self.len += 1;
            Handle::new(idx, s.gen)
        } else {
            let idx = self.slots.len() as u32;
            self.slots.push(Slot { gen: 0, val: Some(v) });
            self.len += 1;
            Handle::new(idx, 0)
        }
    }
    pub fn contains(&self, h: Handle) -> bool {
        self.get(h).is_some()
    }
    pub fn get(&self, h: Handle) -> Option<&T> {
        self.slots
            .get(h.idx as usize)
            .and_then(|s| (s.gen == h.gen).then_some(()))
            .and_then(|_| self.slots[h.idx as usize].val.as_ref())
    }
    pub fn get_mut(&mut self, h: Handle) -> Option<&mut T> {
        if let Some(s) = self.slots.get_mut(h.idx as usize) {
            if s.gen == h.gen {
                return s.val.as_mut();
            }
        }
        None
    }
    pub fn remove(&mut self, h: Handle) -> Option<T> {
        if let Some(s) = self.slots.get_mut(h.idx as usize) {
            if s.gen == h.gen {
                self.len -= 1;
                s.gen = s.gen.wrapping_add(1);
                self.free.push(h.idx);
                return s.val.take();
            }
        }
        None
    }
    pub fn clear(&mut self) {
        self.slots.clear();
        self.free.clear();
        self.len = 0;
    }

    pub fn iter(&self) -> impl Iterator<Item = (Handle, &T)> {
        self.slots
            .iter()
            .enumerate()
            .filter_map(|(i, s)| s.val.as_ref().map(|v| (Handle::new(i as u32, s.gen), v)))
    }
    pub fn iter_mut(&mut self) -> impl Iterator<Item = (Handle, &mut T)> {
        self.slots
            .iter_mut()
            .enumerate()
            .filter_map(|(i, s)| s.val.as_mut().map(|v| (Handle::new(i as u32, s.gen), v)))
    }
}

// ───────────────────────────── Bump allocator ────────────────────────────

/// Allocateur **bump** simplifié (mono-thread). `reset()` libère tout.
#[derive(Clone, Debug)]
pub struct Bump {
    buf: Vec<u8>,
    cur: usize,
}
impl Bump {
    pub fn with_capacity(cap: usize) -> Self {
        Self { buf: vec![0; cap], cur: 0 }
    }
    pub fn capacity(&self) -> usize {
        self.buf.len()
    }
    pub fn used(&self) -> usize {
        self.cur
    }
    pub fn reset(&mut self) {
        self.cur = 0;
    }

    /// Alloue `n` octets **alignés** à `align` (puissance de 2).
    pub fn alloc(&mut self, n: usize, align: usize) -> Result<&mut [u8]> {
        debug_assert!(align.is_power_of_two());
        let base = (self.cur + (align - 1)) & !(align - 1);
        let end = base.checked_add(n).ok_or(MmError::OutOfMemory)?;
        if end > self.buf.len() {
            return Err(MmError::OutOfMemory);
        }
        self.cur = end;
        Ok(&mut self.buf[base..end])
    }

    /// Écrit un bloc et retourne son slice.
    pub fn copy_from(&mut self, data: &[u8], align: usize) -> Result<&mut [u8]> {
        let out = self.alloc(data.len(), align)?;
        out.copy_from_slice(data);
        Ok(out)
    }
}

// ───────────────────────────── Bytes utilitaire ──────────────────────────

#[derive(Clone, Debug, Default)]
pub struct Bytes {
    data: Vec<u8>,
}
impl Bytes {
    pub fn new() -> Self {
        Self { data: Vec::new() }
    }
    pub fn with_capacity(n: usize) -> Self {
        Self { data: Vec::with_capacity(n) }
    }
    pub fn as_slice(&self) -> &[u8] {
        &self.data
    }
    pub fn as_mut_slice(&mut self) -> &mut [u8] {
        &mut self.data
    }
    pub fn len(&self) -> usize {
        self.data.len()
    }
    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }
    pub fn clear(&mut self) {
        self.data.clear();
    }
    pub fn push(&mut self, b: u8) {
        self.data.push(b);
    }
    pub fn extend_from_slice(&mut self, s: &[u8]) {
        self.data.extend_from_slice(s);
    }
    pub fn align_to(&mut self, align: usize) {
        let pad = (align - (self.data.len() & (align - 1))) & (align - 1);
        self.data.resize(self.data.len() + pad, 0);
    }
    pub fn write_u32_le(&mut self, x: u32) {
        self.data.extend_from_slice(&x.to_le_bytes());
    }
}

// ─────────────────────────── Interneur de chaînes ─────────────────────────

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub struct Symbol(u32);
impl Symbol {
    /// Retourne l'indice interne associé à ce symbole.
    pub const fn as_u32(self) -> u32 {
        self.0
    }
}

#[derive(Clone, Debug, Default)]
pub struct StrInterner {
    strings: Vec<String>,
}
impl StrInterner {
    pub fn new() -> Self {
        Self { strings: Vec::new() }
    }
    pub fn intern<S: AsRef<str>>(&mut self, s: S) -> Symbol {
        let s = s.as_ref();
        // impl légère : recherche linéaire; remplace-la par hashmap si besoin
        if let Some((i, _)) = self.strings.iter().enumerate().find(|(_, x)| x.as_str() == s) {
            Symbol(i as u32)
        } else {
            let idx = self.strings.len() as u32;
            self.strings.push(s.to_owned());
            Symbol(idx)
        }
    }
    pub fn resolve(&self, sym: Symbol) -> &str {
        self.strings.get(sym.0 as usize).map(|s| s.as_str()).unwrap_or("")
    }
    pub fn len(&self) -> usize {
        self.strings.len()
    }
}

// ───────────────────────────── Mémoire paginée ───────────────────────────

/// Adresse virtuelle simple (page, offset). Encodée sur 64 bits : high=page, low=offset.
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub struct VAddr(u64);
impl VAddr {
    pub fn new(page: u32, off: u32) -> Self {
        Self(((page as u64) << 32) | off as u64)
    }
    pub fn page(self) -> u32 {
        (self.0 >> 32) as u32
    }
    pub fn off(self) -> u32 {
        (self.0 & 0xFFFF_FFFF) as u32
    }
}

#[derive(Clone, Debug)]
pub struct Pages {
    page_size: usize,
    pages: Vec<Vec<u8>>,
}
impl Pages {
    pub fn new(page_size: usize) -> Self {
        assert!(page_size.is_power_of_two());
        Self { page_size, pages: Vec::new() }
    }
    pub fn page_size(&self) -> usize {
        self.page_size
    }
    pub fn alloc_page(&mut self) -> u32 {
        let id = self.pages.len() as u32;
        self.pages.push(vec![0; self.page_size]);
        id
    }
    pub fn alloc(&mut self, n: usize) -> VAddr {
        let p = self.alloc_page();
        debug_assert!(n <= self.page_size);
        VAddr::new(p, 0)
    }

    pub fn read(&self, addr: VAddr, dst: &mut [u8]) -> Result<()> {
        let p = self.pages.get(addr.page() as usize).ok_or(MmError::Bounds)?;
        let off = addr.off() as usize;
        let end = off.checked_add(dst.len()).ok_or(MmError::Bounds)?;
        if end > p.len() {
            return Err(MmError::Bounds);
        };
        dst.copy_from_slice(&p[off..end]);
        Ok(())
    }
    pub fn write(&mut self, addr: VAddr, src: &[u8]) -> Result<()> {
        let p = self.pages.get_mut(addr.page() as usize).ok_or(MmError::Bounds)?;
        let off = addr.off() as usize;
        let end = off.checked_add(src.len()).ok_or(MmError::Bounds)?;
        if end > p.len() {
            return Err(MmError::Bounds);
        };
        p[off..end].copy_from_slice(src);
        Ok(())
    }
}

// ───────────────────────────── Tests ─────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn arena_handles_invalidate() {
        let mut a = GenArena::<i32>::new();
        let h = a.insert(7);
        assert_eq!(a.get(h), Some(&7));
        let _ = a.remove(h).unwrap();
        assert!(a.get(h).is_none());
        let h2 = a.insert(8);
        assert_ne!(h, h2);
    }

    #[test]
    fn bump_align_and_copy() {
        let mut b = Bump::with_capacity(64);
        let s1 = b.alloc(3, 1).unwrap();
        assert_eq!(s1.len(), 3);
        let s2 = b.alloc(4, 4).unwrap();
        assert_eq!((s2.as_ptr() as usize) & 3, 0);
        let s3 = b.copy_from(&[1, 2, 3, 4, 5], 2).unwrap();
        assert_eq!(s3[0], 1);
        b.reset();
        assert_eq!(b.used(), 0);
    }

    #[test]
    fn bytes_align() {
        let mut b = Bytes::new();
        b.push(1);
        b.align_to(8);
        assert_eq!(b.len() % 8, 0);
        b.write_u32_le(0xABCD);
        assert!(b.len() >= 12);
    }

    #[test]
    fn intern_basic() {
        let mut i = StrInterner::new();
        let a = i.intern("hello");
        let b = i.intern("world");
        let c = i.intern("hello");
        assert_eq!(a, c);
        assert_ne!(a, b);
        assert_eq!(i.resolve(a), "hello");
    }

    #[test]
    fn pages_rw() {
        let mut p = Pages::new(4096);
        let va = p.alloc(128);
        let src = [1u8, 2, 3, 4, 5];
        p.write(va, &src).unwrap();
        let mut dst = [0u8; 5];
        p.read(va, &mut dst).unwrap();
        assert_eq!(&dst, &src);
    }
}
