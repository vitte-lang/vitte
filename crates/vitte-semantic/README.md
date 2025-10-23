

# vitte-semantic

Moteur **d’analyse sémantique** du langage Vitte.  
`vitte-semantic` est responsable de la validation sémantique des programmes Vitte, de la résolution des symboles, du typage, et de la gestion des portées et dépendances entre modules.

---

## Objectifs

- Offrir une **analyse sémantique rapide et précise**.  
- Gérer les symboles, types, modules et espaces de noms.  
- Détecter les erreurs de typage, références circulaires et incohérences.  
- Intégration directe avec `vitte-ast`, `vitte-analyzer` et `vitte-compiler`.  
- Fournir une base pour les optimisations et la génération de code.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `scope`       | Gestion des environnements et des portées lexicales |
| `symbol`      | Table des symboles et résolution des identifiants |
| `typecheck`   | Vérification des types, inférence et coercition |
| `module`      | Gestion des dépendances inter-modulaires |
| `diagnostic`  | Emission d’erreurs et avertissements sémantiques |
| `visitor`     | Parcours des arbres AST pour validation et liaison |
| `tests`       | Validation de cohérence et couverture des règles |

---

## Exemple d’utilisation

```rust
use vitte_semantic::SemanticAnalyzer;
use vitte_ast::Ast;

fn main() -> anyhow::Result<()> {
    let ast = Ast::parse_file("examples/demo.vitte")?;
    let mut analyzer = SemanticAnalyzer::new();
    analyzer.check(&ast)?;
    println!("Analyse sémantique réussie.");
    Ok(())
}
```

---

## Intégration

- `vitte-ast` : fournit les structures syntaxiques analysées.  
- `vitte-analyzer` : moteur combiné de validation et résolution.  
- `vitte-compiler` : validation préalable avant IR et codegen.  
- `vitte-lsp` : diagnostics sémantiques en temps réel.  
- `vitte-docgen` : inférence des signatures et types documentés.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }

indexmap = "2.2"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `petgraph` pour analyse de dépendances, `tracing` pour profiling des passes.

---

## Tests

```bash
cargo test -p vitte-semantic
```

- Tests de typage et inférence.  
- Tests de détection de doublons et symboles non résolus.  
- Tests de compatibilité inter-modules.  
- Benchmarks d’analyse sur gros projets.

---

## Roadmap

- [ ] Inférence de type complète avec contraintes polymorphes.  
- [ ] Vérification des lifetimes et emprunts.  
- [ ] Validation des macros et générics.  
- [ ] Support du typage structurel.  
- [ ] Intégration des règles de lint sémantique dans `vitte-clippy`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau