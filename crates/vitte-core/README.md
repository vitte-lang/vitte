

# vitte-core

Noyau **fondamental** de l’écosystème Vitte.  
`vitte-core` définit les types, constantes, traits et abstractions de base partagés entre tous les modules du compilateur, du runtime et des outils.

---

## Objectifs

- Offrir une base stable pour toutes les crates Vitte.  
- Garantir la cohérence des types fondamentaux (`Symbol`, `Span`, `Id`, `Result`, `Error`).  
- Fournir des abstractions communes (traits `Interner`, `ArenaAlloc`, `HashStable`).  
- Assurer la compatibilité cross-platform et `no_std` optionnelle.  
- Servir de point d’ancrage pour les modules dépendants (`vitte-ast`, `vitte-analyzer`, `vitte-runtime`, etc.).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `symbol`      | Gestion des identifiants uniques, interners, hash rapides |
| `span`        | Représentation des positions source et plages de texte |
| `error`       | Système d’erreurs unifié, macros et wrappers `anyhow` |
| `result`      | Alias et types résultat (`CoreResult`, `CoreError`) |
| `arena`       | Allocateurs aréniques et structures à durée contrôlée |
| `traits`      | Traits fondamentaux pour sérialisation, hashing, debug |
| `macros`      | Macros internes : `core_error!`, `span!`, `symbol!` |
| `sync`        | Synchronisation (mutex, atomics, Rc/Arc unifiés) |
| `tests`       | Tests de cohérence et validation inter-crate |

---

## Exemple d’utilisation

```rust
use vitte_core::symbol::Symbol;
use vitte_core::span::Span;

fn main() {
    let name = Symbol::intern("main");
    let span = Span::new(42, 56);

    println!("Symbole: {:?}, Span: {:?}", name, span);
}
```

---

## Intégration

- `vitte-ast` : structure syntaxique et mapping des symboles.  
- `vitte-analyzer` : graphes sémantiques et environnements.  
- `vitte-compiler` : propagation d’erreurs et gestion mémoire.  
- `vitte-runtime` : runtime unifié et cohérent avec les types du core.  
- `vitte-utils` : implémentations auxiliaires des traits et helpers.

---

## Dépendances

```toml
[dependencies]
anyhow = "1"
thiserror = "1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
parking_lot = "0.12"
smallvec = "1.11"
fnv = "1.0"
``` 

> Optionnel : `ahash` pour hashing rapide, `hashbrown` pour tables internes.

---

## Tests

```bash
cargo test -p vitte-core
```

- Tests de stabilité symbolique et interning.  
- Tests de spans et diagnostics.  
- Tests multi-thread de cohérence mémoire.  
- Tests d’intégration entre crates dépendantes.

---

## Roadmap

- [ ] Support complet `no_std`.  
- [ ] Pool global de symboles thread-safe.  
- [ ] Benchmarks mémoire et hashing.  
- [ ] Compatibilité WebAssembly.  
- [ ] Documentation API publique détaillée.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau