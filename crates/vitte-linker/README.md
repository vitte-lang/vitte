

# vitte-linker

Éditeur de liens (**linker**) pour le compilateur Vitte.  
`vitte-linker` assemble et relie les artefacts binaires générés par le compilateur, résout les symboles externes et produit les exécutables finaux (.vitbc, .vitexe, .wasm, etc.).

---

## Objectifs

- Fournir un **linker modulaire et multiplateforme** pour Vitte.  
- Supporter plusieurs formats : ELF, Mach-O, PE, WebAssembly.  
- Gérer la résolution de symboles, relocations et sections personnalisées.  
- Optimiser les tailles binaires et les dépendances croisées.  
- Intégration transparente avec `vitte-compiler`, `vitte-build` et `vitte-codegen`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `object`      | Lecture et fusion des objets intermédiaires (.vobj, .o) |
| `symbol`      | Table des symboles, résolution et vérification de doublons |
| `reloc`       | Application des relocations et ajustements d’adresse |
| `section`     | Construction et alignement des sections mémoire |
| `emit`        | Émission du binaire final (ELF, Mach-O, PE, WASM) |
| `strip`       | Suppression des symboles non nécessaires |
| `opt`         | Passes d’optimisation sur le binaire |
| `tests`       | Tests de cohérence et compatibilité multi-format |

---

## Exemple d’utilisation

```bash
# Lier des objets compilés
vitte linker build/main.vobj build/libmath.vobj -o target/main.vitexe

# Lier vers WebAssembly
vitte linker --target wasm32 build/*.vobj -o target/module.wasm

# Générer un exécutable optimisé
vitte linker build/*.vobj -O2 --strip -o target/release/app.vitexe
```

---

## Intégration

- `vitte-compiler` : production des objets intermédiaires.  
- `vitte-build` : orchestration de la phase de linkage.  
- `vitte-codegen-*` : génération des sections binaires par architecture.  
- `vitte-analyzer` : validation symbolique post-link.  
- `vitte-inspect` : inspection du binaire final.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }

object = "0.36"
goblin = "0.8"
byteorder = "1.5"
rayon = "1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `lz4_flex` pour compression de sections, `memmap2` pour mappage mémoire rapide.

---

## Tests

```bash
cargo test -p vitte-linker
```

- Tests de linkage ELF/Mach-O/PE/WASM.  
- Tests de résolutions symboliques et relocations.  
- Vérification des offsets et alignements mémoire.  
- Benchmarks sur grands binaires.

---

## Roadmap

- [ ] Support complet du format Mach-O sur macOS.  
- [ ] LTO (Link-Time Optimization) intégrée.  
- [ ] Émission incrémentale des sections binaires.  
- [ ] Support JIT et linking en mémoire.  
- [ ] Visualisation du graphe de dépendances dans `vitte-inspect`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau