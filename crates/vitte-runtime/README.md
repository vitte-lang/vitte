

# vitte-runtime

Système **d’exécution du langage Vitte**, responsable de la gestion mémoire, du modèle d’exécution, et de l’interfaçage natif entre le code compilé et les bibliothèques externes.  
`vitte-runtime` constitue le cœur d’exécution dynamique pour les binaires, le REPL et les modules Vitte.

---

## Objectifs

- Fournir un **runtime performant, sûr et portable**.  
- Gérer les allocations mémoire, piles d’exécution et structures dynamiques.  
- Implémenter le modèle de threads, exceptions et contextes d’appel.  
- Intégration complète avec `vitte-compiler`, `vitte-vm`, et `vitte-ffi`.  
- Support des plateformes x86_64, aarch64, rv64 et wasm64.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `memory`      | Gestionnaire mémoire (heap, stack, GC optionnel) |
| `thread`      | Modèle de concurrence et planificateur de threads légers |
| `ffi`         | Interface avec les bibliothèques natives C, C++, Rust |
| `context`     | Gestion du contexte d’exécution (frame, scope, locals) |
| `exception`   | Gestion des erreurs, panic et unwind contrôlé |
| `loader`      | Chargement des modules binaires `.vitbc` |
| `tests`       | Tests de stabilité, performance et compatibilité |

---

## Exemple d’utilisation

### Exécution d’un fichier compilé

```bash
vitte run main.vitbc
```

### Exemple d’appel FFI

```vitte
extern fn printf(fmt: *const char, ...) -> int;

fn main() {
    printf("Hello from Vitte!\n");
}
```

---

## Intégration

- `vitte-compiler` : génère les modules `.vitbc` pour exécution.  
- `vitte-vm` : exécute les instructions et gère la pile virtuelle.  
- `vitte-ffi` : permet les appels externes vers C/C++/Rust.  
- `vitte-analyzer` : instrumentation et profilage.  
- `vitte-studio` : exécution contrôlée avec visualisation temps réel.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ffi = { path = "../vitte-ffi", version = "0.1.0" }
vitte-vm = { path = "../vitte-vm", version = "0.1.0" }

parking_lot = "0.12"
libc = "0.2"
backtrace = "0.3"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `mimalloc` pour allocation optimisée, `rayon` pour parallélisme, `tracing` pour instrumentation.

---

## Tests

```bash
cargo test -p vitte-runtime
```

- Tests de performance et allocation mémoire.  
- Tests de compatibilité inter-architectures.  
- Tests FFI (C, Rust, Wasm).  
- Vérification de stabilité multi-threads.

---

## Roadmap

- [ ] GC incrémental configurable.  
- [ ] Gestion avancée des threads asynchrones.  
- [ ] Mode sandbox sécurisé.  
- [ ] Intégration directe avec `vitte-profiler`.  
- [ ] Visualisation mémoire et call stack dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau