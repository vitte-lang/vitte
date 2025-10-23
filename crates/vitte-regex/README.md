

# vitte-regex

Moteur **d’expressions régulières** pour le langage Vitte.  
`vitte-regex` implémente un moteur rapide, sûr et extensible pour la recherche, le filtrage, la substitution et l’analyse de texte dans les outils du compilateur et l’environnement d’exécution.

---

## Objectifs

- Offrir un **moteur d’expressions régulières performant et portable**.  
- Supporter les syntaxes classiques (Perl, POSIX, Rust) et les extensions Vitte.  
- Permettre l’utilisation dans `vitte-analyzer`, `vitte-docgen` et `vitte-cli`.  
- Intégration complète avec `vitte-lexer`, `vitte-lsp` et `vitte-utils`.  
- Fournir des API bas-niveau et haut-niveau pour la manipulation des motifs.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `parse`       | Analyse syntaxique des motifs et compilation en automates |
| `nfa`         | Génération d’automates non-déterministes |
| `dfa`         | Conversion et exécution déterministe |
| `engine`      | Évaluation rapide des motifs sur flux texte |
| `replace`     | Substitution et capture de groupes |
| `unicode`     | Support des classes Unicode et des correspondances étendues |
| `tests`       | Vérification de conformité et performance |

---

## Exemple d’utilisation

```rust
use vitte_regex::Regex;

fn main() -> anyhow::Result<()> {
    let re = Regex::new(r"^[A-Z][a-z]+$")?;
    println!("{}", re.is_match("Vitte")); // true
    Ok(())
}
```

### Substitution

```rust
let re = Regex::new(r"foo").unwrap();
let result = re.replace_all("foo bar foo", "baz");
assert_eq!(result, "baz bar baz");
```

---

## Intégration

- `vitte-lexer` : reconnaissance lexicale et filtrage symbolique.  
- `vitte-analyzer` : vérification des identifiants et motifs de noms.  
- `vitte-docgen` : recherche textuelle et génération de liens.  
- `vitte-cli` : filtrage d’arguments et logs via `grep` interne.  
- `vitte-lsp` : surlignage des motifs dans les diagnostics.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

regex-automata = "0.4"
regex-syntax = "0.8"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `aho-corasick` pour recherche multi-motifs, `memchr` pour accélération bas-niveau.

---

## Tests

```bash
cargo test -p vitte-regex
```

- Tests de correspondance simple et groupes capturants.  
- Tests de performance sur gros volumes de texte.  
- Tests de conformité Unicode et classes spéciales.  
- Vérification de compatibilité avec les expressions Rust/POSIX.

---

## Roadmap

- [ ] Implémentation complète du moteur DFA parallèle.  
- [ ] Support des lookahead/lookbehind et backreferences.  
- [ ] Compilation JIT des motifs fréquemment utilisés.  
- [ ] Optimisation vectorisée (SIMD).  
- [ ] Visualisation interactive dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau