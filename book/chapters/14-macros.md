# 14. Macros

Niveau: Intermédiaire

Prérequis: chapitre précédent `docs/book/chapters/13-generiques.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/13-generiques.md`, `book/chapters/15-pipeline.md`, `book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif net: vous donner une maîtrise adulte des macros Vitte, c'est-à-dire la capacité de factoriser du code sans introduire une zone grise dans la lecture du programme. Une macro n'est pas un gadget stylistique. C'est un outil de compression syntaxique qui doit rester entièrement compréhensible par la personne qui relit le code six mois plus tard sous pression de production.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 14.1 Commencer par une macro volontairement neutre

```vit
macro nop() {
  let _: int = 0
}
```

Lecture ligne par ligne (débutant):
1. `macro nop() {` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `let _: int = 0` cette ligne crée la variable locale `_` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `_` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: commencer par une macro neutre pour comprendre le mécanisme d'expansion sans bruit métier.

Cette progression évite un piège fréquent: écrire des macros puissantes trop tôt, avant d'avoir stabilisé les conventions de lecture de l'équipe.

À l'exécution, l'appel `nop()` n'ajoute aucune logique fonctionnelle. Le flux observable du programme reste identique, ce qui permet d'isoler le comportement de la macro elle-même.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- introduire une macro sans pouvoir expliquer son expansion.
- utiliser une macro là où une procédure claire suffit.
- multiplier les macros "magiques" qui cachent le flux réel.

## 14.2 Injecter une garde réutilisable dans une procédure sensible

```vit
macro guard_nonzero(x) {
  if x == 0 { return -1 }
}
proc safe_div(num: int, den: int) -> int {
  guard_nonzero(den)
  give num / den
}
```

Lecture ligne par ligne (débutant):
1. `macro guard_nonzero(x) {` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `if x == 0 { return -1 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x == 0` est vrai, `return -1` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
4. `proc safe_div(num: int, den: int) -> int {` ici, le contrat complet est défini pour `safe_div`: entrées `num: int, den: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `safe_div` retourne toujours une valeur compatible avec `int`.
5. `guard_nonzero(den)` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
6. `give num / den` ici, la branche renvoie immédiatement `num / den` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `num / den`.
7. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `num / den`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: utiliser une macro pour factoriser une garde critique répétée.

La valeur de `guard_nonzero` n'est pas seulement de gagner quelques lignes. Elle impose une politique uniforme sur un risque concret: la division par zéro.

À l'exécution:
- `safe_div(10,2)` passe la garde et retourne `5`.
- `safe_div(10,0)` est arrêté par la garde et retourne `-1`.

La frontière d'erreur est placée exactement là où le risque apparaît.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 14.3 Encapsuler une normalisation locale sans dissoudre la responsabilité

```vit
macro clamp01(v) {
  if v < 0 { set v = 0 }
  if v > 1 { set v = 1 }
}
proc normalize01(x: int) -> int {
  let v: int = x
  clamp01(v)
  give v
}
```

Lecture ligne par ligne (débutant):
1. `macro clamp01(v) {` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `if v < 0 { set v = 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `v < 0` est vrai, `set v = 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if v > 1 { set v = 1 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `v > 1` est vrai, `set v = 1` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc normalize01(x: int) -> int {` sur cette ligne, le contrat complet est posé pour `normalize01`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `normalize01` retourne toujours une valeur compatible avec `int`.
6. `let v: int = x` cette ligne crée la variable locale `v` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `v` reçoit ici le résultat de `x` et peut être réutilisé ensuite sans recalcul.
7. `clamp01(v)` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
8. `give v` sur cette ligne, la sortie est renvoyée immédiatement `v` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `v`.
9. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `v`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: encapsuler une normalisation locale sans masquer la responsabilité métier.

La règle d'or est simple: une macro est acceptable tant qu'un lecteur peut reconstruire son expansion sans effort disproportionné.

À l'exécution:
- `normalize01(-3)` retourne `0`.
- `normalize01(0)` retourne `0`.
- `normalize01(5)` retourne `1`.

La sortie est bornée dans `[0,1]`, et cette garantie reste visible dans la logique source.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Une macro Vitte utile n'est ni mystique ni décorative. Elle rend une règle récurrente plus stable, plus auditable et plus facile à faire évoluer. Si elle masque l'intention, supprimez-la. Si elle clarifie un contrat répété, gardez-la et documentez-la comme une pièce d'architecture. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous pouvez expliquer l'expansion d'une macro en quelques phrases.
- vous savez dire pourquoi la macro existe au lieu d'une procédure.
- vous pouvez modifier la règle macro sans casser les appels existants.


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
- `docs/book/keywords/asm.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/give.md`.
- `docs/book/keywords/if.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

