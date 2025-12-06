# Spécification du langage Vitte – Noyau 2025

> Version : 0.1 – Édition du langage : **2025**  
> Statut : brouillon stable (base de référence pour `vitte-core`)

Ce document décrit la **spécification du noyau du langage Vitte** pour l’édition **2025** telle qu’elle est modélisée dans le dépôt `vitte-core`.

Objectifs :

- définir la **syntaxe** et les **règles de base** du langage (lexique, grammaire, blocs, déclarations, expressions) ;
- clarifier la **sémantique** des constructions fondamentales (types, fonctions, contrôle, modules) ;
- aligner la définition du langage avec :
  - l’architecture du compilateur (`docs/compiler-architecture.md`) ;
  - l’architecture du bootstrap (`docs/architecture-bootstrap.md`) ;
  - les manifests Muffin (`muffin.muf`, `vitte.project.muf`, `bootstrap/*.muf`, `src/std/mod.muf`).

Le langage décrit ici est volontairement **concentré sur le noyau** : certaines fonctionnalités avancées (génériques riches, traits, macros, async, etc.) ne sont pas encore spécifiées et seront ajoutées dans des versions ultérieures.

---

## 1. Principes de conception

### 1.1. Vitte seulement

Vitte est un langage auto‑contenu :

- la spécification ne fait référence à **aucun autre langage** comme modèle direct ;
- les constructions de surface, les types et la sémantique sont définis proprement pour Vitte, même si certains concepts peuvent être familiers.

### 1.2. Blocs explicites, `.end`, pas d’accolades

Conventions fondamentales :

- aucune accolade (`{`, `}`) n’est utilisée pour délimiter les blocs de code ;
- les blocs sont introduits par une construction (ex. `fn`, `if`, `loop`) et **fermés explicitement** par le mot-clé `.end` ;
- l’indentation est **significative** pour la lisibilité et une partie de la grammaire, mais la fermeture du bloc ne repose jamais uniquement sur l’indentation.

Exemple :

```vitte
fn add(a: i32, b: i32) -> i32
    let c = a + b
    return c
.end
```

### 1.3. Lisibilité et prédictibilité

Les décisions de design visent à garder Vitte :

- **lisible** : syntaxe claire, mots‑clés explicites ;
- **prédictible** : pas de règles implicites surprenantes ;
- **progressif** : un débutant peut écrire du code simple, un expert peut structurer des systèmes complexes.

---

## 2. Lexique et structure lexicale

### 2.1. Caractères et encodage

- Les sources Vitte sont encodées en **UTF‑8**.
- Les caractères ASCII standard sont acceptés partout où ils sont pertinents (identifiants, symboles, opérateurs).
- L’usage de caractères non ASCII dans les identifiants est à définir dans une extension ultérieure (le noyau 2025 se limite aux caractères ASCII pour les identifiants).

### 2.2. Espaces blancs

Les **espaces blancs** incluent :

- le caractère espace (`U+0020`) ;
- la tabulation horizontale (`U+0009`) ;
- les fins de ligne (`\n`, `\r\n`).

Règles :

- les fins de ligne séparent les **lignes logiques** de code ;
- les blocs utilisent l’indentation pour la lisibilité ;
- les tabulations sont autorisées mais la recommandation est d’utiliser 4 espaces (voir `.editorconfig`), l’implémentation doit traiter tab/espaces de façon prévisible dans la grammaire d’indentation.

### 2.3. Commentaires

Vitte supporte les commentaires de ligne :

```vitte
# Ceci est un commentaire de ligne
let x = 42  # commentaire en fin de ligne
```

Règles :

- un commentaire commence par `#` et s’étend jusqu’à la fin de la ligne ;
- les commentaires sont ignorés par le compilateur sauf pour les besoins de diagnostics et, éventuellement, de documentation.

Les commentaires multilignes pourront être introduits dans une édition ultérieure.

### 2.4. Identifiants

Un **identifiant** Vitte suit les règles suivantes :

- premier caractère : lettre ASCII (`a`–`z`, `A`–`Z`) ou `_` ;
- caractères suivants : lettres ASCII, chiffres (`0`–`9`) ou `_`.

Exemples valides :

```vitte
x
_foo
myVariable
parse_int32
Vec2D
```

Les identifiants sont **sensibles à la casse** (`foo` ≠ `Foo`).

Les mots‑clés réservés ne peuvent pas être utilisés comme identifiants.

### 2.5. Mots‑clés réservés

La liste des mots‑clés réservés du noyau 2025 inclut (non exhaustive, extensible) :

- **structure du module / programme** :
  - `module`, `import`, `export`, `program`, `scenario`, `pipeline`
- **déclarations** :
  - `fn`, `struct`, `enum`, `union`, `type`, `let`, `const`
- **contrôle** :
  - `if`, `else`, `when`, `loop`, `for`, `match`, `break`, `continue`, `return`
- **terminateurs / structure** :
  - `.end` (fermeture de bloc)

Le symbole `.end` est traité comme un **mot‑clé particulier** : il ferme le bloc le plus proche dont la structure attend une fermeture.

### 2.6. Littéraux

#### 2.6.1. Littéraux entiers

Forme de base :

```vitte
0
42
123456
```

Extensions possibles (selon la grammaire officielle) :

- séparateurs `_` pour la lisibilité : `1_000_000`
- préfixes pour les bases :
  - `0b1010` (binaire),
  - `0o755` (octal),
  - `0xFF` (hexadécimal).

La sémantique de typage (ex. `i32`, `u64`, etc.) est déterminée à partir du contexte et/ou d’un suffixe éventuel, selon l’évolution de la spéc.

#### 2.6.2. Littéraux flottants

Forme typique :

```vitte
0.0
3.14
1.0e10
2.5E-3
```

À dépendre de la grammaire détaillée : support des suffixes de type, exponents, etc.

#### 2.6.3. Littéraux booléens

Deux littéraux booléens sont définis :

```vitte
true
false
```

#### 2.6.4. Littéraux chaînes de caractères

Forme de base :

```vitte
"hello"
"Vitte\nlang"
"\"quoted\""
```

Règles :

- délimités par des guillemets doubles `"` ;
- support des séquences d’échappement de base (`\n`, `\t`, `\\`, `\"`, etc.) ;
- le support de chaînes multi‑ligne et brutes est laissé à une extension ultérieure.

#### 2.6.5. Littéraux `null` / unit

Le noyau peut définir :

- un littéral de type « unité » : `()` (équivalent “void” structuré) ;
- la présence ou non d’un `null` explicite dépend des choix de safety (à préciser dans une révision ultérieure de la spéc, la recommandation étant d’éviter un `null` non typé).

---

## 3. Structure globale d’un fichier Vitte

Un fichier Vitte est une **unité de compilation**.

Sa structure générale :

```vitte
module my.project.module

import my.project.other
import std.io
import std.collections as coll

# déclarations de niveau module
struct Point
    x: f64
    y: f64
.end

fn distance(a: Point, b: Point) -> f64
    let dx = a.x - b.x
    let dy = a.y - b.y
    return sqrt(dx * dx + dy * dy)
.end
```

### 3.1. Déclaration de module

La première déclaration non vide devrait être une déclaration de module :

```vitte
module my.project.module
```

Règles :

- un fichier a au plus **un** `module` ;
- le nom de module suit la convention `ident[.ident]*` ;
- la correspondance exacte nom de module ⇔ chemin de fichier est définie par la toolchain (par exemple, `my/project/module.vitte`).

### 3.2. Imports

Les **imports** permettent de référencer des symboles définis dans d’autres modules ou dans la std :

```vitte
import my.project.other
import std.io
import std.collections as coll
```

Formes possibles :

- import direct d’un module ;
- import avec alias (`as`) ;
- import sélectif (future extension) :

```vitte
import std.collections { Vec, Map }  # exemple potentiel
```

### 3.3. Export

Les modules peuvent déclarer explicitement ce qui est exporté :

```vitte
export Point
export distance
```

À défaut, une convention de visibilité (public/privé) est appliquée, définie dans la sémantique du compilateur.

---

## 4. Déclarations

### 4.1. Déclaration de variable (`let`)

Syntaxe :

```vitte
let x = 42
let y: i32 = 10
```

Règles :

- `let` introduit une variable **mutable ou immuable** selon le design (le noyau peut commencer avec des variables immuables par défaut, une variante `let mut` pourra être introduite) ;
- le type peut être :
  - **inféré** : `let x = 42` ;
  - **annoté** : `let y: i32 = 10`.

### 4.2. Constantes (`const`)

Syntaxe :

```vitte
const PI: f64 = 3.14159
```

Règles :

- `const` introduit une constante immuable ;
- la valeur doit être une expression **constante** (évaluable à la compilation).

### 4.3. Structures (`struct`)

Syntaxe :

```vitte
struct Point
    x: f64
    y: f64
.end
```

Règles :

- `struct` introduit un type structuré nommé ;
- le corps liste les champs `ident: Type` un par ligne ;
- la déclaration se termine par `.end`.

### 4.4. Enums (`enum`)

Syntaxe :

```vitte
enum Option[T]
    Some(value: T)
    None
.end
```

Les enums peuvent être **simplement énumératives** ou **sum types** avec payloads.  
La syntaxe exacte générique (`[T]`) est donnée à titre illustratif et pourra être raffinée dans la grammaire officielle.

### 4.5. Unions (`union`)

`union` représente des unions de données à bas niveau (interop, performance).  
La spéc définit leur sémantique de sécurité (accès aux champs, invariants).

### 4.6. Alias de type (`type`)

Syntaxe :

```vitte
type Meter = f64
```

Permet de définir des alias de type sans créer un nouveau type nominal.

### 4.7. Fonctions (`fn`)

Syntaxe générale :

```vitte
fn name(param1: Type1, param2: Type2) -> ReturnType
    # corps de fonction
    ...
.end
```

Exemple :

```vitte
fn add(a: i32, b: i32) -> i32
    return a + b
.end
```

Règles :

- paramètres explicitement typés ;
- retour obligatoire ou implicite selon les conventions (ex : expression finale sans `return`) ;
- le corps est un **bloc** terminé par `.end`.

---

## 5. Blocs, instructions et `.end`

### 5.1. Bloc général

Un **bloc** est une séquence d’instructions :

```vitte
# bloc anonyme
let x = 10
let y = x + 2

# bloc dans une fonction
fn example()
    let x = 10
    let y = x + 2
.end
```

Les blocs sont :

- introduits par une construction (fn, if, loop, match, etc.) ;
- terminés par `.end`.

### 5.2. Instructions de base

Les instructions du noyau incluent :

- déclarations (`let`, `const`, `struct`, `enum`, etc.) au bon niveau ;
- affectations ;
- appels de fonction ;
- instructions de contrôle (`if`, `when`, `loop`, `for`, `match`, `break`, `continue`, `return`) ;
- expression nue (si le langage les autorise comme statements).

---

## 6. Contrôle de flux

### 6.1. `if` / `else`

Syntaxe :

```vitte
if condition
    # bloc si vrai
    ...
else
    # bloc sinon
    ...
.end
```

Règles :

- `condition` est une expression booléenne ;
- le bloc `else` est optionnel ;
- c’est le `.end` final qui ferme la structure `if`/`else`.

### 6.2. `when`

`when` peut être utilisé comme une variante d’`if` plus déclarative (par exemple pour branchement multi‑conditions ou pattern matching simplifié).  
La forme exacte est à préciser dans la grammaire détaillée, mais respecte également le schéma bloc + `.end`.

### 6.3. Boucles (`loop`, `for`)

Syntaxes typiques :

```vitte
loop
    # boucle infinie
    if should_stop()
        break
    .end
.end

for item in items
    process(item)
.end
```

Règles :

- `loop` introduit une boucle infinie jusqu’à `break` ;
- `for` itère sur un itérable ;
- `break` sort de la boucle ;
- `continue` passe à l’itération suivante.

### 6.4. `match`

`match` permet un dispatch conditionnel sur une expression ou un type d’énumération ; la syntaxe détaillée est à définir, mais respecte le modèle bloc + `.end`.

---

## 7. Expressions

Les expressions du noyau incluent :

- littéraux ;
- identifiants ;
- appels de fonction ;
- opérations arithmétiques et logiques ;
- accès champ (e.g. `point.x`) ;
- constructions de structures et enums.

### 7.1. Appels de fonction

```vitte
let result = add(1, 2)
```

Règles :

- la résolution de `add` passe par les scopes ;
- les types d’arguments et de retour sont vérifiés par le type checker.

### 7.2. Opérateurs

Opérateurs arithmétiques de base :

- `+`, `-`, `*`, `/`, `%`.

Opérateurs de comparaison :

- `==`, `!=`, `<`, `<=`, `>`, `>=`.

Opérateurs logiques :

- `&&`, `||`, `!`.

La priorité et l’associativité sont définies dans la grammaire ; la spéc doit inclure un tableau de précisions (à compléter).

---

## 8. Système de types (noyau)

Le système de types du noyau couvre :

- types de base :
  - entiers signés/non signés (ex. `i32`, `u64`),
  - flottants (ex. `f32`, `f64`),
  - booléens (`bool`),
  - chaînes (`string`),
  - type unité (`()`).
- types structurés :
  - `struct`, `enum`, `union` ;
- types composés :
  - vecteurs, maps, etc. via la std (`std.collections`).

La spéc sépare :

- la **représentation** des types (module `vitte.compiler.types`) ;
- la **vérification** des types (`vitte.compiler.typecheck`).

Les règles de compatibilité de types (promotions, conversions, surcharges éventuelles) sont précisées dans une révision détaillée du système de types.

---

## 9. Modules, visibilité et liens

### 9.1. Modules

Les modules sont définis par `module` en tête de fichier.  
Ils organisent le code en namespaces hiérarchiques (`a.b.c`).

### 9.2. Visibilité

La spéc prévoit une notion de visibilité :

- symboles publics exportables ;
- symboles internes au module.

La surface syntaxique exacte (`pub`, etc.) reste à préciser, mais doit être compatible avec la structure des manifests.

### 9.3. Lien logique

Le compilateur regroupe les modules en **unités de compilation** selon :

- les manifests Muffin de projet ;
- les imports déclarés.

Le lien logique (`vitte.compiler.link`) assemble ces unités en bundle de bytecode exécutable.

---

## 10. Erreurs, diagnostics et comportement indéfini

### 10.1. Erreurs de compilation

Les erreurs de compilation **doivent** être détectées pour :

- erreurs lexicales (caractères invalides, littéraux mal formés) ;
- erreurs syntaxiques (structures non fermées, `.end` manquant, etc.) ;
- erreurs de type ;
- références de symboles non résolus.

Chaque erreur inclut au minimum :

- un code d’erreur ;
- un message lisible ;
- un span précis dans le code source.

### 10.2. Warnings

Des avertissements peuvent être émis pour :

- code mort évident ;
- variables non utilisées ;
- constructions redondantes.

Les warnings ne bloquent pas la compilation, sauf si un mode strict est activé.

### 10.3. Comportement indéfini

La spéc doit identifier les cas de **comportement indéfini** (UB) ou non spécifié, par exemple :

- dépassements sur certains types numériques si non définis comme saturés ;
- utilisation de données non initialisées si la sémantique n’interdit pas ces cas.

La préférence est de **réduire au minimum** le comportement indéfini observable.

---

## 11. Interactions avec la std et la VM

### 11.1. Std

- La std est disponible sous le namespace `std.*` (ex. `std.io`, `std.collections`) ;
- certains éléments de la std peuvent être considérés comme **intrinsics** pour la compilation (optimisations) mais doivent rester des abstractions documentées.

### 11.2. VM

- Le compilateur génère un bytecode consommable par la VM Vitte ;
- l’ABI interne (conventions d’appel, layout des frames, etc.) est définie dans la spéc runtime/VM (document séparé), mais le compilateur doit s’y conformer strictement.

---

## 12. Espace de noms Muffin vs langage

Les manifests Muffin (`*.muf`) ne sont pas du code Vitte :

- ils ont leur propre grammaire (déclarative) ;
- ils décrivent la structure des projets, non la logique d’exécution.

La spéc du langage Vitte doit rester distincte de la spéc Muffin, tout en étant compatible avec les besoins de la toolchain (noms de modules, profils, artefacts).

---

## 13. Évolutions de la spécification

Cette spécification du noyau est appelée à évoluer en suivant :

- les éditions du langage (ex. 2025, 2027, …) ;
- les besoins du compilateur et du runtime ;
- les retours d’usage.

Principes d’évolution :

- ne pas casser les programmes valides au sein d’une même édition, sauf cas exceptionnels documentés ;
- introduire les changements incompatibles dans une nouvelle édition, avec des mécanismes d’opt‑in (manifests, flags) et/ou des outils de migration.

---

## 14. Résumé

- Vitte est un langage à blocs explicites, sans accolades, utilisant `.end` pour fermer les blocs.
- La structure lexicale est simple (UTF‑8, identifiants ASCII, commentaires `#`, littéraux de base).
- Les fichiers sont organisés en modules (`module` + `import`), éventuellement exportés.
- Le noyau définit des déclarations (`let`, `const`, `struct`, `enum`, `union`, `type`, `fn`), des blocs et un contrôle de flux (`if`, `when`, `loop`, `for`, `match`, `break`, `continue`, `return`).
- Le système de types couvre les types de base, structurés et composés, avec un type checker dédié.
- Le compilateur transforme les programmes en IRs successifs jusqu’au bytecode, exécuté par une VM dédiée, avec la std comme contrat minimal.
- Les manifests Muffin décrivent la structure des projets, mais sont séparés de la spéc du langage.
- La spécification est conçue comme une base stable pour l’édition 2025, avec de la marge pour des fonctionnalités avancées dans les éditions ultérieures.

Ce document sert de référence pour toutes les décisions liées à la syntaxe et aux règles de base du langage Vitte dans le dépôt `vitte-core`.
