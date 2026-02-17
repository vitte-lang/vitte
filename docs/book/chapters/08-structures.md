# 8. Structures de données

Niveau: Débutant

Prérequis: chapitre précédent `docs/book/chapters/07-controle.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/07-controle.md`, `docs/book/chapters/09-modules.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: placer la complexité dans la modélisation plutôt que dans les conditions. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 8.1 Structure ticket

```vit
form Ticket {
  id: int
  priority: int
  assignee: string
}
```

Lecture ligne par ligne (débutant):
1. `form Ticket {` cette ligne ouvre la structure `Ticket` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `Ticket` sans redéfinir ses champs.
2. `id: int` cette ligne déclare le champ `id` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
3. `priority: int` cette ligne déclare le champ `priority` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
4. `assignee: string` cette ligne déclare le champ `assignee` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `string`.
5. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: décrire une entité métier explicite avec des champs nommés, plutôt que manipuler des valeurs anonymes.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qu'est un ticket et quelles informations il doit porter.

À l'exécution, la vérification de structure se fait dès la compilation:
- un `Ticket` doit toujours avoir `id`, `priority` et `assignee`.
- un champ manquant ou de mauvais type est rejeté avant exécution.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
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

Lecture ligne par ligne (débutant):
1. `pick TicketState {` cette ligne ouvre le type fermé `TicketState` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
2. `case Open` cette ligne décrit le cas `Open` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Open`, ce bloc devient le chemin actif.
3. `case Assigned(user: string)` cette ligne décrit le cas `Assigned(user: string)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Assigned(user: string)`, ce bloc devient le chemin actif.
4. `case Closed(code: int)` cette ligne décrit le cas `Closed(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Closed(code: int)`, ce bloc devient le chemin actif.
5. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Open` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: modéliser un cycle de vie par états exclusifs, avec une charge utile seulement quand c'est utile.

Ce modèle empêche les combinaisons incohérentes: un ticket ne peut pas être "Open et Closed" en même temps.

À l'exécution, toute valeur est exactement une variante:
- `Open`.
- `Assigned(user)`.
- `Closed(code)`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 8.3 Composition de règles

```vit
proc is_critical(t: Ticket) -> bool {
  give t.priority >= 9
}
proc route(t: Ticket, s: TicketState) -> int {
  if is_critical(t) and not (match s { case Closed(_) { give true } otherwise { give false } }) {
    give 1
  }
give 0
}
```

Lecture ligne par ligne (débutant):
1. `proc is_critical(t: Ticket) -> bool {` ici, le contrat complet est défini pour `is_critical`: entrées `t: Ticket` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `is_critical` retourne toujours une valeur compatible avec `bool`.
2. `give t.priority >= 9` ici, la branche renvoie immédiatement `t.priority >= 9` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `t.priority >= 9`.
3. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
4. `proc route(t: Ticket, s: TicketState) -> int {` sur cette ligne, le contrat complet est posé pour `route`: entrées `t: Ticket, s: TicketState` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `route` retourne toujours une valeur compatible avec `int`.
5. `if is_critical(t) and not (match s { case Closed(_) { give true } otherwise { give false } }) {` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
6. `give 1` sur cette ligne, la sortie est renvoyée immédiatement `1` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `1`.
7. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
8. `give 0` ce passage retourne immédiatement `0` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `0`.
9. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `t.priority >= 9`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: composer une règle métier à partir de deux axes explicites, la structure (`Ticket`) et l'état (`TicketState`).

Logique de la fonction `route`:
- un ticket est critique si `priority >= 9`.
- un ticket fermé ne doit pas être routé comme critique.
- le code retourne `1` seulement si les deux conditions sont réunies.

À l'exécution:
- priorité haute + état non fermé -> `1`.
- ticket fermé, même prioritaire -> `0`.
- priorité basse -> `0`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
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
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
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

- `docs/book/keywords/and.md`.
- `docs/book/keywords/bool.md`.
- `docs/book/keywords/break.md`.
- `docs/book/keywords/case.md`.
- `docs/book/keywords/continue.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

