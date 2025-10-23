# vitte-opt

Moteur **d’optimisation intermédiaire et binaire** pour le compilateur Vitte.  
`vitte-opt` implémente les passes d’optimisation sur l’IR et le code machine généré par les backends (`vitte-codegen-*`), afin d’améliorer la performance et de réduire la taille des binaires.

---

## Objectifs

- Offrir un **pipeline d’optimisation modulaire et extensible**.  
- Gérer les optimisations au niveau IR et post-génération (binaire).  
- Supporter les passes SSA, DCE, CSE, inlining et propagation constante.  
- Intégration directe avec `vitte-compiler`, `vitte-linker` et `vitte-lto`.  
- Mesurer l’impact des passes via `vitte-metrics`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `ir`          | Représentation intermédiaire optimisable |
| `passes`      | Ensemble des transformations (inline, fold, dce, cse) |
| `analyze`     | Collecte d’informations de flux et dépendances |
| `ssa`         | Construction et simplification de la forme SSA |
| `inline`      | Fusion et réduction des appels de fonction |
| `constprop`   | Propagation et évaluation de constantes |
| `simplify`    | Nettoyage du graphe de contrôle et fusion de blocs |
| `emit`        | Sortie optimisée vers `vitte-codegen` |
| `tests`       | Tests de cohérence et benchmarks |

---

## Exemple d’utilisation

```bash
# Compilation avec optimisations complètes
vitte build --release -O3

# Compilation avec optimisations spécifiques
vitte build -C opt-level=2 -C inline-threshold=128
```

---

## Intégration

- `vitte-compiler` : exécution automatique des passes après IR.  
- `vitte-lto` : fusion et optimisation inter-modulaire.  
- `vitte-linker` : application des passes binaires.  
- `vitte-metrics` : analyse et profilage des gains de performance.  
- `vitte-inspect` : visualisation des graphes d’optimisation.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ir = { path = "../vitte-ir", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }

petgraph = "0.6"
rayon = "1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `cranelift` pour passes bas-niveau, `llvm-sys` pour passes externes.

---

## Tests

```bash
cargo test -p vitte-opt
```

- Tests des passes SSA, DCE, CSE et const-prop.  
- Tests de validité IR avant/après transformation.  
- Tests de performance sur grands graphes.  
- Comparaison des tailles binaires avant/après.

---

## Roadmap

- [ ] Support complet du profilage basé sur usage réel (PGO).  
- [ ] Optimisation vectorielle et SIMD.  
- [ ] Détection automatique des boucles parallélisables.  
- [ ] Intégration des passes GPU via `vitte-gpu`.  
- [ ] Visualisation du graphe d’optimisation dans `vitte-inspect`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
