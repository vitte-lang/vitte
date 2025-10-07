#![deny(missing_docs)]
//! vitte-prof — profilage léger pour Vitte
//!
//! Fournit :
//! - Compteurs et minuteries simples avec API thread-safe.
//! - Garde `SpanTimer` pour mesurer des sections de code.
//! - Export JSON des statistiques.
//! - Intégration `tracing` optionnelle (spans -> timers).
//! - Export pprof/flamegraph optionnels.
//!
//! Par défaut, l’impact en l’absence d’appel est quasi nul.

use std::collections::HashMap;
use std::sync::{Mutex, MutexGuard};
use std::time::{Duration, Instant};

use once_cell::sync::OnceCell;
use thiserror::Error;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "tracing")]
use tracing::{self, span, Level};

/// Erreurs de profilage.
#[derive(Debug, Error)]
pub enum ProfError {
    /// Error returned when exporting profiling data (e.g., JSON or flamegraph).
    #[error("export error: {0}")]
    Export(String),
    /// Error coming from the `pprof` profiler integration.
    #[error("pprof error: {0}")]
    #[cfg(feature = "pprof")]
    Pprof(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, ProfError>;

/// Statistiques d’un compteur.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, Default, PartialEq)]
pub struct CounterStat {
    /// Nombre d’incréments.
    pub count: u64,
    /// Somme des valeurs ajoutées.
    pub sum: u64,
}

/// Statistiques d’une minuterie agrégée.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, Default, PartialEq)]
pub struct TimerStat {
    /// Nombre d’échantillons.
    pub samples: u64,
    /// Durée totale cumulée en nanosecondes.
    pub total_ns: u128,
    /// Plus longue durée observée en nanosecondes.
    pub max_ns: u128,
}

impl TimerStat {
    fn add_sample(&mut self, d: Duration) {
        self.samples += 1;
        let ns = d.as_nanos();
        self.total_ns += ns;
        if ns > self.max_ns { self.max_ns = ns; }
    }

    /// Durée moyenne en nanosecondes.
    pub fn mean_ns(&self) -> Option<u128> {
        if self.samples == 0 { None } else { Some(self.total_ns / (self.samples as u128)) }
    }
}

/// Données de profilage globales.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Default, Clone)]
pub struct ProfData {
    counters: HashMap<String, CounterStat>,
    timers: HashMap<String, TimerStat>,
}

impl ProfData {
    fn guard() -> MutexGuard<'static, ProfData> { PROF.get().unwrap().lock().unwrap() }

    fn ensure() { PROF.get_or_init(|| Mutex::new(ProfData::default())); }

    /// Incrémente un compteur.
    pub fn incr(name: &str, v: u64) {
        Self::ensure();
        let mut g = Self::guard();
        let e = g.counters.entry(name.to_string()).or_default();
        e.count += 1;
        e.sum += v;
    }

    /// Ajoute un échantillon de durée pour une minuterie.
    pub fn sample(name: &str, d: Duration) {
        Self::ensure();
        let mut g = Self::guard();
        g.timers.entry(name.to_string()).or_default().add_sample(d);
    }

    /// Snapshot des stats.
    pub fn snapshot() -> Self {
        Self::ensure();
        let g = Self::guard();
        (*g).clone()
    }

    /// Réinitialise toutes les stats.
    pub fn reset() {
        Self::ensure();
        let mut g = Self::guard();
        *g = ProfData::default();
    }
}

static PROF: OnceCell<Mutex<ProfData>> = OnceCell::new();

/// Garde de minuterie. En drop, enregistre la durée sous `name`.
pub struct SpanTimer<'a> {
    name: &'a str,
    start: Instant,
}

impl<'a> SpanTimer<'a> {
    /// Démarre un timer nommé.
    pub fn new(name: &'a str) -> Self { Self { name, start: Instant::now() } }
}

impl<'a> Drop for SpanTimer<'a> {
    fn drop(&mut self) {
        ProfData::sample(self.name, self.start.elapsed());
    }
}

/// Mesure l’exécution d’une fonction et retourne son résultat.
pub fn time_fn<F, T>(name: &str, f: F) -> T
where F: FnOnce() -> T {
    let _t = SpanTimer::new(name);
    f()
}

/// Incrémente un compteur.
pub fn incr(name: &str, v: u64) { ProfData::incr(name, v) }

/// Export JSON des stats courantes.
#[cfg(feature = "serde")]
pub fn export_json() -> Result<String> {
    serde_json::to_string_pretty(&ProfData::snapshot()).map_err(|e| ProfError::Export(e.to_string()))
}

/// Écrit un flamegraph SVG à partir d’un profil échantillonné au runtime (si `pprof` + `flame`).
#[cfg(all(feature = "pprof", feature = "flame"))]
pub fn capture_flamegraph_svg(seconds: u64) -> Result<String> {
    use pprof::ProfilerGuard;
    use std::io::Cursor;

    let guard = ProfilerGuard::new(100).map_err(|e| ProfError::Pprof(e.to_string()))?;
    std::thread::sleep(Duration::from_secs(seconds));
    if let Ok(report) = guard.report().build() {
        let mut out = Cursor::new(Vec::new());
        report.flamegraph(&mut out).map_err(|e| ProfError::Pprof(e.to_string()))?;
        let bytes = out.into_inner();
        let svg = String::from_utf8(bytes).map_err(|e| ProfError::Export(e.to_string()))?;
        Ok(svg)
    } else {
        Err(ProfError::Pprof("no report".into()))
    }
}

/// Démarre un profil pprof et renvoie le guard. L’appelant gère la durée et l’export.
#[cfg(feature = "pprof")]
pub fn start_pprof(freq_hz: i32) -> Result<pprof::ProfilerGuard> {
    pprof::ProfilerGuard::new(freq_hz).map_err(|e| ProfError::Pprof(e.to_string()))
}

/// Intégration tracing : créer un span et timer associé.
#[cfg(feature = "tracing")]
pub fn traced_span(name: &str) -> (tracing::Span, SpanTimer<'_>) {
    let sp = span!(Level::TRACE, "vitte_prof", name = name);
    (sp, SpanTimer::new(name))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn counters_and_timers() {
        ProfData::reset();
        incr("req", 1);
        incr("req", 2);
        {
            let _t = SpanTimer::new("op");
            std::thread::sleep(Duration::from_millis(5));
        }
        let snap = ProfData::snapshot();
        let c = snap.counters.get("req").unwrap();
        assert_eq!(c.count, 2);
        assert_eq!(c.sum, 3);
        let t = snap.timers.get("op").unwrap();
        assert!(t.samples >= 1);
        assert!(t.total_ns > 0);
    }

    #[cfg(feature = "serde")]
    #[test]
    fn json_export_ok() {
        ProfData::reset();
        incr("x", 7);
        let s = export_json().unwrap();
        assert!(s.contains("\"x\""));
    }
}
