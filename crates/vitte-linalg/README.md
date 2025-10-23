

# vitte-linalg

Bibliothèque **d’algèbre linéaire** pour le langage Vitte.  
`vitte-linalg` fournit les structures et opérations fondamentales sur les vecteurs, matrices et tenseurs utilisés dans les calculs scientifiques, graphiques et audio.

---

## Objectifs

- Offrir une **API mathématique unifiée** pour les opérations linéaires.  
- Supporter les types scalaires (i32, f32, f64) et les opérations génériques.  
- Fournir des implémentations optimisées pour CPU et SIMD.  
- Servir de base pour `vitte-math`, `vitte-audio` et `vitte-gpu`.  
- Intégration directe dans le compilateur et le runtime.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `vector`      | Structures `Vec2`, `Vec3`, `Vec4` et opérations de base |
| `matrix`      | Matrices 2x2, 3x3, 4x4 avec inversion, transposition, déterminant |
| `tensor`      | Opérations multi-dimensionnelles et contractions |
| `ops`         | Implémentations génériques (dot, cross, mul, add, etc.) |
| `simd`        | Optimisations CPU (SSE, AVX, NEON) |
| `decomp`      | Décompositions LU, QR, SVD |
| `traits`      | Traits mathématiques génériques (`Linear`, `Scalar`, `MatrixOps`) |
| `tests`       | Tests de précision, stabilité et performance |

---

## Exemple d’utilisation

```rust
use vitte_linalg::{Vec3, Matrix4};

fn main() {
    let a = Vec3::new(1.0, 2.0, 3.0);
    let b = Vec3::new(4.0, 5.0, 6.0);

    let dot = a.dot(&b);
    println!("Produit scalaire = {}", dot);

    let m = Matrix4::identity();
    println!("Matrice identité : {:?}", m);
}
```

---

## Intégration

- `vitte-math` : opérations vectorielles et géométriques.  
- `vitte-gpu` : transmission des matrices aux shaders.  
- `vitte-audio` : traitement du signal multi-canal.  
- `vitte-analyzer` : calculs statistiques et transformations.  
- `vitte-runtime` : support des opérations SIMD et multi-thread.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

nalgebra = "0.33"
num-traits = "0.2"
simd = { package = "packed_simd_2", version = "0.3" }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `rayon` pour calcul parallèle, `approx` pour tolérances flottantes.

---

## Tests

```bash
cargo test -p vitte-linalg
```

- Tests unitaires sur vecteurs et matrices.  
- Vérification des décompositions numériques.  
- Tests de stabilité et précision en virgule flottante.  
- Benchmarks SIMD.

---

## Roadmap

- [ ] Support GPU via `vitte-gpu`.  
- [ ] Accélération BLAS/LAPACK.  
- [ ] Détection automatique du backend SIMD.  
- [ ] Calculs symboliques.  
- [ ] Compatibilité avec WebAssembly SIMD.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau