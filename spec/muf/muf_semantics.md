# MUF Semantics — workspace, packages, modules, targets, profiles, deps, lock

Ce document définit la **sémantique** (signification) du manifest **MUF** (`muffin.muf`).
La syntaxe est décrite dans `muf_grammar.ebnf`.

Principes :
- **Déterminisme** : mêmes inputs ⇒ mêmes sorties (graphes, ordres, artefacts).
- **Réglages explicites** : les defaults existent mais doivent être documentés.
- **Portabilité** : séparation claire entre **targets** (plateformes) et **profiles** (modes de build).
- **Lockfile** : toute résolution externe (git/registry) doit être **pinnée** dans `muffin.lock`.

---

## 1) Modèle mental

Un workspace MUF décrit :
- 1..N **modules** (locaux) regroupés en **workspace**
- des **targets** (triples / environnements) qui définissent l’outillage et les flags
- des **profiles** qui définissent le niveau d’optimisation/debug/sanitizers
- des **deps** (externes et internes) résolues en graphe
- des **scripts** (entrées CLI) et du **publish** (packaging)

Le build se résume à :

1. **Lire** `muffin.muf` (+ includes)
2. **Normaliser** (paths, defaults, tri stable)
3. **Résoudre** les deps (et produire/valider `muffin.lock`)
4. **Planifier** un DAG de build (modules + deps)
5. **Exécuter** selon (target, profile)
6. **Produire** artefacts + index de sortie

---

## 2) Identités, noms et chemins

### 2.1 Identité d’un module

Identité logique d’un module local :
- `module_path` (ex: `std/io`, `compiler/frontend`)

Règle : un `module_path` est unique dans un workspace.

### 2.2 Résolution des chemins

Les chemins (Path/PathBare) sont résolus relativement à :
1) le dossier contenant le `muffin.muf` racine (**root_dir**)
2) si déclaré dans un `module`, relativement à `module.dir` si présent, sinon `root_dir`

Normalisation recommandée :
- conversion en chemin canonique (résolution de `.` / `..`)
- séparateurs `/` en interne

### 2.3 Include

`include "file.muf"` :
- insère le contenu MUF à l’emplacement de l’include
- le `root_dir` reste celui du fichier racine
- les chemins relatifs **dans** le fichier inclus sont résolus relativement au fichier inclus

Détection :
- cycles d’include interdits (`E_INCLUDE_CYCLE`)
- profondeur maximale configurable (ex: 32) (`E_INCLUDE_DEPTH`)

---

## 3) workspace

Déclaration :

```muf
workspace my/ws
  # ...
.end
```

### 3.1 Champs usuels

Clés recommandées :
- `name` : string (nom humain)
- `version` : Version (version du workspace)
- `edition` : string (ex: "2025")
- `default_target` : string (nom de `target`)
- `default_profile` : string (nom de `profile`)
- `resolver` : string (ex: "unify" | "multi")

### 3.2 members / paths

`members` et `paths` sont des listes de `Value` :

- `members` : liste de `module_path` (string ou ident) à inclure dans le workspace
- `paths` : répertoires scannés pour découvrir des modules (future extension)

Sémantique actuelle recommandée :
- si `members` est présent : seuls ces modules sont considérés “racines”
- sinon : tous les `module ...` déclarés dans le manifest sont racines

### 3.3 overrides

Permet de forcer une résolution de dépendance :

```muf
overrides
  override foo
    source.kind = "git"
    source.url = "..."
    source.rev = "..."
  .end
.end
```

- L’override s’applique à toutes les occurrences de la dépendance `foo`.
- En mode unify, l’override doit conduire à une seule EntryId.

### 3.4 allow/deny

Contrôle des features ou policies (ex: “deny unsafe”, “allow nightly”).
La signification précise dépend de la toolchain/target.

---

## 4) package

Déclaration :

```muf
package vitte
  # ...
.end
```

### 4.1 Champs usuels

- `name` : string (si absent, prend l’ident du bloc)
- `version` : Version
- `description` : string
- `license` : string
- `authors` : array de strings
- `readme` : path/string
- `homepage`, `repository` : string
- `keywords`, `categories` : arrays

### 4.2 Rôle

- Le bloc `package` sert au packaging/publication.
- Il ne contrôle pas directement la compilation (hors champs référencés par `publish`).

---

## 5) module

Déclaration :

```muf
module std/io
  # ...
.end
```

### 5.1 Champs usuels

- `kind` : string ∈ {"lib", "bin", "plugin", "tool"} (default: "lib")
- `dir` : path/string (répertoire source)
- `root` : path/string (fichier d’entrée, optionnel)
- `sources` : liste (fichiers ou globs, future)
- `exports` : liste (API exportée, optionnel)
- `imports` : liste (imports imposés, optionnel)

Defaults recommandés :
- `dir = module_path` (si le dossier existe) sinon `root_dir/module_path`
- `root = dir/mod.vitte` pour `lib`, `root = dir/main.vitte` pour `bin` (si existant)

### 5.2 deps dans un module

Un module peut contenir un sous-bloc `deps` :

```muf
module compiler/frontend
  deps
    dep unicode
      source git "https://..." rev "..."
    .end
  .end
.end
```

- Ces deps sont **scopées** au module.
- Lors de la résolution, elles s’agrègent au graphe global.

### 5.3 features

Bloc `features` à l’intérieur d’un module :
- définit les features disponibles
- peut activer/désactiver des sous-features ou comportements

Sémantique minimale recommandée :
- Une feature est un drapeau booléen.
- Les features activées proviennent de :
  1) CLI (`--features a,b`)
  2) defaults déclarés (future : `feature default = true`)
  3) dépendances (si une dep requiert une feature d’un autre module)

Résolution des features :
- appliquer un point fixe sur le graphe (activation transitive)
- détecter les boucles (ok si convergence, sinon `E_FEATURE_LOOP`)

---

## 6) target

Déclaration :

```muf
target macos_arm64
  triple = "aarch64-apple-darwin"
  cflags
    - "-O2"
  .end
.end
```

### 6.1 Champs usuels

- `triple` : string (ex: `x86_64-unknown-linux-gnu`)
- `os` / `arch` : strings (optionnels si `triple` suffit)
- `sysroot` : path/string
- `defines` : map (C/C++/asm defines)
- `link` : liste (libs, frameworks)
- `cflags`, `ldflags` : listes (tokens/strings)
- `env` : map (variables d’environnement)
- `steps` : pipeline optionnel d’exécution

### 6.2 Héritage & composition

Sémantique recommandée :
- `target` peut référencer un autre target via `base = "name"` (future extension)
- en l’absence de `base`, aucun héritage

### 6.3 steps

Les `steps` définissent une pipeline de commandes.

- `step <name>` contient :
  - `run "cmd ..."`
  - `inputs = [...]`, `outputs = [...]` (optionnel)

Sémantique :
- Les steps s’exécutent dans l’ordre de déclaration.
- Si `inputs/outputs` sont fournis, un moteur peut faire de l’incrémental.

---

## 7) profile

Déclaration :

```muf
profile release
  opt 3
  debug false
  lto thin
.end
```

### 7.1 Champs usuels

- `opt` : 0|1|2|3|s|z
- `debug` : bool
- `sanitizers` : array (ex: ["asan", "ubsan"])
- `lto` : off|thin|full

### 7.2 Application

Un `profile` s’applique à :
- la compilation de tous les modules (workspace + deps)
- la génération d’artefacts (strip, debug info)

Règle :
- le couple (target, profile) définit une **configuration** de build.

---

## 8) toolchain

Déclaration :

```muf
toolchain system
  cc "clang"
  ld "ld"
.end
```

### 8.1 Champs usuels

- `cc` : string (ex: "clang", "gcc")
- `asm` : string
- `ld` : string
- `rust` : string (optionnel)
- `sysroot` : path/string

### 8.2 Sélection

Sémantique recommandée :
- `target.toolchain = "name"` (future extension)
- sinon, `workspace.toolchain` si présent
- sinon, toolchain système par défaut

---

## 9) deps (global)

Déclaration :

```muf
deps
  dep unicode
    source git "https://..." rev "..."
    features = ["fast"]
  .end
.end
```

### 9.1 Définition d’une dep

Une dep a :
- un nom (ident)
- une source (path/git/registry)
- éventuellement `features`
- éventuellement `version` (si registry)

### 9.2 Source

- `source path <path>` : dépendance locale (vendoring ou mono-repo)
- `source git <url> rev <sha>` : dépendance git **pinnée**
- `source registry <url>` : dépendance depuis registry

Règles :
- Une source git **doit** être pinnée par `rev` lors de la génération du lock (`E_SOURCE_UNPINNED`).
- Une source registry doit être résolue en `version` exacte dans le lock.

### 9.3 Groupes

`group <name>` : groupe logique.
Sémantique recommandée :
- utilisé par la CLI (`muffin build --group dev`)
- par défaut, les groupes ne changent pas la résolution (organisation uniquement)

---

## 10) scripts

Déclaration :

```muf
scripts
  script fmt
    run "vitte-fmt ."
  .end
.end
```

Sémantique :
- un `script` expose une commande exécutable via `muffin run <script>`
- `run` est une string shell-like (découpée par le runner)

Bonnes pratiques :
- exécuter dans `root_dir`
- propager `target/profile` via env (`VITTE_TARGET`, `VITTE_PROFILE`)

---

## 11) publish

Déclaration :

```muf
publish
  artifacts
    - "dist/vittec-${version}-${triple}.tar.gz"
  .end
  checksums
    - "sha256"
  .end
  sign true
.end
```

Sémantique :
- `artifacts` : liste de chemins glob/strings d’artefacts à publier
- `checksums` : algos demandés (au minimum sha256)
- `sign` : active la signature (impl dépendante)

Le publish lit `package.*` (nom, version, etc.).

---

## 12) meta

Bloc libre :
- réservé aux extensions
- doit être ignoré par les moteurs qui ne le comprennent pas

---

## 13) Lockfile — interaction

### 13.1 Politique par défaut

- `muffin build` :
  - si `muffin.lock` existe → l’utiliser (strict)
  - sinon → résoudre et générer (si policy autorise)

Policy recommandée contrôlée par :
- `workspace.lock = "strict" | "auto" | "off"`

- `strict` : erreur si lock manquant (`E_LOCK_MISSING`)
- `auto` : génère si absent
- `off` : résout sans lock (non reproductible, déconseillé)

### 13.2 Résolution

Le lockfile :
- fige `source.rev` (git) et `version` (registry)
- fige `checksum.sha256`
- fige le graphe `deps` et `features` par entrée

Voir `muf_lockfile.md` pour le format et les règles.

---

## 14) Plan de build (DAG)

### 14.1 Nœuds

Nœuds du DAG :
- modules locaux
- deps externes (entrées lock)

### 14.2 Arêtes

Une arête `A -> B` signifie :
- A dépend de B

### 14.3 Ordonnancement

Ordre d’exécution :
- topological sort stable
- tie-break lexicographique sur `module_path` ou `name@version`

Erreurs :
- cycles interdits (par défaut) : `E_CYCLE`

---

## 15) Sorties et répertoires de build

Convention recommandée :
- `build/<target>/<profile>/...`
- `dist/` pour les artefacts packaging

Un moteur doit produire un index interne :
- liste d’artefacts (bins/libs)
- mapping module→artefacts

---

## 16) Erreurs standard

Codes recommandés :
- `E_PARSE` : syntaxe MUF invalide
- `E_DUP_MODULE` : module_path dupliqué
- `E_UNKNOWN_TARGET` / `E_UNKNOWN_PROFILE` / `E_UNKNOWN_TOOLCHAIN`
- `E_INCLUDE_CYCLE` / `E_INCLUDE_DEPTH`
- `E_LOCK_MISSING` / `E_LOCK_CONFLICT` / `E_SOURCE_UNPINNED` / `E_CHECKSUM_MISMATCH`
- `E_CYCLE` : cycle de dépendances
- `E_FEATURE_LOOP` : features non convergentes

---

## 17) Exemple minimal (workspace + 2 modules)

```muf
workspace vitte
  name = "vitte"
  version = v0.1.0
  default_target = "macos_arm64"
  default_profile = "debug"
.end

package vitte
  name = "vitte"
  version = v0.1.0
  license = "MIT"
  authors = ["Vincent"]
.end

profile debug
  opt 0
  debug true
  lto off
.end

profile release
  opt 3
  debug false
  lto thin
.end

target macos_arm64
  triple = "aarch64-apple-darwin"
  cflags
    - "-Wall"
  .end
.end

module std/io
  kind = "lib"
  dir = path("std/io")
.end

module tools/vittec
  kind = "bin"
  dir = path("tools/vittec")
  deps
    dep unicode
      source git "https://github.com/roussov/vitte-unicode" rev "2d9a4c3d3c2e1c0f9e0a8b7c6d5e4f3a2b1c0d9e"
    .end
  .end
.end

scripts
  script build
    run "muffin build --profile release --target macos_arm64"
  .end
.end
```
