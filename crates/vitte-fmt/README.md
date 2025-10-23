

# vitte-fmt

Outil **de formatage de code et normalisation syntaxique** pour le langage Vitte.

`vitte-fmt` formate automatiquement le code source Vitte selon les conventions officielles (EBNF, indentation, alignement, blocs, commentaires). Il garantit une cohérence de style dans tout le codebase et s’intègre avec `vitte-lsp`, `vitte-analyzer` et `vitte-cli`.

---

## Objectifs

- Formatage stable, déterministe et reproductible.
- Respect strict de la grammaire officielle (`vitte.ebnf`).
- Gestion des blocs, fonctions, types, et modules.
- Reformatage automatique des imports et des expressions.
- Mode *diff*, *check*, et *fix*.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `parser`       | Parsing du code source et AST intermédiaire |
| `formatter`    | Règles de formatage et génération du texte |
| `rules`        | Ensemble de règles configurables |
| `diff`         | Comparaison avant/après pour mode check |
| `config`       | Chargement du fichier `.vittefmt.toml` |
| `cli`          | Interface en ligne de commande |
| `tests`        | Tests de stabilité et snapshots |

---

## Exemple d’utilisation

```bash
vitte fmt main.vit
vitte fmt src/ --check
vitte fmt --diff
```

Avant :
```vitte
fn  add(a,b){return a+b;}
```

Après :
```vitte
fn add(a: Int, b: Int) -> Int {
    return a + b;
}
```

---

## Fichier de configuration `.vittefmt.toml`

```toml
indent_width = 4
max_line_length = 100
space_around_operators = true
align_comments = true
newline_after_brace = true
group_imports = "crate"
```

---

## Intégration

- `vitte-lsp` : formatage à la volée dans les éditeurs.
- `vitte-analyzer` : vérification syntaxique et structurelle.
- `vitte-docgen` : alignement et beautify automatique avant génération.

---

## Dépendances

```toml
[dependencies]
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }
vitte-parser = { path = "../vitte-parser", version = "0.1.0" }
vitte-core = { path = "../vitte-core", version = "0.1.0" }
difference = "2.0"
serde = { version = "1.0", features = ["derive"] }
toml = "0.8"
anyhow = "1"
colored = "2"
```

---

## Tests

```bash
cargo test -p vitte-fmt
```

- Tests de stabilité de formatage (idempotence).
- Snapshots de blocs complexes.
- Tests de performance sur grands fichiers.

---

## Roadmap

- [ ] Formatage partiel basé sur l’AST incrémental.
- [ ] Mode interactif (stdin/stdout).
- [ ] Extension VS Code intégrée.
- [ ] Auto-fix des imports et alias inutilisés.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau