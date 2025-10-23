# vitte-flow

Moteur **de flux de données et d’exécution parallèle** pour l’écosystème Vitte.

`vitte-flow` gère la composition, la planification et l’exécution des graphes de tâches au sein du runtime Vitte. Il constitue la base du modèle de concurrence, d’async/await et de traitement en flux continu (streams) intégré à `vitte-runtime` et `vitte-async`.

---

## Objectifs

- Modèle de flux unifié pour tâches synchrones et asynchrones.
- Planificateur de graphe de dépendances.
- Support des pipelines de données et traitements parallèles.
- Intégration directe avec `vitte-runtime` et `vitte-async`.
- API sécurisée, modulaire et extensible.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `graph`        | Représentation du graphe de dépendances |
| `node`         | Définition des nœuds et états |
| `executor`     | Exécution planifiée et ordonnancement |
| `channel`      | Communication et synchronisation entre nœuds |
| `stream`       | Flux de données asynchrones |
| `task`         | Gestion des futures et join handles |
| `tests`        | Tests de performance et de cohérence |

---

## Exemple d’utilisation

```rust
use vitte_flow::{Flow, Node};

fn main() -> anyhow::Result<()> {
    let mut flow = Flow::new();
    let a = flow.node("A", || 1);
    let b = flow.node("B", || 2);
    let sum = flow.node("Sum", move || a.get() + b.get());
    flow.run()?;
    println!("Résultat : {}", sum.get());
    Ok(())
}
```

---

## Intégration

- `vitte-async` : exécution asynchrone, futures et streams.
- `vitte-runtime` : planification bas-niveau et ressources systèmes.
- `vitte-analyzer` : introspection et visualisation du graphe de flux.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-async = { path = "../vitte-async", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

futures = "0.3"
parking_lot = "0.12"
petgraph = "0.6"
rayon = "1.10"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-flow
```

- Tests de dépendances circulaires.
- Validation du scheduling parallèle.
- Tests de flux asynchrones.
- Benchmarks du graphe d’exécution.

---

## Roadmap

- [ ] Visualisation graphique en temps réel des flux.
- [ ] Support pour traitements distribués.
- [ ] Intégration avec `vitte-analyzer` et `vitte-lsp`.
- [ ] Gestion de backpressure et retry policies.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
