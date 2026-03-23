# 27. Grammaire du langage

Niveau: Avancé

Prérequis: chapitre précédent `book/chapters/26-projet-editor.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/26-projet-editor.md`, `book/chapters/28-conventions.md`, `book/glossaire.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Grammaire du langage**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Grammaire du langage**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Plan recommandé (version finale)

Ce plan remplace la variante avec `try/catch` et aligne le vocabulaire sur la grammaire réelle (`select/when`, `space/pull/use/share`).

1. Vue d’ensemble du langage
2. Lexique et tokens (espaces, commentaires, identifiants, mots-clés)
3. Littéraux (nombres, chaînes, booléens, listes, etc.)
4. Expressions (priorité, associativité, parenthèses)
5. Instructions de base (blocs, affectation, expression statements)
6. Contrôle de flux (`if`, `loop`, `for`, `select/when`, `match`)
7. Fonctions et portée (`proc`, paramètres, closures)
8. Types et annotations
9. Structures de données (formes, picks/cases, listes, slices, pointeurs)
10. Modules et imports (`space`, `pull`, `use`, `share`)
11. Diagnostics et gestion des erreurs (lexicales/syntaxiques)
12. Grammaire formelle en EBNF (règles complètes)
13. Ambiguïtés et résolution (précédence, conflits LL/LR, `else`)
14. AST et mapping grammaire -> parser
15. Exemples complets commentés
16. Appendices (cheat sheet, conventions, tests de grammaire)

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Grammaire du langage**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Grammaire du langage** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Grammaire du langage**.

## Exercice court

Prenez un exemple du chapitre sur **Grammaire du langage**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Grammaire du langage**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Guide de lecture EBNF (court)

Utilisez ce chemin de lecture pour aller vite quand un parseur échoue:

1. Commencez par `program` puis `toplevel` pour savoir si la forme du fichier est valide.
2. Descendez ensuite vers `stmt` ou `expr` selon la ligne qui échoue.
3. Vérifiez la priorité opératoire dans `book/grammar/precedence.md` si l’arbre attendu diverge.
4. Reproduisez avec un exemple minimal dans `tests/grammar/valid` ou `tests/grammar/invalid`.
5. Confirmez le diagnostic attendu dans `book/grammar/diagnostics/expected`.

Voir aussi:
- `book/chapters/31-erreurs-build.md` pour la table erreur -> correction.
- `book/grammar/grammar-notes.md` pour les ambiguïtés connues.
- `docs/GENERIC_CALL_SYNTAX.md` pour la borne actuelle des appels génériques explicites.

## Diagrammes Railroad (SVG)

Les diagrammes sont générés automatiquement depuis la source EBNF synchronisée.

- Index: `book/grammar/railroad/README.md`
- Règles clés:
  - `book/grammar/railroad/program.svg`
  - `book/grammar/railroad/toplevel.svg`
  - `book/grammar/railroad/stmt.svg`
  - `book/grammar/railroad/expr.svg`
  - `book/grammar/railroad/type_expr.svg`

Commandes utiles:

```bash
make grammar-docs
make grammar-gate
bin/vitte grammar check
```

## 27.1 Construire une déclaration de procédure valide

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Lecture ligne par ligne (débutant):
1. `proc add(a: int, b: int) -> int {` : le contrat est défini pour `add`: entrées `a: int, b: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `give a + b` : la branche renvoie immédiatement `a + b` pour la branche courante, la sortie de branche est explicite et vérifiable.
3. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `a + b`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: valider la forme canonique d'une déclaration de procédure.

Lecture grammaticale:
- en-tête `proc` avec nom et paramètres typés.
- type de retour explicite `-> int`.
- bloc délimité contenant une instruction de retour.

À l'exécution, `add(1,2)=3`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 27.2 Construire une entrée programme explicite

```vit
// Point d'entree du scenario
entry main at core/app {
  return 0
}
```

Lecture ligne par ligne (débutant):
1. `entry main at core/app {` : cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre.
2. `return 0` : cette ligne termine l'exécution du bloc courant avec le code `0`, utile pour observer le résultat global du scénario.
3. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return 0`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `0`.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: rendre explicite le point d'entrée du programme.

La forme `entry.. at..` fixe à la fois le nom logique et le module de rattachement.

À l'exécution, le programme quitte immédiatement avec le code `0`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 27.3 Construire un `match` avec repli (patterns)

```vit
pick Resp { case Ok, case Err }
proc to_code(r: Resp) -> int {

  match r {
    case Ok { give 0 }
    otherwise { give 1 }
  }
}
```

Lecture ligne par ligne (débutant):
1. `pick Resp { case Ok, case Err }` cette ligne définit un type fermé pour disposer de patterns valides dans `match`.
2. `proc to_code(r: Resp) -> int {` cette ligne fixe un contrat simple: entrée variante, sortie code.
3. `match r {` cette ligne démarre le dispatch par pattern.
4. `case Ok { give 0 }` cette ligne couvre explicitement le pattern `Ok`.
5. `otherwise { give 1 }` cette ligne couvre le repli.
5. `}` : cette accolade clôt le bloc logique.
6. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: construire un `match` lisible et exhaustif, conforme aux patterns de la grammaire.

Ici, `otherwise` ferme explicitement l'arbre de décision, ce qui évite les trous de contrôle de flux.

À l'exécution, `to_code(Ok)=0` et `to_code(Err)=1`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 27.4 Lire la forme `foo[T](..)` sans surinterpréter

```vit
proc id[T](x: T) -> T {
  give x
}

proc main() -> int {
  let a = id[int](1)
  let i = 0
  let b = arr[i](1)

  give 0
}
```

Lecture grammaticale utile:
1. `id[int](1)` peut être lu comme un appel générique explicite.
2. `arr[i](1)` ne doit pas être relu trop vite comme un appel générique.
3. La même surface `foo[..](..)` porte donc une ambiguïté réelle.

Règle actuelle du parser:
- si le contenu entre crochets est lu comme une liste de types non ambigus, la forme devient un appel générique explicite
- sinon, la lecture reste `index_suffix` puis `call_suffix`

Conséquences actuelles:
- `id[int](1)` est accepté comme appel générique explicite
- `Public[int](1)` et `facade_mod.Public[int](1)` suivent la même logique
- `id[i](1)` reste une indexation suivie d'appel

Cette borne est volontaire.
Elle documente le comportement réel sans prétendre que toute forme `foo[..](..)` est déjà stabilisée.

## À retenir

Les formes syntaxiques critiques sont maîtrisées, ce qui réduit les erreurs de structure avant type-check. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez reconnaître immédiatement une forme syntaxique invalide.
- vous savez distinguer erreur grammaticale et erreur de type.
- vous pouvez relire une fonction comme une phrase de grammaire complète.

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
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.
- Postfix: la surface EBNF inclut maintenant `generic_call_suffix`, mais l'acceptation réelle reste plus étroite et dépend d'une règle d'ambiguïté documentée.

## Keywords à revoir

- `book/keywords/at.md`.
- `book/keywords/case.md`.
- `book/keywords/entry.md`.
- `book/keywords/give.md`.
- `book/keywords/int.md`.

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

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **grammaire du langage**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Lecture ligne par ligne:
1. `proc add(a: int, b: int) -> int {` -> pose un contrat clair de fonction.
2. `give a + b` -> renvoie la sortie vérifiable.
3. `}` -> participe au déroulé du traitement.

### Exemple B: variante cas limite (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant une garde explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter une garde avant cette ligne.
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
// Scenario grammaire: execution complete et verifiable
space demo/grammaire

form SourceUnit { bytes: int lines: int tokens_hint: int }
pick ParseState { case Parsed(nodes: int) case Failed(code: int) }

// Scan: transforme l'entrée brute en signal exploitable
proc scan(u: SourceUnit) -> int {

  if u.bytes <= 0 { give 0 }

  if u.lines <= 0 { give 0 }

  give (u.tokens_hint + u.lines)
}

// Parse: construit un état syntaxique déterministe
proc parse(token_count: int) -> ParseState {

  if token_count == 0 { give ParseState.Failed(101) }

  if token_count < 4 { give ParseState.Failed(102) }

  give ParseState.Parsed(token_count)
}

proc validate_structure(nodes: int) -> int {

  if nodes <= 0 { give 201 }

  if nodes > 200000 { give 202 }

  give 0
}

// Conversion finale vers un code de sortie
proc to_exit(p: ParseState) -> int {

  match p {
    case Parsed(n) {
      let v: int = validate_structure(n)

      if v != 0 { give v }

      give 0
    }
    case Failed(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let u: SourceUnit = SourceUnit(120, 12, 18)
  let t: int = scan(u)
  let p: ParseState = parse(t)

  return to_exit(p)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Unité valide -> sortie 0.
- Entrée vide (bytes=0) -> sortie 101.
- Structure surdimensionnée -> sortie 202.


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
| Sortie inattendue | Garde absente ou mal ordonnée | Rejouer avec cas limite | Remonter la garde avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Contrat implicite | Comparer nominal vs limite | Formaliser le contrat dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas limite réel et prévoir sa sortie,
- identifier où ajouter une garde sans casser le nominal.


## Pourquoi Cette Erreur Arrive En Prod

Cause fréquente: entrée partiellement valide, hypothèse implicite dans une branche, puis projection de sortie trop tardive.
Symptôme: comportement correct en nominal mais instable sous charge ou données incomplètes.
Mesure utile: tracer l'entrée effective, rejouer le cas limite, verrouiller la garde au bon niveau.


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: gardes d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
