

# vitte-sym

Système **de gestion des symboles, identifiants et résolutions** pour le langage Vitte.  
`vitte-sym` centralise la création, la manipulation et la recherche de symboles utilisés par le compilateur, l’analyseur sémantique et le runtime.

---

## Objectifs

- Offrir une **infrastructure cohérente et rapide** pour la résolution des symboles.  
- Gérer les espaces de noms, les symboles globaux et locaux.  
- Supporter les symboles importés, exportés et internes.  
- Intégration directe avec `vitte-analyzer`, `vitte-compiler`, `vitte-ir` et `vitte-lsp`.  
- Fournir un modèle extensible compatible multi-langage et multi-module.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `symbol`      | Représentation d’un symbole unique (nom, type, portée) |
| `scope`       | Gestion des tables de symboles et des portées lexicales |
| `resolver`    | Résolution et liaison inter-modulaire |
| `index`       | Indexation rapide des symboles pour IDE et LSP |
| `mangle`      | Mécanisme de nommage unique (mangling/demangling) |
| `tests`       | Tests de cohérence, performance et compatibilité |

---

## Exemple d’utilisation

```rust
use vitte_sym::{SymbolTable, Symbol};

fn main() -> anyhow::Result<()> {
    let mut table = SymbolTable::new();
    let sym = Symbol::new("main", "fn", Some("global"));
    table.insert(sym.clone());

    if let Some(found) = table.lookup("main") {
        println!("Symbole trouvé : {:?}", found);
    }
    Ok(())
}
```

---

## Intégration

- `vitte-analyzer` : analyse sémantique et gestion des tables de symboles.  
- `vitte-compiler` : liaison des symboles au moment de la génération IR.  
- `vitte-ir` : représentation des symboles et dépendances dans le code intermédiaire.  
- `vitte-lsp` : navigation, renommage et survol des symboles.  
- `vitte-docgen` : documentation des identifiants et export des signatures.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

indexmap = "2.2"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `dashmap` pour accès concurrent, `blake3` pour hash rapide des symboles.

---

## Tests

```bash
cargo test -p vitte-sym
```

- Tests de résolution locale et globale.  
- Tests de renommage et de collision.  
- Tests d’intégration avec l’analyseur et le compilateur.  
- Benchmarks de performance sur gros graphes de symboles.

---

## Roadmap

- [ ] Indexation incrémentale pour LSP et IDE.  
- [ ] Gestion des symboles dynamiques à l’exécution.  
- [ ] Compression des noms et signatures.  
- [ ] Visualisation du graphe de symboles dans `vitte-studio`.  
- [ ] Export complet JSON/GraphViz pour outils externes.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau