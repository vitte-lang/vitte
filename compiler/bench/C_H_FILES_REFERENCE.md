# Fichiers .C et .H — Documentation Complète

## 📊 Vue d'Ensemble

**32 fichiers source** (16 .c + 16 .h) implémentent une suite de benchmarking complète et modulaire.

```
Total: 32 fichiers source
├── 14 implémentations (.c) dans src/bench/
├── 2 implémentations (.c) dans src/micro/
├── 1 implémentation (.c) dans src/macro/
├── 16 headers (.h) dans src/bench/
└── 4 benchmarks réels
```

---

## 🏗️ Architecture Modulaire

```
┌─────────────────────────────────────┐
│  Public API (bench.h)               │
├─────────────────────────────────────┤
│ Types (types.h, config.h, ...)      │
├─────────────────────────────────────┤
│ Core Modules                        │
├─ Timing (bench_time.c/h)            │
├─ Registry (bench_registry.c)        │
├─ Statistics (bench_stats.c)         │
├─ Output (output.c/h, csv.c/h)       │
├─────────────────────────────────────┤
│ Advanced Modules                    │
├─ Runner (runner.c/h)                │
├─ Sampling (sample.c/h)              │
├─ Parser (json_parser.c/h)           │
├─ Options (options.c/h)              │
├─ Utilities (alloc, strutil, fmt)    │
├─────────────────────────────────────┤
│ Benchmarks (bm_*.c)                 │
└─────────────────────────────────────┘
```

---

## 📋 Liste Complète des Fichiers

### Noyau Principal (4 fichiers)

| Fichier | Rôle | Lignes |
|---------|------|--------|
| **bench_main.c/h** | CLI exécutable, point d'entrée | 233+ |
| **bench_registry.c** | Registry statique des benchmarks | 100+ |
| **bench_stats.c** | Analyse statistique robuste | 87+ |
| **bench_time.c** | Timing cross-plateforme | 83+ |

### Fonctionnalités Avancées (10 fichiers)

| Fichier | Rôle | Lignes |
|---------|------|--------|
| **runner.c/h** | Orchestration benchmark | 50+ |
| **options.c/h** | Parsing arguments CLI | 100+ |
| **output.c/h** | Formatage résultats | 60+ |
| **timing.c/h** | Timers scoped avancés | 70+ |
| **sample.c/h** | Buffer échantillons | 60+ |
| **json_parser.c/h** | Parser JSON léger | 200+ |
| **alloc.c/h** | Allocation mémoire tracked | 50+ |
| **strutil.c/h** | Utilitaires strings | 150+ |
| **format.c/h** | Formatage numérique | 80+ |
| **csv.c/h** | Export CSV structuré | 80+ |
| **log.c/h** | Logging structuré | 50+ |

### Benchmarks (4 fichiers)

| Fichier | Type | Mesure |
|---------|------|--------|
| **bm_add.c** | Micro | Integer addition (ns) |
| **bm_hash.c** | Micro | FNV-1a 64-bit hash (ns) |
| **bm_memcpy.c** | Micro | Memory copy (ns) |
| **bm_json_parse.c** | Macro | JSON parsing (ms) |

---

## 🔐 Headers Publiques (16)

### Core API
- **bench.h** — Public API principale (max specification)

### Type Definitions
- **types.h** — Types cœur (bench_case, bench_stats, bench_result_t)
- **config.h** — Flags configuration compile-time
- **platform.h** — Détection plateforme (OS, compiler, arch)
- **common.h** — Macros & utilitaires communs

### Module Headers
- **runner.h** — Configuration & interface runner
- **options.h** — Types options CLI
- **output.h** — Types & interface output
- **timing.h** — Timers avancés
- **sample.h** — Buffer d'échantillons
- **json_parser.h** — Parser JSON interface
- **alloc.h** — Allocation mémoire interface
- **strutil.h** — String utilities interface
- **format.h** — Formatage interface
- **csv.h** — CSV writer interface
- **log.h** — Logging interface

---

## 🔧 Modules Détaillés

### 1. **TIMING** (bench_time.c/h)
```c
// Platform-specific monotonic clock
uint64_t bench_now_ns(void);

// Advanced timers
bench_timer_t timer = bench_timer_new();
bench_timer_start(&timer);
// ... work ...
uint64_t elapsed = bench_timer_stop(&timer);
```

**Plateformes:**
- macOS: `mach_absolute_time()` avec timebase
- POSIX: `clock_gettime(CLOCK_MONOTONIC)`
- Windows: `QueryPerformanceCounter()`

### 2. **REGISTRY** (bench_registry.c)
```c
// Static array-based registry
const bench_case* bench_registry_all(int* out_count);
```

**Caractéristiques:**
- Allocation zéro
- Patterns d'extension simples
- 4 benchmarks pré-enregistrés

### 3. **STATISTICS** (bench_stats.c)
```c
// Robust statistical analysis
bench_stats bench_compute_stats(const double* samples, int n);
// Returns: mean, p50, p95, min, max
```

**Techniques:**
- Kahan summation (précision)
- Percentile via tri
- Outlier-resistant

### 4. **RUNNER** (runner.c/h)
```c
// Orchestration des benchmarks
bench_result_t result = bench_run_case(&case, &config);

// Types:
// - Micro: iterations fixes
// - Macro: durée limitée
// - Warmup: pré-échauffe
```

### 5. **OUTPUT** (output.c/h, csv.c/h)
```c
// Text output
bench_print_result(&result);

// CSV export
bench_csv_writer_t* w = bench_csv_writer_new("out.csv");
bench_csv_write_result(w, &result);
```

### 6. **OPTIONS** (options.c/h)
```c
// CLI argument parsing
bench_options_t opts = {0};
bench_parse_options(argc, argv, &opts);

// Supporte:
// --list, --iters, --seconds, --csv, --filter, etc.
```

### 7. **SAMPLING** (sample.c/h)
```c
// Dynamic sample buffer
bench_sample_buffer_t* buf = bench_sample_buffer_new(7);
bench_sample_buffer_add(buf, 12.34);
```

### 8. **JSON PARSER** (json_parser.c/h)
```c
// Lightweight JSON parsing
bench_json_parser_t p = bench_json_parser_new(json_str);
int ok = bench_json_parse_value(&p);
```

**Support:**
- Objects, arrays, strings
- Numbers, booleans, null
- Recursive parsing

### 9. **MEMORY** (alloc.c/h)
```c
// Tracked allocation
void* ptr = bench_malloc(size);
bench_alloc_stats(&total, &count);
```

### 10. **STRINGS** (strutil.c/h)
```c
// String utilities
char* dup = bench_strdup(str);
char* replaced = bench_str_replace(str, old, new);
int match = bench_str_startswith(str, prefix);
```

### 11. **FORMAT** (format.c/h)
```c
// Numeric formatting
bench_format_ns(buf, sz, 1234567.89);     // "1.23 ms"
bench_format_ops_per_sec(buf, sz, 1e6);   // "1.00 M ops/s"
```

### 12. **CSV** (csv.c/h)
```c
// CSV structured output
bench_csv_writer_t* w = bench_csv_writer_new("out.csv");
bench_csv_write_header(w);
bench_csv_write_result(w, &result);
```

### 13. **LOGGING** (log.c/h)
```c
// Structured logging
bench_log_set_level(BENCH_LOG_INFO);
bench_info("Starting benchmark: %s", name);
bench_warn("High variance detected: %.2f%%", variance);
bench_error("Failed to allocate buffer");
```

**Niveaux:**
- DEBUG (0)
- INFO (1)
- WARN (2)
- ERROR (3)

---

## 🚀 Graphe de Dépendances

```
bench.h (public API)
├─ bench_time.c      (stand-alone)
├─ bench_stats.c     (stand-alone)
├─ bench_registry.c  → bench_time
├─ runner.c          → bench_time, stats, types
├─ options.c         → runner, types
├─ output.c          → types, format
│  └─ format.c       (stand-alone)
├─ csv.c             → types, alloc
├─ log.c             (stand-alone)
├─ sample.c          → alloc
├─ timing.c          → bench_time
├─ json_parser.c     (stand-alone)
├─ alloc.c           (stand-alone)
├─ strutil.c         → alloc
├─ Benchmarks        → bench_time
│  ├─ bm_add.c       (stand-alone)
│  ├─ bm_hash.c      (stand-alone)
│  ├─ bm_memcpy.c    (stand-alone)
│  └─ bm_json_parse.c → json_parser
```

---

## 📊 Statistiques

| Métrique | Valeur |
|----------|--------|
| Fichiers .c | 16 |
| Fichiers .h | 16 |
| Total | 32 |
| LOC | ~2500+ |
| Modules | 15 |
| Benchmarks | 4 |
| Headers publiques | 16 |

---

## ✅ Checklist Complétude

- ✓ Timing cross-plateforme
- ✓ Registry benchmarks
- ✓ Analyse statistique robuste
- ✓ Runner flexible
- ✓ Options CLI complètes
- ✓ Output texte & CSV
- ✓ Sampling dynamique
- ✓ Parser JSON intégré
- ✓ Memory tracking
- ✓ String utilities
- ✓ Format numbers
- ✓ Logging structuré
- ✓ 4 benchmarks réels
- ✓ Headers modulaires
- ✓ Zéro dépendances externes

---

## 🔗 Fichiers Connexes

- [CMakeLists.txt](../CMakeLists.txt) — Build configuration
- [BENCHMARK_GUIDE.md](../BENCHMARK_GUIDE.md) — User guide
- [DESIGN.md](../DESIGN.md) — Architecture document
- [CONTRIBUTING.md](../CONTRIBUTING.md) — Contributing guidelines

---

**Status**: ✅ Complet — 32 fichiers source générés & documentés
