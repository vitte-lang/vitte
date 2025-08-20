//! vitte-core — Cœur du langage Vitte
//!
//! Contient tout ce qu’il faut pour manipuler le bytecode Vitte, sans
//! dépendre du compilateur ou de la VM complète.
//!
//! ## Modules
//! - `bytecode`  : format `Chunk`, pool de constantes, opcodes `Op`.
//! - `asm`       : assembleur texte → `Chunk` (MVP).
//! - `disasm`    : désassembleur lisible (humain).
//! - `loader`    : chargement / linkage multi-chunks.
//! - `utils`     : briques internes (hash, pretty, etc.).
//!
//! ## Features
//! - **std** *(par défaut)* : active la std (fs, io). Désactive pour no_std(+alloc).
//! - **alloc-only** : build no_std avec `alloc` (sans fs). Mutuellement exclusif avec `std`.
//! - **serde** : (dé)sérialisation via serde pour certains helpers/types.

#![forbid(unsafe_code)]
#![deny(rust_2018_idioms, unused_must_use)]
#![cfg_attr(not(debug_assertions), warn(missing_docs))]
#![cfg_attr(all(not(feature = "std"), feature = "alloc-only"), no_std)]

/* ---------- Garde-fous de configuration ---------- */
#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Active la feature `std` ou `alloc-only` (no_std+alloc) pour vitte-core.");

#[cfg(all(feature = "std", feature = "alloc-only"))]
compile_error!("`std` et `alloc-only` sont mutuellement exclusifs.");

/* ---------- Imports dépendants de l'environnement ---------- */
#[cfg(all(not(feature = "std"), feature = "alloc-only"))]
extern crate alloc;

#[cfg(all(not(feature = "std"), feature = "alloc-only"))]
use alloc::{boxed::Box, string::String, vec::Vec};

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

/* ---------- Modules publics ---------- */
pub mod asm;
pub mod disasm;
pub mod loader;
pub mod utils;
pub mod bytecode;

/* (⚠️ ancien bloc #[cfg(feature = "eval")] retiré) */

/* ---------- Reexports (un seul bloc, pas de doublons) ---------- */
pub use bytecode::chunk::{Chunk, ChunkFlags, ConstPool, ConstValue, LineTable};
pub use bytecode::op::Op; // <- Op vient du sous-module `bytecode::op`

/* ---------- Version ---------- */
#[cfg(feature = "std")]
/// Version du crate (lisible, via Cargo).
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

#[cfg(feature = "std")]
/// Renvoie une jolie bannière de version (utile pour logs/outils).
pub fn version() -> String {
    format!("vitte-core {}", VERSION)
}

#[cfg(all(not(feature = "std"), feature = "alloc-only"))]
/// En `no_std`, on ne peut pas lire `env!` de façon fiable côté binaire distribué.
/// On expose une valeur neutre.
pub const VERSION_NO_STD: &str = "vitte-core (no_std)";

/* ---------- Erreurs & Résultat ---------- */
#[cfg(feature = "std")]
use thiserror::Error;

#[cfg(feature = "std")]
#[derive(Debug, Error)]
pub enum Error {
    /// I/O (quand `std` est activé)
    #[error("io: {0}")]
    Io(#[from] std::io::Error),

    /// Erreurs de (dé)sérialisation bincode (bytecode)
    #[error("bincode: {0}")]
    Bincode(#[from] bincode::Error),

    /// Format de chunk invalide ou attendu ≠ trouvé
    #[error("chunk: {0}")]
    Chunk(String),

    /// Erreur générique
    #[error("{0}")]
    Msg(String),
}

#[cfg(all(not(feature = "std"), feature = "alloc-only"))]
#[derive(Debug)]
pub enum Error {
    /// Erreur générique (no_std)
    Msg(&'static str),
}

pub type Result<T, E = Error> = core::result::Result<T, E>;

/* ---------- Prelude ---------- */
pub mod prelude {
    pub use crate::{
        bytecode::{chunk, op}, // on expose les modules si besoin
        helpers::*,
        Chunk, ChunkFlags, ConstPool, ConstValue, Error, LineTable, Op, Result,
    };
    #[cfg(feature = "std")]
    pub use crate::version;
}

/* ---------- Macros utilitaires ---------- */
#[macro_export]
macro_rules! vit_assert {
    ($cond:expr, $($arg:tt)*) => {
        if !$cond {
            #[cfg(feature = "std")]
            {
                return Err($crate::Error::Msg(format!($($arg)*)));
            }
            #[cfg(all(not(feature="std"), feature="alloc-only"))]
            {
                return Err($crate::Error::Msg("assertion failed"));
            }
        }
    };
    ($cond:expr) => {
        if !$cond {
            #[cfg(feature = "std")]
            { return Err($crate::Error::Msg("assertion failed".into())); }
            #[cfg(all(not(feature="std"), feature="alloc-only"))]
            { return Err($crate::Error::Msg("assertion failed")); }
        }
    };
}

/* ---------- Helpers “batteries-incluses” ---------- */
pub mod helpers {
    use super::*;
    use bytecode::chunk::{CHUNK_MAGIC, CHUNK_VERSION};

    /// Crée un `Chunk` vide “prêt à remplir”.
    pub fn new_chunk(stripped: bool) -> Chunk { Chunk::new(ChunkFlags { stripped }) }

    /// Ajoute des constantes (raccourcis).
    pub fn k_str(c: &mut Chunk, s: &str) -> u32 { c.add_const(ConstValue::Str(s.into())) }
    pub fn k_i64(c: &mut Chunk, i: i64) -> u32 { c.add_const(ConstValue::I64(i)) }
    pub fn k_f64(c: &mut Chunk, x: f64) -> u32 { c.add_const(ConstValue::F64(x)) }
    pub fn k_bool(c: &mut Chunk, b: bool) -> u32 { c.add_const(ConstValue::Bool(b)) }
    pub fn k_null(c: &mut Chunk) -> u32 { c.add_const(ConstValue::Null) }

    /// Vérifie quelques invariants d’un `Chunk`. Étends selon ton format.
    pub fn validate_chunk(c: &Chunk) -> Result<()> {
        if c.consts.len() > (u32::MAX as usize) {
            #[cfg(feature = "std")] { return Err(Error::Chunk("trop de constantes".into())); }
            #[cfg(all(not(feature="std"), feature="alloc-only"))] { return Err(Error::Msg("trop de constantes")); }
        }
        if let Some(main) = &c.debug.main_file {
            if main.trim().is_empty() {
                #[cfg(feature = "std")] { return Err(Error::Chunk("debug.main_file vide".into())); }
                #[cfg(all(not(feature="std"), feature="alloc-only"))] { return Err(Error::Msg("debug.main_file vide")); }
            }
        }
        Ok(())
    }

    /// Signature binaire attendue (vérif toolchain).
    pub fn compiled_format_signature() -> (&'static [u8; 4], u16) {
        (&CHUNK_MAGIC, CHUNK_VERSION)
    }

    // --- IO fichiers : seulement avec `std` ---
    #[cfg(feature = "std")]
    /// (std) Lire un `Chunk` depuis un fichier `.vitbc`.
    pub fn read_chunk_from_file(path: impl AsRef<std::path::Path>) -> Result<Chunk> {
        use std::fs;
        let bytes = fs::read(path)?;
        let c = Chunk::from_bytes(&bytes).map_err(|e| Error::Chunk(format!("{e}")))?;
        validate_chunk(&c)?;
        Ok(c)
    }

    #[cfg(feature = "std")]
    /// (std) Écrire un `Chunk` dans un fichier `.vitbc`.
    pub fn write_chunk_to_file(mut chunk: Chunk, path: impl AsRef<std::path::Path>) -> Result<()> {
        use std::fs;
        let bytes = chunk.to_bytes();
        fs::write(path, bytes)?;
        Ok(())
    }

    // --- IO générique: traits & impl par défaut ---
    /// Abstraction storage pour tests/embarqués.
    pub trait BytecodeIo {
        /// Charge un `Chunk` depuis des bytes.
        fn load(&self, bytes: &[u8]) -> Result<Chunk>;
        /// Sauvegarde un `Chunk` vers des bytes.
        fn save(&self, chunk: &mut Chunk) -> Result<Vec<u8>>;
    }

    /// Impl “native” basée sur le format interne de `Chunk`.
    pub struct NativeBytecode;
    impl BytecodeIo for NativeBytecode {
        fn load(&self, bytes: &[u8]) -> Result<Chunk> {
            Chunk::from_bytes(bytes).map_err(|e| {
                #[cfg(feature = "std")] { Error::Chunk(format!("{e}")) }
                #[cfg(all(not(feature="std"), feature="alloc-only"))] { Error::Msg("from_bytes failed") }
            })
        }
        fn save(&self, chunk: &mut Chunk) -> Result<Vec<u8>> {
            Ok(chunk.to_bytes())
        }
    }
}

/* ---------- Serde (optionnelle) ---------- */
#[cfg(feature = "serde")]
pub mod serde_support {
    use super::*;
    use serde::{Deserialize, Serialize};

    /// Exemple d’export JSON “humain” d’un Chunk minimal (métadonnées + op count).
    #[derive(Debug, Serialize, Deserialize)]
    pub struct ChunkSummary {
        pub ops: usize,
        pub consts: usize,
        pub stripped: bool,
        pub main_file: Option<String>,
    }

    impl From<&Chunk> for ChunkSummary {
        fn from(c: &Chunk) -> Self {
            Self {
                ops: c.ops.len(),
                consts: c.consts.len(),
                stripped: c.flags().stripped,
                main_file: c.debug.main_file.clone(),
            }
        }
    }
}

/* ---------- Tests ---------- */
#[cfg(test)]
mod tests {
    use super::*;
    use crate::bytecode::chunk::ChunkFlags;

    #[test]
    fn chunk_roundtrip_bytes() {
        let mut c = helpers::new_chunk(false);
        let k = helpers::k_str(&mut c, "yo");
        c.ops.push(Op::LoadConst(k));
        c.ops.push(Op::Print);
        c.ops.push(Op::Return);

        let mut bytes = c.to_bytes();
        // on relit
        let c2 = Chunk::from_bytes(&bytes).expect("roundtrip ok");
        assert_eq!(c2.ops.len(), 3);

        // corruption volontaire → doit échouer
        bytes[bytes.len().saturating_sub(1)] ^= 0xFF;
        let err = Chunk::from_bytes(&bytes).unwrap_err();
        let s = format!("{err}");
        assert!(s.to_lowercase().contains("hash"));
    }

    #[test]
    fn compiled_sig_exposed() {
        let (_magic, _ver) = helpers::compiled_format_signature();
        // just presence check
        assert!(_ver > 0);
    }
}
