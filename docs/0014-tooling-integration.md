# RFC 0014 — Tooling Integration

## Résumé
Ce RFC décrit l’intégration des **outils de développement** dans l’écosystème Vitte.  
Il définit comment le compilateur, le gestionnaire de paquets, la VM et la bibliothèque standard interagissent avec des outils externes tels que formatters, linters, IDE, et systèmes de CI/CD.

## Motivation
Un langage n’est adopté massivement que s’il est accompagné d’outils puissants :  
- faciliter la productivité des développeurs,  
- fournir une expérience complète dès l’installation (`vitpm`, `vitc`, `vitfmt`, `vitlint`),  
- s’intégrer avec les écosystèmes existants (GitHub Actions, Docker, IDEs).  

## Design proposé

### Outils officiels
- **`vitc`** : compilateur principal.  
- **`vitpm`** : gestionnaire de paquets.  
- **`vitvm`** : machine virtuelle.  
- **`vitfmt`** : formateur de code (style standard).  
- **`vitlint`** : analyse statique et lints.  
- **`vitdoc`** : génération automatique de documentation.  
- **`vitdbg`** : débogueur interactif.  

### Intégration IDE / LSP
- Serveur **LSP (Language Server Protocol)** officiel.  
- Fonctionnalités : autocomplétion, diagnostics, refactoring, go-to-definition.  
- Intégration prévue avec **VS Code**, **Vim/Neovim**, **JetBrains**.  

### Intégration CI/CD
- Actions GitHub officielles :  
  - `vitte/setup-vitte` pour installer la toolchain.  
  - `vitte/vitpm-build` pour construire et tester.  
- Images Docker prêtes à l’emploi.  
- Support des systèmes de build (Bazel, CMake interop).  

### Exemple d’utilisation CI
```yaml
name: CI

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: vitte/setup-vitte@v1
      - run: vitpm build
      - run: vitpm test
```

### Workflow développeur
1. `vitpm init projet` — initialise un projet.  
2. `vitfmt` — applique le formatage standard.  
3. `vitlint` — vérifie les erreurs de style et sûreté.  
4. `vitc` — compile vers VM/natif/WASM.  
5. `vitdoc` — génère documentation HTML.  
6. `vitdbg` — débogue le bytecode.  

### Extensibilité
- Les outils tiers peuvent s’intégrer via **API JSON-RPC** exposées par `vitc`.  
- `vitpm` peut exécuter des plugins (`vitpm plugin add …`).  

## Alternatives considérées
- **Pas d’outillage officiel** (comme C) : rejeté pour manque de cohérence.  
- **Outils non standardisés** : rejetés pour éviter fragmentation de l’écosystème.  
- **Intégration unique IDE (comme Swift/Xcode)** : rejetée pour garder flexibilité multi-éditeurs.  

## Impact et compatibilité
- Impact fort sur la productivité des développeurs.  
- Compatible avec CI/CD modernes.  
- Améliore l’adoption du langage dans les équipes professionnelles.  
- Légère charge supplémentaire de maintenance (outils synchronisés avec compilateur).  

## Références
- [Rust Tools](https://www.rust-lang.org/tools)  
- [Go Tooling](https://go.dev/doc/cmd)  
- [LSP Protocol](https://microsoft.github.io/language-server-protocol/)  
