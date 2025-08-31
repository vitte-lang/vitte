//! Benchmarks VM Vitte — micro workloads (Criterion) **ULTRA COMPLET**
//!
//! Backends :
//!   - `lib` (défaut) : appelle directement les fonctions Rust (pack + run).
//!   - `cli`          : lance les binaires (`vitte-run` ou `vitte run`).
//!
//! Variables d’environnement :
//!   VM_BACKEND=lib|cli             (def=lib)
//!   VM_REBUILD_EACH=0|1            (def=0)  — recompile le source à CHAQUE itération
//!   VM_VALIDATE=0|1                (def=1)  — vérifie stdout vs expected (si dispo)
//!   VM_ENTRY=main                  (def=main) — symbole d’entrée (si ton runtime le supporte)
//!   VM_BC_DIR=benchmarks/data/bytecode   — .vitbc précompilés (pour suite “bytecode_dir”)
//!   VM_PROJ_DIR=benchmarks/data/projects — projets {src/*.vit, expected.txt} (suite “projects”)
//!   VM_MAX_MB=4                    (def=4)   — taille max par projet ou .vitbc
//!
//! Criterion :
//!   CRIT_SAMPLES (def=50) | CRIT_WARMUP_MS (def=300) | CRIT_MEASURE_MS (def=1200)
//!
//! Suites :
//!   - vm/micro       → snippets embarqués (arith, récursion, strings, arrays…)
//!   - vm/synthetic   → générés (boucles/branches) pour [16, 64, 256, 1024 KiB]
//!   - vm/bytecode    → *.vitbc dans VM_BC_DIR (bypass compilation)
//!   - vm/projects    → pack+run projets sous VM_PROJ_DIR (expected.txt optionnel)
//!
//! ⚙️ ADAPT ME : branche `build_bc_from_src()`, `vm_run_bc()`, (optionnel) `vm_stats()`.

use anyhow::{anyhow, Context, Result};
use criterion::{black_box, criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use std::{
    ffi::OsStr,
    fs,
    path::{Path, PathBuf},
    process::Command,
    time::Duration,
};

// -------------------------------------------------------------------------------------
// Helpers env
// -------------------------------------------------------------------------------------
fn env_usize(k: &str, d: usize) -> usize {
    std::env::var(k).ok().and_then(|s| s.parse().ok()).unwrap_or(d)
}
fn env_u64(k: &str, d: u64) -> u64 {
    std::env::var(k).ok().and_then(|s| s.parse().ok()).unwrap_or(d)
}
fn env_bool(k: &str, d: bool) -> bool {
    std::env::var(k)
        .ok()
        .and_then(|s| s.parse::<u8>().ok())
        .map(|v| v != 0)
        .unwrap_or(d)
}
fn env_string(k: &str, d: &str) -> String {
    std::env::var(k).unwrap_or_else(|_| d.to_string())
}

// -------------------------------------------------------------------------------------
// Backend selection
// -------------------------------------------------------------------------------------
#[derive(Clone, Copy, Debug)]
enum Backend {
    Lib,
    Cli,
}
fn pick_backend() -> Backend {
    match std::env::var("VM_BACKEND").ok().as_deref() {
        Some("cli") => Backend::Cli,
        _ => Backend::Lib,
    }
}

// -------------------------------------------------------------------------------------
// ADAPTERS — **A BRANCHER** sur tes vraies APIs vitte_runtime / vitte_vm
// -------------------------------------------------------------------------------------

/// Compile/pack un *source* Vitte en **VITBC**. Hors itération par défaut.
fn build_bc_from_src(src: &str, _entry: &str) -> Result<Vec<u8>> {
    // 💡 ADAPT ME (1/3)
    // Exemple si tu exposes une API pack haut-niveau :
    //
    // use vitte_runtime;
    // let bc = vitte_runtime::pack_source(src, vitte_runtime::PackOptions {
    //     entry: Some(_entry.to_string()),
    //     ..Default::default()
    // })?;
    // Ok(bc)
    //
    // Fallback (pour te forcer à brancher ; sinon choisis backend=cli) :
    Err(anyhow!("build_bc_from_src(): non branché (ADAPT ME)"))
}

/// Exécute un **VITBC** dans la VM et renvoie la sortie standard (et éventuellement des stats).
fn vm_run_bc(bytecode: &[u8], _entry: &str) -> Result<VmOutput> {
    // 💡 ADAPT ME (2/3)
    // Exemple si tu exposes un runner in-memory :
    //
    // use vitte_vm;
    // let mut vm = vitte_vm::Vm::new();
    // vm.load_bytecode(bytecode)?;
    // let out = vm.run_entry(_entry)?;
    // Ok(VmOutput { stdout: out.stdout, instructions: out.instructions, cycles: out.cycles })
    //
    Err(anyhow!("vm_run_bc(): non branché (ADAPT ME)"))
}

/// Optionnel : métriques supplémentaires depuis la VM après run (retourne None si pas dispo).
fn vm_stats(_bytecode: &[u8]) -> Option<RunStats> {
    // 💡 ADAPT ME (3/3) — si tu sais compter instructions/cycles, remplis.
    None
}

// Modèle de sortie VM
#[derive(Debug, Clone)]
struct VmOutput {
    stdout: String,
    #[allow(dead_code)]
    instructions: Option<u64>,
    #[allow(dead_code)]
    cycles: Option<u64>,
}
impl VmOutput {
    fn new(stdout: String) -> Self {
        Self { stdout, instructions: None, cycles: None }
    }
}
#[derive(Debug, Clone, Copy)]
struct RunStats {
    instructions: u64,
    cycles: u64,
}

// -------------------------------------------------------------------------------------
// Backend CLI (fallback pratique si tu n’as pas encore les APIs lib)
// -------------------------------------------------------------------------------------
#[derive(Debug, Clone)]
struct CliCfg {
    cmd: String,          // "vitte-run" ou "vitte"
    use_subcommand: bool, // si true → `vitte run --input ...`, sinon `vitte-run --input ...`
}
fn cli_cfg_from_env() -> CliCfg {
    let cmd = env_string("VM_RUN", "vitte-run");
    let use_sub = cmd == "vitte";
    CliCfg { cmd, use_subcommand: use_sub }
}
fn cli_run_bc(bytecode: &[u8], _entry: &str, cfg: &CliCfg) -> Result<VmOutput> {
    // écrit le BC en temp puis lance le runner
    let tmp = temp_file(".vitbc")?;
    fs::write(tmp.path(), bytecode).context("écriture temp vitbc")?;
    let output = if cfg.use_subcommand {
        Command::new(&cfg.cmd).args(["run", "--input"]).arg(tmp.path()).output()
    } else {
        Command::new(&cfg.cmd).args(["--input"]).arg(tmp.path()).output()
    }
    .context("échec lancement vitte-run")?;
    if !output.status.success() {
        return Err(anyhow!("vitte-run échec: {}", output.status));
    }
    Ok(VmOutput::new(String::from_utf8_lossy(&output.stdout).to_string()))
}

// -------------------------------------------------------------------------------------
// Snippets embarqués (micro)
// -------------------------------------------------------------------------------------
const SRC_HELLO: &str = r#"
fn main() {
  let msg = "hello, vitte!";
  print(msg);
}
"#;

const SRC_LOOP_ADD: &str = r#"
fn main() {
  let mut s = 0;
  let n = 200_000;
  let mut i = 0;
  while i < n { s = s + i; i = i + 1; }
  print(s);
}
"#;

const SRC_RECURSION: &str = r#"
fn fib(n) { if n < 2 { return n; } return fib(n-1) + fib(n-2); }
fn main() { print(fib(24)); }
"#;

const SRC_CALLS: &str = r#"
fn add(a,b){a+b} fn mul(a,b){a*b} fn mix(x){ mul(add(x,1), add(x,2)) }
fn main(){ print(mix(1234)); }
"#;

const SRC_STRINGS: &str = r#"
fn main(){
  let a = "汉字🚀🔥"; let b = " — concat — ";
  let s = a + b + "ASCII_0123456789";
  print(s);
}
"#;

const SRC_ARRAYS: &str = r#"
fn main(){
  let arr = [1,2,3,4,5,6,7,8,9,10];
  let mut s = 0;
  let mut i = 0;
  while i < 1_000_00 {
    s = s + arr[i % 10];
    i = i + 1;
  }
  print(s);
}
"#;

// Synthèse : génère des boucles/branches “réalistes”
fn inflate_to_kib(seed: &str, kib: usize) -> String {
    let target = kib * 1024;
    let mut out = String::with_capacity(target + seed.len());
    while out.len() < target {
        out.push_str(seed);
        out.push('\n');
    }
    out
}
fn syn_loops(kib: usize) -> String {
    let seed = r#"
fn hot(n) {
  let mut s = 0;
  let mut i = 0;
  while i < n {
    if (i & 1) == 0 { s = s + i; } else { s = s - 1; }
    i = i + 1;
  }
  s
}
fn main(){ print(hot(200_000)); }"#;
    inflate_to_kib(seed, kib)
}
fn syn_branches(kib: usize) -> String {
    let seed = r#"
fn f(x){
  match x {
    0 => 1,
    1..=3 => 2,
    _ => if x%2==0 { x/2 } else { 3*x+1 }
  }
}
fn main(){ let mut i=0; let mut s=0; while i<150_000 { s=s+f(i); i=i+1;} print(s); }"#;
    inflate_to_kib(seed, kib)
}

// -------------------------------------------------------------------------------------
// Cache build: prépare du bytecode “supplier” (rebuild_each vs clone)
// -------------------------------------------------------------------------------------
fn make_bc_supplier_from_src(src: String, entry: String) -> impl Fn() -> Vec<u8> + Clone {
    let rebuild_each = env_bool("VM_REBUILD_EACH", false);
    if rebuild_each {
        move || build_bc_from_src(&src, &entry).expect("build_bc_from_src (rebuild)")
    } else {
        let base = build_bc_from_src(&src, &entry).expect("build_bc_from_src");
        move || base.clone()
    }
}

// -------------------------------------------------------------------------------------
// File utils (bytecode dir & projects dir)
// -------------------------------------------------------------------------------------
fn list_files_with_ext(root: &Path, ext: &str, max_mb: usize) -> Vec<PathBuf> {
    let mut out = Vec::new();
    let max = (max_mb as u64) * 1024 * 1024;
    if let Ok(rd) = fs::read_dir(root) {
        for e in rd.flatten() {
            let p = e.path();
            if p.is_dir() {
                out.extend(list_files_with_ext(&p, ext, max_mb));
            } else if p.extension().and_then(OsStr::to_str) == Some(ext) {
                if fs::metadata(&p).map(|m| m.len() <= max).unwrap_or(false) {
                    out.push(p);
                }
            }
        }
    }
    out.sort();
    out
}
fn sanitize_id(p: &Path) -> String {
    p.to_string_lossy()
        .replace('\\', "/")
        .replace("../", "")
        .replace("./", "")
        .replace(':', "_")
}
fn read_expected(dir: &Path) -> Option<String> {
    fs::read_to_string(dir.join("expected.txt")).ok().map(|s| s.trim().to_string())
}
fn concat_project_src(dir: &Path) -> String {
    let mut files = list_files_with_ext(dir, "vit", usize::MAX);
    files.sort();
    let mut s = String::new();
    for f in files {
        if let Ok(part) = fs::read_to_string(&f) {
            s.push_str(&part);
            s.push('\n');
        }
    }
    s
}

// -------------------------------------------------------------------------------------
// BENCH: micro (snippets embarqués → pack once → run)
// -------------------------------------------------------------------------------------
pub fn bench_vm_micro(c: &mut Criterion) {
    let mut group = c.benchmark_group("vm/micro");
    group.sample_size(env_usize("CRIT_SAMPLES", 50));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1200)));

    let entry = env_string("VM_ENTRY", "main");
    let validate = env_bool("VM_VALIDATE", true);
    let be = pick_backend();
    let clicfg = if matches!(be, Backend::Cli) { Some(cli_cfg_from_env()) } else { None };

    let cases = [
        ("hello", SRC_HELLO, Some("hello")),
        ("loop-add", SRC_LOOP_ADD, None),
        ("recursion", SRC_RECURSION, None),
        ("calls", SRC_CALLS, None),
        ("strings", SRC_STRINGS, None),
        ("arrays", SRC_ARRAYS, None),
    ];

    for (name, src, expect_sub) in cases {
        // Préparer un supplier de bytecode
        let supplier = make_bc_supplier_from_src(src.to_string(), entry.clone());
        // Throughput = taille du source (fallback). Si tu exposes vm_stats(), tu peux préférer instructions.
        group.throughput(Throughput::Bytes(src.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(name), &entry, |b, _| {
            b.iter(|| {
                let bc = supplier();
                // run selon backend
                let out = match be {
                    Backend::Lib => vm_run_bc(black_box(&bc), &entry),
                    Backend::Cli => cli_run_bc(black_box(&bc), &entry, clicfg.as_ref().unwrap()),
                }
                .expect("vm run failed");

                if validate {
                    if let Some(sub) = expect_sub {
                        assert!(
                            out.stdout.contains(sub),
                            "stdout ne contient pas {sub:?}\nReçu: {}",
                            out.stdout
                        );
                    }
                }
                // si stats dispo, expose-les à Criterion
                if let Some(stats) = vm_stats(&bc) {
                    criterion::black_box((stats.instructions, stats.cycles));
                }
            });
        });
    }

    group.finish();
}

// -------------------------------------------------------------------------------------
// BENCH: synthetic (tailles croissantes) — compile hors itération, run en boucle
// -------------------------------------------------------------------------------------
pub fn bench_vm_synthetic(c: &mut Criterion) {
    let mut group = c.benchmark_group("vm/synthetic");
    group.sample_size(env_usize("CRIT_SAMPLES", 45));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1300)));

    let entry = env_string("VM_ENTRY", "main");
    let be = pick_backend();
    let clicfg = if matches!(be, Backend::Cli) { Some(cli_cfg_from_env()) } else { None };

    for kib in [16usize, 64, 256, 1024] {
        for (kind, src) in [("loops", syn_loops(kib)), ("branches", syn_branches(kib))] {
            let supplier = make_bc_supplier_from_src(src.clone(), entry.clone());
            group.throughput(Throughput::Bytes(src.len() as u64));
            group.bench_with_input(BenchmarkId::new(kind, kib), &entry, |b, _| {
                b.iter(|| {
                    let bc = supplier();
                    let out = match be {
                        Backend::Lib => vm_run_bc(black_box(&bc), &entry),
                        Backend::Cli => {
                            cli_run_bc(black_box(&bc), &entry, clicfg.as_ref().unwrap())
                        },
                    }
                    .expect("vm run failed");
                    criterion::black_box(out.stdout.len());
                    if let Some(stats) = vm_stats(&bc) {
                        criterion::black_box((stats.instructions, stats.cycles));
                    }
                });
            });
        }
    }

    group.finish();
}

// -------------------------------------------------------------------------------------
// BENCH: bytecode_dir — exécute directement des *.vitbc (précompilés)
// -------------------------------------------------------------------------------------
pub fn bench_vm_bytecode_dir(c: &mut Criterion) {
    let root = Path::new(&env_string("VM_BC_DIR", "benchmarks/data/bytecode"));
    let max_mb = env_usize("VM_MAX_MB", 4);
    let files = list_files_with_ext(root, "vitbc", max_mb);
    if files.is_empty() {
        eprintln!("[vm/bytecode] aucun .vitbc sous {}", root.display());
        return;
    }

    let mut group = c.benchmark_group("vm/bytecode");
    group.sample_size(env_usize("CRIT_SAMPLES", 40));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1200)));

    let entry = env_string("VM_ENTRY", "main");
    let be = pick_backend();
    let clicfg = if matches!(be, Backend::Cli) { Some(cli_cfg_from_env()) } else { None };

    for f in files {
        let id = sanitize_id(&f);
        let Ok(bc) = fs::read(&f) else {
            continue;
        };
        group.throughput(Throughput::Bytes(bc.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(id), &bc, |b, bc_in| {
            b.iter(|| {
                let out = match be {
                    Backend::Lib => vm_run_bc(black_box(bc_in), &entry),
                    Backend::Cli => cli_run_bc(black_box(bc_in), &entry, clicfg.as_ref().unwrap()),
                }
                .expect("vm run failed");
                criterion::black_box(out.stdout.len());
                if let Some(stats) = vm_stats(bc_in) {
                    criterion::black_box((stats.instructions, stats.cycles));
                }
            });
        });
    }

    group.finish();
}

// -------------------------------------------------------------------------------------
// BENCH: projects — pack+run par projet (src/**/*.vit, expected.txt optionnel)
// -------------------------------------------------------------------------------------
pub fn bench_vm_projects(c: &mut Criterion) {
    let root = Path::new(&env_string("VM_PROJ_DIR", "benchmarks/data/projects"));
    let max_mb = env_usize("VM_MAX_MB", 4);
    let mut projs: Vec<PathBuf> = Vec::new();
    if let Ok(rd) = fs::read_dir(root) {
        for e in rd.flatten() {
            let p = e.path();
            if p.is_dir() {
                let size = list_files_with_ext(&p, "vit", max_mb)
                    .into_iter()
                    .map(|f| fs::metadata(f).map(|m| m.len()).unwrap_or(0))
                    .sum::<u64>();
                if size <= (max_mb as u64) * 1024 * 1024 {
                    projs.push(p);
                }
            }
        }
    }
    if projs.is_empty() {
        eprintln!("[vm/projects] aucun projet sous {}", root.display());
        return;
    }

    let mut group = c.benchmark_group("vm/projects");
    group.sample_size(env_usize("CRIT_SAMPLES", 35));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 400)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1500)));

    let entry = env_string("VM_ENTRY", "main");
    let validate = env_bool("VM_VALIDATE", true);
    let be = pick_backend();
    let clicfg = if matches!(be, Backend::Cli) { Some(cli_cfg_from_env()) } else { None };

    for proj in projs {
        let id = sanitize_id(&proj);
        let src = concat_project_src(&proj);
        if src.trim().is_empty() {
            eprintln!("[vm/projects] skip {}: pas de .vit", proj.display());
            continue;
        }
        let expect = read_expected(&proj);
        let supplier = make_bc_supplier_from_src(src.clone(), entry.clone());

        group.throughput(Throughput::Bytes(src.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(id), &entry, |b, _| {
            b.iter(|| {
                let bc = supplier();
                let out = match be {
                    Backend::Lib => vm_run_bc(black_box(&bc), &entry),
                    Backend::Cli => cli_run_bc(black_box(&bc), &entry, clicfg.as_ref().unwrap()),
                }
                .expect("vm run failed");
                if validate {
                    if let Some(exp) = &expect {
                        assert!(
                            out.stdout.contains(exp),
                            "stdout ne contient pas {:?}\nReçu: {}",
                            exp,
                            out.stdout
                        );
                    }
                }
                if let Some(stats) = vm_stats(&bc) {
                    criterion::black_box((stats.instructions, stats.cycles));
                }
            });
        });
    }

    group.finish();
}

// -------------------------------------------------------------------------------------
// Entrée Criterion
// -------------------------------------------------------------------------------------
criterion_group!(
    benches,
    bench_vm_micro,
    bench_vm_synthetic,
    bench_vm_bytecode_dir,
    bench_vm_projects
);
criterion_main!(benches);

// -------------------------------------------------------------------------------------
// Temp file helper
// -------------------------------------------------------------------------------------
use std::{fs::File, io, io::Write as _};

struct TempPath {
    path: PathBuf,
}
impl TempPath {
    fn new(ext: &str) -> io::Result<Self> {
        let mut p = std::env::temp_dir();
        let name = format!("vitte-vm-{}-{:x}{}", std::process::id(), randomish(), ext);
        p.push(name);
        Ok(Self { path: p })
    }
}
impl TempPath {
    fn path(&self) -> &Path {
        &self.path
    }
}
impl Drop for TempPath {
    fn drop(&mut self) {
        let _ = fs::remove_file(&self.path);
    }
}
fn temp_file(ext: &str) -> io::Result<TempPath> {
    let t = TempPath::new(ext)?;
    File::create(t.path())?.flush()?;
    Ok(t)
}
fn randomish() -> u64 {
    use std::time::{SystemTime, UNIX_EPOCH};
    let t = SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or_default().as_nanos() as u64;
    let mut x = t ^ 0x9E3779B97F4A7C15;
    x ^= x << 7;
    x ^= x >> 9;
    x
}
