# 0. Avant-propos

Niveau: Fondations (débutant exigeant à intermédiaire)

Prérequis: aucun.
Voir aussi: `book/glossaire.md`, `book/chapters/01-demarrer.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Positionnement

Ce livre traite Vitte comme un outil de construction logicielle, pas comme une grammaire à mémoriser.
La ligne directrice est simple: **un code lisible est un code dont les contrats, les invariants et les frontières de responsabilité sont explicites**.

## Ce que ce livre optimise

1. **Prédictibilité**: une entrée donnée produit un comportement explicable.
2. **Auditabilité**: on peut justifier une décision sans interprétation implicite.
3. **Évolutivité**: le code reste modifiable sans casser les hypothèses de base.

Ce livre n'optimise pas l'effet “démo rapide”. Il optimise la tenue du code dans le temps.

## Méthode éditoriale utilisée dans tous les chapitres

Chaque section suit le même protocole:
1. contexte technique et contrainte
2. code minimal suffisant
3. exécution déterministe (ordre réel des étapes)
4. preuve: cas nominal + cas limite
5. contre-exemple (anti-pattern) et correction

Quand une section ne passe pas ce protocole, elle est incomplète.

## Modèle mental de base

- `entry` définit une frontière d'exécution.
- `proc` porte la logique métier testable.
- `form` et `pick` portent le vocabulaire métier.
- `give` et `return` rendent les sorties explicites.

En pratique: on déplace la complexité dans des unités testables, puis on garde l'orchestration mince.

## Exemple de calibration

```vit
pick ParseInt {
  case Ok(value: int)
  case Err(code: int)
}

proc non_negative(x: int) -> ParseInt {

  if x < 0 { give ParseInt.Err(22) }

  give ParseInt.Ok(x)
}

// Conversion finale vers un code de sortie
proc to_exit(r: ParseInt) -> int {

  match r {
    case Ok(_) { give 0 }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  return to_exit(non_negative(-3))
}
```

Lecture d'ingénierie:
1. la politique d'erreur est typée (`ParseInt`), pas implicite
2. la validation est locale (`non_negative`)
3. la projection technique est centralisée (`to_exit`)
4. `entry` ne contient pas de règle métier

Propriété obtenue:
- on peut modifier le transport (CLI, service, batch) sans réécrire la logique métier.

## Règles de lecture recommandées

Avant de valider un bloc, vérifier:
1. quel est le contrat d'entrée/sortie ?
2. quel invariant doit rester vrai ?
3. où est le traitement des cas invalides ?
4. la frontière métier/technique est-elle nette ?

## Keywords à revoir

- `book/keywords/entry.md`
- `book/keywords/proc.md`
- `book/keywords/form.md`
- `book/keywords/pick.md`
- `book/keywords/give.md`
- `book/keywords/return.md`



## Exemple Étendu


```vit
// Scenario avant propos: execution complete et verifiable
space demo/avant-propos

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


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: gardes d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
