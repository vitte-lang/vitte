#![deny(missing_docs)]
//! vitte-plugin — plugin de référence pour l'ABI Vitte
//!
//! Expose les symboles attendus par `vitte-loader` :
//! - `vitte_plugin_desc() -> *const VittePluginDesc`
//! - `vitte_plugin_init() -> i32` (0 = OK)
//! - `vitte_plugin_shutdown()`
//!
//! Fournit en plus :
//! - État interne thread-safe avec horodatage et compteur d'initialisation
//! - Journalisation optionnelle via `vitte-logs` (feature `logs`)
//! - Configuration optionnelle via JSON (feature `serde`) avec `vitte_plugin_config(json)`
//! - Contrôle de santé minimal via `vitte_plugin_health() -> i32`
//!
//! Ce plugin est auto‑contenu et peut servir de squelette pour d’autres plugins Vitte.

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::OnceLock;
use std::time::{Duration, Instant};

use vitte_loader::VittePluginDesc;

#[cfg(feature = "logs")]
use vitte_logs as vlog;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Version sémantique du plugin.
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Nom logique.
pub const NAME: &str = "vitte-plugin";

/// Vendor.
pub const VENDOR: &str = "vitte";

/// Description courte.
pub const DESCRIPTION: &str = "Plugin de démonstration Vitte (référence ABI, logging, configuration).";

/// État interne du plugin.
#[derive(Debug)]
struct PluginState {
    started_at: Instant,
    init_count: AtomicUsize,
    #[cfg(feature = "serde")]
    config: OnceLock<Config>,
}

impl PluginState {
    fn new() -> Self {
        Self {
            started_at: Instant::now(),
            init_count: AtomicUsize::new(0),
            #[cfg(feature = "serde")]
            config: OnceLock::new(),
        }
    }
}

/// Configuration optionnelle du plugin (si `serde`).
#[cfg(feature = "serde")]
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct Config {
    /// Niveau de log par défaut: "error","warn","info","debug","trace".
    #[serde(default = "default_log")]
    pub log_level: String,
    /// Payload configurable pour la logique métier du plugin.
    #[serde(default)]
    pub payload: serde_json::Value,
}
#[cfg(feature = "serde")]
fn default_log() -> String {
    "info".to_owned()
}

/// Singleton d’état global.
static STATE: OnceLock<PluginState> = OnceLock::new();

/// Descripteur statique exporté.
static DESC: OnceLock<VittePluginDesc> = OnceLock::new();

fn ensure_desc() -> &'static VittePluginDesc {
    DESC.get_or_init(|| {
        VittePluginDesc::from_strs(NAME, VERSION, VENDOR, DESCRIPTION)
            .expect("VittePluginDesc initialization should not fail")
    })
}

#[cfg(feature = "logs")]
fn init_logs_from_env() {
    // VITTE_PLUGIN_LOG=error|warn|info|debug|trace
    let lvl = std::env::var("VITTE_PLUGIN_LOG").unwrap_or_else(|_| "info".to_string());
    let level = match lvl.to_ascii_lowercase().as_str() {
        "error" => vlog::Level::Error,
        "warn" => vlog::Level::Warn,
        "debug" => vlog::Level::Debug,
        "trace" => vlog::Level::Trace,
        _ => vlog::Level::Info,
    };
    // Initialise le backend si disponible.
    vlog::init(level);
    vlog::info!("{} v{} starting with level={}", NAME, VERSION, level);
}

/// Fonction interne d'initialisation.
fn do_init() -> Result<(), String> {
    let st = STATE.get_or_init(PluginState::new);
    let n = st.init_count.fetch_add(1, Ordering::SeqCst);
    if n == 0 {
        #[cfg(feature = "logs")]
        init_logs_from_env();

        #[cfg(feature = "logs")]
        vlog::info!("{} initialized (first init)", NAME);
    } else {
        #[cfg(feature = "logs")]
        vlog::debug!("{} additional init call (count = {})", NAME, n + 1);
    }
    Ok(())
}

/// Fonction interne de shutdown.
fn do_shutdown() {
    if let Some(st) = STATE.get() {
        let uptime = st.started_at.elapsed();
        #[cfg(feature = "logs")]
        vlog::info!("{} shutting down after {:?}", NAME, uptime);
    }
}

/// Exporte un pointeur vers le descripteur du plugin.
///
/// # Safety
/// Utilisation FFI. Le pointeur reste valide pendant la durée du processus.
#[no_mangle]
pub extern "C" fn vitte_plugin_desc() -> *const VittePluginDesc {
    ensure_desc() as *const VittePluginDesc
}

/// Point d’entrée d’initialisation (0 = succès).
#[no_mangle]
pub extern "C" fn vitte_plugin_init() -> c_int {
    match do_init() {
        Ok(()) => 0,
        Err(e) => {
            #[cfg(feature = "logs")]
            vlog::error!("init error: {}", e);
            1
        }
    }
}

/// Fermeture du plugin.
#[no_mangle]
pub extern "C" fn vitte_plugin_shutdown() {
    do_shutdown();
}

/// Renvoie 0 si OK, !=0 si dégradé. Simple health check pour orchestrateurs.
#[no_mangle]
pub extern "C" fn vitte_plugin_health() -> c_int {
    if STATE.get().is_some() { 0 } else { 2 }
}

/// Renvoie un C string alloué décrivant l’état (uptime en secondes).
/// L’appelant doit libérer via `vitte_plugin_string_free`.
#[no_mangle]
pub extern "C" fn vitte_plugin_status() -> *mut c_char {
    let uptime = STATE
        .get()
        .map(|s| s.started_at.elapsed())
        .unwrap_or(Duration::from_secs(0));
    let s = format!("name={NAME}; version={VERSION}; uptime={:.3}s", uptime.as_secs_f64());
    CString::new(s).map(|c| c.into_raw()).unwrap_or(std::ptr::null_mut())
}

/// Libère une chaîne retournée par ce plugin.
#[no_mangle]
pub extern "C" fn vitte_plugin_string_free(ptr: *mut c_char) {
    if ptr.is_null() { return; }
    unsafe { let _ = CString::from_raw(ptr); }
}

/// Configure le plugin via JSON (optionnel, feature `serde`).
/// Retourne 0 si succès, 1 sinon.
/// Exemple de payload:
/// `{ "log_level": "debug", "payload": { "foo": 1 } }`
#[no_mangle]
pub extern "C" fn vitte_plugin_config(json: *const c_char) -> c_int {
    #[cfg(not(feature = "serde"))]
    {
        let _ = json;
        return 1;
    }
    #[cfg(feature = "serde")]
    {
        if json.is_null() {
            #[cfg(feature = "logs")]
            vlog::warn!("config called with null pointer");
            return 1;
        }
        let c = unsafe { CStr::from_ptr(json) };
        let s = match c.to_str() {
            Ok(v) => v,
            Err(e) => {
                #[cfg(feature = "logs")]
                vlog::error!("config invalid utf8: {}", e);
                return 1;
            }
        };
        match serde_json::from_str::<Config>(s) {
            Ok(conf) => {
                let st = STATE.get_or_init(PluginState::new);
                let _ = st.config.set(conf.clone());
                #[cfg(feature = "logs")]
                {
                    let lvl = conf.log_level.to_ascii_lowercase();
                    let level = match lvl.as_str() {
                        "error" => vlog::Level::Error,
                        "warn" => vlog::Level::Warn,
                        "debug" => vlog::Level::Debug,
                        "trace" => vlog::Level::Trace,
                        _ => vlog::Level::Info,
                    };
                    vlog::init(level);
                    vlog::info!("config applied: level={}, payload={}", level, conf.payload);
                }
                0
            }
            Err(e) => {
                #[cfg(feature = "logs")]
                vlog::error!("config parse error: {}", e);
                1
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn desc_non_null() {
        let p = vitte_plugin_desc();
        assert!(!p.is_null());
    }

    #[test]
    fn init_and_health() {
        assert_eq!(vitte_plugin_health(), 2);
        assert_eq!(vitte_plugin_init(), 0);
        assert_eq!(vitte_plugin_health(), 0);
    }

    #[test]
    fn status_string_and_free() {
        let ptr = vitte_plugin_status();
        assert!(!ptr.is_null());
        unsafe {
            let s = CStr::from_ptr(ptr);
            assert!(s.to_str().unwrap().contains("name=vitte-plugin"));
        }
        vitte_plugin_string_free(ptr);
    }

    #[cfg(feature = "serde")]
    #[test]
    fn apply_config() {
        let js = r#"{ "log_level": "debug", "payload": {"x":42} }"#;
        let c = CString::new(js).unwrap();
        let rc = vitte_plugin_config(c.as_ptr());
        assert_eq!(rc, 0);
    }
}
