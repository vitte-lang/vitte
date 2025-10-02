#![deny(missing_docs)]
//! vitte-logs — système de logs unifié pour Vitte
//!
//! Objectifs :
//! - Fournir une API simple, avec macros (`info!`, `warn!`, etc.).
//! - Support des backends `tracing` et `log`.
//! - Zero-cost si les features sont désactivées.
//! - Configurable par feature (par défaut `std` + macros).
//! - Fournir une initialisation avec niveaux filtrables.
//!
//! Exemple :
//! ```
//! use vitte_logs as vlog;
//! vlog::init(Level::Debug);
//! vlog::info!("hello {}", 42);
//! ```

use core::fmt;

#[cfg(feature = "log")]
pub use log;

#[cfg(feature = "tracing")]
pub use tracing;

/// Niveau de log.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum Level {
    /// Erreur bloquante.
    Error,
    /// Avertissement.
    Warn,
    /// Information générale.
    Info,
    /// Débogage.
    Debug,
    /// Trace détaillée.
    Trace,
    /// Niveau désactivé.
    Off,
}

impl fmt::Display for Level {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Level::Error => write!(f, "ERROR"),
            Level::Warn  => write!(f, "WARN"),
            Level::Info  => write!(f, "INFO"),
            Level::Debug => write!(f, "DEBUG"),
            Level::Trace => write!(f, "TRACE"),
            Level::Off   => write!(f, "OFF"),
        }
    }
}

impl Level {
    /// Convertit en LevelFilter du crate log (si activé).
    #[cfg(feature="log")]
    pub fn to_log(self) -> log::LevelFilter {
        match self {
            Level::Error => log::LevelFilter::Error,
            Level::Warn  => log::LevelFilter::Warn,
            Level::Info  => log::LevelFilter::Info,
            Level::Debug => log::LevelFilter::Debug,
            Level::Trace => log::LevelFilter::Trace,
            Level::Off   => log::LevelFilter::Off,
        }
    }
    /// Convertit en niveau tracing (si activé).
    #[cfg(feature="tracing")]
    pub fn to_tracing(self) -> tracing::Level {
        match self {
            Level::Error => tracing::Level::ERROR,
            Level::Warn  => tracing::Level::WARN,
            Level::Info  => tracing::Level::INFO,
            Level::Debug => tracing::Level::DEBUG,
            Level::Trace => tracing::Level::TRACE,
            Level::Off   => tracing::Level::TRACE, // fallback, Off handled externally
        }
    }
}

/// Macros de log unifiées.
#[macro_export]
macro_rules! error {
    ($($arg:tt)*) => {{
        #[cfg(feature="tracing")] { tracing::error!($($arg)*); }
        #[cfg(all(not(feature="tracing"), feature="log"))] { log::error!($($arg)*); }
        #[cfg(all(not(feature="tracing"), not(feature="log")))] { let _ = format!($($arg)*); }
    }};
}
#[macro_export]
macro_rules! warn {
    ($($arg:tt)*) => {{
        #[cfg(feature="tracing")] { tracing::warn!($($arg)*); }
        #[cfg(all(not(feature="tracing"), feature="log"))] { log::warn!($($arg)*); }
        #[cfg(all(not(feature="tracing"), not(feature="log")))] { let _ = format!($($arg)*); }
    }};
}
#[macro_export]
macro_rules! info {
    ($($arg:tt)*) => {{
        #[cfg(feature="tracing")] { tracing::info!($($arg)*); }
        #[cfg(all(not(feature="tracing"), feature="log"))] { log::info!($($arg)*); }
        #[cfg(all(not(feature="tracing"), not(feature="log")))] { let _ = format!($($arg)*); }
    }};
}
#[macro_export]
macro_rules! debug {
    ($($arg:tt)*) => {{
        #[cfg(feature="tracing")] { tracing::debug!($($arg)*); }
        #[cfg(all(not(feature="tracing"), feature="log"))] { log::debug!($($arg)*); }
        #[cfg(all(not(feature="tracing"), not(feature="log")))] { let _ = format!($($arg)*); }
    }};
}
#[macro_export]
macro_rules! trace {
    ($($arg:tt)*) => {{
        #[cfg(feature="tracing")] { tracing::trace!($($arg)*); }
        #[cfg(all(not(feature="tracing"), feature="log"))] { log::trace!($($arg)*); }
        #[cfg(all(not(feature="tracing"), not(feature="log")))] { let _ = format!($($arg)*); }
    }};
}

/// Initialise selon le niveau souhaité.
pub fn init(level: Level) {
    #[cfg(feature="log")]
    {
        log::set_max_level(level.to_log());
    }
    #[cfg(feature="tracing")]
    {
        use tracing_subscriber::prelude::*;
        let fmt_layer = tracing_subscriber::fmt::layer().with_filter(tracing_subscriber::filter::LevelFilter::from_level(level.to_tracing()));
        tracing_subscriber::registry()
            .with(fmt_layer)
            .init();
    }
}

/// Initialisation simple pour backend choisi.
#[cfg(feature="log")]
pub fn init_log() {
    log::set_max_level(Level::Info.to_log());
}

#[cfg(feature="tracing")]
pub fn init_tracing_default() {
    use tracing_subscriber::prelude::*;
    let fmt_layer = tracing_subscriber::fmt::layer();
    tracing_subscriber::registry()
        .with(fmt_layer)
        .init();
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn level_display() {
        assert_eq!(Level::Error.to_string(), "ERROR");
        assert_eq!(Level::Info.to_string(), "INFO");
        assert_eq!(Level::Off.to_string(), "OFF");
    }
}
