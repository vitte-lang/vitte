

# vitte-lexer

Analyseur **lexical** du compilateur Vitte.  
`vitte-lexer` découpe le code source Vitte en une séquence de **tokens** typés utilisés par le parseur (`vitte-parser`) et les outils d’analyse (`vitte-analyzer`, `vitte-lsp`).

---

## Objectifs

- Fournir un **lexeur rapide, précis et extensible** pour le langage Vitte.  
- Gérer les littéraux, opérateurs, symboles, identifiants et mots-clés.  
- Supporter les modes multilingues (Unicode, UTF-8 strict).  
- Intégration directe avec l’AST, la coloration syntaxique et la LSP.  
- Offrir une API stable pour l’analyse et la génération de diagnostics.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `token`       | Définition des types de tokens et catégories lexicales |
| `cursor`      | Gestion du pointeur sur le flux source (UTF-8 safe) |
| `scanner`     | Lecture séquentielle et segmentation du texte |
| `rules`       | Règles lexicales (identifiants, nombres, chaînes, opérateurs) |
| `error`       | Diagnostic des erreurs lexicales |
| `tests`       | Cas de test complets et benchmarks |

---

## Exemple d’utilisation

```rust
use vitte_lexer::{Lexer, TokenKind};

fn main() {
    let src = "fn main() { let x = 42; }";
    let lexer = Lexer::new(src);

    for token in lexer {
        println!("{:?}", token.kind);
    }
}
```

---

## Intégration

- `vitte-parser` : consomme les tokens pour construire l’AST.  
- `vitte-analyzer` : utilise les positions lexicales pour diagnostics.  
- `vitte-lsp` : coloration syntaxique et navigation symbolique.  
- `vitte-docgen` : documentation du langage à partir des tokens.  
- `vitte-formatter` : indentation et reformattage lexical.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

unicode-ident = "1.0"
unicode-segmentation = "1.11"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
``` 

> Optionnel : `logos` pour génération auto de règles, `criterion` pour benchmarks.

---

## Tests

```bash
cargo test -p vitte-lexer
```

- Tests de reconnaissance de tokens.  
- Tests de gestion des erreurs et récupération.  
- Tests de performance sur grands fichiers.  
- Benchmarks comparatifs (`cargo bench`).

---

## Roadmap

- [ ] Support complet des littéraux multi-lignes et chaînes brutes.  
- [ ] Repli automatique sur flux incrémental.  
- [ ] Lexing partiel pour LSP (fenêtrage).  
- [ ] Export des diagnostics JSON.  
- [ ] Génération automatique des tables lexicales.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau