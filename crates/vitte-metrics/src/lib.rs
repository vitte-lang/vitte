//! vitte-metrics — collecte de métriques pour Vitte
//!
//! Fournit un registre global de compteurs, jauges et histogrammes.
//! Peut exporter en JSON ou Prometheus si les features sont activées.
//!
//! API :
//! - [`counter!`], [`gauge!`], [`histogram!`] (macros)
//! - [`MetricsRegistry`] : registre global
//! - [`Counter`], [`Gauge`], [`Histogram`]
//!
//! Exemple :
//! ```ignore
//! use vitte_metrics::{counter, MetricsRegistry};
//! counter!("compiles").inc();
//! println!("{:?}", MetricsRegistry::snapshot_json());
//! ```

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use std::{collections::HashMap, sync::Arc, time::Instant};
use parking_lot::RwLock;

#[cfg(feature = "json")]
use serde::{Deserialize, Serialize};

lazy_static::lazy_static! {
    static ref REGISTRY: MetricsRegistry = MetricsRegistry::default();
}

/// Compteur cumulatif.
#[derive(Debug, Default)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub struct Counter {
    value: RwLock<u64>,
}
impl Counter {
    pub fn inc(&self) { *self.value.write() += 1; }
    pub fn add(&self, v: u64) { *self.value.write() += v; }
    pub fn get(&self) -> u64 { *self.value.read() }
}

/// Jauge (valeur arbitraire).
#[derive(Debug, Default)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub struct Gauge {
    value: RwLock<i64>,
}
impl Gauge {
    pub fn set(&self, v: i64) { *self.value.write() = v; }
    pub fn add(&self, v: i64) { *self.value.write() += v; }
    pub fn get(&self) -> i64 { *self.value.read() }
}

/// Histogramme (latences, tailles, etc.).
#[derive(Debug, Default)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub struct Histogram {
    samples: RwLock<Vec<u64>>,
}
impl Histogram {
    pub fn observe(&self, v: u64) { self.samples.write().push(v); }
    pub fn snapshot(&self) -> Vec<u64> { self.samples.read().clone() }
}

/// Registre global de métriques.
#[derive(Debug, Default)]
pub struct MetricsRegistry {
    counters: RwLock<HashMap<String, Arc<Counter>>>,
    gauges: RwLock<HashMap<String, Arc<Gauge>>>,
    histograms: RwLock<HashMap<String, Arc<Histogram>>>,
}

impl MetricsRegistry {
    pub fn global() -> &'static Self { &REGISTRY }

    pub fn counter(name: &str) -> Arc<Counter> {
        let reg = Self::global();
        let mut map = reg.counters.write();
        map.entry(name.to_string()).or_insert_with(|| Arc::new(Counter::default())).clone()
    }

    pub fn gauge(name: &str) -> Arc<Gauge> {
        let reg = Self::global();
        let mut map = reg.gauges.write();
        map.entry(name.to_string()).or_insert_with(|| Arc::new(Gauge::default())).clone()
    }

    pub fn histogram(name: &str) -> Arc<Histogram> {
        let reg = Self::global();
        let mut map = reg.histograms.write();
        map.entry(name.to_string()).or_insert_with(|| Arc::new(Histogram::default())).clone()
    }

    #[cfg(feature = "json")]
    pub fn snapshot_json() -> String {
        let reg = Self::global();
        let mut data = HashMap::new();
        let counters: HashMap<_, _> = reg.counters.read().iter().map(|(k, v)| (k.clone(), v.get())).collect();
        let gauges: HashMap<_, _> = reg.gauges.read().iter().map(|(k, v)| (k.clone(), v.get())).collect();
        let histograms: HashMap<_, _> = reg.histograms.read().iter().map(|(k, v)| (k.clone(), v.snapshot())).collect();
        data.insert("counters", counters);
        data.insert("gauges", gauges);
        data.insert("histograms", histograms);
        serde_json::to_string_pretty(&data).unwrap()
    }

    #[cfg(feature = "prometheus")]
    pub fn snapshot_prometheus() -> String {
        use prometheus::{Encoder, TextEncoder};
        let enc = TextEncoder::new();
        // Convert only counters/gauges
        let mut fams = Vec::new();
        {
            for (k, v) in Self::global().counters.read().iter() {
                let mut c = prometheus::Counter::new(k, "counter").unwrap();
                c.inc_by(v.get() as f64);
                fams.push(c.collect()[0].clone());
            }
            for (k, v) in Self::global().gauges.read().iter() {
                let g = prometheus::Gauge::new(k, "gauge").unwrap();
                g.set(v.get() as f64);
                fams.push(g.collect()[0].clone());
            }
        }
        let mut buf = Vec::new();
        enc.encode(&fams, &mut buf).unwrap();
        String::from_utf8(buf).unwrap()
    }
}

/// Mesure une durée en histogramme.
pub fn time_block<F, R>(hist: &Histogram, f: F) -> R
where
    F: FnOnce() -> R,
{
    let start = Instant::now();
    let r = f();
    let d = start.elapsed().as_micros() as u64;
    hist.observe(d);
    r
}

/// Macros.
#[macro_export]
macro_rules! counter {
    ($name:expr) => {
        $crate::MetricsRegistry::counter($name)
    };
}
#[macro_export]
macro_rules! gauge {
    ($name:expr) => {
        $crate::MetricsRegistry::gauge($name)
    };
}
#[macro_export]
macro_rules! histogram {
    ($name:expr) => {
        $crate::MetricsRegistry::histogram($name)
    };
}

// ----------------------------- Tests --------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn counters_work() {
        let c = MetricsRegistry::counter("compiles");
        c.inc();
        assert!(c.get() >= 1);
    }

    #[test]
    fn gauges_work() {
        let g = MetricsRegistry::gauge("memory");
        g.set(42);
        g.add(8);
        assert_eq!(g.get(), 50);
    }

    #[test]
    fn histograms_work() {
        let h = MetricsRegistry::histogram("latency");
        time_block(&h, || { let _ = 2+2; });
        assert!(!h.snapshot().is_empty());
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_snapshot() {
        let s = MetricsRegistry::snapshot_json();
        assert!(s.contains("counters"));
    }
}