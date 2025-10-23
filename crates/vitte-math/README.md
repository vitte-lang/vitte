

# vitte-math

Bibliothèque **mathématique standard** du langage Vitte.  
`vitte-math` fournit les primitives, fonctions et structures fondamentales pour les calculs numériques, trigonométriques, statistiques et symboliques utilisés dans tout l’écosystème.

---

## Objectifs

- Offrir une **librairie mathématique complète et performante**.  
- Supporter les calculs entiers, flottants, complexes et vectoriels.  
- Intégrer les fonctions avancées : trigonométrie, exponentielles, logarithmes, statistiques, combinatoire.  
- Servir de base pour `vitte-linalg`, `vitte-audio`, `vitte-gpu` et `vitte-analyzer`.  
- Fournir des optimisations SIMD et précision flottante contrôlée.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `consts`      | Constantes mathématiques (`PI`, `E`, `TAU`, `SQRT_2`, etc.) |
| `scalar`      | Opérations sur les nombres entiers et flottants |
| `complex`     | Support des nombres complexes et opérations associées |
| `vector`      | Vecteurs et opérations génériques de base |
| `matrix`      | Matrices et transformations élémentaires |
| `stats`       | Fonctions statistiques (moyenne, variance, corrélation) |
| `combinatorics` | Factorielle, permutations, combinaisons |
| `approx`      | Algorithmes d’approximation et d’interpolation |
| `simd`        | Accélérations vectorielles CPU |
| `tests`       | Tests de précision, cohérence et performance |

---

## Exemple d’utilisation

```rust
use vitte_math::{PI, sin, powf};

fn main() {
    let angle = PI / 4.0;
    let value = sin(angle).powf(2.0);
    println!("Résultat : {}", value);
}
```

---

## Intégration

- `vitte-linalg` : opérations vectorielles et matricielles.  
- `vitte-audio` : transformations FFT, traitement du signal.  
- `vitte-analyzer` : calculs statistiques et inférences.  
- `vitte-compiler` : évaluation des expressions constantes.  
- `vitte-docgen` : documentation automatique des constantes et fonctions.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

num-traits = "0.2"
num-complex = "0.4"
approx = "0.5"
rayon = "1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `packed_simd_2` pour SIMD avancé, `nalgebra` pour extensions linéaires.

---

## Tests

```bash
cargo test -p vitte-math
```

- Tests unitaires sur fonctions scalaires et trigonométriques.  
- Tests de précision flottante.  
- Tests de stabilité des calculs complexes.  
- Benchmarks vectoriels et matriciels.

---

## Roadmap

- [ ] Implémentation complète des fonctions spéciales (erf, gamma, beta).  
- [ ] Optimisation SIMD et GPU via `vitte-gpu`.  
- [ ] Système de calcul symbolique (CAS).  
- [ ] Génération automatique de tables de constantes.  
- [ ] Documentation interactive via `vitte-docgen`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau