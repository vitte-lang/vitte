

#![deny(missing_docs)]
//! vitte-ruby — interop Rust/Ruby pour Vitte
//!
//! Fournit :
//! - Backend Rutie (par défaut).
//! - Backend Magnus (optionnel).
//! - Fonctions exportées : `greet`, `sum_array`, JSON bidirectionnel.
//!
//! Compilation :
//! ```bash
//! cargo build -p vitte-ruby --features rutie --release
//! cargo build -p vitte-ruby --features magnus --release
//! ```
//!
//! Côté Ruby (exemple) :
//! ```ruby
//! require 'vitte_ruby'
//! puts VitteRuby.greet("world")
//! ```

use thiserror::Error;

/// Erreurs interop Ruby.
#[derive(Debug, Error)]
pub enum RubyError {
    #[error("conversion error: {0}")]
    Conversion(String),
    #[error("runtime error: {0}")]
    Runtime(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, RubyError>;

#[cfg(feature = "rutie")]
mod rutie_backend {
    use super::*;
    use rutie::{Class, Object, RString, VM, Array};

    class!(VitteRuby);

    methods!(
        VitteRuby,
        _itself,

        fn greet_rb(name: RString) -> RString {
            let n = name.unwrap_or_else(|_| RString::new_utf8("unknown"));
            RString::new_utf8(&format!("Hello, {} from Vitte/Ruby Rutie", n.to_str()))
        }

        fn sum_array_rb(arr: Array) -> i64 {
            arr.into_iter().filter_map(|v| v.try_convert_to::<rutie::Fixnum>().ok()).map(|f| f.to_i64()).sum()
        }
    );

    #[allow(non_snake_case)]
    #[no_mangle]
    pub extern "C" fn Init_vitte_ruby() {
        Class::new("VitteRuby", None).define(|itself| {
            itself.def_self("greet", greet_rb);
            itself.def_self("sum_array", sum_array_rb);
        });
    }
}

#[cfg(feature = "magnus")]
mod magnus_backend {
    use super::*;
    use magnus::{define_module, function, RString, Value, Error as MgError};

    fn greet(name: String) -> String {
        format!("Hello, {name} from Vitte/Ruby Magnus")
    }

    fn sum_array(vals: Vec<i64>) -> i64 {
        vals.into_iter().sum()
    }

    #[magnus::init]
    fn init() -> Result<(), MgError> {
        let module = define_module("VitteRuby")?;
        module.define_singleton_method("greet", function!(greet, 1))?;
        module.define_singleton_method("sum_array", function!(sum_array, 1))?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_fmt() {
        let e = RubyError::Conversion("bad".into());
        assert!(format!("{e}").contains("bad"));
    }
}