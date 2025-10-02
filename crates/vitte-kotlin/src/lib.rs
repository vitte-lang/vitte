#![deny(missing_docs)]
//! vitte-kotlin — Interop Kotlin/JVM pour Vitte
//!
//! Fournit des abstractions sûres pour :
//! - JNI bas niveau (via crate `jni`).
//! - Conversions Rust <-> Kotlin types de base (String, ByteArray, Option).
//! - Appels statiques et d’instance.
//! - Gestion exceptions Kotlin/Java.
//! - Lancement JVM embarquée (feature `jvm-embed`).
//!
//! # Features
//! - `jni`: active l'intégration via le crate `jni`.
//! - `jvm-embed`: démarre une JVM embarquée (desktop/serveur).
//! - `android`: cible Android NDK (pas de jvm-embed).
//! - `serde`: conversions JSON optionnelles.

use thiserror::Error;

#[cfg(feature = "jni")]
pub use jni;

/// Erreurs JVM/JNI.
#[derive(Debug, Error)]
pub enum KotlinError {
    #[error("JNI error: {0}")] Jni(String),
    #[error("Class not found: {0}")] ClassNotFound(String),
    #[error("Method not found: {class}.{method}{sig}")] MethodNotFound { class: String, method: String, sig: String },
    #[error("Field not found: {class}.{field}")] FieldNotFound { class: String, field: String },
    #[error("Java exception thrown: {0}")] JavaException(String),
    #[error("JVM not available")] JvmUnavailable,
    #[error("Conversion error: {0}")] Conversion(String),
    #[error("Invalid argument: {0}")] InvalidArg(String),
    #[error("Null pointer")] NullPtr,
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, KotlinError>;

/// Infos de runtime JVM.
#[derive(Debug, Clone)]
pub struct JvmInfo {
    /// Version.
    pub version: String,
    /// Vendor.
    pub vendor: String,
    /// Runtime.
    pub runtime: String,
}

#[cfg(feature = "jni")]
pub mod api {
    use super::*;
    use jni::objects::{GlobalRef, JByteArray, JClass, JFieldID, JMethodID, JObject, JString, JValue};
    use jni::signature::{Primitive, ReturnType};
    use jni::{JNIEnv, JavaVM};

    /// Contexte d'interop sûr.
    pub struct KotlinCtx {
        vm: JavaVM,
    }

    impl KotlinCtx {
        /// Construit depuis une VM existante.
        pub fn from_vm(vm: JavaVM) -> Self { Self { vm } }

        /// Récupère un `JNIEnv`.
        pub fn env(&self) -> Result<JNIEnv<'_>> { self.vm.get_env().map_err(|e| KotlinError::Jni(e.to_string())) }

        /// Vérifie et remonte une exception Java éventuelle.
        pub fn check_exception(&self, env: &JNIEnv<'_>) -> Result<()> {
            if env.exception_check().map_err(|e| KotlinError::Jni(e.to_string()))? {
                env.exception_describe().ok();
                env.exception_clear().ok();
                return Err(KotlinError::JavaException("java.lang.Throwable".into()));
            }
            Ok(())
        }

        /// Trouve une classe et retourne un GlobalRef.
        pub fn find_class_global(&self, name: &str) -> Result<GlobalRef> {
            let env = self.env()?;
            let cls = env.find_class(name).map_err(|_| KotlinError::ClassNotFound(name.into()))?;
            env.new_global_ref(JObject::from(cls)).map_err(|e| KotlinError::Jni(e.to_string()))
        }

        /// Conversion String Rust -> Java String.
        pub fn new_string(&self, s: &str) -> Result<JString<'_>> {
            let env = self.env()?;
            env.new_string(s).map_err(|e| KotlinError::Jni(e.to_string()))
        }

        /// Conversion Java String -> Rust String.
        pub fn to_rust_string(&self, js: JString<'_>) -> Result<String> {
            let env = self.env()?;
            env.get_string(&js).map(|os| os.into()).map_err(|e| KotlinError::Jni(e.to_string()))
        }

        /// ByteArray -> Vec<u8>.
        pub fn bytearray_to_vec(&self, arr: JByteArray<'_>) -> Result<Vec<u8>> {
            let env = self.env()?;
            let len = env.get_array_length(&arr).map_err(|e| KotlinError::Jni(e.to_string()))? as usize;
            let mut buf = vec![0u8; len];
            env.get_byte_array_region(&arr, 0, &mut buf).map_err(|e| KotlinError::Jni(e.to_string()))?;
            Ok(buf)
        }

        /// Vec<u8> -> ByteArray.
        pub fn vec_to_bytearray(&self, v: &[u8]) -> Result<JByteArray<'_>> {
            let env = self.env()?;
            let arr = env.new_byte_array(v.len() as i32).map_err(|e| KotlinError::Jni(e.to_string()))?;
            env.set_byte_array_region(&arr, 0, v).map_err(|e| KotlinError::Jni(e.to_string()))?;
            Ok(arr)
        }

        /// Récupère une propriété système.
        pub fn system_property(&self, key: &str) -> Result<Option<String>> {
            let env = self.env()?;
            let k: JString = env.new_string(key).map_err(|e| KotlinError::Jni(e.to_string()))?;
            let cls = env.find_class("java/lang/System").map_err(|_| KotlinError::ClassNotFound("java/lang/System".into()))?;
            let v = env.call_static_method(cls, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;", &[JValue::Object(&k.into())])
                .map_err(|e| KotlinError::Jni(e.to_string()))?;
            self.check_exception(&env)?;
            if let Ok(o) = v.l() {
                if !o.is_null() {
                    let s: String = env.get_string(&JString::from(o)).map_err(|e| KotlinError::Jni(e.to_string()))?.into();
                    return Ok(Some(s));
                }
            }
            Ok(None)
        }

        /// Obtient un champ d'instance i32.
        pub fn get_field_i32(&self, obj: JObject<'_>, field: &str, sig: &str) -> Result<i32> {
            let env = self.env()?;
            let cls = env.get_object_class(&obj).map_err(|e| KotlinError::Jni(e.to_string()))?;
            let fid = env.get_field_id(cls, field, sig).map_err(|_| KotlinError::FieldNotFound { class: "<obj>".into(), field: field.into() })?;
            let val = env.get_field_unchecked(obj, fid, ReturnType::Primitive(Primitive::Int)).map_err(|e| KotlinError::Jni(e.to_string()))?;
            val.i().map_err(|e| KotlinError::Jni(e.to_string()))
        }

        /// Appelle une méthode d'instance retournant String.
        pub fn call_instance_string(&self, obj: JObject<'_>, method: &str, sig: &str, args: &[JValue]) -> Result<String> {
            let env = self.env()?;
            let cls = env.get_object_class(&obj).map_err(|e| KotlinError::Jni(e.to_string()))?;
            let mid: JMethodID = env.get_method_id(cls, method, sig).map_err(|_| KotlinError::MethodNotFound { class: "<obj>".into(), method: method.into(), sig: sig.into() })?;
            let j = env.call_method_unchecked(obj, mid, ReturnType::Object, args).map_err(|e| KotlinError::Jni(e.to_string()))?;
            self.check_exception(&env)?;
            let s = env.get_string(&JString::from(j.l().map_err(|e| KotlinError::Jni(e.to_string()))?)).map_err(|e| KotlinError::Jni(e.to_string()))?;
            Ok(s.into())
        }

        /// Appel statique retournant un i32.
        pub fn call_static_i32(&self, class: &str, method: &str, sig: &str, args: &[JValue]) -> Result<i32> {
            let env = self.env()?;
            let cls = env.find_class(class).map_err(|_| KotlinError::ClassNotFound(class.into()))?;
            let out = env.call_static_method(cls, method, sig, args).map_err(|e| KotlinError::Jni(e.to_string()))?;
            self.check_exception(&env)?;
            out.i().map_err(|e| KotlinError::Jni(e.to_string()))
        }
    }

    /// Conversions utilitaires.
    pub mod conv {
        use super::*;
        /// Option<&str> -> Java String or null.
        pub fn opt_str<'a>(env: &JNIEnv<'a>, s: Option<&str>) -> Result<JObject<'a>> {
            match s {
                Some(v) => Ok(JObject::from(env.new_string(v).map_err(|e| KotlinError::Jni(e.to_string()))?)),
                None => Ok(JObject::null()),
            }
        }
    }
}

#[cfg(all(feature = "jvm-embed", feature = "jni", not(feature = "android")))]
pub mod embed {
    use super::*;
    use jni::InitArgsBuilder;
    use jni::JavaVM;

    /// Lance une JVM embarquée.
    pub fn launch_jvm() -> Result<JavaVM> {
        let args = InitArgsBuilder::new().version(jni::JNIVersion::V8).build().map_err(|e| KotlinError::Jni(e.to_string()))?;
        JavaVM::new(args).map_err(|e| KotlinError::Jni(e.to_string()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_fmt() {
        let e = KotlinError::Conversion("bad".into());
        assert!(format!("{e}").contains("bad"));
    }
}
