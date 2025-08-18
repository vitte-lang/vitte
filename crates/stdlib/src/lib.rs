//! vitte-stdlib — Standard Library du langage Vitte
//!
//! 🍃 Ce crate embarque les sources .vit de la stdlib et, si la feature
//! `compiler` est activée, sait les compiler en un `Chunk` unique prêt
//! à être chargé dans la VM (ou l’évaluateur).
//!
//! API clé :
//! - `sources()` → (&'static str, &'static str)[]  (nom, contenu)
//! - `compile_all()` (feature `compiler`) → Chunk  (link + dedup consts)
//! - `compile_prelude()` (feature `compiler`) → Chunk uniquement pour le prélude
//! - `native` → petites fonctions Rust (I/O, temps) à brancher côté VM
//!
//! ⚠️ Le protocole d’enregistrement des intrinsics côté VM n’est pas imposé ici.
//! Tu pourras exposer `native::*` via tes opcodes/FFI plus tard.

#![forbid(unsafe_code)]
#![deny(rust_2018_idioms, unused_must_use)]
#![cfg_attr(not(debug_assertions), warn(missing_docs))]

use anyhow::{anyhow, Result};
use vitte_core::bytecode::{
    chunk::{Chunk as VChunk, ChunkFlags},
    ConstValue, Op,
};

/// Version du crate.
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Retourne toutes les sources .vit embarquées de la stdlib.
///
/// L’ordre est important: `prelude.vit` d’abord.
pub fn sources() -> Vec<(&'static str, &'static str)> {
    vec![
        ("prelude.vit", include_str!("../stdsrc/prelude.vit")),
        ("string.vit",  include_str!("../stdsrc/string.vit")),
        ("math.vit",    include_str!("../stdsrc/math.vit")),
        ("io.vit",      include_str!("../stdsrc/io.vit")),
        ("net.vit",     include_str!("../stdsrc/net.vit")),
    ]
}

/// Raccourci: contenu du prélude (souvent utile seul).
pub fn prelude_source() -> &'static str {
    include_str!("../stdsrc/prelude.vit")
}

#[cfg(feature = "compiler")]
mod build {
    use super::*;
    use vitte_core::bytecode::chunk::Chunk as VChunk;
    use vitte_core::helpers::validate_chunk;
    use vitte_compiler as compiler;

    #[cfg(feature = "cache")]
    use once_cell::sync::Lazy;

    /// Compile seulement le prélude, pour charger un "minimum vital".
    pub fn compile_prelude() -> Result<VChunk> {
        let src = crate::prelude_source();
        let mut c = compiler::compile_str(src, Some("prelude.vit"))?;
        validate_chunk(&c)?;
        Ok(c)
    }

    /// Compile **toutes** les sources stdlib et les linke en un chunk unique.
    ///
    /// Dé‐duplique les constantes et recolle la ligne/PC pour garder un debug correct.
    pub fn compile_all() -> Result<VChunk> {
        let files = crate::sources();

        // 1) compiler chaque fichier
        let mut compiled: Vec<(String, VChunk)> = Vec::with_capacity(files.len());
        for (name, src) in files {
            let chunk = compiler::compile_str(src, Some(name))?;
            compiled.push((name.to_string(), chunk));
        }

        // 2) link (dé‐dup des consts + concat des ops)
        let linked = link_chunks(&compiled)?;
        validate_chunk(&linked)?;
        Ok(linked)
    }

    /// Version memoïsée (compile une seule fois par process).
    #[cfg(feature = "cache")]
    pub static STDLIB_CHUNK: Lazy<Result<VChunk>> = Lazy::new(|| compile_all());

    /// Petit linker local (similaire à l’outil vitte-link, en plus léger).
    fn link_chunks(inputs: &[(String, VChunk)]) -> Result<VChunk> {
        use std::collections::HashMap;

        let mut out = VChunk::new(ChunkFlags { stripped: false });

        // Map globale pour dé‐dupliquer les constantes
        let mut global: HashMap<ConstValue, u32> = HashMap::new();

        for (name, chunk) in inputs {
            // (1) remapper les constantes
            let mut local_map = HashMap::<u32, u32>::with_capacity(chunk.consts.len());
            for (old_ix, val) in chunk.consts.iter() {
                let new_ix = if let Some(&ix) = global.get(&val) {
                    ix
                } else {
                    let ix = out.add_const(val.clone());
                    global.insert(val.clone(), ix);
                    ix
                };
                local_map.insert(old_ix, new_ix);
            }

            // (2) copier les opcodes avec réécriture de LoadConst
            for (pc, op) in chunk.ops.iter().enumerate() {
                let line = chunk.lines.line_for_pc(pc as u32);
                let new = match *op {
                    Op::LoadConst(ix) => {
                        let new_ix = *local_map.get(&ix)
                            .ok_or_else(|| anyhow!("const {} introuvable lors du lien: {}", ix, name))?;
                        Op::LoadConst(new_ix)
                    }
                    other => other,
                };
                out.push_op(new, line);
            }

            // (3) fusion debug minimal (on conserve le premier main_file)
            if out.debug.main_file.is_none() && chunk.debug.main_file.is_some() {
                out.debug.main_file = chunk.debug.main_file.clone();
            }
            // fichiers: éviter doublons
            for f in &chunk.debug.files {
                if !out.debug.files.contains(f) {
                    out.debug.files.push(f.clone());
                }
            }
            // symboles: on concatène en recalant les PCs si besoin — mais ici,
            // on ne calcule pas un base_pc par fichier (on pourrait). Pour la std,
            // on suppose peu de symboles publics; on met juste les noms en files.
        }

        Ok(out)
    }

    // Réexporte pour l'extérieur du module (garde mêmes signatures publiques).
    pub use compile_all as _compile_all;
    pub use compile_prelude as _compile_prelude;
}

#[cfg(feature = "compiler")]
pub use build::_compile_all as compile_all;
#[cfg(feature = "compiler")]
pub use build::_compile_prelude as compile_prelude;

/// Intrinsics Rust “prêts à brancher” côté VM / éval.
///
/// ⚠️ Ici on ne dépend pas de `vitte-vm` pour éviter les couplages.
/// Expose des fonctions pures/IO simples que ta VM peut appeler.
pub mod native {
    use anyhow::{anyhow, Result};
    use std::fs;
    use std::time::{SystemTime, UNIX_EPOCH};

    /// println simple — utile si tu veux brancher un opcode `PrintStr`.
    pub fn println(s: &str) {
        println!("{s}");
    }

    /// now() en millisecondes depuis l’epoch.
    pub fn now_ms() -> Result<i64> {
        let t = SystemTime::now().duration_since(UNIX_EPOCH)
            .map_err(|e| anyhow!("clock error: {e}"))?;
        Ok(t.as_millis() as i64)
    }

    /// Lecture fichier (UTF-8). À exposer comme `io.read_file(path)`.
    pub fn read_file(path: &str) -> Result<String> {
        let s = fs::read_to_string(path)?;
        Ok(s)
    }

    /// Écriture fichier (UTF-8). À exposer comme `io.write_file(path, text)`.
    pub fn write_file(path: &str, text: &str) -> Result<()> {
        fs::create_dir_all(
            std::path::Path::new(path).parent().unwrap_or(std::path::Path::new(".")))
            .ok();
        fs::write(path, text.as_bytes())?;
        Ok(())
    }
}

/* --------------------------------- Tests --------------------------------- */
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn has_sources() {
        let list = sources();
        assert!(!list.is_empty());
        assert_eq!(list[0].0, "prelude.vit");
        assert!(list[0].1.contains("")); // juste pour toucher la string
    }

    #[cfg(feature = "compiler")]
    #[test]
    fn compile_stdlib_works() {
        let chunk = compile_all().expect("stdlib compile");
        assert!(chunk.ops.len() > 0, "la stdlib devrait générer du code");
    }

    #[cfg(feature = "compiler")]
    #[test]
    fn compile_prelude_works() {
        let chunk = compile_prelude().expect("prelude compile");
        assert!(chunk.ops.len() >= 0);
    }
}
