# 11. Collections et itération

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/10-diagnostics.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/10-diagnostics.md`, `book/chapters/12-pointeurs.md`, `book/glossaire.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Collections et itération**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Collections et itération**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Collections et itération**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Collections et itération** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Collections et itération**.

## Exercice court

Prenez un exemple du chapitre sur **Collections et itération**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Collections et itération**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 11.1 Reduction simple sur un tableau d'entiers

```vit
proc sum(values: int[]) -> int {
  let acc: int = 0
  for x in values {
    set acc = acc + x
  }

  give acc
}
```

Lecture simple du code:
1. `proc sum(values: int[]) -> int {` : le contrat est défini pour `sum`: entrées `values: int[]` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `let acc: int = 0` : cette ligne crée la variable `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `for x in values {` : cette ligne définit une étape explicite du flux.
4. `set acc = acc + x` : cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux.
5. `}` : cette accolade ferme le bloc logique.
6. `give acc` : la branche renvoie immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable.
7. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: lire la collection en flux linéaire avec un seul état mutable, `acc`.

Ce modèle est simple à vérifier: chaque élément est lu une fois, puis ajouté à l'accumulateur.

Lecture pas à pas pour `[2,3,4]`:
- départ `acc=0`.
- après `2`: `acc=2`.
- après `3`: `acc=5`.
- après `4`: `acc=9`.
- retour final `9`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 11.2 Moyenne entiere avec garde sur collection vide

```vit
proc mean_floor(values: int[]) -> int {
  let total: int = 0
  let count: int = 0
  for x in values {
    set total = total + x
    set count = count + 1
  }

  if count == 0 { give 0 }

  give total / count
}
```

Lecture simple du code:
1. `proc mean_floor(values: int[]) -> int {` : le contrat est posé pour `mean_floor`: entrées `values: int[]` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `let total: int = 0` : cette ligne crée la variable `total` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `let count: int = 0` : cette ligne crée la variable `count` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
4. `for x in values {` : cette ligne définit une étape explicite du flux.
5. `set total = total + x` : cette ligne réalise une mutation volontaire et visible: l'état `total` change ici, à cet endroit précis du flux.
6. `set count = count + 1` : cette ligne réalise une mutation volontaire et visible: l'état `count` change ici, à cet endroit précis du flux.
7. `}` : cette accolade clôt le bloc logique.
8. `if count == 0 { give 0 }` : cette garde traite le cas limite avant le calcul.
9. `give total / count` : la sortie est renvoyée immédiatement `total / count` pour la branche courante, la sortie de branche est explicite et vérifiable.
10. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `count == 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `total / count`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: calculer une moyenne entière sans jamais exécuter une division invalide.

Le point clé est la garde `if count == 0`: elle traite explicitement le cas vide avant la division.

À l'exécution:
- `mean_floor([])` retourne `0`.
- `mean_floor([4,5,6])` calcule `total=15`, `count=3`, puis retourne `5`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 11.3 Filtrage positif et projection finale

```vit
proc positive_only(values: int[]) -> int[] {
  let out: int[] = []
  for x in values {
    if x <= 0 { continue }
    out.push(x)
  }

  give out
}
```

Lecture simple du code:
1. `proc positive_only(values: int[]) -> int[] {` : le contrat est fixé pour `positive_only`: entrées `values: int[]` et sortie `int[]`, elle clarifie l'intention avant lecture détaillée du corps.
2. `let out: int[] = []` : cette ligne crée la variable `out` de type `int[]` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `for x in values {` : cette ligne définit une étape explicite du flux.
4. `if x <= 0 { continue }` : cette garde traite le cas limite avant le calcul.
5. `out.push(x)` : cette ligne définit une étape explicite du flux.
6. `}` : cette accolade ferme le bloc logique.
7. `give out` : retourne immédiatement `out` pour la branche courante, la sortie de branche est explicite et vérifiable.
8. `}` : cette accolade clôt le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `out`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: filtrer tôt pour conserver une boucle lisible.

La règle est locale et explicite: toute valeur `<= 0` est ignorée immédiatement.

À l'exécution, sur `[-1,2,0,7]`:
- `-1` est ignoré.
- `2` est conservé -> `[2]`.
- `0` est ignoré.
- `7` est conservé -> `[2,7]`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

L'itération est linéaire, les gardes de sécurité sont explicites et la mutation est strictement locale. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez suivre l'état de l'accumulateur étape par étape.
- vous savez justifier la présence d'une garde avant une opération risquée.
- vous savez isoler clairement filtrage et transformation finale.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Repère: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/continue.md`.
- `book/keywords/field.md`.
- `book/keywords/for.md`.
- `book/keywords/form.md`.
- `book/keywords/give.md`.



## Exemple Étendu


```vit
// Scenario collections: execution complete et verifiable
space demo/collections

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
