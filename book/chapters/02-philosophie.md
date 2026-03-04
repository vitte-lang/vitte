# 2. Philosophie du langage

Niveau: Débutant

Prérequis: chapitre précédent `docs/book/chapters/01-demarrer.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/01-demarrer.md`, `docs/book/chapters/03-projet.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Philosophie du langage**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Philosophie du langage**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Philosophie du langage**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Philosophie du langage** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Philosophie du langage**.

## Exercice court

Prenez un exemple du chapitre sur **Philosophie du langage**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Philosophie du langage**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 2.1 Rendre l'erreur explicite

```vit
proc safe_div(num: int, den: int) -> int {
  if den == 0 { give 0 }
  give num / den
}
```

Lecture ligne par ligne (débutant):
1. `proc safe_div(num: int, den: int) -> int {` -> Comportement: contrat clair pour la division. -> Preuve: la sortie attendue est toujours un `int`.
2. `if den == 0 { give 0 }` -> Comportement: bloque le cas dangereux avant l’opération. -> Preuve: `safe_div(10,0)` renvoie `0` sans division.
3. `give num / den` -> Comportement: exécute uniquement le chemin nominal. -> Preuve: `safe_div(10,2)` renvoie `5`.
4. `}` -> Comportement: fin déterministe du bloc. -> Preuve: aucun comportement implicite après les `give`.

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
1. `pick Auth { ... }` -> Comportement: définit un espace d’états fermé (`Granted`/`Denied`). -> Preuve: les cas sont visibles et auditables.
2. `proc can_access(a: Auth) -> bool {` -> Comportement: convertit un état métier en décision booléenne. -> Preuve: signature simple à tester.
3. `case Granted(_) { give true }` -> Comportement: autorise explicitement le cas de succès. -> Preuve: `Granted(1)` produit `true`.
4. `case Denied(_) { give false }` -> Comportement: refuse explicitement le cas d’échec. -> Preuve: `Denied(403)` produit `false`.
5. `otherwise { give false }` -> Comportement: protège contre un état non géré. -> Preuve: la sortie reste sûre (`false`).

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
1. `proc run() -> int { give 0 }` -> Comportement: logique métier isolée dans une fonction courte. -> Preuve: test unitaire possible sans `entry`.
2. `entry main at core/app {` -> Comportement: orchestration uniquement, pas de logique lourde. -> Preuve: le bloc se limite à appeler `run`.
3. `return run()` -> Comportement: relaie explicitement le résultat métier. -> Preuve: la sortie de `main` suit directement `run`.
4. `}` -> Comportement: fin nette de l’orchestration. -> Preuve: pas d’effet caché supplémentaire.

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
