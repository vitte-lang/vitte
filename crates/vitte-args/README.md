# vitte-args

Bibliothèque **de parsing et gestion avancée des arguments CLI** pour le compilateur et les outils Vitte.

`vitte-args` centralise toute la logique d’analyse, de validation et de description des arguments de ligne de commande utilisés par `vitte`, `vitte-cli` et les utilitaires associés (`vitte-fmt`, `vitte-docgen`, `vitte-build`, etc.).  
Il constitue le socle de la couche interface utilisateur en ligne de commande du langage.

---

## Objectifs

- Fournir une API unifiée et typée pour la gestion des arguments.
- Support complet des sous-commandes (`build`, `run`, `fmt`, `docgen`, etc.).
- Validation stricte et génération automatique des messages d’aide.
- Intégration fluide avec le moteur de terminal (`vitte-ansi`).
- Gestion des erreurs CLI cohérente avec les standards de Vitte.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `parser`       | Tokenisation et interprétation des arguments bruts |
| `command`      | Définition des sous-commandes et de leurs options |
| `help`         | Génération automatique du texte d’aide et des manpages |
| `error`        | Gestion centralisée des erreurs et retours utilisateur |
| `completion`   | Support des complétions Shell (bash/zsh/fish/pwsh) |
| `tests`        | Couverture des cas unitaires et intégration CLI |

---

## Exemple d’utilisation

```rust
use vitte_args::{ArgParser, Command};

fn main() -> anyhow::Result<()> {
    let parser = ArgParser::new("vitte")
        .about("Le compilateur et toolkit du langage Vitte")
        .subcommand(Command::new("build").about("Compile un projet"))
        .subcommand(Command::new("run").about("Exécute un fichier ou binaire Vitte"));

    let matches = parser.parse()?;
    match matches.subcommand_name() {
        Some("build") => println!("Compilation du projet..."),
        Some("run") => println!("Exécution..."),
        _ => parser.print_help(),
    }

    Ok(())
}
```

---

## Sous-commandes supportées (par défaut)

| Commande | Description |
|-----------|--------------|
| `build`   | Compile le projet Vitte actuel |
| `run`     | Exécute un fichier source ou binaire |
| `fmt`     | Formate le code source |
| `docgen`  | Génère la documentation HTML/MD |
| `test`    | Lance les tests unitaires |
| `check`   | Vérifie le projet sans compilation complète |
| `clean`   | Supprime les artefacts de build |
| `repl`    | Lance un interpréteur interactif |

---

## Intégration

- `vitte-cli` : exécution directe des commandes utilisateur.
- `vitte-docgen` : gestion des options de génération de documentation.
- `vitte-build` : parsing des arguments de build et de cible.
- `vitte-fmt` : options de formatage CLI.
- `vitte-lsp` : configuration du mode serveur.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-ansi = { path = "../vitte-ansi", version = "0.1.0" }
clap = { version = "4.5", features = ["derive", "color"] }
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-args
```

- Vérification de parsing correct des commandes.
- Tests des erreurs utilisateur et du mode strict.
- Validation de la génération d’aide.
- Tests d’intégration avec `vitte-cli`.

---

## Roadmap

- [ ] Mode interactif de complétion intelligente (CLI dynamique).
- [ ] Génération automatique de pages man (`man vitte`).
- [ ] Support multi-langue pour les messages CLI.
- [ ] Intégration avec `vitte-lsp` pour suggestions contextuelles.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau