# 8. Structures de données

Niveau: Débutant

Prérequis: chapitre précédent `book/chapters/07-controle.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/07-controle.md`, `book/chapters/09-modules.md`, `book/glossaire.md`.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Structures de données**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Structures de données**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Structures de données**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Structures de données** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Structures de données**.

## Exercice court

Prenez un exemple du chapitre sur **Structures de données**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Structures de données**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 8.1 Structure ticket

```vit
form Ticket {
  id: int
  priority: int
  assignee: string
}
```

Lecture simple du code:
1. `form Ticket {` : cette ligne ouvre la structure `Ticket` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
2. `id: int` : cette ligne déclare le champ `id` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `priority: int` : cette ligne déclare le champ `priority` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
4. `assignee: string` : cette ligne déclare le champ `assignee` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation.
5. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: décrire une entité métier explicite avec des champs nommés, plutôt que manipuler des valeurs anonymes.

En pratique, ce choix simplifie la lecture: on voit immédiatement ce qu'est un ticket et quelles informations il doit porter.

À l'exécution, la vérification de structure se fait dès la compilation:
- un `Ticket` doit toujours avoir `id`, `priority` et `assignee`.
- un champ manquant ou de mauvais type est rejeté avant exécution.

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 8.2 État de cycle de vie

```vit
pick TicketState {
  case Open
  case Assigned(user: string)
  case Closed(code: int)
}
```

Lecture simple du code:
1. `pick TicketState {` : cette ligne ouvre le type fermé `TicketState` pour forcer un ensemble fini de cas possibles et supprimer les états implicites.
2. `case Open` : ce cas décrit `Open` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
3. `case Assigned(user: string)` : ce cas décrit `Assigned(user: string)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
4. `case Closed(code: int)` : ce cas décrit `Closed(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
5. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Open` permet de confirmer la branche attendue.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: modéliser un cycle de vie par états exclusifs, avec une charge utile seulement quand c'est utile.

Ce modèle empêche les combinaisons incohérentes: un ticket ne peut pas être "Open et Closed" en même temps.

À l'exécution, toute valeur est exactement une variante:
- `Open`.
- `Assigned(user)`.
- `Closed(code)`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 8.3 Composition de règles

```vit
proc is_critical(t: Ticket) -> bool {
  // Sortie locale: valeur retournee par la procedure
  give t.priority >= 9
}
proc route(t: Ticket, s: TicketState) -> int {
  // Garde: bloque un cas invalide avant de continuer
  if is_critical(t) and not (match s { case Closed(_) { give true } otherwise { give false } }) {
    // Sortie locale: valeur retournee par la procedure
  give 1
  }
// Sortie locale: valeur retournee par la procedure
  give 0
}
```

Lecture simple du code:
1. `proc is_critical(t: Ticket) -> bool {` : le contrat est défini pour `is_critical`: entrées `t: Ticket` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps.
2. `give t.priority >= 9` : la branche renvoie immédiatement `t.priority >= 9` pour la branche courante, la sortie de branche est explicite et vérifiable.
3. `}` : cette accolade clôt le bloc logique.
4. `proc route(t: Ticket, s: TicketState) -> int {` : le contrat est posé pour `route`: entrées `t: Ticket, s: TicketState` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
5. `if is_critical(t) and not (match s { case Closed(_) { give true } otherwise { give false } }) {` : cette ligne définit une étape explicite du flux.
6. `give 1` : la sortie est renvoyée immédiatement `1` pour la branche courante, la sortie de branche est explicite et vérifiable.
7. `}` : cette accolade ferme le bloc logique.
8. `give 0` : retourne immédiatement `0` pour la branche courante, la sortie de branche est explicite et vérifiable.
9. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `t.priority >= 9`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: composer une règle métier à partir de deux axes explicites, la structure (`Ticket`) et l'état (`TicketState`).

Logique de la fonction `route`:
- un ticket est critique si `priority >= 9`.
- un ticket fermé ne doit pas être routé comme critique.
- le code retourne `1` seulement si les deux conditions sont réunies.

À l'exécution:
- priorité haute + état non fermé -> `1`.
- ticket fermé, même prioritaire -> `0`.
- priorité basse -> `0`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Donnée et état séparés, règles courtes, prédicats réutilisables. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez distinguer clairement structure, état et règle métier.
- vous savez ajouter une nouvelle règle sans casser les anciennes.
- vous savez relire la logique sans deviner ce que "veut dire" une donnée.

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

- `book/keywords/and.md`.
- `book/keywords/bool.md`.
- `book/keywords/break.md`.
- `book/keywords/case.md`.
- `book/keywords/continue.md`.



## Exemple Étendu

Exemple approfondi pour **structures**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/structures

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

Scénarios recommandés (structures):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
