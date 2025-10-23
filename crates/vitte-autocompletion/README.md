

# vitte-autocompletion

Moteur **d’autocomplétion contextuelle** pour le langage Vitte.

`vitte-autocompletion` fournit les suggestions intelligentes utilisées par l’environnement Vitte (CLI, LSP, IDE).  
Il s’intègre directement avec `vitte-analyzer` et `vitte-lsp` pour proposer des complétions de symboles, mots-clés, types, chemins de modules et signatures de fonctions en temps réel.

---

## Objectifs

- Fournir des complétions précises, rapides et contextuelles.  
- Support complet du langage (fonctions, types, constantes, imports, traits).  
- Moteur de suggestions multi-source (projet, dépendances, standard).  
- API publique pour `vitte-lsp` et les éditeurs tiers.  
- Mécanisme de scoring et de tri dynamique.  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `context`      | Analyse du curseur et de la portée courante |
| `resolver`     | Résolution des symboles visibles |
| `engine`       | Génération et filtrage des suggestions |
| `rank`         | Classement par pertinence, fréquence et type |
| `snippet`      | Expansion de modèles (fonctions, structures) |
| `integration`  | API commune pour LSP et CLI |
| `tests`        | Cas complets d’autocomplétion contextuelle |

---

## Exemple d’utilisation

```rust
use vitte_autocompletion::CompletionEngine;
use vitte_analyzer::Analyzer;

fn main() -> anyhow::Result<()> {
    let src = "fn main() { pri }";
    let analyzer = Analyzer::from_source(src)?;
    let completions = CompletionEngine::complete(&analyzer, src, 14)?;
    for item in completions {
        println!("Suggestion: {}", item.label);
    }
    Ok(())
}
```

---

## Intégration

- `vitte-lsp` : moteur principal pour VS Code, Neovim, Helix, etc.  
- `vitte-cli` : complétion de commandes et sous-commandes.  
- `vitte-analyzer` : extraction du contexte lexical et symbolique.  
- `vitte-docgen` : complétion des tags et références croisées.  

---

## Dépendances

```toml
[dependencies]
vitte-core       = { path = "../vitte-core", version = "0.1.0" }
vitte-analyzer   = { path = "../vitte-analyzer", version = "0.1.0" }
vitte-ast        = { path = "../vitte-ast", version = "0.1.0" }
vitte-utils      = { path = "../vitte-utils", version = "0.1.0" }

anyhow = "1"
serde = { version = "1.0", features = ["derive"] }
fuzzy-matcher = "0.3"
```

---

## Tests

```bash
cargo test -p vitte-autocompletion
```

- Vérification du scoring et du tri.  
- Tests sur complétions lexicales, symboliques et contextuelles.  
- Snapshots des suggestions dans plusieurs contextes de code.  

---

## Roadmap

- [ ] Intégration avec le moteur d’analyse incrémentale.  
- [ ] Complétion basée sur la fréquence d’usage (télémetrie opt-in).  
- [ ] Support multi-langue pour IDE (i18n).  
- [ ] Suggestions dynamiques basées sur le type d’expressions.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau