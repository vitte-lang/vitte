

# vitte-loader

Système **de chargement et de liaison dynamique** pour l’écosystème Vitte.  
`vitte-loader` gère le chargement des modules binaires ou scriptés (`.vitmod`, `.vitbc`, `.wasm`) ainsi que leur initialisation, dépendances et exécution dans le runtime.

---

## Objectifs

- Offrir un **chargeur de modules dynamique** pour Vitte.  
- Supporter les formats binaires (VBC, WASM, ELF dynamiques).  
- Gérer les dépendances, symboles exportés et imports croisés.  
- Intégration directe avec `vitte-runtime`, `vitte-compiler` et `vitte-analyzer`.  
- Supporter les extensions externes et plugins dynamiques (`.vitplug`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `module`      | Représentation des modules chargés (métadonnées, exports) |
| `resolver`    | Résolution des dépendances et liens symboliques |
| `loader`      | Chargement en mémoire des fichiers binaires ou scripts |
| `linker`      | Liaison dynamique entre modules et bibliothèques externes |
| `ffi`         | Intégration avec C et autres langages (ABI unifié) |
| `cache`       | Mise en cache des modules déjà chargés |
| `tests`       | Tests de cohérence, performance et compatibilité binaire |

---

## Exemple d’utilisation

```rust
use vitte_loader::ModuleLoader;

fn main() -> anyhow::Result<()> {
    let mut loader = ModuleLoader::new();
    let module = loader.load_file("build/main.vitbc")?;

    println!("Module chargé : {}", module.name);
    Ok(())
}
```

---

## Intégration

- `vitte-runtime` : exécution et gestion des modules dynamiques.  
- `vitte-compiler` : export et liaison des artefacts compilés.  
- `vitte-analyzer` : inspection des dépendances et symboles.  
- `vitte-inspect` : visualisation des imports/exports de modules.  
- `vitte-cli` : commandes `vitte load`, `vitte unload` et `vitte modules`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

libloading = "0.8"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `dlopen` pour compatibilité Unix, `wasmtime` pour modules WebAssembly.

---

## Tests

```bash
cargo test -p vitte-loader
```

- Tests de chargement et déchargement de modules.  
- Tests de résolutions symboliques.  
- Tests de compatibilité multi-architecture.  
- Benchmarks de performance du chargement dynamique.

---

## Roadmap

- [ ] Support complet de WASM runtime intégré.  
- [ ] Hot-reload des modules à chaud.  
- [ ] Liaison sécurisée via signatures (`vitte-key`).  
- [ ] Chargement différé et paresseux des dépendances.  
- [ ] API plugin pour `vitte-studio` et `vitte-docgen`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau