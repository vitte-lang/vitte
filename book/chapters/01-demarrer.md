# 1. Démarrer avec Vitte

Niveau: Fondations

Prérequis: `book/chapters/00-avant-propos.md`, `book/glossaire.md`.
Voir aussi: `book/chapters/02-philosophie.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif technique

Établir une base exécutable avec trois propriétés non négociables:
1. point d'entrée explicite
2. calcul isolé dans des procédures
3. sortie observable sans ambiguïté

## 1.1 Point d'entrée et contrat d'exécution

```vit
// Point d'entree du scenario
entry main at core/app {
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
  give a + b
}

// Point d'entree du scenario
entry main at core/app {
  let r: int = add(20, 22)

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


```vit
// Scenario demarrer: execution complete et verifiable
space demo/demarrer

form Input { id: int value: int quota: int }
pick Eval { case Accepted(score: int) case Rejected(code: int) }

proc validate(x: Input) -> Eval {

  if x.id <= 0 { give Eval.Rejected(21) }

  if x.quota < 0 { give Eval.Rejected(22) }

  if x.value < 0 { give Eval.Rejected(23) }

  give Eval.Accepted(x.value)
}

proc transform(score: int, quota: int) -> int {
  let capped: int = score
  if capped > quota { set capped = quota }

  if capped < 0 { give 0 }

  give capped * 2
}

proc decide(r: Eval, quota: int) -> Eval {

  match r {
    case Accepted(s) {
      let out: int = transform(s, quota)

      if out >= 10 { give Eval.Accepted(out) }

      give Eval.Rejected(31)
    }
    case Rejected(c) { give Eval.Rejected(c) }
    otherwise { give Eval.Rejected(70) }
  }
}

// Conversion finale vers un code de sortie
proc to_exit(r: Eval) -> int {

  match r {
    case Accepted(_) { give 0 }
    case Rejected(code) { give code }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let x: Input = Input(1, 8, 9)
  let v: Eval = validate(x)
  let d: Eval = decide(v, x.quota)

  return to_exit(d)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.


## Trade-offs

| Contrainte | Option A | Option B | Décision recommandée |
| --- | --- | --- | --- |
| Lisibilité prioritaire | Branches explicites | Code compact | A si l'équipe maintient le code longtemps |
| Perf critique | Spécialisation ciblée | Généralisation | A si profiling confirme le gain |
| Évolution rapide | Contrats stricts | Conventions implicites | A pour réduire les régressions |


## Décision Selon Contrainte

- Si la contrainte dominante est la sûreté: valider tôt, échouer explicitement.
- Si la contrainte dominante est la latence: mesurer d'abord, optimiser ensuite.
- Si la contrainte dominante est l'évolutivité: isoler orchestration, décisions et conversion de sortie.


## Diagnostic Rapide

| Symptôme | Cause probable | Vérification | Correction |
| --- | --- | --- | --- |
| Sortie inattendue | Garde absente ou mal ordonnée | Rejouer avec cas limite | Remonter la garde avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Contrat implicite | Comparer nominal vs limite | Formaliser le contrat dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas limite réel et prévoir sa sortie,
- identifier où ajouter une garde sans casser le nominal.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
