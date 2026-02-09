# 6. Fonctions et procédures

Une fonction est une promesse : « si vous me donnez ces entrées, je vous rends cette sortie ». Dans Vitte, cette promesse se veut simple, lisible, et stable. Une bonne fonction donne envie de la réutiliser.

## Définir une procédure

```vit
proc add(x: int, y: int) -> int {
  return x + y
}
```

Le nom est court et précis. Les paramètres sont explicitement typés. La signature est un point de repère important.

## Procédures locales

Vitte autorise les procédures comme valeurs, ce qui aide à structurer un code de manière locale :

```vit
entry main at core/app {
  let add = proc(x: int, y: int) -> int { return x + y }
  return add(1, 2)
}
```

## Effets et lisibilité

Une procédure qui touche l’extérieur (fichier, réseau, horloge) devrait le dire clairement. Le but est de rendre l’effet visible au lecteur. L’ambiguïté est une source de bugs.

## L’interface comme contrat

Une signature simple permet de changer l’implémentation sans toucher aux appelants. Plus votre interface est claire, plus votre code est durable.

## Petite taille, grande clarté

Une procédure courte favorise la compréhension locale. Si une fonction devient longue, posez‑vous la question : est‑ce un seul problème ou plusieurs ?

## Découper au bon endroit

Le bon découpage n’est pas celui qui minimise les lignes, mais celui qui minimise l’effort de compréhension. Un lecteur doit pouvoir comprendre une procédure sans sauter ailleurs toutes les trois lignes.

## À retenir

Les signatures sont des contrats. Une procédure courte vaut mieux qu’une procédure polyvalente. Les effets doivent être visibles.


## Exemple guidé : clarifier une interface

Partir d’une procédure “fourre‑tout”, puis découper en trois fonctions claires. Chaque fonction doit tenir en moins de 15 lignes.

## Erreurs courantes

Utiliser des paramètres “optionnels” sans le dire. Faire une fonction qui “fait un peu de tout”. Cacher un effet derrière une signature trop neutre.

## Checklist procédures

Le nom décrit l’intention. La signature est courte. Les effets sont visibles.


## Exercice : réduire la signature

Prenez une procédure avec six paramètres. Essayez de regrouper ceux qui vont ensemble dans un type dédié. La signature devient plus lisible, et les erreurs d’appel sont moins probables.


## Code complet (API actuelle)

```vit
proc parse_port(s: string) -> i32 {
  let i: i32 = 0
  let n: i32 = 0
  if s.len == 0 { give -1 }
  loop {
    if i >= s.len as i32 { break }
    let ch = s.slice(i as usize, (i + 1) as usize)
    if ch < "0" || ch > "9" { give -1 }
    n = n * 10 + (ch.as_bytes()[0] as i32 - 48)
    i = i + 1
  }
  give n
}
```

## API idéale (future)

Un module `std/parse` avec `parse_i32` et `parse_usize` standard.

