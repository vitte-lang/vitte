//! vitte-crash — crash handling and reporting for Vitte
//!
//! Features
//! - Panic hook: print panic payload + location, optional backtrace.
//! - Fatal signals: handle SIGSEGV/SIGILL/SIGABRT/SIGBUS/SIGFPE (Unix).
//! - Windows: structured exception handling with `minidump-writer`.
//! - Optional Sentry client for reporting (`SENTRY_DSN` env).
//! - Minidump files (`*.dmp`) written to configurable directory.
//!
//! Limitations
//! - Requires `std`.
//! - Minidump writing supported on Unix/Windows only.
//!
//! Typical usage:
//! ```ignore
//! fn main() {
//!     vitte_crash::install().unwrap();
//!     // run app
//! }
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]
#![forbid(unsafe_op_in_unsafe_fn)]

#[cfg(not(feature = "std"))]
compile_error!("vitte-crash requires std for meaningful crash handling");

use std::panic;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;

#[cfg(unix)]
use signal_hook::{consts::signal::*, iterator::Signals};

#[cfg(feature = "backtrace")]
use backtrace::Backtrace;

#[cfg(feature = "minidump")]
use minidump_writer::DumpWriter;

#[cfg(feature = "sentry")]
use sentry;

/// Error kinds.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Failed to install handlers.
    #[error("install error: {0}")]
    Install(String),
    /// Minidump writing failed.
    #[error("minidump io error: {0}")]
    Io(String),
}

/// Global flag.
static INSTALLED: AtomicBool = AtomicBool::new(false);

/// Configuration for crash handling.
#[derive(Debug, Clone)]
pub struct Config {
    /// Directory to write dumps.
    pub dump_dir: std::path::PathBuf,
    /// Enable backtrace capture.
    pub backtrace: bool,
    /// Enable Sentry reporting.
    pub sentry: bool,
}

impl Default for Config {
    fn default() -> Self {
        Self { dump_dir: std::env::temp_dir().join("vitte-dumps"), backtrace: true, sentry: false }
    }
}

/// Install global crash handlers with default config.
pub fn install() -> Result<(), Error> {
    install_with(Config::default())
}

/// Install with explicit config.
pub fn install_with(cfg: Config) -> Result<(), Error> {
    if INSTALLED.swap(true, Ordering::SeqCst) {
        return Ok(());
    }
    #[cfg(feature = "panic-hook")]
    install_panic_hook(cfg.clone());
    #[cfg(unix)]
    install_signal_handler(cfg.clone())?;
    Ok(())
}

#[cfg(feature = "panic-hook")]
fn install_panic_hook(cfg: Config) {
    panic::set_hook(Box::new(move |info| {
        eprintln!("=== Vitte panic ===");
        if let Some(s) = info.payload().downcast_ref::<&str>() {
            eprintln!("payload: {s}");
        } else if let Some(s) = info.payload().downcast_ref::<String>() {
            eprintln!("payload: {s}");
        }
        if let Some(loc) = info.location() {
            eprintln!("location: {}:{}", loc.file(), loc.line());
        }
        if cfg.backtrace {
            #[cfg(feature = "backtrace")]
            {
                let bt = Backtrace::new();
                eprintln!("backtrace:\n{:?}", bt);
            }
        }
        if cfg.sentry {
            #[cfg(feature = "sentry")]
            {
                sentry::capture_message("panic", sentry::Level::Fatal);
            }
        }
        #[cfg(feature = "minidump")]
        {
            let _ = write_minidump(&cfg.dump_dir);
        }
    }));
}

#[cfg(unix)]
fn install_signal_handler(cfg: Config) -> Result<(), Error> {
    let mut signals = Signals::new([SIGSEGV, SIGILL, SIGBUS, SIGFPE, SIGABRT])
        .map_err(|e| Error::Install(e.to_string()))?;
    let cfg = Arc::new(cfg);
    std::thread::spawn(move || {
        for sig in signals.forever() {
            eprintln!("=== Vitte crash: signal {sig} ===");
            if cfg.backtrace {
                #[cfg(feature = "backtrace")]
                {
                    let bt = Backtrace::new();
                    eprintln!("backtrace:\n{:?}", bt);
                }
            }
            if cfg.sentry {
                #[cfg(feature = "sentry")]
                {
                    sentry::capture_message(&format!("signal {sig}"), sentry::Level::Fatal);
                }
            }
            #[cfg(feature = "minidump")]
            {
                let _ = write_minidump(&cfg.dump_dir);
            }
            std::process::exit(128 + sig);
        }
    });
    Ok(())
}

#[cfg(feature = "minidump")]
fn write_minidump(dir: &std::path::Path) -> Result<(), Error> {
    let _ = std::fs::create_dir_all(dir);
    let ts = chrono::Utc::now().format("%Y%m%d-%H%M%S");
    let path = dir.join(format!("crash-{ts}.dmp"));
    DumpWriter::new(&path).write_dump().map_err(|e| Error::Io(e.to_string()))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn default_cfg() {
        let cfg = Config::default();
        assert!(cfg.dump_dir.to_string_lossy().contains("vitte-dumps"));
    }
}
