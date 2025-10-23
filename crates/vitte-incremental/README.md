

# vitte-incremental

Système **de compilation incrémentale** pour le langage Vitte.  
`vitte-incremental` gère la détection des changements, la réutilisation des artefacts précédents et la reconstruction partielle des modules.

---

## Objectifs

- Accélérer considérablement la compilation par **reconstruction partielle**.  
- Détecter les dépendances affectées via un **graphe de compilation persistant**.  
- Stocker et réutiliser les artefacts valides dans le cache (`vitte-cache`).  
- Intégration transparente avec `vitte-build`, `vitte-compiler` et `vitte-analyzer`.  
- Supporter le suivi précis des métadonnées et timestamps (`mtime`, `hash`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `graph`       | Représentation du graphe de dépendances de compilation |
| `tracker`     | Suivi des fichiers, modules et symboles affectés |
| `analyzer`    | Détection des changements et invalidation ciblée |
| `store`       | Sauvegarde des métadonnées de build et artefacts réutilisables |
| `engine`      | Orchestration de la compilation incrémentale |
| `hash`        | Calcul des empreintes de contenu (BLAKE3, xxHash) |
| `serialize`   | Persistance des états entre sessions |
| `tests`       | Vérification de la cohérence et des gains de performance |

---

## Exemple d’utilisation

```rust
use vitte_incremental::IncrementalEngine;
use vitte_core::config::BuildConfig;

fn main() -> anyhow::Result<()> {
    let config = BuildConfig::load("vitte.toml")?;
    let mut engine = IncrementalEngine::new(config)?;

    engine.build_project("src/main.vitte")?;
    println!("Compilation incrémentale terminée.");

    Ok(())
}
```

---

## Intégration

- `vitte-build` : gestion du graphe global et ordonnancement.  
- `vitte-compiler` : compilation sélective des unités valides.  
- `vitte-analyzer` : détection des dépendances et analyse symbolique.  
- `vitte-cache` : stockage des résultats valides.  
- `vitte-lsp` : feedback incrémental dans l’IDE.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-cache = { path = "../vitte-cache", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }

blake3 = "1.5"
xxhash-rust = "0.8"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
rayon = "1"
``` 

> Optionnel : `notify` pour surveillance de fichiers, `crossbeam` pour parallélisme.

---

## Tests

```bash
cargo test -p vitte-incremental
```

- Tests de détection de changement par hash et timestamp.  
- Tests de validité des reconstructions partielles.  
- Tests de performance sur projets complexes.  
- Vérification de la compatibilité avec `vitte-build`.

---

## Roadmap

- [ ] Suivi temps réel via watcher de fichiers.  
- [ ] Recompilation adaptative (priorité symbolique).  
- [ ] Visualisation du graphe de dépendances.  
- [ ] Compilation incrémentale distribuée.  
- [ ] Intégration IDE complète avec `vitte-lsp`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau