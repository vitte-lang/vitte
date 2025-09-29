//! vitte-logger — initialisation et utilitaires `tracing` pour Vitte.
//!
//! Objectif:
//! - Démarrage idempotent du subscriber.
//! - Config via `RUST_LOG` si la feature `env-filter` est activée.
//! - Fallback simple sans `env-filter`.
//!
//! Usage rapide:
//! ```rust
//! vitte_logger::init();               // lit RUST_LOG si feature env-filter
//! tracing::info!("ready");
//!
//! #[cfg(feature = "env-filter")]
//! vitte_logger::init_with("vitte=debug,info");
//! ```
//!
//! Features:
//! - `std`        : activée par défaut. Requis pour `tracing-subscriber`.
//! - `env-filter` : active le contrôle fin via `RUST_LOG` ou spécification explicite.
//!
//! Notes:
//! - En `no_std` (feature `std` désactivée), l'init est un no-op sûr et le code compile,
//!   mais sans sortie formatée (pas de `tracing-subscriber`).

#![deny(missing_docs)]
#![forbid(unsafe_code)]
#![cfg_attr(not(feature = "std"), no_std)]

/* ─────────────────────────── Imports ─────────────────────────── */

use core::sync::OnceLock;

pub use tracing::{debug, error, info, trace, warn, Level};
pub use tracing::{event, instrument, span};

/* ─────────────────────────── État global ─────────────────────── */

static STARTED: OnceLock<()> = OnceLock::new();

/* ─────────────────────────── API publique ────────────────────── */

/// Initialise le logger une seule fois.
///
/// Comportement:
/// - Avec `env-filter`: tente `RUST_LOG`, sinon `info`.
/// - Sans `env-filter`: niveau max `INFO`.
///
/// Sûr à appeler plusieurs fois. Les appels suivants n’ont aucun effet.
pub fn init() {
    // En no_std: no-op idempotent.
    #[cfg(not(feature = "std"))]
    {
        let _ = STARTED.set(());
        return;
    }

    #[cfg(feature = "std")]
    {
        if STARTED.get().is_some() {
            return;
        }

        #[cfg(feature = "env-filter")]
        {
            use tracing_subscriber::{fmt, layer::SubscriberExt, EnvFilter, Registry};

            let filter =
                EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));

            let fmt_layer = fmt::layer()
                .with_target(true)
                .with_thread_ids(false)
                .with_thread_names(false)
                .with_ansi(stdio_is_tty_stderr());

            let subscriber = Registry::default().with(filter).with(fmt_layer);

            if tracing::subscriber::set_global_default(subscriber).is_ok() {
                let _ = STARTED.set(());
            }
            return;
        }

        // Sans env-filter: fallback minimal
        #[cfg(not(feature = "env-filter"))]
        {
            use tracing_subscriber::fmt;

            // ignore l’erreur si déjà initialisé ailleurs
            let _ = fmt()
                .with_target(true)
                .with_max_level(Level::INFO)
                .with_ansi(stdio_is_tty_stderr())
                .try_init();

            let _ = STARTED.set(());
        }
    }
}

/// Initialise avec une spécification explicite de filtre, ex: `"vitte=debug,info"`.
/// Nécessite la feature `env-filter`.
///
/// Sans effet si déjà initialisé.
#[cfg(all(feature = "std", feature = "env-filter"))]
pub fn init_with(spec: &str) {
    if STARTED.get().is_some() {
        return;
    }
    use tracing_subscriber::{fmt, layer::SubscriberExt, EnvFilter, Registry};

    let filter = EnvFilter::try_new(spec).unwrap_or_else(|_| EnvFilter::new("info"));

    let fmt_layer = fmt::layer()
        .with_target(true)
        .with_thread_ids(false)
        .with_thread_names(false)
        .with_ansi(stdio_is_tty_stderr());

    let subscriber = Registry::default().with(filter).with(fmt_layer);

    if tracing::subscriber::set_global_default(subscriber).is_ok() {
        let _ = STARTED.set(());
    }
}

/// Retourne vrai si un subscriber global a été installé par ce crate.
pub fn is_initialized() -> bool {
    STARTED.get().is_some()
}

/* ─────────────────────────── Internes ────────────────────────── */

#[cfg(feature = "std")]
#[inline]
fn stdio_is_tty_stderr() -> bool {
    // Disponible depuis Rust 1.70+
    std::io::IsTerminal::is_terminal(&std::io::stderr())
}

#[cfg(not(feature = "std"))]
#[inline]
fn stdio_is_tty_stderr() -> bool {
    false
}

/* ─────────────────────────── Tests ───────────────────────────── */

#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;

    #[test]
    fn idempotent() {
        init();
        init();
        assert!(is_initialized());
        info!("logger ok");
    }

    #[cfg(feature = "env-filter")]
    #[test]
    fn with_spec() {
        // Doit être appelé avant un éventuel init() par d’autres tests.
        init_with("warn");
        assert!(is_initialized());
        warn!("visible");
        debug!("masqué avec 'warn'");
    }
}
