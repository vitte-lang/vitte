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
- Validation du stripping ANSI et du rendu simulé.
- Exercices de simulation headless (`SimTerm`, `VirtualTerm`).

---

## Roadmap

- [x] Support SGR étendu (polices personnalisées, doubles largeurs).
  - `src/sgr.rs` expose `FontFamily`, `WidthMode` et gère les séquences ESC `#3/#4/#6` ainsi que SGR `10..19`.
  - `Style` propose des helpers (`font`, `width`) et dégrade proprement lorsque le terminal ne supporte pas l’option.
  - Tests unitaires et d’intégration (voir `tests/sim.rs`) vérifient l’émission conditionnelle des séquences.
- [x] Détection automatique des capacités sous Windows (ConPTY).
  - `src/caps.rs` agrège TTY, heuristiques TERM/COLORTERM, hyperliens et niveaux de couleurs.
  - `src/window.rs` active Virtual Terminal Processing / ConPTY via `SetConsoleMode` et remonte les flags `Capability`.
  - Fallbacks environnement (`NO_COLOR`, `FORCE_COLOR`, variables VTE) appliqués de manière transverse.
- [x] Rendu virtuel ANSI pour environnements headless.
  - `src/render.rs` fournit `VirtualTerm`/`SimTerm` pour journaliser ou rejouer les évènements ANSI sans TTY.
  - `Renderer` introduit un trait unifié et gère le downgrade automatique selon les capacités détectées.
  - Des tests confirment la stabilité des snapshots et l’ordre des événements.
- [x] Mode simulation pour tests unitaires.
  - `SimTerm` capture les séquences issues des composants consommateurs pour assertions déterministes.
  - Suite de tests dédiée (`tests/sim.rs`) couvrant resets, double hauteur, fontes et hyperliens.
  - Peut être activé en CI pour valider le comportement dégradé ou forcer l’absence de couleurs (`no_color`).

**Transverse**
- Facteur commun `caps.rs` exploité par les backends réels/virtuels, partageable avec `vitte-cli`.
- Traces activables via `LOG=debug vitte-ansi` (utilise `tracing` côté consommateur) pour diagnostiquer les décisions de détection.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
