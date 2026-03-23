# 43. Anti-patterns frequents en code Vitte

Niveau: Intermediaire

Prérequis: `book/chapters/28-conventions.md`, `book/chapters/42-patterns-procedures-robustes.md`.
Voir aussi: à définir.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Identifier rapidement les formes de code qui augmentent dette technique, bugs et cout de maintenance.

## Anti-patterns frequents

1. Procedures trop longues avec responsabilites mixtes.
2. Validation absente des cas limites.
3. Types implicites la ou une signature explicite est necessaire.
4. Imports excessifs au lieu d'interfaces stables.
5. Messages d'erreur non actionnables.

## Correctif minimal

1. Extraire une responsabilite par procedure.
2. Ajouter gardes en tete.
3. Rendre les signatures explicites.
4. Reduire la surface importee.

## Exemples progressifs (N1 -> N3)

### N1 (base): anti-pattern detecte

```vit
proc process(x: int, y: int, z: int) -> int {

  if x < 0 { give -1 }

  if y < 0 { give -1 }

  if z < 0 { give -1 }

  give x + y + z
}
```

### N2 (intermediaire): extraction

```vit
proc is_valid(x: int) -> bool { give x >= 0 }
proc process3(x: int, y: int, z: int) -> int {

  if not is_valid(x) { give -1 }

  if not is_valid(y) { give -1 }

  if not is_valid(z) { give -1 }

  give x + y + z
}
```

### N3 (avance): responsabilites separees

```vit
proc validate3(x: int, y: int, z: int) -> bool { give x>=0 and y>=0 and z>=0 }
proc sum3(x: int, y: int, z: int) -> int { give x+y+z }
```

### Anti-exemple

```vit
proc mega(a:int,b:int,c:int,d:int,e:int) -> int { give a+b+c+d+e }
```

## Validation rapide

1. Detecter fonction trop large.
2. Extraire responsabilites.
3. Ajouter test de non-regression.

## Pourquoi

Cette section explicite la valeur pratique: réduire les erreurs, accélérer le diagnostic et stabiliser les évolutions.

## Test mental

Question de contrôle: si vous modifiez une hypothèse clé, quel résultat doit changer et pourquoi?

## À faire

1. Exécuter l’exemple nominal.
2. Introduire un cas limite.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte le contrat, puis ajouter un test de non-régression.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **anti-patterns frequents en code vitte**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc process(x: int, y: int, z: int) -> int {

  if x < 0 { give -1 }

  if y < 0 { give -1 }

  if z < 0 { give -1 }

  give x + y + z
}
```

Lecture ligne par ligne:
1. `proc process(x: int, y: int, z: int) -> int {` -> pose un contrat clair de fonction.
2. `if x < 0 { give -1 }` -> sépare nominal et cas limite.
3. `if y < 0 { give -1 }` -> sépare nominal et cas limite.
4. `if z < 0 { give -1 }` -> sépare nominal et cas limite.
5. `give x + y + z` -> renvoie la sortie vérifiable.
6. `}` -> participe au déroulé du traitement.

### Exemple B: variante cas limite (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant une garde explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter une garde avant cette ligne.
3. Vérifier la nouvelle sortie sur une entrée limite.

### Exemple C: bug reproductible puis correction locale

Procédure:
1. Introduire une incompatibilité de type sur un appel.
2. Compiler et lire le premier diagnostic.
3. Corriger une seule ligne (pas de refactor global).
4. Recompiler et vérifier le retour nominal.

### Résultat attendu

- Le lecteur comprend ce que fait le code sans abstraction inutile.
- Chaque exemple est relié à une action concrète.
- La correction est reproductible et testable.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->



## Exemple Étendu


```vit
// Scenario anti patterns code vitte: execution complete et verifiable
space demo/anti-patterns-code-vitte

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
