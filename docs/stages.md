# Vitte core – Stages du bootstrap

Ce document détaille les **stages du bootstrap Vitte** tels qu’ils sont modélisés dans le dépôt `vitte-core` et décrits par les manifests Muffin sous `bootstrap/` ainsi que par les scripts dans `scripts/`.

Il complète :

- `docs/architecture-bootstrap.md` – vue d’architecture par couches (host, front, middle, back, cli, core, pipeline) ;
- `docs/compiler-architecture.md` – architecture du compilateur Vitte (front/middle/back, IR, bytecode) ;
- `docs/roadmap.md` – phases de développement (0 → 6) ;
- `docs/overview.md` – vue globale du dépôt.

Ce document se concentre sur les **stages** au sens du **bootstrap** : stage0, stage1, stage2, leurs objectifs, leurs artefacts et leurs interactions avec les scripts et manifests.

---

## 1. Vue d’ensemble des stages

Le bootstrap Vitte est découpé en trois stages principaux :

1. **Stage0 – Host / Préparation**  
   - Vérifie la structure du workspace.
   - Prépare les répertoires de bootstrap dans `target/`.
   - Produit des rapports et artefacts **placeholders** pour la grammaire, les samples, etc.
   - Ne dépend pas d’un compilateur Vitte déjà fonctionnel.

2. **Stage1 – vittec-stage1**  
   - Produit une première version du compilateur Vitte (binaire ou équivalent logique).
   - Peut s’appuyer sur des outils existants, des scripts externes ou un sous‑ensemble de Vitte déjà implémenté.
   - Sert à compiler une partie du code Vitte du projet.

3. **Stage2 – vittec-stage2 (Self-host)**  
   - Recompile le compilateur Vitte en utilisant vittec-stage1.
   - Produit une version **auto‑hébergée** du compilateur (self‑host).
   - Peut exposer un binaire logique `vittec` (souvent via `target/debug/vittec`).

Chaque stage est décrit de manière **déclarative** dans les manifests Muffin (`bootstrap/*.muf`) et orchestré via les scripts (`scripts/*.sh`) et le `Makefile`.

---

## 2. Stage0 – Host / Préparation

### 2.1. Objectifs

Le stage0 a pour but de :

- vérifier que le workspace Vitte est cohérent (structure attendue, manifests présents) ;
- préparer les répertoires `target/bootstrap/host/...` ;
- produire des rapports initiaux (placeholder) pour :
  - la grammaire Vitte (fichiers de grammaire, spec),
  - les samples / fixtures de tests,
  - les logs du bootstrap ;
- éviter de dépendre d’un compilateur ou d’une VM Vitte déjà opérationnels.

Stage0 est donc un **stage host-only**, centré sur la machine de développement ou la CI.

### 2.2. Manifests concernés

- `bootstrap/host/mod.muf` :
  - décrit la couche `host` du bootstrap ;
  - déclare les projets, artefacts et profils liés au stage0 ;
  - relie la couche host aux autres couches (front/middle/pipeline) via des sections de liens.

- `bootstrap/mod.muf` :
  - agrège la couche host avec les autres couches du bootstrap ;
  - peut définir des profils de bootstrap (`dev`, `fast-dev`, `ci`) où la présence de stage0 est obligatoire.

### 2.3. Scripts et Makefile

Stage0 est orchestré par :

- `scripts/bootstrap_stage0.sh` :
  - détermine `VITTE_WORKSPACE_ROOT` et `VITTE_BOOTSTRAP_ROOT` (souvent via `env_local.sh`) ;
  - vérifie la présence des fichiers clés (`muffin.muf`, `bootstrap/`, `src/`, éventuellement `tests/`) ;
  - crée les répertoires :
    - `target/bootstrap/host/logs/` ;
    - `target/bootstrap/host/grammar/` ;
    - `target/bootstrap/host/samples/` ;
  - génère au minimum :
    - `target/bootstrap/host/grammar/report.txt` ;
    - `target/bootstrap/host/samples/report.txt` ;
    - `target/bootstrap/host/logs/stage0.log`.

- `scripts/env_local.sh` :
  - fournit les variables d’environnement pour les scripts (chemins, profils, etc.).

Au niveau du `Makefile` :

```bash
make bootstrap-stage0
```

- exécute `scripts/bootstrap_stage0.sh` dans un environnement configuré ;
- affiche des messages de log lisibles côté développeur/CI.

### 2.4. Artefacts

Principaux artefacts générés à ce stage :

- **Grammaire** :
  - `target/bootstrap/host/grammar/report.txt` :
    - liste des fichiers de grammaire détectés,
    - état de base (présent / manquant),
    - éventuellement un court rapport de validation future.

- **Samples / fixtures** :
  - `target/bootstrap/host/samples/report.txt` :
    - inventaire des fixtures Vitte (ex. sous `tests/data`),
    - vérifications simples (fichiers existants, structure attendue).

- **Logs** :
  - `target/bootstrap/host/logs/stage0.log` :
    - détail de l’exécution du script stage0,
    - messages d’info, avertissements, erreurs éventuelles.

Ces artefacts servent de **base de contrôle** pour s’assurer que la suite du bootstrap (stages 1 et 2) part sur des fondations saines.

---

## 3. Stage1 – vittec-stage1

### 3.1. Objectifs

Le stage1 vise à construire un premier **compilateur Vitte** (que l’on appelle `vittec-stage1`), à partir de :

- code source Vitte (ou éventuellement d’une autre représentation initiale, selon la progression du projet) ;
- scripts de build externes (C, autre, Vitte partiel, etc.), encapsulés dans des hooks ;
- la structure déclarée dans les manifests bootstrap.

Ce compilateur stage1 :

- couvre au moins un sous‑ensemble du noyau Vitte 2025 ;
- est suffisamment complet pour recompiler tout ou partie du compilateur lui‑même (en vue du stage2).

### 3.2. Manifests concernés

Stage1 est principalement modélisé dans :

- `bootstrap/middle/mod.muf` :
  - décrit les projets de la couche middle (compiler front/middle/back, runtime minimal, tools de base) ;
  - déclare des artefacts logiques comme `vittec-stage1`, `vittec-stage2`, `std-bundle` minimal, etc. ;
  - prévoit des profils (`dev`, `ci`) indiquant quels composants sont attendus à l’issue de stage1.

- `bootstrap/core/mod.muf` :
  - offre une vue agrégée sur le compilateur, le runtime, la std et les outils ;
  - relie les artefacts de stage1 à la vision “core” du projet.

- `vitte.project.muf` :
  - déclare des entrées pour les binaries logiques (`vittec`, `vitte-run`, `vitte-tools`, `vitte-bootstrap`) ;
  - désigne le sous‑projet `compiler` comme producteur de `vittec`.

Stage1 peut aussi faire référence à des artefacts de test définis dans les manifests (rapports d’IR, dumps bytecode, etc.), même si ceux‑ci sont plus sollicités à partir du stage2.

### 3.3. Scripts et hooks

Stage1 est orchestré via :

- `scripts/self_host_stage1.sh` :
  - gère le stage1 et le stage2 ;
  - peut commencer par vérifier que stage0 a bien été exécuté (ou le lancer, sauf si `VITTE_SKIP_STAGE0=1`) ;
  - crée des répertoires dans :
    - `target/bootstrap/stage1/logs/` ;
    - `target/bootstrap/stage2/logs/` (préparation du stage2).

Pour la construction concrète de `vittec-stage1`, des **hooks** sont prévus :

- `scripts/hooks/build_vittec_stage1.sh` (nom indicatif) :
  - implémente la logique de build réelle pour produire le binaire (ou équivalent) `vittec-stage1` ;
  - peut être adapté au fur et à mesure (au début, il peut ne faire qu’une maquette ou appeler un autre tool ; plus tard, il utilisera le compilateur Vitte lui‑même).

Si aucun hook ou build réel n’est encore branché, `self_host_stage1.sh` doit :

- créer un fichier `target/bootstrap/stage1/status.txt` ;
- y noter qu’aucun compilateur stage1 n’a été construit (placeholder) ;
- retourner un code de sortie cohérent (succès soft pour développement local, échec explicite en mode CI strict, selon la configuration).

### 3.4. Artefacts de stage1

Artefacts conceptuels :

- `vittec-stage1` :
  - binaire logique produit par le stage1 ;
  - utilisé comme compilateur pour le stage2.

- `target/bootstrap/stage1/logs/stage1.log` :
  - trace de l’exécution du stage1 (hooks, builds, erreurs).

- `target/bootstrap/stage1/status.txt` :
  - état du stage1 (succès partiel, complet, placeholder, etc.).

À mesure que le projet avance, stage1 peut produire d’autres artefacts :

- dumps d’IR intermédiaire pour vérification ;
- std minimal compilé ;
- outils partiels.

### 3.5. Strate middle : IR unique SSA léger + passes minimales + bytecode VM simple

- **IR unique SSA léger** : blocs + terminators structurés (`br`, `br_if`, `return`), valeurs SSA typées (primitives, structs/enums/alias transparents), `phi` uniquement sur les jonctions structurées, effets explicites (`call`, `store_field`, `alloc_heap`).
- **Passes minimales middle** :
  1. Résolution des types de base sur MIR/HIR (alignée sur `docs/type-system.md`).
  2. Structuration du contrôle (`if`/`while`/`match` → CFG structuré).
  3. SSA léger (renommage + `phi` restreints) sans optimisations agressives.
  4. Pré-lowering bytecode : normalisation des ops arith/comparaison, accès champ, load/store pour correspondre 1:1 au bytecode.
- **Bytecode VM simple (MVP)** : instructions linéaires consommées par la VM (arith/comparaisons `add/sub/mul/div/mod/neg`, `cmp_eq/ne/lt/le/gt/ge`; contrôle `jmp`, `jmp_if`, `ret`; appels `call`, `call_indirect` optionnel; mémoire `alloc_heap`, `alloc_stack`, `load_local`, `store_local`, `load_field`, `store_field`, `move/copy`) + tables associées (constantes, fonctions/signatures, types runtime).
- **Artefacts stage1/2** : dumps IR SSA (`vitte-ir-dump`), listings bytecode (`vitte-bytecode-emit`), reports CFG/link pour debug et tests.

---

## 4. Stage2 – vittec-stage2 / Self-host

### 4.1. Objectifs

Le stage2 constitue la phase d’**auto‑hébergement** du compilateur :

- utiliser `vittec-stage1` comme compilateur pour recompiler les sources du compilateur Vitte (ou une partie significative) ;
- produire un deuxième compilateur, `vittec-stage2`, notionnellement plus “proche” de l’état final ;
- éventuellement :

  - comparer les sorties (par ex. IR/bytecode) stage1 vs stage2 pour détecter des divergences ;
  - définir `vittec` comme alias/symlink vers la version stage2.

L’objectif central : garantir que **le compilateur est capable de se recompiler lui‑même**, ce qui est un jalon important pour la stabilité d’un langage.

### 4.2. Manifests concernés

Les mêmes manifests que pour stage1 sont impliqués, avec un focus particulier sur :

- `bootstrap/middle/mod.muf` :
  - doit décrire les artefacts `vittec-stage2`, les rapports associés, et leurs liens avec les projets compiler/runtime/std ;

- `bootstrap/pipeline/mod.muf` :
  - décrit explicitement le pipeline :
    - `stage0-host-tools` → `stage1-bootstrap-compiler` → `stage2-self-host` ;
  - relie les artefacts de stage1 à ceux de stage2 (ex. `vittec-stage1` utilisé comme étape d’entrée pour `stage2`).

Les profils `dev` / `ci` peuvent exiger que le stage2 soit entièrement réussi (présence de `vittec-stage2` et d’un `vittec` stable).

### 4.3. Scripts et hooks

Stage2 est également orchestré par `scripts/self_host_stage1.sh` :

- après la construction réussie de `vittec-stage1` (ou d’un placeholder, en phase d’amorçage) ;
- le script :

  1. prépare `target/bootstrap/stage2/` ;
  2. appelle un hook de build stage2 (par exemple `scripts/hooks/build_vittec_stage2.sh`) ;
  3. enregistre les logs et l’état du stage2.

En présence d’un hook stage2 :

- `build_vittec_stage2.sh` peut :
  - lancer `vittec-stage1` pour recompiler les sources du compilateur ;
  - produire un binaire `vittec-stage2` dans un sous‑répertoire de `target/` (`target/bootstrap/stage2/` ou `target/debug/`).

En l’absence de hook :

- `self_host_stage1.sh` doit :
  - créer `target/bootstrap/stage2/status.txt` avec un message explicite ;
  - consigner l’absence de build dans `target/bootstrap/stage2/logs/stage2.log`.

### 4.4. Artefacts de stage2

Artefacts principaux :

- `vittec-stage2` :
  - binaire logique auto‑compilé ;
  - candidat à devenir l’implémentation principale du compilateur Vitte.

- `target/debug/vittec` :
  - optionnel, symlink ou copie vers `vittec-stage2` (ou vers `vittec-stage1` tant que stage2 n’est pas stable) ;
  - utilisé par le `Makefile` (`make mini_project`) et les outils (VS Code, tasks) comme “compiler par défaut”.

- `target/bootstrap/stage2/logs/stage2.log` :
  - log détaillé de l’exécution stage2.

- `target/bootstrap/stage2/status.txt` :
  - résumé de l’état du stage2.

À terme, stage2 peut aussi produire :

- des rapports de comparaison stage1/stage2 (IR, bytecode, diagnostics) ;
- des snapshots de std compilée par stage2 ;
- des métriques (temps de compilation, taille du bytecode, etc.).

---

## 5. Stages vs phases de roadmap

Il est important de distinguer :

- les **stages** du bootstrap (stage0/stage1/stage2) :  
  séquencement spécifique à la construction du compilateur ;

- les **phases** de la roadmap (`docs/roadmap.md`) :  
  niveaux de maturité du projet (0 → 6) couvrant tous les sous‑systèmes.

La relation approximative est la suivante :

- **Phase 0** : préparation du workspace, docs, manifests → prérequis pour stage0.
- **Phase 1** : langage noyau + front-end → permet d’enrichir stage0 (validation grammaire) et prépare stage1.
- **Phase 2** : types + IRs → nécessaire pour un `vittec-stage1` plus réaliste (phase 3 et 4 aussi).
- **Phase 3** : back-end + VM + std minimale → rend possible un pipeline complet jusqu’au bytecode.
- **Phase 4** : bootstrap complet + auto‑hébergement → correspond pleinement à la mise en place des stages 1 et 2.
- **Phase 5 et 6** : tooling, std élargie, qualité/perf → consolident et exploitent les stages existants (plus de tests, plus d’outils, meilleure CI).

Les stages peuvent exister sous une forme “maquette” dès les phases précoces (avec des placeholders), puis devenir de plus en plus concrets à mesure que les phases avancent.

---

## 6. Interactions avec les couches bootstrap

Les stages consomment et produisent des artefacts dans les **couches** décrites par `bootstrap/*.muf` :

- Stage0 interagit surtout avec :
  - `bootstrap/host/mod.muf` (scripts, grammaire, fixtures),
  - et indirectement avec `bootstrap/front` (car la grammaire concerne le front-end).

- Stage1 se nourrit de :
  - `bootstrap/front` (lexer/parser/AST/diag),
  - `bootstrap/middle` (compiler core, runtime minimal, outils),
  - `bootstrap/core` (vue agrégée compiler/runtime/std/tools).

- Stage2 s’appuie sur :
  - `bootstrap/middle` (étapes stage1 et stage2),
  - `bootstrap/core` (binaire `vittec` + runtime/std/prérequis),
  - `bootstrap/pipeline` (description des étapes de la chaîne de compilation).

Les manifests permettent d’exprimer les dépendances entre artefacts de stage0/1/2 sans enfermement dans un système de build particulier (les scripts et hooks réalisent le travail concret).

---

## 7. Utilisation pratique dans le workflow

### 7.1. Développement local

Scénario typique :

```bash
# 1) Configurer l’environnement
source ./scripts/env_local.sh

# 2) Stage0 (préparation, rapports)
make bootstrap-stage0

# 3) Stage1 + Stage2
make bootstrap-stage1

# 4) Smoke test mini_project (si vittec/vitte-run sont suffisamment prêts)
make mini_project
```

À ce stade :

- `target/bootstrap/host/...` contient les rapports stage0 ;
- `target/bootstrap/stage1/...` et `target/bootstrap/stage2/...` contiennent les logs/status pour stage1/2 ;
- `target/debug/vittec` peut exister et servir de compilateur principal.

### 7.2. Intégration continue (CI)

En CI, on peut :

- exiger que stage0 soit toujours exécuté (validation rapide du workspace) ;
- choisir :

  - soit de tolérer un stage1/2 partiellement implémenté (build maquette) en début de projet ;
  - soit d’imposer un stage2 complet (self‑host) comme prérequis de l’intégration sur la branche principale, une fois la toolchain mature.

La granularité des checks (par stage et par artefact) peut être ajustée via les profils dans les manifests Muffin (profil `ci`, `dev`, `fast-dev`, etc.).

---

## 8. Résumé

- Le bootstrap Vitte est structuré en **trois stages** :
  - **stage0** : host / préparation (workspace, rapports, logs),
  - **stage1** : vittec-stage1 (premier compilateur bootstrap),
  - **stage2** : vittec-stage2 (compilateur auto‑hébergé, candidate `vittec`).

- Chaque stage est :
  - décrit dans les manifests bootstrap (`bootstrap/*.muf`),
  - orchestré par des scripts (`bootstrap_stage0.sh`, `self_host_stage1.sh`),
  - instrumenté par des artefacts dans `target/bootstrap/...`.

- Les stages s’articulent avec :
  - les **couches** du bootstrap (host, front, middle, back, cli, core, pipeline),
  - les **phases** de la roadmap (0 → 6),
  - les outils (`vittec`, `vitte-run`, `vitte-tools`).

- Ce document sert de vue **opérationnelle** des stages :  
  il décrit ce que chaque stage est censé vérifier/produire, indépendamment de la mise en œuvre concrète du système de build.

Il doit être mis à jour dès que :
- la structure des artefacts change,
- de nouveaux hooks/scripts sont ajoutés,
- la définition de ce que `stage1` ou `stage2` doit couvrir est modifiée.
