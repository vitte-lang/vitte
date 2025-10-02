

#![deny(missing_docs)]
//! vitte-stats — primitives statistiques pour Vitte
//!
//! Fournit :
//! - Compteurs, moyennes, variance en ligne (Welford).
//! - Histogrammes (HDR) et t-digest (optionnels).
//! - Percentiles, min/max, somme.
//! - Sérialisation optionnelle.
//!
//! Exemple :
//! ```
//! use vitte_stats::OnlineStats;
//! let mut s = OnlineStats::new();
//! s.update(1.0);
//! s.update(2.0);
//! assert_eq!(s.count(), 2);
//! ```

use thiserror::Error;
use parking_lot::Mutex;
use std::sync::Arc;

#[cfg(feature = "serde")]
use serde::{Serialize, Deserialize};

/// Erreurs statistiques
#[derive(Debug, Error)]
pub enum StatsError {
    #[error("empty data")]
    Empty,
    #[error("other: {0}")]
    Other(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, StatsError>;

/// Compteurs / stats en ligne
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, Default)]
pub struct OnlineStats {
    n: u64,
    mean: f64,
    m2: f64,
    min: f64,
    max: f64,
    sum: f64,
}

impl OnlineStats {
    /// Nouveau compteur vide
    pub fn new() -> Self {
        Self { n: 0, mean: 0.0, m2: 0.0, min: f64::INFINITY, max: f64::NEG_INFINITY, sum: 0.0 }
    }

    /// Met à jour avec une valeur
    pub fn update(&mut self, x: f64) {
        self.n += 1;
        let delta = x - self.mean;
        self.mean += delta / self.n as f64;
        let delta2 = x - self.mean;
        self.m2 += delta * delta2;
        if x < self.min { self.min = x; }
        if x > self.max { self.max = x; }
        self.sum += x;
    }

    /// Nombre d'échantillons
    pub fn count(&self) -> u64 { self.n }
    /// Moyenne
    pub fn mean(&self) -> f64 { self.mean }
    /// Variance
    pub fn variance(&self) -> Result<f64> {
        if self.n < 2 { return Err(StatsError::Empty); }
        Ok(self.m2 / (self.n - 1) as f64)
    }
    /// Ecart-type
    pub fn stddev(&self) -> Result<f64> { Ok(self.variance()?.sqrt()) }
    /// Min
    pub fn min(&self) -> f64 { self.min }
    /// Max
    pub fn max(&self) -> f64 { self.max }
    /// Somme
    pub fn sum(&self) -> f64 { self.sum }
}

/// Histogramme HDR optionnel
#[cfg(feature = "histogram")]
pub mod hist {
    use super::*;
    use hdrhistogram::Histogram;

    /// Wrapper autour `hdrhistogram::Histogram`
    #[derive(Debug)]
    pub struct Hdr {
        inner: Histogram<u64>,
    }

    impl Hdr {
        /// Crée un histogramme
        pub fn new(low: u64, high: u64, sig: u8) -> Self {
            Self { inner: Histogram::new_with_bounds(low, high, sig).unwrap() }
        }
        /// Ajoute une valeur
        pub fn record(&mut self, v: u64) { let _ = self.inner.record(v); }
        /// Percentile
        pub fn value_at(&self, q: f64) -> u64 { self.inner.value_at_quantile(q) }
    }
}

/// T-digest optionnel
#[cfg(feature = "tdigest")]
pub mod td {
    use super::*;
    use tdigest::TDigest;

    /// Wrapper t-digest
    #[derive(Debug)]
    pub struct Digest {
        inner: TDigest,
    }

    impl Digest {
        /// Crée un digest vide
        pub fn new() -> Self { Self { inner: TDigest::new() } }
        /// Ajoute une valeur
        pub fn add(&mut self, v: f64) { self.inner = self.inner.merge(&TDigest::from(vec![v])); }
        /// Approx percentile
        pub fn quantile(&self, q: f64) -> f64 { self.inner.quantile(q) }
    }
}

/// Counter partagé threadsafe
#[derive(Debug, Clone, Default)]
pub struct Counter {
    inner: Arc<Mutex<u64>>,
}

impl Counter {
    /// Nouveau compteur
    pub fn new() -> Self { Self { inner: Arc::new(Mutex::new(0)) } }
    /// Incrémente
    pub fn incr(&self) { *self.inner.lock() += 1; }
    /// Valeur courante
    pub fn get(&self) -> u64 { *self.inner.lock() }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn basic_online_stats() {
        let mut s = OnlineStats::new();
        s.update(1.0);
        s.update(3.0);
        assert_eq!(s.count(), 2);
        assert_eq!(s.min(), 1.0);
        assert_eq!(s.max(), 3.0);
        assert_eq!(s.sum(), 4.0);
        assert!((s.mean()-2.0).abs() < 1e-9);
    }

    #[test]
    fn counter_incr() {
        let c = Counter::new();
        c.incr(); c.incr();
        assert_eq!(c.get(), 2);
    }
}