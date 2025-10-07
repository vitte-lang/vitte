// crates/vitte-tools/src/bin/vitte-disasm.rs
//! Désassembleur Vitte pour fichiers `.vitbc`.
//!
//! Exemples :
//!   vitte-disasm a.vitbc
//!   vitte-disasm a.vitbc b.vitbc --out-dir target/disasm/
//!   cat a.vitbc | vitte-disasm - --stdin-name a.vitbc --disasm
//!   vitte-disasm a.vitbc --json | jq
//!
//! Options utiles :
//!   --disasm        : imprime le désassemblage lisible sur stdout
//!   --emit <f>      : écrit le désassemblage dans un fichier
//!   --json          : imprime une vue JSON structurée
//!   --emit-json <f> : écrit le JSON dans un fichier
//!   --compact       : listing 1-ligne-par-op (moins verbeux)
//!   --summary       : affiche un résumé (consts/ops/version/flags)
//!   --verify        : round-trip (lecture/validation)
//!   --color <mode>  : auto|always|never (couleurs pour résumé)
//!   --time          : chrono
//!   --strict        : échec si chunk suspect (ex: 0 op)
//!
//! NB: pour plusieurs entrées, utiliser --out-dir/--emit-json-dir.

use std::fs;
use std::io::{self, Read, Write};
use std::path::PathBuf;
use std::time::Instant;

use anyhow::{Context, Result, anyhow};
use camino::{Utf8Path, Utf8PathBuf};
use clap::{ArgGroup, Parser, ValueEnum};
use serde::Serialize;
use yansi::{Color, Paint};

use vitte_core::bytecode::{ConstValue, Op, chunk::Chunk as VChunk};
use vitte_core::disasm::{disassemble_compact, disassemble_full};
use vitte_core::helpers;
use vitte_tools::{ColorMode as GlobalColorMode, setup_colors as global_setup_colors};

#[derive(Clone, Copy, Debug, ValueEnum)]
enum ColorMode {
    Auto,
    Always,
    Never,
}

#[derive(Parser, Debug)]
#[command(
    name = "vitte-disasm",
    version,
    about = "Désassembleur Vitte (.vitbc -> texte/JSON)"
)]
#[command(group(
    ArgGroup::new("stdout_mode")
        .args(["disasm", "json", "summary"])
        .multiple(true)
))]
struct Cli {
    /// Fichier(s) .vitbc à analyser (ou '-' pour stdin, unique)
    inputs: Vec<String>,

    /// Affiche le désassemblage lisible sur stdout
    #[arg(long)]
    disasm: bool,

    /// Chemin fichier où écrire le désassemblage (si 1 entrée)
    #[arg(long, conflicts_with = "out_dir")]
    emit: Option<PathBuf>,

    /// Dossier où écrire le(s) désassemblage(s) (si N entrées)
    #[arg(long)]
    out_dir: Option<PathBuf>,

    /// Affiche un JSON structuré sur stdout
    #[arg(long)]
    json: bool,

    /// Écrit le JSON structuré dans un fichier (si 1 entrée)
    #[arg(long, conflicts_with = "emit_json_dir")]
    emit_json: Option<PathBuf>,

    /// Dossier où écrire les JSON (si N entrées)
    #[arg(long)]
    emit_json_dir: Option<PathBuf>,

    /// Résumé rapide (counts/flags/version/hash)
    #[arg(long)]
    summary: bool,

    /// Vue compacte (1 ligne par op)
    #[arg(long)]
    compact: bool,

    /// Valide le chunk en le rechargeant depuis les bytes
    #[arg(long)]
    verify: bool,

    /// Échec si chunk vide (0 op)
    #[arg(long)]
    strict: bool,

    /// Nom logique quand l’entrée est '-' (stdin)
    #[arg(long, default_value = "<stdin>")]
    stdin_name: String,

    /// Affiche la durée de traitement
    #[arg(long)]
    time: bool,

    /// Couleurs du résumé: auto|always|never
    #[arg(long, value_enum, default_value_t = ColorMode::Auto)]
    color: ColorMode,
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
    setup_colors(cli.color);

    if cli.inputs.is_empty() {
        return Err(anyhow!("Aucune entrée. Exemple: vitte-disasm a.vitbc --disasm"));
    }
    let use_stdin = cli.inputs.len() == 1 && cli.inputs[0] == "-";
    if use_stdin {
        if cli.out_dir.is_some() || cli.emit_json_dir.is_some() {
            return Err(anyhow!(
                "--out-dir / --emit-json-dir n’ont pas de sens avec stdin; utilise --emit/--emit-json"
            ));
        }
    } else {
        if cli.inputs.len() > 1 && (cli.emit.is_some() || cli.emit_json.is_some()) {
            return Err(anyhow!(
                "Plusieurs entrées → utilise --out-dir / --emit-json-dir au lieu de --emit / --emit-json"
            ));
        }
    }

    if use_stdin {
        let (bytes, name) = read_input("-", &cli.stdin_name)?;
        process_one(&bytes, &name, &cli)?;
    } else {
        for inp in &cli.inputs {
            let (bytes, name) = read_input(inp, &cli.stdin_name)?;
            process_one(&bytes, &name, &cli)?;
        }
    }

    Ok(())
}

fn process_one(bytes: &[u8], name: &Utf8Path, cli: &Cli) -> Result<()> {
    let t0 = Instant::now();
    // Chargement (validation format/hash assurée par from_bytes)
    let chunk = VChunk::from_bytes(bytes).with_context(|| format!("Chargement échoué: {name}"))?;
    helpers::validate_chunk(&chunk)?;

    if cli.strict && chunk.ops.is_empty() {
        return Err(anyhow!("Chunk vide (0 op) — échec dû à --strict ({name})"));
    }

    // Résumé optionnel
    if cli.summary {
        print_summary(&chunk, name);
    }

    // Vue texte (désasm)
    if cli.disasm || cli.emit.is_some() || cli.out_dir.is_some() {
        let title = name.file_name().unwrap_or("chunk");
        let txt =
            if cli.compact { disassemble_compact(&chunk) } else { disassemble_full(&chunk, title) };

        if let Some(file) = &cli.emit {
            let out = Utf8PathBuf::from_path_buf(file.clone())
                .map_err(|_| anyhow!("Chemin `--emit` non UTF-8"))?;
            write_text(&out, &txt)?;
            eprintln!("📝 Disasm → {out}");
        } else if let Some(dir) = &cli.out_dir {
            let dir = Utf8PathBuf::from_path_buf(dir.clone())
                .map_err(|_| anyhow!("Chemin `--out-dir` non UTF-8"))?;
            let file = default_disasm_filename(name, cli.compact);
            let out = dir.join(file);
            write_text(&out, &txt)?;
            eprintln!("📝 Disasm → {out}");
        } else if cli.disasm {
            // stdout
            println!("{txt}");
        }
    }

    // Vue JSON
    if cli.json || cli.emit_json.is_some() || cli.emit_json_dir.is_some() {
        let j = build_json(&chunk, name);
        let pretty = serde_json::to_string_pretty(&j)?;
        if let Some(file) = &cli.emit_json {
            let out = Utf8PathBuf::from_path_buf(file.clone())
                .map_err(|_| anyhow!("Chemin `--emit-json` non UTF-8"))?;
            write_text(&out, &pretty)?;
            eprintln!("🧾 JSON → {out}");
        } else if let Some(dir) = &cli.emit_json_dir {
            let dir = Utf8PathBuf::from_path_buf(dir.clone())
                .map_err(|_| anyhow!("Chemin `--emit-json-dir` non UTF-8"))?;
            let file = default_json_filename(name);
            let out = dir.join(file);
            write_text(&out, &pretty)?;
            eprintln!("🧾 JSON → {out}");
        } else if cli.json {
            println!("{pretty}");
        }
    }

    // Round-trip de validation (facultatif)
    if cli.verify {
        let rt = chunk.to_bytes();
        let chk = VChunk::from_bytes(&rt)?;
        helpers::validate_chunk(&chk)?;
        eprintln!("{}", "✓ verify round-trip OK".paint(Color::Green));
    }

    if cli.time {
        eprintln!("⏱️  {}", human_millis(t0.elapsed()));
    }

    Ok(())
}

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

fn write_text(path: &Utf8Path, s: &str) -> Result<()> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let mut f = fs::File::create(path)?;
    f.write_all(s.as_bytes())?;
    Ok(())
}

fn default_disasm_filename(input: &Utf8Path, compact: bool) -> String {
    let stem = input.file_stem().unwrap_or("chunk");
    if compact { format!("{stem}.compact.disasm.txt") } else { format!("{stem}.disasm.txt") }
}

fn default_json_filename(input: &Utf8Path) -> String {
    let stem = input.file_stem().unwrap_or("chunk");
    format!("{stem}.disasm.json")
}

fn setup_colors(mode: ColorMode) {
    let mapped = match mode {
        ColorMode::Auto => GlobalColorMode::Auto,
        ColorMode::Always => GlobalColorMode::Always,
        ColorMode::Never => GlobalColorMode::Never,
    };
    global_setup_colors(mapped);
}

fn print_summary(chunk: &VChunk, name: &Utf8Path) {
    let ops = chunk.ops.len();
    let consts = chunk.consts.len();
    let stripped = chunk.flags().stripped;
    let version = chunk.version();
    let hash = chunk.compute_hash();

    let title = name.file_name().unwrap_or("<stdin>");
    let hdr = format!("== {title} ==");
    eprintln!("{}", hdr.paint(Color::Cyan).bold());
    eprintln!(
        "{} ops={}, consts={}, version={}, stripped={}, hash=0x{hash:016x}",
        "•".paint(Color::Blue),
        ops,
        consts,
        version,
        stripped
    );

    if let Some(main) = &chunk.debug.main_file {
        eprintln!("{} main_file: {}", "•".paint(Color::Blue), main);
    }
    if !chunk.debug.files.is_empty() {
        eprintln!("{} files: {}", "•".paint(Color::Blue), chunk.debug.files.len());
    }
    if !chunk.debug.symbols.is_empty() {
        eprintln!("{} symbols: {}", "•".paint(Color::Blue), chunk.debug.symbols.len());
    }

    // Un petit aperçu des premières constantes
    let preview = chunk.consts.iter().take(5).map(|(i, v)| (i, show_const(v))).collect::<Vec<_>>();
    if !preview.is_empty() {
        eprintln!("{} consts[0..5]:", "•".paint(Color::Blue));
        for (i, s) in preview {
            eprintln!("   [{i}] {s}");
        }
    }
}

fn show_const(v: &ConstValue) -> String {
    match v {
        ConstValue::Null => "null".into(),
        ConstValue::Bool(b) => format!("{b}"),
        ConstValue::I64(i) => format!("{i}"),
        ConstValue::F64(x) => format!("{x}"),
        ConstValue::Str(s) => {
            if s.len() <= 64 {
                format!("\"{s}\"")
            } else {
                format!("\"{}…\"", &s[..64])
            }
        },
        ConstValue::Bytes(b) => format!("bytes[{}]", b.len()),
    }
}

#[derive(Serialize)]
struct ChunkJson<'a> {
    file: &'a str,
    version: u16,
    stripped: bool,
    consts: Vec<ConstJson<'a>>,
    ops: Vec<OpJson>,
    line_runs: Vec<LineRunJson>,
    debug: DebugJson<'a>,
    hash: u64,
}

#[derive(Serialize)]
struct ConstJson<'a> {
    index: u32,
    #[serde(rename = "type")]
    ty: &'a str,
    value: serde_json::Value,
}

#[derive(Serialize)]
struct OpJson {
    pc: u32,
    line: Option<u32>,
    op: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    load_const_preview: Option<String>,
}

#[derive(Serialize)]
struct LineRunJson {
    range_start: u32,
    range_end: u32,
    line: u32,
}

#[derive(Serialize)]
struct DebugJson<'a> {
    main_file: Option<&'a str>,
    files: &'a [String],
    symbols: &'a [(String, u32)],
}

fn build_json<'a>(chunk: &'a VChunk, name: &'a Utf8Path) -> ChunkJson<'a> {
    let consts = chunk
        .consts
        .iter()
        .map(|(i, v)| ConstJson {
            index: i,
            ty: match v {
                ConstValue::Null => "null",
                ConstValue::Bool(_) => "bool",
                ConstValue::I64(_) => "i64",
                ConstValue::F64(_) => "f64",
                ConstValue::Str(_) => "str",
                ConstValue::Bytes(_) => "bytes",
            },
            value: match v {
                ConstValue::Null => serde_json::Value::Null,
                ConstValue::Bool(b) => serde_json::json!(b),
                ConstValue::I64(x) => serde_json::json!(x),
                ConstValue::F64(x) => serde_json::json!(x),
                ConstValue::Str(s) => serde_json::json!(s),
                ConstValue::Bytes(b) => serde_json::json!({ "len": b.len() }),
            },
        })
        .collect::<Vec<_>>();

    let mut ops = Vec::with_capacity(chunk.ops.len());
    for (pc, op) in chunk.ops.iter().enumerate() {
        let pc = pc as u32;
        let line = chunk.lines.line_for_pc(pc);
        let mut preview = None;
        if let Op::LoadConst(ix) = *op {
            if let Some(v) = chunk.consts.get(ix) {
                preview = Some(show_const(v));
            }
        }
        ops.push(OpJson {
            pc,
            line: Some(line),
            op: format!("{op:?}"),
            load_const_preview: preview,
        });
    }

    let line_runs = chunk
        .lines
        .iter_ranges()
        .map(|(r, line)| LineRunJson { range_start: r.start, range_end: r.end, line })
        .collect::<Vec<_>>();

    ChunkJson {
        file: name.as_str(),
        version: chunk.version(),
        stripped: chunk.flags().stripped,
        consts,
        ops,
        line_runs,
        debug: DebugJson {
            main_file: chunk.debug.main_file.as_deref(),
            files: &chunk.debug.files,
            symbols: &chunk.debug.symbols,
        },
        hash: chunk.compute_hash(),
    }
}

fn human_millis(d: std::time::Duration) -> String {
    let ms = d.as_millis();
    if ms < 1_000 {
        return format!("{ms} ms");
    }
    let s = ms as f64 / 1000.0;
    if s < 60.0 {
        return format!("{s:.3} s");
    }
    let m = (s / 60.0).floor();
    let rest = s - m * 60.0;
    format!("{m:.0} min {rest:.1} s")
}
