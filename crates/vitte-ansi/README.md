# vitte-ansi

Bibliothèque **ANSI/VT100** pour le formatage, la couleur et le contrôle de terminal dans l’écosystème Vitte.

`vitte-ansi` fournit une implémentation complète des séquences d’échappement ANSI, avec gestion des couleurs, du style, du curseur et des commandes OSC (Operating System Command).  
C’est une brique essentielle pour `vitte-cli`, `vitte-logger`, et `vitte-tools`.

---

## Objectifs

- Gestion complète des séquences ANSI standard.
- Support des couleurs 8/16/256 bits et RGB.
- Styles combinés : gras, italique, souligné, inversé, clignotant.
- Contrôle du curseur, du terminal et de la sortie texte.
- Compatibilité multi-plateforme et détection automatique des capacités.
- Suppression ou désactivation des séquences pour les sorties non TTY.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `codes`        | Définitions brutes des séquences d’échappement |
| `style`        | Couleurs, attributs et combinaisons |
| `cursor`       | Mouvements, position et visibilité du curseur |
| `clear`        | Effacement d’écran, lignes et buffers |
| `osc`          | Commandes OSC : titre, liens hypertexte, notifications |
| `detect`       | Détection des capacités du terminal (truecolor, support ANSI) |
| `util`         | Helpers, parsing et stripping de séquences |
| `tests`        | Vérification de conformité et tests visuels |

---

## Exemple d’utilisation

```rust
use vitte_ansi::{paint, fg_rgb, bold};

fn main() {
    println!("{}", paint("Vitte", fg_rgb(255, 180, 0).bold()));
}
```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
colored = "2"
```

---

## Tests

```bash
cargo test -p vitte-ansi
```

- Vérification des codes générés.
- Tests de compatibilité avec divers terminaux.
- Validation du stripping ANSI.

---

## Roadmap

- [ ] Support SGR étendu (polices personnalisées, doubles largeurs).
- [ ] Détection automatique des capacités sous Windows (ConPTY).
- [ ] Rendu virtuel ANSI pour environnements headless.
- [ ] Mode simulation pour tests unitaires.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau