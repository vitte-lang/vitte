

# vitte-gc

Moteur **de ramasse-miettes (Garbage Collector)** pour l’écosystème Vitte.

`vitte-gc` implémente la gestion automatique de la mémoire pour les programmes Vitte. Il fournit un collecteur hybride (générationnel + incrémental) optimisé pour les environnements à faible latence et s’intègre profondément dans `vitte-runtime` et `vitte-async`.

---

## Objectifs

- Gestion mémoire sécurisée et performante.
- Collecte générationnelle avec seuils adaptatifs.
- Support du mode concurrent/incrémental.
- Intégration directe avec le runtime et les coroutines.
- API de diagnostics et profilage mémoire.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `heap`         | Gestion du tas, allocations et libérations |
| `object`       | Structures d’objets Vitte (Obj, Handle, WeakRef) |
| `mark`         | Phase de marquage et exploration du graphe |
| `sweep`        | Libération des objets non marqués |
| `arena`        | Allocation groupée et optimisation cache |
| `stats`        | Statistiques d’utilisation mémoire |
| `trace`        | Interface de traçage pour objets utilisateur |
| `tests`        | Tests de stress et mesures de latence |

---

## Exemple d’utilisation

```rust
use vitte_gc::{Gc, GcCell};

fn main() {
    let a = Gc::new(42);
    let b = Gc::new(7);
    let sum = *a + *b;
    println!("Somme: {sum}");
}
```

---

## Modes de collecte

- **Stop-the-world** : simple et déterministe, utilisé pour le debug.
- **Incrémental** : collecte par étapes pendant l’exécution.
- **Concurrent** : exécution parallèle sur un thread dédié.
- **Générationnel** : séparation des objets jeunes et matures.

---

## Intégration

- `vitte-runtime` : allocation d’objets dynamiques.
- `vitte-async` : suivi mémoire des coroutines.
- `vitte-analyzer` : profilage et statistiques GC.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-async = { path = "../vitte-async", version = "0.1.0" }
parking_lot = "0.12"
crossbeam = "0.9"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-gc
```

- Tests de stress (allocations massives).
- Vérification du comportement concurrent.
- Mesures de latence et fragmentation.

---

## Roadmap

- [ ] Compactage mémoire à la volée.
- [ ] Collecte adaptative pilotée par heuristiques.
- [ ] Visualiseur graphique du tas.
- [ ] Export de métriques Prometheus.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau