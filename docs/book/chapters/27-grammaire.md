# 27. Annexes : grammaire (description narrative)

Cette annexe décrit la syntaxe de Vitte en prose continue, avec des exemples courts. Elle sert de référence exhaustive, mais dans un style lisible. Chaque section explique le rôle de la construction, sa forme, et la logique qui la justifie. L’objectif n’est pas de fournir une grammaire formelle, mais un guide qui aide à écrire du code clair et stable.

## Structure générale d’un fichier

Un fichier Vitte est une suite d’éléments déclaratifs et d’éléments exécutables. La manière dont ces éléments sont ordonnés n’est pas seulement une question esthétique, elle définit la lisibilité du fichier. En pratique, un fichier lisible commence par son identité, puis ses dépendances, puis ses déclarations, et enfin ses procédures. Cette hiérarchie évite d’obliger le lecteur à faire des allers‑retours dans le fichier.

Une structure simple rend la compréhension immédiate. Lorsque le fichier grandit, cette discipline devient un gain réel, car elle transforme un bloc de texte en une architecture visible.

## Modules et espace de noms

La déclaration `space` associe le fichier à un chemin de module. Ce chemin est une adresse, pas un décor. Il doit rester cohérent avec l’arborescence réelle, sinon l’architecture devient un puzzle inutile. Le module est un contrat de visibilité, et ce contrat a plus de valeur que n’importe quelle optimisation de packaging.

```vit
space std/cli/args
```

Un bon `space` simplifie tout le reste, parce qu’il rend les imports cohérents et évite les collisions de noms. Un mauvais `space` force les exceptions et fragilise la maintenance.

## Imports et visibilité

L’import `use` rend des symboles visibles dans le fichier courant. L’import `pull` est réservé aux modules locaux, plus explicite et plus contrôlé. L’export `share` rend un symbole disponible pour les autres modules. Ces mots‑clés ne sont pas décoratifs, ils incarnent une politique de visibilité.

```vit
use std/cli
pull core/io as io
share read_config
```

Il est important de garder les imports ciblés. Un import trop large rend les dépendances invisibles et augmente le couplage. À long terme, la clarté des imports est un gain de maintenance.

## Constantes

Une constante (`const`) exprime une valeur immuable et stable. Elle doit être typée, nommée, et pensée comme un contrat. Les constantes remplacent les valeurs magiques et rendent l’intention lisible.

```vit
const MAX_RETRIES: i32 = 3
```

Si une valeur change souvent, ce n’est pas une constante. Utiliser `const` sans discipline est une source de bugs subtils.

## Alias de types

Un alias de type (`type`) donne un nom à une intention. Il n’ajoute pas de coût d’exécution, mais il augmente la lisibilité. Un alias comme `Path` est plus lisible qu’un `string` nu, et cette différence devient cruciale à grande échelle.

```vit
type Path = string
```

## Structures (`form`)

Une structure regroupe des champs cohérents. Elle doit correspondre à une réalité stable du domaine. Un `form` qui change en permanence indique un modèle flou. Chaque champ doit porter un invariant, et la structure doit être suffisamment petite pour être comprise rapidement.

```vit
form User {
  name: string
  uid: i32
}
```

## Variants (`pick`)

Un `pick` exprime un choix fini. C’est l’outil principal pour modéliser des états exclusifs. Remplacer un `pick` par des booléens multiples est un anti‑pattern classique, car cela crée des combinaisons invalides.

```vit
pick Status {
  Ok
  Err(msg: string)
}
```

Un `pick` bien choisi clarifie la logique. Il impose un traitement explicite des cas, ce qui réduit les bugs.

## Procédures (`proc`)

Une procédure est un contrat. Elle doit être courte, explicite, et claire sur ses effets. Une signature lisible est une promesse de stabilité. Si une procédure nécessite un commentaire pour être comprise, c’est un signal qu’elle est trop complexe.

```vit
proc add(x: i32, y: i32) -> i32 {
  return x + y
}
```

## Entrée (`entry`)

Le point d’entrée relie votre code à la plateforme. La forme `entry main at core/app` rend explicite cette liaison. Cette explicitation est un choix de design : elle évite l’ambiguïté et prépare le code à plusieurs cibles.

```vit
entry main at core/app {
  return 0
}
```

## Macros (`macro`)

Les macros sont puissantes et dangereuses. Elles doivent rester petites et documentées. Une macro qui cache une logique complexe devient un piège. Si une fonction suffit, il faut préférer une fonction.

```vit
macro nop() {
  asm("nop")
}
```

## Instructions de base

Les instructions `let`, `set`, `return`, et `give` constituent le cœur du langage. `let` déclare une variable. `set` rend explicite la mutation. `return` termine une procédure. `give` est une forme de retour explicite utilisée fréquemment dans la stdlib pour rendre le flux de valeur visible.

```vit
let x = 1
set x = x + 1
return x
```

## Expressions et opérateurs

Les expressions utilisent une grammaire classique, mais leur lisibilité doit être surveillée. Une expression dense devient rapidement opaque. La règle pragmatique est simple : si vous devez relire une expression, elle est trop dense et mérite un nom intermédiaire.

```vit
let b = baz(x)
let c = bar(b)
let r = foo(c)
```

## Contrôle de flux

Le contrôle de flux doit être lisible à voix haute. Les blocs explicites sont obligatoires pour éviter les ambiguïtés. `if`, `loop`, `for`, `break`, et `continue` forment la grammaire du flux. Une boucle claire vaut mieux qu’une boucle compacte et obscure.

```vit
if ready {
  return 0
} else {
  return 1
}

loop {
  if done { break }
}

for item in items {
  continue
}
```

## `match` et `when`

`match` exprime des alternatives explicites. Il doit être complet, et inclure un cas par défaut avec `otherwise`. `when` est utile pour tester un variant et garder le flux lisible. Ces constructions existent pour rendre les cas visibles, pas pour économiser des lignes.

```vit
match v {
  case x { }
  otherwise { }
}

when opt is Option.Some {
  // ...
}
```

## Littéraux

Les littéraux sont simples mais portent des décisions implicites. Utilisez des constantes pour éviter les valeurs magiques. Les littéraux acceptés sont les entiers, flottants, hexadécimaux, chaînes, et booléens.

```vit
const MAX: i32 = 3
let pi = 3.14
let hex = 0xFF
let s = "text"
```

## Attributs et ABI

Les attributs `#[...]` modifient la compilation ou la liaison. Ils doivent rester rares, visibles, et documentés. L’attribut `#[extern]` marque une frontière ABI. Cette frontière est un contrat binaire qui doit être documenté rigoureusement.

```vit
#[extern]
proc c_add(x: i32, y: i32) -> i32
```

## Lisibilité, style, et stabilité

La grammaire dit ce qui est valide, mais le style dit ce qui est lisible. La lisibilité est un choix, pas un accident. Un code lisible réduit les erreurs et accélère la maintenance. Un code illisible est un coût durable.

La stabilité d’un projet dépend de la stabilité de ses conventions. Quand un projet évolue, les conventions sont ce qui maintient l’unité. Si les conventions changent à chaque fichier, la syntaxe reste correcte, mais le projet devient illisible.

## Diagnostics, erreurs fréquentes, et bonnes pratiques

Les diagnostics doivent être lus comme des guides. Un message “expected `{`” indique un bloc manquant. Un “unexpected token” indique presque toujours un mot‑clé manquant ou un ordre incorrect. Ces erreurs ne sont pas des punitions, elles sont des indications de forme.

Les erreurs les plus fréquentes viennent de l’oubli des blocs, de l’usage de `set` sur des variables non déclarées, de `match` incomplets, ou d’imports incorrects. Ces erreurs sont évitables par des conventions simples et une discipline d’écriture régulière.

## Exemple complet annoté

Un exemple complet est plus utile qu’une centaine de règles isolées. Le fichier suivant illustre une structure lisible : module déclaré, imports ciblés, types clairs, procédure courte.

```vit
space app/utils
use std/io/print

type Path = string

form User {
  name: string
  uid: i32
}

proc banner(path: Path) -> string {
  give "[" + path + "]"
}
```

Cet exemple est valide et lisible, parce que chaque élément est explicitement déclaré et que les dépendances sont visibles.

## Résumé express

La grammaire de Vitte est conçue pour rendre les intentions explicites. `space` définit le module, `use`/`pull` gèrent la visibilité, `form` et `pick` définissent les données, `proc` et `entry` définissent l’exécution. Les blocs explicites et les mutations visibles (`set`) sont des choix de lisibilité. En respectant ces formes, vous obtenez un code stable, transmissible, et facile à relire.

