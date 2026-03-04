# 29. Style d'architecture

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/28-conventions.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/28-conventions.md`, `book/chapters/30-faq.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Style d'architecture**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Style d'architecture**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Style d'architecture**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Style d'architecture** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Style d'architecture**.

## Exercice court

Prenez un exemple du chapitre sur **Style d'architecture**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Style d'architecture**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 29.1 Écrire un domaine pur sans dépendance d'infrastructure

```vit
space app/domain
form Order {
  amount: int
}
proc approve(o: Order) -> bool {
  give o.amount > 0
}
```

Lecture ligne par ligne (débutant):
1. `space app/domain` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `form Order {` -> Comportement: cette ligne ouvre la structure `Order` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. -> Preuve: plusieurs fonctions peuvent manipuler `Order` sans redéfinir ses champs.
3. `amount: int` -> Comportement: cette ligne déclare le champ `amount` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `int`.
4. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc approve(o: Order) -> bool {` -> Comportement: le contrat est défini pour `approve`: entrées `o: Order` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `approve` retourne toujours une valeur compatible avec `bool`.
6. `give o.amount > 0` -> Comportement: la branche renvoie immédiatement `o.amount > 0` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `o.amount > 0`.
7. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `o.amount > 0`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: isoler un domaine pur, testable sans dépendance d'infrastructure.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `approve(Order(10))=true` et `approve(Order(0))=false`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 29.2 Traduire le métier dans une couche service

```vit
space app/service
pull app/domain as d
proc handle(amount: int) -> int {
  let ok: bool = d.approve(d.Order(amount))
  if ok { give 200 }
  give 422
}
```

Lecture ligne par ligne (débutant):
1. `space app/service` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `pull app/domain as d` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
3. `proc handle(amount: int) -> int {` -> Comportement: le contrat est posé pour `handle`: entrées `amount: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `handle` retourne toujours une valeur compatible avec `int`.
4. `let ok: bool = d.approve(d.Order(amount))` -> Comportement: cette ligne crée la variable `ok` de type `bool` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `ok` reçoit ici le résultat de `d.approve(d.Order(amount))` et peut être réutilisé ensuite sans recalcul.
5. `if ok { give 200 }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `ok` est vrai, `give 200` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `give 422` -> Comportement: la sortie est renvoyée immédiatement `422` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `422`.
7. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `ok` est vrai, la sortie devient `200`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `422`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: faire jouer au service un rôle de traduction entre métier et sortie applicative.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `handle(10)=200` et `handle(0)=422`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 29.3 Garder un point d'entrée mince dans la couche IO

```vit
space app/io
pull app/service as s
entry main at core/app {
  let code: int = s.handle(10)
  return code
}
```

Lecture ligne par ligne (débutant):
1. `space app/io` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `pull app/service as s` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
3. `entry main at core/app {` -> Comportement: cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre. -> Preuve: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
4. `let code: int = s.handle(10)` -> Comportement: cette ligne crée la variable `code` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `code` reçoit ici le résultat de `s.handle(10)` et peut être réutilisé ensuite sans recalcul.
5. `return code` -> Comportement: cette ligne termine l'exécution du bloc courant avec le code `code`, utile pour observer le résultat global du scénario. -> Preuve: un test d'exécution peut vérifier directement que le programme retourne `code`.
6. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return code`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `code`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: garder un point d'entrée mince, limité à l'orchestration.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, le scénario montre un retour final `200`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Domaine pur, service de traduction et entry minimal forment une architecture stable à long terme. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez dire dans quelle couche doit vivre une règle métier.
- vous pouvez modifier l'IO sans toucher au domaine.
- vous pouvez testér la logique principale sans démarrer l'application complète.

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

- `docs/book/keywords/as.md`.
- `docs/book/keywords/at.md`.
- `docs/book/keywords/bool.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/entry.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.
