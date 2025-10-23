

# vitte-logs

Système **de collecte et d’agrégation de journaux** pour l’écosystème Vitte.  
`vitte-logs` centralise, stocke et met à disposition les traces d’exécution provenant des modules, du runtime et des outils de la suite Vitte.

---

## Objectifs

- Offrir un **collecteur de logs unifié** et structuré.  
- Supporter les formats **texte**, **JSON**, et **binaire compact**.  
- Fournir des capacités de filtrage, agrégation et recherche.  
- Intégration directe avec `vitte-logger`, `vitte-runtime` et `vitte-analyzer`.  
- Exporter les journaux pour usage CI, observabilité et audit.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `collector`   | Réception et mise en file des logs entrants |
| `storage`     | Persistance sur disque, mémoire ou base externe |
| `query`       | Recherche et filtrage (niveaux, sources, temps) |
| `formatter`   | Mise en forme des sorties (CLI, JSON, HTML) |
| `rotate`      | Gestion de la rotation et purge des anciens fichiers |
| `server`      | Service HTTP/WS pour consultation des logs |
| `tests`       | Tests fonctionnels et de performance |

---

## Exemple d’utilisation

```rust
use vitte_logs::{LogCollector, LogRecord};

fn main() -> anyhow::Result<()> {
    let mut collector = LogCollector::new("./logs")?;
    collector.append(LogRecord::info("Compilation démarrée"));
    collector.append(LogRecord::warn("Cache obsolète détecté"));

    collector.flush()?;
    Ok(())
}
```

---

## Intégration

- `vitte-logger` : source principale des événements enregistrés.  
- `vitte-runtime` : trace d’exécution et profiling.  
- `vitte-build` : collecte des événements de compilation.  
- `vitte-analyzer` : corrélation avec les diagnostics sémantiques.  
- `vitte-cli` : accès aux logs via sous-commandes `vitte logs`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-logger = { path = "../vitte-logger", version = "0.1.0" }

tokio = { version = "1", features = ["full"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
tracing = "0.1"
tracing-subscriber = "0.3"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `warp` pour le serveur HTTP, `sqlite` pour stockage structuré.

---

## Tests

```bash
cargo test -p vitte-logs
```

- Tests de persistance et rotation des fichiers.  
- Tests d’intégration avec `vitte-logger`.  
- Vérification de cohérence JSON et tri temporel.  
- Benchmarks d’ingestion et de lecture.

---

## Roadmap

- [ ] API REST/WS de consultation distante.  
- [ ] Compression des fichiers de logs (LZ4/ZSTD).  
- [ ] Indexation par hash ou UUID.  
- [ ] Interface Web intégrée (`vitte-studio`).  
- [ ] Support syslog et journald.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau