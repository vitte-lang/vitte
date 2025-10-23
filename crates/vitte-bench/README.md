

# vitte-bench

Framework **de benchmarks et mesures de performance** pour le langage Vitte.

`vitte-bench` fournit une infrastructure unifiée pour exécuter, comparer et profiler des benchmarks sur le compilateur, le runtime, et les bibliothèques standard de Vitte.  
Il permet d’évaluer les performances des builds, du GC, des backends et des modules via un format reproductible et exportable.

---

## Objectifs

- Benchmarks reproductibles avec isolation et contrôle de contexte.  
- Intégration directe avec `cargo bench` et `vitte bench`.  
- Comparaison entre versions (historique, CI/CD).  
- Profilage des performances de compilation, exécution et GC.  
- Export des résultats vers JSON, CSV, Markdown et graphiques HTML.  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `runner`       | Gestion des sessions de benchmark et exécution isolée |
| `metrics`      | Collecte de mesures : temps, mémoire, CPU, IO |
| `compare`      | Différentiel entre runs successifs |
| `report`       | Génération de rapports Markdown/HTML |
| `export`       | Exportation des données (JSON, CSV) |
| `cli`          | Interface en ligne de commande (`vitte bench run`, `diff`) |
| `tests`        | Tests de cohérence et reproductibilité |

---

## Exemple d’utilisation

```bash
vitte bench run --target vitte-compiler
vitte bench diff --baseline previous.json --new current.json
```

Depuis Rust :
```rust
use vitte_bench::{Bench, Measure};

fn main() {
    let mut bench = Bench::new("fib_iterative", || fib(30));
    bench.run();
    println!("{}", bench.report());
}
```

---

## Sortie type

```
Benchmark: fib_iterative
  Time (mean ± σ): 1.32 ms ± 0.03 ms
  Memory: 4.5 MB
  GC Pauses: 0.02 ms
  Speedup vs baseline: +4.2%
```

---

## Intégration

- `vitte-compiler` : mesures de temps de compilation.  
- `vitte-runtime` : profilage d’exécution.  
- `vitte-gc` : suivi des pauses et allocations.  
- `vitte-analyzer` : génération automatique de rapports.  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
criterion = "0.5"
anyhow = "1"
colored = "2"
```

---

## Tests

```bash
cargo test -p vitte-bench
```

- Tests de cohérence temporelle.  
- Comparaison multi-runs et détection d’écarts.  
- Validation des exports et rapports HTML.  

---

## Roadmap

- [ ] Intégration avec GitHub Actions pour benchmarks automatiques.  
- [ ] Export interactif (graphiques SVG/Plotly).  
- [ ] Benchmarks GPU (`vitte-gpu`) et async (`vitte-async`).  
- [ ] Mode CI silencieux avec seuils de régression.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau