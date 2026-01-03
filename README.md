# Vitte — un langage simple, clair, et coherent

Vitte est un langage en construction qui cherche a rester lisible, regulier, et accessible. Il vise une experience "sans surprise" : une syntaxe nette, des regles explicites, et des erreurs compréhensibles.

Ce README presente le langage de maniere detaillee, avec un exemple complet et un tour d'horizon des grandes idees.

## Exemple (syntaxe "phrase")

```vitte
mod hello.app
use core.math as math

type Point
  field x : Int
  field y : Int
.end

fn add a:Int b:Int -> Int
  set sum = a + b
  ret sum
.end

scn example
  set total = add 2 3
  when total > 0
    say "positif"
  else
    say "zero ou negatif"
  .end

  loop i from 0 to 3 step 1
    say i
  .end
.end

prog hello.app.main
  do example
.end
```

## Exemple (syntaxe "core")

```vitte
module hello.app
import core.math as math

struct Point
  x: Int
  y: Int
.end

fn add(a: Int, b: Int) -> Int
  return a + b
.end

program hello.app
  let total: Int = add(2, 3)
  if total > 0
    return 0
  else
    return 1
  .end
.end
```

## Idees principales

- **Lisibilite avant tout** : un code qui se lit naturellement.
- **Regles explicites** : pas de magie cachee.
- **Uniformite** : meme style pour les declarations, blocs et expressions.
- **Progression douce** : commencer simple, aller plus loin sans changer de modele mental.

## Pourquoi Vitte ?

Vitte veut rester lisible, explicite et uniforme : peu de surprises, une structure visible, et une progression claire entre l'idee et le code. L'objectif est d'offrir un socle simple, sans sacrifier l'expressivite ni la rigueur.

- **Lisibilite avant tout** : une syntaxe reguliere pour limiter les ambiguïtes.
- **Regles explicites** : des choix visibles et stables, faciles a anticiper.
- **Uniformite** : un style coherent pour declarations, blocs et expressions.
- **Progression douce** : avancer sans changer de modele mental.

## Deux syntaxes, un seul langage

Vitte propose deux formes de syntaxe qui representent la meme chose :

- **Syntaxe phrase** : plus courte, plus directe, pensée pour l'ecriture rapide.
- **Syntaxe core** : plus explicite et plus reguliere, utile pour les outils et les specs.

Le projet desugare la syntaxe phrase vers la syntaxe core.

## Modules et importations

Le langage est organise par modules. La syntaxe permet :

- declarer un module (ex. `mod` ou `module`),
- importer un module avec ou sans alias,
- exposer un ensemble de symboles si besoin.

## Types

Vitte propose des types simples et clairs :

- types de base (entiers, flottants, booleens, chaines),
- `struct` pour regrouper des champs,
- `enum` pour des variantes avec ou sans donnees,
- `type` pour definir un alias.

### Systeme de types (resume)

- **Generiques** : les types peuvent prendre des arguments, par exemple `Vec[Int]` ou `Result[Int, Error]`.
- **Enums avec payload** : une variante peut porter des valeurs nommees, par exemple `enum Msg Ok(value: Int) Err(code: Int) .end`.
- **Pattern matching** : `match` peut deconstruire des enums et tester des litteraux, par exemple `Msg::Ok(value: v)` ou `_`.

## Fonctions et scenarios

Les fonctions sont declarees avec `fn`. La forme "phrase" permet des parametres sans parenthese, la forme "core" utilise une liste explicite.

Les **scenarios** (mot-cle `scn` ou `scenario`) sont un bloc d'instructions qui peut servir de script ou d'exemple.

## Flux de controle

La grammaire inclut :

- `if` / `elif` / `else`,
- boucles `while` et `for ... in ...`,
- `match` avec patterns (litteraux, identifiants, variantes).

## Expressions

Les expressions supportent :

- operateurs arithmetiques et logiques,
- appels de fonctions,
- acces aux champs,
- indexation.

## Points d'entree

Vitte distingue plusieurs "entrees" possibles, comme `program`, `service`, `kernel`, `driver`, `tool`, `pipeline`. L'idee est de declarer l'intention d'un module d'execution.

## FAQ

**Pourquoi deux syntaxes ?**  
Pour separer l'ecriture rapide (phrase) de la forme canonique, plus reguliere pour les outils. La syntaxe phrase est desugaree en syntaxe core.

**A quoi sert `.end` ?**  
`.end` marque explicitement la fin d'un bloc (types, fonctions, boucles, etc.). Cela rend la structure claire sans acolades.

**Exemple rapide (generiques + match)**  
```vitte
type IntVec = Vec[Int]

enum Result
  Ok(value: Int)
  Err(code: Int)
.end

fn example(value: Int) -> Int
  let res: Result = Result::Ok(value)
  match res
    Result::Ok(value: v) => return v
    Result::Err(code: _) => return -1
  .end
.end
```

**Version phrase (structure simple)**  
```vitte
fn add a:Int b:Int -> Int
  ret a + b
.end

scn example
  set total = add 2 3
  when total > 0
    say "positif"
  else
    say "zero ou negatif"
  .end
.end
```

**Le `match` existe-t-il en syntaxe phrase ?**  
Non, pas pour l'instant : la syntaxe phrase est un sous-ensemble et `match` reste en core.

## Mini lexique

- `mod` / `module` : declare un module.
- `use` / `import` : importe un module, optionnellement avec alias.
- `struct` : regroupe des champs nommes.
- `enum` : declare des variantes (avec ou sans payload).
- `type` : definit un alias de type.
- `fn` : declare une fonction.
- `scn` / `scenario` : declare un scenario (script).
- `prog` / `program` : declare un point d'entree executable.
- `set` / `let` / `const` : declaration et affectation.
- `when` / `if` : conditions (phrase / core).
- `loop` / `for` / `while` : boucles.
- `match` : pattern matching.
- `.end` : fin explicite d'un bloc.

## Etat actuel

Le langage est encore en construction. Le parsing de la syntaxe "phrase" est operationnel, et la structure interne existe. Le parsing "core" et l'integration complete avancent progressivement.

## Ou en savoir plus

- Grammaire : `grammar/vitte.pest`
- Documentation : `docs/`
- Specifications : `spec/`
- Exemples : `examples/`
