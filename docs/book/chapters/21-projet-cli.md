# 21. Projet guide CLI

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/20-repro.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/20-repro.md`, `docs/book/chapters/22-projet-http.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


## Niveau local

- Niveau local section coeur: Avancé.
- Niveau local exemples guidés: Intermédiaire.
- Niveau local exercices de diagnostic: Avancé.


Ce chapitre poursuit un objectif de terrain: construire un CLI Vitte qui reste propre quand les erreurs réelles arrivent. Le problème d'un outil en ligne de commande n'est pas de produire un résultat dans le cas nominal. Le vrai problème est de garder une logique lisible quand l'utilisateur oublie un argument, choisit un mode inconnu, ou enchaine des usages invalides. La méthode suivie ici est stricte: typer les erreurs, segmenter les étapes, puis centraliser la projection finale vers le système.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.

Schéma pipeline du chapitre:
- Entrée: données initiales ou requête.
- Traitement: validation, logique métier, transformations.
- Sortie: résultat métier ou code de statut.
- Invariant: propriété qui doit rester vraie à chaque étape.

## 21.1 Déclarer le contrat de résultat avant d'écrire l'orchestration

```vit
pick CliError {
  case MissingArgs
  case UnknownMode(mode: int)
}
pick CliResult {
  case Ok(code: int)
  case Err(e: CliError)
}
```

Lecture ligne par ligne (débutant):
1. `pick CliError {` cette ligne ouvre le type fermé `CliError` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
2. `case MissingArgs` cette ligne décrit le cas `MissingArgs` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `MissingArgs`, ce bloc devient le chemin actif.
3. `case UnknownMode(mode: int)` cette ligne décrit le cas `UnknownMode(mode: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `UnknownMode(mode: int)`, ce bloc devient le chemin actif.
4. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `pick CliResult {` cette ligne ouvre le type fermé `CliResult` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
6. `case Ok(code: int)` cette ligne décrit le cas `Ok(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok(code: int)`, ce bloc devient le chemin actif.
7. `case Err(e: CliError)` cette ligne décrit le cas `Err(e: CliError)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Err(e: CliError)`, ce bloc devient le chemin actif.
8. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `MissingArgs` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: poser le contrat de sortie avant toute orchestration. Tant que les fautes restent des entiers dispersés, le programme cache sa structure réelle. En introduisant `CliError` et `CliResult`, chaque fonction assume explicitement son issue.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, à ce stade, rien ne "tourne" encore, mais l'espace des sorties possibles est déjà fermé:
- soit `Ok(code)`.
- soit `Err(e)`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 21.2 Isoler la validation des arguments

```vit
proc parse_arg_count(argc: int) -> CliResult {
  if argc < 2 { give Err(MissingArgs) }
  give Ok(argc)
}
```

Lecture ligne par ligne (débutant):
1. `proc parse_arg_count(argc: int) -> CliResult {` ici, le contrat complet est défini pour `parse_arg_count`: entrées `argc: int` et sortie `CliResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `parse_arg_count` retourne toujours une valeur compatible avec `CliResult`.
2. `if argc < 2 { give Err(MissingArgs) }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `argc < 2` est vrai, `give Err(MissingArgs)` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `give Ok(argc)` ici, la branche renvoie immédiatement `Ok(argc)` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok(argc)`.
4. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `argc < 2` est vrai, la sortie devient `Err(MissingArgs)`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Ok(argc)`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: placer la validation au bord du système, avant toute logique métier.

Cette séparation est décisive: vous testéz la politique d'arité indépendamment du routage.

À l'exécution, `parse_arg_count(1)` retourne `Err(MissingArgs)` et coupe le flux nominal. `parse_arg_count(3)` retourne `Ok(3)` et autorise la suite.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 21.3 Centraliser le routage de mode

```vit
proc run_mode(mode: int) -> CliResult {
  if mode == 1 { give Ok(10) }
  if mode == 2 { give Ok(20) }
  give Err(UnknownMode(mode))
}
```

Lecture ligne par ligne (débutant):
1. `proc run_mode(mode: int) -> CliResult {` sur cette ligne, le contrat complet est posé pour `run_mode`: entrées `mode: int` et sortie `CliResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `run_mode` retourne toujours une valeur compatible avec `CliResult`.
2. `if mode == 1 { give Ok(10) }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `mode == 1` est vrai, `give Ok(10)` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if mode == 2 { give Ok(20) }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `mode == 2` est vrai, `give Ok(20)` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give Err(UnknownMode(mode))` sur cette ligne, la sortie est renvoyée immédiatement `Err(UnknownMode(mode))` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Err(UnknownMode(mode))`.
5. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `mode == 1` est vrai, la sortie devient `Ok(10)`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Err(UnknownMode(mode))`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: isoler le routage métier dans une fonction pure.

Le cas inconnu n'est pas aplati dans un `-1` anonyme: il conserve la valeur fautive dans `UnknownMode(mode)`. Cette conservation est précieuse pour le diagnostic, la télémétrie et les tests de non-régression: on ne sait pas seulement qu'il y a erreur, on sait laquelle.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `run_mode(1)` retourne `Ok(10)`, `run_mode(2)` retourne `Ok(20)`, `run_mode(9)` retourne `Err(UnknownMode(9))`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 21.4 Projeter une politique système stable

```vit
proc to_exit_code(r: CliResult) -> int {
  match r {
    case Ok(_) { give 0 }
    case Err(MissingArgs) { give 64 }
    case Err(UnknownMode(_)) { give 65 }
    otherwise { give 70 }
  }
}
```

Lecture ligne par ligne (débutant):
1. `proc to_exit_code(r: CliResult) -> int {` ce passage fixe le contrat complet de `to_exit_code`: entrées `r: CliResult` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `to_exit_code` retourne toujours une valeur compatible avec `int`.
2. `match r {` cette ligne démarre un dispatch déterministe sur `r`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `r`, la même branche sera toujours exécutée.
3. `case Ok(_) { give 0 }` cette ligne décrit le cas `Ok(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok(_)`, ce bloc devient le chemin actif.
4. `case Err(MissingArgs) { give 64 }` cette ligne décrit le cas `Err(MissingArgs)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Err(MissingArgs)`, ce bloc devient le chemin actif.
5. `case Err(UnknownMode(_)) { give 65 }` cette ligne décrit le cas `Err(UnknownMode(_))` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Err(UnknownMode(_))`, ce bloc devient le chemin actif.
6. `otherwise { give 70 }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give 70` est exécuté pour garantir une sortie stable.
7. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
8. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok(_)` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: séparer la sémantique métier de la politique d'exit code.

La conversion vers les exit-codes est un sujet d'intégration système, pas un sujet métier. La sortir dans une fonction dédiée vous donne une frontière claire: vous pouvez faire évoluer la sémantique interne sans casser les conventions du shell.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, un succès donne `0`, une arité manquante donne `64`, un mode inconnu donne `65`. Le `otherwise` garantit un filet de sécurité avec `70`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- mélanger règles métier et règles de projection système.
- réécrire les mêmes correspondances d'exit code dans plusieurs fonctions.
- laisser des cas d'erreur sans code de sortie explicite.

## 21.5 Orchestrer dans `entry` sans réinventer la logique

```vit
entry main at core/app {
  let argc: int = 2
  let parse_res: CliResult = parse_arg_count(argc)
  match parse_res {
    case Err(e) {
      return to_exit_code(Err(e))
    }
  case Ok(_) {
    let run_res: CliResult = run_mode(1)
    return to_exit_code(run_res)
  }
otherwise {
  return 70
}
}
}
```

Lecture ligne par ligne (débutant):
1. `entry main at core/app {` cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre. Exemple concret: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
2. `let argc: int = 2` cette ligne crée la variable locale `argc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `argc` reçoit ici le résultat de `2` et peut être réutilisé ensuite sans recalcul.
3. `let parse_res: CliResult = parse_arg_count(argc)` cette ligne crée la variable locale `parse_res` de type `CliResult` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `parse_res` reçoit ici le résultat de `parse_arg_count(argc)` et peut être réutilisé ensuite sans recalcul.
4. `match parse_res {` cette ligne démarre un dispatch déterministe sur `parse_res`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `parse_res`, la même branche sera toujours exécutée.
5. `case Err(e) {` cette ligne décrit le cas `Err(e)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Err(e)`, ce bloc devient le chemin actif.
6. `return to_exit_code(Err(e))` cette ligne termine l'exécution du bloc courant avec le code `to_exit_code(Err(e))`, utile pour observer le résultat global du scénario. Exemple concret: un test d'exécution peut vérifier directement que le programme retourne `to_exit_code(Err(e))`.
7. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
8. `case Ok(_) {` cette ligne décrit le cas `Ok(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok(_)`, ce bloc devient le chemin actif.
9. `let run_res: CliResult = run_mode(1)` cette ligne crée la variable locale `run_res` de type `CliResult` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `run_res` reçoit ici le résultat de `run_mode(1)` et peut être réutilisé ensuite sans recalcul.
10. `return to_exit_code(run_res)` cette ligne termine l'exécution du bloc courant avec le code `to_exit_code(run_res)`, utile pour observer le résultat global du scénario. Exemple concret: un test d'exécution peut vérifier directement que le programme retourne `to_exit_code(run_res)`.
11. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
12. `otherwise {` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
13. `return 70` cette ligne termine l'exécution du bloc courant avec le code `70`, utile pour observer le résultat global du scénario. Exemple concret: un test d'exécution peut vérifier directement que le programme retourne `70`.
14. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
15. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
16. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return to_exit_code(Err(e))`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `to_exit_code(Err(e))`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: `main` orchestre, il ne redéfinit pas les règles métier.

Cette sobriété est une force structurelle: en lisant l'entrée du programme, vous comprenez immédiatement l'ordre du flux.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- scénario nominal `argc=2`, `mode=1`: flux `parse -> run -> to_exit_code`, sortie `0`.
- scénario invalide `argc=1`: échec immédiat de validation, sortie `64`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.


## Table erreur -> diagnostic -> correction

| Erreur | Diagnostic | Correction |
| --- | --- | --- |
| Entrée invalide | Validation absente ou trop tardive. | Centraliser la validation en entrée de pipeline. |
| État incohérent | Mutation partielle ou invariant non vérifié. | Appliquer le principe d'atomicité et rejeter sinon. |
| Sortie inattendue | Couche projection mélangée avec la logique métier. | Séparer `apply` (métier) et `project` (sortie). |

## À retenir

Un CLI Vitte robuste n'est pas celui qui contient beaucoup de code, mais celui qui expose une grammaire claire de décisions. Entrée validée en frontière, métier routé dans des fonctions pures, projection système centralisée, et `entry` réduite au rôle d'orchestrateur. Avec cette discipline, la complexité augmente sans détruire la lisibilité. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez tracer le flux complet d'un appel CLI en quelques étapes.
- vous savez modifier une règle de validation sans toucher au routage.
- vous savez faire évoluer les exit codes sans toucher à la logique métier.


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

- `docs/book/keywords/at.md`.
- `docs/book/keywords/case.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/entry.md`.
- `docs/book/keywords/give.md`.


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

