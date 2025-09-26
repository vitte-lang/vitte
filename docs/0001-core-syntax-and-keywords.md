---

rfc: 0001
title: "Core Syntax and Keywords"
status: proposed
authors: \["Vincent"]
shepherd: "Core Team"
reviewers: \["@frontend", "@vm", "@lsp"]
created: 2025-09-02
updated: 2025-09-02
tracking-issues: \["vitte-lang/vitte#1"]
requires: \[]
supersedes: \[]
superseded-by: \[]
discussions-to: "[https://github.com/vitte-lang/vitte/discussions](https://github.com/vitte-lang/vitte/discussions)"
target-release: "v0.1"
labels: \["lang", "syntax", "keywords", "breaking?"]
----------------------------------------------------

> **Résumé** — Ce RFC fixe la **grammaire de base** de Vitte et l’ensemble **minimal** de mots‑clés réservés. Objectifs : lisibilité, régularité, potentiel d’évolution et outillage facile (LSP, formatter, lints).

---

## 1) Motivation

* Assurer une **base stable** pour le parseur, les outils (LSP/formatter) et les docs.
* Prévenir les **collisions futures** de mots‑clés.
* Définir des invariants syntaxiques qui facilitent l’optimisation et l’implémentation du **bytecode/VM**.

## 2) Objectifs / Non‑objectifs

**Objectifs**

* O1 — Définir les **lexèmes** (identificateurs, littéraux, opérateurs, ponctuation).
* O2 — Définir une **grammaire** (EBNF) des constructions de base : déclarations, blocs, fonctions, types nominaux, `match`.
* O3 — Spécifier les **mots‑clés** (réservés vs futurs) et règles de **terminaison** des instructions.
* O4 — Fixer une **table de précédence/associativité** des opérateurs.

**Non‑objectifs**

* N1 — Sémantique fine de l’ownership/mémoire (RFC 0003).
* N2 — Erreur/`try`/`?` (RFC 0004).
* N3 — Macros/attributs (RFC 0008) et modules complets (RFC 0002). Ici on ne statue que sur la **forme** minimale.

---

## 3) Lexique (tokenisation)

### 3.1 Identificateurs

* Sensibles à la casse ; Unicode letters (XID\_Start/XID\_Continue), `_`, chiffres (non initiaux).
* Recommandations : `snake_case` variables/fonctions, `PascalCase` types/structs/enums, `SCREAMING_SNAKE_CASE` constantes.

### 3.2 Littéraux

* **Entiers** : `42`, `0xFF`, `0o77`, `0b1010` ; séparateurs `_` permis (`1_000_000`).

  * Suffixes de type : `i8,i16,i32,i64,i128,isize` • `u8,…,usize`.
* **Flottants** : `3.14`, `1e-9`, `1_000.0`, suffixes `f32`,`f64`.
* **Booléens** : `true`, `false`.
* **Caractères** : `'a'`, `'\n'`, `'\u{1F600}'`.
* **Chaînes** : `"texte"` (évasions : `\n \r \t \\ \" \xHH \u{…}`), chaînes **brutes** : `r"..."`, `r#"..."#` (aucune évasion).
* **Null** : `null` (littéral spécial ; sémantique détaillée en RFC 0004/0005).

### 3.3 Commentaires

* Ligne : `// ...`
* Bloc : `/* ... */` (imbriqués autorisés).
* Documentation : `///` (item) • `//!` (module/crate).

### 3.4 Ponctuation & opérateurs (jetons)

```
() [] {} , . ; : :: -> => =>> ? .. ..= #
+ - * / % **
& | ^ ~ << >>
= += -= *= /= %= &= |= ^= <<= >>=
== != < <= > >=
&& || !
```

> `**` (puissance) est optionnel/unstable et **hors spec** pour v0.1 — réservé.

---

## 4) Terminaison des instructions (ASI contrôlé)

Vitte emploie des **points‑virgules optionnels** :

* Fin d’instruction à **nouvelle ligne** *sauf si* la ligne se termine par un **token de continuation** : opérateur binaire, `,`, `:`, `->`, `=>`, `(`, `[`, `{`.
* `;` reste toujours accepté pour forcer une coupure.
* Dans un **bloc `{ ... }`**, les instructions sont séparées par **NL ou `;`**. L’expression **finale d’un bloc** *sans* `;` est sa **valeur** (retour implicite), sinon `()`.

---

## 5) Grammaire (EBNF simplifiée)

```ebnf
program      ::= { item } ;
item         ::= fn_item | struct_item | enum_item | type_alias | use_item | mod_item | const_item ;

ident        ::= XID_Start { XID_Continue } ;
path         ::= ident { '::' ident } ;

block        ::= '{' { stmt } [ expr ] '}' ;
stmt         ::= let_stmt | const_stmt | expr_stmt | item ;
expr_stmt    ::= expr [ ';' ] ;

let_stmt     ::= 'let' [ 'mut' ] pattern [ ':' type ] [ '=' expr ] ;
const_stmt   ::= 'const' ident [ ':' type ] '=' expr ;

fn_item      ::= vis? 'fn' ident '(' params? ')' ret? block ;
params       ::= param { ',' param } ;
param        ::= pattern [ ':' type ] ;
ret          ::= '->' type ;

struct_item  ::= vis? 'struct' ident '{' fields? '}' ;
fields       ::= field { ',' field } ;
field        ::= ident ':' type ;

enum_item    ::= vis? 'enum' ident '{' variants? '}' ;
variants     ::= variant { ',' variant } ;
variant      ::= ident [ '(' types? ')' | '{' fields? '}' ] ;

use_item     ::= 'use' use_path ;
use_path     ::= path [ 'as' ident ] ;

mod_item     ::= 'mod' ident block? ;

type_alias   ::= vis? 'type' ident '=' type ;

expr         ::= assign ;
assign       ::= logic_or { assign_op assign } ;
assign_op    ::= '=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^=' | '<<=' | '>>=' ;
logic_or     ::= logic_and { '||' logic_and } ;
logic_and    ::= bit_or { '&&' bit_or } ;
bit_or       ::= bit_xor { '|' bit_xor } ;
bit_xor      ::= bit_and { '^' bit_and } ;
bit_and      ::= equality { '&' equality } ;
equality     ::= compare { ( '==' | '!=' ) compare } ;
compare      ::= shift { ( '<' | '<=' | '>' | '>=' ) shift } ;
shift        ::= add { ( '<<' | '>>' ) add } ;
add          ::= mul { ( '+' | '-' ) mul } ;
mul          ::= unary { ( '*' | '/' | '%' ) unary } ;
unary        ::= ( '!' | '-' | '+' | '~' ) unary | postfix ;
postfix      ::= primary { call | index | field } ;
call         ::= '(' [ args ] ')' ;
args         ::= expr { ',' expr } ;
index        ::= '[' expr ']' ;
field        ::= '.' ident ;
primary      ::= lit | path | group | lambda | match | if_else ;

lit          ::= INT | FLOAT | STRING | CHAR | 'true' | 'false' | 'null' ;
group        ::= '(' expr ')' ;

lambda       ::= '\\' params_lam '->' expr | 'fn' '(' params? ')' ret? '=>'
                 ( expr | block ) ;
params_lam   ::= ident { ',' ident } ;

if_else      ::= 'if' expr block { 'else' 'if' expr block } [ 'else' block ] ;

match        ::= 'match' expr '{' arms '}' ;
arms         ::= arm { ',' arm } [ ',' ] ;
arm          ::= pattern [ 'if' expr ] '=>' ( expr | block ) ;

pattern      ::= '_' | ident | lit | '(' patterns? ')' | path '(' patterns? ')' | path '{' pat_fields? '}' ;
patterns     ::= pattern { ',' pattern } ;
pat_fields   ::= ident { ',' ident } ;

vis          ::= 'pub' ;

type         ::= path [ '<' types? '>' ] | '(' types? ')' | type '->' type ;
types        ::= type { ',' type } ;
```

---

## 6) Opérateurs : précédence & associativité

| Niveau (haut→bas) | Opérateurs                           | Associativité        |        |            |   |          |
| ----------------- | ------------------------------------ | -------------------- | ------ | ---------- | - | -------- |
| Postfix           | `f(…)` `a[b]` `a.b`                  | Gauche               |        |            |   |          |
| Unaire            | `!` `~` `+` `-`                      | Droite               |        |            |   |          |
| Multiplicatifs    | `*` `/` `%`                          | Gauche               |        |            |   |          |
| Additifs          | `+` `-`                              | Gauche               |        |            |   |          |
| Décalages         | `<<` `>>`                            | Gauche               |        |            |   |          |
| Comparaisons      | `<` `<=` `>` `>=`                    | Non associatifs      |        |            |   |          |
| Égalité           | `==` `!=`                            | Non associatifs      |        |            |   |          |
| Bit‑and/xor/or    | `&` `^` \`                           | \`                   | Gauche |            |   |          |
| Logiques          | `&&` \`                              |                      | \`     | Gauche (\` |   | `>`&&\`) |
| Assignation       | `=` `+=` `-=` `*=` `/=` `%=` `&=` \` | =` `^=` `<<=` `>>=\` | Droite |            |   |          |

> Le **range** `..`/`..=` est syntactique (opérateur binaire non associatif) et se place entre Comparaisons et Additifs.

---

## 7) Mots‑clés

### 7.1 Réservés (v0.1)

```
if, else, match, for, while, loop, break, continue,
fn, return, let, mut, const, static,
struct, enum, trait, impl, type, alias,
pub, use, mod, package,
async, await, spawn,
unsafe, extern, ffi,
null, true, false
```

### 7.2 Réservés pour usage futur (ne doivent pas être utilisés comme ident)

```
yield, defer, where, self, super, crate, try, operator
```

### 7.3 Mots‑clés contextuels

* `as` (dans `use … as …`, conversions futures).
* `in` (réservé pour `for … in …`).

---

## 8) Exemples canoniques

### 8.1 Variables & constantes

```vitte
let x = 42          // immuable
let mut y = 3.14    // mutable
const PI = 3.14159  // constante
```

### 8.2 Fonctions & retours

```vitte
fn add(a: i32, b: i32) -> i32 {
    a + b  // retour implicite de la dernière expression
}

fn add_explicit(a: i32, b: i32) -> i32 {
    return a + b
}
```

### 8.3 Structures & enums

```vitte
struct Point { x: f64, y: f64 }

enum Result<T, E> { Ok(T), Err(E) }
```

### 8.4 Contrôle de flux

```vitte
if x > 0 { print("positif") }
else if x < 0 { print("négatif") }
else { print("zéro") }

for i in 0..10 { print(i) }

while condition { faire_truc() }
```

### 8.5 Pattern matching

```vitte
match result {
    Ok(value) => print(value),
    Err(e)    => panic(e),
}
```

### 8.6 Modules (forme minimale)

```vitte
mod math { pub fn square(x: i32) -> i32 { x * x } }
use math::square
```

### 8.7 Lambdas

```vitte
let id = \x -> x
let plus = \a, b -> a + b
```

---

## 9) Règles de formatage minimales (interop `vitte-fmt`)

* Indentation 4 espaces, accolades K\&R.
* Un **espace** autour des opérateurs binaires.
* Pas de `;` sauf besoin de séparer deux expressions sur la même ligne.
* `match` : flèches `=>` alignées par le formatter.

(Le style détaillé est dans RFC 0018.)

---

## 10) Diagnostics & LSP

* Messages d’erreur à **pointe unique** (file\:line\:col), *notes* pour suggestions.
* Tokens et scopes exposés via LSP (semantic tokens: `keyword`, `type`, `function`, `variable`, `enumMember`).

---

## 11) Compatibilité & stabilisation

* Les mots‑clés **réservés** ne seront pas relâchés avant 1.0.
* Les constructions marquées **futures** pourront être activées derrière un flag `unstable-…`.

---

## 12) Alternatives

* Syntaxe type‑inférence **à la ML only** — écartée pour conserver une familiarité C‑like et une migration plus douce.
* Indentation‑sensible à la Python — écartée (ambiguïtés espace/indent, tooling plus fragile).

---

## 13) Questions ouvertes

* `null` : restreint à l’FFI ? coercion vers un `?T` optionnel ? (RFC 0004/0005)
* `trait/impl` : réservés, mais surface exacte renvoyée à RFC future.

---

## 14) Références

* Rust Reference (grammaire & tokens)
* C# Language Spec (expressions, `=>` syntax)
* C99 (opérateurs et précédence)
