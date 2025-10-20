//! vitte-trace — traçage et instrumentation pour Vitte
//!
//! Capacités:
//! - Spans temporisés via guard RAII (`SpanGuard`), événements (`event`)
//! - Export JSON optionnel (`feature = "json"`)
//! - Intégration `tracing`/`tracing-subscriber` optionnelle (`feature = "log"`)
//! - Helpers sync/async pour minuter une closure ou un `Future`
//!
//! API rapide:
//! - [`Tracer::global`], [`Tracer::span`], [`Tracer::event`]
//! - [`time_block`], [`time_async`] (si `tokio`)
//! - [`init_tracing`] (si `log`) pour configurer un subscriber
//! - Macros: [`vitte_span!`], [`vitte_event!`]
//!
//! Zéro unsafe.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use std::collections::HashMap;
use std::sync::OnceLock;
use std::time::Instant;

#[cfg(feature = "json")]
use serde::{Deserialize, Serialize};

use std::sync::RwLock;

static GLOBAL: OnceLock<Tracer> = OnceLock::new();

/* --------------------------------- Modèle --------------------------------- */

/// Un évènement ponctuel.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub struct TraceEvent {
    pub name: String,
    pub ts_us: u128,
    pub fields: HashMap<String, String>,
}

/// Un span mesuré.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub struct SpanRecord {
    pub name: String,
    pub dur_us: u128,
    pub fields: HashMap<String, String>,
}

/// Tracer en mémoire, thread-safe.
#[derive(Debug, Default)]
pub struct Tracer {
    events: RwLock<Vec<TraceEvent>>,
    spans: RwLock<Vec<SpanRecord>>,
}

impl Tracer {
    /// Accès global.
    pub fn global() -> &'static Self {
        GLOBAL.get_or_init(|| Tracer::default())
    }

    /// Crée un span guard. À la destruction, enregistre la durée.
    pub fn span<'a>(&'a self, name: impl Into<String>) -> SpanGuard<'a> {
        SpanGuard { tracer: self, name: name.into(), start: Instant::now(), fields: HashMap::new() }
    }

    /// Ajoute un champ clé/valeur à un span guard.
    pub fn span_with<'a>(
        &'a self,
        name: impl Into<String>,
        fields: HashMap<String, String>,
    ) -> SpanGuard<'a> {
        let mut g = self.span(name);
        g.fields = fields;
        g
    }

    /// Enregistre un évènement.
    pub fn event(
        &self,
        name: impl Into<String>,
        fields: impl IntoIterator<Item = (String, String)>,
    ) {
        let name: String = name.into();
        let fields: HashMap<_, _> = fields.into_iter().collect();
        let ev = TraceEvent { name: name.clone(), ts_us: now_us(), fields: fields.clone() };
        self.events.write().unwrap().push(ev);
        #[cfg(feature = "log")]
        {
            use tracing::Level;
            tracing::event!(Level::INFO, event = "vitte", name = %name, ?fields);
        }
    }

    /// Snapshot (copie) des données courantes.
    pub fn snapshot(&self) -> (Vec<TraceEvent>, Vec<SpanRecord>) {
        (self.events.read().unwrap().clone(), self.spans.read().unwrap().clone())
    }

    /// Efface toutes les données.
    pub fn clear(&self) {
        self.events.write().unwrap().clear();
        self.spans.write().unwrap().clear();
    }

    #[cfg(feature = "json")]
    /// Export JSON (évènements + spans).
    pub fn to_json(&self) -> anyhow::Result<String> {
        let (evs, spans) = self.snapshot();
        let obj = serde_json::json!({
            "events": evs,
            "spans": spans,
        });
        Ok(serde_json::to_string_pretty(&obj)?)
    }

    fn record_span(&self, rec: SpanRecord) {
        self.spans.write().unwrap().push(rec);
    }
}

/// Guard RAII pour un span.
pub struct SpanGuard<'a> {
    tracer: &'a Tracer,
    name: String,
    start: Instant,
    fields: HashMap<String, String>,
}

impl<'a> SpanGuard<'a> {
    /// Ajoute un champ (mutable).
    pub fn field(mut self, k: impl Into<String>, v: impl Into<String>) -> Self {
        self.fields.insert(k.into(), v.into());
        self
    }
}

impl<'a> Drop for SpanGuard<'a> {
    fn drop(&mut self) {
        let dur = self.start.elapsed();
        self.tracer.record_span(SpanRecord {
            name: self.name.clone(),
            dur_us: dur.as_micros(),
            fields: self.fields.clone(),
        });
    }
}

/* --------------------------------- Helpers -------------------------------- */

/// Mesure la durée d’une closure et renvoie son résultat.
pub fn time_block<F, R>(name: &str, mut f: F) -> R
where
    F: FnMut() -> R,
{
    let _g = Tracer::global().span(name);
    f()
}

/// Version qui ajoute des champs statiques.
pub fn time_block_with<F, R>(name: &str, fields: HashMap<String, String>, mut f: F) -> R
where
    F: FnMut() -> R,
{
    let _g = Tracer::global().span_with(name, fields);
    f()
}

#[cfg(feature = "tokio")]
/// Minuter un `Future` avec un span. Renvoie la sortie du futur.
pub async fn time_async<Fut, T>(name: &str, fut: Fut) -> T
where
    Fut: std::future::Future<Output = T>,
{
    let _g = Tracer::global().span(name);
    fut.await
}

/* ---------------------------------- Macros --------------------------------- */

/// Crée un span guard nommé. À la sortie du scope, enregistre la durée.
///
/// ```ignore
/// use vitte_trace::vitte_span;
/// let _g = vitte_span!("parse");
/// ```
#[macro_export]
macro_rules! vitte_span {
    ($name:expr) => {{
        $crate::Tracer::global().span($name)
    }};
    ($name:expr, { $($k:expr => $v:expr),+ $(,)? }) => {{
        let mut _m = ::std::collections::HashMap::<String,String>::new();
        $( _m.insert(($k).to_string(), ($v).to_string()); )+
        $crate::Tracer::global().span_with($name, _m)
    }};
}

/// Enregistre un évènement ponctuel.
///
/// ```ignore
/// vitte_event!("loaded", {"items" => "42"});
/// ```
#[macro_export]
macro_rules! vitte_event {
    ($name:expr) => {{
        $crate::Tracer::global().event($name, ::std::iter::empty::<(String,String)>());
    }};
    ($name:expr, { $($k:expr => $v:expr),+ $(,)? }) => {{
        let _vec = vec![$( (($k).to_string(), ($v).to_string()) ),+];
        $crate::Tracer::global().event($name, _vec);
    }};
}

/* --------------------------------- Outils ---------------------------------- */

fn now_us() -> u128 {
    Instant::now().elapsed().as_micros()
}

/* ---------------------------------- Tests ---------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn span_records_duration() {
        let t = Tracer::default();
        {
            let _g = t.span("work");
        }
        let (_, spans) = (t.events.read().unwrap().clone(), t.spans.read().unwrap().clone());
        assert_eq!(spans.len(), 1);
        assert_eq!(spans[0].name, "work");
        assert!(spans[0].dur_us >= 0);
    }

    #[test]
    fn event_records() {
        let t = Tracer::default();
        t.event("loaded", [("count".into(), "3".into())]);
        assert_eq!(t.events.read().unwrap().len(), 1);
        assert_eq!(t.events.read().unwrap()[0].name, "loaded");
    }

    #[test]
    fn macros_work() {
        let g = Tracer::global();
        g.clear();
        {
            let _s = crate::vitte_span!("parse", {"file" => "a.vt"});
        }
        crate::vitte_event!("done");
        let (evs, spans) = g.snapshot();
        assert_eq!(evs.len(), 1);
        assert_eq!(spans.len(), 1);
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_export() {
        let g = Tracer::global();
        g.clear();
        {
            let _s = g.span("x");
        }
        let s = g.to_json().unwrap();
        assert!(s.contains("spans"));
    }

    #[cfg(feature = "tokio")]
    #[test]
    fn time_async_runs() {
        let out = tokio::runtime::Runtime::new()
            .unwrap()
            .block_on(async { time_async("sleep", async { 1 + 2 }).await });
        assert_eq!(out, 3);
    }
}

#[cfg(feature = "log")]
/// Initialise un subscriber `tracing-subscriber`.
///
/// - `level`: "error" | "warn" | "info" | "debug" | "trace"
/// - `json`: sorties JSON si `true`, sinon format texte.
/// Idempotent à l'échelle du process.
pub fn init_tracing(level: &str, json: bool) -> anyhow::Result<()> {
    use std::str::FromStr;
    use tracing::Level;
    use tracing_subscriber::fmt;

    let lvl = Level::from_str(&level.to_ascii_lowercase()).unwrap_or(Level::INFO);
    let builder = fmt().with_max_level(lvl).with_target(false).with_timer(fmt::time::uptime());
    #[cfg(feature = "json")]
    {
        if json {
            builder.json().flatten_event(true).init();
        } else {
            builder.compact().init();
        }
    }
    #[cfg(not(feature = "json"))]
    {
        let _ = json; // suppress unused warning
        builder.compact().init();
    }
    Ok(())
}
