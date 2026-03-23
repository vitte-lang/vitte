# 1. Démarrer avec Vitte

Niveau: Fondations

Prérequis: `book/chapters/00-avant-propos.md`, `book/glossaire.md`.
Voir aussi: `book/chapters/02-philosophie.md`.

## Objectif technique

Établir une base exécutable avec trois propriétés non négociables:
1. point d'entrée explicite
2. calcul isolé dans des procédures
3. sortie observable sans ambiguïté

## 1.1 Point d'entrée et contrat d'exécution

```vit
// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  // Sortie programme: code de retour observable
  return 0
}
```

Ce que ce bloc fixe:
- frontière d'exécution: `main` dans `core/app`
- sémantique de terminaison: sortie immédiate via `return`
- comportement déterministe: aucune dépendance cachée

Si ce bloc est instable, tout le reste l'est aussi.

## 1.2 Extraction du calcul hors de l'orchestration

```vit
proc add(a: int, b: int) -> int {
  // Sortie locale: valeur retournee par la procedure
  give a + b
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let r: int = add(20, 22)
  // Sortie programme: code de retour observable
  return r
}
```

Analyse:
1. `add` porte un contrat pur (`int, int -> int`).
2. `entry` orchestre l'appel et restitue le résultat.
3. la responsabilité est séparée: calcul vs exécution.

Invariant utile:
- pour une même paire `(a,b)`, `add` doit produire la même sortie.

Conséquence de design:
- tests unitaires sur `add` sans dépendre de l'environnement d'entrée.

## 1.3 Boucle bornée: preuve d'arrêt et invariant d'état

```vit
proc sum_to(n: int) -> int {
  let i: int = 0
  let s: int = 0

  // Boucle: progression controlee jusqu'a la borne

  loop {
    // Borne d'arret: stoppe la boucle de maniere explicite
    if i > n { break }
    set s = s + i
    set i = i + 1
  }

  // Sortie locale: valeur retournee par la procedure
  give s
}
```

Invariants:
- avant le test `i > n`, `s` contient la somme `[0..i-1]`
- `i` progresse strictement de `+1`

Preuve d'arrêt:
- la variable de progression `i` est monotone croissante
- la borne `n` est fixe
- donc la condition `i > n` devient vraie en temps fini (pour `n` fini)

Trace compacte pour `n=3`:
- `(i,s)=(0,0)->(1,0)->(2,1)->(3,3)->(4,6)` puis arrêt, sortie `6`

## 1.4 Erreurs structurantes à éviter

1. surcharger `entry` avec de la logique métier
2. introduire une boucle sans borne d'arrêt explicite
3. mélanger calcul et conversion de sortie dans la même fonction

## 1.5 Check-list de fin de chapitre

1. le point d'entrée est-il unique et explicite ?
2. chaque `proc` a-t-elle un contrat lisible ?
3. chaque boucle a-t-elle borne + progression + invariant ?
4. la sortie finale est-elle directement traçable ?

## Keywords à revoir

- `book/keywords/entry.md`
- `book/keywords/proc.md`
- `book/keywords/loop.md`
- `book/keywords/give.md`
- `book/keywords/return.md`



## Exemple Étendu

Exemple approfondi pour **demarrer**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/demarrer

form Input { id: int value: int quota: int }
pick Eval { case Accepted(score: int) case Rejected(code: int) }

proc validate(x: Input) -> Eval {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x.id <= 0 { give Eval.Rejected(21) }
  // Garde: bloque un cas invalide avant de continuer
  if x.quota < 0 { give Eval.Rejected(22) }
  // Garde: bloque un cas invalide avant de continuer
  if x.value < 0 { give Eval.Rejected(23) }
  // Sortie locale: valeur retournee par la procedure
  give Eval.Accepted(x.value)
}

proc transform(score: int, quota: int) -> int {
  let capped: int = score
  if capped > quota { set capped = quota }
  // Garde: bloque un cas invalide avant de continuer
  if capped < 0 { give 0 }
  // Sortie locale: valeur retournee par la procedure
  give capped * 2
}

proc decide(r: Eval, quota: int) -> Eval {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Accepted(s) {
      let out: int = transform(s, quota)
      // Garde: bloque un cas invalide avant de continuer
  if out >= 10 { give Eval.Accepted(out) }
      // Sortie locale: valeur retournee par la procedure
  give Eval.Rejected(31)
    }
    case Rejected(c) { give Eval.Rejected(c) }
    otherwise { give Eval.Rejected(70) }
  }
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(r: Eval) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Accepted(_) { give 0 }
    case Rejected(code) { give code }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let x: Input = Input(1, 8, 9)
  let v: Eval = validate(x)
  let d: Eval = decide(v, x.quota)
  // Sortie programme: code de retour observable
  return to_exit(d)
}
```

Scénarios recommandés (demarrer):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
