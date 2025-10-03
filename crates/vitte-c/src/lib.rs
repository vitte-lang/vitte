#[cfg(not(feature = "ffi"))]
mod no_ffi {
    //! Ce crate est construit sans FFI C. Activez la feature `ffi` pour exposer l'API C.
    //! Aucun symbole exporté, aucun `unsafe`.
    // Optionnel: fonctions Rust internes ou docs.
}