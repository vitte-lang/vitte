//! Benchmarks E2E Vitte: **pack → run** par projet (Criterion)
//!
//! Backends supportés:
//!   - `lib` (par défaut) : appelle directement des fonctions Rust (pack & run).
//!   - `cli`              : invoque les binaires (`vitte`, `vitte-pack`, `vitte-run`).
//!
//! Variables d’environnement utiles :
//!   E2E_BACKEND=lib|cli          (def=lib)
//!   E2E_DIR=benchmarks/data/projects (racine des projets .vit)
//!   E2E_MAX_MB=5                 (taille max par projet, pour éviter les mammouths)
//!   E2E_VALIDATE=1|0             (valider la sortie via expected.txt si présent, def=1)
//!   E2E_PACK=vitte-pack          (si backend=cli ; ou `vitte` avec sous-commande `pack`)
//!   E2E_RUN=vitte-run            (si backend=cli ; ou `vitte` avec sous-commande `run`)
//!
//! Criterion :
//!   CRIT_SAMPLES (def=30) | CRIT_WARMUP_MS (def=400) | CRIT_MEASURE_MS (def=1500)
//!
//! Dossier projet attendu :
//!   projects/<name>/
//!     ├─ src/**/*.vit    (ou *.vit à plat)
//!     └─ expected.txt    (optionnel ; substring à trouver dans stdout du run)
//!
//! ⚠️ ADAPTATION LIB (3 lignes marquées “ADAPT ME” CI-DESSOUS) :
//!    - `pack_project(dir) -> anyhow::Result<Vec<u8>>`
//!    - `run_bytecode(&[u8]) -> anyhow::Result<String>` (stdout)

use criterion::{black_box, criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use std::{
    ffi::OsStr,
    fs,
    io::{self, Read, Write},
    path::{Path, PathBuf},
    process::Command,
    time::Duration,
};

use anyhow::{anyhow, Context, Result};

// --- Choix de backend ---------------------------------------------------------

#[derive(Debug, Clone, Copy)]
enum Backend {
    Lib,
    Cli,
}

fn pick_backend() -> Backend {
    match std::env::var("E2E_BACKEND").ok().as_deref() {
        Some("cli") => Backend::Cli,
        _ => Backend::Lib,
    }
}

// --- Adapters LIB (ADAPT ME) --------------------------------------------------
// Si tes APIs sont différentes, adapte seulement ces 2 fonctions.

#[allow(unused)]
fn lib_pack_project(dir: &Path) -> Result<Vec<u8>> {
    // ADAPT ME (1/3): remplace par ton API réelle, p.ex. vitte_runtime::pack(dir)
    // use vitte_runtime; // assure-toi d’avoir la dep dans benchmarks/Cargo.toml
    // let bc = vitte_runtime::pack(dir)?;
    // Ok(bc)
    Err(anyhow!("lib_pack_project non branché (ADAPT ME)"))
}

#[allow(unused)]
fn lib_run_bytecode(bytecode: &[u8]) -> Result<String> {
    // ADAPT ME (2/3): remplace par ton API réelle, p.ex. vitte_runtime::run(&bc)
    // let out = vitte_runtime::run(bytecode)?;
    // Ok(out.stdout_string())
    Err(anyhow!("lib_run_bytecode non branché (ADAPT ME)"))
}

// Option bonus : pack + run “high-level” si tu exposes déjà une façade unique.
#[allow(unused)]
fn lib_pack_and_run(dir: &Path) -> Result<String> {
    // ADAPT ME (3/3) si tu as une façade directe ; sinon, garde la version générique:
    let bc = lib_pack_project(dir)?;
    lib_run_bytecode(&bc)
}

// --- Adapters CLI -------------------------------------------------------------
// On accepte soit des binaires dédiés (vitte-pack / vitte-run), soit `vitte pack` / `vitte run`.

#[derive(Debug, Clone)]
struct CliCfg {
    pack_cmd: String, // ex: "vitte-pack" ou "vitte"
    run_cmd: String,  // ex: "vitte-run"  ou "vitte"
    use_subcommands: bool, // true si on doit appeler `vitte pack` au lieu de `vitte-pack`
}

fn cli_cfg_from_env() -> CliCfg {
    let pack_cmd = std::env::var("E2E_PACK").unwrap_or_else(|_| "vitte-pack".to_string());
    let run_cmd  = std::env::var("E2E_RUN").unwrap_or_else(|_| "vitte-run".to_string());
    // Heuristique: si le nom est exactement "vitte", on passe par des sous-commandes.
    let use_sub = pack_cmd == "vitte" || run_cmd == "vitte";
    CliCfg { pack_cmd, run_cmd, use_subcommands: use_sub }
}

fn cli_pack_project(dir: &Path, cfg: &CliCfg) -> Result<Vec<u8>> {
    let tmp = temp_file(".vitbc")?;
    let out_path = tmp.path().to_owned();

    let status = if cfg.use_subcommands {
        Command::new(&cfg.pack_cmd)
            .args(["pack", "--input"])
            .arg(dir)
            .args(["--output"])
            .arg(&out_path)
            .status()
    } else {
        Command::new(&cfg.pack_cmd)
            .args(["--input"])
            .arg(dir)
            .args(["--output"])
            .arg(&out_path)
            .status()
    }.with_context(|| "échec lancement pack CLI")?;

    if !status.success() {
        return Err(anyhow!("pack CLI a échoué (exit={status})"));
    }
    let bc = fs::read(&out_path).with_context(|| "lecture du bytecode packé")?;
    Ok(bc)
}

fn cli_run_bytecode(bytecode: &[u8], cfg: &CliCfg) -> Result<String> {
    let tmp = temp_file(".vitbc")?;
    fs::write(tmp.path(), bytecode).with_context(|| "écriture temp vitbc")?;

    let output = if cfg.use_subcommands {
        Command::new(&cfg.run_cmd)
            .args(["run", "--input"])
            .arg(tmp.path())
            .output()
    } else {
        Command::new(&cfg.run_cmd)
            .args(["--input"])
            .arg(tmp.path())
            .output()
    }.with_context(|| "échec lancement run CLI")?;

    if !output.status.success() {
        return Err(anyhow!("run CLI a échoué (exit={})", output.status));
    }
    let stdout = String::from_utf8_lossy(&output.stdout).to_string();
    Ok(stdout)
}

// --- Utils fichiers -----------------------------------------------------------

fn read_expected(dir: &Path) -> Option<String> {
    let exp = dir.join("expected.txt");
    fs::read_to_string(exp).ok().map(|s| s.trim().to_string())
}

fn project_src_bytes(dir: &Path) -> io::Result<u64> {
    let mut total = 0u64;
    walk(dir, &mut |p| {
        if p.extension().and_then(OsStr::to_str) == Some("vit") {
            if let Ok(md) = fs::metadata(p) {
                total = total.saturating_add(md.len());
            }
        }
    })?;
    Ok(total)
}

fn list_projects(root: &Path, max_mb: usize) -> io::Result<Vec<PathBuf>> {
    let mut out = Vec::new();
    let max_bytes = (max_mb as u64) * 1024 * 1024;
    for entry in fs::read_dir(root)? {
        let entry = entry?;
        let p = entry.path();
        if p.is_dir() {
            // somme approximative des .vit du projet
            let bytes = project_src_bytes(&p).unwrap_or(0);
            if bytes <= max_bytes {
                out.push(p);
            }
        }
    }
    out.sort();
    Ok(out)
}

fn walk<F: FnMut(&Path)>(dir: &Path, f: &mut F) -> io::Result<()> {
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

struct TempPath {
    path: PathBuf,
}
impl TempPath {
    fn new(ext: &str) -> io::Result<Self> {
        let mut p = std::env::temp_dir();
        let fname = format!("vitte-bench-{}-{:x}{}", std::process::id(), rand_u64(), ext);
        p.push(fname);
        Ok(Self { path: p })
    }
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
    TempPath::new(ext)
}
fn rand_u64() -> u64 {
    // petit PRNG basique (pas cryptographique)
    use std::time::{SystemTime, UNIX_EPOCH};
    let t = SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or_default().as_nanos();
    // xorshift-ish
    let mut x = (t as u64) ^ 0x9E3779B97F4A7C15;
    x ^= x << 7;
    x ^= x >> 9;
    x
}

// --- Suites de bench ----------------------------------------------------------

pub fn bench_e2e(c: &mut Criterion) {
    let root = std::env::var("E2E_DIR").unwrap_or_else(|_| "benchmarks/data/projects".into());
    let max_mb = std::env::var("E2E_MAX_MB").ok().and_then(|s| s.parse().ok()).unwrap_or(5usize);
    let validate = std::env::var("E2E_VALIDATE").ok().map(|s| s != "0").unwrap_or(true);

    let projects = match list_projects(Path::new(&root), max_mb) {
        Ok(v) if !v.is_empty() => v,
        Ok(_) => {
            eprintln!("[e2e] aucun projet .vit dans {root}/ (<= {max_mb} MiB). Ajoute p.ex. {root}/hello");
            return;
        }
        Err(e) => {
            eprintln!("[e2e] impossible de lister {root}: {e}");
            return;
        }
    };

    let mut group = c.benchmark_group("e2e/pack+run");
    group.sample_size(env_usize("CRIT_SAMPLES", 30));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 400)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1500)));

    let backend = pick_backend();
    let clicfg = if matches!(backend, Backend::Cli) { Some(cli_cfg_from_env()) } else { None };

    for proj in projects {
        let id = sanitize_id(&proj);
        let size = project_src_bytes(&proj).unwrap_or(0);
        if size == 0 {
            eprintln!("[e2e] skip {id}: aucun .vit trouvé");
            continue;
        }
        group.throughput(Throughput::Bytes(size));

        // Pré-calc: expected.txt (si présent on check stdout contient le substring)
        let expected = read_expected(&proj);

        // Sous-bench: PACK SEUL
        group.bench_with_input(BenchmarkId::new("pack", &id), &proj, |b, dir| {
            b.iter(|| {
                let bc = match backend {
                    Backend::Lib => lib_pack_project(black_box(dir)),
                    Backend::Cli => cli_pack_project(black_box(dir), clicfg.as_ref().unwrap()),
                };
                if let Err(e) = bc {
                    panic!("pack failed on {}: {e:?}", dir.display());
                }
                // on ne garde pas le bytecode au-delà de l’itération
            });
        });

        // Sous-bench: RUN SEUL (nécessite pack préalable par itération)
        group.bench_with_input(BenchmarkId::new("run", &id), &proj, |b, dir| {
            b.iter(|| {
                // pack
                let bc = match backend {
                    Backend::Lib => lib_pack_project(black_box(dir)),
                    Backend::Cli => cli_pack_project(black_box(dir), clicfg.as_ref().unwrap()),
                }.expect("pack failed");

                // run
                let out = match backend {
                    Backend::Lib => lib_run_bytecode(black_box(&bc)),
                    Backend::Cli => cli_run_bytecode(black_box(&bc), clicfg.as_ref().unwrap()),
                };
                let stdout = out.expect("run failed");
                if validate {
                    if let Some(exp) = &expected {
                        assert!(
                            stdout.contains(exp),
                            "stdout ne contient pas l’extrait attendu.\nAttendu: {:?}\nReçu: {}",
                            exp, stdout
                        );
                    }
                }
            });
        });

        // Sous-bench: PACK+RUN combiné (mesure pipeline complet)
        group.bench_with_input(BenchmarkId::new("pack+run", &id), &proj, |b, dir| {
            b.iter(|| {
                // selon backend, on peut mutualiser
                let stdout = match backend {
                    Backend::Lib => lib_pack_and_run(black_box(dir)),
                    Backend::Cli => {
                        let bc = cli_pack_project(black_box(dir), clicfg.as_ref().unwrap())?;
                        cli_run_bytecode(black_box(&bc), clicfg.as_ref().unwrap())
                    }
                }.expect("pack+run failed");

                if validate {
                    if let Some(exp) = &expected {
                        assert!(
                            stdout.contains(exp),
                            "stdout ne contient pas l’extrait attendu.\nAttendu: {:?}\nReçu: {}",
                            exp, stdout
                        );
                    }
                }
            });
        });
    }

    group.finish();
}

// --- Entrée Criterion ---------------------------------------------------------

criterion_group!(benches, bench_e2e);
criterion_main!(benches);

// --- Helpers env pour Criterion -----------------------------------------------

fn env_usize(key: &str, default: usize) -> usize {
    std::env::var(key).ok().and_then(|s| s.parse::<usize>().ok()).unwrap_or(default)
}
fn env_u64(key: &str, default: u64) -> u64 {
    std::env::var(key).ok().and_then(|s| s.parse::<u64>().ok()).unwrap_or(default)
}
