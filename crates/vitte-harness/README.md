


# vitte-harness

Cadre **d’exécution et de test unifié** pour l’écosystème Vitte.  
`vitte-harness` gère l’initialisation, l’exécution et la capture des résultats des tests unitaires, d’intégration et de performance.

---

## Objectifs

- Offrir un **framework de test standardisé** pour toutes les crates Vitte.  
- Support des tests unitaires, d’intégration, de fuzzing et benchmarks.  
- Fournir une API stable compatible `cargo test` et `vitte test`.  
- Permettre la capture structurée des logs, diagnostics et timings.  
- Supporter la parallélisation, la coloration ANSI et le mode silencieux.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `runner`      | Gestion de l’exécution des tests (planification, threads) |
| `capture`     | Collecte des sorties standard et erreurs |
| `report`      | Formatage et rendu des résultats (CLI, JSON, HTML) |
| `filter`      | Sélection des tests par nom, tag ou pattern |
| `bench`       | Exécution des benchmarks chronométrés |
| `fuzz`        | Génération aléatoire d’entrées pour tests de robustesse |
| `assert`      | Macros d’assertions enrichies |
| `tests`       | Auto-tests du framework et validations de sortie |

---

## Exemple d’utilisation

```rust
use vitte_harness::test;

#[test]
fn addition() {
    let a = 2 + 2;
    assert_eq!(a, 4);
}

#[bench]
fn bench_addition(b: &mut vitte_harness::Bencher) {
    b.iter(|| (1..1000).sum::<u64>());
}
```

---

## Intégration

- `vitte-cli` : sous-commande `vitte test`.  
- `vitte-build` : intégration du mode test dans les builds CI.  
- `vitte-analyzer` : analyse des résultats et logs.  
- `vitte-cov` : instrumentation et couverture.  
- `vitte-docgen` : génération automatique de rapports.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ansi = { path = "../vitte-ansi", version = "0.1.0" }

criterion = { version = "0.5", optional = true }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
rayon = "1"
``` 

> Optionnel : `tokio` pour exécution asynchrone, `indicatif` pour barres de progression.

---

## Tests

```bash
cargo test -p vitte-harness
```

- Tests de compatibilité avec `cargo test`.  
- Tests de capture de sortie et filtrage.  
- Benchmarks de performance.  
- Tests d’intégration avec `vitte-cov`.

---

## Roadmap

- [ ] Support complet des tests asynchrones.  
- [ ] Export HTML interactif des rapports.  
- [ ] Fuzzing distribué multi-core.  
- [ ] Intégration GitHub Actions (résumés automatiques).  
- [ ] Mode JIT test pour exécution à chaud.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau