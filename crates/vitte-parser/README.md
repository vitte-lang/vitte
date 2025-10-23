

# vitte-parser

Analyseur **syntaxique (parser)** du compilateur Vitte.  
`vitte-parser` transforme la séquence de tokens produite par `vitte-lexer` en un arbre syntaxique abstrait (AST) riche, typé et navigable, utilisé dans toutes les phases du compilateur et des outils de l’écosystème.

---

## Objectifs

- Fournir un **parser rapide, robuste et extensible**.  
- Gérer la grammaire complète du langage Vitte (déclarations, expressions, modules, macros).  
- Offrir une API stable pour la construction et l’exploration de l’AST.  
- Intégration directe avec `vitte-analyzer`, `vitte-ast` et `vitte-lsp`.  
- Support des erreurs syntaxiques récupérables et messages contextuels détaillés.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `grammar`     | Définition de la grammaire et des règles syntaxiques |
| `builder`     | Construction incrémentale de l’AST |
| `cursor`      | Gestion du flux de tokens et positions |
| `error`       | Diagnostic syntaxique et récupération d’erreurs |
| `expression`  | Analyse des expressions, opérateurs et priorités |
| `declaration` | Analyse des fonctions, types, structs, enums, use, modules |
| `statement`   | Analyse des instructions et blocs de code |
| `recovery`    | Système de synchronisation après erreurs |
| `tests`       | Tests unitaires, snapshots et benchmarks |

---

## Exemple d’utilisation

```rust
use vitte_lexer::Lexer;
use vitte_parser::Parser;

fn main() -> anyhow::Result<()> {
    let src = "fn add(a: i32, b: i32) -> i32 { a + b }";
    let tokens = Lexer::new(src).collect::<Vec<_>>();

    let mut parser = Parser::new(&tokens);
    let ast = parser.parse_module()?;

    println!("Module contient {} éléments", ast.items.len());
    Ok(())
}
```

---

## Exemple d’AST généré

```text
Module {
  items: [
    Fn { name: "add", params: ["a", "b"], body: BinOp(Add, Ident("a"), Ident("b")) }
  ]
}
```

---

## Intégration

- `vitte-lexer` : fournit la séquence de tokens d’entrée.  
- `vitte-ast` : reçoit la structure syntaxique finale.  
- `vitte-analyzer` : exploite l’AST pour les vérifications sémantiques.  
- `vitte-docgen` : documentation et introspection syntaxique.  
- `vitte-lsp` : diagnostics et navigation structurelle.  
- `vitte-cli` : affichage de la structure du code avec `vitte parse`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-lexer = { path = "../vitte-lexer", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }

unicode-ident = "1.0"
unicode-segmentation = "1.11"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
``` 

> Optionnel : `rowan` pour représentation en arbre syntaxique immuable, `criterion` pour benchmarks.

---

## Tests

```bash
cargo test -p vitte-parser
```

- Tests unitaires par type de nœud (expression, statement, item).  
- Tests de parsing d’exemples complets (`core.vitte`, `math.vitte`).  
- Tests de résilience aux erreurs lexicales et syntaxiques.  
- Benchmarks sur grands fichiers sources.

---

## Diagnostic

`vitte-parser` génère des erreurs détaillées avec position, contexte et message explicite :

```text
Erreur: attendu `}` à la ligne 4, trouvé `let`
  --> src/main.vitte:4:5
   |
 4 |     let x = 42
   |     ^^^
   = Note: un bloc doit être fermé avant la fin du fichier
```

---

## Roadmap

- [ ] Support complet des macros procédurales.  
- [ ] Mode incrémental pour `vitte-lsp`.  
- [ ] Export JSON complet de l’AST.  
- [ ] Représentation arbre verte (immutable).  
- [ ] Visualisation graphique via `vitte-inspect`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau