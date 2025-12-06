# Architecture du bootstrap Vitte

Ce document décrit l’architecture du **bootstrap Vitte** telle qu’elle est modélisée dans le workspace `vitte-core` :

- structure globale des couches de bootstrap ;
- rôle de chaque couche (host, front, middle, back, cli, core, pipeline) ;
- liens avec les manifests Muffin (`bootstrap/*.muf`, `vitte.project.muf`, `muffin.muf`) ;
- place des scripts shell (`scripts/*.sh`) dans l’orchestration ;
- conventions sur les artefacts (`target/bootstrap/...`).

L’objectif est d’avoir une vision claire et **purement déclarative** du bootstrap, centrée uniquement sur Vitte.

---

## 1. Vue d’ensemble

Le bootstrap Vitte vise à amener le compilateur Vitte à l’état **auto-hébergé** (self-host), en plusieurs étapes logiques :

1. **Stage0 – host layer**  
   - Prépare l’environnement hôte, les scripts, les fixtures de tests.
   - Effectue des vérifications minimales et génère des rapports placeholders.

2. **Stage1 – vittec-stage1**  
   - Construit une première version du compilateur Vitte, potentiellement à l’aide d’outils externes ou de scripts spécifiques.

3. **Stage2 – vittec-stage2 (self-host)**  
   - Recompile le compilateur Vitte à l’aide de vittec-stage1 pour obtenir une version auto-hébergée.
   - Éventuellement expose un binaire `vittec` stable (par exemple via un symlink).

La logique de ces étapes est modélisée via des **couches de bootstrap**, chacune décrite par un manifest Muffin dédié dans `bootstrap/` :

- `bootstrap/host/mod.muf`      : couche **host** (stage0),
- `bootstrap/front/mod.muf`     : couche **front-end**,
- `bootstrap/middle/mod.muf`    : couche **middle** (compiler + runtime + tools),
- `bootstrap/back/mod.muf`      : couche **back-end**,
- `bootstrap/cli/mod.muf`       : couche **CLI**,
- `bootstrap/core/mod.muf`      : vue **core** (compiler/runtime/std/tools),
- `bootstrap/pipeline/mod.muf`  : couche **pipeline** (phases, artefacts, stages),
- `bootstrap/mod.muf`           : agrégateur **bootstrap** global.

Les scripts shell sous `scripts/` sont les **orchestrateurs** :

- `scripts/bootstrap_stage0.sh` → stage0 (host),
- `scripts/self_host_stage1.sh` → stage1+stage2 (self-host),
- `scripts/env_local.sh` → configuration d’environnement locale,
- `scripts/clean.sh` → nettoyage des artefacts.

Le tout est piloté par un `Makefile` et éventuellement par les configurations VS Code (`.vscode/`).

---

## 2. Manifests principaux

### 2.1. `muffin.muf` – manifest racine

Le manifest `muffin.muf` (à la racine du repo) décrit le **workspace Vitte** global. Il est le point d’entrée logique pour :

- la toolchain Vitte (future commande `vitte` ou `vitte-bootstrap`) ;
- les outils d’indexation (LSP, analyseurs, etc.) ;
- la CI qui souhaite comprendre la structure générale du projet.

Il ne contient pas de règles de build impératives, uniquement des **métadonnées** (modules, projets, profils, etc.).

### 2.2. `vitte.project.muf` – vue projet

Le manifest `vitte.project.muf` fournit une vue **centrée “projet”** du workspace `vitte-core` :

- sous-projets : `compiler`, `runtime`, `std`, `tools`, `bootstrap`, `tests` ;
- profils projet : `dev`, `release`, `ci` ;
- dépendances logiques entre sous-projets ;
- entrées principales : `vittec`, `vitte-run`, `vitte-tools`, `vitte-bootstrap`.

Cette vue est utile pour les outils de haut niveau (launchers de projet, UIs, etc.).

### 2.3. `bootstrap/mod.muf` – agrégateur bootstrap

Ce manifest agrège toutes les couches de bootstrap :

- `[bootstrap.layer."host"]` → `bootstrap/host/mod.muf` ;
- `[bootstrap.layer."front"]` → `bootstrap/front/mod.muf` ;
- `[bootstrap.layer."middle"]` → `bootstrap/middle/mod.muf` ;
- `[bootstrap.layer."back"]` → `bootstrap/back/mod.muf` ;
- `[bootstrap.layer."cli"]` → `bootstrap/cli/mod.muf` ;
- `[bootstrap.layer."core"]` → `bootstrap/core/mod.muf` ;
- `[bootstrap.layer."pipeline"]` → `bootstrap/pipeline/mod.muf`.

Il définit également des **profils globaux** de bootstrap (`dev`, `fast-dev`, `ci`) et des hints tooling (`[tool.vitte.bootstrap_root]`).

---

## 3. Couches de bootstrap

Chaque couche est décrite par un manifest `mod.muf` dédié. Les mots-clés suivants reviennent dans ces manifests :

- `[<layer>]` : métadonnées générales (nom, édition, description, version, profil par défaut).
- `[<layer>.roles]` : rôles logiques de la couche.
- `[<layer>.project."…"]` : projets logiques (bibliothèques, tooling, scripts).
- `[<layer>.binary."…"]` : binaries logiques (programmes, bundles d’outils).
- `[<layer>.artifact."…"]` : artefacts générés (reports, dumps, bundles, logs).
- `[<layer>.profile."…"]` : profils (listes de projets/binaries/artefacts activés).
- `[<layer>.link.…]` : liens vers d’autres couches ou la pipeline.
- `[tool.vitte.<layer>]` : hints pour la toolchain Vitte (ordre d’init, outils, tests).

### 3.1. Couche host (stage0)

Manifest : `bootstrap/host/mod.muf`

Rôle :

- Préparer l’environnement hôte (`scripts/`, `tests/`, grammaire) ;
- Fournir des rapports initiaux (validation de la grammaire, fixtures, etc.) ;
- Offrir une base stable pour les stages suivants (stage1/stage2).

Éléments principaux :

- Projets :
  - `bootstrap-scripts` → `scripts/` (`bootstrap_stage0.sh`, `self_host_stage1.sh`, `clean.sh`, `env_local.sh`) ;
  - `spec-and-grammar` → `grammar/`, `docs/language-spec.md` ;
  - `test-fixtures` → `tests/data/...` ;
  - `host-tools-core` → `bootstrap/host/tools`.

- Binaries logiques :
  - `vittec0` (compiler de bootstrap initial, concept logique) ;
  - `host-test-runner` ;
  - `host-tools-bundle`.

- Artefacts :
  - `grammar-checked` → `target/bootstrap/host/grammar/report.txt` ;
  - `samples-validated` → `target/bootstrap/host/samples/report.txt` ;
  - `stage0-logs` → `target/bootstrap/host/logs/`.

- Profils `dev`, `fast-dev`, `ci` ;
- Liens : `[host.link.pipeline]`, `[host.link.middle]` ;
- Hints tooling : `[tool.vitte.host]`, `[tool.vitte.host.test]`.

Scripts associés :

- `scripts/bootstrap_stage0.sh` :
  - vérifie la structure du workspace, la présence des manifests, etc. ;
  - génère effectivement les rapports placeholders `grammar/report.txt` et `samples/report.txt` ;
  - logge dans `target/bootstrap/host/logs/stage0.log`.

### 3.2. Couche front

Manifest : `bootstrap/front/mod.muf`

Rôle :

- Modéliser le **front-end du compilateur Vitte** :
  - lexer, parser, AST, spans, diagnostics, scopes/symboles,
  - outils d’inspection (AST dump, traces de parse, etc.).

Éléments principaux :

- Projets :
  - `compiler-front-core` → `src/vitte/compiler/front` (lexer/parser/ast/span/diagnostics) ;
  - `compiler-front-scope` → `scope/symbols` ;
  - `compiler-front-tools` → outils d’inspection front.

- Binaries logiques :
  - `vitte-front-check` ;
  - `vitte-ast-dump` ;
  - `vitte-parse-trace`.

- Artefacts :
  - `ast-dumps` → `target/bootstrap/front/ast/` ;
  - `parse-traces` → `target/bootstrap/front/parse_traces/` ;
  - `front-diag-report` → `target/bootstrap/front/diag/report.txt`.

Cette couche est essentielle pour valider la **syntaxe** et la structure des programmes Vitte lors du bootstrap.

### 3.3. Couche middle

Manifest : `bootstrap/middle/mod.muf`

Rôle :

- Représenter le cœur du compilateur (front/middle/back intégrés), le runtime et certains outils core, pour les stages 1 et 2.
- **IR unique SSA léger (middle/back)** : blocs + terminators structurés (`br`, `br_if`, `return`), valeurs SSA typées (primitifs, structs/enums/alias) et `phi` uniquement sur les jonctions structurées. Les effets (alloc, store, call) sont explicites.
- **Passes minimales middle** :
  1. résolution des types de base sur MIR/HIR (alignée sur `docs/type-system.md`) ;
  2. structuration du contrôle (`if`/`while`/`match` → CFG structuré) ;
  3. SSA léger (renommage, `phi` restreints) sans optimisations agressives ;
  4. pré‑lowering vers bytecode (normalisation des ops arith/comparaison, accès champs, load/store).
- **Bytecode VM simple (MVP)** : jeu d’instructions linéaire consommé par `vitte.runtime.vm` :
  - arith/comparaisons : `add/sub/mul/div/mod/neg`, `cmp_eq/ne/lt/le/gt/ge` ;
  - contrôle : `jmp`, `jmp_if`, `ret` (structures haut niveau déjà abaissées) ;
  - appels : `call`, `call_indirect` (optionnel), passage registres/stack, retour via registre cible ;
  - mémoire : `alloc_heap`, `alloc_stack`, `load_local`, `store_local`, `load_field`, `store_field`, `move/copy`.
  - tables associées : pool de constantes, table des fonctions/signatures, table des types runtime.
- **Artefacts de debug** : dumps IR SSA (`vitte-ir-dump`), listings bytecode annotés (`vitte-bytecode-emit`), reports de link/CFG pour stage1/2.

Éléments principaux (vue logique) :

- Projets :
  - `compiler-front`, `compiler-middle`, `compiler-back` ;
  - `runtime-core` ;
  - `tools-core`.

- Binaries logiques :
  - `vittec-stage1` ;
  - `vittec-stage2` ;
  - `vitte-run` (vue liée au runtime) ;
  - éventuellement `vitte-tools` (tools-core).

- Artefacts intermédiaires :
  - `std-bundle`, `bytecode-tests`, `compiler-ir-dumps`, etc.

- Profils `dev`, `fast-dev`, `ci`.

Cette couche est celle que le script **stage1+2** manipule conceptuellement ; les vrais builds sont branchés via des hooks.

### 3.4. Couche back

Manifest : `bootstrap/back/mod.muf`

Rôle :

- Modéliser le **back-end du compilateur** :
  - IR, passes, codegen, link, outils de visualisation/diagnostic.

Éléments principaux :

- Projets :
  - `compiler-back-core` → IR, codegen.bytecode, codegen.text, link ;
  - `compiler-back-tools` → ir_dump, bytecode_emit, link_inspect ;
  - `compiler-back-passes` → lowering, simplify, layout.

- Binaries :
  - `vitte-ir-dump` ;
  - `vitte-bytecode-emit` ;
  - `vitte-link-check`.

- Artefacts :
  - `ir-text-dumps`, `ir-graph-dumps` ;
  - `bytecode-output` ;
  - `link-report`.

Cette couche est fortement liée au runtime (le bytecode généré est consommé par la VM).

### 3.5. Couche CLI

Manifest : `bootstrap/cli/mod.muf`

Rôle :

- Décrire toutes les **commandes CLI Vitte** :
  - `vittec`, `vitte-run`, `vitte-tools`, `vitte-bootstrap`,
  - leurs modules CLI (parsing des arguments, sous-commandes, etc.),
  - les artefacts ergonomiques (help-text, complétions).

Éléments principaux :

- Projets :
  - `compiler-cli`, `runtime-cli`, `tools-cli`, `bootstrap-cli`.

- Binaries :
  - `vittec`, `vitte-run`, `vitte-tools`, `vitte-bootstrap`.

- Artefacts :
  - `help-text` → `target/bootstrap/cli/help/` ;
  - `shell-completions` → `target/bootstrap/cli/completions/` ;
  - `cli-usage-report` → `target/bootstrap/cli/report.txt`.

Cette couche fournit la **surface utilisateur** de la toolchain Vitte.

### 3.6. Couche core

Manifest : `bootstrap/core/mod.muf`

Rôle :

- Offrir une vue **agrégée** des composants clé :

  - compiler-core (front/middle/back, IR, CLI),
  - runtime-core,
  - std-core,
  - tools-core,

- Définir les binaries globaux :

  - `vittec`, `vitte-run`, `vitte-tools`,

- Modéliser des artefacts communs :

  - `core-ir-snapshots`,
  - `core-bytecode-bundle`,
  - `core-test-report`.

La couche core est la “vue workspace” du cœur de la toolchain, au-dessus des détails spécifiques à chaque step de bootstrap.

### 3.7. Couche pipeline

Manifest : `bootstrap/pipeline/mod.muf`

Rôle :

- Décrire la **pipeline de compilation logique** Vitte :
  - phases (`front-end`, `middle-end`, `back-end`, `runtime`, `tooling`),
  - étapes (`stage0-host-tools`, `stage1-bootstrap-compiler`, `stage2-self-host`, etc.),
  - artefacts clés qui circulent (AST, IR, bytecode, logs, reports).

Ce manifest est la documentation déclarative de “comment” un programme Vitte traverse la toolchain, indépendamment des implémentations concrètes.

---

## 4. Scripts d’orchestration

### 4.1. `scripts/env_local.sh`

- Doit être **sourcé** (`source ./scripts/env_local.sh`), non exécuté directement.
- Définit et exporte :
  - `VITTE_WORKSPACE_ROOT`,
  - `VITTE_BOOTSTRAP_ROOT`,
  - `VITTE_EDITION`, `VITTE_PROFILE`, `VITTE_BOOTSTRAP_PROFILE`.
- Ajoute à `PATH` (si existants) :
  - `target/debug`, `target/release`, `.local/bin`.
- Peut être rendu silencieux via `VITTE_ENV_SILENT=1`.

### 4.2. `scripts/bootstrap_stage0.sh`

- Rôle :
  - préparer la couche host (stage0), vérifier la structure du workspace, produire des rapports placeholders.

- Comportement (simplifié) :
  - calcule `VITTE_WORKSPACE_ROOT` et `VITTE_BOOTSTRAP_ROOT`,
  - crée :
    - `target/bootstrap/host/logs/`,
    - `target/bootstrap/host/grammar/`,
    - `target/bootstrap/host/samples/`,
  - vérifie :
    - présence de `muffin.muf`, `bootstrap/`, `src/`, éventuellement `tests/`,
    - présence des manifests bootstrap/std,
  - génère :
    - `grammar/report.txt` (rapport placeholder),
    - `samples/report.txt` (rapport placeholder),
    - `logs/stage0.log` (log complet).

### 4.3. `scripts/self_host_stage1.sh`

- Rôle :
  - orchestrer **stage1** (vittec-stage1) et **stage2** (vittec-stage2, self-host).

- Comportement clé :

  1. Résout `VITTE_WORKSPACE_ROOT`, `VITTE_BOOTSTRAP_ROOT`.
  2. Assure un workspace valide (présence de `muffin.muf`, `bootstrap/`, `src/`).
  3. Peut invoquer automatiquement `bootstrap_stage0.sh` sauf si `VITTE_SKIP_STAGE0=1`.
  4. Créé les répertoires :
     - `target/bootstrap/stage1/logs/`,
     - `target/bootstrap/stage2/logs/`.
  5. Expose des **hooks** configurable sous `scripts/hooks/` :
     - `build_vittec_stage1.sh` / `build_stage1_compiler.sh`,
     - `build_vittec_stage2.sh` / `build_stage2_compiler.sh`.
  6. Si aucun hook n’est présent, crée des fichiers `status.txt` placeholders dans `target/bootstrap/stage1/` et `target/bootstrap/stage2/`.
  7. Expose éventuellement un `target/debug/vittec` (symlink vers vittec-stage2 ou vittec-stage1).

Ces scripts ne codent pas de build system spécifique ; ils fournissent un **cadre** dans lequel tu peux brancher tes propres scripts de compilation.

### 4.4. `scripts/clean.sh`

- Nettoie de manière sûre :
  - le contenu de `target/` (en laissant le dossier lui-même),
  - certains fichiers temporaires (`*.tmp`, `*.swp`, `*~`, `.DS_Store`) dans `src/`, `tests/`, `bootstrap/`.
- Ne touche ni au code source ni aux manifests ni à `.vscode/`.

---

## 5. Artefacts et structure `target/`

En suivant les manifests et scripts, la structure cible est conceptuellement la suivante (simplifiée) :

```text
target/
  bootstrap/
    host/
      logs/
        stage0.log
      grammar/
        report.txt
      samples/
        report.txt

    stage1/
      logs/
        stage1.log
      status.txt         # placeholder si aucun hook n’est encore branché

    stage2/
      logs/
        stage2.log
      status.txt         # placeholder

    front/
      ast/
        ...              # dumps d’AST
      parse_traces/
        ...              # traces de parsing
      diag/
        report.txt

    back/
      ir/
        ...              # dumps IR textuels
      ir_graph/
        ...              # dumps IR sous forme de graphes/logs
      bytecode/
        ...              # bytecode généré
      link/
        report.txt       # rapport de link

    cli/
      help/
        ...              # textes d’aide CLI
      completions/
        ...              # scripts de complétion
      report.txt

  core/
    ir/
      ...                # snapshots IR (vue core)
    bytecode/
      ...                # bundle bytecode std/tests
    tests/
      report.txt

  debug/
    vittec               # symlink vers vittec-stage2 ou vittec-stage1 (optionnel)
    vitte-run            # binaire runtime (futur)
    vitte-tools          # binaire bundle outils (futur)

  release/
    vittec               # binaire release (futur)
```

Cette structure est **déclarée** par les manifests et **réalisée** progressivement par les scripts et les futures commandes Vitte.

---

## 6. Intégration avec Makefile et VS Code

### 6.1. Makefile

Le `Makefile` fournit des cibles simples :

- `make env` → affiche l’environnement Vitte (via `env_local.sh`),
- `make bootstrap-stage0` → lance `bootstrap_stage0.sh`,
- `make bootstrap-stage1` → lance `self_host_stage1.sh`,
- `make bootstrap-all` → enchaîne stage0 + stage1,
- `make mini_project` → smoke build du mini projet (si `target/debug/vittec` existe),
- `make clean` / `make distclean` → nettoyage,
- `make fmt` / `make lint` / `make test` → placeholders.

Ces cibles sont alignées avec les scripts et la structure `target/` décrite plus haut.

### 6.2. VS Code

Les fichiers sous `.vscode/` sont configurés pour ce workspace :

- `settings.json` :
  - associations de fichiers (`*.vitte`, `*.muf`, `*.lex`, `*.l`),
  - indentation/whitespace/rulers,
  - exclusions de recherche (`target/`, `dist/`, `node_modules/`),
  - environnement terminal (`VITTE_WORKSPACE_ROOT`, `VITTE_BOOTSTRAP_ROOT`).

- `launch.json` :
  - configs pour `vittec` debug/release, `vitte-run`, mini_project, commande personnalisée.

- `tasks.json` :
  - tasks pour bootstrap, build vittec/vitte-run, mini_project, commande custom,
  - alignement des `preLaunchTask` avec les cibles de `launch.json`.

---

## 7. Conventions et évolutions

- Le modèle décrit ici est **purement déclaratif** :
  - aucun manifest ne décrit les détails d’un build system externe,
  - tout est exprimé en termes de projets, binaries, artefacts, profils, liens.

- Les scripts shell sont volontairement **agnostiques** :
  - ils n’imposent ni CMake, ni Ninja, ni un autre outil,
  - ils exposent des hooks pour brancher les implémentations de build Vitte.

- À mesure que la toolchain Vitte mûrit, les points suivants pourront être raffinés :
  - structure exacte de la IR (et des dumps IR),
  - format du bytecode et de ses bundles,
  - comportement détaillé des commandes CLI (`vittec`, `vitte-run`, `vitte-tools`, `vitte-bootstrap`),
  - intégration avec une extension VS Code Vitte dédiée (language server, etc.).

L’important est que **l’architecture du bootstrap reste stable** : un ensemble de couches clairement identifiées, liées entre elles par des manifests Muffin et des scripts d’orchestration minimalistes, entièrement centrées sur Vitte et sur rien d’autre.

---

## 8. Résumé

- Le bootstrap Vitte est structuré en **couches** (host, front, middle, back, cli, core, pipeline), chacune décrite par un manifest `mod.muf` dans `bootstrap/`.
- Un manifest global `bootstrap/mod.muf` agrège ces couches et définit des profils `dev`/`fast-dev`/`ci`.
- `vitte.project.muf` donne une vue **projet** de `vitte-core` (compiler, runtime, std, tools, bootstrap, tests).
- Les scripts `env_local.sh`, `bootstrap_stage0.sh`, `self_host_stage1.sh` et `clean.sh` fournissent une **orchestration générique** pour le bootstrap et le nettoyage.
- La structure `target/` est explicitement décrite via les manifests (artefacts) et les scripts (répertoires, logs, rapports).
- Makefile et VS Code sont configurés pour exploiter cette architecture de manière cohérente.

Ce document doit servir de référence lorsque tu feras évoluer la grammaire, les modules Vitte, ou la manière dont les binaires sont produits : l’architecture **conceptuelle** restera, les implémentations pourront changer.
