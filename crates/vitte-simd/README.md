

# vitte-simd

Bibliothèque **de calcul vectoriel et parallélisme matériel (SIMD)** pour le langage Vitte.  
`vitte-simd` fournit des primitives, abstractions et optimisations bas niveau permettant d’exploiter les instructions vectorielles CPU (SSE, AVX, NEON, RVV) dans le compilateur et le runtime Vitte.

---

## Objectifs

- Offrir une **API unifiée et portable pour le SIMD**.  
- Supporter les architectures x86_64 (SSE2, AVX2, AVX-512), aarch64 (NEON) et rv64 (RVV).  
- Fournir des fonctions mathématiques vectorisées hautes performances.  
- Intégration directe avec `vitte-math`, `vitte-codegen` et `vitte-optimizer`.  
- Génération automatique d’instructions SIMD via analyse IR et autovectorisation.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `intrinsics`  | Abstractions sur les instructions natives (SSE, AVX, NEON) |
| `vector`      | Types et opérations génériques sur vecteurs (`Vec2`, `Vec4`, `Vec8`) |
| `matrix`      | Calculs matriciels et transformations vectorisées |
| `dispatch`    | Sélection dynamique d’instructions selon CPU |
| `analyze`     | Détection des boucles et patterns vectorisables |
| `bench`       | Tests de performance et comparaison entre architectures |
| `tests`       | Vérification de précision et cohérence SIMD vs scalaire |

---

## Exemple d’utilisation

```rust
use vitte_simd::vector::Vec4f32;

fn main() {
    let a = Vec4f32::new(1.0, 2.0, 3.0, 4.0);
    let b = Vec4f32::splat(2.0);
    let c = a * b + Vec4f32::splat(1.0);
    println!("Résultat : {:?}", c);
}
```

---

## Intégration

- `vitte-math` : opérations vectorielles et trigonométriques.  
- `vitte-codegen` : génération automatique d’instructions SIMD IR.  
- `vitte-runtime` : exécution des primitives SIMD optimisées.  
- `vitte-profiler` : mesure de la vitesse et vectorisation effective.  
- `vitte-studio` : visualisation des pipelines vectoriels et performances.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

wide = "0.7"
simd = "0.2"
rayon = "1.10"
num-traits = "0.2"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `packed_simd_2` pour compatibilité expérimentale, `criterion` pour benchmarks.

---

## Tests

```bash
cargo test -p vitte-simd
```

- Tests de précision entre exécutions SIMD et scalaires.  
- Tests de compatibilité inter-architectures.  
- Benchmarks sur multiplications et sommes vectorisées.  
- Vérification du fallback scalaire en absence d’instructions SIMD.

---

## Roadmap

- [ ] Support complet AVX-512 et RVV.  
- [ ] Autovectorisation au niveau IR.  
- [ ] Compilation conditionnelle dynamique selon CPU.  
- [ ] Visualisation des performances dans `vitte-studio`.  
- [ ] Intégration dans `vitte-bench` et `vitte-opt`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau