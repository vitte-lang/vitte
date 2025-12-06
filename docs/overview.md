# Vitte core – Overview

Ce document donne une vue d’ensemble du dépôt **`vitte-core`**, c’est‑à‑dire du cœur du langage **Vitte** et de sa toolchain :

- ce que contient ce dépôt ;
- comment il est structuré (workspace, manifests, dossiers) ;
- comment les différents composants (bootstrap, compilateur, runtime, std, outils) s’articulent ;
- par où commencer pour explorer ou faire évoluer Vitte.

Il complète les autres documents de référence :

- `README.md` : présentation générale et usage pratique (Makefile, VS Code, etc.) ;
- `docs/architecture-bootstrap.md` : architecture du bootstrap Vitte ;
- `docs/compiler-architecture.md` : architecture du compilateur Vitte ;
- `docs/design-decisions.md` : décisions de design de haut niveau ;
- `docs/language-spec.md` : spécification du noyau du langage Vitte (édition 2025).

---

## 1. Objectif du dépôt `vitte-core`

`vitte-core` modélise **tout l’univers central de Vitte** :

- le langage Vitte lui‑même (syntaxe, sémantique de base, représentation interne) ;
- le compilateur Vitte (front/middle/back, IR, bytecode, CLI) ;
- le runtime/VM Vitte et le format de bytecode associé ;
- la bibliothèque standard (std) et ses modules ;
- les outils de développement (formatter, LSP, symbol browser, test runner, etc.) ;
- le bootstrap du compilateur (stages 0/1/2, auto‑hébergement) ;
- les métadonnées de workspace et de projet via les manifests Muffin.

Ce dépôt est conçu comme la **“colonne vertébrale”** de Vitte : tout ce qui définit ce qu’est Vitte (et comment il fonctionne) doit être représenté ici, de façon déclarative et stable.

---

## 2. Structure globale du repo

Vue simplifiée (voir aussi `README.md`) :

```text
vitte-core/
  Makefile
  muffin.muf
  vitte.project.muf

  .editorconfig
  .gitignore
  .vscode/

  docs/
    overview.md
    architecture-bootstrap.md
    compiler-architecture.md
    design-decisions.md
    language-spec.md
    dmca-takedown.md (optionnel / à venir)

  bootstrap/
    mod.muf
    host/mod.muf
    front/mod.muf
    middle/mod.muf
    back/mod.muf
    cli/mod.muf
    core/mod.muf
    pipeline/mod.muf

  scripts/
    env_local.sh
    bootstrap_stage0.sh
    self_host_stage1.sh
    clean.sh

  src/
    std/
      mod.muf
      ... (modules std.*)

    vitte/
      compiler/
      runtime/
      tools/

  tests/
    data/
      mini_project/
      samples/
      lex/
      parse/
```

Les fichiers clés :

- `muffin.muf` : manifest racine du workspace Vitte ;
- `vitte.project.muf` : vue projet complète (compiler, runtime, std, tools, bootstrap, tests) ;
- `bootstrap/*.muf` : manifests décrivant les couches du bootstrap ;
- `src/std/mod.muf` : manifest de la std ;
- `docs/*.md` : documentation de référence.

---

## 3. Composants principaux

### 3.1. Langage Vitte

Le langage Vitte est décrit dans :

- `docs/language-spec.md` : spécification du noyau (édition 2025) ;
- les fichiers de grammaire (Pest/EBNF) dans `grammar/` (à compléter) ;
- la modélisation interne dans les modules `vitte.compiler.*` (AST, types, IR, etc.).

Caractéristiques de base (voir la spec pour le détail) :

- blocs explicites, **sans accolades**, terminés par `.end` ;
- indentation significative, mais non ambigüe (fermeture de bloc toujours explicite) ;
- syntaxe centrée sur les modules, fonctions, structs, enums, blocs de contrôle (`if`, `loop`, `match`, etc.) ;
- système de types statique, avec types de base, structurés et composés (std).

### 3.2. Compilateur

Le compilateur Vitte est détaillé dans `docs/compiler-architecture.md` et modélisé par les modules :

- `vitte.compiler.lexer`, `vitte.compiler.parser`, `vitte.compiler.ast`, `vitte.compiler.span` ;
- `vitte.compiler.diagnostics`, `vitte.compiler.scope`, `vitte.compiler.symbols` ;
- `vitte.compiler.types`, `vitte.compiler.typecheck` ;
- `vitte.compiler.hir`, `vitte.compiler.mir`, `vitte.compiler.ir` ;
- `vitte.compiler.constant_fold`, `vitte.compiler.flow_analysis`, `vitte.compiler.optimize` ;
- `vitte.compiler.codegen.bytecode`, `vitte.compiler.codegen.text`, `vitte.compiler.link` ;
- `vitte.compiler.cli.*` (CLI `vittec`).

Pipeline logique (simplifiée) :

1. Source Vitte → tokens (lexer) ;
2. tokens → AST (parser) ;
3. AST → HIR/MIR/IR (analyses + transformations) ;
4. IR → bytecode Vitte (codegen) ;
5. bytecode + std → exécutable VM via `vitte-run`.

### 3.3. Runtime / VM

Le runtime est modélisé par les modules `vitte.runtime.*` :

- `vitte.runtime.vm` : cœur de la VM ;
- `vitte.runtime.bytecode` : format/structures de bytecode ;
- `vitte.runtime.gc` : gestion mémoire ;
- `vitte.runtime.std_hooks` : intégration de la std ;
- `vitte.runtime.cli.run` : CLI `vitte-run`.

L’objectif est d’avoir une VM dédiée, avec un bytecode propre à Vitte, facile à analyser et à outiller.

### 3.4. Bibliothèque standard (std)

La std est structurée sous `src/std/`, avec un manifest principal `src/std/mod.muf`.

Exemples de modules (logiques) :

- `std.collections` (vecteurs, maps, etc.) ;
- `std.fs` ;
- `std.io` ;
- `std.path` ;
- `std.string` ;
- `std.time`.

Elle sert de **contrat minimal** : tout environnement Vitte conforme doit fournir au moins ce socle.

### 3.5. Outils (tools)

Les outils Vitte sont modélisés par `vitte.tools.*` :

- formatter ;
- LSP / language server ;
- symbol browser ;
- test runner ;
- autres utilitaires.

Ils sont exposés via un binaire logique `vitte-tools`, décrit dans `vitte.project.muf` et la couche `bootstrap/cli/mod.muf`.

---

## 4. Bootstrap et auto‑hébergement

Le bootstrap Vitte est détaillé dans `docs/architecture-bootstrap.md`. Il est structuré en **couches** et en **stages**.

### 4.1. Couches de bootstrap

Manifests dans `bootstrap/` :

- `host` : stage0, prérequis host, scripts, fixtures ;
- `front` : front-end du compilateur (lex/parse/AST/diag) ;
- `middle` : cœur compilateur + runtime, vittec-stage1/2 ;
- `back` : IR/bytecode/link, outils de debug ;
- `cli` : commandes CLI (vittec, vitte-run, vitte-tools, vitte-bootstrap) ;
- `core` : vue agrégée compiler/runtime/std/tools ;
- `pipeline` : pipeline de compilation logique.

Chaque couche est déclarée dans un `mod.muf` dédié et agrégée par `bootstrap/mod.muf`.

### 4.2. Stages du bootstrap

Stages principaux :

- **stage0 (host)** : vérification du workspace, génération de rapports placeholders, préparation de `target/bootstrap/host/` ;
- **stage1 (vittec-stage1)** : construction d’un premier compilateur Vitte ;
- **stage2 (vittec-stage2, self-host)** : recompilation du compilateur par lui‑même, exposition éventuelle de `target/debug/vittec`.

Orchestration via les scripts :

- `scripts/env_local.sh` : configuration d’environnement (VITTE_WORKSPACE_ROOT, PATH, etc.) ;
- `scripts/bootstrap_stage0.sh` : stage0 ;
- `scripts/self_host_stage1.sh` : stage1+stage2 ;
- `scripts/clean.sh` : nettoyage.

Des hooks (`scripts/hooks/*.sh`) peuvent être ajoutés pour brancher les builds réels sans modifier les scripts principaux.

---

## 5. Manifests Muffin et profils

Les manifests Muffin sont des fichiers déclaratifs qui décrivent la **structure** du workspace et des projets.

### 5.1. `muffin.muf` (workspace)

- Décrit le workspace Vitte global ;
- référence les manifests de sous-systèmes (bootstrap, std, etc.) ;
- sert de point d’entrée pour les outils et la future CLI workspace.

### 5.2. `vitte.project.muf` (projet)

- Vue projet complète : sous‑projets `compiler`, `runtime`, `std`, `tools`, `bootstrap`, `tests` ;
- profils projet : `dev`, `release`, `ci` ;
- dépendances logiques entre sous‑projets ;
- entrées principales : `vittec`, `vitte-run`, `vitte-tools`, `vitte-bootstrap` ;
- paramètres tooling (`tool.vitte.project`), expectations globales (présence du compilateur, runtime, std, tests…).

### 5.3. `bootstrap/*.muf`

- `bootstrap/mod.muf` : agrégateur de couches ;
- `bootstrap/host/mod.muf`, `front/mod.muf`, `middle/mod.muf`, etc. : détails des projets, artefacts, binaries logiques par couche ;
- profils bootstrap : `dev`, `fast-dev`, `ci` (sélection de projets/artefacts).

### 5.4. `src/std/mod.muf`

- Index de la std ;
- liste des modules std, artefacts de bundles, profils std (par exemple, minimum/étendu).

Les manifests ont un rôle **structurant**, pas de logique de build : les scripts/tooling Vitte les interprètent pour savoir quoi construire, tester ou analyser.

---

## 6. Scripts, Makefile et VS Code

### 6.1. Scripts

- `scripts/env_local.sh` : à sourcer, configure les chemins et profils Vitte ;
- `scripts/bootstrap_stage0.sh` : stage0 host ;
- `scripts/self_host_stage1.sh` : stage1+2 (self‑host) ;
- `scripts/clean.sh` : nettoyage de `target/` et des fichiers temporaires.

### 6.2. Makefile

Le `Makefile` fournit des commandes de haut niveau :

- `make help` : aperçu des cibles ;
- `make env` : affiche l’environnement Vitte ;
- `make bootstrap-stage0` ;
- `make bootstrap-stage1` ;
- `make bootstrap-all` ;
- `make mini_project` ;
- `make clean` / `make distclean` ;
- `make fmt` / `make lint` / `make test` (placeholders pour futurs outils Vitte).

### 6.3. VS Code

Le dossier `.vscode/` contient :

- `settings.json` : associations de fichiers (`*.vitte`, `*.muf`, `*.lex`, `*.l`), indentation, exclusions, variables d’environnement pour les terminaux ;
- `launch.json` : configurations de debug pour `vittec`, `vitte-run`, mini_project, commandes custom ;
- `tasks.json` : tasks alignées sur les scripts (bootstrap, build vittec/vitte-run, smoke tests).

---

## 7. Documentation de référence

Les principaux documents dans `docs/` :

- `overview.md` (ce fichier) : vue d’ensemble du dépôt ;
- `architecture-bootstrap.md` : architecture détaillée du bootstrap Vitte ;
- `compiler-architecture.md` : architecture détaillée du compilateur Vitte (front/middle/back, IR, codegen) ;
- `design-decisions.md` : décisions de design (philosophie Vitte, choix syntaxiques, IRs, VM, std, bootstrap, tooling, gouvernance) ;
- `language-spec.md` : spécification du noyau du langage Vitte (édition 2025) ;
- `dmca-takedown.md` (si présent) : politique de DMCA/takedown et aspects légaux.

Ces documents constituent la **référence texte** de ce que Vitte est censé être, indépendamment du code concret.

---

## 8. Par où commencer ?

Selon ton objectif :

### 8.1. Comprendre le langage

1. Lire `docs/language-spec.md` (syntaxe, blocs, types, contrôle de flux).  
2. Consulter `docs/design-decisions.md` pour comprendre les motivations derrière la syntaxe et les IRs.  
3. Explorer les exemples de code (tests/mini_project, samples) lorsque disponibles.

### 8.2. Comprendre l’architecture du compilateur

1. Lire `docs/compiler-architecture.md`.  
2. Parcourir les modules `vitte.compiler.*` dans `src/vitte/compiler/` (une fois implémentés).  
3. Observer comment la pipeline est modélisée dans `bootstrap/pipeline/mod.muf`.

### 8.3. Comprendre le bootstrap

1. Lire `docs/architecture-bootstrap.md`.  
2. Regarder les manifests sous `bootstrap/`.  
3. Lancer les scripts, une fois prêts :
   ```bash
   source ./scripts/env_local.sh
   make bootstrap-stage0
   make bootstrap-stage1
   ```

### 8.4. Travailler sur la std ou les outils

- Pour la std : lire `src/std/mod.muf` et les modules `std.*`.  
- Pour les outils : consulter `vitte.tools.*` et `vitte.project.muf` (binaire `vitte-tools`).

---

## 9. État du projet et évolutions

L’architecture décrite par `vitte-core` est volontairement **plus large** que l’implémentation actuelle :

- certains modules peuvent être encore vides, incomplets ou en brouillon ;
- les scripts de bootstrap fonctionnent d’abord avec des placeholders et des hooks ;
- la std, la VM, le compilateur et les outils sont amenés à évoluer progressivement.

Les axes d’évolution majeurs sont listés dans `docs/design-decisions.md` (macros, traits, async, IR avancée, tooling, packaging, etc.).

Le présent dépôt sert de **cadre de référence** : même si l’implémentation change, la structure (manifests, docs, séparation front/middle/back, VM dédiée, std) doit rester cohérente avec ce qui est décrit ici.

---

## 10. Résumé

- `vitte-core` est le workspace qui définit Vitte (langage, compilateur, runtime, std, tooling, bootstrap).  
- La structure est entièrement décrite par des **manifests Muffin** et une documentation claire dans `docs/`.  
- Les scripts et le Makefile offrent une première orchestration pour le bootstrap et les tâches courantes.  
- VS Code est pré‑configuré pour travailler confortablement sur Vitte (fichiers `.vitte`, `.muf`, `.lex`, `.l`).  
- Ce dépôt constitue la base pour construire, documenter et faire évoluer un langage Vitte moderne, auto‑hébergé, avec une VM et un écosystème cohérents.
