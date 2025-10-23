

# vitte-profiler

Outil **d’analyse et de profilage avancé** pour le compilateur et le runtime Vitte.  
`vitte-profiler` fournit une infrastructure complète pour mesurer, enregistrer et visualiser les performances de la compilation, de l’exécution et des systèmes internes Vitte.

---

## Objectifs

- Offrir un **profilage global, modulaire et précis**.  
- Mesurer les durées des passes du compilateur, allocations mémoire et latences IO.  
- Fournir un export compatible avec Chrome Tracing et `vitte-studio`.  
- Intégration directe avec `vitte-compiler`, `vitte-runtime`, `vitte-metrics` et `vitte-pgo`.  
- Permettre une instrumentation fine sans impacter les performances globales.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `session`     | Gestion du cycle de vie d’une session de profilage |
| `collector`   | Enregistrement des événements et durées |
| `event`       | Définition et typage des événements instrumentés |
| `trace`       | Exportation des données au format `.vittrace` ou JSON |
| `analyze`     | Calcul des statistiques et corrélations |
| `report`      | Génération des rapports lisibles et graphiques |
| `cli`         | Interface en ligne de commande `vitte profiler` |
| `tests`       | Tests de cohérence et benchmarks de précision |

---

## Exemple d’utilisation

### Profilage d’un build

```bash
vitte profiler start --session build --out build.vittrace
vitte build --release
vitte profiler stop --session build
```

### Visualisation

```bash
vitte inspect build.vittrace --format chrome
```

### Code instrumenté

```rust
use vitte_profiler::ProfileScope;

fn compile_stage() {
    let _p = ProfileScope::new("type_inference");
    run_type_inference();
}
```

---

## Intégration

- `vitte-compiler` : instrumentation des passes d’analyse et d’optimisation.  
- `vitte-runtime` : mesure des appels, threads et IO.  
- `vitte-metrics` : envoi des données consolidées vers le système de métriques.  
- `vitte-pgo` : génération automatique des profils d’exécution.  
- `vitte-studio` : visualisation temps réel des données collectées.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
vitte-metrics = { path = "../vitte-metrics", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
rayon = "1"
chrono = "0.4"
tracing = "0.1"
tracing-subscriber = "0.3"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `flamegraph` pour sortie SVG, `criterion` pour comparaison de profils.

---

## Tests

```bash
cargo test -p vitte-profiler
```

- Tests de précision temporelle des événements.  
- Tests d’exportation des fichiers `.vittrace`.  
- Tests de compatibilité avec `vitte-studio`.  
- Benchmarks sur la surcharge introduite par instrumentation.

---

## Roadmap

- [ ] Mode d’analyse temps réel dans `vitte-studio`.  
- [ ] Fusion de plusieurs traces en session unique.  
- [ ] Corrélation automatique entre compilateur et runtime.  
- [ ] Visualisation par thread et par passe.  
- [ ] Export au format standard `perf` et `chrome://tracing`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau