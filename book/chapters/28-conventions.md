# 28. Conventions de code

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/27-grammaire.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/27-grammaire.md`, `docs/book/chapters/29-style.md`, `book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: uniformiser l'écriture Vitte pour réduire le coût de relecture et accélérer la correction en équipe. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 28.1 Nommer les procédures selon leur intention

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  if x > 65535 { give -1 }
  give x
}
```

Lecture ligne par ligne (débutant):
1. `proc parse_port(x: int) -> int {` ici, le contrat complet est défini pour `parse_port`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `parse_port` retourne toujours une valeur compatible avec `int`.
2. `if x < 0 { give -1 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x < 0` est vrai, `give -1` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if x > 65535 { give -1 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x > 65535` est vrai, `give -1` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give x` ici, la branche renvoie immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `x`.
5. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `x < 0` est vrai, la sortie devient `-1`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: faire porter l'intention métier par le nom de la procédure.

Le verbe `parse` signale un contrat de validation, et les gardes en tête rendent le flux immédiat à relire.

À l'exécution, `parse_port(8080)=8080`, `parse_port(70000)=-1`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 28.2 Nommer les structures selon le domaine

```vit
form User {
  id: int
  name: string
}
```

Lecture ligne par ligne (débutant):
1. `form User {` cette ligne ouvre la structure `User` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `User` sans redéfinir ses champs.
2. `id: int` cette ligne déclare le champ `id` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
3. `name: string` cette ligne déclare le champ `name` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `string`.
4. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: utiliser des noms de structure et de champs qui décrivent le domaine, pas l'implémentation.

Le schéma devient auto-documenté: la structure se lit comme une mini spécification métier.

À l'exécution, cette déclaration n'exécute rien, mais elle fixe un contrat de données clair au compile-time.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 28.3 Nommer les prédicats en booléen explicite

```vit
proc is_admin(role: int) -> bool {
  give role == 9
}
```

Lecture ligne par ligne (débutant):
1. `proc is_admin(role: int) -> bool {` sur cette ligne, le contrat complet est posé pour `is_admin`: entrées `role: int` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `is_admin` retourne toujours une valeur compatible avec `bool`.
2. `give role == 9` sur cette ligne, la sortie est renvoyée immédiatement `role == 9` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `role == 9`.
3. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `role == 9`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: rendre les prédicats immédiatement reconnaissables dans les conditions.

Le préfixe `is_` réduit la charge cognitive: dans un `if`, on lit directement une question booléenne.

À l'exécution, `is_admin(9)=true` et `is_admin(1)=false`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Les noms expriment l'intention, les gardes sont en tête et les prédicats booléens sont reconnaissables instantanément. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous pouvez deviner le rôle d'une fonction en lisant uniquement son nom.
- vous savez distinguer un prédicat booléen d'une transformation.
- vous pouvez relire un fichier et repérer les conventions violées en quelques secondes.


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

- `docs/book/keywords/bool.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/false.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/give.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

