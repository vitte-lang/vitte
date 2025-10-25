

# vitte-cli

Interface **en ligne de commande** pour le langage Vitte.  
`vitte-cli` fournit les sous-commandes principales de compilation, exécution, documentation, analyse et gestion de modules pour l’écosystème Vitte.

---

## Objectifs

- Outil unifié pour **compiler**, **exécuter**, **inspecter**, et **documenter** les projets Vitte.  
- Interface stable et extensible (sous-commandes dynamiques).  
- Support complet du *workspace* multi-crates.  
- Couche CLI commune à `vitte-build`, `vitte-docgen`, `vitte-analyzer`, `vitte-lsp`, etc.  
- Intégration transparente avec le cache (`vitte-cache`) et le système de capacités (`vitte-cap`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `args`        | Définition des sous-commandes et options via `vitte-args` |
| `dispatch`    | Routage vers les sous-modules de compilation, analyse, etc. |
| `context`     | Gestion du contexte d’exécution (workspace, config, cache) |
| `output`      | Gestion des sorties : texte, table, JSON, TTY enrichi |
| `progress`    | Affichage des barres de progression et journaux ANSI |
| `completion`  | Génération de complétions shell (bash, zsh, fish, powershell) |
| `inspect`     | Analyse des fichiers `.vitbc` et introspection binaire |
| `help`        | Documentation embarquée et manuel de commandes |
| `tests`       | Tests d’intégration CLI et snapshots |

---

## Exemple d’utilisation

```bash
# Compiler un fichier Vitte
vitte build main.vitte -o main.vitbc

# Exécuter directement
vitte run examples/hello.vitte

# Inspecter un binaire compilé
vitte inspect --summary target/main.vitbc

# Générer la documentation du projet
vitte docgen --output site/

# Gérer les modules externes
vitte modules add vitte-lang/std

# Activer le mode verbose et couleurs
vitte --verbose --color=always build
```

---

## Intégration

- `vitte-build` : orchestration de la compilation et du caching.  
- `vitte-docgen` : rendu de la documentation locale.  
- `vitte-analyzer` : diagnostics statiques et symboliques.  
- `vitte-lsp` : support IDE via protocoles JSON-RPC.  
- `vitte-cap` : contrôle d’accès et exécution confinée.  
- `vitte-cache` : réutilisation d’artefacts binaires.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-args = { path = "../vitte-args", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-cache = { path = "../vitte-cache", version = "0.1.0" }
vitte-cap = { path = "../vitte-cap", version = "0.1.0" }

clap = { version = "4", features = ["derive"] }
indicatif = "0.17"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

---

## Tests

```bash
cargo test -p vitte-cli
```

- Tests fonctionnels et de parsing des arguments.  
- Tests d’intégration CLI multi-sous-commandes.  
- Tests de performance sur projets volumineux.  
- Snapshots de sortie JSON et textuelle.

---

## Roadmap

- [ ] Autocomplétion interactive (repl auto‑pilotée).  
  - Ajouter un moteur REPL incrémental basé sur `vitte-analyzer` et `vitte-inc` pour suggérer complétions et snippets.
  - Implémenter un protocole d’échanges asynchrones entre l’entrée utilisateur et le générateur de suggestions.
  - Intégrer des commandes d’aide contextuelle (`:doc`, `:type`, `:impl`) et un historique persisté.
  - Couvrir le flux par des tests d’intégration simulant des sessions interactives.
- [ ] Support des profils d’environnement (`--profile dev/release`).  
  - Centraliser la configuration des profils (flags optimisation, diagnostics, cache) dans `context`.
  - Propager le profil choisi aux sous-commandes `build`, `run`, `test` et `docgen`.
  - Ajouter des fichiers de config optionnels (`vitte.toml`) avec overrides par profil.
  - Vérifier via tests que les profils modifient bien les artefacts (artefacts, logs, timings).
- [ ] Mode serveur RPC pour intégration IDE.  
  - Exposer un serveur longue durée (JSON-RPC) encapsulant les commandes clés du CLI.
  - Définir un protocole d’authentification légère et une API de streaming pour la sortie des builds.
  - Intégrer la compilation incrémentale et la synchronisation de diagnostics avec `vitte-lsp`.
  - Ajouter une batterie de tests d’intégration qui valident la compatibilité avec des clients IDE simulés.
- [ ] Commande `vitte doctor` pour diagnostics globaux.  
  - Scanner l’environnement (toolchain, cache, permissions, versions) et détecter les anomalies courantes.
  - Générer un rapport structuré (texte/JSON) avec recommandations.
  - Proposer des actions correctives (purge cache, téléchargement dépendances) avec confirmation utilisateur.
  - Ajouter des tests couvrant le diagnostic sur environnements volontairement dégradés.
- [ ] Internationalisation (i18n) CLI et messages.
  - Externaliser les chaînes utilisateur vers un catalogue (`.ftl`/`.po`) et intégrer `vitte-i18n`.
  - Supporter la sélection de langue via variable d’environnement/option CLI (`--lang`).
  - Assurer la pluralisation et les formats (dates, nombres) via une bibliothèque dédiée.
  - Ajouter des tests pour garantir la présence des traductions et l’absence de chaînes non localisées.

**Transverse**
- Harmoniser la journalisation et les codes de sortie pour faciliter l’intégration IDE & scripts.
- Documenter chaque fonctionnalité livrée (man page, `--help`, guides) et ajouter des exemples pratiques.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
