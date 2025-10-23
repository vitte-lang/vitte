

# vitte-hir

Représentation intermédiaire **haute-niveau (HIR)** du compilateur Vitte.  
`vitte-hir` sert de pont entre l’AST (analyse syntaxique) et le MIR (optimisation et génération de code), en conservant les informations sémantiques et structurelles du programme.

---

## Objectifs

- Fournir une représentation **sémantiquement riche** et typée.  
- Simplifier la résolution et l’analyse des symboles.  
- Conserver la hiérarchie des modules, fonctions et types.  
- Permettre la transformation efficace vers MIR.  
- Supporter la documentation, le linting et la génération de diagnostics.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `context`     | Contexte global HIR : symboles, types, environnement |
| `item`        | Représentation des entités globales (fonctions, structs, enums, traits) |
| `expr`        | Expressions et opérations sémantiques |
| `stmt`        | Instructions structurées (let, loop, if, return) |
| `types`       | Gestion du typage et des inférences |
| `resolver`    | Résolution des symboles et dépendances inter-modules |
| `builder`     | Construction du HIR depuis l’AST |
| `lower`       | Conversion du HIR vers MIR |
| `tests`       | Tests de cohérence, typage et transformation |

---

## Exemple d’utilisation

```rust
use vitte_hir::Context;
use vitte_ast::Ast;

fn main() -> anyhow::Result<()> {
    let ast = Ast::parse_file("main.vitte")?;
    let mut ctx = Context::default();

    let hir = ctx.lower_from_ast(&ast)?;
    println!("HIR généré avec {} fonctions.", hir.functions().len());

    Ok(())
}
```

---

## Intégration

- `vitte-ast` : source directe du HIR.  
- `vitte-analyzer` : validation sémantique et typage.  
- `vitte-compiler` : conversion MIR et génération de code.  
- `vitte-clippy` : lint statique basé sur le HIR.  
- `vitte-docgen` : extraction des signatures pour documentation.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
``` 

> Optionnel : `petgraph` pour graphe de dépendances, `rayon` pour typage parallèle.

---

## Tests

```bash
cargo test -p vitte-hir
```

- Tests de transformation AST → HIR.  
- Tests de typage et de cohérence.  
- Tests d’intégration avec `vitte-analyzer`.  
- Benchmarks de performance sur gros modules.

---

## Roadmap

- [ ] Typage incrémental.  
- [ ] Support des traits et impls génériques.  
- [ ] Vérification des lifetimes et des régions mémoire.  
- [ ] Émission de diagnostics enrichis.  
- [ ] Export visuel du graphe HIR pour analyse statique.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau