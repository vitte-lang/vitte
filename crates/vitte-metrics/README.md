

# vitte-metrics

Système **de métriques et de télémétrie** pour l’écosystème Vitte.  
`vitte-metrics` collecte, agrège et expose les données de performance, d’utilisation et d’exécution issues des outils, du runtime et du compilateur.

---

## Objectifs

- Fournir un **cadre de mesure unifié** pour les composants Vitte.  
- Supporter les métriques temps réel (latence, mémoire, CPU).  
- Offrir une exportation standardisée (Prometheus, JSON, CSV).  
- Intégrer la télémétrie pour les modules `vitte-runtime`, `vitte-lsp` et `vitte-build`.  
- Permettre la visualisation et l’analyse via `vitte-studio`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `collector`   | Collecte des métriques locales (timers, compteurs, jauges) |
| `registry`    | Enregistrement global des métriques disponibles |
| `exporter`    | Export vers formats externes (Prometheus, JSON, CSV) |
| `histogram`   | Mesure des distributions (latences, tailles, fréquences) |
| `recorder`    | API pour instrumenter le code applicatif |
| `server`      | Service HTTP intégré pour exposition des données |
| `tests`       | Vérification de cohérence et de performance |

---

## Exemple d’utilisation

```rust
use vitte_metrics::{MetricsRegistry, Counter};

fn main() {
    let mut metrics = MetricsRegistry::default();
    let requests = Counter::new("requests_total", "Nombre total de requêtes");

    metrics.register(requests.clone());
    requests.inc();

    println!("{} requêtes enregistrées", requests.get());
}
```

---

## Intégration

- `vitte-runtime` : suivi de l’exécution et du temps CPU.  
- `vitte-lsp` : métriques sur diagnostics et requêtes LSP.  
- `vitte-build` : collecte de métriques de compilation.  
- `vitte-docgen` : suivi de génération documentaire.  
- `vitte-studio` : visualisation graphique des données collectées.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

metrics = "0.21"
metrics-exporter-prometheus = "0.14"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
tokio = { version = "1", features = ["full"] }
``` 

> Optionnel : `chrono` pour horodatage, `csv` pour export local.

---

## Tests

```bash
cargo test -p vitte-metrics
```

- Tests d’enregistrement et de mise à jour des compteurs.  
- Tests de précision des histogrammes.  
- Tests d’intégration avec `vitte-runtime`.  
- Benchmarks sur export et latence.

---

## Roadmap

- [ ] Support complet OpenTelemetry.  
- [ ] Corrélation entre événements LSP et runtime.  
- [ ] API REST/WS pour monitoring distant.  
- [ ] Visualisation en temps réel dans `vitte-studio`.  
- [ ] Export InfluxDB et Graphite.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau