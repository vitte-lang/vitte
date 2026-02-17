# 0. Préface

Niveau: Débutant

Prérequis: aucun prérequis technique; lire `docs/book/glossaire.md` pour le vocabulaire de base.
Voir aussi: `docs/book/chapters/01-demarrer.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Rôle d'ouverture: présenter la trajectoire d'apprentissage globale du livre, du niveau débutant jusqu'aux projets complets.

Ce chapitre poursuit un objectif clair: installer un cadre d'apprentissage progressif pour maîtriser Vitte en profondeur, du programme minimal jusqu'aux projets complets. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.

Rappel éditorial: les bases (`int`, `string`, `bool`, `proc`, `let`, `give`, etc.) sont définies une fois dans `docs/book/chapters/00-avant-propos.md`. Dans cette préface, nous les utilisons sans les redéfinir à chaque bloc.


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 0.1 Comprendre la promesse du langage

```vit
proc promise(a: int, b: int) -> int {
  give a + b
}
```

Lecture ligne par ligne (débutant):
1. `proc promise(a: int, b: int) -> int {` ici, le contrat complet est défini pour `promise`: entrées `a: int, b: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `promise` retourne toujours une valeur compatible avec `int`.
2. `give a + b` ici, la branche renvoie immédiatement `a + b` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `a + b`.
3. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `a + b`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Vitte privilégie la clarté du contrat sur la magie syntaxique. Le lecteur doit toujours voir ce qui entre, ce qui sort et pourquoi.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `promise(20,22)=42`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 0.2 Accepter la discipline des invariants

```vit
proc bounded(x: int) -> int {
  if x < 0 { give 0 }
  if x > 100 { give 100 }
  give x
}
```

Lecture ligne par ligne (débutant):
1. `proc bounded(x: int) -> int {` sur cette ligne, le contrat complet est posé pour `bounded`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `bounded` retourne toujours une valeur compatible avec `int`.
2. `if x < 0 { give 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x < 0` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if x > 100 { give 100 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x > 100` est vrai, `give 100` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give x` sur cette ligne, la sortie est renvoyée immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `x`.
5. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `x < 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Un invariant est une règle locale qui evite des classes entieres de bugs. Le livre insiste sur cette pratique dans tous les chapitres.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `bounded(-1)=0`, `bounded(70)=70`, `bounded(140)=100`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 0.3 Construire une progression orientée projet

```vit
entry main at core/app {
  let v: int = bounded(140)
  return v
}
```

Lecture ligne par ligne (débutant):
1. `entry main at core/app {` cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre. Exemple concret: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
2. `let v: int = bounded(140)` cette ligne crée la variable locale `v` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `v` reçoit ici le résultat de `bounded(140)` et peut être réutilisé ensuite sans recalcul.
3. `return v` cette ligne termine l'exécution du bloc courant avec le code `v`, utile pour observer le résultat global du scénario. Exemple concret: un test d'exécution peut vérifier directement que le programme retourne `v`.
4. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return v`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `v`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: La théorie est toujours raccordee à un flux d'exécution concret. Chaque notion abstraite est ramenee à un comportement observable.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `main` calcule `bounded(140)` puis termine avec `100`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- mélanger orchestration et logique métier dans la même fonction.
- dupliquer une règle transversale au lieu de la centraliser.
- faire évoluer l'architecture sans clarifier les responsabilités.

## À retenir

Le ton du livre est technique, le cheminement est progressif et chaque chapitre sert la construction d'un vrai code Vitte. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin. L'objectif final est de rendre chaque décision de code explicable à la première lecture, comme dans un texte de référence.


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

- `docs/book/keywords/at.md`.
- `docs/book/keywords/bool.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/entry.md`.
- `docs/book/keywords/give.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

