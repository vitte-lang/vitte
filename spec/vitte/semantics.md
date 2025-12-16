# Semantics — Vitte (Core + Phrase) — référence

Ce document définit la **sémantique** du langage Vitte (niveau “core + phrase”), en cohérence avec :
- `spec/vitte/grammar_core_phrase.ebnf` (syntaxe)
- `spec/vitte/module_system.md` (modules/imports/exports)
- `spec/vitte/abi_ffi.md` + `spec/runtime/rt_abi.md` (ABI/FFI)
- `spec/vitte/error_model.md` (diagnostics)

Objectifs :
- Définir des règles **déterministes** et **implémentables** (compilateur + VM).
- Distinguer clairement **syntaxe**, **résolution**, **typage**, **exécution**.
- Fournir une base pour : IR, codegen, VM, LSP, formatter.

Non-objectifs :
- Décrire une bibliothèque standard complète.
- Spécifier tous les détails d’optimisation.

---

## 0) Vue d’ensemble (pipeline)

Le traitement d’un programme Vitte suit typiquement :

1) **Lex/Parse** → AST (selon EBNF)
2) **Lowering phrase→core** (désucrage : `set/say/do/ret/when/loop`)
3) **Résolution modules/imports** (table des symboles par module)
4) **Résolution noms** (scopes) + **détection erreurs** (shadowing/ambiguïtés)
5) **Typage** (inference limitée + annotations) + validation patterns/match
6) **Lowering** → IR (SSA ou bytecode IR)
7) **Codegen** → bytecode VM ou C/obj (selon backend)
8) **Link** (modules + deps) + génération artefacts
9) **Runtime** : init modules, exécution entrypoint, traps/erreurs

---

## 1) Unités sémantiques

### 1.1 Programme

Un programme est une `CompilationUnit` composée de déclarations top-level.
L’unité minimale d’organisation est le **module**.

### 1.2 Module

Un module est identifié par un `module_path` (cf. `module_system.md`).
Un module peut regrouper plusieurs fichiers (TUs). Les règles de fusion sont :
- les `use` et `export` sont agrégés au niveau module
- les définitions top-level forment un espace de noms unique
- les collisions de noms top-level sont interdites (sauf surcharge explicite si un jour supportée)

### 1.3 Déclarations top-level

Top-level :
- `type` (`struct/union/enum/alias`)
- `fn` / `scn`
- `global` (`let/const` au top-level)
- `use` / `export`

---

## 2) Désucrage “phrase” → core

Le “phrase surface syntax” est une couche de statements équivalents au core.

### 2.1 `set`

```
set x = expr
```
Désucre en :
- `AssignStmt`: `x = expr`

### 2.2 `say`

```
say expr
```
Désucre en :
- `ExprStmt`: appel de fonction builtin `std/io::print(expr)` (ou `core::print`) selon la policy.

Règle :
- si aucun runtime/std n’est disponible, `say` doit être interdit (`E_PHRASE_REQUIRES_STD`).

### 2.3 `do`

```
do expr
```
Désucre en :
- `ExprStmt`: `expr;`

### 2.4 `ret`

```
ret expr?
```
Désucre en :
- `ReturnStmt`.

### 2.5 `when`

`when` est un sugar de `if/elif/else` :
- `when cond` ≡ `if cond`
- `when cond` suivants ≡ `elif cond`

### 2.6 `loop`

`loop .end` ≡ `while true .end`.

---

## 3) Valeurs, expressions, ordre d’évaluation

### 3.1 Modèle de valeurs

Vitte manipule :
- scalaires (int/float/bool/nil)
- tuples
- structs/enums/unions
- références (si/quan supportées par runtime)

Le modèle mémoire exact dépend du backend (VM ou natif), mais la sémantique observable doit rester stable.

### 3.2 Ordre d’évaluation

Ordre d’évaluation **déterministe** :
- appels : évaluer la fonction puis les arguments de gauche à droite
- opérateurs binaires : évaluer LHS puis RHS
- index/dot : évaluer le receveur puis l’index/nom

### 3.3 Court-circuit

- `and` et `or` sont **short-circuit**.

### 3.4 Cast `as`

`expr as T` :
- conversions numériques (int↔float) autorisées sous règles (voir §8)
- conversions interdites → `E_CAST_INVALID`

---

## 4) Scopes & résolution des noms

### 4.1 Scopes

Scopes imbriqués :
- module scope
- function/scenario scope
- block scope
- pattern binding scope

### 4.2 Règles de shadowing

Recommandation :
- shadowing autorisé dans un scope interne
- shadowing au même scope interdit (`E_NAME_REDEF`)

### 4.3 Résolution ident

Lookup (résumé) :
1) locals du scope le plus interne
2) params
3) bindings de patterns
4) symboles module
5) imports (`use` alias)
6) prelude (optionnel)

Ambiguïtés import → `E_IMPORT_AMBIGUOUS`.

---

## 5) Déclarations et mutabilité

### 5.1 `let`

`let x: T? = expr?` :
- introduit une variable locale
- si type absent, tentative d’inférence depuis `expr` (si présent)
- si `expr` absent, le type doit être fourni (`E_LET_NEEDS_INIT_OR_TYPE`)

Mutabilité :
- core n’expose pas encore `mut` dans la grammaire. Convention :
  - par défaut, `let` est mutable (assign autorisé)
  - `const` est immutable

(Option future : `let mut` / `let` immutable.)

### 5.2 `const`

`const x: T? = expr` :
- doit être **évaluable** à la compilation (const-eval) si la cible le requiert
- sinon (VM), peut être initialisé au load module

Règle stable recommandée :
- `const` doit être déterministe et sans effets de bord.

### 5.3 Globals

`let/const` top-level :
- crée un symbole module
- initialisation lors de l’init module

Cycles d’init → `E_INIT_CYCLE`.

---

## 6) Contrôle de flux

### 6.1 `if/elif/else`

- `if` évalue la condition (bool)
- un bloc est exécuté
- expression condition doit typer en `Bool` sinon `E_EXPECTED_BOOL`

### 6.2 `while`

- boucle tant que condition true
- `break` sort, `continue` passe à l’itération suivante

### 6.3 `for x in expr`

Sémantique minimale (core) :
- `expr` doit être itérable.
- Le protocole d’itération est une extension future.

En attendant :
- `for` est accepté si backend fournit un lowering (ex: sugar sur `iter_next`).
- sinon `E_FOR_NOT_SUPPORTED`.

### 6.4 `loop`

Voir désucrage §2.6.

### 6.5 `match`

`match expr` sélectionne un `arm` par pattern.

Règles :
- les arms sont testés de haut en bas
- premier match gagne
- patterns doivent être valides pour le type de `expr`

Exhaustivité :
- par défaut : warning si non exhaustif (`W_MATCH_NON_EXHAUSTIVE`)
- policy stricte : erreur `E_MATCH_NON_EXHAUSTIVE`

---

## 7) Patterns

Patterns core supportés :
- `_` wildcard
- literal
- ident binding
- tuple pattern
- variant pattern (`Type::Variant(...)`)

### 7.1 Bindings

`IdentPat` introduit un binding dans l’arm.
Règle :
- collision de binding dans le même pattern → `E_PATTERN_BINDING_DUP`.

### 7.2 Variant patterns

`TypeExpr::Ident(...)` :
- `TypeExpr` doit référencer un enum/variant container
- `Ident` doit être un variant valide
- arité doit matcher la définition

Mismatch → `E_PATTERN_VARIANT_ARITY`.

---

## 8) Système de types (noyau)

Le système de types officiel évoluera, mais la base minimale suivante est stable.

### 8.1 Types primitifs

Types recommandés :
- `Bool`
- `i8 i16 i32 i64`
- `u8 u16 u32 u64`
- `f32 f64`
- `Str` (string UTF-8, sémantique std)
- `Bytes` (buffer)
- `Unit` (type “rien”) associé à `nil`

Note : la grammaire accepte des `TypeName` arbitraires. La table des types builtins est une convention.

### 8.2 `nil`

`nil` a le type `Unit`.

### 8.3 Tuples

Un tuple `(A, B, ...)` est un type produit.
- le littéral tuple nécessite au moins une virgule (syntaxe)

### 8.4 Structs

- champs nommés
- accès via `.`

### 8.5 Enums

- variants, avec payload optionnel (tuple-like)

### 8.6 Unions

- représentent une union C-like (unsafe par nature)
- sans mécanisme de tag, l’accès est backend-specific

Policy recommandée :
- unions autorisées uniquement en `unsafe` (future) ; en attendant, warnings.

### 8.7 Aliases

`type X = T` :
- alias transparent
- `X` et `T` ont même représentation (sauf si future newtype)

### 8.8 Génériques

La grammaire supporte :
- `FnDecl`/`TypeDecl` avec `GenericsDecl` et `TypeArgs`.

Sémantique minimale :
- monomorphisation (spécialisation) au compile-time
- chaque instanciation `Foo[T]` produit un type concret

Règles :
- arité générique doit matcher (`E_GENERIC_ARITY`).
- pas de contraintes trait dans cette version (future).

### 8.9 Inférence

Inférence recommandée :
- locale, basée sur initialisation `let x = expr`
- pas d’inférence globale complexe

Si inférence impossible → `E_TYPE_ANNOTATION_REQUIRED`.

### 8.10 Conversions numériques

Règles recommandées (stables) :
- int→int : autorisé si largeur destination >= source et signe compatible, sinon nécessite cast explicite.
- int→float : autorisé explicitement via `as`.
- float→int : uniquement via `as`.
- float→float : via `as` si changement de précision.

Les casts peuvent “saturer” ou “trunc” selon policy.
Policy recommandée :
- cast float→int : trunc vers 0, overflow → `E_OVERFLOW` (ou trap runtime)
- cast int→narrow : overflow → `E_OVERFLOW` (compile-time si constant, sinon trap)

---

## 9) Fonctions et appels

### 9.1 Définition

`fn name(params) -> T? block` :
- paramètres typés obligatoires
- retour optionnel : si absent, type retour = `Unit`

### 9.2 Return

- `return expr` doit typer avec le type de retour
- `return` sans expr équivaut à `return nil`

### 9.3 Appel

`f(a,b)` :
- `f` doit être callable
- arité doit matcher

### 9.4 Récursion

Autorisé.

---

## 10) Scenarios (`scn`)

`scn` est une fonction spéciale “orchestration” (exécution scriptée), destinée aux pipelines/outillage.

Sémantique recommandée :
- même règles de type/call qu’une `fn`
- peut accéder à un contexte runtime (env, IO) selon policy

Entrypoint `scenario <path>` peut exécuter un scenario.

---

## 11) Entrypoints

`program/service/kernel/driver/tool/scenario/pipeline <ModulePath> block` :

Sémantique :
- déclare un point d’entrée de build/exécution
- associe une “kind” (mode runtime)

Règles :
- un artefact buildé doit choisir un entrypoint.
- si plusieurs, MUF/outils choisissent par défaut ou via flag.

---

## 12) Runtime model (VM)

Cette section décrit les invariants observables, sans imposer un backend.

### 12.1 Init modules

- Construire un graphe d’import/init.
- Init dans un ordre topologique stable.
- Détecter cycles d’init : `E_INIT_CYCLE`.

### 12.2 Erreurs runtime

Deux familles :
- traps (program faults) : out-of-bounds, div0
- faults (OS) : IO, permission

Mapping recommandé :
- faults via `VITTE_E*` (ABI runtime)
- traps via namespace `VITTE_TRAP_*` puis rendu diagnostics `E7xxx`

### 12.3 Panics

Un panic est fatal (par défaut) :
- message + optional backtrace
- code process non-zéro

(Option future : catch/try.)

---

## 13) Const-eval (minimal)

Const-eval est requis pour :
- `const` si policy strict
- certaines tailles/initialisateurs (future)

Règles :
- les expressions const doivent être pures
- appels autorisés uniquement vers fonctions marquées const (future)

En V1 minimal :
- const-eval limité aux littéraux + ops arith/comp + tuples simples.

---

## 14) Spécification des opérations (ops)

### 14.1 Opérateurs

- arith : `+ - * / %`
- bit : `& | ^ ~ << >>`
- comp : `== != < <= > >=`
- logiques : `and or !`

### 14.2 Types admissibles

Règles recommandées :
- arith : int/float compatibles
- bit/shift : entiers uniquement
- comp : mêmes types ou types comparables
- `and/or/!` : bool uniquement

Violation → `E_OP_TYPE`.

### 14.3 Division/modulo

- division int : trunc vers 0
- modulo : signe comme langage C (reste a le signe du numérateur)
- division par zéro : trap runtime (`E7001`) ou compile-time error si constant

---

## 15) Accès membres / index

### 15.1 Dot

`x.field` :
- `x` doit être struct/namespace
- `field` doit exister

### 15.2 Index

`a[i]` :
- `a` doit être indexable
- `i` doit être int

Out-of-bounds : trap runtime.

---

## 16) Diagnostics & erreurs (résumé)

Les codes d’erreurs doivent suivre `error_model.md`.

Erreurs typiques :
- parse : `E0xxx`
- module/import : `E01xx`
- type : `E2xxx`
- codegen/ABI : `E5xxx`
- runtime traps : `E7xxx`

---

## 17) Déterminisme (règles globales)

- tri stable des modules/imports/diagnostics
- ordre d’évaluation défini
- init module topologique stable
- output JSON diagnostics stable

---

## 18) Extensions prévues (non normatives)

- `pub` / visibility fine
- `mut` / immutabilité stricte
- traits/constraints génériques
- borrow/ownership model
- pattern matching avancé (guards)
- `try`/`catch` et erreurs typées
- iterators (for protocol)
- async/await
