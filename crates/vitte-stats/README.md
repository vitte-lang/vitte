

# vitte-stats

Bibliothèque **de statistiques, mesures et analyse des performances** pour l’écosystème Vitte.  
`vitte-stats` permet de collecter, agréger et interpréter des données de performance, des métriques système et des indicateurs de compilation ou d’exécution.

---

## Objectifs

- Offrir une **infrastructure robuste pour les statistiques internes**.  
- Mesurer les temps de compilation, d’exécution, et les allocations mémoire.  
- Supporter les métriques système (CPU, RAM, IO, threads).  
- Intégration directe avec `vitte-profiler`, `vitte-runtime`, `vitte-build` et `vitte-analyzer`.  
- Fournir un format exportable (JSON, CSV, Prometheus).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `metrics`     | Structures de base pour les mesures et compteurs |
| `timer`       | Gestion du temps et chronométrage précis |
| `system`      | Collecte des statistiques CPU/mémoire via sysinfo |
| `analyze`     | Traitement des séries de données et calculs dérivés |
| `export`      | Exportation vers JSON, CSV ou formats externes |
| `tests`       | Validation de précision et benchmarks |

---

## Exemple d’utilisation

```rust
use vitte_stats::{Timer, Metrics};
use std::thread;
use std::time::Duration;

fn main() -> anyhow::Result<()> {
    let mut timer = Timer::start("calcul_intensif");
    thread::sleep(Duration::from_millis(250));
    timer.stop();

    let mut metrics = Metrics::default();
    metrics.insert(timer);

    println!("Statistiques : {}", metrics.to_json()?);
    Ok(())
}
```

---

## Intégration

- `vitte-profiler` : collecte et visualisation des données de performance.  
- `vitte-runtime` : instrumentation du code exécuté.  
- `vitte-analyzer` : corrélation des statistiques avec les passes du compilateur.  
- `vitte-build` : export des métriques de compilation.  
- `vitte-studio` : affichage interactif des métriques temps réel.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

sysinfo = "0.30"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
csv = "1.3"
chrono = "0.4"
statrs = "0.17"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `prometheus` pour exportation serveur, `plotters` pour visualisation graphique.

---

## Tests

```bash
cargo test -p vitte-stats
```

- Tests de précision temporelle et agrégation.  
- Tests d’exportation JSON/CSV.  
- Tests de cohérence entre plusieurs threads.  
- Benchmarks d’échantillonnage haute fréquence.

---

## Roadmap

- [ ] Export Prometheus natif.  
- [ ] Intégration avec `vitte-profiler` pour affichage direct.  
- [ ] Support des métriques asynchrones.  
- [ ] Analyse graphique intégrée dans `vitte-studio`.  
- [ ] Corrélation entre statistiques runtime et AST.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau