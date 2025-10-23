

# vitte-grammar

Grammaire **officielle du langage Vitte**, exprimée en EBNF ISO 14977.  
`vitte-grammar` définit la syntaxe formelle utilisée par le parser, les outils d’analyse et la documentation du langage.

---

## Objectifs

- Définir la **grammaire canonique** du langage Vitte (style Rust‑like).  
- Servir de référence unique pour les parseurs (`vitte-parser`, `vitte-analyzer`, `vitte-lsp`).  
- Générer automatiquement les tables de parsing et l’AST initial.  
- Fournir des exports en formats **EBNF**, **JSON**, **YAML**, et **HTML**.  
- Maintenir la compatibilité avec les exemples publics DSU, Graph, Search, WAV, ANSI.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `lexer`       | Définitions lexicales (mots‑clés, identifiants, littéraux) |
| `tokens`      | Énumération des symboles terminaux |
| `rules`       | Règles syntaxiques principales (expr, stmt, type, fn, module) |
| `builder`     | Générateur EBNF → structure Rust sérialisable |
| `export`      | Export vers formats EBNF, JSON et HTML |
| `verify`      | Validation et détection des ambiguïtés |
| `tests`       | Tests de cohérence et de round‑trip sur exemples Vitte |

---

## Exemple d’utilisation

```rust
use vitte_grammar::Grammar;

fn main() -> anyhow::Result<()> {
    let grammar = Grammar::load_from("src/vitte.ebnf")?;
    grammar.verify()?;

    grammar.export_json("target/grammar.json")?;
    grammar.export_html("target/grammar.html")?;

    println!("Grammaire Vitte vérifiée et exportée.");
    Ok(())
}
```

---

## Exemple de règle EBNF

```ebnf
FnItem = "fn" identifier [ GenericParams ] "(" [ ParamList ] ")" [ "->" Type ] Block ;
```

---

## Intégration

- `vitte-parser` : génération automatique des tables LR.  
- `vitte-analyzer` : correspondance AST ↔ grammaire.  
- `vitte-docgen` : inclusion du HTML EBNF dans la documentation.  
- `vitte-lsp` : vérification de la validité syntaxique en temps réel.  
- `vitte-core` : définitions partagées des tokens et spans.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
yaml-rust = "0.4"
regex = "1"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `pest`, `chumsky`, ou `lalrpop-util` pour tests croisés.

---

## Tests

```bash
cargo test -p vitte-grammar
```

- Tests de round‑trip EBNF → JSON → EBNF.  
- Tests sur les exemples DSU, Graph et Search.  
- Vérification des ambiguïtés et conflits LR(1).  
- Export complet des versions EBNF et HTML.

---

## Roadmap

- [ ] Support PEG (Parsing Expression Grammar).  
- [ ] Visualisation interactive de la grammaire (Graphviz).  
- [ ] Génération automatique de parseurs Rust et C.  
- [ ] Validation en CI des changements syntaxiques.  
- [ ] Export complet en Markdown pour vitte-book.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau