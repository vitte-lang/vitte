# Vitte – Design decisions

Ce document rassemble les **principales décisions de design** du projet Vitte telles qu’elles s’appliquent au dépôt `vitte-core` :

- philosophie générale du langage et de la toolchain ;
- choix de syntaxe, de sémantique et de représentation interne ;
- structure du workspace et manifests Muffin ;
- stratégie de bootstrap, runtime/VM, std et tooling ;
- règles de gouvernance (versionning, édition, compatibilité).

L’objectif est de fournir une référence stable sur le *pourquoi* derrière l’architecture décrite dans les autres documents (`architecture-bootstrap.md`, `compiler-architecture.md`, `README.md`, manifests `*.muf`, etc.), en restant **strictement centré sur Vitte** et en évitant toute dépendance conceptuelle à d’autres langages.

---

## 1. Philosophie générale

### 1.1. Langage unique, toolchain unique

- **Vitte est le seul langage concerné** par ce dépôt :
  - pas de surface publique en C, Rust, C++, etc. ;
  - pas d’API “officielle” qui exposerait d’autres langages au niveau du design.
- Le dépôt `vitte-core` modélise uniquement :
  - le langage Vitte,
  - sa toolchain (compilateur, runtime/VM, std, outils),
  - ses manifests, scripts, et architecture de bootstrap.

Toute mention à d’autres technologies (compilateurs externes, backends, etc.) doit rester **hors** de l’architecture conceptuelle. Si un bridge existe un jour, il sera modélisé comme un module Vitte/FFI clairement isolé.

### 1.2. Simplicité conceptuelle, puissance progressive

- Première cible : **simplicité lisible** pour un développeur ayant déjà vu un langage moderne.  
- Seconde cible : possibilité d’ajouter des concepts avancés **sans casser** la surface initiale.
- Le langage lui-même vise à être :
  - *lisible* (syntaxe claire, peu de symboles exotiques),
  - *structuré* (blocs explicites, indentation maîtrisée),
  - *progressif* (on peut écrire du simple comme du plus sophistiqué).

### 1.3. Déclaratif dès que possible

- Les manifests (Muffin) sont **purement déclaratifs** :
  - pas de logique de build impérative codée dans les manifests,
  - uniquement une description de la structure (projets, modules, artefacts, profils).
- Les scripts shell orchestrent mais ne définissent pas le cœur conceptuel :
  - ils appellent des hooks ou des binaries Vitte,
  - ils ne codent pas de règles complexes d’architecture.

---

## 2. Syntaxe et surface du langage

### 2.1. Blocs, indentation, pas d’accolades

Décision centrale :

- **Aucune accolade** (`{`, `}`) dans la syntaxe Vitte.
- Les blocs sont délimités par des structures explicites et un terminator dédié, de la forme `.end` (ou variantes dérivées décidées dans la grammaire officielle).
- L’indentation est significative mais :
  - la fermeture de bloc reste explicite via `.end` pour éviter toute ambiguïté,
  - l’indentation sert surtout à la lisibilité et à des règles simples (pas à implémenter toutes les contraintes structurelles).

Conséquences :

- Le code Vitte est visuellement hiérarchisé sans dépendre uniquement de l’indentation ;
- le parsing est plus simple à raisonner (terminators explicites) ;
- les éditeurs et outils peuvent manipuler des blocs sans heuristiques d’indentation trop complexes.

### 2.2. Mots-clés et structures de base

Le langage possède un ensemble de mots-clés réservés pour structurer le code et les manifests, par exemple (non exhaustif, conceptuel) :

- déclarations : `module`, `import`, `export`, `struct`, `enum`, `union`, `fn`, `type`, etc. ;
- contrôle : `if`, `when`, `else`, `loop`, `for`, `match`, `return`, etc. ;
- scénarios/programmes : `program`, `scenario`, `pipeline` (selon la grammaire officielle).

L’approche générale :

- préférer des mots-clés explicites (lisibles) plutôt que des symboles courts et ambigus ;
- éviter les surcharges lexicales (un mot-clé = un rôle clair).

### 2.3. Manifests Muffin

Les manifests Muffin (`*.muf`, `muffin.muf`, `vitte.project.muf`, `bootstrap/*.muf`, `src/std/mod.muf`) sont un **langage déclaratif** spécifique, mais doivent rester cohérents avec la philosophie Vitte :

- syntaxe inspirée des fichiers TOML/INI, mais alignée sur les besoins du projet ;
- structure en blocs `[section]`, `[section.subsection]`, avec tableaux et champs typés ;
- aucune logique impérative à l’intérieur (pas de scripts, pas de conditions).

Décisions :

- Utiliser les manifests Muffin comme *source unique de vérité* pour :
  - la structure du workspace,
  - les sous-projets,
  - les artefacts,
  - les profils (dev/release/ci, bootstrap, etc.).
- Refuser d’ajouter de la logique procédurale dans Muffin même si “pratique” à court terme.

---

## 3. Représentations internes (AST, IRs)

### 3.1. AST canonique

- L’AST Vitte (`vitte.compiler.ast`) est la **représentation canonique** de la structure syntaxique :
  - chaque construction syntaxique correspond à un nœud explicite,
  - les `Span` (`vitte.compiler.span`) sont attachés à chaque nœud pour les diagnostics.

Décisions :

- L’AST ne doit pas être trop “magique” : pas de nodes polymorphes sans typage clair.
- Les transformations lourdes (désugaring, normalisation) sont déportées en HIR/MIR.

### 3.2. HIR / MIR / IR

Pour garder une toolchain claire et extensible, Vitte adopte **plusieurs niveaux d’IR** :

- **HIR** :
  - proche de l’AST,
  - désucre les constructions syntaxiques complexes,
  - prépare le terrain pour des analyses globales.

- **MIR** :
  - structure en blocs de base, flux de contrôle explicite,
  - support privilégié des analyses de flot et de l’optimisation.

- **IR logique (backend IR)** :
  - prêt à être transformé en bytecode,
  - reflète la structure de la VM Vitte sans en être une copie brute.

Décision clé :

- Séparer clairement front (AST), middle (HIR/MIR), back (IR/bytecode) pour éviter un “gros bloc monolithique” difficile à faire évoluer.

---

## 4. Système de types et sémantique

### 4.1. Type system (principe)

Le système de types doit être :

- **statique** (détection des erreurs de type à la compilation) ;
- **expressif** (structures, enums, unions, types composites) ;
- **extensible** (ajout possible de génériques/traits plus tard).

Décisions de principe :

- Modéliser les types dans un module dédié (`vitte.compiler.types`) ;
- séparer la représentation des types de leur vérification (`vitte.compiler.typecheck`) ;
- conserver un chemin clair pour l’introduction de fonctionnalités avancées (génériques, traits, etc.) sans casser le modèle de base.

### 4.2. Safety et invariants

Même si tous les détails ne sont pas encore fixés, les invariants suivants guident les décisions :

- éviter les comportements “surprises” à la compilation et à l’exécution ;
- privilégier des règles de typage simples et documentées ;
- permettre des diagnostics *précis* avec spans et messages clairs.

---

## 5. Runtime/VM et bytecode

### 5.1. VM dédiée Vitte

Le runtime de Vitte est basé sur une **VM dédiée** (`vitte.runtime.vm`) qui exécute un **bytecode Vitte** (`vitte.runtime.bytecode`).

Décisions :

- Le format de bytecode n’est pas une copie d’un format existant : il est défini spécifiquement pour Vitte ;
- Le bytecode est pensé pour être :
  - compact,
  - relativement stable,
  - facilement inspectable via des outils (`vitte-ir-dump`, `vitte-bytecode-emit`).

### 5.2. Couplage compilateur/VM

- Le compilateur génère un IR logique qui se mappe **clairement** sur le bytecode ;
- la VM consomme ce bytecode sans dépendre des détails internes du compilateur (hir/mir, etc.) ;
- seules les conventions documentées (format des instructions, layout des frames, etc.) sont contractuelles.

Objectif : conserver une **frontière claire** entre compilateur et runtime, tout en gardant un pipeline cohérent.

---

## 6. Bibliothèque standard (std)

### 6.1. Std comme contrat minimal

La std Vitte (décrite par `src/std/mod.muf` et les modules `std.*`) définit un **socle minimal** que tout environnement d’exécution doit supporter.

Décisions :

- La std est modélisée comme un bundle logique dans les manifests (profil std-core) ;
- dans certains scénarios (bootstrap, tests), la std peut être précompilée en bytecode ;
- le compilateur ne doit pas “hardcoder” la std, mais la traiter comme une **bibliothèque** :
  - chargée par défaut,
  - versionnée via les manifests.

### 6.2. Domaines couverts

La std couvre au minimum :

- collections (vecteurs, maps, etc.) ;
- I/O de base ;
- système de fichiers (fs) ;
- chemins et manipulation de path ;
- manipulation de chaînes ;
- temps/horloges.

Tout ce qui dépasse ce socle doit être considéré comme un module additionnel (hors `std` cœur) ou un crate/bundle séparé dans l’écosystème futur.

---

## 7. Workspace, manifests et structure de projet

### 7.1. Workspace unique

Décision : **un workspace Vitte principal** pour ce dépôt (`vitte-core`) :

- `muffin.muf` à la racine décrit la vue globale ;
- `vitte.project.muf` ajoute la vue “projet” (compiler/runtime/std/tools/bootstrap/tests).

### 7.2. Manifests par couche / sous-système

Chaque sous-système structurant possède un manifest dédié :

- `bootstrap/*.muf` pour les couches de bootstrap ;
- `src/std/mod.muf` pour l’index de la std ;
- d’autres manifests seront ajoutés si nécessaire, mais toujours dans une logique **d’index et de structure**, pas de build.

### 7.3. Profils

Les profils (dev, release, ci, fast-dev, etc.) sont un **concept manifeste** :

- ils sélectionnent des sous-projets, binaries et artefacts pertinents pour un usage donné ;
- ils ne sont pas des “build modes” compilation au sens traditionnel (pas de flags) ;
- ils permettent aux outils d’adapter leur comportement (par exemple, la CI active plus d’artefacts/tests).

---

## 8. Bootstrap et auto-hébergement

### 8.1. Stages clairs

Décision : le bootstrap est structuré en **stages** :

- stage0 (host) ;
- stage1 (vittec-stage1) ;
- stage2 (vittec-stage2, self-host).

Ces stages sont modélisés dans `bootstrap/mod.muf` et la couche `middle`, et orchestrés par des scripts shell génériques.

### 8.2. Scripts agnostiques, hooks extensibles

Les scripts (`bootstrap_stage0.sh`, `self_host_stage1.sh`) ne fixent pas un “build system” externe :

- ils s’occupent de la **structure cible** (`target/bootstrap/...`, logs, reports) ;
- ils délèguent les builds concrets à des hooks (`scripts/hooks/build_vittec_stage1.sh`, etc.) ;
- ils se contentent de générer des `status.txt` placeholders si les hooks n’existent pas encore.

But : la **structure conceptuelle** du bootstrap doit rester valable quelles que soient les implémentations futures (toolchain Vitte native, wrappers, etc.).

---

## 9. Outillage, UX et intégration éditeur

### 9.1. CLI unifiée

Décision :

- `vittec` est le point d’entrée principal pour la compilation ;
- `vitte-run` exécute le bytecode ;
- `vitte-tools` agrège des utilitaires (formatter, LSP helper, symbol browser, test runner) ;
- `vitte-bootstrap` pilote les scénarios de bootstrap si besoin.

Ces binaries sont modélisés dans :

- `vitte.project.muf` (entrées `vittec`, `vitte-run`, `vitte-tools`, `vitte-bootstrap`) ;
- `bootstrap/cli/mod.muf` (couche CLI).

### 9.2. VS Code et tooling local

Le dépôt fournit des fichiers `.vscode/*` et `.editorconfig` avec des décisions claires :

- extensions de fichiers Vitte :
  - `*.vitte` → langage Vitte,
  - `*.muf`, `*.muffin` → manifests Muffin Vitte,
  - `*.lex`, `*.l` → grammaire/lexers Vitte ;
- indentation :
  - 4 espaces pour le code Vitte et les scripts,
  - 2 espaces pour les manifests et JSON/YAML/TOML ;
- exclusion des artefacts (`target/`, `dist/`, `node_modules/`) ;
- tasks et launch configs alignées sur les scripts de bootstrap (stage0, stage1+2).

Objectif : avoir un **environnement de travail cohérent** immédiatement après clonage du repo.

---

## 10. Versioning, édition et compatibilité

### 10.1. Editions

Décision : Vitte introduit la notion d’**édition** (par exemple `2025`) pour structurer les évolutions majeures du langage.

- Les manifests mentionnent `edition = "2025"` ;
- une édition fige :
  - la grammaire et la sémantique de base,
  - le contrat minimal de la std,
  - le format de bytecode (avec éventuellement une rétrocompatibilité).

### 10.2. Versions du projet

Le projet `vitte-core` a sa propre version (ex. `0.1.0`) indépendante de l’édition du langage :

- cette version concerne l’état de la toolchain (implémentations, features supportées) ;
- un changement d’édition peut entraîner un saut de version significatif, mais ce n’est pas strictement couplé.

### 10.3. Compatibilité

Principe général :

- éviter de casser les programmes existants dans une même édition ;
- réserver les évolutions incompatibles à des nouvelles éditions, avec des scripts/outils de migration possible.

---

## 11. Gouvernance, licence et marque

### 11.1. Gouvernance technique

Les décisions d’architecture de ce document servent de **contrat technique** pour toute évolution du dépôt `vitte-core` :

- toute refonte majeure doit rester compatible avec :
  - la séparation front/middle/back,
  - la structure AST/HIR/MIR/IR,
  - la modélisation manifests (workspace, projet, bootstrap, std),
  - l’existence d’une VM/bytecode Vitte dédiée.

### 11.2. Licence

- Le projet est distribué sous licence **MIT** (voir `LICENSE`).  
- Toute contribution doit accepter cette licence pour être intégrée.

### 11.3. DMCA / Takedown

Un document dédié (`docs/dmca-takedown.md`) décrit la procédure pour les demandes de retrait de contenu.

Décision :

- protéger la marque Vitte et la structure du projet contre une réutilisation abusive dans d’autres dépôts publics ;
- fournir une adresse de contact (`legal@vitte.dev` dans la doc publique) pour les demandes formelles.

---

## 12. Axes d’évolution prévus

Même si ce document vise à stabiliser les décisions de design, certains axes sont explicitement identifiés comme évolutifs :

- enrichissement du langage (macros, traits, async, etc.) ;
- amélioration du système de types (génériques, type inference globale, effets) ;
- IR plus riche (SSA, analyses avancées) ;
- tooling plus poussé (LSP complet, formatter robuste, coverage, profiling bytecode/VM) ;
- packaging et distribution (écosystème de paquets Vitte).

Ces évolutions devront toutefois respecter :

- la séparation claire front/middle/back,
- le rôle des manifests Muffin,
- l’existence d’une VM/bytecode Vitte autonome,
- la philosophie “Vitte-only” de ce dépôt.

---

## 13. Résumé

- Vitte est un langage unique avec sa toolchain dédiée (compilateur, runtime/VM, std, outils), sans dépendance conceptuelle à d’autres langages.
- La syntaxe évite les accolades, s’appuie sur des blocs explicites et un terminator `.end`, avec une indentation significative mais non ambigüe.
- Les représentations internes sont structurées (AST, HIR, MIR, IR) pour garder le compilateur modulaire et évolutif.
- Le runtime est une VM dédiée, alimentée par un bytecode Vitte conçu spécialement pour elle.
- La std est un contrat minimal, modélisé comme un bundle logique décrit par des manifests Muffin.
- La structure du workspace repose entièrement sur des manifests déclaratifs (`muffin.muf`, `vitte.project.muf`, `bootstrap/*.muf`, `src/std/mod.muf`), complétés par quelques scripts shell neutres et extensibles via hooks.
- Le bootstrap est organisé en stages (0, 1, 2) et en couches (host, front, middle, back, cli, core, pipeline).
- L’outillage (CLI, VS Code, Makefile) est aligné sur cette architecture pour fournir une UX cohérente.
- Les notions d’édition, de version et de compatibilité encadrent l’évolution du langage et de la toolchain.
- Les aspects gouvernance, licence et DMCA sont intégrés dès la conception pour protéger le projet et clarifier son utilisation.

Ce document doit être mis à jour lorsque des décisions majeures sont prises, afin de rester la référence de **haut niveau** pour la conception de Vitte et de sa toolchain.
