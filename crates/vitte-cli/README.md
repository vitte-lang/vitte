

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

# Diagnostiquer l'environnement et produire un rapport JSON
vitte doctor --json

# Utiliser la CLI en français
VITTE_LANG=fr vitte --lang fr help modules
```

## Profils d’environnement

- `--profile dev|release` applique des defaults centralisés (`optimize`, `diagnostics`, `cache`).
- Les profils peuvent être surchargés depuis `vitte.toml` :

```toml
[profiles.dev]
optimize = false
emit_debug = true

[profiles.release]
optimize = true
diagnostics = false
cache = true
```

Sans fichier, le profil `dev` désactive l’optimisation et conserve les diagnostics, tandis que `release` active l’optimisation et le cache.

## Mode serveur JSON-RPC

- `vitte server --listen 127.0.0.1:7411 --token secret` lance un serveur longue durée.
- Authentification légère via la méthode `auth` (JSON-RPC 2.0).
- Méthodes exposées :
  - `compile` : déclenche la compilation d’un fichier (supporte `profile`, `optimize`, `emit_debug`).
  - `run` : exécute un bytecode ou une source (`auto_compile`, passage d’arguments).
  - `fmt` : retourne le code formaté.
  - `ping`, `shutdown`.
- Notifications `build/progress` streament l’état (`started`/`finished`).
- Les chemins de fichiers sont résolus relativement à `--root` (par défaut le dossier courant).

## Commande doctor

- `vitte doctor` exécute une série de diagnostics (toolchain, cache `target`, modules).
- `--json` produit un rapport structuré, `--fix-cache` propose la purge du dossier `target` (confirmation par défaut, ignorable via `--yes`).
- Retourne un code de sortie `0` si tout est sain, `1` en présence d’avertissements et `2` si des erreurs bloquantes sont détectées.

## Internationalisation

- La langue par défaut est l’anglais. `--lang en|fr` ou la variable `VITTE_LANG` permettent de forcer la langue active.
- Les rapports de `vitte doctor` et les messages interactifs sont traduits (français/anglais).
- De nouvelles chaînes sont à ajouter dans `src/i18n/*.ftl`; si une traduction manque, la valeur anglaise est utilisée.

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

- [x] Autocomplétion interactive (repl auto‑pilotée).  
  - Moteur de REPL avec complétions basées sur `vitte-analyzer` (symboles connus) et snippets de base.
  - Incrémental optionnel via la feature `analyzer-incremental` (active `vitte-analyzer/incremental`).
  - Brancher l’API de complétion sur le REPL (`vitte-repl`) et/ou `vitte-lsp` pour bénéficier des symboles/projets existants.
  - File d’événements asynchrone: capture de l’entrée utilisateur, envoi des requêtes (completion/hover/definition), rendu non bloquant.
  - Générer des snippets contextuels (ex: signatures de fonctions, patterns `impl`, `match`) et proposer l’insertion directe.
  - Gérer l’historique persistant et l’aide contextuelle (`:doc`, `:type`, `:impl`) depuis la session interactive.
  - Tests d’intégration simulant des sessions auto‑pilotées (scénarios de suggestion, insertion, rollback) avec budgets de latence définis.
  - Implémenter un protocole d’échanges asynchrones entre l’entrée utilisateur et le générateur de suggestions.
  - Intégrer des commandes d’aide contextuelle (`:doc`, `:type`, `:impl`) et un historique persisté.
  - Couvrir le flux par des tests d’intégration simulant des sessions interactives.
- [x] Support des profils d’environnement (`--profile dev/release`).  
  - Ajout d’un module `context` chargé de charger les profils (`ProfileConfig`).
  - Les commandes `compile` et `run` héritent des defaults du profil (optimize, diagnostics, cache) avec possibilité d’override via flags.
  - Lecture facultative d’un fichier `vitte.toml` (section `[profiles.{dev,release}]`) pour surcharger les réglages.
  - Les profils sont testés via compilation ciblée (`cargo build -p vitte-cli`).
- [x] Mode serveur RPC pour intégration IDE.  
  - Serveur JSON-RPC (`vitte server`) avec notifications de progression.
  - Authentification optionnelle via token, API `compile`/`run`/`fmt`/`ping`/`shutdown`.
  - Résolution de profils et options de build partagées avec la CLI (`context::ProfileConfig`).
  - Prochaine étape : étendre les tests d’intégration avec des clients IDE simulés.
- [x] Commande `vitte doctor` pour diagnostics globaux.  
  - Scanner l’environnement (toolchain, cache, permissions, versions) et détecter les anomalies courantes.
  - Générer un rapport structuré (texte/JSON) avec recommandations.
  - Proposer des actions correctives (purge cache, téléchargement dépendances) avec confirmation utilisateur.
  - Ajouter des tests couvrant le diagnostic sur environnements volontairement dégradés.
- [x] Internationalisation (i18n) CLI et messages.
  - Catalogue simple (`src/i18n/en.ftl`, `src/i18n/fr.ftl`) chargé au démarrage via `--lang` / `VITTE_LANG`.
  - Gestion basique du pluriel et des messages dynamiques pour `vitte doctor`.
  - Confirmation interactive localisée et suggestions traduites.

**Transverse**
- Harmoniser la journalisation et les codes de sortie pour faciliter l’intégration IDE & scripts.
- Documenter chaque fonctionnalité livrée (man page, `--help`, guides) et ajouter des exemples pratiques.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
