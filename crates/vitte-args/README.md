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
  - Étendre `parser` pour exposer un moteur incrémental capable de suggérer arguments et sous-commandes à chaque frappe.
  - Brancher l’API sur `vitte-cli` afin de piloter un REPL interactif (tab completion, hints inline).
  - Ajouter une couche de détection de capacités (`vitte-ansi`) pour désactiver proprement en TTY non compatibles.
  - Couvrir l’expérience via tests d’intégration simulant des sessions interactives.
- [ ] Génération automatique de pages man (`man vitte`).
  - Introduire un backend `help::man` qui exporte le graphe de commandes vers un format manpage (roff).
  - Intégrer la génération au pipeline de release (`build.rs`/CI) et fournir un `vitte help --man`.
  - Ajouter des tests de snapshot sur les sections (NAME, SYNOPSIS, OPTIONS) pour prévenir les régressions.
- [ ] Support multi-langue pour les messages CLI.
  - Externaliser toutes les chaînes utilisateur vers un catalogue (ex: Fluent) et ajouter une couche de localisation.
  - Détecter la locale via env (`LANG`, `LC_MESSAGES`) avec possibilité d’override `--lang`.
  - Garantir que chaque entrée dispose d’une traduction et ajouter des tests de complétude du catalogue.
- [ ] Intégration avec `vitte-lsp` pour suggestions contextuelles.
  - Exposer un protocole léger (JSON-RPC ou pipe) fournissant la liste des sous-commandes/arguments disponibles.
  - Synchroniser les descriptions et types d’arguments avec l’IDE pour auto-complétion contextuelle.
  - Mettre en place des tests d’intégration LSP simulant requêtes `completion` et `hover`.

---

## Priorités produit & suivi

1. **Complétion interactive** — impact direct UX, dépendances prêtes (`vitte-ansi`, parser).
2. **Génération de manpages** — nécessite packaging/distribution.
3. **Localisation (i18n)** — exigence marchés.
4. **Pont LSP** — dépend des APIs CLI stabilisées.

Épics recommandés pour le suivi :

- *Complétion interactive* : moteur incrémental, REPL interactif, détection TTY/capacités, tests interactifs headless.
- *Manpages* : générateur roff, intégration build/CI, snapshots NAME/SYNOPSIS/OPTIONS.
- *Localisation* : extraction des chaînes, catalogue (Fluent/PO), détection locale/env, tests de complétude.
- *LSP bridge* : API JSON-RPC/pipe, synchronisation métadonnées CLI↔IDE, tests `completion`/`hover`.

Ces épics reprennent les sous-tâches détaillées dans la roadmap et cadrent la planification produit/ingénierie.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
