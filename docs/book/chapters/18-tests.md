# 18. Tests et validation

Niveau: Intermédiaire

Prérequis: chapitre précédent `docs/book/chapters/17-stdlib.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/17-stdlib.md`, `docs/book/chapters/19-performance.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: Écrire des tests Vitte qui verrouillent le contrat nominal, les bornes et la non-régression. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 18.1 Tester une fonction de saturation

```vit
proc clamp(x: int, lo: int, hi: int) -> int {
  if x < lo { give lo }
  if x > hi { give hi }
  give x
}
```

Lecture ligne par ligne (débutant):
1. `proc clamp(x: int, lo: int, hi: int) -> int {` ici, le contrat complet est défini pour `clamp`: entrées `x: int, lo: int, hi: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `clamp` retourne toujours une valeur compatible avec `int`.
2. `if x < lo { give lo }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x < lo` est vrai, `give lo` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if x > hi { give hi }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x > hi` est vrai, `give hi` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give x` ici, la branche renvoie immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `x`.
5. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `x < lo` est vrai, la sortie devient `lo`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: couvrir systématiquement les trois classes de cas d'un contrat borné.

Cette grille simple évite les tests "au hasard": on testé sous la borne, dans l'intervalle, au-dessus de la borne.

À l'exécution:
- `clamp(-1,0,10)` retourne `0`.
- `clamp(5,0,10)` retourne `5`.
- `clamp(99,0,10)` retourne `10`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- testér uniquement le cas nominal et ignorer les frontières.
- confondre le symptôme observé et la cause réelle.
- traiter les erreurs dans tous les sens au lieu de centraliser la politique.

## 18.2 Verrouiller les frontières d'un parseur

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  if x > 65535 { give -1 }
  give x
}
```

Lecture ligne par ligne (débutant):
1. `proc parse_port(x: int) -> int {` sur cette ligne, le contrat complet est posé pour `parse_port`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `parse_port` retourne toujours une valeur compatible avec `int`.
2. `if x < 0 { give -1 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x < 0` est vrai, `give -1` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if x > 65535 { give -1 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x > 65535` est vrai, `give -1` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give x` sur cette ligne, la sortie est renvoyée immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `x`.
5. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `x < 0` est vrai, la sortie devient `-1`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: verrouiller les frontières du domaine de validité.

Le choix des valeurs de test est volontaire:
- juste en dessous de la borne basse.
- exactement sur les bornes.
- juste au-dessus de la borne haute.

À l'exécution:
- `parse_port(0)` retourne `0`.
- `parse_port(65535)` retourne `65535`.
- `parse_port(65536)` retourne `-1`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 18.3 Capturer une regression sur division

```vit
proc non_reg_demo(x: int) -> int {
  if x == 0 { give 0 }
  give 10 / x
}
```

Lecture ligne par ligne (débutant):
1. `proc non_reg_demo(x: int) -> int {` ce passage fixe le contrat complet de `non_reg_demo`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `non_reg_demo` retourne toujours une valeur compatible avec `int`.
2. `if x == 0 { give 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x == 0` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `give 10 / x` ce passage retourne immédiatement `10 / x` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `10 / x`.
4. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `x == 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `10 / x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: capturer explicitement un bug déjà rencontré pour empêcher son retour.

Ce test sert de mémoire technique: il documente un risque réel et verrouille la correction.

À l'exécution:
- `non_reg_demo(0)` retourne `0`.
- `non_reg_demo(2)` retourne `5`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Vos tests couvrent nominal, bornes et historiques de bugs avec des attentes explicites. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez justifier chaque cas de test par une règle du contrat.
- vous savez distinguer test de frontière et test de non-régression.
- vous pouvez identifier rapidement quel invariant échoue quand un test casse.


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
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `u32`, `u64` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/keywords/const.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/give.md`.
- `docs/book/keywords/if.md`.
- `docs/book/keywords/int.md`.


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

