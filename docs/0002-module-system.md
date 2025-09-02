---

rfc: 0002
title: "Module System"
status: proposed
authors: \["Vincent"]
shepherd: "Core Team"
reviewers: \["@resolver", "@compiler", "@lsp"]
created: 2025-09-02
updated: 2025-09-02
tracking-issues: \["vitte-lang/vitte#2"]
requires: \["0001-core-syntax-and-keywords"]
supersedes: \[]
superseded-by: \[]
discussions-to: "[https://github.com/vitte-lang/vitte/discussions](https://github.com/vitte-lang/vitte/discussions)"
target-release: "v0.1"
labels: \["lang", "modules", "resolver", "tooling"]
---------------------------------------------------

> **Résumé** — Ce RFC spécifie le **système de modules** de Vitte : structure des fichiers/dossiers, visibilité, import/export, résolution des chemins et re‑exports. Objectifs : simplicité, lisibilité, compilation incrémentale efficace, et outillage LSP/formatter naturel.

---

## 1) Motivation

* Organiser le code en **unités logiques** réutilisables.
* **Isoler** les symboles, éviter les collisions et clarifier les dépendances.
* Supporter la compilation **incrémentale** et une **résolution rapide** (alignez `vitte-resolver`).
* Préparer l’intégration au gestionnaire de paquets **`vitpm`** (noms stables côté code).

## 2) Objectifs / Non‑objectifs

**Objectifs**

* O1 — Définir **module = fichier** *ou* **dossier+`mod.vit`**.
* O2 — Spécifier **`pub`**, **`use`**, **alias `as`**, **re‑exports `pub use`**.
* O3 — Définir la **résolution de chemins** (absolus/relatifs) et les **règles de visibilité**.
* O4 — Données pour l’**outillage** : tables d’exports, navigation, renames sûrs.

**Non‑objectifs**

* N1 — Spécifier `vitpm` (manifest/lock/registry) → RFC 0010.
* N2 — Macros/attributs d’import → RFC 0008.
* N3 — Visibilités fines (`pub(crate)`, `pub(super)`) — **hors v0.1**.

---

## 3) Terminologie

* **Module** : espace de noms logique et unité de compilation.
* **Sous‑module** : module enfant déclaré dans un module parent.
* **Chemin** : suite d’identifiants séparés par `::`.
* **Re‑export** : export d’un symbole importé (`pub use`).

---

## 4) Mise en forme du code (fichiers & dossiers)

### 4.1 Racine du package

* Le package a un **module racine** : `src/mod.vit`.
* Entrées spécifiques (ex. binaire/tests) sont hors de ce RFC.

### 4.2 Deux formes équivalentes

**A) Module‑fichier**

* `foo.vit` déclare **`mod foo`** implicitement quand référencé.
* Référence depuis le parent via `mod foo;`.

**B) Module‑dossier**

* Dossier `foo/` contenant **`mod.vit`** → module `foo`.
* Référence depuis le parent via `mod foo;`.

**Règles**

* Dans `mod.vit` (parent), déclarer les enfants via :

  ```vitte
  mod math;
  mod io;
  ```
* Un identifiant de module **ne peut pas** être à la fois fichier **et** dossier.
* Nommage **`snake_case`** recommandé.

---

## 5) Visibilité & exports

* Par défaut, tout **symbole est privé** au module où il est défini.
* `pub` **exporte** le symbole vers l’extérieur (visible depuis les modules qui importent le module exporteur).
* L’export d’un enfant n’est visible **à la racine** que si **chaque niveau** de la chaîne est `pub` (**chaîne d’exports**).

Exemple :

```vitte
mod geometry {
    pub struct Point { x: f64, y: f64 }
    fn area_internal() { /* privé */ }
}

// dehors, `geometry::Point` est accessible, `geometry::area_internal` non.
```

---

## 6) Imports (`use`) & alias (`as`)

### 6.1 Formes supportées

```vitte
use math::square
use math::trig::{sin, cos}
use io::fs as filesystem
pub use geometry::Point       // re‑export public
pub use math::trig::{sin as s}
```

* **Import groupé** avec `{ … }`.
* **Alias** via `as`.
* **Re‑export** via `pub use`.
* **Interdit en v0.1** : import glob `*` (ambigu, ralentit l’outillage).

### 6.2 Grammaire (extrait)

```ebnf
use_item  ::= ('pub')? 'use' use_tree ';' ;
use_tree  ::= path ('as' ident)?
            | path '::' '{' use_list '}' ;
use_list  ::= use_tree { ',' use_tree } ;
```

---

## 7) Résolution des chemins

### 7.1 Chemins absolus

* Un chemin **non préfixé** est résolu **depuis la racine du package**.
* Le premier segment correspond à un **module enfant de la racine** ou à un **export** de la racine.

### 7.2 Chemins relatifs (contextuels)

* Ce RFC introduit **`self`** et **`super`** comme **mots‑clés contextuels** utilisables **dans les chemins uniquement** (ils restent des identifiants ailleurs).

  * `self::foo` : dans le module courant.
  * `super::bar` : dans le module parent direct.
* Cela **amende RFC 0001** : `self`/`super` passent de « réservés pour futur » à **contextuels en chemins**.

### 7.3 Algorithme (aligné `vitte-resolver`)

1. Si chemin commence par `self`/`super` → point de départ relatif, sinon **racine**.
2. Résolution **segment par segment** via les **exports `pub`** et symboles internes.
3. Les **types/structs/enums** peuvent porter un **espace de noms** pour leurs membres statiques si le langage l’expose (hors v0.1).
4. En cas de collision : erreur **`symbole ambigu`** (un seul gagnant par nom et par portée).
5. Les re‑exports sont transparents (la cible finale est retenue pour la doc/`goto definition`).

---

## 8) Re‑exports (`pub use`)

* Permettent de **remonter** un symbole importé dans l’espace de noms du module courant.
* Utiles pour exposer une **API plate** depuis une arborescence profonde.
* Les chaînes `pub use` peuvent **traverser plusieurs modules**. La résolution suit le **chemin réel** pour éviter les boucles ; les boucles de re‑export sont **détectées et interdites**.

---

## 9) Conflits, doublons, cycles

* **Doublon de nom** dans un module : erreur à la définition (sauf si **alias** explicite).
* **Import en conflit** :

  ```vitte
  use math::trig::sin
  use geometry::trig::sin   // ❌ conflit (même nom)
  use geometry::trig::sin as gsin // ✅
  ```
* **Cycles de modules** (structure parent‑enfant) : impossibles (arbre).
* **Cycles d’imports** via `use`/`pub use` : détectés, **interdits** si nécessaires à la résolution au moment de l’analyse.

---

## 10) Exemples complets

### 10.1 Layout

```
src/
 ├─ mod.vit
 ├─ math/
 │   ├─ mod.vit
 │   └─ trig.vit
 └─ geometry/
     └─ mod.vit
```

**`src/mod.vit`**

```vitte
mod math;
mod geometry;

pub use math::trig::sin   // re‑export direct
```

**`src/math/mod.vit`**

```vitte
mod trig;

pub fn square(x: i32) -> i32 { x * x }
```

**`src/math/trig.vit`**

```vitte
pub fn sin(x: f64) -> f64 { /* … */ }
pub fn cos(x: f64) -> f64 { /* … */ }
```

**`src/geometry/mod.vit`**

```vitte
pub struct Point { x: f64, y: f64 }

use self::Point as P

pub fn area_square(side: f64) -> f64 { side * side }
```

**Utilisation**

```vitte
use math::square
use math::trig::{sin, cos}
use geometry::area_square

fn main() {
    let a = area_square(4.0)
    print(square(3) as i32)
    print(sin(1.0))
}
```

---

## 11) Grammaire (compléments EBNF)

```ebnf
mod_item   ::= 'mod' ident (';' | block) ;
export     ::= 'pub' ;

// rappel use_tree plus haut
path       ::= ident { '::' ident } | relpath ;
relpath    ::= ('self' | 'super') '::' ident { '::' ident } ;
```

---

## 12) Tooling (LSP / Formatter / Docs)

* **Resolver** produit : tables `def_of`, `ref_of`, `exports` (voir `vitte-resolver`).
* LSP : navigation inter‑fichiers (`go‑to‑def`, `find‑refs`), hovers, renames sûrs (respect de l’alias `as`).
* Formatter : ordonne les `mod …;` en tête de fichier, groupe les `use` par racine (`math::…`, `geometry::…`).
* Docs : re‑exports **réattribuent** l’ownership doc au symbole original avec un lien de provenance.

---

## 13) Intégration `vitpm` (survol)

* Chaque dépendance `vitpm` est montée sous un **préfixe racine** égal au **nom du package**.

  * Exemple : package `json` → `use json::value::Value`.
* Les versions/résolutions sont **hors code** (manifest/lock gérés par `vitpm`).

---

## 14) Compatibilité & phase‑in

* **v0.1** : pas de glob `*`, pas de visibilités fines (`pub(crate)`, `pub(super)`).
* `self`/`super` **contextuels en chemins uniquement** (amende RFC 0001).
* Migrations automatisables : rename imports, re‑exports contrôlés par l’outil de refactor.

---

## 15) Alternatives considérées

* Modules **implicites** par hiérarchie (Python‑like) → rejeté (magie, tooling fragile).
* **Namespaces uniques** (C++‑like) → trop verbeux, import ambigu.
* Import **relatif uniquement** → manque de clarté globale, path frági.

---

## 16) Questions ouvertes

* Introduire `pub(crate)`, `pub(super)`, `pub(package)` ? (v0.2+)
* Autoriser `use *` derrière un flag `unstable-import-glob` ?
* Statut des **membres statiques** de types dans les chemins (v1.0‑ ?) : `Type::const`.

---

## 17) Références

* Rust Reference — Modules & Paths
* C# — Namespaces & using
* Python — Modules & packages
