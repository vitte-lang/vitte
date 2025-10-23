

# vitte-threads

Système **de gestion des threads et du parallélisme** pour le langage Vitte.  
`vitte-threads` fournit les primitives de création, synchronisation et ordonnancement des threads utilisés par le runtime, le compilateur et les bibliothèques standard.

---

## Objectifs

- Offrir une **API performante et portable** pour la gestion multithread.  
- Intégration directe avec `vitte-runtime`, `vitte-task`, `vitte-scheduler` et `vitte-bench`.  
- Supporter les modèles de concurrence basés sur threads, pools et fibres.  
- Fournir des primitives sûres et optimisées pour la communication entre threads.  
- Faciliter l’analyse et le profilage des performances.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `thread`      | Création, gestion et contrôle de threads natifs |
| `pool`        | Gestion de pools de threads avec file de tâches |
| `sync`        | Primitives de synchronisation (Mutex, Condvar, Barrier) |
| `atomic`      | Types atomiques et opérations bas niveau |
| `channel`     | Communication inter-threads (MPSC, SPSC, broadcast) |
| `tests`       | Validation de la sécurité et des performances |

---

## Exemple d’utilisation

```rust
use vitte_threads::{Thread, Channel};

fn main() -> anyhow::Result<()> {
    let (tx, rx) = Channel::new();

    let t = Thread::spawn(|| {
        tx.send("Hello depuis un thread!").unwrap();
    });

    let msg = rx.recv()?;
    println!("{}", msg);

    t.join()?;
    Ok(())
}
```

---

## Intégration

- `vitte-runtime` : moteur d’exécution multi-thread du langage.  
- `vitte-task` : ordonnancement et exécution parallèle des tâches.  
- `vitte-scheduler` : gestion des priorités et files d’attente.  
- `vitte-bench` : mesure des temps de synchronisation et scalabilité.  
- `vitte-studio` : visualisation du graphe des threads et profils CPU.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

crossbeam = "0.9"
rayon = "1.10"
parking_lot = "0.12"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `tokio` pour compatibilité async, `flume` pour canaux performants, `tracing` pour logs de threads.

---

## Tests

```bash
cargo test -p vitte-threads
```

- Tests de synchronisation (Mutex, Condvar, Barrier).  
- Tests de performance sur pools de threads.  
- Tests de communication inter-thread (canaux, files).  
- Benchmarks de création/join massifs.

---

## Roadmap

- [ ] Support complet des fibres et coroutines.  
- [ ] Planification coopérative et priorités dynamiques.  
- [ ] Profilage intégré pour `vitte-profiler`.  
- [ ] Visualisation du graphe de threads dans `vitte-studio`.  
- [ ] Gestion des threads isolés pour sandboxing.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau