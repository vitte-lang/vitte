# Vitte core – Type System (Noyau 2025, MVP)

Ce document fixe la **spécification minimale** du système de types pour le **Vitte MVP 2025**. Elle aligne la documentation, la grammaire (`grammar/vitte.pest`, `grammar/vitte.ebnf`) et les exemples exécutables (`tests/data/mini_project`).

Objectifs immédiats :

- donner une base **stable** pour le front-end (résolution, HIR), le type checker (`vitte.compiler.types` / `vitte.compiler.typecheck`) et les IR ;
- décrire la syntaxe et les règles de typage réellement supportées dans le MVP ;
- signaler clairement ce qui est hors périmètre (génériques riches, ownership, async, etc.).

---

## 1. Cadre général

- **Système statique** : la plupart des erreurs de type sont détectées à la compilation ; un programme bien typé ne déclenche pas d’erreur de type au runtime (hors erreurs d’exécution classiques : I/O, débordements non définis, etc.).
- **Types nominaux** : `struct`, `enum`, `union` et les types de la std sont nominaux ; deux définitions distinctes ne sont jamais interchangeables.
- **Spécification verrouillée** : seules les constructions décrites ici font partie du MVP. Toute extension (traits, génériques avancés, effets) doit être ajoutée explicitement.

---

## 2. Formes de types retenues

1. **Types de base**
   - entiers signés : `i8`, `i16`, `i32`, `i64` ;
   - entiers non signés : `u8`, `u16`, `u32`, `u64` ;
   - flottants : `f32`, `f64` ;
   - booléen : `bool` ;
   - chaîne : `string` ;
   - unité : `()` ;
   - optionnel : `never` (pour les fonctions qui ne retournent pas).

2. **Types nominaux structurés**
   - `struct` : produits nommés avec champs nommés ;
   - `enum` : sommes nommées avec variants, chacun pouvant porter un payload (liste nommée) ;
   - `union` : unions bas niveau (interop, performance) ; la sécurité dépend du programmeur.

3. **Alias**
   - `type Name = ExistingType` (alias transparent, pas de nouveau type).

4. **Arguments de type (fermés)**
   - forme : `TypeName[Arg1, Arg2, ...]` ;
   - pas de généricité implicite : chaque occurrence est **entièrement appliquée** ; aucune inférence d’arguments.

Les noms de types sont toujours qualifiés par leur module (`module.path.Type`). Les types de la std suivent la même règle (`std.collections.Vec[i32]`, etc.).

---

## 3. Syntaxe de déclaration (alignée sur la grammaire)

### 3.1. Modules et imports

```vitte
module app.main
import app.math
import std.string as str
```

- un fichier contient un seul `module`.
- `import` prend un chemin de module et optionnellement un alias.

### 3.2. `struct`

```vitte
struct Point
    x: f64
    y: f64
.end
```

- champs nommés, pas de champs anonymes ;
- pas de paramètres de type implicites sur la définition (utiliser `Point[T]` comme nom si nécessaire, mais les arguments doivent être donnés à chaque usage).

### 3.3. `enum`

```vitte
enum Result
    Ok(value: i32)
    Error(code: i32)
.end
```

- chaque variant peut avoir **0 ou plusieurs** champs nommés ;
- notation de construction : `Result::Ok(1)` ou `Result::Error(42)` ; l’ordre suit la payload list.

### 3.4. `union`

```vitte
union RawNumber
    as_i32: i32
    as_f32: f32
.end
```

- aucune garantie de sûreté automatique ; toute lecture/écriture d’un champ repose sur des invariants définis par le code appelant.

### 3.5. Alias

```vitte
type Distance = f64
type Label = string
```

- alias transparent : `Distance` et `f64` sont le même type pour le type checker.

---

## 4. Littéraux et constructions de valeur

- **Littéraux** : entiers (`42`, `0xFF`), flottants (`3.14`, `1.0e-3`), booléens (`true`, `false`), chaînes (`"Hello"`).
- **Unit** : `()` est utilisé pour signifier « pas de valeur utile ».
- **Struct literal** : `Point { x = 1.0, y = 2.0 }` ; chaque champ doit être fourni, aucune valeur par défaut dans le MVP.
- **Enum constructor** : `Result::Ok(1)` ou `Result::Error(2)`.
- **Pas de `null`** dans le MVP ; les valeurs absentes sont modélisées avec des `enum` (ex. `Option[T]`).

---

## 5. Bindings, mutabilité et portée

- `let` crée un binding **immuable** ; `let mut` crée un binding mutable.
- Annotations optionnelles sur les bindings locaux et constants :

```vitte
let count: i32 = 0
let mut total = 10
const limit: i32 = 100
```

- une assignation (`=` ou opérateurs composés) n’est autorisée que sur un binding mutable ou sur un champ d’une valeur mutable.
- les blocs sont terminés par `.end` ; la portée d’un binding est le bloc qui le contient.

---

## 6. Fonctions

Syntaxe :

```vitte
fn add(a: i32, b: i32) -> i32
    return a + b
.end
```

Règles :

- tous les paramètres et le type de retour sont **annotés** dans le MVP ;
- le type d’un appel `f(e1, e2, ..., en)` est le type de retour déclaré si chaque argument est typé comme attendu ;
- un appel de constructeur d’`enum` ou de `struct` suit les mêmes règles (vérification des champs/arguments).

---

## 7. Expressions et opérateurs

- **Unaires** : `-`, `not`.
- **Binaires** (associativité gauche) :
  - arithmétiques : `+`, `-`, `*`, `/`, `%` ;
  - comparaisons : `==`, `!=`, `<`, `<=`, `>`, `>=` (retournent `bool`) ;
  - logiques : `and`, `or` (opèrent sur `bool`).
- Les expressions composées (`a + b * c`) suivent la précédence définie par la grammaire (Pest / EBNF).
- Accès champ/index : `value.field`, `value[index]`.
- Appels : `callee(args...)` ; variantes `Type::Variant(...)` et `Type { ... }` sont des formes spécialisées.

---

## 8. Structures de contrôle

### 8.1. `if` / `elif` / `else`

- la condition doit être de type `bool` ;
- lorsqu’il produit une valeur, toutes les branches doivent avoir un type compatible (même type dans le MVP).

### 8.2. `while`

- forme : `while condition <block>` ;
- la condition est de type `bool` ; le bloc est de type `()` sauf si la boucle est non terminante (`never`).

### 8.3. `for ... in ...`

- itère sur une valeur itérable ; le type du binding dépend du type de l’itérateur exposé par la std (hors portée de la grammaire, à spécifier dans la std).

### 8.4. `match`

- seule forme de pattern autorisée dans le MVP :
  - `_` (joker) ;
  - littéraux (`42`, `"hi"`, `true`) ;
  - identifiant (binding) ;
  - variant d’`enum` avec payload optionnel : `Result::Ok(value)` ; payload = liste de sous-patterns.
- toutes les branches doivent retourner le même type ; la couverture des variants d’un `enum` est à contrôler (warning ou erreur selon le mode).

---

## 9. Inférence et annotations obligatoires

- **Inférence locale** : le compilateur infère le type d’un `let`/`let mut` à partir de l’expression de droite si aucune annotation n’est fournie.
- **Pas d’inférence globale** : les signatures de fonctions sont annotées, les arguments de type (`Vec[i32]`) sont toujours écrits.
- **Conversions automatiques** : absentes. Toute conversion entre entiers de tailles/signes différents doit être explicite (std ou intrinsic).

---

## 10. Erreurs de type et diagnostics

- incompatibilité de types dans une assignation ou un retour ;
- appel de fonction avec mauvais nombre/type d’arguments ;
- accès à un champ inexistant ou à une variant non déclarée ;
- branche de `match` manquante (selon la politique d’exhaustivité).

Les diagnostics pointent une `Span` précise, fournissent le type attendu / obtenu et, si possible, une suggestion (ex. « convertir `u32` en `i32` explicitement »).

---

## 11. Hors périmètre MVP / extensions prévues

- génériques avancés (traits, contraintes, higher-kinded) ;
- ownership/borrowing, références explicites, pointeurs bruts ;
- effets/async/concurrence ;
- `null` typé ou non typé ;
- promotion implicite des nombres, coercions automatiques ;
- pattern matching structurel sur `struct` (à ajouter ultérieurement si besoin).

Ces sujets doivent être ajoutés explicitement dans la spec et la grammaire avant d’être implémentés.

---

## 12. Résumé (MVP verrouillé)

- Types disponibles : primitives, `struct`/`enum`/`union` nominaux, alias transparents, arguments de type fermés.
- Bindings : `let` immuable, `let mut` mutable, annotations optionnelles localement ; assignation seulement sur mutables.
- Fonctions : paramètres et retour annotés ; appels vérifiés statiquement.
- Expressions : littéraux, accès champ/index, appels, constructions `Type { ... }` et `Type::Variant(...)`, opérateurs `-`, `not`, arithmétique, comparaison, logique.
- Contrôle : `if`/`elif`/`else`, `while`, `for ... in ...`, `match` sur `enum` avec variants + `_`.
- Pas de conversions implicites ni de `null` ; diagnostics précis attendus.

Cette spec sert de référence unique pour l’implémentation du type checker et de la grammaire Vitte MVP 2025.
