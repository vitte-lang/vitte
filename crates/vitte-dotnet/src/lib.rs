//! vitte-dotnet — interop .NET pour Vitte
//!
//! Objectifs
//! - Héberger le runtime .NET Core via `nethost` / `hostfxr`.
//! - Appeler des méthodes managées depuis Rust.
//! - Fournir une API P/Invoke depuis .NET vers Rust.
//! - Intégrations optionnelles Windows COM/WinRT.
//!
//! Limitations
//! - Requiert `std`.
//! - Nécessite que .NET runtime (>=6.0) soit installé sur la machine.
//!
//! Exemple (pseudo-code):
//! ```ignore
//! use vitte_dotnet::host::{DotnetHost, RuntimeConfig};
//! let cfg = RuntimeConfig::new("MyApp.runtimeconfig.json");
//! let mut host = DotnetHost::init(cfg).unwrap();
//! let res: i32 = host.call_static("MyLib.dll", "MyNamespace.Type", "Method", "(System.Int32)", &[42i32.into()]).unwrap().try_into().unwrap();
//! println!("ret = {res}");
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]
#![forbid(unsafe_op_in_unsafe_fn)]

#[cfg(not(feature = "std"))]
compile_error!("vitte-dotnet nécessite std");

extern crate alloc;
use alloc::string::String;

/// Résultat alias.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Erreurs .NET interop.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Fichier absent ou invalide.
    #[error("invalid file: {0}")]
    InvalidFile(String),
    /// Erreur d’hôte .NET.
    #[error("hostfxr error: {0}")]
    Host(String),
    /// Fonction non trouvée.
    #[error("missing method: {0}")]
    Missing(String),
    /// Appel échoué.
    #[error("invoke failed: {0}")]
    Invoke(String),
    /// Fonctionnalité absente.
    #[error("unsupported: {0}")]
    Unsupported(&'static str),
}

/* ------------------------------------------------------------------------- */
/* Host .NET via hostfxr                                                      */
/* ------------------------------------------------------------------------- */

#[cfg(feature = "hostfxr")]
pub mod host {
    use super::*;
    use camino::Utf8PathBuf;
    use netcorehost::{hostfxr, nethost};
    use std::ptr;

    /// Configuration runtime .NET.
    #[derive(Debug, Clone)]
    pub struct RuntimeConfig {
        /// Chemin du fichier `.runtimeconfig.json`.
        pub path: Utf8PathBuf,
    }

    impl RuntimeConfig {
        /// Nouveau config depuis chemin.
        pub fn new<P: Into<Utf8PathBuf>>(p: P) -> Self {
            Self { path: p.into() }
        }
    }

    /// Instance du runtime .NET hébergé.
    pub struct DotnetHost {
        ctx: hostfxr::HostContext,
    }

    impl DotnetHost {
        /// Initialise le runtime.
        pub fn init(cfg: RuntimeConfig) -> Result<Self> {
            let ctx = hostfxr::HostContext::initialize_for_runtime_config(&cfg.path)
                .map_err(|e| Error::Host(format!("{e:?}")))?;
            ctx.run_app().map_err(|e| Error::Host(format!("{e:?}")))?;
            Ok(Self { ctx })
        }

        /// Appelle une méthode statique managée.
        pub fn call_static(
            &mut self,
            assembly: &str,
            type_name: &str,
            method: &str,
            sig: &str,
            args: &[DotnetValue],
        ) -> Result<DotnetValue> {
            // Simplifié: dans une vraie impl, il faut binder un delegate.
            // Ici: stub / démonstration.
            Err(Error::Unsupported("call_static stub; nécessite delegate binder"))
        }
    }

    /// Valeurs d’interop basiques.
    #[derive(Debug, Clone)]
    pub enum DotnetValue {
        /// Entier 32 bits.
        I32(i32),
        /// Entier 64 bits.
        I64(i64),
        /// Chaîne UTF-8.
        Str(String),
        /// Autres types.
        Other,
    }

    impl From<i32> for DotnetValue {
        fn from(v: i32) -> Self {
            DotnetValue::I32(v)
        }
    }
    impl From<i64> for DotnetValue {
        fn from(v: i64) -> Self {
            DotnetValue::I64(v)
        }
    }
    impl From<String> for DotnetValue {
        fn from(v: String) -> Self {
            DotnetValue::Str(v)
        }
    }
    impl From<&str> for DotnetValue {
        fn from(v: &str) -> Self {
            DotnetValue::Str(v.to_owned())
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Windows COM/WinRT optionnel                                               */
/* ------------------------------------------------------------------------- */

#[cfg(all(feature = "win-com", target_os = "windows"))]
pub mod win {
    use super::*;
    use windows::Win32::System::Com::*;
    use windows::Win32::System::WinRT::*;
    /// Initialise COM pour thread courant.
    pub fn init_com() -> Result<()> {
        unsafe {
            CoInitializeEx(None, COINIT_MULTITHREADED)
                .map_err(|e| Error::Host(format!("COM init: {e:?}")))?;
        }
        Ok(())
    }
}

/* ------------------------------------------------------------------------- */
/* IPC / Sérialisation                                                       */
/* ------------------------------------------------------------------------- */

#[cfg(feature = "serde")]
pub mod ipc {
    use super::*;
    use serde::{Deserialize, Serialize};

    /// Message IPC.
    #[derive(Debug, Serialize, Deserialize)]
    pub struct Message {
        /// Sujet.
        pub topic: String,
        /// Corps.
        pub body: String,
    }
}

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn error_fmt() {
        let e = Error::InvalidFile("nope".into());
        assert!(format!("{e}").contains("nope"));
    }
}
