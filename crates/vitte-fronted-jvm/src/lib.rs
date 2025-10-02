//! vitte-frontend-jvm — pont JNI pour le frontend Vitte (lexer, parser, AST)
//!
//! Objectif : permettre d’appeler le frontend (parse source → AST JSON)
//! depuis des applis Java/Kotlin via JNI.
//!
//! Exemple côté Java :
//! ```java
//! public class VitteFrontend {
//!     static { System.loadLibrary("vitte_frontend_jvm"); }
//!     private static native String parse(String source);
//!
//!     public static void main(String[] args) {
//!         String ast = VitteFrontend.parse("fn main(){42}");
//!         System.out.println(ast);
//!     }
//! }
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]

extern crate alloc;
use alloc::string::String;

use vitte_frontend::compile_to_ast;

#[cfg(feature = "jni")]
use jni::objects::{JClass, JString};
#[cfg(feature = "jni")]
use jni::sys::jstring;
#[cfg(feature = "jni")]
use jni::JNIEnv;

/// Fonction JNI : parse et retourne un AST JSON.
#[cfg(feature = "jni")]
#[no_mangle]
pub extern "system" fn Java_VitteFrontend_parse(
    env: JNIEnv,
    _class: JClass,
    input: JString,
) -> jstring {
    let src: String = env
        .get_string(&input)
        .map(|s| s.into())
        .unwrap_or_else(|_| "".into());

    let json = match compile_to_ast(&src) {
        Ok(ast) => {
            #[cfg(feature = "serde")]
            {
                serde_json::to_string(&ast).unwrap_or_else(|_| "{}".into())
            }
            #[cfg(not(feature = "serde"))]
            {
                "serde feature disabled".into()
            }
        }
        Err(e) => format!("{e:?}"),
    };

    env.new_string(json).unwrap().into_raw()
}

/* ------------------------------------------------------------------------- */
/* Tests (Rust side)                                                         */
/* ------------------------------------------------------------------------- */
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn compile_ok() {
        let ast = compile_to_ast("fn main(){1+2}");
        assert!(ast.is_ok());
    }
}