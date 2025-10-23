

# vitte-logger

Système **de journalisation unifié et extensible** pour l’écosystème Vitte.  
`vitte-logger` fournit les primitives de log structurées, colorées et hiérarchisées, utilisées dans tous les outils (`vitte-cli`, `vitte-build`, `vitte-runtime`, etc.).

---

## Objectifs

- Offrir une **infrastructure de log centralisée** et performante.  
- Supporter plusieurs niveaux de gravité (trace, debug, info, warn, error, fatal).  
- Produire des sorties colorées ANSI et des flux JSON pour les outils automatisés.  
- Intégrer des filtres dynamiques et contextes enrichis (fichier, ligne, module).  
- Compatibilité totale avec `vitte-runtime`, `vitte-analyzer`, `vitte-docgen` et `vitte-lsp`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `core`        | API principale de logging (Logger, Record, Level) |
| `fmt`         | Formatage des messages (ANSI, JSON, textuel) |
| `filter`      | Gestion des filtres et masques de niveau |
| `sink`        | Émission vers stdout, fichier, buffer mémoire |
| `async`       | Pipeline de log asynchrone basé sur Tokio |
| `context`     | Ajout automatique de métadonnées de contexte |
| `init`        | Initialisation globale (config, couleur, format) |
| `tests`       | Tests unitaires et benchmarks de performance |

---

## Exemple d’utilisation

```rust
use vitte_logger::{init_default, log_info, log_error};

fn main() -> anyhow::Result<()> {
    init_default()?;
    log_info!("Compilation démarrée");
    log_error!("Erreur de lien : symbole non résolu");
    Ok(())
}
```

---

## Intégration

- `vitte-cli` : affichage des messages d’état et erreurs utilisateurs.  
- `vitte-build` : suivi du pipeline de compilation.  
- `vitte-analyzer` : rapport des diagnostics sémantiques.  
- `vitte-docgen` : suivi de génération documentaire.  
- `vitte-runtime` : trace d’exécution et profiling.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

log = "0.4"
tracing = { version = "0.1", features = ["log"] }
tracing-subscriber = { version = "0.3", features = ["fmt", "env-filter"] }
colored = "2.1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `tokio` pour logs asynchrones, `chrono` pour timestamps.

---

## Tests

```bash
cargo test -p vitte-logger
```

- Tests des niveaux de log et filtres dynamiques.  
- Tests de formatage ANSI et JSON.  
- Tests de performance en mode asynchrone.  
- Tests d’intégration avec `vitte-build` et `vitte-cli`.

---

## Roadmap

- [ ] Système de log distribué (multi-processus).  
- [ ] Export vers journald, syslog et fichiers rotatifs.  
- [ ] Collecte distante des logs via HTTP/WS.  
- [ ] Profiling automatique avec `vitte-analyzer`.  
- [ ] Mode trace multi-thread avec timestamps précis.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau