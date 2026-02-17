# 10. Diagnostics et erreurs

Niveau: Intermédiaire

Prérequis: chapitre précédent `docs/book/chapters/09-modules.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/09-modules.md`, `docs/book/chapters/11-collections.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: convertir les fautes en sorties diagnostiques exploitables. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 10.1 Garde de division

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

L'intention de cette étape est directe: placer la frontière d'erreur au plus près de l'opération risquée.

Ce principe évite les crashs évitables: la division est protégée avant d'être exécutée.

À l'exécution:
- `safe_div(12,3)` retourne `4`.
- `safe_div(12,0)` retourne `0` sans tenter la division.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 10.2 Erreur typée

```vit
pick ParsePort {
  case Ok(value: int)
  case Err(code: int)
}
proc parse_port(x: int) -> ParsePort {
  if x < 0 { give Err(400) }
  if x > 65535 { give Err(422) }
  give Ok(x)
}
```

Lecture ligne par ligne (débutant):
1. `pick ParsePort {` cette ligne ouvre le type fermé `ParsePort` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
2. `case Ok(value: int)` cette ligne décrit le cas `Ok(value: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok(value: int)`, ce bloc devient le chemin actif.
3. `case Err(code: int)` cette ligne décrit le cas `Err(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Err(code: int)`, ce bloc devient le chemin actif.
4. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc parse_port(x: int) -> ParsePort {` sur cette ligne, le contrat complet est posé pour `parse_port`: entrées `x: int` et sortie `ParsePort`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `parse_port` retourne toujours une valeur compatible avec `ParsePort`.
6. `if x < 0 { give Err(400) }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x < 0` est vrai, `give Err(400)` est exécuté immédiatement; sinon on continue sur la ligne suivante.
7. `if x > 65535 { give Err(422) }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `x > 65535` est vrai, `give Err(422)` est exécuté immédiatement; sinon on continue sur la ligne suivante.
8. `give Ok(x)` sur cette ligne, la sortie est renvoyée immédiatement `Ok(x)` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok(x)`.
9. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `x < 0` est vrai, la sortie devient `Err(400)`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Ok(x)`.
- Observation testable: forcer le cas `Ok(value: int)` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: encoder le diagnostic dans le type de retour, pour rendre les échecs aussi explicites que les succès.

Avec ce modèle, on ne perd pas l'information d'erreur: chaque cas garde son code associé.

À l'exécution:
- `parse_port(-1)` retourne `Err(400)`.
- `parse_port(8080)` retourne `Ok(8080)`.
- `parse_port(70000)` retourne `Err(422)`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 10.3 Projection technique

```vit
proc to_exit(p: ParsePort) -> int {
  match p {
    case Ok(_) { give 0 }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}
```

Lecture ligne par ligne (débutant):
1. `proc to_exit(p: ParsePort) -> int {` ce passage fixe le contrat complet de `to_exit`: entrées `p: ParsePort` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `to_exit` retourne toujours une valeur compatible avec `int`.
2. `match p {` cette ligne démarre un dispatch déterministe sur `p`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `p`, la même branche sera toujours exécutée.
3. `case Ok(_) { give 0 }` cette ligne décrit le cas `Ok(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok(_)`, ce bloc devient le chemin actif.
4. `case Err(c) { give c }` cette ligne décrit le cas `Err(c)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Err(c)`, ce bloc devient le chemin actif.
5. `otherwise { give 70 }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give 70` est exécuté pour garantir une sortie stable.
6. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
7. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok(_)` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: séparer la logique métier de sa projection technique (ici, le code de sortie).

Ce découplage est important: la politique système peut évoluer sans réécrire la logique de parsing.

À l'exécution:
- `to_exit(Ok(_))` retourne `0`.
- `to_exit(Err(422))` retourne `422`.
- `otherwise` garde un code de secours (`70`).

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Cause localisée, typée, projetée proprement. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez localiser la cause d'une erreur sans parcourir tout le code.
- vous savez distinguer résultat métier et projection technique.
- vous pouvez testér séparément le parsing et la politique d'exit code.


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

- `docs/book/keywords/case.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/field.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/give.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

