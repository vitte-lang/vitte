// crates/vitte-tools/src/bin/vitte-asm.rs
//! Assembleur Vitte : .vit.s -> .vitbc
//! Usage basique :
//!   vitte-asm source.vit.s -o out.vitbc
//!   cat source.vit.s | vitte-asm - --out out.vitbc --disasm
//! Multi-fichiers :
//!   vitte-asm a.vit.s b.vit.s --out-dir target/ --verify --time
//! Flags utiles :
//!   --strip        : retire les infos debug (vrai strip, header flag à true)
//!   --check        : dry-run, n’écrit rien
//!   --verify       : round-trip (to_bytes → from_bytes) pour valider
//!   --disasm       : affiche le désassemblage sur stdout
//!   --emit-disasm  : écrit le désassemblage dans un fichier
//!   --stdin-name   : nom logique quand l’entrée est '-'
//!   --time         : affiche le temps d’assemblage
//!   --strict       : échoue si chunk vide (aucun op)

use std::fs;
use std::io::{self, Read, Write};
use std::path::PathBuf;
use std::time::Instant;

use anyhow::{anyhow, Context, Result};
use camino::{Utf8Path, Utf8PathBuf};
use clap::{ArgAction, Parser};

use vitte_core::bytecode::chunk::{Chunk as VChunk, ChunkFlags, DebugInfo};
use vitte_core::disasm::disassemble_full;
use vitte_core::helpers;

#[derive(Parser, Debug)]
#[command(name="vitte-asm", version, about="Assembleur Vitte (.vit.s -> .vitbc)")]
struct Cli {
    /// Fichier(s) source .vit.s (ou '-' pour stdin, unique)
    inputs: Vec<String>,

    /// Fichier de sortie .vitbc (uniquement si 1 entrée)
    #[arg(short, long, conflicts_with = "out_dir")]
    out: Option<PathBuf>,

    /// Dossier de sortie pour plusieurs entrées (garde <stem>.vitbc)
    #[arg(long)]
    out_dir: Option<PathBuf>,

    /// Retirer les infos de debug (stripped)
    #[arg(long, action=ArgAction::SetTrue)]
    strip: bool,

    /// N’écrit rien, vérifie seulement que l’assemblage passe
    #[arg(long, action=ArgAction::SetTrue)]
    check: bool,

    /// Affiche le désassemblage du chunk assemblé (stdout)
    #[arg(long, action=ArgAction::SetTrue)]
    disasm: bool,

    /// Écrit le désassemblage dans ce fichier (.txt)
    #[arg(long)]
    emit_disasm: Option<PathBuf>,

    /// Valide le chunk en le rechargeant depuis les bytes
    #[arg(long, action=ArgAction::SetTrue)]
    verify: bool,

    /// Nom logique du fichier quand l’entrée est '-' (stdin)
    #[arg(long, default_value = "<stdin>")]
    stdin_name: String,

    /// Affiche la durée d’assemblage
    #[arg(long, action=ArgAction::SetTrue)]
    time: bool,

    /// Échoue si le chunk ne contient aucun opcode
    #[arg(long, action=ArgAction::SetTrue)]
    strict: bool,
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
        return Err(anyhow!("Aucune entrée fournie. Exemple: vitte-asm src.vit.s -o out.vitbc"));
    }

    // Gestion des cas stdin/multi-fichiers
    let use_stdin = cli.inputs.len() == 1 && cli.inputs[0] == "-";
    if use_stdin && cli.out_dir.is_some() {
        return Err(anyhow!("`--out-dir` n’a pas de sens pour stdin; utilise `--out`"));
    }
    if cli.inputs.len() > 1 && cli.out.is_some() {
        return Err(anyhow!("Avec plusieurs entrées, utilise `--out-dir` au lieu de `--out`"));
    }

    if use_stdin {
        // Un seul flux stdin
        let (src, in_name) = read_source("-", &cli.stdin_name)?;
        run_one(&src, &in_name, &cli)?;
    } else {
        // Plusieurs fichiers
        for inp in &cli.inputs {
            let (src, in_name) = read_source(inp, &cli.stdin_name)?;
            run_one(&src, &in_name, &cli)?;
        }
    }

    Ok(())
}

fn run_one(src: &str, in_name: &Utf8Path, cli: &Cli) -> Result<()> {
    let t0 = Instant::now();

    // Assemble
    let chunk = assemble_src(src, in_name.as_str(), cli.strip)?;

    if cli.strict && chunk.ops.is_empty() {
        return Err(anyhow!("Chunk vide (aucune instruction) — échec dû à --strict"));
    }

    // Vérification optionnelle
    if cli.verify {
        let bytes = chunk.to_bytes();
        let re = VChunk::from_bytes(&bytes)
            .map_err(|e| anyhow!("Round-trip bincode échoué: {e}"))?;
        helpers::validate_chunk(&re)?;
    }

    // Check only ?
    if cli.check {
        if cli.disasm {
            let title = in_name.as_str();
            println!("{}", disassemble_full(&chunk, title));
        }
        if let Some(path) = &cli.emit_disasm {
            write_text(path, &disassemble_full(&chunk, in_name.file_name().unwrap_or("chunk")))?;
            eprintln!("📝 Disasm écrit → {}", Utf8PathBuf::from_path_buf(path.clone()).unwrap_or_else(|_| Utf8PathBuf::from("<invalid>")));
        }
        if cli.time {
            eprintln!("⏱️  {}", human_millis(t0.elapsed()));
        }
        eprintln!("✅ Assemblage OK (check-only) — {}", in_name);
        return Ok(());
    }

    // Déterminer le chemin de sortie
    let out_path = if let Some(ref out) = cli.out {
        if cli.inputs_len_is_many() {
            return Err(anyhow!("Plusieurs entrées → utilise --out-dir"));
        }
        Utf8PathBuf::from_path_buf(out.clone()).map_err(|_| anyhow!("Chemin out non-UTF8"))?
    } else if let Some(ref dir) = cli.out_dir {
        let dir = Utf8PathBuf::from_path_buf(dir.clone()).map_err(|_| anyhow!("Chemin out-dir non-UTF8"))?;
        let file = default_filename(in_name);
        dir.join(file)
    } else {
        default_out_path(in_name)?
    };

    // Écriture bytes
    let bytes = chunk.to_bytes();
    write_bytes(&out_path, &bytes)?;
    eprintln!("✅ Assemblé → {}", out_path);

    // Désassemblage stdout
    if cli.disasm {
        let title = out_path.file_name().unwrap_or("chunk");
        println!("{}", disassemble_full(&chunk, title));
    }

    // Disasm vers fichier
    if let Some(path) = &cli.emit_disasm {
        write_text(path, &disassemble_full(&chunk, out_path.file_name().unwrap_or("chunk")))?;
        eprintln!("📝 Disasm écrit → {}", Utf8PathBuf::from_path_buf(path.clone()).unwrap_or_else(|_| Utf8PathBuf::from("<invalid>")));
    }

    if cli.time {
        eprintln!("⏱️  {}", human_millis(t0.elapsed()));
    }

    Ok(())
}

fn assemble_src(src: &str, main_file: &str, strip: bool) -> Result<VChunk> {
    // Assemble
    let mut chunk = vitte_core::asm::assemble(src)
        .with_context(|| format!("Erreur d’assemblage ({main_file})"))?;

    // Injecter le nom de fichier en debug si absent
    if !strip && chunk.debug.main_file.is_none() {
        chunk.debug.main_file = Some(main_file.to_string());
    }

    if strip {
        // Reconstruire un chunk propre avec flag stripped = true
        chunk = strip_chunk(&chunk);
    }

    // Validation légère
    helpers::validate_chunk(&chunk)?;
    Ok(chunk)
}

fn strip_chunk(src: &VChunk) -> VChunk {
    let mut dst = VChunk::new(ChunkFlags { stripped: true });
    // Copie ops/consts/lines
    for (_, c) in src.consts.iter() {
        dst.add_const(c.clone());
    }
    for (pc, op) in src.ops.iter().enumerate() {
        // On recopie la line info si dispo
        let line = src.lines.line_for_pc(pc as u32);
        dst.push_op(*op, line);
    }
    // Debug minimal (stripped)
    dst.debug = DebugInfo::default();
    dst
}

fn read_source(input: &str, stdin_name: &str) -> Result<(String, Utf8PathBuf)> {
    if input == "-" {
        let mut s = String::new();
        io::stdin().read_to_string(&mut s)?;
        Ok((s, Utf8PathBuf::from(stdin_name)))
    } else {
        let p = Utf8PathBuf::from(input);
        let s = fs::read_to_string(&p)
            .with_context(|| format!("Lecture échouée: {p}"))?;
        Ok((s, p))
    }
}

fn default_out_path(in_name: &Utf8Path) -> Result<Utf8PathBuf> {
    Ok(in_name.with_extension("vitbc").to_path_buf())
}

fn default_filename(in_name: &Utf8Path) -> String {
    let stem = in_name.file_stem().unwrap_or("out");
    format!("{stem}.vitbc")
}

fn write_bytes(path: &Utf8Path, bytes: &[u8]) -> Result<()> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let mut f = fs::File::create(path)?;
    f.write_all(bytes)?;
    Ok(())
}

fn write_text(path: &PathBuf, s: &str) -> Result<()> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let mut f = fs::File::create(path)?;
    f.write_all(s.as_bytes())?;
    Ok(())
}

// Petit sucre
trait InputsLen {
    fn inputs_len_is_many(&self) -> bool;
}
impl InputsLen for Cli {
    fn inputs_len_is_many(&self) -> bool { self.inputs.len() > 1 }
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
