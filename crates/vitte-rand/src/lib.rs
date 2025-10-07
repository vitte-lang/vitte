//! vitte-rand — abstractions RNG pour le projet Vitte.
//!
//! Ce module fournit pour le moment un générateur déterministe simple afin de
//! permettre la compilation des dépendants. Il sera remplacé par une
//! implémentation robuste (PCG, WyHash, etc.).

#![forbid(unsafe_code)]
#![deny(missing_docs)]

/// Générateur pseudo-aléatoire linéaire congruentiel très simple.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct LcgRng {
    state: u64,
}

impl LcgRng {
    /// Constante multiplicative (identique à Borland).
    const MUL: u64 = 2_147_483_647;
    /// Constante additive.
    const INC: u64 = 1;

    /// Crée un RNG initialisé avec la seed donnée.
    pub fn new(seed: u64) -> Self {
        Self { state: seed }
    }

    /// Retourne le prochain entier 32 bits pseudo-aléatoire.
    pub fn next_u32(&mut self) -> u32 {
        self.state = self.state.wrapping_mul(Self::MUL).wrapping_add(Self::INC);
        (self.state >> 16) as u32
    }

    /// Retourne un flottant `f64` dans `[0, 1)`.
    pub fn next_f64(&mut self) -> f64 {
        (self.next_u32() as f64) / (u32::MAX as f64 + 1.0)
    }
}

#[cfg(test)]
mod tests {
    use super::LcgRng;

    #[test]
    fn deterministic_sequence() {
        let mut rng1 = LcgRng::new(1234);
        let mut rng2 = LcgRng::new(1234);
        for _ in 0..10 {
            assert_eq!(rng1.next_u32(), rng2.next_u32());
        }
    }
}
