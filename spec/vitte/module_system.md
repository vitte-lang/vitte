# TODO: module system

# Module System — Vitte (modules, paths, imports, exports, packages)

Ce document spécifie le **système de modules** de Vitte :
- structure logique (module paths)
- layout disque (fichiers/dossiers)
- règles `module/use/export`
- visibilité, résolution, cycles
- interaction avec MUF (`muffin.muf`) et `muffin.lock`

Il complète :
- `grammar_core_phrase.ebnf` (syntaxe `module/use/export`)
- `muf_semantics.md` (workspace/modules/deps)

Objectifs :
- Résolution **déterministe**.
- Imports **explicites**.
- Support workspace multi-modules + deps externes.
- Nomination stable pour tooling (fmt/lsp).

Non-objectifs :
- Définir un système de macros/traits.

---

## 1) Concepts

### 1.1 Module path

Un **module path** est un identifiant logique hiérarchique :

- séparateurs autorisés : `/` et `.`
- canonical interne : `/`

Exemples :
- `std/io`
- `compiler/frontend/parser`
- `tools/vittec`

Règles :
- segments : `Ident` (voir grammaire) `[A-Za-z_][A-Za-z0-9_]*`
- pas de segments vides
- pas de `..` dans un module path

### 1.2 Unité de compilation

Une unité de compilation (TU) = un fichier source `.vitte`.
Chaque TU appartient à **un module**.

### 1.3 Package / workspace

- **Workspace** : ensemble de modules locaux déclarés dans MUF.
- **Package** : metadata publication (MUF `package`).

Le module system fonctionne dans un **contexte** :
- `root_dir` (workspace)
- table des modules locaux
- table des deps externes (lock)

---

## 2) Layout disque (source tree)

### 2.1 Convention minimale

Pour un module `A/B/C` :

- dossier : `A/B/C/`
- fichier racine (lib) : `A/B/C/mod.vitte`
- fichier binaire (bin) : `A/B/C/main.vitte`

Le choix du root file dépend de `module.kind` (MUF) :
- `lib` → root par défaut `mod.vitte`
- `bin` → root par défaut `main.vitte`
- override possible via `module.root`.

### 2.2 Sous-modules

Un module peut contenir des sous-modules, par convention :

- `A/B/C/x.vitte` définit `A/B/C/x`
- `A/B/C/x/mod.vitte` définit `A/B/C/x`

Règle de priorité (si collision) :
1) `x/mod.vitte` (dossier module)
2) `x.vitte` (fichier module)

Collision non autorisée : si les deux existent → `E_DUP_MODULE_FILE`.

### 2.3 Nommage de fichier et module déclaré

Chaque fichier peut contenir en première ligne un `module <path>`.

Règle :
- si `module <path>` est présent, il doit **matcher** le path attendu par le loader (MUF + layout).
- sinon, le path est inféré.

Mismatch → `E_MODULE_PATH_MISMATCH`.

---

## 3) `module` (déclaration de module)

Syntaxe :

```vitte
module std/io
```

Sémantique :
- définit le namespace racine du fichier.
- doit apparaître **au plus une fois** par fichier.
- recommandé en tête de fichier.

Si absent :
- l’outil infère depuis la TU (voir §2).

---

## 4) `use` (imports)

Syntaxe :

```vitte
use std/io
use std/io as io
```

### 4.1 Résolution

`use X/Y` importe le module `X/Y`.

Résolution se fait en 2 étapes :
1) Chercher dans modules **locaux workspace**.
2) Sinon chercher dans deps externes (lock)

Si introuvable → `E0103` (import introuvable).

### 4.2 Alias

`use A/B as name` :
- `name` devient le préfixe dans le scope courant.

Règle :
- alias doit être unique dans un scope.
- conflit → `E_IMPORT_NAME_CONFLICT`.

### 4.3 Import cycle

Les cycles d’imports sont autorisés syntaxiquement.
Politique sémantique recommandée :
- cycles au niveau module : autorisés si le langage les supporte (résolution en 2 passes)
- cycles au niveau init runtime : interdits par défaut (voir §9)

Le compilateur doit détecter et rapporter les cycles quand ils cassent l’init.

---

## 5) `export` (exposition API)

Syntaxe :

```vitte
export all
export [Foo, Bar]
export from std/io [Read, Write]
```

### 5.1 Default visibility

Règle recommandée :
- Tout symbole top-level est **private** par défaut.
- `export` le rend visible aux imports.

Alternative (future) : `pub` sur symboles ; `export` resterait macro-level.

### 5.2 Export list

`export [A, B]` :
- expose uniquement les symboles listés.

`export all` :
- expose tout ce qui est top-level (sauf explicitement `private` si support).

### 5.3 Re-exports

`export from X [A, B]` :
- re-exporte des symboles d’un module importé.
- nécessite que `X` soit résolu.

Règle :
- re-export doit échouer si symbole non exporté par X → `E_EXPORT_MISSING`.

---

## 6) Résolution des noms

### 6.1 Scope module

Scope module =
- symboles définis dans le module
- imports (`use`)
- re-exports

### 6.2 Lookup

Lookup d’un ident `foo` :
1) locals du fichier (bloc)
2) symboles module
3) imports (alias)
4) prelude (si présent, optionnel)

Lookup qualifié `io.Read` :
- `io` doit être un alias importé
- puis lookup dans les exports du module `io`

### 6.3 Ambiguïtés

- Si deux imports fournissent le même nom sans alias → `E_IMPORT_AMBIGUOUS`.

---

## 7) Interaction MUF (workspace/modules)

### 7.1 Modules locaux

`muffin.muf` déclare :

```muf
module std/io
  kind = "lib"
  dir = path("std/io")
.end
```

Règle :
- MUF est la source de vérité sur l’existence des modules locaux.
- un fichier `.vitte` hors module déclaré est :
  - ignoré, ou
  - erreur (policy)

Policy recommandée :
- `workspace.strict_modules = true` (future) → erreur `E_MODULE_NOT_DECLARED`.

### 7.2 Dépendances externes

Les deps résolues (lock) fournissent un namespace de modules.

Règle :
- un package dep expose un ou plusieurs modules racines.
- ces racines sont identifiées par le lock (voir `muf_lockfile.md`).

### 7.3 Override & vendoring

Si une dep est “vendored” (`source.kind = path`):
- elle est résolue comme module local mais sous son namespace de dep.

---

## 8) Déterminisme de compilation

### 8.1 Ordre de découverte

L’ordre de découverte des modules ne doit pas dépendre du FS.
Règle :
- trier lexicographiquement les chemins.

### 8.2 Ordre des imports

Les `use` sont traités dans l’ordre d’apparition mais la résolution finale doit être stable.
En cas d’erreur, diagnostiquer au plus tôt (premier import fautif) mais produire un ensemble stable.

---

## 9) Initialisation (runtime) & cycles

Même si les cycles d’import peuvent être supportés au compile-time, le runtime peut nécessiter une init.

Recommandation :
- Chaque module peut avoir une init implicite (const eval / globals).
- Le runtime construit un graphe d’init.
- Les cycles d’init sont une erreur : `E_INIT_CYCLE`.

Stratégies :
- lazy init
- init en deux phases

---

## 10) Erreurs standard (module system)

Codes recommandés :
- `E0103` : import introuvable
- `E0104` : import name conflict
- `E0105` : import ambiguous
- `E0110` : export missing
- `E0111` : export invalid
- `E0120` : module path mismatch
- `E0121` : dup module file (x.vitte vs x/mod.vitte)
- `E0122` : module not declared (workspace strict)
- `E0130` : init cycle

---

## 11) Exemples

### 11.1 Module lib simple

```
std/io/
  mod.vitte
  read.vitte
  write.vitte
```

`std/io/mod.vitte` :

```vitte
module std/io
export [Read, Write]

use std/io/read as Read
use std/io/write as Write
.end
```

`std/io/read.vitte` :

```vitte
module std/io/read

fn read_all(path: Str) -> Bytes
  # ...
.end
```

### 11.2 Re-export

```vitte
module app
use std/io
export from std/io [Read]
```

### 11.3 Alias

```vitte
module app
use std/io as io

fn main() -> i32
  do io.Read("file.txt")
  ret 0
.end
```

---

## 12) Tooling (fmt/lsp)

Recommandations :
- LSP doit pouvoir résoudre un symbole via `module_path + span`.
- Formatter ne réordonne pas les `use` par défaut, mais peut offrir `--sort-imports`.

---

## 13) Notes & extensions futures

- Prelude (`use std/prelude`) implicite optionnel.
- Visibility fine-grained (`pub`, `pub(crate)`) option.
- Modules générés (build scripts).

```