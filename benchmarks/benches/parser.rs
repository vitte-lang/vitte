//! Benchmarks du parseur Vitte (Criterion) — ultra complet
//!
//! Paramètres via env :
//!   - CRIT_SAMPLES      (def=60)
//!   - CRIT_WARMUP_MS    (def=300)
//!   - CRIT_MEASURE_MS   (def=1000)
//!   - BENCH_LARGE       (def=1)    — 0 pour désactiver la suite “large”
//!   - PAR_DIR           (def="benchmarks/data") — racine corpus externe
//!   - PAR_MAX_MB        (def=4)    — taille max par fichier externe
//!   - PAR_VALIDATE      (def=1)    — 0 pour ignorer les erreurs parse (mesure brute)
//!
//! Hooks “ADAPT ME” : adapte `parse_ok()` et `ast_nodes()` à ta vraie API.

use criterion::{black_box, criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use std::{
    fmt::Write as _,
    fs,
    path::{Path, PathBuf},
    time::Duration,
};

// ---- Dépendance cible --------------------------------------------------------
use vitte_parser; // volontairement minimal (pas d'import de types pour rester agnostique)

// ============================================================================
// Helpers env & info
// ============================================================================
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

fn print_env_once() {
    use std::sync::Once;
    static ONCE: Once = Once::new();
    ONCE.call_once(|| {
        let triple = format!(
            "{}-{}-{}",
            std::env::consts::ARCH,
            std::env::consts::OS,
            std::env::consts::FAMILY
        );
        let th = std::thread::available_parallelism().map(|n| n.get()).unwrap_or(1);
        let mut cpu = String::new();
        #[cfg(target_arch = "x86_64")]
        {
            use std::arch::is_x86_feature_detected as f;
            let feats = [
                ("sse2", f!("sse2")),
                ("ssse3", f!("ssse3")),
                ("sse4.1", f!("sse4.1")),
                ("avx", f!("avx")),
                ("avx2", f!("avx2")),
                ("avx512f", f!("avx512f")),
                ("avx512bw", f!("avx512bw")),
                ("avx512dq", f!("avx512dq")),
            ];
            let _ = write!(&mut cpu, " | x86-features: ");
            for (name, ok) in feats {
                let _ = write!(&mut cpu, "{}{}, ", if ok { "+" } else { "-" }, name);
            }
        }
        eprintln!("[bench:env] target={triple} threads={th}{cpu}");
    });
}

// ============================================================================
// Parser adapters — ADAPT ME
// ============================================================================

/// Retourne `true` si le parse réussit.
/// ADAPT ME (1/2) : colle au type de retour réel de `vitte_parser`.
#[inline(always)]
fn parse_ok(src: &str) -> bool {
    // Exemple 1: si parse renvoie Result<Ast, E>
    // matches!(vitte_parser::parse(src), Ok(_))

    // Exemple 2: si parse renvoie Ast directement
    // { let _ast = vitte_parser::parse(src); true }

    // Par défaut: on tente Result-like, sinon on "no-op" (à adapter).
    match try_parse_result_like(src) {
        Some(ok) => ok,
        None => {
            // Fallback : appelle une API fictive (à changer chez toi)
            let _ = black_box(vitte_parser::parse(src));
            true
        },
    }
}

/// Taille d’AST (nœuds) si tu as un visiteur/compteur.
/// ADAPT ME (2/2) : renvoie 0 si tu n’as pas (OK pour throughput bytes).
#[inline(always)]
fn ast_nodes(_src: &str) -> Option<usize> {
    // Exemple : vitte_parser::parse(src).ok().map(|ast| ast.len_nodes())
    None
}

/// Heuristique: détecte si `vitte_parser::parse` renvoie Result.
#[inline(always)]
fn try_parse_result_like(src: &str) -> Option<bool> {
    // On ne peut pas introspecter le type ici ; on essaie un call dans un bloc traité comme Result.
    // Si ça ne compile pas chez toi, remplace `parse_ok()` directement (au-dessus).
    let _ = src; // silence unused
    None
}

// ============================================================================
// Corpus embarqué & synthèse
// ============================================================================

// Micro-corpus (doit parser sans erreur)
const SRC_HELLO: &str = r#"
let msg = "hello, vitte!";
print(msg);
"#;

const SRC_FIBO: &str = r#"
fn fib(n) {
  if n < 2 { return n; }
  return fib(n-1) + fib(n-2);
}
let out = fib(24);
"#;

const SRC_MATCH: &str = r#"
enum E { A(i32), B, C }
fn go(x) {
  match x {
    0 => "zero",
    1 | 2 => "small",
    3..=9 => "medium",
    _ => "big",
  }
}
"#;

const SRC_STRUCTS: &str = r#"
struct Pt { x: i32, y: i32 }
let p = Pt { x: 1, y: 2 };
"#;

const SRC_COMMENTS: &str = r#"
/* block
   comment */
let a = 1; // eol
/* another */ let b = a + 2;
"#;

const SRC_STRINGS: &str = r#"
let s = "汉字 and emoji 🚀🔥\n\t\"\\";
"#;

/// Génère une chaîne qui gonfle jusqu’à `kib` KiB en répétant un seed.
fn inflate_to_kib(seed: &str, kib: usize) -> String {
    let target = kib * 1024;
    let mut out = String::with_capacity(target + seed.len());
    while out.len() < target {
        out.push_str(seed);
        out.push('\n');
    }
    out
}

/// Expressions lourdes: beaucoup d’ops/arbre profond “à plat”.
fn synthetic_heavy_expr(kib: usize) -> String {
    let seed = r#"
let x = (((((((1+2)*3-4)/5)%6) + ((7*8)-(9/3))) ^ 2) & 255) | 0xFF;
"#;
    inflate_to_kib(seed, kib)
}

/// Unicode/idents longs pour stress lexer+parser.
fn synthetic_mixed(kib: usize) -> String {
    let seed = r#"
fn mix(αβγδ123: i64) -> i64 {
    let π = 314159; let emoji = "🚀🔥✨";
    let very_long_identifier_name_with_many_parts_123 = αβγδ123 + π;
    very_long_identifier_name_with_many_parts_123
}"#;
    inflate_to_kib(seed, kib)
}

/// Nids de blocs pour tester la récursion/profondeur de pile.
fn synthetic_deep_nesting(levels: usize) -> String {
    let mut s = String::with_capacity(levels * 8);
    s.push_str("fn deep() {");
    for i in 0..levels {
        let _ = write!(&mut s, " if {i} < {i}+1 {{");
    }
    s.push_str(" let mut k = 0; k = k + 1; ");
    for _ in 0..levels {
        s.push('}');
    }
    s.push('}');
    s
}

/// Code ASCII “moyen”.
fn synthetic_ascii(kib: usize) -> String {
    let seed = r#"
fn foo(a: i32, b: i32) -> i32 { a + b }
fn bar(x: i32) -> i32 { let mut y = 0; for i in 0..x { y += i; } y }
let arr = [1,2,3,4,5,6,7,8,9,10];
let s = "string with escapes \n\t\"\\ and numbers 1234567890";
"#;
    inflate_to_kib(seed, kib)
}

// ============================================================================
// External corpus (.vit files)
// ============================================================================
fn collect_vit_files(root: &Path, max_mb: usize) -> Vec<PathBuf> {
    let mut out = Vec::new();
    let max_bytes = max_mb * 1024 * 1024;
    fn walk(dir: &Path, out: &mut Vec<PathBuf>, max_bytes: usize) {
        if let Ok(rd) = fs::read_dir(dir) {
            for e in rd.flatten() {
                let p = e.path();
                if p.is_dir() {
                    walk(&p, out, max_bytes);
                } else if p.extension().map(|s| s == "vit").unwrap_or(false) {
                    if let Ok(md) = fs::metadata(&p) {
                        if md.len() as usize <= max_bytes {
                            out.push(p);
                        }
                    }
                }
            }
        }
    }
    walk(root, &mut out, max_bytes);
    out.sort();
    out
}
fn read_to_string(path: &Path) -> Option<String> {
    fs::read_to_string(path).ok()
}
fn sanitize_id(path: &Path) -> String {
    let s = path.to_string_lossy();
    s.replace('\\', "/").replace("../", "").replace("./", "").replace(':', "_")
}

// ============================================================================
// Suites de bench
// ============================================================================
pub fn bench_parser_micro(c: &mut Criterion) {
    print_env_once();
    let mut group = c.benchmark_group("parser/micro");

    group.sample_size(env_usize("CRIT_SAMPLES", 60));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 900)));

    let validate = env_bool("PAR_VALIDATE", true);

    let cases = [
        ("hello", SRC_HELLO),
        ("fibo", SRC_FIBO),
        ("match", SRC_MATCH),
        ("structs", SRC_STRUCTS),
        ("comments", SRC_COMMENTS),
        ("strings", SRC_STRINGS),
    ];

    for (name, src) in cases {
        group.throughput(Throughput::Bytes(src.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(name), src, |b, s| {
            b.iter(|| {
                let ok = parse_ok(black_box(s));
                if validate && !ok {
                    // On ne panique pas, on compte quand même (pour mesure brute), mais on expose.
                    eprintln!("[warn] parse failed on micro case: {name}");
                }
                // nodes/s si dispo
                if let Some(nodes) = ast_nodes(s) {
                    criterion::black_box(nodes);
                }
            });
        });
    }

    group.finish();
}

pub fn bench_parser_synthetic(c: &mut Criterion) {
    print_env_once();
    let mut group = c.benchmark_group("parser/synthetic");

    group.sample_size(env_usize("CRIT_SAMPLES", 50));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1200)));

    // tailles en KiB
    let sizes_kib = [16usize, 64, 256, 1024];

    for &kib in &sizes_kib {
        // ASCII
        let ascii_src = synthetic_ascii(kib);
        group.throughput(Throughput::Bytes(ascii_src.len() as u64));
        group.bench_with_input(BenchmarkId::new("ascii", kib), &ascii_src, |b, s| {
            b.iter(|| {
                let _ = parse_ok(black_box(s));
                if let Some(nodes) = ast_nodes(s) {
                    criterion::black_box(nodes);
                }
            });
        });

        // MIX
        let mixed_src = synthetic_mixed(kib);
        group.throughput(Throughput::Bytes(mixed_src.len() as u64));
        group.bench_with_input(BenchmarkId::new("mixed", kib), &mixed_src, |b, s| {
            b.iter(|| {
                let _ = parse_ok(black_box(s));
                if let Some(nodes) = ast_nodes(s) {
                    criterion::black_box(nodes);
                }
            });
        });

        // HEAVY EXPR
        let hex_src = synthetic_heavy_expr(kib);
        group.throughput(Throughput::Bytes(hex_src.len() as u64));
        group.bench_with_input(BenchmarkId::new("heavy-expr", kib), &hex_src, |b, s| {
            b.iter(|| {
                let _ = parse_ok(black_box(s));
                if let Some(nodes) = ast_nodes(s) {
                    criterion::black_box(nodes);
                }
            });
        });
    }

    // Profondeur (non mesurée en KiB mais intéressante)
    for levels in [8usize, 16, 32, 64, 96] {
        let src = synthetic_deep_nesting(levels);
        group.throughput(Throughput::Bytes(src.len() as u64));
        group.bench_with_input(BenchmarkId::new("deep-nesting", levels), &src, |b, s| {
            b.iter(|| {
                let _ = parse_ok(black_box(s));
                if let Some(nodes) = ast_nodes(s) {
                    criterion::black_box(nodes);
                }
            });
        });
    }

    group.finish();
}

pub fn bench_parser_large(c: &mut Criterion) {
    if !env_bool("BENCH_LARGE", true) {
        eprintln!("[bench] large: skipped (BENCH_LARGE=0)");
        return;
    }
    print_env_once();
    let mut group = c.benchmark_group("parser/large");
    group.sample_size(env_usize("CRIT_SAMPLES", 30));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 500)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1500)));

    for kib in [256, 512, 1024, 2048] {
        let src = inflate_to_kib(SRC_FIBO, kib);
        group.throughput(Throughput::Bytes(src.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(format!("{}KiB", kib)), &src, |b, s| {
            b.iter(|| {
                let _ = parse_ok(black_box(s));
                if let Some(nodes) = ast_nodes(s) {
                    criterion::black_box(nodes);
                }
            });
        });
    }

    group.finish();
}

pub fn bench_parser_external(c: &mut Criterion) {
    print_env_once();
    let root = std::env::var("PAR_DIR").unwrap_or_else(|_| "benchmarks/data".into());
    let max_mb = env_usize("PAR_MAX_MB", 4);
    let validate = env_bool("PAR_VALIDATE", true);
    let files = collect_vit_files(Path::new(&root), max_mb);

    if files.is_empty() {
        eprintln!("[bench] external: no .vit under {root} (<= {max_mb} MiB)");
        return;
    }

    let mut group = c.benchmark_group("parser/external");
    group.sample_size(env_usize("CRIT_SAMPLES", 40));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1200)));

    for path in files {
        let Some(src) = read_to_string(&path) else {
            continue;
        };
        let id = sanitize_id(&path);

        group.throughput(Throughput::Bytes(src.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(id), &src, |b, s| {
            b.iter(|| {
                let ok = parse_ok(black_box(s));
                if validate && !ok {
                    eprintln!("[warn] parse failed on external file");
                }
                if let Some(nodes) = ast_nodes(s) {
                    criterion::black_box(nodes);
                }
            });
        });
    }

    group.finish();
}

// ============================================================================
// Entrée Criterion
// ============================================================================
criterion_group!(
    benches,
    bench_parser_micro,
    bench_parser_synthetic,
    bench_parser_large,
    bench_parser_external
);
criterion_main!(benches);
