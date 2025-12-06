# Vitte core – Roadmap

Ce document décrit la **roadmap de haut niveau** du projet `vitte-core`, c’est‑à‑dire du langage Vitte, de son compilateur, de sa VM/runtime, de la std et des outils associés.

Il complète les autres documents de référence :

- `docs/overview.md` – vue d’ensemble du dépôt ;
- `docs/language-spec.md` – spécification du noyau du langage Vitte (édition 2025) ;
- `docs/compiler-architecture.md` – architecture du compilateur ;
- `docs/architecture-bootstrap.md` – architecture du bootstrap ;
- `docs/design-decisions.md` – décisions de design.

L’objectif de cette roadmap est de fournir :

- une vision claire des **phases** d’évolution de Vitte ;
- des **jalons** concrets (milestones) par sous-système ;
- une base pour structurer le travail (implémentation, docs, tooling, CI) ;
- un référentiel pour prioriser les tâches à moyen/long terme.

Cette roadmap porte uniquement sur Vitte et sa toolchain (aucun autre langage).

---

## 0. Principes de roadmap

1. La roadmap est organisée en **phases** logiques, pas en dates fixes.
2. Chaque phase a :
   - des objectifs principaux,
   - des jalons par sous-système (langage, compilateur, runtime, std, bootstrap, tooling, qualité).
3. Les phases peuvent se chevaucher, mais les dépendances majeures sont explicites.
4. L’édition de langage ciblée est : **Vitte 2025** (noyau).
5. La roadmap est **vivante** : elle doit être ajustée au fur et à mesure de l’avancement réel.

---

## 1. Vision globale

À horizon moyen terme, `vitte-core` doit fournir :

- un langage Vitte **noyau 2025** utilisable pour écrire du code réel ;
- un compilateur Vitte **auto‑hébergé** (self‑host) :
  - pipeline front/middle/back claire,
  - IRs structurés (AST, HIR, MIR, IR logique),
  - génération de bytecode Vitte stable ;
- une VM/runtime Vitte capable d’exécuter le bytecode du noyau 2025 ;
- une std minimale mais cohérente (collections, I/O, fs, path, string, time) ;
- un ensemble d’outils :
  - `vittec` (compilateur CLI),
  - `vitte-run` (runner VM),
  - `vitte-tools` (formatter, LSP helper, symbol browser, test runner…),
  - scripts et Makefile intégrés ;
- un bootstrap structuré en stages (0/1/2) correctement documenté ;
- une intégration minimale avec l’écosystème (VS Code, CI, packaging).

---

## 2. Axes principaux

Les évolutions de `vitte-core` se répartissent sur les axes suivants :

1. **Langage & Spec**
   - Spécification du noyau, grammaire, règles de typage.
2. **Compiler Front/Middle/Back**
   - Lexer, parser, AST, diagnostics, scopes/types, IRs, codegen, link.
3. **Runtime & VM**
   - VM bytecode, GC, std hooks, CLI `vitte-run`.
4. **Std**
   - Modules standard de base (collections, fs, io, path, string, time).
5. **Bootstrap & Auto‑hébergement**
   - Stages, manifests bootstrap, scripts, auto‑compilation du compilateur.
6. **Tooling & UX**
   - CLI, outils, VS Code, formatteur, tests/CI, packaging.
7. **Gouvernance & Légal**
   - éditions, versioning, licence, DMCA/takedown.

Chaque phase de la roadmap mentionne les jalons clés dans ces axes.

---

## 3. Phase 0 – Fondation du workspace (structure & docs)

**Objectif global :** poser la structure de base de `vitte-core` et la documentation d’architecture sans dépendre d’une implémentation complète.

### 3.1. Workspace & manifests

- Définir le workspace racine :
  - `muffin.muf` (workspace Vitte-only),
  - `vitte.project.muf` (vue projet complète).
- Définir les manifests bootstrap :
  - `bootstrap/mod.muf` (agrégation),
  - `bootstrap/host/front/middle/back/cli/core/pipeline/mod.muf`.
- Définir l’index de la std :
  - `src/std/mod.muf` (modules logiques).

### 3.2. Scripts & Makefile

- Créer les scripts génériques :
  - `scripts/env_local.sh`,
  - `scripts/bootstrap_stage0.sh`,
  - `scripts/self_host_stage1.sh`,
  - `scripts/clean.sh`.
- Créer un `Makefile` minimal mais structuré :
  - cibles `env`, `bootstrap-stage0`, `bootstrap-stage1`, `bootstrap-all`,
  - `mini_project`, `clean`, `distclean`, `fmt`, `lint`, `test` (placeholders).

### 3.3. Documentation

- Rédiger :
  - `docs/overview.md`,
  - `docs/architecture-bootstrap.md`,
  - `docs/compiler-architecture.md`,
  - `docs/design-decisions.md`,
  - `docs/language-spec.md` (noyau 2025 – brouillon stable),
  - `docs/roadmap.md` (ce fichier).

### 3.4. Outillage de base

- `.editorconfig` et `.gitignore` adaptés à Vitte.
- `.vscode/settings.json`, `launch.json`, `tasks.json` alignés avec les scripts.

**Sortie attendue de la phase 0 :** un repo structuré, documenté, “prêt à implémenter”, centré 100 % sur Vitte.

---

## 4. Phase 1 – Noyau du langage & front-end du compilateur

**Objectif global :** rendre possible la compilation de **programmes Vitte simples** jusqu’à l’AST/HIR, avec diagnostics de base.

### 4.1. Spécification & grammaire

- Stabiliser un premier noyau de syntaxes :
  - modules (`module`, `import`, `export`),
  - déclarations de base (`let`, `const`, `struct`, `enum`, `type`, `fn`),
  - contrôle (`if`, `else`, `loop`, `for`, `return`),
  - blocs explicites avec `.end`.
- Alignement de `docs/language-spec.md` avec les fichiers de grammaire :
  - grammaire Pest (`grammar/vitte.pest`),
  - EBNF de référence (si distinct).

### 4.2. Lexeur & parser

- `vitte.compiler.lexer` :
  - tokens pour identifiants, mots-clés, littéraux, opérateurs, `.end`,
  - gestion de l’indentation et des fins de ligne,
  - reporting d’erreurs lexicales avec spans.
- `vitte.compiler.parser` :
  - parsing des modules, imports, déclarations top‑level,
  - parsing des blocs, expressions simples, instructions de base,
  - récupération d’erreurs et diagnostics syntaxiques.

### 4.3. AST, spans et diagnostics

- `vitte.compiler.ast` :
  - nœuds pour modules, déclarations, instructions, expressions,
  - design extensible (ajout d’extensions ultérieures sans tout casser).
- `vitte.compiler.span` :
  - structure de `Span`, `FileId`, mapping lignes/colonnes.
- `vitte.compiler.diagnostics` :
  - modèle de diagnostic (code, niveau, message, span),
  - formatage CLI simple (texte brut).

### 4.4. Scope & symboles (niveau de base)

- `vitte.compiler.scope`, `vitte.compiler.symbols` :
  - scopes racine / module,
  - registre basique des symboles (variables, fonctions, types),
  - détection de symboles dupliqués ou non résolus pour des cas simples.

**Sortie attendue de la phase 1 :** un compilateur capable de parser des programmes simples, construire un AST, et produire des diagnostics lex/syntax/nommage de base (sans génération de bytecode).

---

## 5. Phase 2 – Type system & IRs (HIR/MIR/IR logique)

**Objectif global :** introduire un type system utilisable et les IRs, pour pouvoir raisonner sémantiquement sur les programmes Vitte et préparer le back-end.

### 5.1. Types & type checker

- `vitte.compiler.types` :
  - types de base (`bool`, `i32`, `u64`, `f64`, `string`, `()`),
  - structures, enums, unions simples,
  - modèle pour extensions futures (génériques, traits).

- `vitte.compiler.typecheck` :
  - typage des expressions simples (arithmétique, booléens, appels de fonctions),
  - vérification des signatures de fonctions et de la cohérence retour/`return`,
  - diagnostics de type (incompatibilités, symboles non typés, etc.).

### 5.2. HIR & MIR

- `vitte.compiler.hir` :
  - IR de haut niveau reprenant l’AST mais normalisé,
  - désugaring de constructions syntaxiques,
  - préparation aux analyses de flot.

- `vitte.compiler.mir` :
  - représentation en blocs de base,
  - contrôles explicites (`br`, `cond_br`, structure de CFG),
  - compatibilité avec les futures passes d’optimisation.

### 5.3. IR logique & analyses

- `vitte.compiler.ir` :
  - IR backend‑orienté, proche du bytecode mais indépendant des détails d’encodage,
  - structure des instructions et opérandes.

- Analyses :
  - `vitte.compiler.constant_fold` (évaluation de constantes),
  - `vitte.compiler.flow_analysis` (reachability, tests de code mort),
  - `vitte.compiler.optimize` (simplifications, DCE basique).

**Sortie attendue de la phase 2 :** capacité à typer et transformer un programme non trivial en IR logique, avec diagnostics sémantiques cohérents.

---

## 6. Phase 3 – Back-end, bytecode & VM minimale

**Objectif global :** amener la toolchain à produire du **bytecode Vitte exécutable** par une VM minimale.

### 6.1. Génération de bytecode

- `vitte.compiler.codegen.bytecode` :
  - mapping IR logique → instructions bytecode,
  - gestion des sauts et labels,
  - représentation du pool de constantes.

- `vitte.compiler.codegen.text` :
  - format textuel pour IR/bytecode,
  - outils d’inspection (`vitte-ir-dump`, `vitte-bytecode-emit`).

### 6.2. Lien logique

- `vitte.compiler.link` :
  - résolution cross‑module,
  - bundle de bytecode unique pour une application,
  - métadonnées nécessaires au runtime (points d’entrée, infos debug de base).

### 6.3. VM & runtime minimal

- `vitte.runtime.vm` :
  - boucle d’exécution de base (fetch/decode/execute),
  - gestion minimale de la stack et du heap.

- `vitte.runtime.bytecode` :
  - structures de représentation du bytecode,
  - primitives de chargement et d’itération.

- `vitte.runtime.cli.run` :
  - CLI `vitte-run` pour exécuter un bundle de bytecode.

### 6.4. Std minimale

- Implémenter un socle minimal dans `src/std` :
  - `std.io` (stdout/stderr),
  - `std.string`,
  - `std.time` (horloge simple).

**Sortie attendue de la phase 3 :** capacité à écrire un petit programme Vitte, le compiler en bytecode, et l’exécuter via `vitte-run` avec std minimale.

---

## 7. Phase 4 – Bootstrap complet & auto‑hébergement

**Objectif global :** rendre le compilateur **auto‑hébergé** (self‑host) via le bootstrap structuré (stage0/1/2).

### 7.1. Stage0 solide

- Finaliser et stabiliser `bootstrap/host/mod.muf` :
  - grammaire, fixtures, scripts,
  - rapports de validation dans `target/bootstrap/host/*`.

- `scripts/bootstrap_stage0.sh` :
  - logs détaillés,
  - vérifications et rapports plus riches (éventuellement validation AST/IR des échantillons).

### 7.2. Stage1 – vittec-stage1

- Implémenter un build pipeline pour produire un binaire **vittec-stage1** :
  - via des hooks `scripts/hooks/build_vittec_stage1.sh`,
  - aligné sur `bootstrap/middle/mod.muf`.

- Utiliser ce compilateur pour :  
  - compiler une partie du code Vitte du projet (ex. quelques modules du compilateur lui-même).

### 7.3. Stage2 – vittec-stage2 (self-host)

- Pipeline pour produire **vittec-stage2** (compilateur recompilé par vittec-stage1) :
  - hook `scripts/hooks/build_vittec_stage2.sh`,
  - intégration avec `self_host_stage1.sh`.

- Exposer `target/debug/vittec` comme symlink vers vittec-stage2.

### 7.4. Tests bootstrap & mini_project

- Utiliser `tests/data/mini_project` comme smoke test end‑to‑end :
  - compiler avec vittec-stage2,
  - exécuter avec `vitte-run`,
  - vérifier l’intégrité des diagnostics et du bytecode.

**Sortie attendue de la phase 4 :** compilateur Vitte auto‑hébergé pour un sous‑ensemble solide du langage noyau, avec bootstrap reproductible via scripts et Makefile.

---

## 8. Phase 5 – Std élargie & tooling développeur

**Objectif global :** rendre Vitte agréable à utiliser au quotidien via une std plus riche et un tooling de qualité.

### 8.1. Std élargie

- Étendre `std.collections` (vecteurs, maps, iterators) ;
- `std.fs` (files, dirs, paths) ;
- `std.path` (manipulation de chemins) ;
- `std.time` (horloges, durées) ;
- documentation minimale pour la std (docstring/primitives d’API doc).

### 8.2. Formatter & lint

- `vitte.tools.format` :
  - règles de formatage canonique (indentation, espaces, sauts de ligne),
  - intégration avec CLI (`vitte-tools fmt`) et VS Code.

- `vitte.tools.lint` :
  - lint de base (noms, code mort simple, style).

### 8.3. LSP & IDE

- `vitte.tools.lsp` :
  - serveur LSP minimal (hover, diagnostics, go‑to‑definition simple),
  - alignement avec la grammaire et les modules `vitte.compiler.*`.

- Extension VS Code dédiée Vitte :
  - syntax highlighting (`*.vitte`, `*.muf`, `*.lex`, `*.l`),
  - intégration LSP (si séparée),
  - commandes confort : run, test, format.

### 8.4. Test runner & coverage (optionnel)

- `vitte.tools.test_runner` :
  - exécution de tests Vitte,
  - reporting basique pour la CI.

- Support de coverage simple (bytecode level) à étudier.

**Sortie attendue de la phase 5 :** environnement de développement Vitte utilisable pour des projets réels (std raisonnable, formatter, lint, LSP, tests).

---

## 9. Phase 6 – Qualité, perf & packaging

**Objectif global :** consolider la qualité, améliorer les performances et préparer un minimum de packaging/écosystème.

### 9.1. Qualité & CI

- Intégration CI :
  - exécution des smoke tests bootstrap,
  - tests unitaires/IR/bytecode,
  - vérification de la doc (cohérence).

- Rapports :
  - artefacts de tests dans `target/core/tests/`,
  - dashboards simples (texte/HTML).

### 9.2. Performance

- Profiling compilateur :
  - mesurer les hot paths (lexing, parsing, IR passes, codegen).

- Profiling runtime/VM :
  - benchmark de micro‑scénarios Vitte,
  - optimisation ciblée (interpréteur, représentation du bytecode, GC).

### 9.3. Packaging & distribution (design)

- Définir un premier design pour :
  - un format de **paquet Vitte** (bundle de bytecode + metadata),
  - un manifest de package (peut être une extension de Muffin ou un fichier séparé),
  - un outil CLI pour gérer les paquets (`vitte-tools pkg`, futur).

**Sortie attendue de la phase 6 :** pipeline de qualité et perf raisonnable, début de réflexion/implémentation sur l’écosystème de paquets Vitte.

---

## 10. Axes de R&D futurs (au‑delà du noyau 2025)

Les points suivants sont identifiés comme **axes de recherche/développement** potentiels, à traiter dans des éditions ultérieures ou branches d’expérimentation :

- **Génériques et traits** avancés ;
- **Macros Vitte** (déclaratives/procédurales) ;
- **Async/await** ou modèle de concurrence (acteurs, CSP, etc.) ;
- IR SSA et analyses plus avancées ;
- Compilation AOT vers d’autres cibles (C, LLVM IR, etc.) via backends dédiés ;
- Outils avancés :
  - profiler Vitte,
  - debugger bytecode,
  - visualisation IR/CFG (graphique).

Ces axes ne sont pas dans le périmètre strict du noyau 2025, mais doivent être anticipés dans les design decisions (compatibilité future).

---

## 11. Gouvernance et mise à jour de la roadmap

- La roadmap doit être revue régulièrement (par exemple à chaque jalon significatif) et synchronisée avec :
  - `docs/design-decisions.md`,
  - `docs/language-spec.md`,
  - les manifests Muffin (ajout de modules, artefacts, profils).

- Toute modification majeure :
  - doit rester cohérente avec la philosophie Vitte (Vitte-only, blocs `.end`, IRs clairs, VM dédiée, std contractuelle),
  - doit être reflétée ici sous forme de nouvelle phase, de jalon ajouté ou de re‑priorisation.

Cette roadmap ne remplace pas les TODO techniques locaux dans le code, mais sert de **boussole globale** pour l’évolution de `vitte-core`.
