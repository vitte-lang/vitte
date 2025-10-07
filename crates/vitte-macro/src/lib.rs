//! vitte-macro — macros procédurales pour Vitte
//!
//! Fournit :
//! - `#[derive(Trace)]` pour générer une impl de [`Trace`] (GC).
//! - `vitte_macro_export!` pour exporter des helpers communs.
//! - `#[vitte_fn]` pour marquer des fonctions comme natives / exposées.
//!
//! Les macros sont destinées à l’écosystème Vitte (compilateur, runtime).
//!
//! Exemple d’utilisation :
//! ```ignore
//! use vitte_macro::Trace;
//!
//! #[derive(Trace)]
//! struct Node {
//!     value: i32,
//!     next: Option<Box<Node>>,
//! }
//! ```

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

extern crate proc_macro;

use proc_macro::TokenStream;
use quote::quote;
use syn::{DeriveInput, ItemFn, parse_macro_input};

/// Derive macro `Trace` pour GC.
/// Implémente le trait `Trace` (défini dans vitte-gc).
#[proc_macro_derive(Trace)]
pub fn derive_trace(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);
    let name = input.ident;
    let gen = quote! {
        impl ::vitte_gc::Trace for #name {
            fn trace(&self, vis: &mut dyn FnMut(*const ())) {
                // Impl par défaut: ne visite rien.
                // Impl custom nécessaire si la struct contient des Gc<T>.
                let _ = vis;
            }
        }
    };
    gen.into()
}

/// Attribut `#[vitte_fn]`
/// Permet de marquer une fonction comme exposée à la VM.
#[proc_macro_attribute]
pub fn vitte_fn(_args: TokenStream, input: TokenStream) -> TokenStream {
    let item = parse_macro_input!(input as ItemFn);
    let vis = item.vis.clone();
    let sig = item.sig.clone();
    let block = item.block.clone();
    let gen = quote! {
        #vis #sig {
            // Hook potentiel: logging ou registration
            #block
        }

        #[allow(non_upper_case_globals)]
        pub const _: () = {
            // Enregistrement statique (ex: table des natives)
            // Ici: no-op placeholder
        };
    };
    gen.into()
}

/// Macro utilitaire d’export (non-proc).
#[proc_macro]
pub fn vitte_macro_export(_input: TokenStream) -> TokenStream {
    let gen = quote! {
        pub fn vitte_macro_version() -> &'static str {
            "vitte-macro 0.1.0"
        }
    };
    gen.into()
}

// ============================= Tests ======================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn version_macro_generates() {
        let ts: TokenStream = vitte_macro_export(TokenStream::new());
        let s = ts.to_string();
        assert!(s.contains("vitte_macro_version"));
    }
}
