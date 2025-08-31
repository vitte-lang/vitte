//! Benchmarks du lexer Vitte (Criterion) — version “plus que complète”
//!
//! ▶ Paramètres via variables d’environnement :
//!   - CRIT_SAMPLES      (def=60)   — taille d’échantillon Criterion
//!   - CRIT_WARMUP_MS    (def=300)  — warmup en ms
//!   - CRIT_MEASURE_MS   (def=1000) — fenêtre de mesure en ms
//!   - BENCH_LARGE       (def=1)    — 0 pour désactiver la suite “large”
//!   - LEX_DIR           (def="benchmarks/data") — racine pour corpus externe
//!   - LEX_MAX_MB        (def=4)    — taille max par fichier externe
//!   - LEX_ALLOC_PROBE   (def=0)    — 1 pour faire UNE mesure d’alloc avant bench
//!
//! Suites incluses :
//!   1) micro        — petits fichiers embarqués (variété de tokens)
//!   2) synthetic    — générateurs (ASCII, mix Unicode, comments) avec tailles [16, 64, 256, 1024 KiB]
//!   3) large        — blocs répétés pour 256 KiB → 2 MiB (désactivable)
//!   4) external     — tous les .vit sous LEX_DIR (scan récursif)
//!
//! Si ton API n’est pas `vitte_lexer::tokenize(&str) -> impl IntoIterator<Token>`,
//! adapte `lex_count()` (lignes marquées “ADAPT ME”).

use criterion::{black_box, criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use std::{
    fmt::Write as _,
    fs,
    path::{Path, PathBuf},
    sync::Once,
    time::Duration,
};

// ---- Dépendance cible --------------------------------------------------------
use vitte_lexer; // volontairement minimal

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

static PRINT_ENV_ONCE: Once = Once::new();
fn print_env_once() {
    PRINT_ENV_ONCE.call_once(|| {
        let triple = format!(
            "{}-{}-{}",
            std::env::consts::ARCH,
            std::env::consts::OS,
            std::env::consts::FAMILY
        );
        let th = std::thread::available_parallelism().map(|n| n.get()).unwrap_or(1);
        let mut cpu = String::new();
        // détecte quelques features x86 (guardées par cfg)
        #[cfg(target_arch = "x86_64")]
        {
            use std::arch::is_x86_feature_detected as f;
            let feats = [
                ("sse2", f!("sse2")),
                ("ssse3", f!("ssse3")),
                ("sse4.1", f!("sse4.1")),
                ("avx", f!("avx")),
                ("avx2", f!("avx2")),
                // avx512 variations (toutes ne sont pas pertinentes)
                ("avx512f", f!("avx512f")),
                ("avx512bw", f!("avx512bw")),
                ("avx512dq", f!("avx512dq")),
            ];
            let _ = write!(&mut cpu, " | x86-features: ");
            for (name, ok) in feats {
                let _ = write!(&mut cpu, "{}{}, ", if ok { "+" } else { "-" }, name);
            }
        }
        eprintln!(
            "[bench:env] target={triple} threads={th} RUSTC={}{}",
            rustc_version_string(),
            cpu
        );
    });
}
fn rustc_version_string() -> String {
    // best-effort : si non dispo, renvoie "unknown"
    std::env::var("RUSTC_VERSION").unwrap_or_else(|_| "unknown".into())
}

// ============================================================================
// Alloc probe (optionnelle) — prend un “snapshot” une fois avant le bench
// ============================================================================
#[cfg(all(not(miri), not(target_os = "android")))]
mod alloc_probe {
    use core::sync::atomic::{AtomicUsize, Ordering};
    use std::alloc::{GlobalAlloc, Layout, System};

    static ALLOCS: AtomicUsize = AtomicUsize::new(0);
    static DEALLOCS: AtomicUsize = AtomicUsize::new(0);
    static BYTES_ALLOC: AtomicUsize = AtomicUsize::new(0);
    static BYTES_FREE: AtomicUsize = AtomicUsize::new(0);

    pub struct CountingAlloc;
    unsafe impl GlobalAlloc for CountingAlloc {
        unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
            ALLOCS.fetch_add(1, Ordering::Relaxed);
            BYTES_ALLOC.fetch_add(layout.size(), Ordering::Relaxed);
            System.alloc(layout)
        }
        unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
            DEALLOCS.fetch_add(1, Ordering::Relaxed);
            BYTES_FREE.fetch_add(layout.size(), Ordering::Relaxed);
            System.dealloc(ptr, layout)
        }
        unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
            // compte comme alloc + free (approx)
            DEALLOCS.fetch_add(1, Ordering::Relaxed);
            BYTES_FREE.fetch_add(layout.size(), Ordering::Relaxed);
            ALLOCS.fetch_add(1, Ordering::Relaxed);
            BYTES_ALLOC.fetch_add(new_size, Ordering::Relaxed);
            System.realloc(ptr, layout, new_size)
        }
    }
    #[global_allocator]
    static GA: CountingAlloc = CountingAlloc;

    #[derive(Clone, Copy, Debug)]
    pub struct Snapshot {
        pub allocs: usize,
        pub frees: usize,
        pub bytes_alloc: usize,
        pub bytes_free: usize,
    }
    pub fn snapshot() -> Snapshot {
        Snapshot {
            allocs: ALLOCS.load(Ordering::Relaxed),
            frees: DEALLOCS.load(Ordering::Relaxed),
            bytes_alloc: BYTES_ALLOC.load(Ordering::Relaxed),
            bytes_free: BYTES_FREE.load(Ordering::Relaxed),
        }
    }
    pub fn diff(a: Snapshot, b: Snapshot) -> Snapshot {
        Snapshot {
            allocs: b.allocs.saturating_sub(a.allocs),
            frees: b.frees.saturating_sub(a.frees),
            bytes_alloc: b.bytes_alloc.saturating_sub(a.bytes_alloc),
            bytes_free: b.bytes_free.saturating_sub(a.bytes_free),
        }
    }
}
#[cfg(all(not(miri), not(target_os = "android")))]
use alloc_probe as ap;

// ============================================================================
// Lexer adapter
// ============================================================================

#[inline(always)]
fn lex_count(src: &str) -> usize {
    // ADAPT ME (1/2): si `tokenize` renvoie un Vec<Token>, ceci marche.
    let tokens = vitte_lexer::tokenize(src);
    // ADAPT ME (2/2): si c’est un itérateur, into_iter().count() idem.
    tokens.into_iter().count()
}

// ============================================================================
// Corpus embarqué & synthèse
// ============================================================================
const SRC_HELLO: &str = r#"let msg = "hello, vitte!"; print(msg);"#;
const SRC_FIBO: &str = r#"
fn fib(n) { if n < 2 { return n; } return fib(n-1) + fib(n-2); }
let out = fib(24);
"#;
const SRC_STRINGS: &str = r#"
let s1 = "ASCII only";
let s2 = "Accents éèà";
let s3 = "汉字 and emoji 🚀🔥";
let s4 = "escapes \n \t \" \\";
"#;
const SRC_MATCH: &str = r#"
match x { 0 => "zero", 1 | 2 => "small", 3..=9 => "medium", _ => "big" }
"#;
const SRC_ENUMS: &str =
    r#"enum Color { Red, Green, Blue(RGB) } let c = Color::Blue(RGB(12, 34, 56));"#;
const SRC_KEYWORDS: &str = r#"
let if_ = 0; let for_ = 1; let match_ = 2; let return_ = 3; let while_ = 4; let fn_ = 5; let enum_ = 6; let struct_ = 7;
"#;
const SRC_COMMENTS_HEAVY: &str = r#"
/* block comment start
   nested? maybe not, but long enough to stress the scanner.
*/
let a = 1; // end of line comment
// another comment
/* another block */ let b = a + 2; // trailing
"#;

#[inline]
fn inflate_to_kib(seed: &str, kib: usize) -> String {
    let target = kib * 1024;
    let mut out = String::with_capacity(target + seed.len());
    while out.len() < target {
        out.push_str(seed);
        out.push('\n');
    }
    out
}
fn synthetic_ascii(kib: usize) -> String {
    let seed = r#"
fn foo(a: i32, b: i32) -> i32 { a + b }
fn bar(x: i32) -> i32 { let mut y = 0; for i in 0..x { y += i; } y }
let arr = [1,2,3,4,5,6,7,8,9,10];
let s = "string with escapes \n\t\"\\ and numbers 1234567890";
"#;
    inflate_to_kib(seed, kib)
}
fn synthetic_mixed(kib: usize) -> String {
    let seed = r#"
fn mix(αβγδ123: i64) -> i64 {
    let π = 314159; let emoji = "🚀🔥✨";
    let very_long_identifier_name_with_many_parts_123 = αβγδ123 + π;
    very_long_identifier_name_with_many_parts_123
}"#;
    inflate_to_kib(seed, kib)
}
fn synthetic_commentary(kib: usize) -> String {
    let seed = r#"
// lots of comments /* block */ // line // line // line
let x = 1 + 2 + 3 + 4 + 5; /* trailing block */ // end
"#;
    inflate_to_kib(seed, kib)
}

// ============================================================================
// External corpus scanner (.vit files)
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
pub fn bench_lexer_micro(c: &mut Criterion) {
    print_env_once();
    let mut group = c.benchmark_group("lexer/micro");

    let samples = env_usize("CRIT_SAMPLES", 60);
    let warmup_ms = env_u64("CRIT_WARMUP_MS", 300);
    let measure_ms = env_u64("CRIT_MEASURE_MS", 800);

    group.sample_size(samples);
    group.warm_up_time(Duration::from_millis(warmup_ms));
    group.measurement_time(Duration::from_millis(measure_ms));

    let cases = [
        ("hello", SRC_HELLO),
        ("fibo", SRC_FIBO),
        ("strings", SRC_STRINGS),
        ("match", SRC_MATCH),
        ("enums", SRC_ENUMS),
        ("keywords", SRC_KEYWORDS),
        ("comments-heavy", SRC_COMMENTS_HEAVY),
    ];

    for (name, src) in cases {
        // probe alloc (unique) si demandé
        if env_bool("LEX_ALLOC_PROBE", false) {
            #[cfg(all(not(miri), not(target_os = "android")))]
            {
                let a0 = ap::snapshot();
                let n = lex_count(src);
                let a1 = ap::snapshot();
                let d = ap::diff(a0, a1);
                eprintln!(
                    "[alloc:{name}] tokens={n} allocs={} frees={} +{}B -{}B",
                    d.allocs, d.frees, d.bytes_alloc, d.bytes_free
                );
            }
        }

        group.throughput(Throughput::Bytes(src.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(name), src, |b, s| {
            b.iter(|| {
                let n = lex_count(black_box(s));
                // ratio bytes/token (sanity pré-calculable si besoin)
                criterion::black_box(n);
            });
        });
    }
    group.finish();
}

pub fn bench_lexer_synthetic(c: &mut Criterion) {
    print_env_once();
    let mut group = c.benchmark_group("lexer/synthetic");
    group.sample_size(env_usize("CRIT_SAMPLES", 50));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1000)));

    let sizes_kib = [16usize, 64, 256, 1024];

    for &kib in &sizes_kib {
        // ASCII
        let ascii_src = synthetic_ascii(kib);
        group.throughput(Throughput::Bytes(ascii_src.len() as u64));
        group.bench_with_input(BenchmarkId::new("ascii", kib), &ascii_src, |b, s| {
            b.iter(|| criterion::black_box(lex_count(black_box(s))));
        });

        // MIX
        let mixed_src = synthetic_mixed(kib);
        group.throughput(Throughput::Bytes(mixed_src.len() as u64));
        group.bench_with_input(BenchmarkId::new("mixed", kib), &mixed_src, |b, s| {
            b.iter(|| criterion::black_box(lex_count(black_box(s))));
        });

        // COMMENTS
        let comm_src = synthetic_commentary(kib);
        group.throughput(Throughput::Bytes(comm_src.len() as u64));
        group.bench_with_input(BenchmarkId::new("comments", kib), &comm_src, |b, s| {
            b.iter(|| criterion::black_box(lex_count(black_box(s))));
        });
    }

    group.finish();
}

pub fn bench_lexer_large(c: &mut Criterion) {
    if !env_bool("BENCH_LARGE", true) {
        eprintln!("[bench] large: skipped (BENCH_LARGE=0)");
        return;
    }
    print_env_once();
    let mut group = c.benchmark_group("lexer/large");
    group.sample_size(env_usize("CRIT_SAMPLES", 30));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 500)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1500)));

    for kib in [256, 512, 1024, 2048] {
        let src = inflate_to_kib(SRC_STRINGS, kib);
        group.throughput(Throughput::Bytes(src.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(format!("{}KiB", kib)), &src, |b, s| {
            b.iter(|| criterion::black_box(lex_count(black_box(s))));
        });
    }

    group.finish();
}

pub fn bench_lexer_external(c: &mut Criterion) {
    print_env_once();
    let root = std::env::var("LEX_DIR").unwrap_or_else(|_| "benchmarks/data".into());
    let max_mb = env_usize("LEX_MAX_MB", 4);
    let files = collect_vit_files(Path::new(&root), max_mb);

    if files.is_empty() {
        eprintln!("[bench] external: no .vit under {root} (<= {max_mb} MiB)");
        return;
    }

    let mut group = c.benchmark_group("lexer/external");
    group.sample_size(env_usize("CRIT_SAMPLES", 40));
    group.warm_up_time(Duration::from_millis(env_u64("CRIT_WARMUP_MS", 300)));
    group.measurement_time(Duration::from_millis(env_u64("CRIT_MEASURE_MS", 1200)));

    for path in files {
        let Some(src) = read_to_string(&path) else {
            continue;
        };
        let id = sanitize_id(&path);
        if env_bool("LEX_ALLOC_PROBE", false) {
            #[cfg(all(not(miri), not(target_os = "android")))]
            {
                let a0 = ap::snapshot();
                let n = lex_count(&src);
                let a1 = ap::snapshot();
                let d = ap::diff(a0, a1);
                eprintln!(
                    "[alloc:file:{id}] tokens={n} allocs={} frees={} +{}B -{}B",
                    d.allocs, d.frees, d.bytes_alloc, d.bytes_free
                );
            }
        }
        group.throughput(Throughput::Bytes(src.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(id), &src, |b, s| {
            b.iter(|| criterion::black_box(lex_count(black_box(s))));
        });
    }

    group.finish();
}

// ============================================================================
// Entrée Criterion
// ============================================================================
criterion_group!(
    benches,
    bench_lexer_micro,
    bench_lexer_synthetic,
    bench_lexer_large,
    bench_lexer_external
);
criterion_main!(benches);
