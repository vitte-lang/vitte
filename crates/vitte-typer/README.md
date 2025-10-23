

# vitte-typer

Système **d’inférence, vérification et résolution des types** pour le langage Vitte.  
`vitte-typer` constitue le moteur central d’analyse de types, de propagation des contraintes et de validation des signatures dans le compilateur Vitte.

---

## Objectifs

- Offrir une **inférence de types complète et performante** pour le langage.  
- Supporter les types primitifs, génériques, unions, tuples et références.  
- Intégration directe avec `vitte-analyzer`, `vitte-compiler`, `vitte-ir` et `vitte-lsp`.  
- Détection et résolution automatique des conversions implicites.  
- Garantir la cohérence inter-modulaire et l’inférence incrémentale.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `type`        | Définitions des structures de type (`Ty`, `TyKind`, `TyRef`) |
| `infer`       | Moteur d’inférence et propagation des contraintes |
| `unify`       | Unification et compatibilité entre types |
| `context`     | Gestion des environnements de typage et symboles |
| `error`       | Diagnostic des erreurs de typage et suggestions |
| `tests`       | Vérification de cohérence, performance et couverture |

---

## Exemple d’utilisation

```rust
use vitte_typer::{Typer, TypeContext};

fn main() -> anyhow::Result<()> {
    let mut ctx = TypeContext::default();
    let expr = "a + b";
    let ty = ctx.infer_expr(expr)?;
    println!("Type inféré : {:?}", ty);
    Ok(())
}
```

### Exemple Vitte équivalent

```vitte
fn add(a: int, b: int) -> int {
    return a + b;
}
```

---

## Intégration

- `vitte-analyzer` : résolution sémantique et propagation des types.  
- `vitte-compiler` : génération IR et vérification de cohérence.  
- `vitte-ir` : représentation typée intermédiaire.  
- `vitte-lsp` : survol et diagnostic de type en temps réel.  
- `vitte-docgen` : génération automatique des signatures typées.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }

indexmap = "2.2"
unification = "0.4"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `petgraph` pour graphe de dépendances de types, `insta` pour tests snapshot.

---

## Tests

```bash
cargo test -p vitte-typer
```

- Tests d’inférence sur expressions et fonctions.  
- Tests d’unification et compatibilité de types.  
- Tests de propagation contextuelle.  
- Benchmarks de performance et scalabilité.

---

## Roadmap

- [ ] Support complet des traits et types paramétriques.  
- [ ] Inférence incrémentale multi-module.  
- [ ] Typage structurel et nominal mixte.  
- [ ] Visualisation du graphe d’inférence dans `vitte-studio`.  
- [ ] Export des diagnostics détaillés pour `vitte-lsp`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau