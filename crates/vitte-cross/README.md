

# vitte-cross

Outils et infrastructure de **cross-compilation** pour l’écosystème Vitte.

Ce crate gère la compilation croisée vers différentes architectures, systèmes et formats binaires (ELF, Mach-O, Wasm, PE). Il centralise la détection, la configuration et la gestion des toolchains cibles pour `vitte-build`, `vitte-compiler`, et `vitte-cli`.

---

## Objectifs

- Simplifier la compilation multiplateforme pour tous les backends  
- Fournir une interface uniforme pour les toolchains externes (LLVM, GCC, WASM, etc.)  
- Automatiser la détection des triples, architectures et ABI  
- Assurer la reproductibilité et l’isolation des builds  
- Supporter les environnements CI/CD et cross-containerisés  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `detect`       | Détection des toolchains et environnements système |
| `target`       | Modélisation des triples, ABI et sous-architectures |
| `config`       | Lecture et validation des profils de cross-compilation |
| `link`         | Gestion des outils de linkage externes |
| `emit`         | Génération d’artefacts binaires selon la cible |
| `runner`       | Exécution distante ou simulée pour tests |
| `cli`          | Commandes d’inspection et de gestion |
| `tests`        | Tests unitaires et d’intégration multi-arch |

---

## Exemple d’utilisation

### Compilation croisée simple

```
vitte build --target aarch64-apple-darwin
```

Produit un binaire macOS ARM64 à partir d’un hôte x86_64.

### Cross Linux → Windows

```
vitte build --target x86_64-pc-windows-gnu
```

Génère un exécutable `.exe` compatible avec la toolchain MinGW.

### Cross vers WebAssembly

```
vitte build --target wasm32-wasi
vitte run --target wasm32-wasi main.wasm
```

---

## Fichiers de configuration

Exemple `Cross.toml` :

```toml
[target.aarch64-apple-darwin]
cc = "/usr/bin/clang"
sysroot = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.sdk"
ar = "/usr/bin/ar"
cflags = ["-O2", "-arch", "arm64"]

[target.x86_64-unknown-linux-gnu]
linker = "x86_64-linux-gnu-gcc"
lib_dir = "/usr/x86_64-linux-gnu/lib"
```

---

## Commandes CLI

```
vitte cross list-targets
vitte cross detect
vitte cross test --target <triple>
vitte cross info <triple>
vitte cross doctor
```

Exemples :
```
vitte cross list-targets
vitte cross info wasm32-wasi
vitte cross doctor
```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-config = { path = "../vitte-config", version = "0.1.0" }
serde = { version = "1.0", features = ["derive"] }
toml_edit = "0.21"
which = "5"
anyhow = "1"
colored = "2"
target-lexicon = "0.12"
```

---

## Tests

```bash
cargo test -p vitte-cross
vitte cross detect
vitte cross test --target aarch64-apple-darwin
```

- Tests d’intégration multi-plateformes (mock toolchains)  
- Tests de détection d’environnement (`$PATH`, SDKs, sysroots)  
- Snapshots de configuration  
- Benchmarks de génération de triples  

---

## Roadmap

- [ ] Support complet des plateformes iOS et Android  
- [ ] Toolchain cache local et validation SHA256  
- [ ] Cross-linking vers Windows ARM64  
- [ ] Compilation distribuée (remote build clusters)  
- [ ] Interface graphique pour inspection des cibles  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau