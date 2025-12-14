# Architecture du compilateur Vitte

Ce document décrit l’architecture du **compilateur Vitte** telle qu’elle est modélisée dans le workspace `vitte-core`.  
Il complète les autres documents d’architecture (notamment `docs/architecture-bootstrap.md`) et les manifests Muffin (`muffin.muf`, `vitte.project.muf`, `bootstrap/*.muf`, `src/std/mod.muf`).

L’objectif est de fournir une vue claire, stable et **purement centrée sur Vitte** de la toolchain de compilation :

- pipeline de compilation (de la source Vitte jusqu’au bytecode et à l’exécution) ;
- découpage en modules (`vitte.compiler.*`) et en couches (front / middle / back) ;
- représentation des programmes (AST, HIR, MIR, IR) ;
- diagnostics, span, scopes, symboles, types ;
- intégration avec le runtime/VM, la std, les outils et le bootstrap.

---

## 1. Vue d’ensemble

Le compilateur Vitte est structuré en **trois grandes couches** :

1. **Front-end**  
   Transforme le texte source Vitte en une représentation structurée (AST), enrichie de spans, diagnostics et informations de symboles de base.

2. **Middle-end**  
   Enrichit, analyse et transforme le programme :
   - construction de représentations intermédiaires (HIR, MIR, IR logique) ;
   - typage, vérifications sémantiques, simplifications et optimisations ;
   - préparation pour l’émission de bytecode.

3. **Back-end**  
   Produit des artefacts exécutables :
   - IR final adapté au runtime/VM ;
   - bytecode Vitte ;
   - format texte IR/bytecode à des fins de debug et de tests ;
   - lien logique pour rassembler plusieurs unités de compilation.

Le tout est orchestré par un **CLI** unique (`vittec`) et un ensemble d’outils spécialisés (IR dump, analyseurs, etc.), modélisés principalement par les modules `vitte.compiler.cli.*` et `vitte.compiler.back.*`.

Dans les manifests Muffin, cette architecture est principalement représentée par les modules suivants :

- `vitte.compiler.lexer`
- `vitte.compiler.parser`
- `vitte.compiler.ast`
- `vitte.compiler.span`
- `vitte.compiler.diagnostics`
- `vitte.compiler.scope`
- `vitte.compiler.symbols`
- `vitte.compiler.types`
- `vitte.compiler.hir`
- `vitte.compiler.mir`
- `vitte.compiler.typecheck`
- `vitte.compiler.constant_fold`
- `vitte.compiler.flow_analysis`
- `vitte.compiler.optimize`
- `vitte.compiler.ir`
- `vitte.compiler.codegen.bytecode`
- `vitte.compiler.codegen.text`
- `vitte.compiler.link`
- `vitte.compiler.cli.*`

Ces noms peuvent évoluer, mais la **séparation conceptuelle** front / middle / back doit rester stable.

---

## 2. Pipeline de compilation

La pipeline de compilation Vitte suit les étapes logiques suivantes :

1. **Lecture des sources et manifests**  
   - Localisation des fichiers `.vitte` à compiler ;
   - Interprétation des manifests Muffin (`*.muf`) pour :
     - déterminer les unités de compilation ;
     - assembler les dépendances (std, autres modules du projet) ;
     - choisir les profils (dev / release / ci).

2. **Lexing** (`vitte.compiler.lexer`)  
   - Analyse des caractères en **tokens** ;
   - Gestion des espaces significatifs / indentation ;
   - Détection des commentaires, littéraux, identifiants, mots-clés, opérateurs.

3. **Parsing** (`vitte.compiler.parser`)  
   - Transformation de la séquence de tokens en **AST** (Abstract Syntax Tree) ;
   - Gestion des règles de grammaire Vitte ;
   - Construction d’une structure hiérarchique : modules, déclarations, blocs, expressions, patterns, etc.

4. **Construction AST + spans**  
   - AST défini dans `vitte.compiler.ast` ;
   - spans (positions dans le fichier source) dans `vitte.compiler.span` ;
   - association systématique de chaque nœud AST à un `Span` (début/fin, fichier, etc.).

5. **Diagnostics initiaux** (`vitte.compiler.diagnostics`)  
   - Erreurs lexicales, syntaxiques, erreurs de structure de base ;
   - Warnings élémentaires (code mort évident, utilisation de symboles inconnus lors du parse, etc.) ;
   - Représentation des diagnostics avec :
     - code, message, niveau (error/warn/info),
     - span ciblé,
     - éventuels snippets et notes d’aide.

6. **Résolution des noms et scopes** (`vitte.compiler.scope`, `vitte.compiler.symbols`)  
   - Construction de la table des symboles ;
   - Résolution lexicale ascendante ;
   - Gestion de la visibilité (module, public, privé) ;
   - Détection de collisions de noms, d’ombres, de symboles manquants.

7. **Type system et type checking** (`vitte.compiler.types`, `vitte.compiler.typecheck`)  
   - Représentation des types de base, des types composites, des génériques (si présents) ;
   - Inférence de types locale et propagation ;
   - Vérification de la compatibilité des types (assignations, appels, opérations) ;
   - Détails spécifiques au langage Vitte (nullabilité, ownership, etc. – à préciser dans la spéc du langage).

8. **HIR / MIR / IR logique**  
   - **HIR** (`vitte.compiler.hir`) : IR de haut niveau proche de l’AST mais normalisée ;
   - **MIR** (`vitte.compiler.mir`) : IR de niveau intermédiaire, plus proche des constructions de contrôle (branches, loops, blocs de base) ;
   - **IR logique** (`vitte.compiler.ir`) : forme prête à être transformée en bytecode ou en d’autres représentations.

9. **Passes d’analyses et d’optimisation**  
   - `vitte.compiler.constant_fold` : évaluation partielle des expressions constantes ;
   - `vitte.compiler.flow_analysis` : analyse de flot de contrôle/données (reachability, liveness, etc.) ;
   - `vitte.compiler.optimize` : passes d’optimisation (simplification, DCE, etc.).

10. **Back-end / codegen**  
    - `vitte.compiler.codegen.bytecode` : transformation de l’IR logique en **bytecode Vitte** ;
    - `vitte.compiler.codegen.text` : formats textuels de debug/IR/bytecode ;
    - `vitte.compiler.link` : lien logique des unités de compilation en un bundle exécutable (pour la VM).

11. **Émission des artefacts**  
    - Bytecode (fichier ou bundle) ;
    - IR textuel (dumps pour tests et debug) ;
    - rapports de compilation (diagnostics, stats, logs).  

12. **Exécution (via runtime/VM)**  
    - Le bytecode produit est ensuite exécuté par le runtime/VM Vitte (`vitte.runtime.*`) via `vitte-run`.

Cette pipeline est reflétée **de manière déclarative** dans `bootstrap/pipeline/mod.muf` et les différents manifests de couche.

---

## 3. Front-end

### 3.1. Lexeur (`vitte.compiler.lexer`)

Responsabilités :

- Transformer la chaîne d’entrée en séquence de tokens ;
- Gérer :
  - les littéraux (entiers, flottants, chaînes, booléens, …) ;
  - les identifiants ;
  - les mots-clés réservés propres à Vitte ;
  - les opérateurs et délimiteurs ;
  - l’indentation et la structure de blocs (spécifique à Vitte) ;
  - les commentaires et la gestion des espaces.

Le lexeur doit être **strictement déterministe** et produire des spans fiables pour chaque token afin de permettre un reporting de diagnostics précis.

### 3.2. Parser (`vitte.compiler.parser`)

Responsabilités :

- Lire la séquence de tokens pour construire l’AST ;
- Appliquer la grammaire Vitte (telle que définie dans les fichiers de grammaire et la spéc) ;
- Supporter :
  - les modules, imports, exports ;
  - les déclarations de types (struct, enum, union, …) ;
  - les fonctions et scénarios ;
  - les expressions (arithmétiques, booléennes, lambda, etc.) ;
  - les instructions (if/when, loop, return, etc.).

Le parser s’appuie sur les modules :

- `vitte.compiler.ast` pour les structures de données ;
- `vitte.compiler.span` pour l’association aux régions de texte.

### 3.3. AST (`vitte.compiler.ast`) et spans (`vitte.compiler.span`)

L’AST représente la structure syntaxique **canonique** des programmes Vitte.  
Chaque nœud AST doit être accompagné :

- d’un **Span** indiquant :
  - le fichier (ou l’ID de fichier),
  - la position de début/fin (ligne, colonne, offset),
- d’**annotations** permettant d’attacher des informations supplémentaires (résolution, types, attributs, etc. – via d’autres structures).

Le module `vitte.compiler.span` définit les abstractions communes autour des positions et ranges dans le texte.

### 3.4. Diagnostics (`vitte.compiler.diagnostics`)

Le système de diagnostics doit être **centralisé** et **stateless côté données** :

- Représentation des diagnostics (erreurs, warnings, infos) ;
- Codes d’erreurs bien définis ;
- Possibilité de formater les messages pour différents front-end (CLI, IDE, LSP) ;
- Fonctionnalités d’enrichissement :
  - snippets de code ;
  - notes de contexte ;
  - suggestions de correction (fix-it hints, dans une phase ultérieure).

Les diagnostics sont collectés au fil des phases (lexing, parsing, résolution, typing, IR) et peuvent être exportés sous forme de rapports (par exemple dans `target/core/tests/report.txt` ou `front-diag-report` côté bootstrap).

---

## 4. Résolution des noms, scopes et symboles

Modules concernés : `vitte.compiler.scope`, `vitte.compiler.symbols`.

### 4.1. Scopes

Un **scope** Vitte représente un environnement de noms valable pour une zone de code donnée :

- scopes racine (module, fichier) ;
- scopes de type (struct, enum, union, etc.) ;
- scopes de fonction (paramètres, variables locales) ;
- scopes de bloc (if, loop, etc.).

Le système de scopes doit gérer :

- l’imbrication (chaîne lexicale) ;
- l’ombre (shadowing) ;
- la visibilité (public, privé, module).

### 4.2. Symboles

Les **symboles** représentent les entités nommées du programme :

- variables, constantes ;
- fonctions ;
- types (struct, enum, union, type alias) ;
- modules et imports ;
- autres entités du langage (scénarios, pipelines, etc. selon les features Vitte).

Les symboles doivent être attachés à :

- un identifiant stable (SymbolId ou similaire) ;
- un type (instancié ou schématique selon la phase) ;
- un span de déclaration ;
- un scope parent.

La résolution des noms prend l’AST, parcourt les déclarations et les références, et établit ces liens symboliques.

---

## 5. Types et typage

Modules principaux : `vitte.compiler.types`, `vitte.compiler.typecheck`.

### 5.1. Représentation des types

Le système de types Vitte doit modéliser au minimum :

- types de base (bool, int, float, string, …) ;
- types composés (tuples, arrays, maps, etc.) ;
- types structurés (struct, enum, union, alias) ;
- (optionnel) types génériques (avec paramètres de type) ;
- (optionnel) références, pointeurs, ownership/borrowing (selon les choix de design du langage).

Les types peuvent être :

- déclarés explicitement dans le code ;
- inférés localement (dans des expressions, initialisations, etc.).

### 5.2. Inférence et vérification

Le **type checker** prend en entrée :

- l’AST annoté (symboles résolus) ;
- les informations de types déclarés.

Il produit :

- des contraintes de types ;
- des substitutions (lorsque l’inférence a lieu) ;
- des diagnostics en cas d’incompatibilité.

Exemples de vérifications :

- cohérence des types dans les assignations ;
- types d’arguments et type de retour pour les appels de fonctions ;
- exhaustivité des patterns (selon le design) ;
- vérifications spécifiques Vitte (null safety, aliasing, etc.).

---

## 6. Représentations intermédiaires (HIR, MIR, IR)

Modules principaux : `vitte.compiler.hir`, `vitte.compiler.mir`, `vitte.compiler.ir`.

### 6.1. HIR (High-level IR)

Le HIR est proche de l’AST mais :

- normalisé (des syntaxes sucrées sont déjà désucrées) ;
- simplifié pour la suite de la pipeline ;
- structuré pour faciliter les analyses globales.

Exemples de normalisation :

- transformation de constructions complexes en séquences de constructions plus simples ;
- désugaring de certaines formes syntaxiques (boucles, patterns).

### 6.2. MIR (Mid-level IR)

Le MIR est plus proche de l’implémentation :

- représentation en blocs de base ;
- contrôle explicite :
  - branches conditionnelles,
  - jumps,
  - loops explicites ;
- variables temporaires intermédiaires.

Il sert de support principal à :

- l’analyse de flot (`vitte.compiler.flow_analysis`) ;
- la simplification (`vitte.compiler.optimize`) ;
- la préparation pour le codegen (bytecode).

### 6.3. IR logique (backend IR)

L’IR logique (modélisée dans `vitte.compiler.ir`) est la représentation juste avant codegen :

- instructions proches du bytecode Vitte, mais encore indépendantes de certains détails du runtime ;
- modèle linéaire ou en blocs, selon les choix d’implémentation ;
- suffisamment expressive pour être sérialisée (format texte) et transformée (lowerings supplémentaires).

### 6.4. IR unique SSA léger (strates middle/back)

Objectif « 100 % Vitte full » : converger vers **une IR unique** pour le middle/back, en forme **SSA léger** :

- blocs de base + terminators structurés (`br`, `br_if`, `return`) ; pas de `goto` arbitraire ;
- identifiants de valeurs uniques (SSA) avec une seule définition, `phi` limité aux jonctions structurées ;
- types portés sur chaque valeur après résolution (primitives, structs/enums/alias selon `docs/type-system.md`) ;
- side effects explicites via instructions dédiées (`call`, `store_field`, `alloc_heap`), pas d’effets cachés dans des expressions.

Pipeline minimal :

1. **Résolution des types de base** : annotation des nœuds MIR avec les types résolus (primitives, nominaux, alias transparents) en s’appuyant sur `vitte.compiler.types`.
2. **Structuration du contrôle** : conversion des `if`/`while`/`match` en CFG structuré (blocs, terminators). Les `match` sont abaissés en cascades de tests + blocs de payload.
3. **SSA léger** : réécriture en SSA (références uniques, `phi` aux jonctions) sans chercher des optimisations globales (pas de rename agressif ni de DCE avancé).
4. **Pré-lowering bytecode** : normalisation des opérations (arith, comparaisons, accès champ, load/store local) pour que chaque instruction IR ait un équivalent direct ou quasi-direct dans la VM.

Deliverables associés :

- dump texte IR unique (utilisé par `vitte-ir-dump`) ;
- API `vitte.compiler.ir` pour itérer sur les blocs/instructions et insérer des passes ultérieures (fold, DCE léger) sans rompre la SSA.

### 6.5. Pipeline structuré (parse → AST → IR SSA → opt → backend)

Phases et entrées/sorties attendues :

- `Parse` : `Token` → `AstNode` (+ `Span`, diagnostics syntaxe). Aucun symbol/type connu.
- `AST enrichi` : AST fermé et annoté (modules/imports résolus, attributs). Invariants : arbre sans références croisées, spans complets.
- `IR SSA build` : CFG en SSA unique. Entrées : AST résolu (noms/types si dispo). Sorties : `Module { functions }`, chaque `Function` est SSA valide (une définition par valeur).
- `Optimisations` : passes locales/globales sur l’IR SSA (fold, DCE, simplif branches). Invariants : SSA conservé, CFG sans blocs morts, typage stable.
- `Backend` : abaissement IR → bytecode/VM. Invariants : uniquement des instructions connues du backend, blocs ordonnés (RPO/topo), pool de constantes fermé.

Guides de transition :

- AST → IR : toute structure de contrôle devient blocs + terminators. Pas d’effets implicites ni de captures implicites.
- IR → Opt : recalculer/valider les dérivés CFG (dominateurs, liveness) après modifications.
- Opt → Backend : interdiction de laisser des instructions « abstract » non supportées par la VM ; normaliser les types (pas de `Unknown` si le backend exige du concret).

### 6.6. Définition SSA (IR unique)

Identifiants et blocs :

- `FuncId`, `BlockId`, `ValueId` uniques par module.
- `Block { id, params: Vec<(ValueId, Type)>, instrs: Vec<Instr>, term: Terminator }` ; les `params` jouent le rôle de `phi`.
- Terminators autorisés : `Jump { target, args }`, `Branch { cond, then_tgt, then_args, else_tgt, else_args }`, `Return { value? }`, `Unreachable`.

Invariants SSA :

- Un bloc d’entrée unique par fonction (sans prédécesseur) ; ses `params` = args formels.
- Un bloc = 0..n instructions puis exactement 1 terminator.
- Toute utilisation est dominée par sa définition ; une seule définition par `ValueId`.
- Les `params` de bloc sont immuables et alimentés par chaque arête sortante (`Jump`/`Branch`).
- Types portés sur chaque valeur ; `Return` respecte le type de sortie déclaré.

Analyses dérivées (à maintenir après chaque passe CFG) :

- `DomTree`/`IDom` + `DominanceFrontier` pour placer/valider les `params`/phi.
- `Liveness` SSA (par bloc) pour DCE et scheduling ; si des accès mémoire persistent, suivre séparément `MemLoc`.
- `UseDef`/`DefUse` pour propager les constantes/copier et compter les usages.

Jeu d’instructions minimal (extensible) :

- Scalaires : `Const*`, `Unary(Neg/Not)`, `Binary(Add/Sub/Mul/Div/Mod/And/Or)`, `Cmp(Eq/Ne/Lt/Le/Gt/Ge)`, `Select` (optionnel).
- Effets : `Call { callee, args }`, `StoreVar/LoadVar` si des slots existent (à supprimer avant le backend si SSA strict), `Cast` optionnel.
- Terminators listés ci-dessus.

---

## 7. Passes d’analyse et d’optimisation

Modules principaux : `vitte.compiler.constant_fold`, `vitte.compiler.flow_analysis`, `vitte.compiler.optimize`.

### 7.1. Constant folding

`vitte.compiler.constant_fold` :

- Évalue à la compilation les expressions dont la valeur est connue à l’avance ;
- Simplifie les arbres d’expressions (par exemple, 2 + 3 → 5) ;
- Peut fonctionner sur l’AST, le HIR ou le MIR selon les besoins.

### 7.2. Analyse de flot

`vitte.compiler.flow_analysis` :

- Construction de graphes de contrôle (CFG) ;
- Analyse de reachability (code mort, branches inaccessibles) ;
- Analyse éventuelle de liveness et d’autres propriétés (pour des optimisations ultérieures).

### 7.3. Optimisation

`vitte.compiler.optimize` regroupe des passes de simplification :

- Dead Code Elimination (DCE) ;
- simplification de branches triviales ;
- fusion de blocs ;
- autres transformations “safe”, conservant la sémantique.

L’objectif principal n’est pas l’optimisation agressive, mais un **nettoyage** de la représentation avant émission du bytecode.

---

## 8. Back-end et génération de code

Modules principaux : `vitte.compiler.codegen.bytecode`, `vitte.compiler.codegen.text`, `vitte.compiler.link`.

### 8.1. Génération de bytecode

`vitte.compiler.codegen.bytecode` :

- Transforme l’IR logique (ou MIR adapté) en **bytecode Vitte** ;
- Assigne des indices d’instruction ;
- Résout les sauts, appels, accès à la stack ou aux registres logiques (selon le modèle de VM Vitte) ;
- Gère la représentation des constantes (pool de constantes, etc.).

Le format de bytecode :

- est conçu spécifiquement pour la VM Vitte (`vitte.runtime.vm`, `vitte.runtime.bytecode`) ;
- peut être sérialisé en fichiers `.vbc` (ou format équivalent) ou en bundles (pour des modules/paquets entiers).

### 8.2. Formats texte

`vitte.compiler.codegen.text` :

- Fournit des sorties **textuelles** lisibles (dumps IR/bytecode) ;
- Sert pour :
  - les outils (vitte-ir-dump, vitte-bytecode-emit côté back) ;
  - les tests (comparaison de golden files) ;
  - le debug (visualisation du flux de contrôle, etc.).

Exemples de formats :

- IR human-readable ;
- représentation de CFG (graphviz ou format logique) ;
- listing de bytecode annoté.

### 8.3. Lien logique

`vitte.compiler.link` :

- Effectue un **lien logique** entre plusieurs unités de compilation Vitte ;
- Résout les symboles cross-module ;
- prépare un bundle cohérent pour le runtime ;
- génère un output unique (fichier de bytecode ou bundle structuré) destiné à être consommé par `vitte-run`.

### 8.4. VM bytecode minimal (arith, contrôle, call, alloc)

Pour le MVP, la VM expose un bytecode **compact et linéaire**, calé sur l’IR SSA léger :

- **Modèle registre/stack hybride** : chaque instruction consomme des registres logiques numérotés (issus de la SSA) et peut pousser/pop sur la stack VM pour les appels.
- **Constantes et accès** : `const` (pool), `load_local`, `store_local`, `load_field`, `store_field`, `load_global`.
- **Arith/comparaisons** (types primitifs alignés sur `docs/type-system.md`) : `add`, `sub`, `mul`, `div`, `mod`, `neg`, `cmp_eq`, `cmp_ne`, `cmp_lt`, `cmp_le`, `cmp_gt`, `cmp_ge`.
- **Contrôle** : terminators `jmp`, `jmp_if`, `ret`; les structures haut niveau (`if`, `while`, `match`) sont déjà abaissées en CFG structuré, donc seuls ces terminators sont nécessaires.
- **Appels** : `call` (fonction connue), `call_indirect` (facultatif), passage d’arguments via registres/stack, retour via registre cible.
- **Allocation et gestion mémoire** : `alloc_heap` (struct/array/closure minimal), `alloc_stack` (frames locales), `move`/`copy` explicites selon la sémantique choisie (pas de GC implicite caché).

Lowering IR → bytecode (pipeline minimal) :

1. **Mapping SSA → registres** : chaque valeur SSA obtient un registre stable (pas de register allocation complexe au MVP).
2. **Émission séquentielle** : blocs IR linéarisés, terminators traduits en `jmp`/`jmp_if` avec résolutions d’offsets.
3. **Tables auxiliaires** : pool de constantes, table des fonctions (ids, signatures), table des types runtime.
4. **Format sérialisé** : header + tables + instructions, consommés par `vitte.runtime.bytecode` et exécutés par `vitte.runtime.vm`.

Cette spec reste minimale : pas d’optimisations (peu ou pas de DCE), pas de GC avancé, pas de JIT. L’accent est mis sur la **prédictibilité** des instructions et l’alignement strict avec l’IR SSA léger.

---

## 9. Intégration avec le runtime/VM et la std

Le compilateur Vitte est étroitement couplé au runtime/VM et à la bibliothèque standard.

### 9.1. Runtime/VM (`vitte.runtime.*`)

- `vitte.runtime.vm` : cœur de la VM (boucle d’exécution, gestion de la stack, etc.) ;
- `vitte.runtime.bytecode` : format et primitives pour manipuler le bytecode ;
- `vitte.runtime.gc` : gestion mémoire ;
- `vitte.runtime.std_hooks` : hooks vers la std (I/O, FS, temps, etc.) ;
- `vitte.runtime.cli.run` : CLI `vitte-run`.

Le compilateur doit :

- respecter le format attendu par `vitte.runtime.bytecode` ;
- fournir des métadonnées suffisantes (par exemple, infos de debug, layout) pour une exécution correcte.

### 9.2. Standard library (`src/std/*`)

La std Vitte (modélisée par `src/std/mod.muf` et les modules `std.*`) :

- fournit des types et fonctions de base accessibles aux programmes ;
- peut être **pré-compilée** en bytecode dans certains scénarios (bootstrap, tests) ;
- sert de référence pour la compatibilité future du compilateur (contrat minimal que le runtime doit supporter).

Dans les manifests, on retrouve des artefacts liés à la std (bundles, IR/bytecode) dans les couches `middle`, `core` et `back`.

---

## 10. CLI et outils

Modules principaux : `vitte.compiler.cli.*`, et côté back `vitte.compiler.back.*` (dans la couche bootstrap `back`).

### 10.1. CLI principal : `vittec`

Le binaire logique `vittec` est modélisé par :

- `vitte.compiler.cli.main` ;
- `vitte.compiler.cli.args` ;
- `vitte.compiler.cli.subcommands` ;
- `vitte.compiler.cli.diagnostics`.

Responsabilités :

- lecture des arguments ligne de commande ;
- sélection du mode d’opération (`build`, `check`, `emit-ir`, `emit-bytecode`, etc.) ;
- pilotage des différentes phases du compilateur ;
- gestion de la sortie diagnostics (format CLI/LSP-friendly).

Sous-commandes MVP alignées avec les manifests Muffin (`vitte.project.muf`) :

- `vittec build <manifest.muf>` : charge un manifest Muffin (projet utilisateur ou `vitte.project.muf`), résout la std `src/std/mod.muf`, produit un bundle bytecode dans `target/core/bytecode/` (profil `dev`) avec métadonnées runtime (entrypoint, version bytecode).
- `vittec run <manifest.muf>` : fait un `build` dans un dir temporaire puis invoque `vitte-run` sur le bundle déclaré, en injectant la std minimale. Sert d’entrée unique pour les exemples/tests smoke.
- `vittec fmt <paths…>` : placeholder (retourne 0, message informatif), sera branché sur `vitte.tools.format`.
- `vittec test` : smoke tests, compile `tests/data/mini_project` et exécute via `vitte-run`, écrit un rapport logique dans `target/core/tests/report.txt`.

Les artefacts écrits par ces commandes sont décrits dans les manifests (profil `dev` par défaut) et réutilisés par les scripts de bootstrap (`bootstrap/mod.muf`, `bootstrap/cli/mod.muf`).

### 10.2. Outils spécialisés

Côté back, des binaries logiques comme :

- `vitte-ir-dump` ;
- `vitte-bytecode-emit` ;
- `vitte-link-check`.

s’appuient sur :

- `vitte.compiler.back.ir_dump` ;
- `vitte.compiler.back.bytecode_emit` ;
- `vitte.compiler.back.link_inspect` ;

et sur `vitte.compiler.codegen.*` / `vitte.compiler.link`.

Ces outils permettent :

- de visualiser l’IR/bytecode ;
- de tester la validité des liens et des bundles ;
- d’automatiser des checks dans la CI.

---

## 11. Diagnostics, logs et tests

### 11.1. Diagnostics et logs

Les diagnostics sont produits à tous les niveaux du compilateur ; ils sont :

- agrégés dans des structures internes (listes/collections) ;
- exposés au CLI sous forme d’erreurs/warnings avec spans ;
- optionnellement sérialisés en rapports (par exemple dans `target/core/tests/report.txt` ou dans des artefacts de couche bootstrap).

Les logs (bootstrap, stage1/2, etc.) sont gérés principalement par les scripts shell et, à terme, par des modules de logging Vitte.

### 11.2. Stratégie de tests

Les tests du compilateur s’appuient sur :

- des **fixtures** sous `tests/data/` (mini projects, échantillons, cas limites) ;
- des scripts et/ou outils dédiés (test runner Vitte via `vitte.tools.test_runner`) ;
- des dumps IR/bytecode comparés à des fichiers de référence (golden files).

Les manifests Muffin dans `bootstrap/` et `vitte.project.muf` peuvent référencer des artefacts de test (`core-test-report`, `bytecode-tests`, etc.), ce qui permet à la CI de vérifier globalement l’état de la toolchain.

---

## 12. Interaction avec le bootstrap

Le compilateur Vitte ne vit pas isolé ; il est au centre du **bootstrap** décrit dans `bootstrap/*.muf` :

- la couche **middle** modélise `vittec-stage1` et `vittec-stage2` ;
- la couche **core** offre une vue globale sur les projets compiler/runtime/std/tools ;
- la couche **back** se concentre sur l’IR/bytecode/link pour ces compilateurs bootstrap ;
- la couche **cli** donne une vue sur les commandes `vittec`, `vitte-run` et `vitte-tools` utilisées dans le bootstrap ;
- la couche **pipeline** décrit l’enchaînement des étapes (stage0 → stage1 → stage2, etc.).

Le modèle **conceptuel** du compilateur (décrit ici) doit rester compatible avec ces manifests, même si les détails d’implémentation évoluent.

---

## 13. Évolutions possibles

La structure décrite ci-dessus est pensée pour être **stable**, mais plusieurs axes d’évolution sont prévus :

- enrichissement de la grammaire et des features du langage Vitte ;
- extension du système de types (génériques plus riches, traits/interfaces, effets, etc.) ;
- raffinement de la hiérarchie IR (SSA, analyses plus avancées) ;
- intégration de macros Vitte (voir modules `vitte.core.macros` lorsque définis) ;
- outillage plus complet :
  - LSP dédié Vitte,
  - formatter Vitte (outil de formatage automatique),
  - symbol browser,
  - documentation générée automatiquement à partir des sources.

L’architecture **front/middle/back**, les modules principaux (`vitte.compiler.*`) et leur découplage du runtime/VM restent des invariants.

---

## 14. Résumé

- Le compilateur Vitte est organisé en couches front / middle / back, pilotées par un CLI (`vittec`) et des outils spécialisés.
- La pipeline de compilation va de la source Vitte (lexer/parser/AST) jusqu’au bytecode (via HIR, MIR, IR, passes d’analyse et d’optimisation).
- Les modules `vitte.compiler.*` modélisent chaque aspect du compilateur : lexing, parsing, AST, spans, diagnostics, scopes, symboles, types, IRs, codegen, link, CLI.
- Les manifests Muffin (`muffin.muf`, `vitte.project.muf`, `bootstrap/*.muf`, `src/std/mod.muf`) décrivent de manière déclarative la structure de ces composants, leurs projets, binaries et artefacts.
- Les scripts d’orchestration (`env_local.sh`, `bootstrap_stage0.sh`, `self_host_stage1.sh`, `clean.sh`) fournissent un cadre pour le bootstrap et la gestion des artefacts.
- Le runtime/VM et la std sont des partenaires directs du compilateur : le bytecode généré est conçu pour la VM Vitte, et la std définit un contrat de base pour le langage.

Ce document doit servir de référence lors de l’implémentation du compilateur Vitte, de l’évolution de la grammaire ou de l’ajout de nouvelles fonctionnalités : l’architecture globale (front/middle/back, IRs, CLI, intégration bootstrap/runtime/std) reste la colonne vertébrale de la toolchain.
