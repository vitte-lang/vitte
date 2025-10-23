

# vitte-regalloc

Moteur **d’allocation de registres (Register Allocation)** pour le compilateur Vitte.  
`vitte-regalloc` gère la distribution optimale des variables temporaires et symboles dans les registres physiques des architectures cibles (x86_64, aarch64, rv64, wasm64).

---

## Objectifs

- Offrir une **implémentation rapide, précise et portable** d’allocation de registres.  
- Supporter plusieurs stratégies : linéaire, graphe de coloration, SSA, et coalescing.  
- Minimiser les accès mémoire et les spill/reload.  
- Intégration directe avec `vitte-codegen`, `vitte-ir`, et `vitte-opt`.  
- Visualisation et débogage via `vitte-inspect` et `vitte-studio`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `graph`       | Construction du graphe d’interférence entre variables |
| `ssa`         | Gestion des versions SSA et renommage |
| `linear`      | Allocation linéaire simple pour builds rapides |
| `coloring`    | Allocation par coloration de graphe |
| `spill`       | Stratégie de spill/reload et heuristiques |
| `analyze`     | Analyse des vivacités et plages de registres |
| `tests`       | Tests de cohérence et benchmarks |

---

## Exemple d’utilisation

```rust
use vitte_regalloc::RegAllocator;
use vitte_ir::FunctionIR;

fn main() -> anyhow::Result<()> {
    let func = FunctionIR::load("examples/add.ir")?;
    let mut alloc = RegAllocator::new();
    let result = alloc.allocate(&func)?;
    println!("{} registres physiques utilisés", result.physical_count);
    Ok(())
}
```

---

## Intégration

- `vitte-codegen` : utilisation directe lors de l’émission d’instructions machine.  
- `vitte-ir` : source principale du graphe de vivacité et des blocs SSA.  
- `vitte-opt` : optimisation post-allocation et fusion de registres.  
- `vitte-analyzer` : validation des plages et interférences.  
- `vitte-inspect` : visualisation du graphe d’allocation.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ir = { path = "../vitte-ir", version = "0.1.0" }

bit-set = "0.5"
petgraph = "0.6"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `dot` pour export du graphe d’interférences, `indexmap` pour allocations déterministes.

---

## Tests

```bash
cargo test -p vitte-regalloc
```

- Tests de vivacité et interférences.  
- Tests de spill minimal et heuristiques.  
- Tests de compatibilité entre architectures.  
- Benchmarks de performance par stratégie d’allocation.

---

## Roadmap

- [ ] Implémentation du mode SSA complet.  
- [ ] Support du coalescing optimisé.  
- [ ] Allocation hiérarchique multi-niveau (thread, SIMD).  
- [ ] Visualisation intégrée dans `vitte-studio`.  
- [ ] Support dynamique des architectures cibles personnalisées.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau