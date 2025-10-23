# vitte-style

Système **de gestion du style, thèmes et apparence** pour les interfaces CLI et graphiques de l’écosystème Vitte.  
`vitte-style` définit les couleurs, typographies, animations et transitions utilisées dans les outils comme `vitte-cli`, `vitte-docgen`, et `vitte-studio`.

---

## Objectifs

- Offrir une **API unifiée de gestion des styles** pour toutes les interfaces Vitte.  
- Supporter les thèmes clair/sombre, avec auto-détection selon l’environnement.  
- Fournir un moteur de styles compatible TUI/GUI/HTML.  
- Intégration directe avec `vitte-cli`, `vitte-banner`, `vitte-docgen` et `vitte-studio`.  
- Assurer la cohérence visuelle et la personnalisation via fichiers de configuration.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `theme`       | Gestion des palettes de couleurs et variantes (clair/sombre) |
| `ansi`        | Rendu des styles dans le terminal (couleurs, gras, italique, etc.) |
| `palette`     | Définition des couleurs de base, accents et tons neutres |
| `config`      | Lecture des fichiers de configuration utilisateur (`.vitte-theme.toml`) |
| `effects`     | Animations et transitions pour TUI et docgen |
| `render`      | Application des styles aux composants CLI ou HTML |
| `tests`       | Validation des thèmes et cohérence inter-mode |

---

## Exemple d’utilisation

```rust
use vitte_style::{Theme, Style};

fn main() -> anyhow::Result<()> {
    let theme = Theme::auto()?; // sélection clair/sombre selon OS
    let ok = Style::success(&theme, "Compilation réussie");
    println!("{}", ok);
    Ok(())
}
```

---

## Intégration

- `vitte-cli` : thèmes et couleurs pour les sorties console.  
- `vitte-banner` : stylisation des bannières ASCII et logos.  
- `vitte-docgen` : application des thèmes aux pages HTML.  
- `vitte-studio` : moteur de rendu unifié (clair/sombre, animé).  
- `vitte-lsp` : stylisation des diagnostics et messages.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ansi = { path = "../vitte-ansi", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
toml = "0.8"
console = "0.15"
crossterm = "0.27"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `ratatui` pour interface TUI enrichie, `yansi` pour compatibilité ANSI, `serde_yaml` pour thèmes YAML.

---

## Tests

```bash
cargo test -p vitte-style
```

- Tests de cohérence entre thèmes clair/sombre.  
- Tests de rendu ANSI et HTML.  
- Tests de compatibilité inter-terminal.  
- Benchmarks d’application des styles massifs.

---

## Roadmap

- [ ] Gestion dynamique du thème via variables d’environnement.  
- [ ] Support des animations pour CLI et HTML.  
- [ ] Thèmes personnalisables par projet (`.vitte-theme.toml`).  
- [ ] Intégration visuelle complète dans `vitte-studio`.  
- [ ] Export des palettes vers `vitte-docgen` et `vitte-banner`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
