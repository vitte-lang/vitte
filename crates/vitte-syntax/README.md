

# vitte-syntax

Bibliothèque **de parsing, analyse lexicale et syntaxique** pour le langage Vitte.  
`vitte-syntax` gère la tokenisation, la grammaire, la construction d’arbres syntaxiques (AST) et la validation structurelle du code source Vitte, selon la grammaire officielle `.ebnf`.

---

## Objectifs

- Offrir une **implémentation complète et performante du parseur Vitte**.  
- Fournir un lexer, un parser et un builder d’AST cohérents et modulaires.  
- Respecter strictement la grammaire EBNF officielle de Vitte.  
- Intégration directe avec `vitte-ast`, `vitte-analyzer`, `vitte-compiler` et `vitte-lsp`.  
- Support du mode tolérant pour l’autocomplétion et la coloration syntaxique.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `lexer`       | Analyse lexicale, génération de tokens à partir du texte source |
| `token`       | Types de jetons (`Identifier`, `Keyword`, `Literal`, `Symbol`, etc.) |
| `parser`      | Analyse syntaxique descendante récursive, construction des nœuds AST |
| `grammar`     | Grammaire formelle conforme à la spécification `.ebnf` |
| `builder`     | Construction et vérification de l’AST intermédiaire |
| `error`       | Gestion et récupération d’erreurs syntaxiques |
| `tests`       | Validation de la conformité, couverture EBNF et tolérance d’erreurs |

---

## Exemple d’utilisation

### Exemple Rust : parsing d’un fichier source

```rust
use vitte_syntax::{Parser, SourceFile};

fn main() -> anyhow::Result<()> {
    let src = r#"
        module example;
        fn add(a: int, b: int) -> int { a + b }
    "#;

    let file = SourceFile::new("example.vitte", src);
    let mut parser = Parser::new(&file);
    let ast = parser.parse()?;

    println!("Arbre syntaxique : {:?}", ast);
    Ok(())
}
```

### Exemple Vitte équivalent

```vitte
module example;
fn add(a: int, b: int) -> int {
    return a + b;
}
```

---

## Intégration

- `vitte-ast` : génération et validation des nœuds syntaxiques.  
- `vitte-analyzer` : base pour la résolution sémantique et les types.  
- `vitte-compiler` : transformation de l’AST vers l’IR.  
- `vitte-lsp` : parsing incrémental pour l’autocomplétion et diagnostics.  
- `vitte-docgen` : extraction des signatures et symboles pour documentation.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }

logos = "0.14"
rowan = "0.16"
unicode-ident = "1.0"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `tree-sitter` pour parsing expérimental, `insta` pour tests snapshot.

---

## Tests

```bash
cargo test -p vitte-syntax
```

- Tests de conformité à la grammaire officielle `.ebnf`.  
- Tests de résilience aux erreurs de syntaxe.  
- Tests de performance sur gros fichiers source.  
- Tests de compatibilité inter-version avec `vitte-analyzer` et `vitte-lsp`.

---

## Roadmap

- [ ] Parsing incrémental pour IDE et LSP.  
- [ ] Amélioration du mode tolérant pour code incomplet.  
- [ ] Support des annotations et attributs AST.  
- [ ] Intégration de `tree-sitter` pour inspection interactive.  
- [ ] Export du parse tree pour `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau