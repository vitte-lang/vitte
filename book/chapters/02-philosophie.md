# 2. Philosophie du langage

Niveau: Débutant

Prérequis: chapitre précédent `book/chapters/01-demarrer.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/01-demarrer.md`, `book/chapters/03-projet.md`, `book/glossaire.md`.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Philosophie du langage**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Philosophie du langage**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Philosophie du langage**.

## Méthode de lecture

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

Lecture simple du code:
1. `proc safe_div(num: int, den: int) -> int {` : contrat clair pour la division.
2. `if den == 0 { give 0 }` : bloque le cas dangereux avant l’opération.
3. `give num / den` : exécute uniquement le chemin nominal.
4. `}` : fin déterministe du bloc.

Ce qu'on vérifie en pratique:
- Cas limite: si `den == 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `num / den`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: La frontière de faute est placee avant l'operation sensible.

En pratique, ce choix simplifie la lecture: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `safe_div(10,2)=5`, `safe_div(10,0)=0`.

Erreurs classiques à éviter:
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

Lecture simple du code:
1. `pick Auth { .. }` : définit un espace d’états fermé (`Granted`/`Denied`).
2. `proc can_access(a: Auth) -> bool {` : convertit un état métier en décision booléenne.
3. `case Granted(_) { give true }` : autorise explicitement le cas de succès.
4. `case Denied(_) { give false }` : refuse explicitement le cas d’échec.
5. `otherwise { give false }` : protège contre un état non géré.

Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Granted(user: int)` permet de confirmer la branche attendue.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Les cas sont portes par le type, pas par convention.

En pratique, ce choix simplifie la lecture: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `Granted` donne `true`, `Denied` donne `false`.

Erreurs classiques à éviter:
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

Lecture simple du code:
1. `proc run() -> int { give 0 }` : logique métier isolée dans une fonction courte.
2. `entry main at core/app {` : orchestration uniquement, pas de logique lourde.
3. `return run()` : relaie explicitement le résultat métier.
4. `}` : fin nette de l’orchestration.

Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return run()`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `run()`.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Orchestration séparée du métier.

En pratique, ce choix simplifie la lecture: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `run` est appelee, puis code retourne.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Erreurs explicites, états modelises, orchestration séparée. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin. L'objectif final est de rendre chaque décision de code explicable à la première lecture, comme dans un texte de référence.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Repère: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
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

- `book/keywords/at.md`.
- `book/keywords/bool.md`.
- `book/keywords/case.md`.
- `book/keywords/continue.md`.
- `book/keywords/entry.md`.
