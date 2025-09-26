//! E2E pipeline benches for Vitte
//!
//! Deux modes :
//! - CLI (défaut) : spawn `vitte pack` puis `vitte run` (robuste, aucune API interne requise).
//! - Lib (feature `bench-lib`) : à activer si/qd tu exposes des fonctions publiques stables.
//!
//! Lancement :
//!   cargo bench -p vitte-benches
//!   VITTE_BIN=./target/release/vitte cargo bench -p vitte-benches
//!   cargo bench -p vitte-benches -- --save-baseline main
//!   cargo bench -p vitte-benches -- --baseline main
//!
//! Données attendues (tu peux adapter les chemins) :
//!   benchmarks/data/tiny/hello.vit
//!   benchmarks/data/mid/router.vit
//!   benchmarks/data/big/compiler.vit

use anyhow::{Context, Result};
use criterion::{black_box, criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use std::path::{Path, PathBuf};
use std::process::Command;
use std::{env, fs};
use tempfile::tempdir;

#[derive(Clone, Debug)]
struct Case {
    name: &'static str,
    path: &'static str,
}

const CASES: &[Case] = &[
    Case { name: "tiny/hello", path: "benchmarks/data/tiny/hello.vit" },
    Case { name: "mid/router", path: "benchmarks/data/mid/router.vit" },
    Case { name: "big/compiler", path: "benchmarks/data/big/compiler.vit" },
];

// --------- Utilitaires communs ---------

fn read_src(path: &str) -> Result<String> {
    Ok(fs::read_to_string(path).with_context(|| format!("read {path}"))?)
}

fn vitte_bin() -> PathBuf {
    // 1) Env override
    if let Ok(p) = env::var("VITTE_BIN") {
        return PathBuf::from(p);
    }
    // 2) Heuristiques locales : release puis debug
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap().to_path_buf();
    let release = root.join("target").join("release").join(vitte_exe_name());
    if release.exists() {
        return release;
    }
    let debug = root.join("target").join("debug").join(vitte_exe_name());
    if debug.exists() {
        return debug;
    }
    // 3) fallback PATH
    PathBuf::from(vitte_exe_name())
}

#[cfg(target_os = "windows")]
fn vitte_exe_name() -> &'static str {
    "vitte.exe"
}
#[cfg(not(target_os = "windows"))]
fn vitte_exe_name() -> &'static str {
    "vitte"
}

// --------- Mode CLI (défaut) ---------

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

// --------- (Optionnel) Mode Lib : adapter ici quand les APIs sont prêtes ---------
#[cfg(feature = "bench-lib")]
mod libmode {
    use super::*;
    // Exemple d’APIs attendues. Adapte les imports/fi signatures à tes crates.
    use vitte_ir as ir;
    use vitte_parser as parser;
    use vitte_runtime as runtime;
    use vitte_vitbc as vitbc;

    pub fn compile_to_vitbc(src: &str) -> Result<Vec<u8>> {
        // EXEMPLES — change selon tes vraies APIs
        let ast = parser::parse_str(src).context("parser::parse_str")?;
        let ir = ir::build_ir(&ast).context("ir::build_ir")?;
        let bc = vitbc::encode(&ir).context("vitbc::encode")?;
        Ok(bc)
    }

    pub fn run_vitbc(bytes: &[u8]) -> Result<()> {
        runtime::run_vitbc(bytes).context("runtime::run_vitbc")
    }
}

// --------- Benches ---------

fn bench_e2e_pack_and_run(c: &mut Criterion) {
    let vitte = vitte_bin();

    let mut group = c.benchmark_group("e2e/pack+run");
    group.sample_size(20); // spawn CLI → un peu plus lourd
    group.warm_up_time(std::time::Duration::from_millis(300));
    group.measurement_time(std::time::Duration::from_secs(6));

    for case in CASES {
        let src_text = read_src(case.path).unwrap_or_else(|e| {
            eprintln!("⚠️  ignore {}: {}", case.name, e);
            String::from("do main(){ print(\"hello\") }")
        });

        group.throughput(Throughput::Bytes(src_text.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(case.name), &src_text, |b, src| {
            let tmp = tempdir().expect("tmp");
            let src_path = tmp.path().join(format!("{}.vit", case.name.replace('/', "_")));
            let out_path = tmp.path().join(format!("{}.vitbc", case.name.replace('/', "_")));
            fs::write(&src_path, src).unwrap();

            b.iter(|| {
                // pack
                cli_pack(&vitte, &src_path, &out_path).unwrap();
                // run
                cli_run(&vitte, &out_path).unwrap();
                black_box(());
            });
        });
    }

    group.finish();
}

fn bench_run_only(c: &mut Criterion) {
    let vitte = vitte_bin();

    let mut group = c.benchmark_group("vm/run-only");
    group.sample_size(30);
    group.warm_up_time(std::time::Duration::from_millis(300));
    group.measurement_time(std::time::Duration::from_secs(6));

    for case in CASES {
        // Pré-pack une fois pour isoler le run
        let tmp = tempdir().expect("tmp");
        let src = read_src(case.path).unwrap_or_else(|_| "do main(){print(\"x\")}".into());
        let src_path = tmp.path().join("bench.vit");
        let out_path = tmp.path().join("bench.vitbc");
        fs::write(&src_path, &src).unwrap();
        cli_pack(&vitte, &src_path, &out_path).unwrap();

        let size = fs::metadata(&out_path).map(|m| m.len()).unwrap_or(0);
        group.throughput(Throughput::Bytes(size));

        group.bench_with_input(BenchmarkId::from_parameter(case.name), &out_path, |b, vitbc| {
            b.iter(|| {
                cli_run(&vitte, vitbc).unwrap();
                black_box(());
            });
        });
    }

    group.finish();
}

#[cfg(feature = "bench-lib")]
fn bench_lib_pipeline(c: &mut Criterion) {
    use libmode::*;
    let mut group = c.benchmark_group("e2e/lib");
    group.sample_size(40);
    group.warm_up_time(std::time::Duration::from_millis(300));
    group.measurement_time(std::time::Duration::from_secs(6));

    for case in CASES {
        let src = read_src(case.path).unwrap_or_else(|_| "do main(){print(\"x\")}".into());
        group.throughput(Throughput::Bytes(src.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(case.name), &src, |b, s| {
            b.iter(|| {
                let bc = compile_to_vitbc(black_box(s)).unwrap();
                run_vitbc(black_box(&bc)).unwrap();
            });
        });
    }

    group.finish();
}

#[cfg(not(feature = "bench-lib"))]
fn bench_lib_pipeline(_c: &mut Criterion) {
    // no-op quand la feature n’est pas activée
}

criterion_group!(benches, bench_e2e_pack_and_run, bench_run_only, bench_lib_pipeline);
criterion_main!(benches);
