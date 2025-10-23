

# vitte-progress

Système **d’indicateurs de progression et de suivi d’exécution** pour l’écosystème Vitte.  
`vitte-progress` fournit des utilitaires pour afficher, suivre et mesurer l’avancement des tâches longues, compilations, téléchargements ou opérations parallèles.

---

## Objectifs

- Offrir un **système unifié de barres de progression et de suivi**.  
- Supporter les tâches parallèles, imbriquées et hiérarchiques.  
- Intégration complète avec `vitte-build`, `vitte-cli`, `vitte-docgen` et `vitte-studio`.  
- Gestion des formats de sortie (console, JSON, graphique).  
- Compatibilité avec les environnements TTY et non-interactifs.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `bar`         | Affichage des barres de progression dans la console |
| `multi`       | Gestion simultanée de plusieurs tâches |
| `state`       | Stockage et mise à jour de l’état des tâches |
| `format`      | Formatage du rendu texte et graphique |
| `event`       | Notification d’événements (start, update, finish) |
| `export`      | Sortie JSON pour les systèmes externes |
| `tests`       | Tests d’affichage, d’état et de cohérence |

---

## Exemple d’utilisation

```rust
use vitte_progress::ProgressBar;
use std::thread;
use std::time::Duration;

fn main() {
    let bar = ProgressBar::new(100);

    for i in 0..=100 {
        bar.set_position(i);
        thread::sleep(Duration::from_millis(30));
    }

    bar.finish_with_message("Compilation terminée.");
}
```

---

## Intégration

- `vitte-build` : suivi de la compilation par crate et module.  
- `vitte-docgen` : affichage de la progression de génération documentaire.  
- `vitte-cli` : gestion dynamique des barres lors des commandes longues.  
- `vitte-analyzer` : progression sur les analyses statiques.  
- `vitte-studio` : affichage graphique du progrès global du projet.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

indicatif = "0.17"
console = "0.15"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `tokio` pour progression asynchrone, `crossterm` pour compatibilité large terminal.

---

## Tests

```bash
cargo test -p vitte-progress
```

- Tests de progression simple et multiple.  
- Tests de format de sortie (TTY, JSON).  
- Tests de performance et d’affichage fluide.  
- Benchmarks sur grands ensembles de tâches.

---

## Roadmap

- [ ] Support complet des sous-tâches hiérarchiques.  
- [ ] Rendu graphique intégré dans `vitte-studio`.  
- [ ] Export des traces de progression vers `vitte-metrics`.  
- [ ] Mode silencieux et logs séparés.  
- [ ] Intégration avec le profiler pour visualisation temps réel.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau