# Architecture du compilateur Vitte (bootstrap-friendly)

Ce document décrit l’architecture **actuelle** (sous-ensemble minimal) et la trajectoire vers une chaîne complète.

Objectif immédiat : fournir un compilateur **stable, simple à auditer, facile à bootstrap**.

---

## Vue d’ensemble

Le compilateur est structuré en couches, avec des frontières nettes :

```
source (.vitte)
  ↓
front/  : lecture → lexing → tokens → (parser minimal) → AST minimal
  ↓
(sema/) : (optionnel au début) validations légères / table des symboles minimale
  ↓
back/   : génération C minimal (ou IR minimal → C)
  ↓
cc      : compilation C → binaire
```

Caractéristiques du sous-ensemble bootstrap :
- blocs délimités par **`.end`** (pas d’accolades requises)
- parsing volontairement **tolérant** : on identifie les top-level items sans analyser toute la grammaire
- priorité : **déterminisme**, diagnostics propres, génération C reproductible

---

## Arborescence et responsabilités

### `support/` — fondations
Rôle : composants bas niveau, sans dépendances “compilo”, réutilisables partout.
- `arena` / allocateurs : allocations groupées pour tokens/AST
- `filesystem` : lecture de fichiers, chemins, normalisation
- `containers` : vec, map, strings, slices, etc.

Exigences :
- zéro dépendance cyclique
- API stable (utilisée par diag/front/back)

### `diag/` — diagnostic et source map
Rôle : tout ce qui concerne **où** est l’erreur et **comment** on l’affiche.
- `source_map` : fichier → offset → (ligne, colonne)
- `diagnostic` : structure d’erreur (severity, code, message, spans)
- `emitters` : sortie console (couleurs), JSON (plus tard), CI format (plus tard)

Principes :
- chaque erreur doit avoir un **span**
- éviter les exceptions : erreurs structurées, remontées explicitement

### `front/` — lexer (et parser minimal)
Rôle : transformer le texte en tokens exploitables.
- `lexer` : itère sur chars/bytes, produit tokens
- `tokens` : enum des types de tokens + payload (ident, literal, punct)
- `parser` (stub ou minimal) : extrait les top-level items sans parser le corps complet

Sous-ensemble reconnu (bootstrap) :
- `module X` (détection de module)
- `import ...`, `export ...` (tokenisation + extraction naïve)
- `fn name(...) -> type` (signature : extraction naïve)
- corps de fonctions : **optionnel** ; peut être “skippé” en s’arrêtant sur `.end`

### `sema/` (optionnel au bootstrap)
Rôle : validations “raisonnables” sans implémenter tout le langage.
Exemples (progressifs) :
- unicité des noms top-level
- table des symboles minimaliste par module
- checks simples : double export, import impossible, etc.

À noter : au bootstrap, `sema/` peut être **désactivé** ou réduit à des checks non bloquants.

### `back/` — génération C minimal
Rôle : produire un C “simple” et portable.
- stratégie : un fichier C par module (ou un agrégat), prototypes, corps minimal
- runtime : appels vers la runtime Vitte si nécessaire (ou stubs)

Contrainte bootstrap :
- générer du C sans dépendre d’une IR complexe
- garder la sortie C lisible (debug) et déterministe (reproductibilité)

### `pack/` — graphe Muffin (placeholder)
Rôle : orchestrer modules/dépendances/build.
- au début : lecture “best effort” d’un manifest, résolution basique
- plus tard : lockfile, cache, fetch, graph complet

---

## Flux de données (contrats)

### 1) Source → `source_map`
- chaque fichier a un `FileId`
- le texte est stocké (ou mappé) avec un index de lignes

### 2) `lexer` → `tokens`
- sortie : `TokenStream` (slice/vec) + erreurs de lexing
- tokens doivent contenir :
  - kind
  - span (offset start/end + FileId)
  - payload (ident/literal si applicable)

### 3) Parser minimal → Top-level index
Au bootstrap, l’objectif n’est pas un AST complet.

On construit plutôt un **index** :
- module name
- liste des imports/exports (texte ou structure légère)
- liste des fonctions : nom + signature stringifiée + span

Le corps peut être ignoré avec :
- un compteur de nesting (si tu as des `fn/scn/prog`)
- et surtout, la délimitation `.end`.

### 4) Génération C
À partir de l’index :
- générer prototypes
- générer stubs pour fonctions si on ne compile pas le corps
- ou générer du C complet quand le parser devient plus riche

---

## Parsing “bootstrap” : stratégie de robustesse

### Pourquoi “naïf” au début
- éviter une implémentation complète de la grammaire avant d’avoir une toolchain stable
- avancer par paliers : lexing solide → top-level index → génération C → tests

### Pattern concret : “scan top-level”
- tu consommes les tokens
- tu repères des marqueurs : `module`, `import`, `export`, `fn`
- pour `fn`, tu captures jusqu’à `.end` (en token mode)

C’est simple, mais ça permet déjà :
- diagnostics précis sur la signature
- un compilateur usable pour un sous-ensemble

---

## Error model

### Niveaux
- `note` : info
- `warning` : non bloquant
- `error` : stop compilation

### Contrat
- une erreur doit avoir :
  - message clair
  - code (ex: `E0001`)
  - span principal
  - hints éventuels

---

## Performance et déterminisme

### Déterminisme
- pas de hash random dans les structures “observables”
- sortie C stable (tri d’exports/imports si nécessaire)

### Perf
- `arena` pour tokens/AST
- éviter copies de strings : interner ou slices + lifetime via arena

---

## Plan d’évolution (roadmap technique)

### Phase 0 — bootstrap (actuelle)
- lexer solide + spans
- parser minimal top-level + skip `.end`
- génération C stubs
- tests + bench micro sur lexer

### Phase 1 — AST noyau
- AST pour expressions/stmt minimal
- parser block `.end`
- génération C simple (if/while/return)

### Phase 2 — Sema
- résolution noms/imports
- types de base + inference minimale (si prévue)

### Phase 3 — IR (si besoin)
- IR stable (SSA ou simple stack IR)
- backends multiples (C / VM)

---

## Tests recommandés

- `front/` : golden tests lexing (tokens + spans)
- `diag/` : snapshots diagnostics (format stable)
- `back/` : compare sortie C (golden) + compile/exec
- fuzz (optionnel) : lexer et parser minimal

---

## Définitions “bootstrap-friendly” (rappel)

Le compilateur actuel vise un sous-ensemble :
- blocs `.end` (skip statements si nécessaire)
- `module X`, `import`, `export` (tokenisés)
- `fn name(...) -> type` (détection naïve)

Le but est de garder une base **solide** et d’élargir progressivement.
