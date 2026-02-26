# 7. Instructions de contrôle

Niveau: Débutant

Prérequis: chapitre précédent `docs/book/chapters/06-procedures.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/06-procedures.md`, `docs/book/chapters/08-structures.md`, `book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: maîtriser les décisions, les itérations et les sorties sans ambiguïté de flux. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 7.1 Garde de classification

```vit
proc classify(temp: int) -> int {
  if temp < 0 { give -1 }
  if temp > 100 { give 2 }
  give 1
}
```

Lecture ligne par ligne (débutant):
1. `proc classify(temp: int) -> int {` ici, le contrat complet est défini pour `classify`: entrées `temp: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `classify` retourne toujours une valeur compatible avec `int`.
2. `if temp < 0 { give -1 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `temp < 0` est vrai, `give -1` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if temp > 100 { give 2 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `temp > 100` est vrai, `give 2` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give 1` ici, la branche renvoie immédiatement `1` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `1`.
5. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `temp < 0` est vrai, la sortie devient `-1`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `1`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: classer une valeur en traitant d'abord les cas hors plage, puis le cas nominal.

Ce schéma rend le contrôle facile à relire: chaque garde correspond à une règle métier visible, et la dernière ligne couvre le cas courant.

Lecture pas à pas:
- `classify(-2)` retourne `-1` (valeur sous la plage).
- `classify(50)` retourne `1` (valeur nominale).
- `classify(120)` retourne `2` (valeur au-dessus de la plage).

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 7.2 Boucle somme

```vit
proc sum_to(limit: int) -> int {
  let i: int = 0
  let acc: int = 0
  loop {
    if i > limit { break }
    set acc = acc + i
    set i = i + 1
  }
give acc
}
```

Lecture ligne par ligne (débutant):
1. `proc sum_to(limit: int) -> int {` sur cette ligne, le contrat complet est posé pour `sum_to`: entrées `limit: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `sum_to` retourne toujours une valeur compatible avec `int`.
2. `let i: int = 0` cette ligne crée la variable locale `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `let acc: int = 0` cette ligne crée la variable locale `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `acc` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
4. `loop {` cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). Exemple concret: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
5. `if i > limit { break }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `i > limit` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `set acc = acc + i` cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux. Exemple concret: après exécution, `acc` prend la nouvelle valeur `acc + i` pour les étapes suivantes.
7. `set i = i + 1` cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. Exemple concret: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
8. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
9. `give acc` sur cette ligne, la sortie est renvoyée immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `acc`.
10. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: construire une boucle dont l'invariant est verbalizable, ici `acc` contient la somme des valeurs déjà parcourues.

Ordre de lecture recommandé:
- vérifier la borne d'arrêt (`i > limit`).
- suivre la mise à jour de l'accumulateur.
- suivre l'incrément de l'index.

À l'exécution, pour `sum_to(3)`:
- départ `i=0`, `acc=0`.
- après `i=0`: `acc=0`.
- après `i=1`: `acc=1`.
- après `i=2`: `acc=3`.
- après `i=3`: `acc=6`.
- `i=4` déclenche l'arrêt, retour `6`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- laisser une boucle sans borne claire ou sans condition d'arrêt vérifiable.
- faire évoluer plusieurs variables d'état sans documenter leur rôle.
- optimiser trop tôt sans verrouiller d'abord le comportement attendu.

## 7.3 Filtrage en parcours

```vit
proc filtered_sum(values: int[]) -> int {
  let acc: int = 0
  for x in values {
    if x < 0 { continue }
    set acc = acc + x
  }
give acc
}
```

Lecture ligne par ligne (débutant):
1. `proc filtered_sum(values: int[]) -> int {` ce passage fixe le contrat complet de `filtered_sum`: entrées `values: int[]` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `filtered_sum` retourne toujours une valeur compatible avec `int`.
2. `let acc: int = 0` cette ligne crée la variable locale `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `acc` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `for x in values {` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
4. `if x < 0 { continue }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x < 0` est vrai, `continue` est exécuté immédiatement; sinon on continue sur la ligne suivante.
5. `set acc = acc + x` cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux. Exemple concret: après exécution, `acc` prend la nouvelle valeur `acc + x` pour les étapes suivantes.
6. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
7. `give acc` ce passage retourne immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `acc`.
8. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: filtrer en parcours sans casser la lisibilité de la boucle.

La règle est simple: les valeurs négatives sont ignorées (`continue`), les autres sont accumulées.

À l'exécution, sur `[5,-2,7]`:
- `5` est ajoutée, `acc=5`.
- `-2` est ignorée.
- `7` est ajoutée, `acc=12`.
- retour final `12`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- laisser une boucle sans borne claire ou sans condition d'arrêt vérifiable.
- faire évoluer plusieurs variables d'état sans documenter leur rôle.
- optimiser trop tôt sans verrouiller d'abord le comportement attendu.

## À retenir

Sortie de boucle explicite, invariant verbalisable, effets localisés. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez expliquer pourquoi une branche est prise.
- vous savez suivre une boucle tour par tour sans ambiguïté.
- vous savez justifier le résultat final à partir de l'invariant.


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

- `docs/book/keywords/break.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/for.md`.
- `docs/book/keywords/give.md`.
- `docs/book/keywords/if.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

