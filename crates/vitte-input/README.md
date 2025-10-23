

# vitte-input

Système **unifié de gestion des entrées utilisateur** pour l’écosystème Vitte.  
`vitte-input` gère les événements clavier, souris, pavé tactile et périphériques externes, avec un modèle réactif et extensible.

---

## Objectifs

- Fournir une couche d’abstraction **multiplateforme** (macOS, Linux, Windows).  
- Offrir une gestion unifiée des **événements d’entrée** : clavier, souris, gamepad, tablette, etc.  
- Intégrer la capture et le mapping des touches personnalisées.  
- Supporter les modèles de propagation, priorités et focus.  
- Intégration native avec `vitte-gui`, `vitte-lsp` et `vitte-studio`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `keyboard`    | Gestion des touches, combinaisons, layouts et raccourcis |
| `mouse`       | Gestion des clics, mouvements, scroll et gestures |
| `gamepad`     | Support des manettes et périphériques HID |
| `tablet`      | Support des tablettes graphiques et stylets |
| `events`      | Événements unifiés avec timestamps et contextes |
| `state`       | État global des entrées (focus, pression, verrouillage) |
| `mapper`      | Remappage dynamique et profils utilisateur |
| `async`       | Événements asynchrones basés sur Tokio |
| `tests`       | Tests fonctionnels et d’intégration multi-périphériques |

---

## Exemple d’utilisation

```rust
use vitte_input::keyboard::{KeyEvent, KeyCode};

fn handle_input(event: KeyEvent) {
    if event.code == KeyCode::Escape && event.is_pressed() {
        println!("Fermeture de la fenêtre.");
    }
}
```

---

## Intégration

- `vitte-gui` : gestion des interactions utilisateur dans l’interface.  
- `vitte-lsp` : capture des raccourcis IDE.  
- `vitte-studio` : événements globaux pour éditeur et visualiseur.  
- `vitte-cli` : support des entrées interactives.  
- `vitte-runtime` : gestion des périphériques d’entrée/sortie.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

winit = "0.29"
tokio = { version = "1", features = ["full"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `gilrs` pour gamepads, `inputbot` pour raccourcis globaux.

---

## Tests

```bash
cargo test -p vitte-input
```

- Tests d’événements clavier/souris.  
- Tests d’intégration HID.  
- Validation des mappings et priorités.  
- Benchmarks de latence d’événement.

---

## Roadmap

- [ ] Support tactile et multi-touch complet.  
- [ ] Profilage temps réel des entrées.  
- [ ] Remappage dynamique via fichier de configuration.  
- [ ] Intégration `vitte-gui` pour gestures complexes.  
- [ ] Support Bluetooth HID.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau