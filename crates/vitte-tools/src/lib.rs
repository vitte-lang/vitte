//! vitte-tools — Bibliothèque commune pour les outils CLI Vitte.
//!
//! Objectifs : mutualiser I/O, chrono, couleurs, disasm/asm, strip, linker, config formatter.
//!
//! ## Modules & zones clés
//! - `prelude` : import rapide des types/fns usuels
//! - I/O       : `read_text`, `read_bytes`, `read_stdin_*`, `write_text`, `write_bytes`
//! - Time      : `Timer`, `human_millis`
//! - Couleurs  : `ColorMode`, `setup_colors`
//! - Bytecode  : `strip_chunk`, `validate_chunk` (re-export vitte-core)
//! - Disasm    : `disasm_full`, `disasm_compact` (to `String`)
//! - Linker    : `link_chunks` (réutilisable par vitte-link)
//! - FmtConf   : `FmtConfig`, `load_fmt_config` (optionnel `fmt-config`)
//!
//! Les fonctions sont pensées "no surprises" et avec `anyhow::Result`.
//!
//! ⚠️ Ce crate n’impose pas de runtime (VM, eval) — il ne dépend que de vitte-core.

#![forbid(unsafe_code)]
#![deny(rust_2018_idioms, unused_must_use)]
#![cfg_attr(not(debug_assertions), warn(missing_docs))]

use std::fs;
use std::io::{self, Read, Write};
use std::path::PathBuf;
use std::time::{Duration, Instant};

use anyhow::{anyhow, Context, Result};
use camino::{Utf8Path, Utf8PathBuf};

pub use vitte_core::helpers::validate_chunk;
pub use vitte_core::{
    bytecode::{
        chunk::{Chunk as VChunk, ChunkFlags, DebugInfo},
        ConstValue, Op,
    },
    disasm::{disassemble_compact as core_disasm_compact, disassemble_full as core_disasm_full},
};

/// Version lisible du crate (hérite de vitte-tools).
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Petite bannière de version utile pour logs/outils.
pub fn version_banner(tool: &str) -> String {
    format!("{tool} — vitte-tools {VERSION}")
}

/* ------------------------------------------------------------------------- */
/* Prelude                                                                   */
/* ------------------------------------------------------------------------- */

/// Prelude pratique pour les bins: re-exports compacts.
pub mod prelude {
    pub use anyhow::{anyhow, Context, Result};
    pub use camino::{Utf8Path, Utf8PathBuf};
    pub use std::path::PathBuf;
    pub use crate::{
        version_banner, human_millis,
        read_text, read_bytes, read_stdin_to_string, read_stdin_to_bytes,
        write_text, write_bytes,
        to_utf8, default_out_path, default_filename_with_ext,
        ColorMode, setup_colors,
        disasm_full, disasm_compact,
        strip_chunk, validate_chunk,
        LinkOptions, LinkManifest, link_chunks,
    };
    #[cfg(feature = "fmt-config")]
    pub use crate::{FmtConfig, load_fmt_config};
}

/* ------------------------------------------------------------------------- */
/* I/O utils                                                                 */
/* ------------------------------------------------------------------------- */

/// Lis un fichier texte en UTF-8.
pub fn read_text(path: &Utf8Path) -> Result<String> {
    fs::read_to_string(path).with_context(|| format!("lecture {}", path))
}

/// Lis un fichier binaire.
pub fn read_bytes(path: &Utf8Path) -> Result<Vec<u8>> {
    fs::read(path).with_context(|| format!("lecture {}", path))
}

/// Lis tout `stdin` en String (UTF-8).
pub fn read_stdin_to_string() -> Result<String> {
    let mut s = String::new();
    io::stdin().read_to_string(&mut s)?;
    Ok(s)
}

/// Lis tout `stdin` en bytes.
pub fn read_stdin_to_bytes() -> Result<Vec<u8>> {
    let mut v = Vec::new();
    io::stdin().read_to_end(&mut v)?;
    Ok(v)
}

/// Écrit un texte (UTF-8). Crée les dossiers au besoin.
pub fn write_text(path: &Utf8Path, s: &str) -> Result<()> {
    if let Some(parent) = path.parent() { fs::create_dir_all(parent)?; }
    let mut f = fs::File::create(path)?;
    f.write_all(s.as_bytes())?;
    Ok(())
}

/// Écrit des bytes. Crée les dossiers au besoin.
pub fn write_bytes(path: &Utf8Path, bytes: &[u8]) -> Result<()> {
    if let Some(parent) = path.parent() { fs::create_dir_all(parent)?; }
    let mut f = fs::File::create(path)?;
    f.write_all(bytes)?;
    Ok(())
}

/// Convertit un `PathBuf` en `Utf8PathBuf` (erreur si non UTF-8).
pub fn to_utf8(p: PathBuf) -> Result<Utf8PathBuf> {
    Utf8PathBuf::from_path_buf(p).map_err(|_| anyhow!("chemin non UTF-8"))
}

/* ------------------------------------------------------------------------- */
/* Time / chrono                                                             */
/* ------------------------------------------------------------------------- */

/// Chrono de scope simple; loggable ensuite.
pub struct Timer {
    start: Instant,
}
impl Timer {
    /// Démarre un chrono.
    pub fn start() -> Self { Self { start: Instant::now() } }
    /// Durée écoulée.
    pub fn elapsed(&self) -> Duration { self.start.elapsed() }
    /// Format humain court.
    pub fn pretty(&self) -> String { human_millis(self.elapsed()) }
}

/// Format "humain" d'une durée.
pub fn human_millis(d: Duration) -> String {
    let ms = d.as_millis();
    if ms < 1_000 { return format!("{ms} ms"); }
    let s = ms as f64 / 1000.0;
    if s < 60.0 { return format!("{s:.3} s"); }
    let m = (s / 60.0).floor();
    let rest = s - m * 60.0;
    format!("{m:.0} min {rest:.1} s")
}

/* ------------------------------------------------------------------------- */
/* Couleurs                                                                  */
/* ------------------------------------------------------------------------- */

/// Mode pour les couleurs.
#[derive(Clone, Copy, Debug)]
/// Contrôle l'application de couleurs ANSI dans les sorties CLI.
pub enum ColorMode {
    /// Active les couleurs seulement si la sortie supporte ANSI (auto-détection).
    Auto,
    /// Force l'activation des couleurs, même si le terminal semble ne pas les supporter.
    Always,
    /// Désactive complètement les couleurs ANSI.
    Never,
}

/// Configure le mode couleur global pour yansi (si feature `colors` active).
pub fn setup_colors(mode: ColorMode) {
    #[cfg(feature = "colors")]
    {
        match mode {
            ColorMode::Auto => {
                yansi::whenever(yansi::Condition::DEFAULT);
            }
            ColorMode::Always => {
                yansi::enable();
            }
            ColorMode::Never => yansi::disable(),
        }
    }
    #[cfg(not(feature = "colors"))]
    { let _ = mode; }
}

/* ------------------------------------------------------------------------- */
/* Disasm helpers                                                            */
/* ------------------------------------------------------------------------- */

/// Désassemble en texte long (lisible).
pub fn disasm_full(chunk: &VChunk, title: &str) -> String {
    core_disasm_full(chunk, title)
}
/// Désassemble en vue compacte (1 ligne / op).
pub fn disasm_compact(chunk: &VChunk) -> String {
    core_disasm_compact(chunk)
}

/* ------------------------------------------------------------------------- */
/* Bytecode helpers                                                           */
/* ------------------------------------------------------------------------- */

/// Strip "propre" : reconstruit un chunk avec flag `stripped=true` et debug vidé.
pub fn strip_chunk(src: &VChunk) -> VChunk {
    let mut dst = VChunk::new(ChunkFlags { stripped: true });
    // Copie consts, puis ops + lignes
    for (_, c) in src.consts.iter() {
        dst.add_const(c.clone());
    }
    for (pc, op) in src.ops.iter().enumerate() {
        let line = src.lines.line_for_pc(pc as u32);
        dst.push_op(*op, line);
    }
    dst.debug = DebugInfo::default();
    dst
}

/* ------------------------------------------------------------------------- */
/* Linker réutilisable                                                        */
/* ------------------------------------------------------------------------- */

use std::collections::HashMap;

/// Options du linker.
#[derive(Debug, Clone)]
pub struct LinkOptions {
    /// Déduplique les constantes (par valeur).
    pub dedup_consts: bool,
    /// Strippper le résultat final (debug minimal).
    pub strip: bool,
    /// Fusionner les infos debug (si non strip).
    pub merge_debug: bool,
    /// Symbole d’entrée à valider / annoter (debug).
    pub entry: Option<String>,
}

/// Statuts et mapping renvoyés par le linker (utiles pour logs/JSON).
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize))]
pub struct LinkManifest {
    /// Version du format de chunk émis (hérité du chunk résultant).
    pub version: u16,
    /// Indique si le chunk final est stripé (debug minimal).
    pub stripped: bool,
    /// Liste des entrées ayant contribué au lien.
    pub inputs: Vec<LinkInput>,
    /// Nombre total d'opcodes après fusion.
    pub total_ops: usize,
    /// Nombre de constantes avant déduplication.
    pub total_consts_before: usize,
    /// Nombre de constantes après déduplication.
    pub total_consts_after: usize,
    /// Détails des remappings de constantes pour chaque entrée.
    pub const_maps: Vec<LinkConstMap>,
    /// Offset PC assigné à chaque fichier source lors du lien.
    pub base_pcs: Vec<(String, u32)>,
    /// Nombre de fichiers de debug fusionnés.
    pub merged_debug_files: usize,
    /// Nombre de symboles de debug fusionnés.
    pub merged_debug_symbols: usize,
    /// Symbole d'entrée retenu, si présent.
    pub entry: Option<String>,
    /// Empreinte simple du chunk lié (utilisée pour logs/cache).
    pub hash: u64,
}

#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize))]
/// Statistiques par chunk d'entrée utilisées pour enrichir le manifeste.
pub struct LinkInput {
    /// Nom lisible (chemin) du chunk initial.
    pub file: String,
    /// Nombre d'opcodes présents dans le chunk source.
    pub ops: usize,
    /// Nombre de constantes présentes dans le chunk source.
    pub consts: usize,
}

#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize))]
/// Méta-données de remappage de constantes pour un chunk donné.
pub struct LinkConstMap {
    /// Chunk auquel la table de remappage s'applique.
    pub file: String,
    /// Paires `(ancien_index, nouvel_index)` appliquées lors du lien.
    pub remap: Vec<(u32, u32)>,
    /// Nombre de constantes réutilisées via la déduplication globale.
    pub dedup_hits: usize,
}

/// Link modularisé (recyclé par `vitte-link`).
pub fn link_chunks(inputs: &[(Utf8PathBuf, VChunk)], opts: LinkOptions) -> Result<(VChunk, LinkManifest)> {
    let mut out = if opts.strip {
        VChunk::new(ChunkFlags { stripped: true })
    } else {
        VChunk::new(ChunkFlags { stripped: false })
    };

    let mut const_map_global: HashMap<ConstValue, u32> = HashMap::new();
    let mut inputs_json = Vec::<LinkInput>::new();
    let mut const_maps = Vec::<LinkConstMap>::new();
    let mut total_consts_before = 0usize;
    let mut base_pcs = Vec::<(String, u32)>::new();

    for (name, chunk) in inputs {
        let base_pc = out.ops.len() as u32;
        base_pcs.push((name.to_string(), base_pc));

        inputs_json.push(LinkInput {
            file: name.to_string(),
            ops: chunk.ops.len(),
            consts: chunk.consts.len(),
        });
        total_consts_before += chunk.consts.len();

        // Map des constantes (local -> global)
        let mut local_map: HashMap<u32, u32> = HashMap::with_capacity(chunk.consts.len());
        let mut local_dedup_hits = 0usize;

        for (old_ix, val) in chunk.consts.iter() {
            let new_ix = if opts.dedup_consts {
                if let Some(&ix) = const_map_global.get(&val) {
                    local_dedup_hits += 1;
                    ix
                } else {
                    let ix = out.add_const(val.clone());
                    const_map_global.insert(val.clone(), ix);
                    ix
                }
            } else {
                out.add_const(val.clone())
            };
            local_map.insert(old_ix, new_ix);
        }

        // Copie des opcodes avec réécriture des LoadConst
        for (pc, op) in chunk.ops.iter().enumerate() {
            let line = chunk.lines.line_for_pc(pc as u32);
            let new_op = match *op {
                Op::LoadConst(ix) => {
                    let new_ix = *local_map.get(&ix).ok_or_else(|| anyhow!("Const index {ix} introuvable lors du lien ({name})"))?;
                    Op::LoadConst(new_ix)
                }
                other => other,
            };
            out.push_op(new_op, line);
        }

        // Debug fusionné (si demandé)
        if opts.merge_debug && !opts.strip {
            for f in &chunk.debug.files {
                if !out.debug.files.contains(f) {
                    out.debug.files.push(f.clone());
                }
            }
            if out.debug.main_file.is_none() && chunk.debug.main_file.is_some() {
                out.debug.main_file = chunk.debug.main_file.clone();
            }
            for (sym, pc) in &chunk.debug.symbols {
                out.debug.symbols.push((sym.clone(), base_pc + *pc));
            }
        }

        const_maps.push(LinkConstMap {
            file: name.to_string(),
            remap: local_map.into_iter().collect(),
            dedup_hits: local_dedup_hits,
        });
    }

    // Validation du symbole d'entrée (si possible)
    if let Some(entry) = &opts.entry {
        if !opts.strip && opts.merge_debug {
            let ok = out.debug.symbols.iter().any(|(s, _)| s == entry);
            if !ok {
                return Err(anyhow!("symbole d’entrée `{entry}` introuvable dans les symboles fusionnés"));
            }
            let note = format!("<entry:{entry}>");
            if !out.debug.files.contains(&note) {
                out.debug.files.push(note);
            }
        }
    }

    // Strip final si demandé (reconstruction propre)
    if opts.strip {
        out = strip_chunk(&out);
    }

    let manifest = LinkManifest {
        version: out.version(),
        stripped: out.flags().stripped,
        inputs: inputs_json,
        total_ops: out.ops.len(),
        total_consts_before,
        total_consts_after: out.consts.len(),
        const_maps,
        base_pcs,
        merged_debug_files: out.debug.files.len(),
        merged_debug_symbols: out.debug.symbols.len(),
        entry: opts.entry,
        hash: out.compute_hash(),
    };

    validate_chunk(&out)?;
    Ok((out, manifest))
}

/* ------------------------------------------------------------------------- */
/* Formatter config (optionnel)                                               */
/* ------------------------------------------------------------------------- */

/// Configuration du formatter (utilisée par `vitte-fmt`).
#[cfg(feature = "fmt-config")]
#[derive(Debug, Clone, serde::Deserialize)]
pub struct FmtConfig {
    #[serde(default = "d_max_width")]
    pub max_width: usize,
    #[serde(default = "d_indent_width")]
    pub indent_width: usize,
    #[serde(default)]
    pub use_tabs: bool,
    #[serde(default = "d_newline")]
    pub newline: String,
    #[serde(default = "d_true")]
    pub space_around_ops: bool,
    #[serde(default = "d_true")]
    pub trim_trailing: bool,
    #[serde(default = "d_true")]
    pub ensure_final_newline: bool,
    #[serde(default = "d_collapse_blank")]
    pub collapse_blank_lines: usize,
}
#[cfg(feature = "fmt-config")]
fn d_max_width() -> usize { 100 }
#[cfg(feature = "fmt-config")]
fn d_indent_width() -> usize { 2 }
#[cfg(feature = "fmt-config")]
fn d_newline() -> String { "lf".into() }
#[cfg(feature = "fmt-config")]
fn d_true() -> bool { true }
#[cfg(feature = "fmt-config")]
fn d_collapse_blank() -> usize { 2 }

/// Charge `.vittefmt.toml` (recherche ascendante) ou renvoie `Default`.
#[cfg(feature = "fmt-config")]
pub fn load_fmt_config(explicit: Option<&Path>) -> Result<FmtConfig> {
    if let Some(p) = explicit {
        let s = fs::read_to_string(p).with_context(|| format!("lecture config {}", p.display()))?;
        let cfg: FmtConfig = toml::from_str(&s).with_context(|| "TOML invalide")?;
        return Ok(cfg);
    }
    // recherche ascendante
    let mut cur = std::env::current_dir()?;
    loop {
        let cand = cur.join(".vittefmt.toml");
        if cand.exists() {
            let s = fs::read_to_string(&cand).with_context(|| format!("lecture {}", cand.display()))?;
            let cfg: FmtConfig = toml::from_str(&s).with_context(|| "TOML invalide")?;
            return Ok(cfg);
        }
        if !cur.pop() { break; }
    }
    Ok(FmtConfig {
        max_width: 100,
        indent_width: 2,
        use_tabs: false,
        newline: "lf".into(),
        space_around_ops: true,
        trim_trailing: true,
        ensure_final_newline: true,
        collapse_blank_lines: 2,
    })
}

/* ------------------------------------------------------------------------- */
/* Divers                                                                     */
/* ------------------------------------------------------------------------- */

/// Construit un nom `<stem>.<ext>` à partir d'un input.
pub fn default_filename_with_ext(input: &Utf8Path, ext: &str) -> String {
    let stem = input.file_stem().unwrap_or("out");
    format!("{stem}.{ext}")
}

/// Remplace l’extension par `ext` (sans point), ex: `.vitbc`.
pub fn default_out_path(input: &Utf8Path, ext: &str) -> Utf8PathBuf {
    input.with_extension(ext).to_path_buf()
}

/// Chemin d’historique pour REPL (si feature `dirs`).
#[cfg(feature = "dirs")]
pub fn history_path() -> Result<PathBuf> {
    let base = dirs::data_dir().ok_or_else(|| anyhow!("dirs::data_dir introuvable"))?;
    Ok(base.join("vitte").join("history"))
}
