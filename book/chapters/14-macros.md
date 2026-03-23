# 14. Macros

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/13-generiques.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/13-generiques.md`, `book/chapters/15-pipeline.md`, `book/glossaire.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Macros**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Macros**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Macros**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Macros** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Macros**.

## Exercice court

Prenez un exemple du chapitre sur **Macros**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Macros**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 14.1 Commencer par une macro volontairement neutre

```vit
macro nop() {
  let _: int = 0
}
```

Lecture simple du code:
1. `macro nop() {` : cette ligne définit une étape explicite du flux.
2. `let _: int = 0` : cette ligne crée la variable `_` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: commencer par une macro neutre pour comprendre le mécanisme d'expansion sans bruit métier.

Cette progression évite un piège fréquent: écrire des macros puissantes trop tôt, avant d'avoir stabilisé les conventions de lecture de l'équipe.

À l'exécution, l'appel `nop()` n'ajoute aucune logique fonctionnelle. Le flux observable du programme reste identique, ce qui permet d'isoler le comportement de la macro elle-même.

Erreurs classiques à éviter:
- introduire une macro sans pouvoir expliquer son expansion.
- utiliser une macro là où une procédure claire suffit.
- multiplier les macros "magiques" qui cachent le flux réel.

## 14.2 Injecter une garde réutilisable dans une procédure sensible

```vit
macro guard_nonzero(x) {
  if x == 0 { return -1 }
}
proc safe_div(num: int, den: int) -> int {
  guard_nonzero(den)

  give num / den
}
```

Lecture simple du code:
1. `macro guard_nonzero(x) {` : cette ligne définit une étape explicite du flux.
2. `if x == 0 { return -1 }` : cette garde traite le cas limite avant le calcul.
3. `}` : cette accolade ferme le bloc logique.
4. `proc safe_div(num: int, den: int) -> int {` : le contrat est défini pour `safe_div`: entrées `num: int, den: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
5. `guard_nonzero(den)` : cette ligne définit une étape explicite du flux.
6. `give num / den` : la branche renvoie immédiatement `num / den` pour la branche courante, la sortie de branche est explicite et vérifiable.
7. `}` : cette accolade clôt le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `num / den`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: utiliser une macro pour factoriser une garde critique répétée.

La valeur de `guard_nonzero` n'est pas seulement de gagner quelques lignes. Elle impose une politique uniforme sur un risque concret: la division par zéro.

À l'exécution:
- `safe_div(10,2)` passe la garde et retourne `5`.
- `safe_div(10,0)` est arrêté par la garde et retourne `-1`.

La frontière d'erreur est placée exactement là où le risque apparaît.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 14.3 Encapsuler une normalisation locale sans dissoudre la responsabilité

```vit
macro clamp01(v) {
  if v < 0 { set v = 0 }
  if v > 1 { set v = 1 }
}
proc normalize01(x: int) -> int {
  let v: int = x
  clamp01(v)

  give v
}
```

Lecture simple du code:
1. `macro clamp01(v) {` : cette ligne définit une étape explicite du flux.
2. `if v < 0 { set v = 0 }` : cette garde traite le cas limite avant le calcul.
3. `if v > 1 { set v = 1 }` : cette garde traite le cas limite avant le calcul.
4. `}` : cette accolade ferme le bloc logique.
5. `proc normalize01(x: int) -> int {` : le contrat est posé pour `normalize01`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
6. `let v: int = x` : cette ligne crée la variable `v` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
7. `clamp01(v)` : cette ligne définit une étape explicite du flux.
8. `give v` : la sortie est renvoyée immédiatement `v` pour la branche courante, la sortie de branche est explicite et vérifiable.
9. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `v`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: encapsuler une normalisation locale sans masquer la responsabilité métier.

La règle d'or est simple: une macro est acceptable tant qu'un lecteur peut reconstruire son expansion sans effort disproportionné.

À l'exécution:
- `normalize01(-3)` retourne `0`.
- `normalize01(0)` retourne `0`.
- `normalize01(5)` retourne `1`.

La sortie est bornée dans `[0,1]`, et cette garantie reste visible dans la logique source.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Une macro Vitte utile n'est ni mystique ni décorative. Elle rend une règle récurrente plus stable, plus auditable et plus facile à faire évoluer. Si elle masque l'intention, supprimez-la. Si elle clarifie un contrat répété, gardez-la et documentez-la comme une pièce d'architecture. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous pouvez expliquer l'expansion d'une macro en quelques phrases.
- vous savez dire pourquoi la macro existe au lieu d'une procédure.
- vous pouvez modifier la règle macro sans casser les appels existants.

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

- `book/keywords/as.md`.
- `book/keywords/asm.md`.
- `book/keywords/continue.md`.
- `book/keywords/give.md`.
- `book/keywords/if.md`.



## Exemple Étendu


```vit
// Scenario macros: execution complete et verifiable
space demo/macros

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
