

# Compiler — vitte (max)

Ce dossier contient le **compilateur Vitte** (`vittec`) et les composants internes (front-end → IR → backends).

Objectifs :
- Compiler des sources `.vitte` selon la spec **Core + Phrase**.
- Produire des artefacts reproductibles (bytecode VM et/ou C/obj selon backend).
- Appliquer un modèle d’erreurs/d’ABI stable (voir `spec/`).

Références (contrats) :
- Syntaxe : `spec/vitte/grammar_core_phrase.ebnf`
- Sémantique : `spec/vitte/semantics.md`
- Modules : `spec/vitte/module_system.md`
- Diagnostics : `spec/vitte/error_model.md`
- ABI/FFI : `spec/vitte/abi_ffi.md`
- Runtime ABI : `spec/runtime/rt_abi.md`
- PAL : `spec/runtime/pal_contract.md`

---

## 1) Vue d’ensemble

### 1.1 Pipeline (haute-niveau)

1) **Lex/Parse** (AST)
2) **Lowering Phrase → Core** (désucrage `set/say/do/ret/when/loop`)
3) **Résolution modules/imports** (workspace + deps lock)
4) **Résolution noms** (scopes)
5) **Typage** (inference locale + validations)
6) **Lowering** vers IR
7) **Optimisations** (optionnel)
8) **Backend** :
   - VM : bytecode + debug info
   - C : génération C (C23) + link toolchain
9) **Diagnostics** (human/json) + code retour

### 1.2 Backends

Le compilateur peut fournir plusieurs backends :
- **VM backend** : produit un artefact exécutable par le runtime/VM.
- **C backend** : produit du C (C23) + compile via toolchain (clang/gcc/msvc).

La sélection du backend dépend de MUF (`muffin.muf`) / flags CLI.

---

## 2) Arborescence (recommandée)

Le repo peut évoluer, mais l’intention est la suivante :

```
compiler/
  README.md
  src/
    driver/              # orchestration (vittec)
    session/             # options, config, caches
    inputs/              # file loading, module graph

    lex/                 # lexer
    parse/               # parser + recovery
    ast/                 # AST nodes

    phrase/              # lowering phrase → core
    hir/                 # high-level IR (optionnel)
    sema/                # name resolution, type checking

    ir/                  # IR stable interne
    opt/                 # passes d’optimisation (optionnel)

    codegen_vm/          # bytecode emitter
    codegen_c/           # C emitter (C23)

    diag/                # diagnostics engine (human/json)
    util/                # helpers

  tests/
    fixtures/            # inputs sources
    golden/              # snapshots output

  fuzz/                  # fuzz harnesses (si séparé du root)

  tools/
    scripts/             # scripts dev (format, regen)
```

Notes :
- Le layout exact dépend du projet, mais les **responsabilités** doivent rester séparées.
- Les composants “public contract” vivent dans `spec/` et `sdk/sysroot/include`.

---

## 3) Entrées / sorties

### 3.1 Inputs

- Sources Vitte : `.vitte`
- Manifests : `muffin.muf` (+ `muffin.lock`)

Le compilateur doit :
- appliquer `module_system.md` pour résoudre les modules
- refuser les incohérences path ↔ `module <path>`

### 3.2 Outputs

Selon backend :

- VM :
  - bytecode/pack (ex: `*.vbc` ou `*.vittebin`)
  - debug info (optionnel)

- C :
  - `.c` généré
  - compilation via toolchain vers exe/dylib

Le naming final est piloté par MUF (targets/profiles) et la couche outillage.

---

## 4) Diagnostics

Le moteur de diagnostics doit suivre `spec/vitte/error_model.md` :

- codes stables (`E####`, `W####`, `ICE####`)
- spans en offsets bytes UTF-8
- rendu :
  - human (couleurs si TTY)
  - JSONL (une ligne par diagnostic)

### 4.1 Code retour process

- `0` : succès
- `1` : erreur utilisateur (diagnostics)
- `2` : erreur interne (ICE)

---

## 5) Front-end (lex/parse)

### 5.1 Lexer

Responsabilités :
- tokeniser selon `grammar_core_phrase.ebnf`
- fournir spans exacts
- erreurs récupérables (string non terminée, escape invalide, etc.)

### 5.2 Parser

Responsabilités :
- parser AST
- recovery déterministe (synchronisation sur NEWLINE, `.end`, tokens structurants)
- éviter cascades d’erreurs

---

## 6) Lowering Phrase → Core

Le lowering doit :
- être purement syntaxique
- préserver spans (traçabilité)

Règles :
- `set` → assign
- `do` → expr stmt
- `ret` → return
- `when` → if/elif/else
- `loop` → while true
- `say` → appel builtin (nécessite std/runtime ; sinon erreur)

---

## 7) Résolution modules/imports

Le module graph est construit selon `module_system.md` :

- priorité : workspace → deps lock
- ordre stable : tri lexicographique
- détection collisions (`x.vitte` vs `x/mod.vitte`)

Le compilateur doit produire :
- un graphe d’import (compile-time)
- un graphe d’init (runtime) si globals/const init

Cycles d’init → `E_INIT_CYCLE`.

---

## 8) Typage (noyau)

Le typer suit `spec/vitte/semantics.md` :

- primitives (`Bool`, `i*/u*`, `f*`, `Unit`, etc.)
- tuples, structs, enums, unions
- conversions explicites via `as`
- inference locale (si possible)

À court terme :
- pas de traits/constraints
- génériques monomorphisés

---

## 9) IR interne & passes

### 9.1 IR

L’IR doit être :
- déterministe
- stable pour les passes
- suffisant pour VM et C backends

Recommandation :
- IR en SSA (ou une variante)
- bloc/fonction explicites
- ops typées

### 9.2 Passes

Exemples (optionnels) :
- const folding
- dead code elimination
- inlining simple

Toute passe ne doit pas changer les diagnostics observables (sauf réduire erreurs spurielles).

---

## 10) Backend VM

Responsabilités :
- générer un bytecode stable (spec VM séparée)
- produire debug info (spans → pc)
- générer tables de constantes/strings

Interactions runtime :
- erreurs traps/faults doivent mapper vers les codes stables.

---

## 11) Backend C (C23)

Responsabilités :
- générer du C lisible et déterministe
- respecter les conventions ABI/FFI (`spec/vitte/abi_ffi.md`)

Standard :
- base : C23 (`-std=c23`)
- fallback clang ancien : `-std=c2x`

Flags et toolchains :
- voir `spec/targets/flags.md`
- voir `sdk/toolchains/README.md`

Règles :
- pas de dépendance à des extensions non portables sans gate
- symbol visibility : hidden par défaut, export explicite ABI

---

## 12) Build (notes)

Le build est piloté par `muffin` et le SDK (sysroot).

Exemples (indicatifs) :

- build projet workspace :
  - `muffin build --target <target> --profile <profile>`

- run :
  - `muffin run --target <target> --profile <profile>`

Le détail exact des commandes dépend de l’implémentation courante de `muffin`.

---

## 13) Tests

### 13.1 Types de tests

- unit tests (lexer/parser/typer)
- golden tests (snapshots diagnostics / codegen)
- integration tests (build + run examples)
- fuzz (voir `fuzz/corpora/README.md`)

### 13.2 Golden snapshots

Recommandation :
- snapshots déterministes
- diff lisible (text)

---

## 14) Fuzzing

Composants typiques à fuzz :
- lexer/parser (sources `.vitte`)
- MUF parser (`.muf`)
- formatter
- loader (runtime)

Les corpora versionnées vivent dans `fuzz/corpora/`.

---

## 15) Compatibilité & contrats

- La spec `spec/` est la source de vérité.
- L’ABI est append-only.
- Les messages texte peuvent évoluer, mais les **codes** et la **structure** des diagnostics sont stables.

---

## 16) TODO (extensions)

- traits/constraints génériques
- iter protocol pour `for`
- debug info enrichie
- plus de passes IR
- séparation explicite “compiler core” vs “driver CLI”