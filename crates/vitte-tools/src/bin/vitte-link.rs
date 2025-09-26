// crates/vitte-tools/src/bin/vitte-link.rs
//! Linker Vitte : fusionne plusieurs .vitbc en un seul chunk.
//!
//! Exemples :
//!   vitte-link a.vitbc b.vitbc --out linked.vitbc --summary
//!   cat a.vitbc | vitte-link - --out out.vitbc --stdin-name a.vitbc
//!   vitte-link a.vitbc b.vitbc --out linked.vitbc --emit-disasm linked.disasm.txt
//!   vitte-link a.vitbc b.vitbc --out linked.vitbc --emit-json linked.manifest.json --verify
//!
//! Remarques :
//! - Le lien concatène le code et **déduplique le pool de constantes** (par valeur).
//! - Les `LoadConst` sont **réécrits** selon le nouveau pool fusionné.
//! - Les `Jump`/`JumpIfFalse` **restent valides** (offsets relatifs) car on conserve
//!   l'ordre d'entrée et on ne réordonne pas les instructions.
//! - Les symboles debug (si présents) sont **relocalisés** (offset PC base).
//! - `--strip` reconstruit un chunk propre avec debug minimal (stripped=true).
//! - `--entry` vérifie simplement la présence du symbole (debug) et l'encode en note.

use std::collections::HashMap;
use std::fs;
use std::io::{self, Read, Write};
use std::path::PathBuf;
use std::time::Instant;

use anyhow::{anyhow, Context, Result};
use camino::{Utf8Path, Utf8PathBuf};
use clap::{ArgAction, Parser};
use serde::Serialize;
use yansi::{Color, Paint};

use vitte_core::bytecode::{
    chunk::{Chunk as VChunk, ChunkFlags},
    ConstValue, Op,
};
use vitte_core::disasm::disassemble_full;
use vitte_core::helpers;

#[derive(Parser, Debug)]
#[command(name="vitte-link", version, about="Linker Vitte (fusion de .vitbc)")]
struct Cli {
    /// Fichier(s) .vitbc à linker (ou '-' pour stdin, unique)
    inputs: Vec<String>,

    /// Fichier de sortie .vitbc
    #[arg(short, long, required_unless_present = "stdout")]
    out: Option<PathBuf>,

    /// Écrit le .vitbc résultant sur stdout (binaire)
    #[arg(long, action=ArgAction::SetTrue)]
    stdout: bool,

    /// Nom logique si l’entrée est '-' (stdin)
    #[arg(long, default_value = "<stdin>")]
    stdin_name: String,

    /// Désactive la déduplication de constantes
    #[arg(long, action=ArgAction::SetTrue)]
    no_dedup: bool,

    /// Retirer les infos de debug (strip)
    #[arg(long, action=ArgAction::SetTrue)]
    strip: bool,

    /// Ne pas fusionner le debug (fichiers/symboles) même sans strip
    #[arg(long, action=ArgAction::SetTrue)]
    no_merge_debug: bool,

    /// Vérifie le résultat en le rechargeant (round-trip)
    #[arg(long, action=ArgAction::SetTrue)]
    verify: bool,

    /// Résumé humain : consts/opcodes/version/flags/hash/dédup
    #[arg(long, action=ArgAction::SetTrue)]
    summary: bool,

    /// Écrit un désassemblage du résultat
    #[arg(long)]
    emit_disasm: Option<PathBuf>,

    /// Écrit un manifest JSON (mappings consts/offsets/sources)
    #[arg(long)]
    emit_json: Option<PathBuf>,

    /// Symbole d’entrée (validation & tag dans debug combiné)
    #[arg(long)]
    entry: Option<String>,

    /// Affiche la durée
    #[arg(long, action=ArgAction::SetTrue)]
    time: bool,
}

fn main() {
    if let Err(e) = real_main() {
        eprintln!("❌ {e}");
        std::process::exit(1);
    }
}

fn real_main() -> Result<()> {
    color_eyre::install().ok();

    let cli = Cli::parse();

    if cli.inputs.is_empty() {
        return Err(anyhow!("Aucune entrée fournie. Exemple : vitte-link a.vitbc b.vitbc --out linked.vitbc"));
    }
    let use_stdin = cli.inputs.len() == 1 && cli.inputs[0] == "-";
    if use_stdin && cli.out.is_none() && !cli.stdout {
        return Err(anyhow!("Avec stdin, précise --out ou --stdout."));
    }
    if !use_stdin && cli.stdout && cli.out.is_some() {
        return Err(anyhow!("Choisis l’un : --out ou --stdout."));
    }

    // Lire toutes les entrées
    let mut inputs = Vec::<(Utf8PathBuf, VChunk)>::new();
    if use_stdin {
        let (bytes, name) = read_input("-", &cli.stdin_name)?;
        let c = VChunk::from_bytes(&bytes).with_context(|| format!("Chargement échoué : {name}"))?;
        inputs.push((name, c));
    } else {
        for a in &cli.inputs {
            let (bytes, name) = read_input(a, &cli.stdin_name)?;
            let c = VChunk::from_bytes(&bytes).with_context(|| format!("Chargement échoué : {name}"))?;
            inputs.push((name, c));
        }
    }

    if inputs.len() == 1 && !cli.stdout {
        eprintln!("ℹ️  Une seule entrée — le linker fera surtout passerelle (dédup/strip éventuels).");
    }

    let t0 = Instant::now();
    let opts = LinkOptions {
        dedup_consts: !cli.no_dedup,
        strip: cli.strip,
        merge_debug: !cli.no_merge_debug && !cli.strip,
        entry: cli.entry.clone(),
    };
    let (linked, manifest) = link_chunks(&inputs, opts)?;

    if cli.verify {
        let rt = linked.to_bytes();
        let chk = VChunk::from_bytes(&rt)?;
        helpers::validate_chunk(&chk)?;
        eprintln!("{}", "✓ verify round-trip OK".paint(Color::Green));
    }

    if cli.summary {
        print_summary(&linked, &manifest);
    }

    if let Some(path) = &cli.emit_disasm {
        let out = Utf8PathBuf::from_path_buf(path.clone()).map_err(|_| anyhow!("Chemin --emit-disasm non UTF-8"))?;
        let title = out.file_name().unwrap_or("linked");
        write_text(&out, &disassemble_full(&linked, title))?;
        eprintln!("📝 Disasm → {out}");
    }

    if let Some(path) = &cli.emit_json {
        let out = Utf8PathBuf::from_path_buf(path.clone()).map_err(|_| anyhow!("Chemin --emit-json non UTF-8"))?;
        write_text(&out, &serde_json::to_string_pretty(&manifest)?)?;
        eprintln!("🧾 Manifest JSON → {out}");
    }

    // Sortie binaire
    let bytes = linked.to_bytes();
    if cli.stdout {
        io::stdout().write_all(&bytes)?;
    } else if let Some(p) = cli.out {
        let out = Utf8PathBuf::from_path_buf(p).map_err(|_| anyhow!("Chemin --out non UTF-8"))?;
        write_bytes(&out, &bytes)?;
        eprintln!("✅ Link → {out}");
    }

    if cli.time {
        eprintln!("⏱️  {}", human_millis(t0.elapsed()));
    }

    Ok(())
}

/* ------------------------------- Linker ------------------------------- */

#[derive(Debug, Clone)]
struct LinkOptions {
    dedup_consts: bool,
    strip: bool,
    merge_debug: bool,
    entry: Option<String>,
}

#[derive(Serialize)]
struct JsonInput {
    file: String,
    ops: usize,
    consts: usize,
}

#[derive(Serialize)]
struct JsonConstMap {
    /// Map d’un chunk source (index ancien) -> (index nouveau)
    file: String,
    remap: Vec<(u32, u32)>,
    dedup_hits: usize,
}

#[derive(Serialize)]
struct JsonManifest {
    version: u16,
    stripped: bool,
    inputs: Vec<JsonInput>,
    total_ops: usize,
    total_consts_before: usize,
    total_consts_after: usize,
    const_maps: Vec<JsonConstMap>,
    base_pcs: Vec<(String, u32)>,
    merged_debug_files: usize,
    merged_debug_symbols: usize,
    entry: Option<String>,
    hash: u64,
}

fn link_chunks(inputs: &[(Utf8PathBuf, VChunk)], opts: LinkOptions) -> Result<(VChunk, JsonManifest)> {
    // Chunk résultat
    let mut out = if opts.strip {
        VChunk::new(ChunkFlags { stripped: true })
    } else {
        VChunk::new(ChunkFlags { stripped: false })
    };

    // Dédup des constantes
    let mut const_map_global: HashMap<ConstValue, u32> = HashMap::new();

    let mut inputs_json = Vec::<JsonInput>::new();
    let mut const_maps = Vec::<JsonConstMap>::new();
    let mut total_consts_before = 0usize;
    let mut base_pcs = Vec::<(String, u32)>::new();

    // Merge !
    for (name, chunk) in inputs {
        let base_pc = out.ops.len() as u32;
        base_pcs.push((name.to_string(), base_pc));

        inputs_json.push(JsonInput {
            file: name.to_string(),
            ops: chunk.ops.len(),
            consts: chunk.consts.len(),
        });
        total_consts_before += chunk.consts.len();

        // Construire la map des constantes pour ce chunk
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

        // Réécrire les opcodes et merger la table des lignes
        for (pc, op) in chunk.ops.iter().enumerate() {
            let line = chunk.lines.line_for_pc(pc as u32);
            let new_op = match *op {
                Op::LoadConst(ix) => {
                    let new_ix = *local_map.get(&ix).ok_or_else(|| anyhow!("Const index {ix} introuvable lors du lien ({name})"))?;
                    Op::LoadConst(new_ix)
                }
                // Pour tout le reste : copie inchangée (offsets relatifs restent valides)
                other => other,
            };
            out.push_op(new_op, line);
        }

        // Debug : fusion optionnelle (si non strip)
        if opts.merge_debug && !opts.strip {
            // Fichiers
            for f in &chunk.debug.files {
                if !out.debug.files.contains(f) {
                    out.debug.files.push(f.clone());
                }
            }
            // main_file — conserve le premier non vide ou accumule note
            if out.debug.main_file.is_none() && chunk.debug.main_file.is_some() {
                out.debug.main_file = chunk.debug.main_file.clone();
            }
            // symbols (relocalisés par base_pc)
            for (sym, pc) in &chunk.debug.symbols {
                out.debug.symbols.push((sym.clone(), base_pc + *pc));
            }
        }
        // Si strip : on laissera DebugInfo::default() en fin (déjà le cas pour flags.stripped)
        const_maps.push(JsonConstMap {
            file: name.to_string(),
            remap: local_map.into_iter().collect(),
            dedup_hits: local_dedup_hits,
        });
    }

    // Tag d’entrée (facultatif)
    if let Some(entry) = &opts.entry {
        // Valide que le symbole existe
        if !opts.strip && opts.merge_debug {
            let ok = out.debug.symbols.iter().any(|(s, _)| s == entry);
            if !ok {
                return Err(anyhow!("Symbole `--entry {entry}` introuvable dans les symboles fusionnés."));
            }
            // Encode une petite note en debug.files
            let note = format!("<entry:{}>", entry);
            if !out.debug.files.contains(&note) {
                out.debug.files.push(note);
            }
        } else {
            // pas de debug… on ne peut que l’annoncer
            eprintln!("⚠️  --entry {} fourni mais debug non fusionné/strippé : validation limitée.", entry);
        }
    }

    // Strip final si demandé : on reconstruit proprement pour être sûr
    if opts.strip {
        let mut stripped = VChunk::new(ChunkFlags { stripped: true });
        // Copie consts/ops/lines sans debug
        for (_, c) in out.consts.iter() {
            stripped.add_const(c.clone());
        }
        for (pc, op) in out.ops.iter().enumerate() {
            let line = out.lines.line_for_pc(pc as u32);
            stripped.push_op(*op, line);
        }
        out = stripped;
    }

    // Manifest JSON
    let manifest = JsonManifest {
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

    helpers::validate_chunk(&out)?;
    Ok((out, manifest))
}

/* ------------------------------- I/O utils ------------------------------- */

fn read_input(arg: &str, stdin_name: &str) -> Result<(Vec<u8>, Utf8PathBuf)> {
    if arg == "-" {
        let mut v = Vec::new();
        io::stdin().read_to_end(&mut v)?;
        Ok((v, Utf8PathBuf::from(stdin_name)))
    } else {
        let p = Utf8PathBuf::from(arg);
        let v = fs::read(&p).with_context(|| format!("Lecture échouée: {p}"))?;
        Ok((v, p))
    }
}

fn write_bytes(path: &Utf8Path, bytes: &[u8]) -> Result<()> {
    if let Some(parent) = path.parent() { fs::create_dir_all(parent)?; }
    let mut f = fs::File::create(path)?;
    f.write_all(bytes)?;
    Ok(())
}

fn write_text(path: &Utf8Path, s: &str) -> Result<()> {
    if let Some(parent) = path.parent() { fs::create_dir_all(parent)?; }
    let mut f = fs::File::create(path)?;
    f.write_all(s.as_bytes())?;
    Ok(())
}

/* ------------------------------- Affichage ------------------------------- */

fn print_summary(_out: &VChunk, m: &JsonManifest) {
    let before = m.total_consts_before as i64;
    let after = m.total_consts_after as i64;
    let saved = before - after;
    let ratio = if before > 0 { (saved as f64) / (before as f64) * 100.0 } else { 0.0 };

    eprintln!("{}", "== Link summary ==".paint(Color::Cyan).bold());
    eprintln!("• inputs: {}", m.inputs.len());
    for i in &m.inputs {
        eprintln!("  - {} (ops={}, consts={})", i.file, i.ops, i.consts);
    }
    eprintln!(
        "• ops: {}   consts: {} (avant: {}, gain: {} ~ {:.1}%)",
        m.total_ops, m.total_consts_after, m.total_consts_before, saved.max(0), ratio
    );
    eprintln!("• version: {}   stripped: {}", m.version, m.stripped);
    eprintln!("• debug: files={}, symbols={}", m.merged_debug_files, m.merged_debug_symbols);
    eprintln!("• hash: 0x{:016x}", m.hash);
    if let Some(e) = &m.entry {
        eprintln!("• entry: {}", e);
    }
}

fn human_millis(d: std::time::Duration) -> String {
    let ms = d.as_millis();
    if ms < 1_000 { return format!("{ms} ms"); }
    let s = ms as f64 / 1000.0;
    if s < 60.0 { return format!("{s:.3} s"); }
    let m = (s / 60.0).floor();
    let rest = s - m * 60.0;
    format!("{m:.0} min {rest:.1} s")
}
