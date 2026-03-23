# 8. Structures de données

Niveau: Débutant

Prérequis: chapitre précédent `book/chapters/07-controle.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/07-controle.md`, `book/chapters/09-modules.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Structures de données**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Structures de données**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Structures de données**.

## Explication pas à pas

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

Lecture ligne par ligne (débutant):
1. `form Ticket {` -> Comportement: cette ligne ouvre la structure `Ticket` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. -> Preuve: plusieurs fonctions peuvent manipuler `Ticket` sans redéfinir ses champs.
2. `id: int` -> Comportement: cette ligne déclare le champ `id` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `int`.
3. `priority: int` -> Comportement: cette ligne déclare le champ `priority` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `int`.
4. `assignee: string` -> Comportement: cette ligne déclare le champ `assignee` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `string`.
5. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
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
1. `pick TicketState {` -> Comportement: cette ligne ouvre le type fermé `TicketState` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. -> Preuve: toute valeur hors des `case` déclarés devient impossible à représenter.
2. `case Open` -> Comportement: ce cas décrit `Open` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. -> Preuve: si la valeur analysée correspond à `Open`, ce bloc devient le chemin actif.
3. `case Assigned(user: string)` -> Comportement: ce cas décrit `Assigned(user: string)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. -> Preuve: si la valeur analysée correspond à `Assigned(user: string)`, ce bloc devient le chemin actif.
4. `case Closed(code: int)` -> Comportement: ce cas décrit `Closed(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. -> Preuve: si la valeur analysée correspond à `Closed(code: int)`, ce bloc devient le chemin actif.
5. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
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
1. `proc is_critical(t: Ticket) -> bool {` -> Comportement: le contrat est défini pour `is_critical`: entrées `t: Ticket` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `is_critical` retourne toujours une valeur compatible avec `bool`.
2. `give t.priority >= 9` -> Comportement: la branche renvoie immédiatement `t.priority >= 9` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `t.priority >= 9`.
3. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
4. `proc route(t: Ticket, s: TicketState) -> int {` -> Comportement: le contrat est posé pour `route`: entrées `t: Ticket, s: TicketState` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `route` retourne toujours une valeur compatible avec `int`.
5. `if is_critical(t) and not (match s { case Closed(_) { give true } otherwise { give false } }) {` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
6. `give 1` -> Comportement: la sortie est renvoyée immédiatement `1` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `1`.
7. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
8. `give 0` -> Comportement: retourne immédiatement `0` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `0`.
9. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
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

- `book/keywords/and.md`.
- `book/keywords/bool.md`.
- `book/keywords/break.md`.
- `book/keywords/case.md`.
- `book/keywords/continue.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

<!-- AUTO_EXPANSION_V1 START -->

## Approfondissement guidé

### 1. Ce qu'il faut vraiment retenir

Le coeur de **structures de données** est de prendre des décisions lisibles et vérifiables.
Dans un projet réel, la compréhension rapide prime sur la complexité apparente.
L'objectif de cette section est de transformer le chapitre en guide opérationnel,
pas en résumé théorique.

Trois idées pratiques gouvernent ce sujet:
1. faire un changement à la fois;
2. garder des invariants explicites;
3. valider le résultat avec une preuve simple (test, sortie, diagnostic).

### 2. Carte mentale utilisable en équipe

Quand vous travaillez sur **structures de données**, posez systématiquement ces questions:
- quel est le contrat d'entrée;
- quel est le résultat attendu;
- quels sont les cas limites visibles;
- quelle erreur doit être compréhensible en moins de 30 secondes.

Cette carte mentale évite les refactors fragiles.
Elle permet aussi d'aligner débutants et profils avancés sur le même langage de travail.

### Étude de cas pratique

Cas: un module lié à **structures de données** (08-structures.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **structures de données** (08-structures.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **structures de données** (08-structures.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **structures de données** (08-structures.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **structures de données** (08-structures.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

<!-- AUTO_EXPANSION_V1 END -->
