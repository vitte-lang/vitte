

# vitte-collections

Bibliothèque **de structures de données et collections** pour le langage Vitte.  
`vitte-collections` implémente les conteneurs fondamentaux, itérateurs, tables de hachage et structures persistantes utilisées dans l’écosystème Vitte.

---

## Objectifs

- Offrir des collections performantes, sûres et idiomatiques.  
- Support complet du **paradigme fonctionnel** et de la **mutabilité contrôlée**.  
- Garantir la compatibilité entre les versions du runtime et du compilateur.  
- Fournir des implémentations génériques adaptées aux besoins du compilateur (`vitte-compiler`, `vitte-analyzer`, etc.).  
- Inclure des variantes **persistantes** et **thread-safe**.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `vec`         | Vecteurs dynamiques (`Vec`, `SmallVec`, `ArenaVec`) |
| `map`         | Tables de hachage (`HashMap`, `FxHashMap`, `IndexMap`) |
| `set`         | Ensembles (`HashSet`, `BTreeSet`, `BitSet`) |
| `list`        | Listes chaînées et structures persistantes |
| `arena`       | Allocateurs aréniques et références stables |
| `deque`       | Files doublement terminées |
| `graph`       | Structures orientées/non orientées (adjacency list/matrix) |
| `queue`       | Files FIFO et priority queues |
| `stack`       | Piles LIFO et variantes thread-safe |
| `tests`       | Tests de cohérence, performances et implémentations alternatives |

---

## Exemple d’utilisation

```rust
use vitte_collections::map::HashMap;

fn main() {
    let mut map = HashMap::new();
    map.insert("a", 1);
    map.insert("b", 2);

    if let Some(v) = map.get("a") {
        println!("clé a → {v}");
    }
}
```

---

## Intégration

- `vitte-core` : dépendance de base pour les types génériques.  
- `vitte-analyzer` : stockage de symboles et environnements.  
- `vitte-compiler` : arènes et graphes de dépendances.  
- `vitte-runtime` : structures optimisées pour exécution.  
- `vitte-utils` : helpers pour conversions et itérateurs custom.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

hashbrown = "0.14"
indexmap = "2.2"
smallvec = "1.11"
crossbeam = "0.8"
parking_lot = "0.12"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
``` 

> Optionnel : `rayon` pour itérateurs parallèles, `ahash` pour hash ultra rapide.

---

## Tests

```bash
cargo test -p vitte-collections
```

- Tests de performance sur grands ensembles.  
- Vérification de la stabilité et de la cohérence mémoire.  
- Comparaisons avec `std::collections` (Rust).  
- Tests multi-thread et stress sur arènes persistantes.

---

## Roadmap

- [ ] Structures immuables persistantes (HAMT).  
- [ ] Intégration complète avec `vitte-analyzer` (symbol tables).  
- [ ] Benchmarks automatiques via `criterion`.  
- [ ] Allocateurs spécialisés pour `no_std`.  
- [ ] Support des itérateurs adaptatifs (lazy sequences).

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau