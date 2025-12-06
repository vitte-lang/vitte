# vitte-core

Noyau du langage **Vitte** : spécification, compilateur, runtime/VM, bibliothèque standard et outils de développement.

Ce dépôt correspond au workspace principal de la toolchain Vitte, décrit par les manifests Muffin :

- `muffin.muf`              : manifest racine du workspace,
- `vitte.project.muf`       : vue “projet” détaillée (compiler/runtime/std/tools/bootstrap/tests),
- `bootstrap/mod.muf`       : vue globale du bootstrap Vitte,
- `src/std/mod.muf`         : index de la bibliothèque standard Vitte (std).

Tout est centré sur Vitte (aucune dépendance conceptuelle à un autre langage).

---

## Objectifs du dépôt

- Définir la **syntaxe et la grammaire** du langage Vitte (Pest, EBNF, docs).
- Fournir une **toolchain complète Vitte** :
  - compilateur (front/middle/back, IR, bytecode),
  - runtime/VM,
  - standard library (std),
  - outils CLI : `vittec`, `vitte-run`, `vitte-tools`, `vitte-bootstrap`.
- Documenter et structurer le **bootstrap Vitte** :
  - stage0 (host layer),
  - stage1 (vittec-stage1),
  - stage2 (vittec-stage2, self-host),
  - pipeline de compilation logique.
- Servir de base à l’outillage : LSP, formatteur, indexeurs, scripts CI, etc.

Ce dépôt ne contient que la logique Vitte (manifests, métadonnées, scripts génériques) ; les détails d’implémentation (binaire final, backends, etc.) sont laissés à la future toolchain Vitte.

---

## Arborescence (vue simplifiée)

```text
vitte-core/
  Makefile
  muffin.muf
  vitte.project.muf

  .editorconfig
  .gitignore
  .vscode/
    launch.json
    tasks.json
    setting.json

  bootstrap/
    mod.muf                # vue globale du bootstrap
    host/mod.muf           # couche host / stage0
    front/mod.muf          # couche front-end
    middle/mod.muf         # couche middle (stage1/stage2)
    back/mod.muf           # couche back-end
    cli/mod.muf            # couche CLI
    core/mod.muf           # vue agrégée core (compiler/runtime/std/tools)
    pipeline/mod.muf       # pipeline de compilation logique

  scripts/
    env_local.sh           # configuration locale d’environnement Vitte
    bootstrap_stage0.sh    # bootstrap stage0 (host layer)
    self_host_stage1.sh    # bootstrap stage1+stage2 (self-host compiler)
    clean.sh               # nettoyage des artefacts générés

  src/
    std/
      mod.muf              # manifest std (bibliothèque standard)
      ...                  # modules std.* (collections, fs, io, path, string, time, …)

    vitte/
      compiler/            # cœur du compilateur (front/middle/back, IR, CLI, …)
      runtime/             # runtime/VM Vitte
      tools/               # outils Vitte (formatter, LSP, symbol browser, test runner, …)

  tests/
    data/
      mini_project/        # mini projet de test (manifest Muffin, sources Vitte, …)
      samples/             # exemples et fixtures
      lex/                 # tests lexeur
      parse/               # tests parser
```

L’arborescence complète (avec tous les fichiers `.vitte`, `.muf`, etc.) est décrite par les différents manifests Muffin.

---

## Bootstrap Vitte – aperçu

Le bootstrap Vitte est entièrement décrit via des manifests Muffin déclaratifs et quelques scripts shell génériques :

- **Manifests de couches** :
  - `bootstrap/host/mod.muf`      : couche host / stage0 (scripts, vittec0, fixtures),
  - `bootstrap/front/mod.muf`     : couche front-end (lex/parse/AST/diag),
  - `bootstrap/middle/mod.muf`    : couche middle (compiler front/middle/back, runtime core, tools),
  - `bootstrap/back/mod.muf`      : couche back-end (IR, codegen, link),
  - `bootstrap/cli/mod.muf`       : couche CLI (vittec, vitte-run, vitte-tools, vitte-bootstrap),
  - `bootstrap/core/mod.muf`      : vue core (compiler/runtime/std/tools),
  - `bootstrap/pipeline/mod.muf`  : pipeline logique (phases, artefacts, stages),
  - `bootstrap/mod.muf`           : agrégateur bootstrap.

- **Scripts d’orchestration** :
  - `scripts/bootstrap_stage0.sh`  : stage0 (host layer, rapports placeholders, checks),
  - `scripts/self_host_stage1.sh`  : stage1+stage2 (self-host compiler, hooks extensibles),
  - `scripts/env_local.sh`         : configuration d’environnement (VITTE_WORKSPACE_ROOT, PATH, etc.),
  - `scripts/clean.sh`             : nettoyage sûr des artefacts (`target/`, fichiers temporaires).

Les manifests sont purement déclaratifs et décrivent : modules, projets, artefacts, profils (`dev`, `fast-dev`, `ci`) et relations entre couches. Les scripts restent volontairement agnostiques sur la toolchain exacte : ils exposent des hooks (scripts/hooks/…) à brancher lorsque tes binaires Vitte seront prêts.

---

## Prérequis (pour utiliser les scripts fournis)

- Environnement Unix-like (macOS, Linux).
- Shell :
  - `bash` (et/ou `zsh` si tu préfères, les scripts utilisent `/usr/bin/env bash`).
- Outils de base recommandés :
  - `git` (pour la gestion du dépôt),
  - un éditeur compatible `.editorconfig` et, idéalement, VS Code.

Aucun compilateur spécifique n’est imposé à ce stade ; les étapes de build réelles (génération des binaires Vitte) sont à brancher via des hooks.

---

## Configuration de l’environnement

Depuis la racine du dépôt :

```bash
source ./scripts/env_local.sh
```

Ce script définit notamment :

- `VITTE_WORKSPACE_ROOT` : racine du workspace Vitte,
- `VITTE_BOOTSTRAP_ROOT` : racine du bootstrap (`bootstrap/`),
- `VITTE_EDITION` / `VITTE_PROFILE` / `VITTE_BOOTSTRAP_PROFILE`,
- met à jour `PATH` pour inclure `target/debug`, `target/release` et `.local/bin` si présents.

Pour rendre l’output silencieux dans les scripts ou CI :

```bash
export VITTE_ENV_SILENT=1
source ./scripts/env_local.sh
```

---

## Commandes Makefile

Un `Makefile` minimal mais structuré est fourni pour piloter les tâches de haut niveau.

### Aide

```bash
make help
```

Affiche les cibles principales :

- `env`,
- `bootstrap-stage0`,
- `bootstrap-stage1`,
- `bootstrap-all`,
- `mini_project`,
- `clean`,
- `distclean`,
- `fmt`, `lint`, `test` (placeholders).

### Bootstrap stage0

```bash
make bootstrap-stage0
```

- Lance `scripts/bootstrap_stage0.sh` ;
- Vérifie la structure du workspace, la présence des manifests, etc. ;
- Génère des rapports placeholders dans `target/bootstrap/host/` :
  - `grammar/report.txt`,
  - `samples/report.txt`,
  - `logs/stage0.log`.

### Bootstrap stage1 + stage2 (self-host compiler)

```bash
make bootstrap-stage1
```

- Lance `scripts/self_host_stage1.sh` ;
- Peut déclencher automatiquement stage0 si nécessaire (sauf si `VITTE_SKIP_STAGE0=1`) ;
- Gère deux étapes logiques :
  - **Stage1** : construction d’un premier compilateur Vitte (vittec-stage1),
  - **Stage2** : rebuild du compilateur avec lui-même (vittec-stage2, self-host) ;
- Expose éventuellement un `target/debug/vittec` (symlink vers le binaire le plus récent) si les hooks de build produisent des binaires.

Les étapes de build réelles sont laissées à des scripts hooks (par exemple : `scripts/hooks/build_vittec_stage1.sh`, `scripts/hooks/build_vittec_stage2.sh`).

### Bootstrap complet

```bash
make bootstrap-all
```

Enchaîne :

1. `bootstrap-stage0`,
2. `bootstrap-stage1`.

### Mini-projet – smoke build

Une fois un `vittec` debug disponible :

```bash
make mini_project
```

- Utilise `target/debug/vittec` pour compiler `tests/data/mini_project/muffin.muf` ;
- Sert de smoke test end-to-end (placeholders tant que le CLI réel n’est pas finalisé).

### Nettoyage

```bash
make clean
```

- Passe par `scripts/clean.sh` ;
- Nettoie `target/` (en conservant le dossier) et quelques fichiers temporaires (`*.tmp`, `*.swp`, `*~`, `.DS_Store`, …).

```bash
make distclean
```

- Appelle `clean` puis supprime complètement le dossier `target/`.

### Placeholders fmt / lint / test

Les cibles suivantes sont prêtes pour être branchées sur les futurs outils Vitte :

```bash
make fmt   # à lier à vitte.tools.format
make lint  # à lier à des passes de validation
make test  # à lier à vitte.tools.test_runner + fixtures
```

---

## Intégration VS Code

Le dossier `.vscode/` contient des configurations prêtes pour ce workspace.

### Associations de fichiers / comportement éditeur

`./.vscode/setting.json` configure notamment :

- Associations de fichiers :
  - `*.vitte` → langage `vitte`,
  - `*.muf`, `*.muffin` → `vitte-muffin`,
  - `*.lex`, `*.l` → `vitte-lex`.
- Exclusions :
  - `target/`, `dist/`, `node_modules/`, etc.
- Indentation et affichage :
  - 4 espaces pour le code Vitte,
  - 2 espaces pour les manifests Muffin et JSON,
  - règles adaptées pour lexers, docs, etc.
- Terminal intégré :
  - export de `VITTE_WORKSPACE_ROOT` et `VITTE_BOOTSTRAP_ROOT` pour les shells VS Code.

### Debug / Run

`./.vscode/launch.json` définit plusieurs configurations `cppdbg` (LLDB) :

- `Vitte: vittec (debug)` → `${workspaceFolder}/target/debug/vittec`,
- `Vitte: vittec (release)` → `${workspaceFolder}/target/release/vittec`,
- `Vitte: vitte-run (debug)` → `${workspaceFolder}/target/debug/vitte-run`,
- `Vitte: mini_project smoke (vittec + vitte-run)`,
- `Vitte: custom command` (chemin + arguments demandés à l’utilisateur).

`./.vscode/tasks.json` fournit les tasks correspondantes :

- `bootstrap: stage0 (host tools)`,
- `bootstrap: stage1+stage2 (self-host compiler)`,
- `bootstrap: clean`,
- `build vittec (debug)` / `build vittec (release)` / `build vitte-run (debug)`,
- `build mini_project (debug)`,
- `vitte: custom command`.

Les `preLaunchTask` du `launch.json` sont alignés sur ces labels.

---

## Style de code et conventions

Les conventions basiques sont décrites dans :

- `.editorconfig` :
  - UTF-8, LF, `insert_final_newline = true`, `trim_trailing_whitespace = true`,
  - 4 espaces pour le code Vitte (`*.vitte`, `.lex`, `.l`, scripts `.sh`),
  - 2 espaces pour les manifests (`*.muf`, `*.muffin`), JSON, YAML, TOML, etc.  
- `src/std/mod.muf` / `bootstrap/*.muf` / `vitte.project.muf` :
  - structure déclarative pour modules, sous-projets, profils, artefacts.

Le style de codage Vitte lui-même (nommage, organisation des modules, etc.) est en cours de définition via la spécification du langage et les exemples de code Vitte.

---

## Licence

Ce projet est distribué sous licence **MIT**.

Le texte complet de la licence est disponible dans le fichier `LICENSE` à la racine du dépôt.  
En contribuant à ce dépôt, vous acceptez que vos contributions soient publiées sous cette même licence.

---

## DMCA / Takedown

Les procédures de notification et de contre-notification sont décrites dans `docs/dmca-takedown.md`.

- L’adresse de contact dédiée : `legal@vitte.dev`.
- Les demandes doivent contenir toutes les informations nécessaires (identification de l’œuvre, URLs ou chemins de fichiers concernés, déclaration sur l’honneur, coordonnées de contact, etc.).
- La politique de retrait est appliquée de façon cohérente avec la législation applicable et les principes décrits dans la documentation DMCA du projet.

En cas de doute sur l’utilisation correcte des fichiers ou sur les droits associés, merci de **ne pas** republier les contenus de ce dépôt dans un autre projet sans vérification préalable des conditions de licence, ni d’utiliser la marque Vitte d’une manière pouvant prêter à confusion.

---

## Remarques

- Ce dépôt décrit l’architecture, les manifests et scripts d’orchestration du monde Vitte.
- Les binaires finaux (`vittec`, `vitte-run`, `vitte-tools`, etc.) et la grammaire définitive peuvent encore évoluer.
- L’objectif est de fournir une base solide et cohérente pour construire une toolchain Vitte moderne, auto-hébergée, avec un écosystème clair (std, tests, bootstrap, outillage).

Toute contribution devra respecter ces principes : focus Vitte seulement, pas d’intrusion d’autres langages/toolchains dans la surface publique du projet.
