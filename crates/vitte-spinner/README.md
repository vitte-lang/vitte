

# vitte-spinner

Bibliothèque **d’indicateurs de progression animés (spinners)** pour l’écosystème Vitte.  
`vitte-spinner` fournit des animations de terminal, barres d’attente et indicateurs dynamiques utilisés pendant les opérations longues du CLI, du build et du runtime.

---

## Objectifs

- Offrir une **API simple et asynchrone** pour afficher des spinners animés.  
- Supporter plusieurs styles (dots, line, arc, bounce, pulse).  
- Intégration directe avec `vitte-cli`, `vitte-progress`, `vitte-build` et `vitte-docgen`.  
- Détection automatique du terminal et fallback non interactif.  
- Synchronisation avec les tâches parallèles du scheduler Vitte.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `style`       | Définition des motifs d’animation et séquences UTF-8 |
| `spinner`     | Gestion de l’état, vitesse et rendu asynchrone |
| `multi`       | Affichage simultané de plusieurs spinners |
| `theme`       | Couleurs et styles (clair/sombre) |
| `status`      | Gestion des états (en cours, succès, erreur) |
| `tests`       | Validation du rendu et des performances |

---

## Exemple d’utilisation

```rust
use vitte_spinner::Spinner;
use std::time::Duration;
use std::thread;

fn main() -> anyhow::Result<()> {
    let mut spinner = Spinner::new("Compilation du projet...");
    spinner.start();
    thread::sleep(Duration::from_secs(3));
    spinner.succeed("Build terminé.");
    Ok(())
}
```

---

## Intégration

- `vitte-cli` : spinners pour les opérations longues et chargements.  
- `vitte-build` : affichage des étapes de compilation et packaging.  
- `vitte-progress` : synchronisation des barres et spinners simultanés.  
- `vitte-docgen` : rendu dynamique lors de la génération documentaire.  
- `vitte-studio` : animation intégrée dans le terminal graphique.

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

> Optionnel : `tokio` pour spinners asynchrones, `crossterm` pour compatibilité large terminal.

---

## Tests

```bash
cargo test -p vitte-spinner
```

- Tests de rendu TTY/non-TTY.  
- Tests de performance et latence d’animation.  
- Tests de synchronisation avec plusieurs tâches.  
- Benchmarks sur 1000 spinners simultanés.

---

## Roadmap

- [ ] Support complet des spinners asynchrones.  
- [ ] Animation adaptative selon le framerate du terminal.  
- [ ] Support graphique pour `vitte-studio`.  
- [ ] Personnalisation des motifs et couleurs.  
- [ ] Mode silencieux pour CI/CD.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau