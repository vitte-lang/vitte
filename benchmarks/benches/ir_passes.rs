//! Benchmarks des passes IR (Criterion) — **ultra complet**
//!
//! Mesure des passes classiques: CFG, DFG, Dominators, SSA, ConstProp, DCE, Inline,
//! plus un *pipeline* combiné. On bench sur :
//!   • micro-corpus embarqué,
//!   • corpus synthétiques (tailles en KiB),
//!   • corpus externe (.vit) sous IR_DIR (optionnel).
//!
//! ⚙️ Variables d’environnement (facultatives) :
//!   - CRIT_SAMPLES      (def=50)
//!   - CRIT_WARMUP_MS    (def=300)
//!   - CRIT_MEASURE_MS   (def=1200)
//!   - IR_DIR            (def="benchmarks/data/projects")  # racine projets .vit
//!   - IR_MAX_MB         (def=4)                           # limite par projet
//!   - IR_PASSES         (def="cfg,dfg,dom,ssa,constprop,dce,inline,pipeline")
//!   - IR_VALIDATE       (def=0)   # 1 => appelle `verify_ir()` après chaque passe
//!   - IR_REBUILD_EACH   (def=0)   # 1 => reconstruit l’IR depuis le source à chaque itération
//!
//! 🔧 ADAPTATION REQUISE (quelques lignes marquées **ADAPT ME**):
//!   1) `build_ir_from_src(&str) -> Result<Module>` : parse + lower vers Vitte IR
//!   2) `deep_clone_ir(&Module) -> Module` : clone profond (pour éviter d’altérer le même IR)
//!   3) `run_*(&mut Module)` pour chaque passe que tu exposes (CFG/DFG/Dom/SSA/etc.)
//!   4) `ir_stats(&Module) -> Option<IrStats>` + `verify_ir(&Module)` (si dispo)
//!
//! Si une passe n’existe pas côté API, laisse la fonction no-op (OK).

use anyhow::{anyhow, Context, Result};
use criterion::{black_box, criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use std::{
    ffi::OsStr,
    fs,
    path::{Path, PathBuf},
    time::Duration,
};

// Évite des import de types précis : on reste agnostique, mais on tire le crate.
use vitte_ir; // <— requis par Cargo (benchmarks/Cargo.toml), même si non utilisé directement.

// ======================================================================================
// Env helpers
// ======================================================================================

fn env_usize(key: &str, default: usize) -> usize {
    std::env::var(key).ok().and_then(|s| s.parse::<usize>().ok()).unwrap_or(default)
}
fn env_u64(key: &str, default: u64) -> u64 {
    std::env::var(key).ok().and_then(|s| s.parse::<u64>().ok()).unwrap_or(default)
}
fn env_bool(key: &str, default: bool) -> bool {
    std::env::var(key)
        .ok()
        .and_then(|s| s.parse::<u8>().ok())
        .map(|v| v != 0)
        .unwrap_or(default)
}
fn env_list(key: &str, default_csv: &str) -> Vec<String> {
    std::env::var(key)
        .unwrap_or_else(|_| default_csv.to_string())
        .split(',')
        .map(|s| s.trim().to_ascii_lowercase())
        .filter(|s| !s.is_empty())
        .collect()
}

// ======================================================================================
/*                                      ADAPTERS                                       */
// ======================================================================================
// Types opaques pour cacher l’impl concrète de l’IR à ce benchmark.
pub struct ModuleOpaque {
    // Remplace ce champ par ton vrai module IR si tu veux typer fort :
    // pub m: vitte_ir::Module,
    _private: (),
}

/* --------------------------- Construction / Clonage IR ------------------------------ */

/// Construit un Module IR depuis du source Vitte.
/// Idéalement: parse -> AST -> lower -> IR.
/// ADAPT ME: remplace ce stub par un vrai appel à tes crates vitte-parser / vitte-compiler / vitte-ir.
fn build_ir_from_src(src: &str) -> Result<ModuleOpaque> {
    // EXEMPLE (à adapter) :
    // let ast = vitte_parser::parse(src).context("parse")?;
    // let module: vitte_ir::Module = vitte_compiler::lower_to_ir(&ast).context("lower")?;
    // Ok(ModuleOpaque { m: module })

    // Fallback (à remplacer) :
    let _ = src;
    Err(anyhow!("build_ir_from_src(): non branché (ADAPT ME)"))
}

/// Clone profond d’un Module. Utilisé pour ne pas altérer le même IR entre itérations.
/// ADAPT ME: si ton Module impl Clone, fais un simple `.clone()`.
fn deep_clone_ir(_m: &ModuleOpaque) -> ModuleOpaque {
    // EXEMPLE :
    // ModuleOpaque { m: _m.m.clone() }

    // fallback “fake” (à remplacer) :
    ModuleOpaque { _private: () }
}

/* ------------------------------ Passes → no-op par défaut -------------------------- */

fn run_cfg(_m: &mut ModuleOpaque) -> Result<()> {
    // EXEMPLE :
    // vitte_ir::passes::cfg::build(&_m.m)?;
    Ok(())
}
fn run_dfg(_m: &mut ModuleOpaque) -> Result<()> {
    // EXEMPLE :
    // vitte_ir::passes::dfg::build(&_m.m)?;
    Ok(())
}
fn run_dom(_m: &mut ModuleOpaque) -> Result<()> {
    // EXEMPLE :
    // vitte_ir::passes::dominators::compute(&_m.m)?;
    Ok(())
}
fn run_ssa(_m: &mut ModuleOpaque) -> Result<()> {
    // EXEMPLE :
    // vitte_ir::passes::ssa::convert(&_m.m)?;
    Ok(())
}
fn run_constprop(_m: &mut ModuleOpaque) -> Result<()> {
    // EXEMPLE :
    // vitte_ir::passes::constprop::run(&_m.m)?;
    Ok(())
}
fn run_dce(_m: &mut ModuleOpaque) -> Result<()> {
    // EXEMPLE :
    // vitte_ir::passes::dce::run(&_m.m)?;
    Ok(())
}
fn run_inline(_m: &mut ModuleOpaque) -> Result<()> {
    // EXEMPLE :
    // vitte_ir::passes::inliner::run(&_m.m, vitte_ir::passes::inliner::Config { .. })?;
    Ok(())
}

/// Vérification d’invariants post-passe (si dispo).
fn verify_ir(_m: &ModuleOpaque) -> Result<()> {
    // EXEMPLE :
    // vitte_ir::verify(&_m.m).context("verify")?;
    Ok(())
}

/// Stats IR (pour metrics : fonctions/blocs/instr/edges).
#[derive(Debug, Clone, Copy, Default)]
struct IrStats {
    functions: usize,
    blocks: usize,
    instrs: usize,
    edges: usize,
}
fn ir_stats(_m: &ModuleOpaque) -> Option<IrStats> {
    // EXEMPLE :
    // Some(IrStats {
    //     functions: _m.m.funcs().len(),
    //     blocks:    _m.m.blocks_total(),
    //     instrs:    _m.m.instrs_total(),
    //     edges:     _m.m.edges_total(),
    // })
    None
}

/* ------------------------------ Pipelines ------------------------------------------ */

fn run_pipeline(mut m: ModuleOpaque) -> Result<ModuleOpaque> {
    run_cfg(&mut m)?;
    run_dfg(&mut m)?;
    run_dom(&mut m)?;
    run_ssa(&mut m)?;
    run_constprop(&mut m)?;
    run_dce(&mut m)?;
    Ok(m)
}

// ======================================================================================
// Corpus embarqué & synthétiques
// ======================================================================================

const SRC_MICRO_HELLO: &str = r#"
fn main() {
  let msg = "hello, vitte!";
  print(msg);
}
"#;

const SRC_MICRO_BRANCHES: &str = r#"
fn f(n) {
  if n < 10 { return n * 2; }
  let mut acc = 0;
  for i in 0..n { acc = acc + (i % 3); }
  acc
}
"#;

const SRC_MICRO_FUNCS: &str = r#"
fn a(x) { x + 1 }
fn b(x) { a(x) * 2 }
fn c(x) { if x > 0 { b(x) } else { 0 } }
"#;

const SRC_MICRO_CALLS: &str = r#"
fn fib(n) { if n < 2 { n } else { fib(n-1) + fib(n-2) } }
fn main() { fib(18) }
"#;

/// Gonfle un source jusqu’à ~kib KiB.
fn inflate_to_kib(seed: &str, kib: usize) -> String {
    let target = kib * 1024;
    let mut out = String::with_capacity(target + seed.len());
    while out.len() < target {
        out.push_str(seed);
        out.push('\n');
    }
    out
}

fn synthetic_loops(kib: usize) -> String {
    let seed = r#"
fn hot(n) {
  let mut s = 0;
  for i in 0..n {
    if (i & 1) == 0 { s = s + i; } else { s = s - 1; }
  }
  s
}"#;
    inflate_to_kib(seed, kib)
}

fn synthetic_calls(kib: usize) -> String {
    let seed = r#"
fn add(a,b){a+b} fn mul(a,b){a*b} fn mix(x){ mul(add(x,1), add(x,2)) }
"#;
    inflate_to_kib(seed, kib)
}

fn synthetic_branches(kib: usize) -> String {
    let seed = r#"
fn sel(x){
  match x {
    0 => 1,
    1..=3 => 2,
    _ => if x%2==0 { x/2 } else { 3*x+1 }
  }
}"#;
    inflate_to_kib(seed, kib)
}

// ======================================================================================
// Corpus externe (.vit dans projets/) — on compte la somme des .vit comme throughput
// ======================================================================================

fn project_src_bytes(dir: &Path) -> u64 {
    let mut total = 0u64;
    let _ = walk(dir, &mut |p| {
        if p.extension().and_then(OsStr::to_str) == Some("vit") {
            if let Ok(md) = fs::metadata(p) {
                total = total.saturating_add(md.len());
            }
        }
    });
    total
}
fn list_projects(root: &Path, max_mb: usize) -> Vec<PathBuf> {
    let mut out = Vec::new();
    let max_bytes = (max_mb as u64) * 1024 * 1024;
    if let Ok(rd) = fs::read_dir(root) {
        for e in rd.flatten() {
            let p = e.path();
            if p.is_dir() && project_src_bytes(&p) <= max_bytes {
                out.push(p);
            }
        }
    }
    out.sort();
    out
}
fn walk<F: FnMut(&Path)>(dir: &Path, f: &mut F) -> std::io::Result<()> {
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let p = entry.path();
        if p.is_dir() {
            walk(&p, f)?;
        } else {
            f(&p);
        }
    }
    Ok(())
}
fn sanitize_id(path: &Path) -> String {
    path.to_string_lossy()
        .replace('\\', "/")
        .replace("../", "")
        .replace("./", "")
        .replace(':', "_")
}

fn read_whole_project(root: &Path) -> String {
    // On concatène les .vit par ordre alphabétique → un seul “module source”.
    let mut files = Vec::new();
    let _ = walk(root, &mut |p| {
        if p.extension().and_then(OsStr::to_str) == Some("vit") {
            files.push(p.to_path_buf());
        }
    });
    files.sort();
    let mut buf = String::new();
    for f in files {
        if let Ok(s) = fs::read_to_string(&f) {
            buf.push_str(&s);
            buf.push('\n');
        }
    }
    buf
}

// ======================================================================================
// Bench helpers
// ======================================================================================

/// Prépare un IR et renvoie un *builder* de module frais pour chaque itération.
/// Deux stratégies :
///   - IR_REBUILD_EACH=1 ⇒ reconstruit depuis le source à chaque itération (mesure passe+build)
///   - sinon ⇒ construit une fois puis *deep clone* à chaque itération (mesure passe pure)
fn make_ir_supplier(src: String) -> impl Fn() -> ModuleOpaque + Clone {
    let rebuild_each = env_bool("IR_REBUILD_EACH", false);
    if rebuild_each {
        move || build_ir_from_src(&src).expect("build_ir_from_src (rebuild)") // panique si non branché
    } else {
        let base = build_ir_from_src(&src).expect("build_ir_from_src");
        move || deep_clone_ir(&base)
    }
}

fn maybe_verify(m: &ModuleOpaque) {
    if env_bool("IR_VALIDATE", false) {
        if let Err(e) = verify_ir(m) {
            eprintln!("[verify] {e:?}");
        }
    }
}

fn throughput_from_stats(stats: Option<IrStats>, fallback_bytes: usize) -> Throughput {
    if let Some(s) = stats {
        // On priorise le *nombre d’instructions* si dispo (plus pertinent qu’octets).
        if s.instrs > 0 {
            return Throughput::Elements(s.instrs as u64);
        } else if s.blocks > 0 {
            return Throughput::Elements(s.blocks as u64);
        } else if s.functions > 0 {
            return Throughput::Elements(s.functions as u64);
        }
    }
    Throughput::Bytes(fallback_bytes as u64)
}

// ======================================================================================
// Suites: micro, synthetic, external ; pour chaque passe + pipeline
// ======================================================================================

pub fn bench_ir_micro(c: &mut Criterion) {
    let mut group = c.benchmark_group("ir/micro");
    group.sample_size(env_usize("CRIT_SAMPLES", 50));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1200)));

    let cases = [
        ("hello", SRC_MICRO_HELLO),
        ("branches", SRC_MICRO_BRANCHES),
        ("funcs", SRC_MICRO_FUNCS),
        ("calls", SRC_MICRO_CALLS),
    ];

    let passes = env_list("IR_PASSES", "cfg,dfg,dom,ssa,constprop,dce,inline,pipeline");

    for (name, src) in cases {
        // Préparation
        let supplier = make_ir_supplier(src.to_string());
        let warm = (src.len().max(1)) as usize;

        for pass in &passes {
            group.throughput(throughput_from_stats(None, warm));
            let bench_id = BenchmarkId::new(pass, name);
            group.bench_function(bench_id, |b| {
                b.iter(|| {
                    let mut m = supplier();
                    let res = match pass.as_str() {
                        "cfg" => run_cfg(&mut m),
                        "dfg" => run_dfg(&mut m),
                        "dom" => run_dom(&mut m),
                        "ssa" => run_ssa(&mut m),
                        "constprop" => run_constprop(&mut m),
                        "dce" => run_dce(&mut m),
                        "inline" => run_inline(&mut m),
                        "pipeline" => run_pipeline(m).map(|_| ()),
                        _ => Ok(()),
                    };
                    if let Err(e) = res {
                        panic!("pass failed on {name}/{pass}: {e:?}");
                    }
                    maybe_verify(&m);
                    black_box(m);
                });
            });
        }
    }

    group.finish();
}

pub fn bench_ir_synthetic(c: &mut Criterion) {
    let mut group = c.benchmark_group("ir/synthetic");
    group.sample_size(env_usize("CRIT_SAMPLES", 45));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1300)));

    let sizes_kib = [16usize, 64, 256, 1024];
    let passes = env_list("IR_PASSES", "cfg,dfg,dom,ssa,constprop,dce,inline,pipeline");

    for &kib in &sizes_kib {
        let s_loops = synthetic_loops(kib);
        let s_calls = synthetic_calls(kib);
        let s_branches = synthetic_branches(kib);

        for (kind, src) in [("loops", s_loops), ("calls", s_calls), ("branches", s_branches)] {
            let supplier = make_ir_supplier(src.clone());
            for pass in &passes {
                group.throughput(Throughput::Bytes(src.len() as u64)); // fallback
                let id = BenchmarkId::new(format!("{kind}-{pass}"), kib);
                group.bench_function(id, |b| {
                    b.iter(|| {
                        let mut m = supplier();
                        let res = match pass.as_str() {
                            "cfg" => run_cfg(&mut m),
                            "dfg" => run_dfg(&mut m),
                            "dom" => run_dom(&mut m),
                            "ssa" => run_ssa(&mut m),
                            "constprop" => run_constprop(&mut m),
                            "dce" => run_dce(&mut m),
                            "inline" => run_inline(&mut m),
                            "pipeline" => run_pipeline(m).map(|_| ()),
                            _ => Ok(()),
                        };
                        if let Err(e) = res {
                            panic!("pass failed on synthetic/{kind}/{pass}: {e:?}");
                        }
                        maybe_verify(&m);
                        black_box(m);
                    });
                });
            }
        }
    }

    group.finish();
}

pub fn bench_ir_external(c: &mut Criterion) {
    let root = std::env::var("IR_DIR").unwrap_or_else(|_| "benchmarks/data/projects".into());
    let max_mb = env_usize("IR_MAX_MB", 4);
    let projects = list_projects(Path::new(&root), max_mb);

    if projects.is_empty() {
        eprintln!("[ir/external] aucun projet sous {root} (<= {max_mb} MiB).");
        return;
    }

    let mut group = c.benchmark_group("ir/external");
    group.sample_size(env_usize("CRIT_SAMPLES", 40));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1500)));

    let passes = env_list("IR_PASSES", "cfg,dfg,dom,ssa,constprop,dce,inline,pipeline");

    for proj in projects {
        let src = read_whole_project(&proj);
        if src.trim().is_empty() {
            eprintln!("[ir/external] skip {}: pas de .vit", proj.display());
            continue;
        }
        let supplier = make_ir_supplier(src);
        let id = sanitize_id(&proj);
        let bytes = project_src_bytes(&proj) as usize;
        for pass in &passes {
            group.throughput(Throughput::Bytes(bytes as u64));
            let bid = BenchmarkId::new(pass, &id);
            group.bench_function(bid, |b| {
                b.iter(|| {
                    let mut m = supplier();
                    let res = match pass.as_str() {
                        "cfg" => run_cfg(&mut m),
                        "dfg" => run_dfg(&mut m),
                        "dom" => run_dom(&mut m),
                        "ssa" => run_ssa(&mut m),
                        "constprop" => run_constprop(&mut m),
                        "dce" => run_dce(&mut m),
                        "inline" => run_inline(&mut m),
                        "pipeline" => run_pipeline(m).map(|_| ()),
                        _ => Ok(()),
                    };
                    if let Err(e) = res {
                        panic!("pass failed on external/{id}/{pass}: {e:?}");
                    }
                    maybe_verify(&m);
                    black_box(m);
                });
            });
        }
    }

    group.finish();
}

// ======================================================================================
// Entrée Criterion
// ======================================================================================

criterion_group!(benches, bench_ir_micro, bench_ir_synthetic, bench_ir_external);
criterion_main!(benches);
