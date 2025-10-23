

# vitte-term

Bibliothèque **de gestion avancée du terminal et affichage interactif** pour l’écosystème Vitte.  
`vitte-term` offre des primitives de contrôle ANSI, gestion d’entrée utilisateur, rendu couleur, positionnement du curseur et compatibilité multiplateforme.

---

## Objectifs

- Offrir une **API de terminal moderne et portable**.  
- Supporter les séquences ANSI, UTF-8 et TrueColor.  
- Intégration directe avec `vitte-cli`, `vitte-style`, `vitte-ansi` et `vitte-progress`.  
- Gérer l’entrée clavier, la taille de l’écran et les événements.  
- Fournir un moteur de rendu textuel performant pour TUI et CLI.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `cursor`      | Positionnement, visibilité et déplacement du curseur |
| `color`       | Couleurs ANSI 8/256/TrueColor et détection de support |
| `input`       | Lecture des touches, événements et combinaisons |
| `screen`      | Gestion de la taille du terminal et du buffer |
| `render`      | Rendu structuré et effacement des zones |
| `tests`       | Vérification de compatibilité inter-plateforme |

---

## Exemple d’utilisation

```rust
use vitte_term::{Cursor, Color, Screen};

fn main() -> anyhow::Result<()> {
    let mut screen = Screen::new();
    screen.clear();

    Cursor::goto(10, 5)?;
    Color::fg_rgb(255, 215, 0)?;
    println!("Bienvenue dans le terminal Vitte!");

    Color::reset()?;
    Ok(())
}
```

---

## Intégration

- `vitte-cli` : affichage coloré et contrôle des flux.  
- `vitte-style` : application des thèmes et palettes.  
- `vitte-progress` : rendu des barres et spinners.  
- `vitte-repl` : interaction utilisateur et entrée ligne.  
- `vitte-studio` : terminal intégré au mode graphique.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ansi = { path = "../vitte-ansi", version = "0.1.0" }

crossterm = "0.27"
termcolor = "1.4"
console = "0.15"
unicode-width = "0.1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `ratatui` pour interface TUI complète, `rustyline` pour entrée interactive.

---

## Tests

```bash
cargo test -p vitte-term
```

- Tests d’affichage ANSI et compatibilité Windows/macOS/Linux.  
- Tests d’entrée clavier et lecture non bloquante.  
- Tests de détection des capacités terminal.  
- Benchmarks de performance du rendu texte.

---

## Roadmap

- [ ] Support complet des événements souris.  
- [ ] Mode plein écran TUI.  
- [ ] Gestion des buffers alternatifs et double affichage.  
- [ ] Détection adaptative des capacités terminal.  
- [ ] Intégration dans `vitte-studio` pour affichage interactif.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau