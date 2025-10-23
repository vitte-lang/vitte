

# vitte-prompt

Système **d’interaction, saisie utilisateur et invite de commande** pour l’écosystème Vitte.  
`vitte-prompt` fournit les primitives nécessaires à la lecture d’entrée interactive, la complétion, la coloration et la gestion de l’historique des commandes.

---

## Objectifs

- Offrir une **API unifiée et ergonomique** pour les entrées utilisateur.  
- Supporter la complétion, l’historique et la coloration syntaxique.  
- Intégration directe avec `vitte-cli`, `vitte-repl` et `vitte-playground`.  
- Fournir un environnement interactif pour les outils et scripts Vitte.  
- Compatibilité totale avec les environnements TTY et non-interactifs.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `input`       | Lecture des lignes et gestion du buffer |
| `history`     | Sauvegarde et récupération des commandes précédentes |
| `completion`  | Système de complétion et suggestions |
| `highlight`   | Coloration syntaxique et surlignage des erreurs |
| `theme`       | Gestion du thème visuel clair/sombre de la console |
| `event`       | Gestion des événements clavier et signal utilisateur |
| `tests`       | Vérification d’interactivité et cohérence du rendu |

---

## Exemple d’utilisation

```rust
use vitte_prompt::Prompt;

fn main() -> anyhow::Result<()> {
    let mut prompt = Prompt::new("vitte> ");
    while let Some(line) = prompt.read_line()? {
        if line.trim() == "exit" { break; }
        println!("Vous avez saisi : {}", line);
    }
    Ok(())
}
```

---

## Intégration

- `vitte-cli` : gestion des interactions utilisateur dans les sous-commandes.  
- `vitte-playground` : saisie interactive et édition des snippets.  
- `vitte-lsp` : prise en charge des requêtes utilisateur côté serveur.  
- `vitte-docgen` : confirmation et interaction lors de la génération documentaire.  
- `vitte-studio` : affichage des invites dans l’interface utilisateur graphique.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

rustyline = "12.0"
console = "0.15"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `crossterm` pour compatibilité terminal, `termcolor` pour sorties enrichies.

---

## Tests

```bash
cargo test -p vitte-prompt
```

- Tests de lecture et complétion basique.  
- Tests de persistance de l’historique.  
- Tests de compatibilité terminal/TTY.  
- Benchmarks sur la latence d’entrée et d’affichage.

---

## Roadmap

- [ ] Support des suggestions contextuelles basées sur l’analyse syntaxique.  
- [ ] Mode multi-ligne pour le REPL.  
- [ ] API de thèmes personnalisables.  
- [ ] Intégration directe dans `vitte-studio`.  
- [ ] Complétion dynamique des fichiers et symboles.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau