//! vitte-profiler — mesures de performance et traces pour Vitte
//!
//! Fournit :
//! - Profils par section de code (timer start/stop)
//! - Compteurs d’événements
//! - Export JSON (optionnel) ou format Chrome tracing (optionnel)
//!
//! Exemple :
//! ```ignore
//! use vitte_profiler::{Profiler, TimerGuard};
//!
//! let mut prof = Profiler::global();
//! {
//!     let _g = prof.start("compilation");
//!     // code
//! }
//! println!("{}", prof.to_json().unwrap());
//! ```

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use once_cell::sync::Lazy;
use parking_lot::RwLock;
use std::collections::HashMap;
use std::time::{Duration, Instant};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

static GLOBAL: Lazy<Profiler> = Lazy::new(Profiler::default);

/// Un évènement de profilage.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Event {
    pub name: String,
    pub start: Instant,
    pub duration: Duration,
}

impl Event {
    pub fn new(name: impl Into<String>, start: Instant, duration: Duration) -> Self {
        Self { name: name.into(), start, duration }
    }
}

/// Profiler global.
#[derive(Debug, Default)]
pub struct Profiler {
    events: RwLock<Vec<Event>>,
    counters: RwLock<HashMap<String, u64>>,
}

impl Profiler {
    pub fn global() -> &'static Self {
        &GLOBAL
    }

    pub fn start<'a>(&'a self, name: &str) -> TimerGuard<'a> {
        TimerGuard { prof: self, name: name.to_string(), start: Instant::now() }
    }

    pub fn record(&self, e: Event) {
        self.events.write().push(e);
    }

    pub fn inc(&self, name: &str) {
        *self.counters.write().entry(name.to_string()).or_insert(0) += 1;
    }

    pub fn snapshot(&self) -> (Vec<Event>, HashMap<String, u64>) {
        (self.events.read().clone(), self.counters.read().clone())
    }

    #[cfg(feature = "json")]
    pub fn to_json(&self) -> anyhow::Result<String> {
        let (events, counters) = self.snapshot();
        let data = serde_json::json!({
            "events": events.iter().map(|e| {
                serde_json::json!({
                    "name": e.name,
                    "duration_us": e.duration.as_micros(),
                })
            }).collect::<Vec<_>>(),
            "counters": counters
        });
        Ok(serde_json::to_string_pretty(&data)?)
    }

    #[cfg(feature = "chrome")]
    pub fn to_chrome_trace(&self) -> anyhow::Result<String> {
        let (events, _) = self.snapshot();
        let mut out = Vec::new();
        for e in events {
            let rec = serde_json::json!({
                "name": e.name,
                "ph": "X",
                "ts": e.start.elapsed().as_micros(),
                "dur": e.duration.as_micros(),
                "pid": 0,
                "tid": 0
            });
            out.push(rec);
        }
        Ok(serde_json::to_string_pretty(&out)?)
    }
}

/// Guard RAII pour timer.
pub struct TimerGuard<'a> {
    prof: &'a Profiler,
    name: String,
    start: Instant,
}

impl<'a> Drop for TimerGuard<'a> {
    fn drop(&mut self) {
        let dur = self.start.elapsed();
        self.prof.record(Event::new(&self.name, self.start, dur));
    }
}

// ================================ Tests ===================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn timer_records_event() {
        let p = Profiler::default();
        {
            let _g = p.start("foo");
        }
        let (evs, _) = p.snapshot();
        assert_eq!(evs.len(), 1);
        assert_eq!(evs[0].name, "foo");
    }

    #[test]
    fn counter_increments() {
        let p = Profiler::default();
        p.inc("calls");
        p.inc("calls");
        let (_, cs) = p.snapshot();
        assert_eq!(cs.get("calls"), Some(&2));
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_output() {
        let p = Profiler::default();
        {
            let _g = p.start("bar");
        }
        let s = p.to_json().unwrap();
        assert!(s.contains("events"));
    }
}
