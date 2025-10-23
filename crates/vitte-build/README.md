

# vitte-build

Système **de construction et d’orchestration de projets** pour le langage Vitte.

`vitte-build` gère la compilation, la résolution des dépendances, le cache incrémental, la génération des artefacts et l’intégration avec les backends (`LLVM`, `Cranelift`, `WASM`).  
Il constitue le cœur du pipeline de build du compilateur Vitte et des outils associés (`vitte-cli`, `vitte-docgen`, `vitte-site`).

---

## Objectifs

- Orchestration complète du processus de compilation.  
- Gestion du cache incrémental et des artefacts binaires.  
- Résolution des dépendances internes et externes.  
- Build multi-crates et graphes de compilation parallélisés.  
- Intégration directe avec `vitte-project` et `vitte-runtime`.  
- Compilation conditionnelle selon la plateforme et le backend.  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `planner`      | Analyse du projet et création du plan de build |
| `executor`     | Gestion des tâches parallèles et dépendances |
| `cache`        | Cache incrémental et vérification des artefacts |
| `artifact`     | Gestion des fichiers compilés et binaires générés |
| `resolver`     | Résolution des modules et dépendances |
| `backend`      | Appel des compilateurs bas-niveau (LLVM, Cranelift, etc.) |
| `watch`        | Build automatique sur modification de fichier |
| `cli`          | Interface CLI (`vitte build`, `vitte clean`) |
| `tests`        | Tests d’intégration et de cohérence |

---

## Exemple d’utilisation

```bash
vitte build
vitte build --backend llvm --release
vitte build --target wasm64 --out target/wasm/
```

Depuis Rust :
```rust
use vitte_build::BuildSystem;

fn main() -> anyhow::Result<()> {
    let build = BuildSystem::new()
        .backend("llvm")
        .release(true)
        .output("target/bin");
    build.run()?;
    Ok(())
}
```

---

## Intégration

- `vitte-cli` : point d’entrée des commandes utilisateur.  
- `vitte-project` : détection automatique du workspace et des crates.  
- `vitte-backends` : sélection et appel du backend de compilation.  
- `vitte-analyzer` : validation du graphe de dépendances avant compilation.  
- `vitte-runtime` : exécution des binaires générés.  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-project = { path = "../vitte-project", version = "0.1.0" }
vitte-backends = { path = "../vitte-backends", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }

anyhow = "1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
colored = "2"
notify = "6"
rayon = "1.10"
```

---

## Tests

```bash
cargo test -p vitte-build
vitte build --watch
```

- Tests de validité du graphe de compilation.  
- Vérification du cache incrémental.  
- Tests multi-backends.  
- Snapshots des builds réussis/échoués.  

---

## Roadmap

- [ ] Compilation distribuée via threads ou réseau.  
- [ ] Build cross-platform et cross-compilation automatique.  
- [ ] Support de profils personnalisés (dev, test, release, bench).  
- [ ] Intégration avec `vitte-bench` pour mesure de performance.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau