

# vitte-backends

Collection **des backends de compilation** du langage Vitte.

`vitte-backends` regroupe et unifie l’ensemble des générateurs de code machine utilisés par le compilateur Vitte, notamment les modules LLVM, Cranelift, WASM et architectures natives (`x86_64`, `aarch64`, `rv64`, `wasm64`).

---

## Objectifs

- Offrir une interface unique et stable pour tous les backends.  
- Gérer la sélection, la configuration et la détection des cibles.  
- Mutualiser les passes d’optimisation et de validation.  
- Simplifier l’intégration de nouveaux backends expérimentaux.  
- Garantir une compatibilité croisée entre architectures.  

---

## Architecture

| Module                  | Rôle |
|--------------------------|------|
| `registry`               | Enregistrement et découverte des backends disponibles |
| `interface`              | Définition du trait `Backend` unifié |
| `llvm`                   | Wrapper vers `vitte-backend-llvm` |
| `cranelift`              | Wrapper vers `vitte-backend-cranelift` |
| `wasm`                   | Wrapper vers `vitte-backend-wasm64` |
| `aarch64` / `x86_64` / `rv64` | Cibles spécifiques |
| `utils`                  | Fonctions d’aide (détection CPU, features LLVM/Cranelift) |
| `tests`                  | Validation inter-backends et cohérence du bytecode |

---

## Exemple d’utilisation

```rust
use vitte_backends::{BackendRegistry, TargetTriple};

fn main() -> anyhow::Result<()> {
    let registry = BackendRegistry::default();
    let backend = registry.get("llvm")?;
    backend.compile("main.vitbc", TargetTriple::host())?;
    Ok(())
}
```

---

## Intégration

- `vitte-compiler` : pilote principal de compilation.  
- `vitte-runtime` : exécution directe des binaires générés.  
- `vitte-analyzer` : instrumentation et profilage backend.  
- `vitte-docgen` : génération des symboles liés aux backends.  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-compiler = { path = "../vitte-compiler", version = "0.1.0" }
vitte-backend-llvm = { path = "../vitte-backend-llvm", version = "0.1.0", optional = true }
vitte-backend-cranelift = { path = "../vitte-backend-cranelift", version = "0.1.0", optional = true }
vitte-backend-wasm64 = { path = "../vitte-backend-wasm64", version = "0.1.0", optional = true }

target-lexicon = "0.12"
anyhow = "1"
serde = { version = "1.0", features = ["derive"] }
```

---

## Tests

```bash
cargo test -p vitte-backends
```

- Vérification de cohérence entre les backends.  
- Tests de compilation croisée (`--target`).  
- Comparaison des performances LLVM/Cranelift/WASM.  

---

## Roadmap

- [ ] Support de backends GPU (Vulkan, Metal, CUDA).  
- [ ] Compilation incrémentale multi-cible.  
- [ ] Sélection automatique du backend optimal.  
- [ ] Support expérimental de RISC-V vectoriel.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau