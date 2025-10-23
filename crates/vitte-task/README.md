

# vitte-task

Système **de gestion et exécution des tâches** pour l’écosystème Vitte.  
`vitte-task` fournit une infrastructure unifiée pour planifier, exécuter et surveiller des tâches synchrones, asynchrones et distribuées au sein du runtime et du compilateur Vitte.

---

## Objectifs

- Offrir une **API cohérente et performante** pour la gestion des tâches.  
- Supporter les modèles de concurrence synchrone, async et multi-thread.  
- Intégration directe avec `vitte-scheduler`, `vitte-runtime`, `vitte-profiler` et `vitte-build`.  
- Gérer la hiérarchie, les dépendances et les statuts des tâches.  
- Permettre la persistance et la reprise d’exécution.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `core`        | Modèle de tâche de base et cycle de vie |
| `executor`    | Boucle d’exécution et gestion de pool de threads |
| `async`       | Support des tâches asynchrones (futures, await) |
| `graph`       | Gestion des dépendances entre tâches (DAG) |
| `monitor`     | Suivi d’état, métriques et logs d’exécution |
| `tests`       | Validation des exécutions et du parallélisme |

---

## Exemple d’utilisation

```rust
use vitte_task::{Task, TaskExecutor};

fn main() -> anyhow::Result<()> {
    let mut exec = TaskExecutor::new(4);

    let t1 = Task::new("analyse", || {
        println!("Analyse du module...");
    });

    let t2 = Task::depends_on("build", &["analyse"], || {
        println!("Compilation du module...");
    });

    exec.add(t1);
    exec.add(t2);
    exec.run_all()?;
    Ok(())
}
```

---

## Intégration

- `vitte-scheduler` : planification et répartition des tâches.  
- `vitte-runtime` : exécution unifiée des tâches internes.  
- `vitte-profiler` : mesure des temps et performances des tâches.  
- `vitte-build` : orchestration des étapes de compilation.  
- `vitte-studio` : visualisation des graphes et états des tâches.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-scheduler = { path = "../vitte-scheduler", version = "0.1.0" }

rayon = "1.10"
tokio = { version = "1", features = ["full"] }
parking_lot = "0.12"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `petgraph` pour le graphe des dépendances, `tracing` pour la journalisation d’exécution.

---

## Tests

```bash
cargo test -p vitte-task
```

- Tests de parallélisme et synchronisation.  
- Tests de dépendances circulaires.  
- Tests de reprise après échec.  
- Benchmarks de scheduling et temps d’exécution.

---

## Roadmap

- [ ] Support complet des tâches distribuées.  
- [ ] Visualisation des DAG dans `vitte-studio`.  
- [ ] Gestion des priorités dynamiques.  
- [ ] Persistances et rechargement d’état des tâches.  
- [ ] Intégration avec `vitte-bench` pour analyse de performance.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau