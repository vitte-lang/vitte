# vitte-parallel

Bibliothèque **de parallélisme et d’exécution concurrente** pour le langage Vitte.  
`vitte-parallel` fournit un ensemble d’outils pour la parallélisation des tâches, le traitement en pipeline, les pools de threads et les exécuteurs asynchrones utilisés dans le runtime et le compilateur.

---

## Objectifs

- Offrir une **infrastructure unifiée de parallélisme** pour tous les modules Vitte.  
- Supporter le parallélisme de données et de tâches.  
- Fournir des abstractions haut niveau pour les threads, workers et futures.  
- Intégration directe avec `vitte-runtime`, `vitte-compiler` et `vitte-metrics`.  
- Assurer une scalabilité maximale sur architectures multi-core et ARM.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `pool`        | Gestion de pools de threads et workers |
| `task`        | API de tâches parallèles et futures |
| `sync`        | Primitives de synchronisation (Mutex, Barrier, Channel) |
| `executor`    | Exécution planifiée et ordonnancement de tâches |
| `pipeline`    | Construction de pipelines de traitement parallèles |
| `atomic`      | Opérations atomiques et primitives bas-niveau |
| `tests`       | Tests de charge et benchmarks de performance |

---

## Exemple d’utilisation

```rust
use vitte_parallel::ThreadPool;

fn main() {
    let pool = ThreadPool::new(4);

    for i in 0..8 {
        pool.spawn(move || {
            println!("Tâche {} exécutée dans un thread.", i);
        });
    }

    pool.join();
}
```

---

## Intégration

- `vitte-runtime` : planification et exécution des tâches du langage.  
- `vitte-compiler` : parallélisation des passes d’analyse et codegen.  
- `vitte-analyzer` : traitement parallèle du graphe de dépendances.  
- `vitte-metrics` : mesure et exposition des performances.  
- `vitte-lsp` : exécution concurrente des requêtes LSP.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

rayon = "1"
crossbeam = "0.9"
tokio = { version = "1", features = ["full"] }
async-trait = "0.1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `parking_lot` pour synchronisation optimisée, `num_cpus` pour détection automatique du nombre de threads.

---

## Tests

```bash
cargo test -p vitte-parallel
```

- Tests de parallélisme et sécurité mémoire.  
- Tests de performance sur charges mixtes.  
- Tests d’intégration avec le runtime et le compilateur.  
- Benchmarks des opérations atomiques et pools de threads.

---

## Roadmap

- [ ] Scheduler adaptatif multi-niveau.  
- [ ] Support GPU et offload asynchrone.  
- [ ] Cooperative multitasking pour `vitte-runtime`.  
- [ ] Traces d’exécution visualisables dans `vitte-studio`.  
- [ ] API de pipelines de données parallèles.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
