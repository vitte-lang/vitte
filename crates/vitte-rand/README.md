

# vitte-rand

Bibliothèque **de génération aléatoire et distributions probabilistes** pour le langage Vitte.  
`vitte-rand` fournit un ensemble complet d’API pour la génération déterministe et non-déterministe de nombres aléatoires, avec support des distributions classiques, des générateurs cryptographiques et des seeds reproductibles.

---

## Objectifs

- Offrir une **API unifiée et performante** pour la génération pseudo-aléatoire.  
- Supporter les générateurs standards (LCG, XorShift, ChaCha, PCG).  
- Fournir un système de seeds reproductibles pour les tests et simulations.  
- Intégration directe avec `vitte-math`, `vitte-sim`, et `vitte-crypto`.  
- Support des distributions uniformes, normales, exponentielles et personnalisées.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `rng`         | Implémentation des générateurs (LCG, XorShift, ChaCha) |
| `seed`        | Gestion des seeds, état interne et reproductibilité |
| `dist`        | Distributions statistiques et probabilistes |
| `crypto`      | Générateurs sécurisés basés sur ChaCha20 / OS RNG |
| `sample`      | API de tirage et conversion typée |
| `tests`       | Tests statistiques et reproductibilité |

---

## Exemple d’utilisation

```rust
use vitte_rand::{Rng, distributions::Uniform};

fn main() {
    let mut rng = Rng::seeded(42);
    let dist = Uniform::new(0, 10);

    for _ in 0..5 {
        println!("{}", dist.sample(&mut rng));
    }
}
```

---

## Intégration

- `vitte-math` : génération de variables aléatoires pour calculs numériques.  
- `vitte-sim` : simulation et Monte Carlo.  
- `vitte-crypto` : génération de clés et nombres sécurisés.  
- `vitte-bench` : génération d’entrées pseudo-aléatoires.  
- `vitte-test` : création de tests stochastiques reproductibles.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

rand = "0.8"
rand_chacha = "0.3"
rand_pcg = "0.3"
rand_distr = "0.4"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `getrandom` pour RNG système, `statrs` pour distributions avancées.

---

## Tests

```bash
cargo test -p vitte-rand
```

- Tests de reproductibilité des seeds.  
- Tests statistiques (moyenne, variance, uniformité).  
- Tests de performance pour RNG cryptographiques.  
- Vérification de compatibilité avec les autres modules numériques.

---

## Roadmap

- [ ] Support des RNG quantiques (QRandom).  
- [ ] Distributions multinomiales et Poisson.  
- [ ] Générateurs déterministes pour simulations distribuées.  
- [ ] Visualisation graphique des distributions dans `vitte-studio`.  
- [ ] Profilage et calibration automatique de RNG.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau