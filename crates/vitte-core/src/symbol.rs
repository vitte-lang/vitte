//! Thread-safe global symbol interner.
//!
//! Symbols are lightweight identifiers (`Copy`, `Eq`, `Hash`) obtained by interning
//! string slices through [`intern`]. Interned strings live for the duration of the
//! process and resolve back to `&'static str` via [`resolve`] or [`Symbol::as_str`].
//!
//! The pool is available in both `std` and `alloc` configurations; in `no_std`
//! builds it falls back to a simple spin mutex.

use core::fmt;

use hashbrown::HashMap;
use spin::{Mutex, Once};

#[cfg(feature = "std")]
use std::{borrow::ToOwned, boxed::Box, string::String, vec::Vec};

#[cfg(not(feature = "std"))]
use alloc::{borrow::ToOwned, boxed::Box, string::String, vec::Vec};

/* ─────────────────────────── Symbol ─────────────────────────── */

/// Interned identifier backed by the global pool.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[repr(transparent)]
pub struct Symbol(u32);

impl Symbol {
    /// Intern a string in the global pool.
    #[inline]
    pub fn intern(value: &str) -> Self {
        intern(value)
    }

    /// Resolve the interned string.
    #[inline]
    pub fn as_str(self) -> &'static str {
        resolve(self)
    }

    /// Zero-based index inside the global pool.
    #[inline]
    pub const fn index(self) -> u32 {
        self.0
    }

    /// Construct a symbol from a raw index (unchecked).
    #[inline]
    pub const fn from_raw(raw: u32) -> Self {
        Symbol(raw)
    }
}

impl From<Symbol> for u32 {
    fn from(value: Symbol) -> Self {
        value.0
    }
}

impl fmt::Display for Symbol {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(resolve(*self))
    }
}

/* ─────────────────────────── Helpers ─────────────────────────── */

fn pool() -> &'static Mutex<SymbolPool> {
    static GLOBAL_POOL: Once<Mutex<SymbolPool>> = Once::new();
    GLOBAL_POOL.call_once(|| Mutex::new(SymbolPool::default()))
}

/* ─────────────────────────── Public API ─────────────────────────── */

/// Intern a string slice into the global symbol pool.
pub fn intern(value: &str) -> Symbol {
    let mut guard = pool().lock();
    guard.get_or_intern(value)
}

/// Try to resolve a symbol back to the interned `&'static str`.
pub fn try_resolve(symbol: Symbol) -> Option<&'static str> {
    let guard = pool().lock();
    guard.resolve(symbol)
}

/// Resolve a symbol to its interned `&'static str`, panicking if unknown.
pub fn resolve(symbol: Symbol) -> &'static str {
    try_resolve(symbol).expect("symbol not found in global pool")
}

/// Returns the number of unique symbols currently interned.
pub fn len() -> usize {
    pool().lock().len()
}

/// Returns `true` if the pool already contains the given string.
pub fn contains(value: &str) -> bool {
    pool().lock().contains(value)
}

/* ─────────────────────────── Symbol pool internals ─────────────────────────── */

#[derive(Default)]
struct SymbolPool {
    map: HashMap<&'static str, Symbol>,
    strings: Vec<&'static str>,
}

impl SymbolPool {
    fn get_or_intern(&mut self, value: &str) -> Symbol {
        if let Some(sym) = self.lookup(value) {
            return sym;
        }
        self.alloc_symbol(value)
    }

    fn resolve(&self, symbol: Symbol) -> Option<&'static str> {
        self.strings.get(symbol.index() as usize).copied()
    }

    fn len(&self) -> usize {
        self.strings.len()
    }

    fn contains(&self, value: &str) -> bool {
        self.map.contains_key(value)
    }

    fn lookup(&self, value: &str) -> Option<Symbol> {
        self.map.get(value).copied()
    }

    fn alloc_symbol(&mut self, value: &str) -> Symbol {
        const MAX_SYMBOLS: usize = u32::MAX as usize;
        if self.strings.len() >= MAX_SYMBOLS {
            panic!("symbol pool overflow");
        }
        let owned: String = value.to_owned();
        let leaked: &'static str = Box::leak(owned.into_boxed_str());
        let symbol = Symbol::from_raw(self.strings.len() as u32);
        self.strings.push(leaked);
        self.map.insert(leaked, symbol);
        symbol
    }
}

/* ─────────────────────────── Mutex abstraction ─────────────────────────── */
// Intentionally empty: synchronization relies on `spin::Mutex`.

/* ─────────────────────────── Tests ─────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn deduplicates_symbols() {
        let base = len();
        let a = intern("alpha");
        let b = intern("alpha");
        assert_eq!(a, b);
        assert_eq!(resolve(a), "alpha");
        assert_eq!(len(), base + 1);
    }

    #[cfg(feature = "std")]
    #[test]
    fn concurrent_interning() {
        use std::thread;

        let handles: Vec<_> = (0..8)
            .map(|i| {
                thread::spawn(move || {
                    for _ in 0..64 {
                        let shared = Symbol::intern("shared");
                        assert_eq!(shared.as_str(), "shared");
                        let own = Symbol::intern(&format!("sym-{i}"));
                        assert!(own.as_str().starts_with("sym-"));
                    }
                })
            })
            .collect();

        for handle in handles {
            handle.join().unwrap();
        }

        assert!(len() >= 9, "expected pooled symbols");
    }
}
