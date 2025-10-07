#![deny(missing_docs)]
//! vitte-java — Interop Java/JVM pour Vitte.
//!
//! Fournit des abstractions sûres et portables pour interagir avec la JVM via JNI :
//! - Chargement dynamique d'une JVM embarquée (si activé).
//! - Conversion de types Rust <-> Java.
//! - Gestion des exceptions Java sous forme de `JavaError` Rust.
//! - Support Android via NDK.
//!
//! # Features
//! - `jni`: active l'intégration via le crate `jni`.
//! - `jvm-embed`: permet de démarrer une JVM embarquée côté desktop.
//! - `android`: désactive `jvm-embed` et suppose une JVM fournie par Android.
//! - `serde`: helpers de sérialisation pour passerelles JSON.

use thiserror::Error;

#[cfg(feature = "jni")]
pub use jni;

/// Erreurs d'interopération Java/JNI et gestion de JVM.
#[derive(Debug, Error)]
pub enum JavaError {
    /// Erreur provenant de la couche JNI (message brut).
    #[error("JNI error: {0}")]
    Jni(String),
    /// JVM absente ou non initialisée pour l'opération demandée.
    #[error("JVM not available")]
    JvmUnavailable,
    /// Échec de conversion de type entre Rust et Java.
    #[error("Type conversion error: {0}")]
    Conversion(String),
    /// Autre erreur générique (message descriptif).
    #[error("Other: {0}")]
    Other(String),
}

/// Résultat spécialisé pour les fonctions Java interop.
pub type Result<T> = std::result::Result<T, JavaError>;

/// Informations sur la JVM en cours.
#[derive(Debug, Clone)]
pub struct JvmInfo {
    /// Version de la JVM.
    pub version: String,
    /// Vendor (Oracle, OpenJDK…).
    pub vendor: String,
    /// Nom runtime.
    pub runtime: String,
}

/// API principale si feature `jni` activée.
#[cfg(feature = "jni")]
pub mod api {
    use super::*;
    use jni::JNIEnv;
    use jni::JavaVM;
    use jni::objects::{JString, JValue};

    /// Contexte principal d'interop.
    pub struct JavaCtx {
        vm: JavaVM,
    }

    impl JavaCtx {
        /// Crée un contexte à partir d'une VM existante.
        pub fn from_vm(vm: JavaVM) -> Self {
            Self { vm }
        }

        /// Récupère un `JNIEnv` pour le thread courant.
        pub fn env(&self) -> Result<JNIEnv<'_>> {
            self.vm.get_env().map_err(|e| JavaError::Jni(e.to_string()))
        }

        /// Construit une chaîne Java depuis une chaîne Rust.
        pub fn new_string(&self, s: &str) -> Result<JString<'_>> {
            let env = self.env()?;
            env.new_string(s).map_err(|e| JavaError::Jni(e.to_string()))
        }

        /// Appelle une méthode statique retournant une String.
        pub fn call_static_string(
            &self,
            class: &str,
            method: &str,
            sig: &str,
            args: &[JValue],
        ) -> Result<String> {
            let mut env = self.env()?;
            let cls = env.find_class(class).map_err(|e| JavaError::Jni(e.to_string()))?;
            let jstr = env
                .call_static_method(cls, method, sig, args)
                .map_err(|e| JavaError::Jni(e.to_string()))?
                .l()
                .map_err(|e| JavaError::Jni(e.to_string()))?;
            let s: String = env
                .get_string(&JString::from(jstr))
                .map_err(|e| JavaError::Jni(e.to_string()))?
                .into();
            Ok(s)
        }
    }
}

/// Helpers pour lancer une JVM embarquée (si feature `jvm-embed`).
#[cfg(feature = "jvm-embed")]
pub mod embed {
    use super::*;
    use jni::InitArgsBuilder;
    use jni::JavaVM;

    /// Lance une JVM embarquée avec des options par défaut.
    pub fn launch_jvm() -> Result<JavaVM> {
        let jvm_args = InitArgsBuilder::new()
            .version(jni::JNIVersion::V8)
            .build()
            .map_err(|e| JavaError::Other(e.to_string()))?;
        JavaVM::new(jvm_args).map_err(|e| JavaError::Other(e.to_string()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_display() {
        let e = JavaError::Conversion("bad type".into());
        assert!(format!("{}", e).contains("bad type"));
    }
}
