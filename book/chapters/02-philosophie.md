# 2. Philosophie du langage

Niveau: Débutant

Prérequis: chapitre précédent `docs/book/chapters/01-demarrer.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/01-demarrer.md`, `docs/book/chapters/03-projet.md`, `book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: traduire la philosophie Vitte en décisions de code vérifiables. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 2.1 Rendre l'erreur explicite

```vit
proc safe_div(num: int, den: int) -> int {
  if den == 0 { give 0 }
  give num / den
}
```

Lecture ligne par ligne (débutant):
1. `proc safe_div(num: int, den: int) -> int {` ici, le contrat complet est défini pour `safe_div`: entrées `num: int, den: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `safe_div` retourne toujours une valeur compatible avec `int`.
2. `if den == 0 { give 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `den == 0` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `give num / den` ici, la branche renvoie immédiatement `num / den` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `num / den`.
4. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `den == 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `num / den`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: La frontière de faute est placee avant l'operation sensible.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `safe_div(10,2)=5`, `safe_div(10,0)=0`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- testér uniquement le cas nominal et ignorer les frontières.
- confondre le symptôme observé et la cause réelle.
- traiter les erreurs dans tous les sens au lieu de centraliser la politique.

## 2.2 Modéliser les états au lieu de coder des nombres magiques

```vit
pick Auth {
  case Granted(user: int)
  case Denied(code: int)
}
proc can_access(a: Auth) -> bool {
  match a {
    case Granted(_) { give true }
    case Denied(_) { give false }
    otherwise { give false }
  }
}
```

Lecture ligne par ligne (débutant):
1. `pick Auth {` cette ligne ouvre le type fermé `Auth` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
2. `case Granted(user: int)` cette ligne décrit le cas `Granted(user: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Granted(user: int)`, ce bloc devient le chemin actif.
3. `case Denied(code: int)` cette ligne décrit le cas `Denied(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Denied(code: int)`, ce bloc devient le chemin actif.
4. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc can_access(a: Auth) -> bool {` sur cette ligne, le contrat complet est posé pour `can_access`: entrées `a: Auth` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `can_access` retourne toujours une valeur compatible avec `bool`.
6. `match a {` cette ligne démarre un dispatch déterministe sur `a`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `a`, la même branche sera toujours exécutée.
7. `case Granted(_) { give true }` cette ligne décrit le cas `Granted(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Granted(_)`, ce bloc devient le chemin actif.
8. `case Denied(_) { give false }` cette ligne décrit le cas `Denied(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Denied(_)`, ce bloc devient le chemin actif.
9. `otherwise { give false }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give false` est exécuté pour garantir une sortie stable.
10. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
11. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Granted(user: int)` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Les cas sont portes par le type, pas par convention.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `Granted` donne `true`, `Denied` donne `false`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 2.3 Garder `entry` mince

```vit
proc run() -> int { give 0 }
entry main at core/app {
  return run()
}
```

Lecture ligne par ligne (débutant):
1. `proc run() -> int { give 0 }` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `entry main at core/app {` cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre. Exemple concret: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
3. `return run()` cette ligne termine l'exécution du bloc courant avec le code `run()`, utile pour observer le résultat global du scénario. Exemple concret: un test d'exécution peut vérifier directement que le programme retourne `run()`.
4. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return run()`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `run()`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Orchestration séparée du métier.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `run` est appelee, puis code retourne.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Erreurs explicites, états modelises, orchestration séparée. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin. L'objectif final est de rendre chaque décision de code explicable à la première lecture, comme dans un texte de référence.


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
- `docs/book/keywords/case.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/entry.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

