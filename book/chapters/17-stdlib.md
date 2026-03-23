# 17. Packages

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/16-interop.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/16-interop.md`, `book/chapters/18-tests.md`, `book/glossaire.md`.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Packages**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Packages**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Packages**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Packages** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Packages**.

## Exercice court

Prenez un exemple du chapitre sur **Packages**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Packages**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 17.1 Import minimal et procédure pure

```vit
use std/core/types.int
proc inc(x: int) -> int {
  // Sortie locale: valeur retournee par la procedure
  give x + 1
}
```

Lecture simple du code:
1. `use std/core/types.int` : cette ligne définit une étape explicite du flux.
2. `proc inc(x: int) -> int {` : le contrat est défini pour `inc`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
3. `give x + 1` : la branche renvoie immédiatement `x + 1` pour la branche courante, la sortie de branche est explicite et vérifiable.
4. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x + 1`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: montrer qu'un import minimal suffit quand la logique reste simple et pure.

Le message est important: les packages sont un support technique, pas un prétexte pour diluer la logique métier.

À l'exécution, `inc(41)=42`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 17.2 Récurrence déterministe sur bornes connues

```vit
use std/core/types.int
proc checksum(seed: int, n: int) -> int {
  let i: int = 0
  let acc: int = seed
  // Boucle: progression controlee jusqu'a la borne
  loop {
    // Borne d'arret: stoppe la boucle de maniere explicite
    if i >= n { break }
    set acc = ((acc * 33) + i + 17) % 1000003
    set i = i + 1
  }
// Sortie locale: valeur retournee par la procedure
  give acc
}
```

Lecture simple du code:
1. `use std/core/types.int` : cette ligne définit une étape explicite du flux.
2. `proc checksum(seed: int, n: int) -> int {` : le contrat est posé pour `checksum`: entrées `seed: int, n: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
3. `let i: int = 0` : cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
4. `let acc: int = seed` : cette ligne crée la variable `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
5. `loop {` : cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`).
6. `if i >= n { break }` : cette garde traite le cas limite avant le calcul.
7. `set acc = ((acc * 33) + i + 17) % 1000003` : cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux.
8. `set i = i + 1` : cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux.
9. `}` : cette accolade ferme le bloc logique.
10. `give acc` : la sortie est renvoyée immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable.
11. `}` : cette accolade clôt le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: écrire une récurrence déterministe dont la terminaison est bornée.

Lecture recommandée:
- identifier l'état initial (`acc = seed`).
- vérifier la borne (`i >= n`).
- suivre la formule de transition.

À l'exécution, `(seed=7,n=3)` produit `248`, puis `8202`, puis `270685`, et retourne `270685`.

Erreurs classiques à éviter:
- laisser une boucle sans borne claire ou sans condition d'arrêt vérifiable.
- faire évoluer plusieurs variables d'état sans documenter leur rôle.
- optimiser trop tôt sans verrouiller d'abord le comportement attendu.

## 17.3 Normalisation métier indépendante du socle

```vit
form Reading {
  value: int
}
proc normalize_reading(r: Reading) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if r.value < 0 { give 0 }
  // Garde: bloque un cas invalide avant de continuer
  if r.value > 100 { give 100 }
  // Sortie locale: valeur retournee par la procedure
  give r.value
}
```

Lecture simple du code:
1. `form Reading {` : cette ligne ouvre la structure `Reading` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
2. `value: int` : cette ligne déclare le champ `value` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `}` : cette accolade ferme le bloc logique.
4. `proc normalize_reading(r: Reading) -> int {` : le contrat est fixé pour `normalize_reading`: entrées `r: Reading` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
5. `if r.value < 0 { give 0 }` : cette garde traite le cas limite avant le calcul.
6. `if r.value > 100 { give 100 }` : cette garde traite le cas limite avant le calcul.
7. `give r.value` : retourne immédiatement `r.value` pour la branche courante, la sortie de branche est explicite et vérifiable.
8. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `r.value < 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `r.value`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: garder la logique métier indépendante des détails du socle technique.

Ici, la règle métier est lisible en trois gardes, sans dépendre d'une API externe.

À l'exécution:
- `Reading(-4)` donne `0`.
- `Reading(50)` donne `50`.
- `Reading(140)` donne `100`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Les imports sont précis, les algorithmes sont bornés et la logique de domaine reste isolée. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez distinguer ce qui relève du socle et ce qui relève du métier.
- vous savez prouver la terminaison d'une boucle avec sa borne.
- vous pouvez modifier une règle métier sans toucher aux imports techniques.

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

- `book/keywords/break.md`.
- `book/keywords/const.md`.
- `book/keywords/continue.md`.
- `book/keywords/form.md`.
- `book/keywords/give.md`.



## Exemple Étendu

Exemple approfondi pour **stdlib**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/stdlib

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

Scénarios recommandés (stdlib):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
