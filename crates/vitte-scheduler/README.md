

# vitte-scheduler

Système **de planification des tâches et exécution concurrente** pour le langage Vitte.  
`vitte-scheduler` gère la distribution, la priorisation et l’ordonnancement des tâches au sein du runtime, offrant un modèle de concurrence efficace et déterministe.

---

## Objectifs

- Offrir un **ordonnanceur hautement performant et portable**.  
- Supporter les modèles d’exécution synchrone, asynchrone et coopératif.  
- Intégration directe avec `vitte-runtime`, `vitte-async` et `vitte-profiler`.  
- Permettre une planification déterministe pour les tests reproductibles.  
- Gérer efficacement les threads légers, coroutines et files de tâches.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `task`        | Représentation interne d’une tâche (future, thread, coroutine) |
| `queue`       | File de tâches concurrente (work-stealing) |
| `worker`      | Gestion des threads et exécuteurs |
| `timer`       | Gestion du temps, délais et planifications différées |
| `policy`      | Stratégies d’ordonnancement (FIFO, LIFO, Priorité, Fair) |
| `metrics`     | Collecte de statistiques de scheduling |
| `tests`       | Vérification de cohérence et benchmarks de performance |

---

## Exemple d’utilisation

```rust
use vitte_scheduler::{Scheduler, Task};

fn main() -> anyhow::Result<()> {
    let sched = Scheduler::new(4)?;

    let task = Task::spawn(|| {
        println!("Tâche exécutée dans le thread {:?}", std::thread::current().id());
    });

    sched.submit(task);
    sched.join_all();
    Ok(())
}
```

---

## Intégration

- `vitte-runtime` : exécution des tâches et gestion du cycle de vie.  
- `vitte-async` : planification des futures et tâches non bloquantes.  
- `vitte-profiler` : mesure et visualisation du comportement concurrent.  
- `vitte-lsp` : traitement parallèle des diagnostics.  
- `vitte-studio` : affichage graphique des files et threads actifs.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

crossbeam = "0.9"
rayon = "1"
tokio = { version = "1", features = ["full"] }
parking_lot = "0.12"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `async-channel` pour files non bloquantes, `metrics` pour suivi de performance.

---

## Tests

```bash
cargo test -p vitte-scheduler
```

- Tests de performance multithread.  
- Tests de synchronisation et ordonnancement.  
- Tests de compatibilité async/sync.  
- Benchmarks de latence et débit des tâches.

---

## Roadmap

- [ ] Ordonnanceur temps réel.  
- [ ] Planification hiérarchique multi-niveaux.  
- [ ] Intégration complète avec le runtime asynchrone.  
- [ ] Visualisation du graphe de tâches dans `vitte-studio`.  
- [ ] Support du scheduling distribué.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau