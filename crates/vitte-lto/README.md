

# vitte-lto

Système **Link-Time Optimization (LTO)** pour le compilateur Vitte.  
`vitte-lto` fusionne et optimise les unités de compilation au moment de l’édition de liens afin de produire des binaires plus rapides, plus compacts et mieux optimisés.

---

## Objectifs

- Fournir un **moteur LTO modulaire et portable** pour le compilateur Vitte.  
- Fusionner les modules IR pour effectuer les optimisations inter-fonctions.  
- Réduire la taille et le temps d’exécution des binaires générés.  
- Intégration transparente avec `vitte-compiler`, `vitte-codegen` et `vitte-linker`.  
- Support des optimisations LLVM et Cranelift.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `merge`       | Fusion des unités IR en graphe global |
| `opt`         | Passes d’optimisation (inlining, DCE, CSE, const-folding) |
| `analyze`     | Analyse des dépendances inter-modulaires |
| `inline`      | Stratégies d’inlining et propagation de constantes |
| `strip`       | Nettoyage des symboles inutilisés |
| `emit`        | Sortie optimisée vers `vitte-linker` |
| `tests`       | Vérification de la cohérence et du gain de performance |

---

## Exemple d’utilisation

```bash
# Compilation avec LTO
vitte build --release --lto

# Compilation multi-crate avec fusion IR
vitte build --workspace --lto full

# LTO partiel (fonctionnel seulement)
vitte build --lto functions
```

---

## Intégration

- `vitte-compiler` : activation des passes LTO globales.  
- `vitte-linker` : application finale des optimisations fusionnées.  
- `vitte-codegen-*` : backends compatibles (LLVM, Cranelift, WASM).  
- `vitte-analyzer` : vérification de la cohérence symbolique.  
- `vitte-inspect` : visualisation du graphe LTO résultant.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ir = { path = "../vitte-ir", version = "0.1.0" }
vitte-linker = { path = "../vitte-linker", version = "0.1.0" }

rayon = "1"
petgraph = "0.6"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
``` 

> Optionnel : `llvm-sys` pour passes natives, `cranelift` pour LTO allégée.

---

## Tests

```bash
cargo test -p vitte-lto
```

- Tests de fusion IR et validité des symboles.  
- Tests de réduction de taille binaire.  
- Tests d’intégration avec `vitte-linker` et `vitte-codegen`.  
- Benchmarks de performance avant/après LTO.

---

## Roadmap

- [ ] LTO incrémental et différé.  
- [ ] Support multi-thread natif via rayon.  
- [ ] Fusion adaptative selon le profil d’exécution.  
- [ ] Compatibilité WASM et JIT.  
- [ ] Visualisation graphique du graphe d’optimisation.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau