

# vitte-tree

Bibliothèque **d’arbres syntaxiques, sémantiques et hiérarchiques** pour le langage Vitte.  
`vitte-tree` fournit une représentation unifiée des structures arborescentes utilisées dans le compilateur, l’analyseur, le runtime et les outils de documentation.

---

## Objectifs

- Offrir une **implémentation robuste et générique d’arbres syntaxiques et logiques**.  
- Supporter les AST (Abstract Syntax Trees), HIR (High-Level IR) et symbol trees.  
- Intégration directe avec `vitte-ast`, `vitte-analyzer`, `vitte-compiler` et `vitte-docgen`.  
- Fournir une API ergonomique pour parcourir, modifier et sérialiser les arbres.  
- Garantir la compatibilité intermodule et la cohérence entre passes de compilation.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `node`        | Représentation d’un nœud (type, enfant, métadonnées) |
| `builder`     | Construction et transformation des arbres |
| `visitor`     | Parcours, filtrage et transformation fonctionnelle |
| `serialize`   | Sauvegarde et chargement des arbres en JSON/CBOR |
| `merge`       | Fusion et comparaison d’arbres entre versions |
| `tests`       | Validation structurelle et de performance |

---

## Exemple d’utilisation

```rust
use vitte_tree::{Node, TreeBuilder};

fn main() -> anyhow::Result<()> {
    let mut builder = TreeBuilder::new();
    let root = builder.root("Module");
    builder.child(root, "Function", Some("add"));
    builder.child(root, "Function", Some("sub"));

    let tree = builder.build();
    println!("{}", tree.to_json_pretty()?);
    Ok(())
}
```

### Sortie JSON simplifiée

```json
{
  "type": "Module",
  "children": [
    { "type": "Function", "name": "add" },
    { "type": "Function", "name": "sub" }
  ]
}
```

---

## Intégration

- `vitte-ast` : base pour la représentation des arbres syntaxiques.  
- `vitte-analyzer` : génération et manipulation des arbres sémantiques.  
- `vitte-compiler` : conversion vers l’IR et vérification des dépendances.  
- `vitte-docgen` : visualisation des structures d’arbre dans la documentation.  
- `vitte-studio` : exploration interactive des arbres et symboles.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
serde_cbor = "0.11"
indexmap = "2.2"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `petgraph` pour représentation graphique, `uuid` pour identification des nœuds, `rayon` pour parallélisation du parcours.

---

## Tests

```bash
cargo test -p vitte-tree
```

- Tests de structure et de sérialisation.  
- Tests de fusion et comparaison d’arbres.  
- Tests de performance sur gros AST.  
- Vérification de cohérence avec `vitte-analyzer` et `vitte-compiler`.

---

## Roadmap

- [ ] Support de visualisation graphique (GraphViz, HTML).  
- [ ] Compression adaptative des grands arbres.  
- [ ] Synchronisation incrémentale avec `vitte-lsp`.  
- [ ] Système de diff interactif dans `vitte-studio`.  
- [ ] Export binaire compact pour IR.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau