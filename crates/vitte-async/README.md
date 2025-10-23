# vitte-async

Moteur **asynchrone et multitâche** du langage Vitte.

`vitte-async` implémente le système d’exécution des tâches concurrentes, des futures et des streams pour tout l’écosystème Vitte.  
Il fournit la base des coroutines, du scheduling parallèle et de la gestion d’événements non bloquants dans `vitte-runtime`, `vitte-flow`, et `vitte-compiler`.

---

## Objectifs

- Implémenter un **runtime asynchrone intégré** sans dépendance externe.  
- Offrir une API similaire à `async/await` Rust, adaptée à la VM Vitte.  
- Supporter les **futures**, **streams**, **tasks** et **channels**.  
- Planification coopérative et exécution lock-free.  
- Intégration avec le GC, la VM et les IO (via `vitte-runtime`).  
- Compatible avec WebAssembly et environnements embarqués.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `executor`     | Planificateur principal des tâches |
| `future`       | Implémentation des futures et wakers |
| `task`         | Gestion des handles, joins et cancellations |
| `stream`       | Traitement de flux asynchrones |
| `channel`      | Canaux MPSC/SPMC lock-free |
| `timer`        | Timers, délais et planification différée |
| `sync`         | Primitive de synchronisation (Mutex, RwLock, Barrier) |
| `tests`        | Tests de performance, stress et cohérence |

---

## Exemple d’utilisation

```rust
use vitte_async::{task, sleep};
use std::time::Duration;

#[vitte::main]
async fn main() {
    let t1 = task::spawn(async {
        sleep(Duration::from_millis(500)).await;
        println!("Tâche 1 terminée");
    });

    let t2 = task::spawn(async {
        println!("Tâche 2 immédiate");
    });

    t1.await;
    t2.await;
}
```

---

## Intégration

- `vitte-runtime` : exécution bas-niveau et gestion des threads.
- `vitte-flow` : planification des graphes de dépendances asynchrones.
- `vitte-compiler` : exécution des tâches parallèles de build.
- `vitte-gc` : coordination du ramasse-miettes en contexte concurrent.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

crossbeam = "0.9"
parking_lot = "0.12"
futures = "0.3"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-async
```

- Tests de planification et d’équité de scheduling.  
- Vérification des channels et des streams.  
- Tests de résistance aux cancellations et races.  
- Benchmarks sur latence et parallélisme.

---

## Roadmap

- [ ] Runtime entièrement sans allocation.  
- [ ] Support multi-thread complet (work-stealing).  
- [ ] Profilage des tasks via `vitte-analyzer`.  
- [ ] Exécution asynchrone sur GPU (`vitte-gpu`).  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
