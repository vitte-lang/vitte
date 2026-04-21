# 21. Projet guide CLI

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/20-repro.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/20-repro.md`, `docs/book/chapters/22-projet-http.md`, `docs/book/glossaire.md`.

## Problème Concret

Situation réelle: Projet guide CLI devient clair quand on trace chaque étape du calcul. L'objectif est de relier ligne de code et effet concret sur la sortie.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: on conserve un même mini-programme pour comparer les effets d'une modification sans changer tout le contexte.
Objectif pédagogique: passer de la lecture passive à la preuve: même entrée, même branche, même sortie attendue.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Projet guide CLI**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Projet guide CLI** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Projet guide CLI**.

## Exercice court

Prenez un exemple du chapitre sur **Projet guide CLI**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Projet guide CLI**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

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

## 21.1 Déclarer la règle de résultat avant d'écrire l'orchestration

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
1. `pick CliError {` : cette ligne ouvre le type fermé `CliError` pour forcer un ensemble fini de cas possibles et supprimer les états implicites.
2. `case MissingArgs` : ce cas décrit `MissingArgs` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
3. `case UnknownMode(mode: int)` : ce cas décrit `UnknownMode(mode: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
4. `}` : cette accolade ferme le bloc logique.
5. `pick CliResult {` : cette ligne ouvre le type fermé `CliResult` pour forcer un ensemble fini de cas possibles et supprimer les états implicites.
6. `case Ok(code: int)` : ce cas décrit `Ok(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
7. `case Err(e: CliError)` : ce cas décrit `Err(e: CliError)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
8. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie stable.
- Observation testable: forcer le cas `MissingArgs` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: poser la règle de sortie avant toute orchestration. Tant que les fautes restent des entiers dispersés, le programme cache sa structure réelle. En introduisant `CliError` et `CliResult`, chaque fonction assume explicitement son issue.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, à ce stade, rien ne "tourne" encore, mais l'espace des sorties possibles est déjà fermé:
- soit `Ok(code)`.
- soit `Err(e)`.

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
1. `proc parse_arg_count(argc: int) -> CliResult {` : la règle est défini pour `parse_arg_count`: entrées `argc: int` et sortie `CliResult`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if argc < 2 { give Err(MissingArgs) }` : cette test traite le cas d'erreur avant le calcul.
3. `give Ok(argc)` : la branche renvoie immédiatement `Ok(argc)` pour la branche courante, la sortie de branche est explicite et vérifiable.
4. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: si `argc < 2` est vrai, la sortie devient `Err(MissingArgs)`.
- Cas nominal: sans test bloquante, la branche principale renvoie `Ok(argc)`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: placer la validation au bord du système, avant toute logique métier.

Cette séparation est décisive: vous testéz la politique d'arité indépendamment du routage.

À l'exécution, `parse_arg_count(1)` retourne `Err(MissingArgs)` et coupe le flux nominal. `parse_arg_count(3)` retourne `Ok(3)` et autorise la suite.

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
1. `proc run_mode(mode: int) -> CliResult {` : la règle est posé pour `run_mode`: entrées `mode: int` et sortie `CliResult`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if mode == 1 { give Ok(10) }` : cette test traite le cas d'erreur avant le calcul.
3. `if mode == 2 { give Ok(20) }` : cette test traite le cas d'erreur avant le calcul.
4. `give Err(UnknownMode(mode))` : la sortie est renvoyée immédiatement `Err(UnknownMode(mode))` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: si `mode == 1` est vrai, la sortie devient `Ok(10)`.
- Cas nominal: sans test bloquante, la branche principale renvoie `Err(UnknownMode(mode))`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: isoler le routage métier dans une fonction pure.

Le cas inconnu n'est pas aplati dans un `-1` anonyme: il conserve la valeur fautive dans `UnknownMode(mode)`. Cette conservation est précieuse pour le diagnostic, la télémétrie et les tests de non-régression: on ne sait pas seulement qu'il y a erreur, on sait laquelle.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `run_mode(1)` retourne `Ok(10)`, `run_mode(2)` retourne `Ok(20)`, `run_mode(9)` retourne `Err(UnknownMode(9))`.

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
1. `proc to_exit_code(r: CliResult) -> int {` : la règle est fixé pour `to_exit_code`: entrées `r: CliResult` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `match r {` : cette ligne démarre un dispatch stable sur `r`: une seule branche sera choisie selon la forme de la valeur analysée.
3. `case Ok(_) { give 0 }` : ce cas décrit `Ok(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
4. `case Err(MissingArgs) { give 64 }` : ce cas décrit `Err(MissingArgs)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
5. `case Err(UnknownMode(_)) { give 65 }` : ce cas décrit `Err(UnknownMode(_))` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
6. `otherwise { give 70 }` : cette ligne définit un chemin de secours explicite.
7. `}` : cette accolade ferme le bloc logique.
8. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie stable.
- Observation testable: forcer le cas `Ok(_)` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: séparer la sémantique métier de la politique d'exit code.

La conversion vers les exit-codes est un sujet d'intégration système, pas un sujet métier. La sortir dans une fonction dédiée vous donne une frontière claire: vous pouvez faire évoluer la sémantique interne sans casser les conventions du shell.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, un succès donne `0`, une arité manquante donne `64`, un mode inconnu donne `65`. Le `otherwise` garantit un filet de sécurité avec `70`.

Erreurs fréquentes à éviter:
- mélanger règles métier et règles de projection système.
- réécrire les mêmes correspondances d'exit code dans plusieurs fonctions.
- laisser des cas d'erreur sans code de sortie explicite.

## 21.5 Orchestrer dans `entry` sans réinventer la logique

```vit
// Point d'entree du scenario
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
1. `entry main at core/app {` : cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre.
2. `let argc: int = 2` : cette ligne crée la variable `argc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `let parse_res: CliResult = parse_arg_count(argc)` : cette ligne crée la variable `parse_res` de type `CliResult` pour nommer explicitement une étape intermédiaire du raisonnement.
4. `match parse_res {` : cette ligne démarre un dispatch stable sur `parse_res`: une seule branche sera choisie selon la forme de la valeur analysée.
5. `case Err(e) {` : ce cas décrit `Err(e)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
6. `return to_exit_code(Err(e))` : cette ligne termine l'exécution du bloc courant avec le code `to_exit_code(Err(e))`, utile pour observer le résultat global du scénario.
7. `}` : cette accolade ferme le bloc logique.
8. `case Ok(_) {` : ce cas décrit `Ok(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
9. `let run_res: CliResult = run_mode(1)` : cette ligne crée la variable `run_res` de type `CliResult` pour nommer explicitement une étape intermédiaire du raisonnement.
10. `return to_exit_code(run_res)` : cette ligne termine l'exécution du bloc courant avec le code `to_exit_code(run_res)`, utile pour observer le résultat global du scénario.
11. `}` : cette accolade ferme le bloc logique.
12. `otherwise {` : cette ligne définit une étape explicite du flux.
13. `return 70` : cette ligne termine l'exécution du bloc courant avec le code `70`, utile pour observer le résultat global du scénario.
14. `}` : cette accolade clôt le bloc logique.
15. `}` : cette accolade ferme le bloc logique.
16. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return to_exit_code(Err(e))`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `to_exit_code(Err(e))`.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: `main` orchestre, il ne redéfinit pas les règles métier.

Cette sobriété est une force structurelle: en lisant l'entrée du programme, vous comprenez immédiatement l'ordre du flux.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- scénario nominal `argc=2`, `mode=1`: flux `parse -> run -> to_exit_code`, sortie `0`.
- scénario invalide `argc=1`: échec immédiat de validation, sortie `64`.

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

Un CLI Vitte robuste n'est pas celui qui contient beaucoup de code, mais celui qui expose une grammaire claire de décisions. Entrée validée en frontière, métier routé dans des fonctions pures, projection système centralisée, et `entry` réduite au rôle d'orchestrateur. Avec cette discipline, la complexité augmente sans détruire la lisibilité. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, règle explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez tracer le flux complet d'un appel CLI en quelques étapes.
- vous savez modifier une règle de validation sans toucher au routage.
- vous savez faire évoluer les exit codes sans toucher à la logique métier.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: un test explicite ou un chemin de secours stable doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de test pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez le test ou la branche de secours réellement utilisée.

## Mini défi transverse

Défi: combinez au moins deux notions des trois derniers chapitres dans une fonction courte (test + transformation + sortie).
Vérification minimale: montrez un cas nominal et un cas invalide, puis expliquez quelle branche est prise.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/chapters/keywords/at.md`.
- `docs/book/chapters/keywords/case.md`.
- `docs/book/chapters/keywords/continue.md`.
- `docs/book/chapters/keywords/entry.md`.
- `docs/book/chapters/keywords/give.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle test évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **projet guide cli**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

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

Lecture ligne par ligne:
1. `pick CliError {` -> participe au déroulé du traitement.
2. `case MissingArgs` -> participe au déroulé du traitement.
3. `case UnknownMode(mode: int)` -> participe au déroulé du traitement.
4. `}` -> participe au déroulé du traitement.
5. `pick CliResult {` -> participe au déroulé du traitement.
6. `case Ok(code: int)` -> participe au déroulé du traitement.
7. `case Err(e: CliError)` -> participe au déroulé du traitement.
8. `}` -> participe au déroulé du traitement.

### Exemple B: variante cas d'erreur (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant un test explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter un test avant cette ligne.
3. Vérifier la nouvelle sortie sur une entrée limite.

### Exemple C: bug reproductible puis correction locale

Procédure:
1. Introduire une incompatibilité de type sur un appel.
2. Compiler et lire le premier diagnostic.
3. Corriger une seule ligne (pas de refactor global).
4. Recompiler et vérifier le retour nominal.

### Résultat attendu

- Le lecteur comprend ce que fait le code sans abstraction inutile.
- Chaque exemple est relié à une action concrète.
- La correction est reproductible et testable.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->


## Exemple Étendu


```vit
// Scenario projet cli: execution complete et verifiable
space demo/projet-cli

form Request { id: int amount: int quota: int }
pick Result { case Accepted(total: int) case Rejected(code: int) }

// Entrée applicative: validation des invariants de requête
proc parse_request(r: Request) -> Result {

  if r.id <= 0 { give Result.Rejected(91) }

  if r.quota < 0 { give Result.Rejected(92) }

  if r.amount < 0 { give Result.Rejected(93) }

  give Result.Accepted(r.amount)
}

// Politique métier: applique les règles de décision
proc apply_policy(total: int, quota: int) -> Result {
  let capped: int = total
  if capped > quota { set capped = quota }

  if capped < 5 { give Result.Rejected(94) }

  give Result.Accepted(capped)
}

// Persistance simulée: matérialise un résultat sans I/O réel
proc persist_sim(x: Result) -> Result {

  match x {
    case Accepted(v) {
      if v % 13 == 0 { give Result.Rejected(95) }

      give Result.Accepted(v)
    }
    case Rejected(c) { give Result.Rejected(c) }
    otherwise { give Result.Rejected(70) }
  }
}

// Conversion finale vers un code de sortie
proc to_exit(x: Result) -> int {

  match x {
    case Accepted(_) { give 0 }
    case Rejected(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let req: Request = Request(7, 12, 15)
  let p: Result = parse_request(req)
  let d: Result = apply_policy(12, req.quota)
  let s: Result = persist_sim(d)
  let _probe: int = to_exit(p)

  return to_exit(s)
}
```

## Explication détaillée du gros bloc

Ici, l'objectif est de comprendre le chemin réel du programme, ligne par ligne, jusqu'au code de sortie.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `parse_request`: lit `r: Request` et renvoie `Result`.
- `apply_policy`: lit `total: int, quota: int` et renvoie `Result`.
- `persist_sim`: lit `x: Result` et renvoie `Result`.
- `to_exit`: lit `x: Result` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `parse_request` est appelé pour traiter l'étape suivante.
3. `apply_policy` est appelé pour traiter l'étape suivante.
4. `persist_sim` est appelé pour traiter l'étape suivante.
5. `to_exit` est appelé pour traiter l'étape suivante.
6. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `r.id <= 0`.
- Test évalué: `r.quota < 0`.
- Test évalué: `r.amount < 0`.
- Test évalué: `capped > quota`.
- Test évalué: `capped < 5`.
- Test évalué: `v % 13 == 0`.
- Sélection par `match x`: le chemin dépend de l'état reçu.
- Sélection par `match x`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> parse_request -> apply_policy -> persist_sim -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> parse_request renvoie un code d'erreur -> sortie non nulle`.

### 5. Pourquoi ce découpage est utile
- Vous testez chaque fonction seule, puis le flux complet.
- Vous savez où modifier une règle sans casser tout le programme.
- Vous pouvez expliquer la sortie en suivant simplement les appels.

### 6. Vérification rapide
1. Relancer avec une entrée normale et noter la sortie.
2. Relancer avec une entrée invalide et vérifier le code d'erreur.
3. Confirmer que la même entrée donne toujours la même sortie.


## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les tests sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas d'erreur réel:
- Entree degradee ou incomplete: le test doit couper le flux tot avec une sortie explicite.

A tester:
- Requête nominale -> sortie 0.
- Entrée invalide id<=0 -> sortie 91.
- Refus métier valeur<5 -> sortie 94.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `parse_request` lit `r: Request` puis renvoie `Result`.
- `apply_policy` lit `total: int, quota: int` puis renvoie `Result`.
- `persist_sim` lit `x: Result` puis renvoie `Result`.
- `to_exit` lit `x: Result` puis renvoie `int`.

- Variables créées (valeur initiale):
- `capped: int` démarre avec `total`.
- `req: Request` démarre avec `Request(7, 12, 15)`.
- `p: Result` démarre avec `parse_request(req)`.
- `d: Result` démarre avec `apply_policy(12, req.quota)`.
- `s: Result` démarre avec `persist_sim(d)`.
- `_probe: int` démarre avec `to_exit(p)`.

- Variables modifiées pendant le traitement:
- `capped` est mis à jour avec `quota`.

- Conditions qui changent le chemin:
- si `r.id <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `r.quota < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `r.amount < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `capped > quota` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `capped < 5` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `v % 13 == 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: capped=total -> req=Request(7, 12, 15) -> p=parse_request(req) -> d=apply_policy(12, req.quota)
- enchaînement: parse_request -> apply_policy -> persist_sim -> to_exit
- sortie finale sur ce chemin: `to_exit(s)`.

- Trace d'erreur (valeurs exemple):
- si `r.id <= 0` devient vrai, la fonction renvoie immédiatement `Result.Rejected(91)`.

- Vérification rapide:
- relancer avec une entrée normale et noter la sortie,
- relancer avec une entrée invalide et noter le code d'erreur,
- confirmer qu'une même entrée produit toujours la même sortie.

## Trade-offs

| Contrainte | Option A | Option B | Décision recommandée |
| --- | --- | --- | --- |
| Lisibilité prioritaire | Branches explicites | Code compact | A si l'équipe maintient le code longtemps |
| Perf critique | Spécialisation ciblée | Généralisation | A si profiling confirme le gain |
| Évolution rapide | Contrats stricts | Conventions implicites | A pour réduire les régressions |


## Décision Selon Contrainte

- Si la contrainte dominante est la sûreté: valider tôt, échouer explicitement.
- Si la contrainte dominante est la latence: mesurer d'abord, optimiser ensuite.
- Si la contrainte dominante est l'évolutivité: isoler orchestration, décisions et conversion de sortie.


## Diagnostic Rapide

| Symptôme | Cause probable | Vérification | Correction |
| --- | --- | --- | --- |
| Sortie inattendue | Test absente ou mal ordonnée | Rejouer avec cas d'erreur | Remonter le test avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Règle implicite | Comparer nominal vs limite | Formaliser la règle dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas d'erreur réel et prévoir sa sortie,
- identifier où ajouter un test sans casser le nominal.


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: tests d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
