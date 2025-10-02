//! vitte-examples — bibliothèque factice regroupant les exemples Vitte.
//!
//! Cette crate n’expose pas d’API significative. Elle sert uniquement à
//! regrouper des binaires d’exemple démontrant l’usage des différentes
//! bibliothèques de l’écosystème Vitte (runtime, embed, FFI, etc.).
//!
//! Binaries fournis :
//! - `hello-vitte` : programme "Hello, World!" utilisant le runtime Vitte.
//! - `calc-vitte`  : calculatrice simple interprétée.
//! - `ffi-vitte`   : démo d’appel FFI (Rust ↔ Vitte).
//!
//! Exemple exécution :
//! ```sh
//! cargo run -p vitte-examples --bin hello-vitte
//! ```

#![deny(missing_docs)]

/// Module factice exposant un compteur de versions.
pub mod about {
    /// Version actuelle des exemples.
    pub const VERSION: &str = env!("CARGO_PKG_VERSION");

    /// Retourne une phrase décrivant la crate d’exemples.
    pub fn description() -> &'static str {
        "Exemples Vitte (hello, calc, ffi). Pas destiné à l’usage en production."
    }
}

/// Re-export basique pour démonstration d’intégration.
pub mod prelude {
    pub use crate::about;
}