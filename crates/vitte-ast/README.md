# vitte-ast

Représentation **abstraite du code source (AST)** du langage Vitte.

`vitte-ast` définit les structures, nœuds et visiteurs représentant la syntaxe interne du langage Vitte.  
C’est un composant central du compilateur (`vitte-compiler`), de l’analyseur (`vitte-analyzer`), du formateur (`vitte-fmt`) et du générateur de documentation (`vitte-docgen`).

---

## Objectifs

- Représenter fidèlement la grammaire officielle (`vitte.ebnf`).
- Fournir une structure sérialisable, stable et lisible.
- Offrir des visiteurs et transformations pour le compilateur.
- Supporter la génération, l’inspection et la manipulation du code.
- Permettre l’export JSON pour outils IDE et docgen.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `node`         | Définition des nœuds syntaxiques (`Fn`, `Struct`, `Expr`, `Stmt`, etc.) |
| `span`         | Positions dans le code source (fichier, ligne, colonne) |
| `token`        | Liens vers les lexèmes d’origine |
| `visitor`      | Traversée et transformation de l’AST |
| `builder`      | Construction programmatique de nœuds |
| `display`      | Conversion AST → texte source |
| `serde`        | (optionnel) Sérialisation et désérialisation |
| `tests`        | Snapshots, parsers et reconstruction |

---

## Exemple d’utilisation

```rust
use vitte_ast::{AstNode, FunctionDecl, Expr};

fn main() {
    let fn_add = FunctionDecl {
        name: "add".into(),
        params: vec!["a".into(), "b".into()],
        body: Expr::Binary("+".into(), Box::new(Expr::Ident("a".into())), Box::new(Expr::Ident("b".into()))),
        span: Default::default(),
    };

    println!("{:#?}", fn_add);
}
```

---

## Intégration

- `vitte-parser` : construit l’AST depuis le code source.
- `vitte-analyzer` : réalise la vérification sémantique.
- `vitte-compiler` : génère le bytecode ou LLVM IR.
- `vitte-fmt` : reformate le code à partir de l’AST.
- `vitte-docgen` : extrait la documentation structurée.

---

## Dépendances

```toml
[dependencies]
vitte-core  = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
serde = { version = "1.0", features = ["derive"], optional = true }
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-ast
```

- Tests de construction et de parcours d’AST.
- Vérification de la cohérence des spans.
- Snapshots des impressions formatées.
- Validation des conversions JSON.

---

## Roadmap

- [ ] Support des macros et annotations.
- [ ] Validation structurelle complète (visitor sémantique).
- [ ] Diff AST entre versions de code.
- [ ] Visualisation graphique (Graphviz/HTML).

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau