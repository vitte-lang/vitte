# 3. Structure d'un projet

Niveau: Débutant

Prérequis: chapitre précédent `docs/book/chapters/02-philosophie.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/02-philosophie.md`, `docs/book/chapters/04-syntaxe.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Structure d'un projet**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Structure d'un projet**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Structure d'un projet**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Structure d'un projet** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Structure d'un projet**.

## Exercice court

Prenez un exemple du chapitre sur **Structure d'un projet**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Structure d'un projet**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 3.1 Domaine pur

```vit
space app/domain
form Ticket {
  id: int
  priority: int
}
```

Lecture ligne par ligne (débutant):
1. `space app/domain` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `form Ticket {` -> Comportement: cette ligne ouvre la structure `Ticket` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. -> Preuve: plusieurs fonctions peuvent manipuler `Ticket` sans redéfinir ses champs.
3. `id: int` -> Comportement: cette ligne déclare le champ `id` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `int`.
4. `priority: int` -> Comportement: cette ligne déclare le champ `priority` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `int`.
5. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Le domaine ne depend ni d'I/O ni de transport.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, verification compile-time de la forme `Ticket`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 3.2 Service qui consomme le domaine

```vit
space app/service
pull app/domain as d
proc is_critical(t: d.Ticket) -> bool {
  give t.priority >= 9
}
```

Lecture ligne par ligne (débutant):
1. `space app/service` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `pull app/domain as d` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
3. `proc is_critical(t: d.Ticket) -> bool {` -> Comportement: le contrat est défini pour `is_critical`: entrées `t: d.Ticket` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `is_critical` retourne toujours une valeur compatible avec `bool`.
4. `give t.priority >= 9` -> Comportement: la branche renvoie immédiatement `t.priority >= 9` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `t.priority >= 9`.
5. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `t.priority >= 9`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Dependance orientée domaine -> service, pas l'inverse.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `priority=10` donne `true`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 3.3 Entrée qui orchestre

```vit
space app/io
pull app/service as s
entry main at core/app {
  let code: int = s.is_critical(app/domain.Ticket(1, 9)) as int
  return code
}
```

Lecture ligne par ligne (débutant):
1. `space app/io` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `pull app/service as s` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
3. `entry main at core/app {` -> Comportement: cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre. -> Preuve: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
4. `let code: int = s.is_critical(app/domain.Ticket(1, 9)) as int` -> Comportement: cette ligne crée la variable `code` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `code` reçoit ici le résultat de `s.is_critical(app/domain.Ticket(1, 9)) as int` et peut être réutilisé ensuite sans recalcul.
5. `return code` -> Comportement: cette ligne termine l'exécution du bloc courant avec le code `code`, utile pour observer le résultat global du scénario. -> Preuve: un test d'exécution peut vérifier directement que le programme retourne `code`.
6. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return code`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `code`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: `entry` assemble les couches sans les fusionner.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, chemin `io -> service -> domain`, sortie `1`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Domaine pur, service compose, entrée mince. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin. L'objectif final est de rendre chaque décision de code explicable à la première lecture, comme dans un texte de référence.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## Mini défi transverse

Défi: combinez au moins deux notions des trois derniers chapitres dans une fonction courte (garde + transformation + sortie).
Vérification minimale: montrez un cas nominal et un cas invalide, puis expliquez quelle branche est prise.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/keywords/as.md`.
- `docs/book/keywords/at.md`.
- `docs/book/keywords/bool.md`.
- `docs/book/keywords/case.md`.
- `docs/book/keywords/entry.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle garde évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?
