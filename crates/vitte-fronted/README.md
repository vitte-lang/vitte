

# vitte-fronted

Front-end **de compilation et d’analyse syntaxique** du langage Vitte.

`vitte-fronted` constitue la première étape de la chaîne de compilation de Vitte. Il prend en charge la lecture, la lexing, la parsing et la construction de l’AST, avant de transmettre la structure au compilateur (`vitte-compiler`) et à l’analyseur (`vitte-analyzer`).

---

## Objectifs

- Lexeur et parseur hautes performances basés sur `vitte-parser`.
- Génération d’AST conforme à la grammaire officielle (`vitte.ebnf`).
- Gestion complète des erreurs de syntaxe et diagnostics contextuels.
- API publique pour intégration avec `vitte-lsp` et `vitte-doc`.
- Compatibilité ascendante avec les futures versions du langage.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `lexer`        | Découpage lexical du code source |
| `parser`       | Construction récursive de l’AST |
| `ast`          | Représentation intermédiaire du code (typed/untyped) |
| `diagnostic`   | Rapports d’erreurs et avertissements |
| `token`        | Définition des types de jetons |
| `visitor`      | Traversée et transformation de l’AST |
| `tests`        | Cas de parsing et snapshots |

---

## Exemple d’utilisation

```rust
use vitte_fronted::Parser;

fn main() -> anyhow::Result<()> {
    let code = r#"fn add(a: Int, b: Int) -> Int { a + b }"#;
    let ast = Parser::parse_source(code)?;
    println!("{:#?}", ast);
    Ok(())
}
```

---

## Diagnostic

Les erreurs de parsing retournent des diagnostics détaillés :

```
error[E1001]: expected identifier after 'fn'
 --> main.vit:1:4
  |
1 | fn  (a, b)
  |    ^ expected identifier
```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-parser = { path = "../vitte-parser", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }
vitte-errors = { path = "../vitte-errors", version = "0.1.0" }

anyhow = "1"
colored = "2"
serde = { version = "1.0", features = ["derive"] }
```

---

## Tests

```bash
cargo test -p vitte-fronted
```

- Vérification de la conformité EBNF.
- Tests de lexing et parsing sur projets complets.
- Snapshots de diagnostics et reconstruction d’AST.

---

## Roadmap

- [ ] Support des macros et annotations.
- [ ] Parsing incrémental pour IDE.
- [ ] Mode tolerant parsing (incomplétude autorisée).
- [ ] Intégration directe avec `vitte-analyzer`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau