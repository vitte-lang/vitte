# 16. Interop et ABI

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/15-pipeline.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/15-pipeline.md`, `book/chapters/17-stdlib.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Interop et ABI**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Interop et ABI**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Interop et ABI**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Interop et ABI** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Interop et ABI**.

## Exercice court

Prenez un exemple du chapitre sur **Interop et ABI**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Interop et ABI**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

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
1. `form Request {` -> Comportement: cette ligne ouvre la structure `Request` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. -> Preuve: plusieurs fonctions peuvent manipuler `Request` sans redéfinir ses champs.
2. `code: int` -> Comportement: cette ligne déclare le champ `code` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `int`.
3. `payload: string` -> Comportement: cette ligne déclare le champ `payload` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `string`.
4. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc encode_code(r: Request) -> int {` -> Comportement: le contrat est défini pour `encode_code`: entrées `r: Request` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `encode_code` retourne toujours une valeur compatible avec `int`.
6. `give r.code` -> Comportement: la branche renvoie immédiatement `r.code` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `r.code`.
7. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
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
1. `pick IoResult {` -> Comportement: cette ligne ouvre le type fermé `IoResult` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. -> Preuve: toute valeur hors des `case` déclarés devient impossible à représenter.
2. `case Ok(value: int)` -> Comportement: ce cas décrit `Ok(value: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. -> Preuve: si la valeur analysée correspond à `Ok(value: int)`, ce bloc devient le chemin actif.
3. `case Err(errno: int)` -> Comportement: ce cas décrit `Err(errno: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. -> Preuve: si la valeur analysée correspond à `Err(errno: int)`, ce bloc devient le chemin actif.
4. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc map_errno(e: int) -> IoResult {` -> Comportement: le contrat est posé pour `map_errno`: entrées `e: int` et sortie `IoResult`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `map_errno` retourne toujours une valeur compatible avec `IoResult`.
6. `if e == 0 { give Ok(0) }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `e == 0` est vrai, `give Ok(0)` est exécuté immédiatement; sinon on continue sur la ligne suivante.
7. `give Err(e)` -> Comportement: la sortie est renvoyée immédiatement `Err(e)` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `Err(e)`.
8. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
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
1. `proc syscall_halt() -> int {` -> Comportement: le contrat est fixé pour `syscall_halt`: entrées `` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `syscall_halt` retourne toujours une valeur compatible avec `int`.
2. `unsafe {` -> Comportement: cette ligne marque une zone sensible qui doit rester courte, justifiée et facile à auditer dans un contexte système. -> Preuve: on y place seulement l'opération technique impossible à exprimer en mode sûr.
3. `asm("hlt")` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
4. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `give 0` -> Comportement: retourne immédiatement `0` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `0`.
6. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
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
