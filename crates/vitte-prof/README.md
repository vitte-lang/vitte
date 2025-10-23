

# vitte-prof

Moteur **de profilage, instrumentation et analyse des performances** pour l’écosystème Vitte.  
`vitte-prof` collecte, enregistre et visualise les métriques d’exécution, de compilation et d’allocation mémoire, permettant d’optimiser les performances du compilateur, du runtime et des programmes Vitte.

---

## Objectifs

- Offrir un **profilage précis et intégré** au cœur du système Vitte.  
- Collecter les métriques CPU, mémoire, IO et latences d’événements.  
- Intégration directe avec `vitte-runtime`, `vitte-compiler` et `vitte-pgo`.  
- Support de l’export des données vers JSON, Chrome Trace et `vitte-studio`.  
- Permettre le profilage en continu ou à la demande.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `collector`   | Collecte et agrégation des événements et métriques |
| `event`       | Définition des types d’événements (compilation, exécution, mémoire) |
| `analyze`     | Analyse des traces et calcul de statistiques |
| `trace`       | Exportation au format `chrome://tracing` ou `.vittrace` |
| `runtime`     | API runtime pour démarrer/arrêter le profilage |
| `report`      | Génération de rapports et visualisations |
| `tests`       | Tests d’intégrité, cohérence et performance |

---

## Exemple d’utilisation

### Profilage d’un programme

```bash
vitte run --profile --out trace.vittrace
```

### Code instrumenté

```rust
use vitte_prof::Profiler;

fn main() {
    let _prof = Profiler::start("phase_compute");
    heavy_computation();
}
```

### Visualisation

```bash
vitte inspect trace.vittrace --format chrome
```

---

## Intégration

- `vitte-runtime` : instrumentation des appels et threads.  
- `vitte-compiler` : profilage des passes d’analyse et codegen.  
- `vitte-pgo` : génération des fichiers `.vitprof` à partir des traces.  
- `vitte-lsp` : affichage des métriques dans l’éditeur.  
- `vitte-studio` : visualisation graphique et temps réel des traces.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
rayon = "1"
tracing = "0.1"
tracing-subscriber = "0.3"
chrono = "0.4"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `flamegraph` pour visualisation SVG, `criterion` pour benchmarks.

---

## Tests

```bash
cargo test -p vitte-prof
```

- Tests de cohérence des traces et des durées.  
- Tests de performance du collecteur.  
- Tests d’intégration avec `vitte-pgo` et `vitte-runtime`.  
- Vérification de compatibilité du format `.vittrace`.

---

## Roadmap

- [ ] Profilage mémoire avancé (heap, allocations par type).  
- [ ] Intégration native avec Chrome Tracing Viewer.  
- [ ] Profilage distribué multi-nœuds.  
- [ ] Visualisation temps réel dans `vitte-studio`.  
- [ ] API publique de hooks personnalisés.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau