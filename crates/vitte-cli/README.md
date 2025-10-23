

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
- [ ] Support des profils d’environnement (`--profile dev/release`).  
- [ ] Mode serveur RPC pour intégration IDE.  
- [ ] Commande `vitte doctor` pour diagnostics globaux.  
- [ ] Internationalisation (i18n) CLI et messages.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau