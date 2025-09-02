# Vitte — Build Guide

## Introduction
Ce document décrit en détail le processus de **build** du projet Vitte.  
Il couvre :
- Les dépendances requises
- Les étapes de compilation multi-plateformes
- La structure des artefacts produits
- L’intégration CI/CD et la cross-compilation
- Les scripts et outils associés

---

## Prérequis

### Outils de base
- **Rust** (version stable recommandée)
- **Cargo** (inclus avec Rust)
- **Git** (gestion du dépôt)
- **CMake** (certains modules optionnels)
- **pkg-config** (Linux)
- **zstd** (optionnel pour compression bytecode)

### Dépendances spécifiques
- Linux : `build-essential`, `libssl-dev`
- macOS : Xcode Command Line Tools
- Windows : Visual Studio Build Tools ou MSVC

---

## Compilation locale

### Cloner le dépôt
```bash
git clone https://github.com/vitte-lang/vitte.git
cd vitte
```

### Build debug (rapide, non optimisé)
```bash
cargo build
```

### Build release (optimisé)
```bash
cargo build --release
```

### Exécuter tests unitaires
```bash
cargo test
```

### Vérifier le formatage et lint
```bash
cargo fmt --all -- --check
cargo clippy --all-targets -- -D warnings
```

---

## Structure du workspace

```
vitte/
├── modules/
│   ├── vitte-core/
│   ├── vitte-cli/
│   ├── vitte-vm/
│   ├── vitte-compiler/
│   ├── vitte-lsp/
│   └── vitte-fuzz/
├── docs/
├── scripts/
│   ├── build.sh
│   ├── bench.sh
│   ├── release.sh
│   └── fmt.sh
└── Cargo.toml
```

---

## Cross-compilation

### Linux → Windows
```bash
cargo build --release --target x86_64-pc-windows-gnu
```

### Linux → macOS (via osxcross)
```bash
cargo build --release --target x86_64-apple-darwin
```

### Linux → ARM (Raspberry Pi)
```bash
cargo build --release --target armv7-unknown-linux-gnueabihf
```

### WebAssembly / WASI
```bash
cargo build --release --target wasm32-wasi
```

---

## Artefacts produits

- `target/release/vitte-cli` : binaire CLI
- `target/release/vitte-vm` : VM autonome
- `target/release/vitte-compiler` : compilateur bytecode
- `target/release/vitte-lsp` : serveur LSP
- `target/release/*.dll` / `*.so` : bibliothèques natives

---

## CI/CD

### Exemple GitHub Actions

```yaml
name: CI

on:
  push:
    branches: [ main ]
  pull_request:

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: dtolnay/rust-toolchain@stable
      - run: cargo build --release
      - run: cargo test
      - run: cargo clippy --all-targets -- -D warnings
```

### Exemple Release Pipeline

```yaml
name: Release

on:
  push:
    tags:
      - "v*.*.*"

jobs:
  release:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: dtolnay/rust-toolchain@stable
      - run: cargo build --release --workspace
      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: vitte-binaries
          path: target/release/*
```

---

## Scripts associés

### build.sh
```bash
#!/bin/bash
set -e
cargo build --release --workspace
```

### release.sh
```bash
#!/bin/bash
set -e
cargo fmt --all
cargo clippy --all-targets -- -D warnings
cargo test --workspace
cargo build --release --workspace
```

### bench.sh
```bash
#!/bin/bash
set -e
cargo bench --workspace
```

---

## Roadmap Build System
- [x] Support multi-plateformes
- [x] Intégration CI/CD
- [x] Scripts automatisés
- [ ] Packaging Homebrew (macOS)
- [ ] Packaging .deb / .rpm (Linux)
- [ ] Scoop/Chocolatey (Windows)
- [ ] Build Docker images
- [ ] Cache de compilation distribué

---

## Conclusion
Le système de build de Vitte est conçu pour être **fiable, portable et extensible**.  
Il permet aux développeurs de compiler facilement sur toutes les plateformes, tout en s’intégrant aux pipelines CI/CD modernes.  
L’objectif est de fournir une expérience fluide, de la compilation locale au packaging et à la distribution mondiale.