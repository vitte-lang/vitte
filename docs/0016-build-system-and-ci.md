# RFC 0016 — Build System and CI

## Résumé
Ce RFC définit le **système de build** et l’intégration avec l’**intégration continue (CI)** pour le langage **Vitte**.  
Il vise à fournir un processus de compilation reproductible, efficace et extensible, adapté aux environnements locaux comme aux pipelines automatisés.

## Motivation
Un système de build clair et robuste est essentiel pour :  
- garantir la reproductibilité des binaires,  
- simplifier l’intégration des dépendances,  
- permettre l’automatisation dans les environnements CI/CD,  
- assurer une expérience cohérente entre développeurs et plateformes.  

## Design proposé

### Commandes principales (`vitpm` + `vitc`)
- `vitpm build` : construit le projet et ses dépendances.  
- `vitpm test` : exécute les tests unitaires et d’intégration.  
- `vitpm bench` : lance les benchmarks.  
- `vitpm doc` : génère la documentation.  
- `vitpm clean` : nettoie les artefacts de build.  

### Modes de build
- **Debug** : compilation rapide, symboles de debug activés.  
- **Release** : optimisations activées, binaire minimal.  
- **Profile** : instrumentation pour analyse de performance.  

```sh
vitpm build --release
```

### Reproductibilité
- Fichiers `vitte.lock` pour figer les dépendances.  
- Cache des builds (`~/.vitpm/cache`).  
- Builds hermétiques : pas de dépendances implicites du système.  

### Intégration CI/CD
Exemple GitHub Actions :  
```yaml
name: CI

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: vitte/setup-vitte@v1
      - run: vitpm build --release
      - run: vitpm test
```

### Cibles de compilation
- **Natif** : Linux, Windows, macOS, BSD.  
- **VM** : bytecode Vitte.  
- **WASM** : WebAssembly pour navigateur et edge computing.  
- **Embedded** : cross-compilation ARM/RISC-V.  

### Support du caching
- Cache incrémental par module (`target/`).  
- Détection automatique des fichiers modifiés.  
- Intégration avec `sccache`/`ccache` optionnelle.  

### Plugins de build
- Support pour étapes personnalisées (`build.rs`).  
- Hooks `pre-build`, `post-build`.  

### Exemple d’utilisation avancée
```sh
vitpm build --target=wasm32 --release
vitpm test --filter network
vitpm bench --output report.html
```

## Alternatives considérées
- **Pas de système de build officiel** (C/C++) : rejeté car trop chaotique.  
- **Build intégré uniquement au compilateur** : rejeté pour manque de modularité.  
- **Systèmes externes uniquement (Make, CMake)** : rejetés pour manque de cohérence.  

## Impact et compatibilité
- Impact majeur : standardise la compilation et l’automatisation.  
- Compatible avec CI/CD modernes (GitHub, GitLab, Jenkins).  
- Introduit un outil unifié réduisant la complexité pour les développeurs.  

## Références
- [Cargo Build System](https://doc.rust-lang.org/cargo/)  
- [Bazel](https://bazel.build/)  
- [CMake](https://cmake.org/)  
- [Go Build](https://pkg.go.dev/cmd/go#hdr-Compile_packages_and_dependencies)  
