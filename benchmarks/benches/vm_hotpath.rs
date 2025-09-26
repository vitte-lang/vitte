//! vm_hotpath.rs — micro-benchs « hot path » de la VM/runtime Vitte
//!
//! Modes :
//!   • CLI (défaut) : pré-packe les sources, puis bench `vitte run file.vitbc`.
//!   • Lib (feature `bench-lib`) : compile→exécute en mémoire via des APIs stables (si dispo).
//!
//! Lancer :
//!   cargo bench -p vitte-benches --bench vm_hotpath
//!   VITTE_BIN=./target/release/vitte cargo bench -p vitte-benches --bench vm_hotpath
//!   cargo bench -p vitte-benches --bench vm_hotpath -- --save-baseline hot
//!   cargo bench -p vitte-benches --bench vm_hotpath -- --baseline hot
//!
//! Données : pas besoin de corpus externe, tout est self-contained (sources embarquées).
//!
//! NB: Le spawn de process a un coût ; ce bench vise la stabilité et la régression visible.
//!     Pour des mesures ultra-fines, préférez le mode `bench-lib` dès que l'API est dispo.

use anyhow::{Context, Result};
use criterion::{black_box, criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use std::path::{Path, PathBuf};
use std::{env, fs, process::Command};
use tempfile::tempdir;

// ────────────────────────────────────────────────────────────────────────────
// Cas « hot path » : petits programmes Vitte auto-contenus
// (syntaxe indicative, adapte si ton front change)
// ────────────────────────────────────────────────────────────────────────────

#[derive(Clone, Debug)]
struct Micro {
    name: &'static str,
    src: &'static str,
}

const MICROS: &[Micro] = &[
    Micro {
        name: "arith/add_loop",
        src: r#"
            do main() {
                let mut acc: i64 = 0
                let mut i: i64 = 0
                while i < 2_000_000 {
                    acc = acc + i
                    i = i + 1
                }
                // pas d'I/O
            }
        "#,
    },
    Micro {
        name: "branch/predictable_if",
        src: r#"
            do main() {
                let mut x: i32 = 0
                let mut i: i32 = 0
                while i < 1_500_000 {
                    if (i & 1) == 0 {
                        x = x + 3
                    } else {
                        x = x - 2
                    }
                    i = i + 1
                }
            }
        "#,
    },
    Micro {
        name: "match/mod5",
        src: r#"
            do main() {
                let mut s: i32 = 0
                let mut i: i32 = 0
                while i < 1_200_000 {
                    let m = i % 5
                    match m {
                        0 => { s = s + 1 }
                        1 => { s = s + 2 }
                        2 => { s = s + 3 }
                        3 => { s = s + 4 }
                        _ => { s = s + 5 }
                    }
                    i = i + 1
                }
            }
        "#,
    },
    Micro {
        name: "calls/call_chain",
        src: r#"
            do f1(x: i32) -> i32 { x + 1 }
            do f2(x: i32) -> i32 { f1(x) + 1 }
            do f3(x: i32) -> i32 { f2(x) + 1 }
            do f4(x: i32) -> i32 { f3(x) + 1 }
            do f5(x: i32) -> i32 { f4(x) + 1 }

            do main() {
                let mut acc: i32 = 0
                let mut i: i32 = 0
                while i < 600_000 {
                    acc = acc + f5(i)
                    i = i + 1
                }
            }
        "#,
    },
    Micro {
        name: "recursion/fib20",
        src: r#"
            do fib(n: i32) -> i32 {
                if n <= 1 { n } else { fib(n - 1) + fib(n - 2) }
            }
            do main() {
                // Appel multiple pour chauffer le code de récursion
                let mut i: i32 = 0
                let mut acc: i32 = 0
                while i < 12 {
                    acc = acc + fib(20)
                    i = i + 1
                }
            }
        "#,
    },
];

// ────────────────────────────────────────────────────────────────────────────
// Outils
// ────────────────────────────────────────────────────────────────────────────

fn vitte_exe_name() -> &'static str {
    #[cfg(target_os = "windows")]
    {
        "vitte.exe"
    }
    #[cfg(not(target_os = "windows"))]
    {
        "vitte"
    }
}

fn vitte_bin() -> PathBuf {
    // 1) Env override
    if let Ok(p) = env::var("VITTE_BIN") {
        return PathBuf::from(p);
    }
    // 2) Heuristiques locales : release puis debug à partir du repo root
    let repo_root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap().to_path_buf();
    let release = repo_root.join("target").join("release").join(vitte_exe_name());
    if release.exists() {
        return release;
    }
    let debug = repo_root.join("target").join("debug").join(vitte_exe_name());
    if debug.exists() {
        return debug;
    }
    // 3) fallback PATH
    PathBuf::from(vitte_exe_name())
}

fn cli_pack(vitte: &Path, src: &Path, out: &Path) -> Result<()> {
    let status = Command::new(vitte)
        .args(["pack", src.to_str().unwrap(), "-o", out.to_str().unwrap()])
        .status()
        .context("spawn vitte pack")?;
    if !status.success() {
        anyhow::bail!("vitte pack failed with status {status}");
    }
    Ok(())
}

fn cli_run(vitte: &Path, vitbc: &Path) -> Result<()> {
    let status = Command::new(vitte)
        .args(["run", vitbc.to_str().unwrap()])
        .status()
        .context("spawn vitte run")?;
    if !status.success() {
        anyhow::bail!("vitte run failed with status {status}");
    }
    Ok(())
}

// ────────────────────────────────────────────────────────────────────────────
// (Optionnel) Mode lib — active `--features bench-lib` quand les APIs existent
// ────────────────────────────────────────────────────────────────────────────
#[cfg(feature = "bench-lib")]
mod libmode {
    use super::*;
    // Adapte ces imports/chemins à tes crates effectifs
    use vitte_ir as ir;
    use vitte_parser as parser;
    use vitte_runtime as runtime;
    use vitte_vitbc as vitbc;

    pub fn compile_to_vitbc(src: &str) -> Result<Vec<u8>> {
        // EXEMPLE — remplis avec tes vraies APIs publiques
        let ast = parser::parse_str(src).context("parser::parse_str")?;
        let mir = ir::build_ir(&ast).context("ir::build_ir")?;
        let bc = vitbc::encode(&mir).context("vitbc::encode")?;
        Ok(bc)
    }

    pub fn run_vitbc(bytes: &[u8]) -> Result<()> {
        runtime::run_vitbc(bytes).context("runtime::run_vitbc")
    }
}

#[cfg(not(feature = "bench-lib"))]
mod libmode {
    // Bouchons pour compilation sans la feature
    use super::*;
    pub fn compile_to_vitbc(_src: &str) -> Result<Vec<u8>> {
        anyhow::bail!("bench-lib non activée")
    }
    pub fn run_vitbc(_bytes: &[u8]) -> Result<()> {
        anyhow::bail!("bench-lib non activée")
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Benchs
// ────────────────────────────────────────────────────────────────────────────

fn bench_vm_run_only_cli(c: &mut Criterion) {
    let vitte = vitte_bin();
    let tmp = tempdir().expect("tmpdir hotpath");

    // Pré-packe tous les micros une fois
    let mut compiled: Vec<(String, PathBuf, usize)> = Vec::new();
    for m in MICROS {
        let src_path = tmp.path().join(format!("{}.vit", m.name.replace('/', "_")));
        let out_path = tmp.path().join(format!("{}.vitbc", m.name.replace('/', "_")));
        fs::write(&src_path, m.src).expect("write src");
        cli_pack(&vitte, &src_path, &out_path).unwrap_or_else(|e| {
            panic!("pack failed for {}: {e}", m.name);
        });
        let size = fs::metadata(&out_path).map(|m| m.len() as usize).unwrap_or(0);
        compiled.push((m.name.to_string(), out_path, size));
    }

    let mut group = c.benchmark_group("vm/hotpath/run-only( CLI )");
    group.sample_size(35);
    group.warm_up_time(std::time::Duration::from_millis(400));
    group.measurement_time(std::time::Duration::from_secs(8));

    for (name, vitbc_path, size) in compiled {
        group.throughput(Throughput::Bytes(size as u64));
        group.bench_with_input(BenchmarkId::from_parameter(name), &vitbc_path, |b, p| {
            b.iter(|| {
                cli_run(&vitte, p).unwrap();
                black_box(());
            });
        });
    }

    group.finish();
}

fn bench_vm_pipeline_lib(c: &mut Criterion) {
    // Bench en mémoire (si bench-lib dispo), sinon il s’exécute mais no-op.
    // On garde le groupe pour que Criterion affiche le résultat de façon homogène.
    let mut group = c.benchmark_group("vm/hotpath(pipeline LIB)");
    group.sample_size(45);
    group.warm_up_time(std::time::Duration::from_millis(300));
    group.measurement_time(std::time::Duration::from_secs(8));

    for m in MICROS {
        group.throughput(Throughput::Bytes(m.src.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(m.name), &m.src, |b, src| {
            b.iter(|| {
                #[cfg(feature = "bench-lib")]
                {
                    let bc = libmode::compile_to_vitbc(black_box(src)).unwrap();
                    libmode::run_vitbc(black_box(&bc)).unwrap();
                }
                #[cfg(not(feature = "bench-lib"))]
                {
                    // Pas d’API lib : on évite d’exploser, et on compte zéro travail.
                    black_box(());
                }
            });
        });
    }

    group.finish();
}

criterion_group!(benches, bench_vm_run_only_cli, bench_vm_pipeline_lib);
criterion_main!(benches);
