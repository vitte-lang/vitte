# 0. Avant-propos

Niveau: Fondations (débutant exigeant à intermédiaire)

Prérequis: aucun.
Voir aussi: `book/glossaire.md`, `book/chapters/01-demarrer.md`.

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
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give ParseInt.Err(22) }
  // Sortie locale: valeur retournee par la procedure
  give ParseInt.Ok(x)
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(r: ParseInt) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Ok(_) { give 0 }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  // Sortie programme: code de retour observable
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

Exemple approfondi pour **avant propos**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/avant-propos

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

Scénarios recommandés (avant propos):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
