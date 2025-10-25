//! vitte-hygiene — outils autour de l'hygiène des macros.
//!
//! Fournit des utilitaires très simples pour générer des noms uniques et
//! suivre les spans produits pendant une expansion.

#![forbid(unsafe_code)]
#![deny(missing_docs, rust_2018_idioms)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
extern crate alloc;

use core::sync::atomic::{AtomicU32, Ordering};

#[cfg(feature = "std")]
use std::{collections::BTreeMap, format, string::String};
#[cfg(not(feature = "std"))]
use alloc::{collections::BTreeMap, format, string::String};

use vitte_ast::Span;

/// Trait permettant de générer des identifiants hygiéniques.
pub trait HygieneProvider {
    /// Produit un identifiant unique basé sur `seed`.
    fn fresh_name(&self, seed: &str) -> String;
    /// Crée un span synthétique en partant éventuellement d'un span d'origine.
    fn synthesized_span(&self, origin: Option<Span>) -> Span;
}

/// Gestionnaire minimal d'hygiène : produit des noms uniques et des spans synthétiques.
#[derive(Debug, Default)]
pub struct Hygiene {
    counter: AtomicU32,
}

impl Hygiene {
    /// Crée un générateur vide.
    pub const fn new() -> Self {
        Self { counter: AtomicU32::new(0) }
    }
}

impl HygieneProvider for Hygiene {
    fn fresh_name(&self, seed: &str) -> String {
        let id = self.counter.fetch_add(1, Ordering::Relaxed);
        if seed.is_empty() {
            format!("__vitte_{id}")
        } else {
            format!("{seed}__{id}")
        }
    }

    fn synthesized_span(&self, origin: Option<Span>) -> Span {
        origin.unwrap_or_else(|| {
            let id = self.counter.fetch_add(1, Ordering::Relaxed);
            Span::new(0, 0, id)
        })
    }
}

/// Enregistre les correspondances entre spans générés et spans d'origine.
#[derive(Debug, Default, Clone)]
pub struct SpanTracker {
    map: BTreeMap<u32, Span>,
}

impl SpanTracker {
    /// Ajoute une correspondance.
    pub fn record(&mut self, generated: Span, origin: Option<Span>) {
        if let Some(origin) = origin {
            self.map.insert(generated.offset, origin);
        }
    }

    /// Retrouve le span d'origine si connu.
    pub fn origin(&self, generated: Span) -> Option<Span> {
        self.map.get(&generated.offset).copied()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn fresh_name_is_unique() {
        let hygiene = Hygiene::new();
        let a = hygiene.fresh_name("foo");
        let b = hygiene.fresh_name("foo");
        assert_ne!(a, b);
    }

    #[test]
    fn tracker_remembers_origin() {
        let mut tracker = SpanTracker::default();
        let generated = Span::new(0, 0, 1);
        let origin = Span::new(1, 2, 3);
        tracker.record(generated, Some(origin));
        assert_eq!(tracker.origin(generated), Some(origin));
    }
}
