# 16. Interop et ABI

Niveau: Intermédiaire

Prérequis: chapitre précédent `docs/book/chapters/15-pipeline.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/15-pipeline.md`, `docs/book/chapters/17-stdlib.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: concevoir une frontière d'interopérabilité Vitte qui conserve des contrats types clairs entre code local et code externe. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 16.1 Encoder explicitement les données de frontière

```vit
form Request {
  code: int
  payload: string
}
proc encode_code(r: Request) -> int {
  give r.code
}
```

Lecture ligne par ligne (débutant):
1. `form Request {` cette ligne ouvre la structure `Request` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `Request` sans redéfinir ses champs.
2. `code: int` cette ligne déclare le champ `code` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
3. `payload: string` cette ligne déclare le champ `payload` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `string`.
4. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc encode_code(r: Request) -> int {` ici, le contrat complet est défini pour `encode_code`: entrées `r: Request` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `encode_code` retourne toujours une valeur compatible avec `int`.
6. `give r.code` ici, la branche renvoie immédiatement `r.code` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `r.code`.
7. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `r.code`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: rendre explicites les données qui traversent la frontière ABI.

Ce niveau d'explicitation évite les ambiguïtés de sérialisation et de mapping: chaque champ est nommé, typé et lisible.

À l'exécution, `encode_code(Request(200,"ok"))=200`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 16.2 Mapper un code externe vers un type somme interne

```vit
pick IoResult {
  case Ok(value: int)
  case Err(errno: int)
}
proc map_errno(e: int) -> IoResult {
  if e == 0 { give Ok(0) }
  give Err(e)
}
```

Lecture ligne par ligne (débutant):
1. `pick IoResult {` cette ligne ouvre le type fermé `IoResult` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
2. `case Ok(value: int)` cette ligne décrit le cas `Ok(value: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok(value: int)`, ce bloc devient le chemin actif.
3. `case Err(errno: int)` cette ligne décrit le cas `Err(errno: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Err(errno: int)`, ce bloc devient le chemin actif.
4. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc map_errno(e: int) -> IoResult {` sur cette ligne, le contrat complet est posé pour `map_errno`: entrées `e: int` et sortie `IoResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `map_errno` retourne toujours une valeur compatible avec `IoResult`.
6. `if e == 0 { give Ok(0) }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `e == 0` est vrai, `give Ok(0)` est exécuté immédiatement; sinon on continue sur la ligne suivante.
7. `give Err(e)` sur cette ligne, la sortie est renvoyée immédiatement `Err(e)` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Err(e)`.
8. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `e == 0` est vrai, la sortie devient `Ok(0)`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Err(e)`.
- Observation testable: forcer le cas `Ok(value: int)` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: convertir un protocole d'erreur externe (codes entiers) en modèle interne typé.

Avec ce mapping, le reste du code ne manipule plus des nombres "magiques". Il manipule des variantes explicites (`Ok` ou `Err`).

À l'exécution:
- `map_errno(0)` retourne `Ok(0)`.
- `map_errno(13)` retourne `Err(13)`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 16.3 Isoler le point machine `unsafe`

```vit
proc syscall_halt() -> int {
  unsafe {
    asm("hlt")
  }
give 0
}
```

Lecture ligne par ligne (débutant):
1. `proc syscall_halt() -> int {` ce passage fixe le contrat complet de `syscall_halt`: entrées `` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `syscall_halt` retourne toujours une valeur compatible avec `int`.
2. `unsafe {` cette ligne marque une zone sensible qui doit rester courte, justifiée et facile à auditer dans un contexte système. Exemple concret: on y place seulement l'opération technique impossible à exprimer en mode sûr.
3. `asm("hlt")` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
4. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `give 0` ce passage retourne immédiatement `0` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `0`.
6. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `0`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: isoler strictement le point `unsafe` lié à l'instruction machine.

Cette isolation rend l'audit concret: un seul point à inspecter, un seul point à encadrer, un seul point à testér indirectement.

À l'exécution, en contexte autorisé, `hlt` est exécuté puis la procédure retourne `0` si le flot revient.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- étendre la zone sensible au lieu de la garder courte et auditable.
- placer la validation après l'opération risquée.
- masquer la frontière technique, ce qui rend le diagnostic plus coûteux.

## À retenir

Les données ABI sont explicites, les erreurs externes sont typées et la zone `unsafe` est strictement bornée. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez identifier exactement quelles données traversent l'ABI.
- vous savez traduire un code externe en variante métier interne.
- vous pouvez montrer que la zone `unsafe` reste minimale et localisée.


## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## ABI/runtime checklist (commun)

- signatures ABI cohérentes entre frontières Vitte et natif.
- symboles runtime effectivement exportés et linkés.
- headers/runtime disponibles sur la cible.
- contraintes freestanding explicitées pour les cibles kernel.
- validation d'appel natif avant optimisation.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/keywords/as.md`.
- `docs/book/keywords/asm.md`.
- `docs/book/keywords/case.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/form.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

