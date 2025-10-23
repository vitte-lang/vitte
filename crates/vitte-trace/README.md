

# vitte-trace

Système **de traçage et d’instrumentation des événements** pour l’écosystème Vitte.  
`vitte-trace` fournit les primitives nécessaires à la capture, à la corrélation et à l’analyse des événements d’exécution dans le compilateur, le runtime et les outils Vitte.

---

## Objectifs

- Offrir une **infrastructure unifiée de traçage** pour le compilateur et le runtime.  
- Permettre la collecte détaillée d’événements (fonctions, allocations, IO, threads).  
- Intégration directe avec `vitte-logger`, `vitte-profiler`, `vitte-runtime` et `vitte-studio`.  
- Supporter le format `trace_event` (Chrome), `flamegraph` et JSON.  
- Faciliter le débogage des performances et l’analyse comportementale.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `event`       | Définition des événements et métadonnées associées |
| `collector`   | Enregistrement, filtrage et agrégation des traces |
| `writer`      | Export vers fichiers JSON, Chrome trace ou Flamegraph |
| `session`     | Gestion des sessions de traçage et de durée |
| `filter`      | Sélection d’événements par module, durée ou tag |
| `tests`       | Vérification de la cohérence et de la performance |

---

## Exemple d’utilisation

```rust
use vitte_trace::{Tracer, trace_scope};

fn main() -> anyhow::Result<()> {
    let tracer = Tracer::new("build-trace.json");

    trace_scope!("compilation_phase");
    heavy_computation();

    tracer.flush()?;
    Ok(())
}

fn heavy_computation() {
    trace_scope!("parsing");
    std::thread::sleep(std::time::Duration::from_millis(100));
}
```

---

## Intégration

- `vitte-runtime` : instrumentation des appels internes et allocations.  
- `vitte-profiler` : génération automatique de flamegraphs.  
- `vitte-compiler` : traçage des passes de compilation.  
- `vitte-logger` : journalisation enrichie des événements.  
- `vitte-studio` : affichage graphique des traces temps réel.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-logger = { path = "../vitte-logger", version = "0.1.0" }

tracing = "0.1"
tracing-subscriber = { version = "0.3", features = ["json"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
chrono = "0.4"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `flamegraph` pour génération visuelle, `opentelemetry` pour intégration cloud, `crossbeam` pour collecteurs concurrents.

---

## Tests

```bash
cargo test -p vitte-trace
```

- Tests de cohérence des événements collectés.  
- Tests de performance du traçage multi-thread.  
- Tests d’intégration avec `vitte-profiler` et `vitte-runtime`.  
- Benchmarks d’écriture et compression JSON.

---

## Roadmap

- [ ] Export OpenTelemetry complet.  
- [ ] Visualisation graphique dans `vitte-studio`.  
- [ ] Support du streaming temps réel.  
- [ ] Compression adaptative des traces longues.  
- [ ] Filtres avancés et vues temporelles hiérarchiques.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau