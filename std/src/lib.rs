//! Vitte Stdlib — brique utilitaire commune, pensée **no_std** par défaut.
//!
//! - `std` : confort complet (I/O, temps, alloc implicite).
//! - `alloc-only` : pas de `std`, mais `Vec`, `String`, etc. via `alloc`.
//! - `serde` : (dé)sérialisation des IDs.
//! - `small` : alias & helpers `smallvec`.
//! - `index` : alias `IndexMap/IndexSet` (`indexmap`).
//! - `errors` : impl `std::error::Error` pour `Error`.
//!
//! # Modules
//! - [`ids`] : IDs typés (`Id<T>`) + générateur [`IdGen`].
//! - [`arena`] : `Arena<T>` append-only & `IdMap<T,U>` compacte.
//! - [`result`] : `VResult<T>` + macros [`bail!`]/[`ensure!`].
//! - [`bytes`] : lecture/écriture endian-safe sans `Read/Write` std.
//! - [`strutil`] : helpers `&str`/`String` (`to_lower_ascii`, `to_snake_case`).
//! - [`time`] : trait `InstantLike` (impl pour `std::time::Instant`).
//!
//! # Exemple
//! ```
//! use vitte_stdlib::prelude::*;
//! let mut gen = IdGen::default();
//! let a: Id<u32> = gen.fresh();
//! let b: Id<u32> = gen.fresh();
//! assert_ne!(a, b);
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![forbid(unsafe_code)]

#[cfg(any(feature = "std", feature = "alloc-only"))]
extern crate alloc;

pub mod prelude {
    //! Prélu : importe les types/symboles utiles “partout”.
    #[allow(unused_imports)]
    pub use crate::{
        arena::{idx, Arena, IdMap},
        bytes::{ReadBytes, WriteBytes},
        ids::{Id, IdGen, RawId},
        result::{Error, VResult},
        strutil::*,
        time::*,
    };
    pub use crate::{bail, ensure};

    // Re-exports choisis (pratiques en interne)
    #[cfg(feature = "small")]
    #[cfg_attr(docsrs, doc(cfg(feature = "small")))]
    pub use smallvec::{self, SmallVec};

    #[cfg(feature = "index")]
    #[cfg_attr(docsrs, doc(cfg(feature = "index")))]
    pub use indexmap::{IndexMap, IndexSet};

    pub use bitflags::bitflags;
}

pub mod ids {
    //! IDs typés, zéro-cost, sûrs et sérialisables (si `serde`).
    use core::{fmt, marker::PhantomData, num::NonZeroU32};

    /// Représentation brute : `u32` non-nul (0 = NULL réservé).
    #[derive(Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Default)]
    pub struct RawId(Option<NonZeroU32>);

    impl RawId {
        #[inline]
        pub const fn from_u32(v: u32) -> Self {
            match NonZeroU32::new(v) {
                Some(nz) => Self(Some(nz)),
                None => Self(None),
            }
        }
        #[inline]
        pub const fn to_u32(self) -> u32 {
            match self.0 {
                Some(nz) => nz.get(),
                None => 0,
            }
        }
        #[inline]
        pub const fn is_null(self) -> bool {
            self.0.is_none()
        }
    }

    impl fmt::Debug for RawId {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            if let Some(nz) = self.0 {
                write!(f, "RawId({})", nz.get())
            } else {
                f.write_str("RawId(NULL)")
            }
        }
    }

    /// ID typé : `Id<T>` empêche le mélange d’espaces d’identifiants.
    #[derive(Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash)]
    pub struct Id<T> {
        raw: RawId,
        _tag: PhantomData<fn() -> T>,
    }

    impl<T> Id<T> {
        #[inline]
        pub const fn new(raw: RawId) -> Self {
            Self { raw, _tag: PhantomData }
        }
        #[inline]
        pub const fn from_u32(v: u32) -> Self {
            Self::new(RawId::from_u32(v))
        }
        #[inline]
        pub const fn as_raw(self) -> RawId {
            self.raw
        }
        #[inline]
        pub const fn to_u32(self) -> u32 {
            self.raw.to_u32()
        }
        #[inline]
        pub const fn is_null(self) -> bool {
            self.raw.is_null()
        }
    }

    impl<T> Default for Id<T> {
        fn default() -> Self {
            Self::from_u32(0)
        }
    }

    impl<T> core::fmt::Debug for Id<T> {
        fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
            write!(f, "Id({})", self.raw.to_u32())
        }
    }

    #[cfg(feature = "serde")]
    #[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
    mod serde_impl {
        use super::*;
        use serde::{Deserialize, Deserializer, Serialize, Serializer};

        impl<T> Serialize for Id<T> {
            fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
                self.to_u32().serialize(s)
            }
        }
        impl<'de, T> Deserialize<'de> for Id<T> {
            fn deserialize<D: Deserializer<'de>>(d: D) -> Result<Self, D::Error> {
                let v = u32::deserialize(d)?;
                Ok(Id::from_u32(v))
            }
        }
    }

    /// Générateur d’IDs monotone (non thread-safe ; simple & rapide).
    #[derive(Debug, Copy, Clone)]
    pub struct IdGen {
        next: u32,
    }

    impl Default for IdGen {
        fn default() -> Self {
            Self { next: 1 } // 0 = NULL réservé
        }
    }

    impl IdGen {
        #[inline]
        pub fn fresh<T>(&mut self) -> Id<T> {
            let id = Id::<T>::from_u32(self.next);
            self.next = self.next.wrapping_add(1);
            if self.next == 0 {
                self.next = 1;
            }
            id
        }
        #[inline]
        pub fn reset(&mut self) {
            self.next = 1;
        }
        #[inline]
        pub fn peek(&self) -> u32 {
            self.next
        }
    }
}

pub mod arena {
    //! `Arena<T>` : allocations append-only, adressées par `Id<T>`.
    //!
    //! - O(1) indexation par `Id<T>`
    //! - pas de déplacement d’éléments
    //! - itération rapide

    use crate::ids::Id;
    use core::{fmt, iter::FromIterator};

    #[cfg(any(feature = "std", feature = "alloc-only"))]
    use alloc::vec::Vec;

    #[derive(Clone)]
    pub struct Arena<T> {
        data: Vec<T>,
    }

    impl<T> Default for Arena<T> {
        fn default() -> Self {
            Self { data: Vec::new() }
        }
    }

    impl<T> fmt::Debug for Arena<T>
    where
        T: fmt::Debug,
    {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            f.debug_struct("Arena").field("len", &self.data.len()).finish()
        }
    }

    impl<T> Arena<T> {
        #[inline]
        pub fn len(&self) -> usize {
            self.data.len()
        }
        #[inline]
        pub fn is_empty(&self) -> bool {
            self.data.is_empty()
        }

        #[inline]
        pub fn alloc(&mut self, value: T) -> Id<T> {
            let idx = self.data.len();
            self.data.push(value);
            // `idx+1` car 0 est réservé
            Id::from_u32((idx as u32).wrapping_add(1))
        }

        #[inline]
        pub fn get(&self, id: Id<T>) -> Option<&T> {
            self.data.get(idx(id))
        }

        #[inline]
        pub fn get_mut(&mut self, id: Id<T>) -> Option<&mut T> {
            self.data.get_mut(idx(id))
        }

        #[inline]
        pub fn iter(&self) -> core::slice::Iter<'_, T> {
            self.data.iter()
        }

        #[inline]
        pub fn iter_mut(&mut self) -> core::slice::IterMut<'_, T> {
            self.data.iter_mut()
        }

        #[inline]
        pub fn into_vec(self) -> Vec<T> {
            self.data
        }
    }

    impl<T> FromIterator<T> for Arena<T> {
        fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> Self {
            let mut a = Arena::default();
            for v in iter {
                a.alloc(v);
            }
            a
        }
    }

    /// Map d’`Id<T>` vers `U` compacte et rapide (vector-map).
    #[derive(Clone)]
    pub struct IdMap<T, U> {
        data: Vec<Option<U>>,
        _tag: core::marker::PhantomData<fn() -> T>,
    }

    impl<T, U> Default for IdMap<T, U> {
        fn default() -> Self {
            Self { data: Vec::new(), _tag: core::marker::PhantomData }
        }
    }

    impl<T, U> IdMap<T, U> {
        #[inline]
        fn ensure_len(&mut self, slot: usize) {
            let need = slot.saturating_add(1);
            if self.data.len() < need {
                self.data.resize_with(need, || None);
            }
        }
        #[inline]
        pub fn insert(&mut self, id: Id<T>, val: U) -> Option<U> {
            let slot = idx(id);
            self.ensure_len(slot);
            core::mem::replace(&mut self.data[slot], Some(val))
        }
        #[inline]
        pub fn get(&self, id: Id<T>) -> Option<&U> {
            self.data.get(idx(id)).and_then(|o| o.as_ref())
        }
        #[inline]
        pub fn get_mut(&mut self, id: Id<T>) -> Option<&mut U> {
            self.data.get_mut(idx(id)).and_then(|o| o.as_mut())
        }
        #[inline]
        pub fn clear(&mut self) {
            self.data.clear()
        }
    }

    /// Convertit un `Id<T>` en index, avec garde.
    #[inline]
    pub fn idx<T>(id: Id<T>) -> usize {
        id.to_u32().wrapping_sub(1) as usize
    }

    // Re-export utile
    pub use crate::ids::RawId;
}

pub mod result {
    //! Résultats, erreurs et macros ergonomiques.
    use core::fmt;

    #[cfg(any(feature = "std", feature = "alloc-only"))]
    use alloc::string::String;

    /// Erreur “lightweight”.
    #[derive(Debug)]
    pub enum Error {
        /// Message libre (nécessite `alloc` ou `std`).
        #[cfg(any(feature = "std", feature = "alloc-only"))]
        Msg(String),

        /// Erreur d’IO (uniquement si `std`).
        #[cfg(feature = "std")]
        Io(std::io::Error),

        /// Variante statique, utile en `no_std` sans `alloc`.
        Other(&'static str),
    }

    #[cfg(feature = "errors")]
    #[cfg_attr(docsrs, doc(cfg(feature = "errors")))]
    impl core::error::Error for Error {}

    impl fmt::Display for Error {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            match self {
                #[cfg(any(feature = "std", feature = "alloc-only"))]
                Self::Msg(s) => f.write_str(s),
                #[cfg(feature = "std")]
                Self::Io(e) => fmt::Display::fmt(e, f),
                Self::Other(s) => f.write_str(s),
            }
        }
    }

    #[cfg(feature = "std")]
    impl From<std::io::Error> for Error {
        fn from(e: std::io::Error) -> Self {
            Self::Io(e)
        }
    }

    /// Alias court pour les résultats “Vitte”.
    pub type VResult<T> = core::result::Result<T, Error>;

    /// `bail!` : quitter tôt avec un message **formaté** (alloc requise).
    #[macro_export]
    macro_rules! bail {
        ($($tt:tt)*) => {{
            #[allow(unused_imports)]
            use $crate::result::Error;
            #[cfg(any(feature = "std", feature = "alloc-only"))]
            {
                return core::result::Result::Err(Error::Msg(alloc::format!($($tt)*)));
            }
            #[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
            {
                // no_std “pur” : exige une string statique exacte
                return core::result::Result::Err(Error::Other($crate::result::__bail_static_str(&format_args!($($tt)*))));
            }
        }};
    }

    // Helper interne : essaie de convertir un format_args! en &str *si* littéral (no-alloc)
    #[doc(hidden)]
    #[inline]
    pub const fn __bail_static_str(s: &core::fmt::Arguments<'_>) -> &'static str {
        // Dans no_std sans alloc, `bail!` ne peut accepter que des littéraux exacts
        // → l’utilisateur doit écrire: bail!("message statique")
        // (impossible de vérifier à la compilation : doc claire dans le message d’erreur)
        let _ = s;
        "error"
    }

    /// `ensure!` : garde défensive, sinon `bail!`.
    #[macro_export]
    macro_rules! ensure {
        ($cond:expr, $($tt:tt)*) => {{
            if !($cond) { $crate::bail!($($tt)*); }
        }};
    }
}

pub mod bytes {
    //! Lecture/écriture endian-safe sans `std`, pour slices.

    pub trait ReadBytes {
        /// Lis un `u32` **little-endian**.
        fn read_u32_le(&mut self) -> Option<u32>;
        /// Lis un `u64` **little-endian**.
        fn read_u64_le(&mut self) -> Option<u64>;
    }

    pub trait WriteBytes {
        fn write_u32_le(&mut self, v: u32) -> bool;
        fn write_u64_le(&mut self, v: u64) -> bool;
    }

    impl<'a> ReadBytes for &'a [u8] {
        #[inline]
        fn read_u32_le(&mut self) -> Option<u32> {
            if self.len() < 4 {
                return None;
            }
            let (head, tail) = self.split_at(4);
            *self = tail;
            Some(u32::from_le_bytes([head[0], head[1], head[2], head[3]]))
        }
        #[inline]
        fn read_u64_le(&mut self) -> Option<u64> {
            if self.len() < 8 {
                return None;
            }
            let (head, tail) = self.split_at(8);
            *self = tail;
            Some(u64::from_le_bytes([
                head[0], head[1], head[2], head[3], head[4], head[5], head[6], head[7],
            ]))
        }
    }

    #[cfg(any(feature = "std", feature = "alloc-only"))]
    impl WriteBytes for alloc::vec::Vec<u8> {
        #[inline]
        fn write_u32_le(&mut self, v: u32) -> bool {
            self.extend_from_slice(&v.to_le_bytes());
            true
        }
        #[inline]
        fn write_u64_le(&mut self, v: u64) -> bool {
            self.extend_from_slice(&v.to_le_bytes());
            true
        }
    }
}

pub mod strutil {
    //! Petites fonctions utiles pour `&str` / `String` (alloc si dispo).
    #[cfg(any(feature = "std", feature = "alloc-only"))]
    use alloc::string::String;

    /// Minuscule ASCII rapide (ne gère pas l’Unicode complet).
    #[inline]
    #[cfg(any(feature = "std", feature = "alloc-only"))]
    pub fn to_lower_ascii(s: &str) -> String {
        let mut out = String::with_capacity(s.len());
        for b in s.bytes() {
            out.push((if (b'A'..=b'Z').contains(&b) { b + 32 } else { b }) as char);
        }
        out
    }

    /// Convertit `fooBarBaz` → `foo_bar_baz` (heuristique simple).
    #[inline]
    #[cfg(any(feature = "std", feature = "alloc-only"))]
    pub fn to_snake_case(s: &str) -> String {
        let mut out = String::with_capacity(s.len() + s.len() / 4);
        let mut prev_is_lower = false;
        for ch in s.chars() {
            if ch.is_uppercase() && prev_is_lower {
                out.push('_');
            }
            for c in ch.to_lowercase() {
                out.push(c);
            }
            prev_is_lower = ch.is_lowercase();
        }
        out
    }
}

pub mod time {
    //! Minicouche temps/instant, dispo si `std`.
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub use std::time::{Duration, Instant};

    /// Trait générique pour des horloges “Instant-like”.
    pub trait InstantLike: Sized + Copy {
        type Duration;
        fn now() -> Self;
        fn elapsed(&self) -> Self::Duration;
    }

    #[cfg(feature = "std")]
    impl InstantLike for std::time::Instant {
        type Duration = std::time::Duration;
        #[inline]
        fn now() -> Self {
            std::time::Instant::now()
        }
        #[inline]
        fn elapsed(&self) -> Self::Duration {
            self.elapsed()
        }
    }
}

// ===== Re-exports utiles (sans polluer fortement) =====
#[cfg(feature = "small")]
#[cfg_attr(docsrs, doc(cfg(feature = "small")))]
pub use smallvec;

#[cfg(feature = "index")]
#[cfg_attr(docsrs, doc(cfg(feature = "index")))]
pub use indexmap;

pub use bitflags;

// ====== Tests rapides (nécessitent généralement `std`) ======
#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;
    use prelude::*;

    #[test]
    fn ids_basic() {
        let mut g = IdGen::default();
        let a: Id<u8> = g.fresh();
        let b: Id<u8> = g.fresh();
        assert_ne!(a, b);
        assert_eq!(a.to_u32(), 1);
        assert_eq!(b.to_u32(), 2);
        assert!(!a.is_null());
    }

    #[test]
    fn arena_basic() {
        let mut a = Arena::<u32>::default();
        let i1 = a.alloc(42);
        let i2 = a.alloc(7);
        assert_eq!(a.len(), 2);
        assert_eq!(*a.get(i1).unwrap(), 42);
        *a.get_mut(i2).unwrap() += 1;
        assert_eq!(*a.get(i2).unwrap(), 8);
        assert_eq!(arena::idx(i1), 0);
    }

    #[test]
    fn bytes_roundtrip() {
        let mut v = alloc::vec::Vec::new();
        v.write_u32_le(0xDEADBEEF);
        v.write_u64_le(0x0123_4567_89AB_CDEF);
        let mut s: &[u8] = &v;
        assert_eq!(s.read_u32_le().unwrap(), 0xDEADBEEF);
        assert_eq!(s.read_u64_le().unwrap(), 0x0123_4567_89AB_CDEF);
        assert!(s.read_u32_le().is_none());
    }

    #[test]
    fn strutils_ok() {
        assert_eq!(strutil::to_lower_ascii("ABCé"), "abcé");
        assert_eq!(strutil::to_snake_case("FooBarBaz"), "foo_bar_baz");
    }

    #[test]
    fn bail_ensure() {
        fn f(x: i32) -> VResult<()> {
            ensure!(x >= 0, "x doit être >= 0 (got {x})");
            if x == 0 {
                bail!("x ne doit pas être 0");
            }
            Ok(())
        }
        assert!(f(2).is_ok());
        assert!(f(-1).is_err());
        assert!(f(0).is_err());
    }
}
