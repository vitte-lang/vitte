# 13. Génériques

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/12-pointeurs.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/12-pointeurs.md`, `book/chapters/14-macros.md`, `book/glossaire.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Génériques**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Génériques**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Génériques**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Génériques** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Génériques**.

## Exercice court

Prenez un exemple du chapitre sur **Génériques**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Génériques**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 13.1 Fonction identité paramétrée par type

```vit
proc id[T](x: T) -> T {
  give x
}
```

Lecture simple du code:
1. `proc id[T](x: T) -> T {` : cette ligne définit une étape explicite du flux.
2. `give x` : la branche renvoie immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable.
3. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: montrer qu'un même corps de fonction peut être spécialisé par type, sans changer le contrat entrée/sortie.

Le comportement est uniforme: la fonction renvoie exactement ce qu'elle reçoit, quel que soit `T`.

À l'exécution:
- `id[int](42)` retourne `42`.
- `id[string]("ok")` retourne `"ok"`.

Note de syntaxe actuelle:
- les appels génériques explicites comme `id[int](42)` sont implémentés
- cette surface reste `experimental`
- une forme comme `id[i](42)` ne devient pas un appel générique: elle reste une indexation suivie d'appel

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 13.2 Homogénéité imposée sur plusieurs arguments

```vit
proc first[T](a: T, b: T) -> T {
  give a
}
```

Lecture simple du code:
1. `proc first[T](a: T, b: T) -> T {` : cette ligne définit une étape explicite du flux.
2. `give a` : la sortie est renvoyée immédiatement `a` pour la branche courante, la sortie de branche est explicite et vérifiable.
3. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `a`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: imposer le même type sur plusieurs arguments pour éviter les mélanges incohérents.

Ici, la sécurité est précoce: l'erreur est signalée à la compilation, avant toute exécution.

À l'exécution:
- `first[int](7,9)` retourne `7`.
- `first(7,"x")` est rejeté au type-check.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 13.3 Structure générique et transformation

```vit
form Pair[T] {
  left: T
  right: T
}
proc swap_pair[T](p: Pair[T]) -> Pair[T] {
  give Pair[T](p.right, p.left)
}
```

Lecture simple du code:
1. `form Pair[T] {` : cette ligne définit une étape explicite du flux.
2. `left: T` : cette ligne déclare le champ `left` avec le type `T`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `right: T` : cette ligne déclare le champ `right` avec le type `T`, ce qui documente son rôle et limite les erreurs de manipulation.
4. `}` : cette accolade clôt le bloc logique.
5. `proc swap_pair[T](p: Pair[T]) -> Pair[T] {` : cette ligne définit une étape explicite du flux.
6. `give Pair[T](p.right, p.left)` : retourne immédiatement `Pair[T](p.right, p.left)` pour la branche courante, la sortie de branche est explicite et vérifiable.
7. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Pair[T](p.right, p.left)`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: montrer que la généricité s'applique autant aux structures qu'aux procédures.

Le point clé est la conservation du type: un `Pair[int]` transformé reste un `Pair[int]`.

À l'exécution, `swap_pair(Pair[int](1,2))` retourne `Pair[int](2,1)`.

Lecture de surface utile:
- `Pair[int](1,2)` utilise aujourd'hui la même famille de syntaxe explicite que `id[int](42)`
- la forme qualifiée `mod.Pair[int](..)` est aussi supportée
- la promotion de cette syntaxe au noyau stable reste volontairement différée

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## À retenir

Le code est mutualisé, les invariants de type restent garantis et les erreurs sont détectées en compilation. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez identifier ce qui dépend de `T` et ce qui n'en dépend pas.
- vous savez détecter un mélange de types avant l'exécution.
- vous savez vérifier qu'une transformation conserve le type attendu.

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
- `book/keywords/form.md`.
- `book/keywords/give.md`.
- `book/keywords/int.md`.



## Exemple Étendu


```vit
// Scenario generiques: execution complete et verifiable
space demo/generiques

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
