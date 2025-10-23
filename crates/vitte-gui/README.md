

# vitte-gui

Interface graphique **modulaire et réactive** pour l’écosystème Vitte.  
`vitte-gui` fournit les composants UI, thèmes, et systèmes de rendu utilisés par les outils visuels (`vitte-docgen`, `vitte-studio`, `vitte-inspect`, etc.).

---

## Objectifs

- Offrir une **interface graphique native** et performante pour Vitte.  
- Fournir des composants UI multiplateformes (macOS, Linux, Windows).  
- Intégrer le thème clair/sombre global défini par le projet.  
- Supporter le rendu dynamique via GPU (OpenGL / WGPU).  
- Fournir une API modulaire réutilisable dans d’autres projets (`Spectra`, `Platon Editor`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `core`        | Base du framework GUI : événements, layout, input, timer |
| `theme`       | Gestion du thème clair/sombre et des styles adaptatifs |
| `widgets`     | Ensemble de composants (boutons, sliders, panneaux, listes, etc.) |
| `renderer`    | Couche de rendu matériel (OpenGL / WGPU) |
| `window`      | Gestion des fenêtres et intégration OS |
| `events`      | Système d’événements unifié (UI, clavier, souris, focus) |
| `anim`        | Gestion des transitions et animations GPU |
| `tests`       | Tests UI et benchmarks de rendu |

---

## Exemple d’utilisation

```rust
use vitte_gui::{App, Button, Theme};

fn main() -> anyhow::Result<()> {
    let mut app = App::new("Vitte GUI Example")?;

    let theme = Theme::auto(); // clair ou sombre selon l’OS
    let button = Button::new("Compiler")
        .on_click(|| println!("Compilation lancée."));

    app.window()
        .set_theme(theme)
        .add(button)
        .run();

    Ok(())
}
```

---

## Intégration

- `vitte-docgen` : rendu graphique de la documentation.  
- `vitte-inspect` : visualisation interactive des binaires `.vitbc`.  
- `vitte-studio` : IDE complet pour le langage Vitte.  
- `vitte-lsp` : interface de diagnostic visuel.  
- `Spectra` : réutilisation du moteur de rendu pour plugins audio.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

winit = "0.29"
wgpu = "0.19"
piet = "0.6"
tiny-skia = "0.11"
iced = { version = "0.11", optional = true }
serde = { version = "1.0", features = ["derive"] }
thiserror = "1"
anyhow = "1"
``` 

> Optionnel : `iced` pour widgets complexes, `egui` pour éditeur interne, `fontdue` pour rendu texte.

---

## Tests

```bash
cargo test -p vitte-gui
```

- Tests de rendu et de performance (GPU).  
- Tests d’événements et interactions utilisateur.  
- Tests de cohérence entre thèmes clair/sombre.  
- Benchmarks de rafraîchissement et input latency.

---

## Roadmap

- [ ] Support complet de WGPU + Metal + Vulkan.  
- [ ] Mode canvas vectoriel interactif (dessin dynamique).  
- [ ] Widgets dynamiques animés GPU.  
- [ ] Mode inspection AST/IR intégré.  
- [ ] Export en image ou canvas HTML.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau