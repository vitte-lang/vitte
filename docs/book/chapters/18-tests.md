# 18. Tests et validation

Niveau: Intermédiaire

Prérequis: chapitre précédent `docs/book/chapters/17-stdlib.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/17-stdlib.md`, `docs/book/chapters/19-performance.md`, `docs/book/glossaire.md`.

## Problème Concret

Situation réelle: pour Tests et validation, la question n'est pas 'quella règle écrire' mais 'quel chemin le code prend vraiment'. Cette lecture par exécution évite les interprétations vagues.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: vous retrouvez le même pipeline pour observer ce qui change réellement quand on modifie une branche.
Objectif pédagogique: comprendre pourquoi une ligne existe et ce qu'elle change dans la trajectoire du programme.

## Objectif

Vous devez pouvoir relire un extrait, prédire son résultat, puis vérifier cette prédiction avec une exécution simple.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Tests et validation**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Tests et validation** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Tests et validation**.

## Exercice court

Prenez un exemple du chapitre sur **Tests et validation**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Tests et validation**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 18.1 Tester une fonction de saturation

```vit
proc clamp(x: int, lo: int, hi: int) -> int {

  if x < lo { give lo }

  if x > hi { give hi }

  give x
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Condition évaluée en premier: `x < lo`. Si elle est vraie, le chemin de test est exécuté immédiatement.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `lo`.
5. Notion intermédiaire: une fonction est une transformation `Entrée -> Sortie`; sa règle sert à limiter les ambiguïtés.
6. Notion intermédiaire: un invariant est une propriété qui reste vraie pendant la boucle ou pendant un pipeline d'appels.
Vérification rapide: testez une entrée nominale puis une entrée limite, et comparez les deux sorties obtenues.


L'intention de cette étape est directe: couvrir systématiquement les trois classes de cas d'une règle borné.

Cette grille simple évite les tests "au hasard": on testé sous la borne, dans l'intervalle, au-dessus de la borne.

À l'exécution:
- `clamp(-1,0,10)` retourne `0`.
- `clamp(5,0,10)` retourne `5`.
- `clamp(99,0,10)` retourne `10`.

Erreurs classiques à éviter:
- tester uniquement le cas nominal et ignorer les frontières.
- confondre le symptôme observé et la cause réelle.
- traiter les erreurs dans tous les sens au lieu de centraliser la politique.

## 18.2 Verrouiller les frontières d'un parseur

```vit
proc parse_port(x: int) -> int {

  if x < 0 { give -1 }

  if x > 65535 { give -1 }

  give x
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Condition évaluée en premier: `x < 0`. Si elle est vraie, le chemin de test est exécuté immédiatement.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `-1`.
5. Notion intermédiaire: une fonction est une transformation `Entrée -> Sortie`; sa règle sert à limiter les ambiguïtés.
6. Notion intermédiaire: un invariant est une propriété qui reste vraie pendant la boucle ou pendant un pipeline d'appels.
Vérification rapide: tracez une exécution avec des valeurs concrètes (`x=...`, `i=...`) pour confirmer la branche réellement prise.


L'intention de cette étape est directe: verrouiller les frontières du domaine de validité.

Le choix des valeurs de test est volontaire:
- juste en dessous de la borne basse.
- exactement sur les bornes.
- juste au-dessus de la borne haute.

À l'exécution:
- `parse_port(0)` retourne `0`.
- `parse_port(65535)` retourne `65535`.
- `parse_port(65536)` retourne `-1`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 18.3 Capturer une regression sur division

```vit
proc non_reg_demo(x: int) -> int {

  if x == 0 { give 0 }

  give 10 / x
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Condition évaluée en premier: `x == 0`. Si elle est vraie, le chemin de test est exécuté immédiatement.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `0`.
5. Notion intermédiaire: une fonction est une transformation `Entrée -> Sortie`; sa règle sert à limiter les ambiguïtés.
6. Notion intermédiaire: un invariant est une propriété qui reste vraie pendant la boucle ou pendant un pipeline d'appels.
Vérification rapide: testez une entrée nominale puis une entrée limite, et comparez les deux sorties obtenues.


L'intention de cette étape est directe: capturer explicitement un bug déjà rencontré pour empêcher son retour.

Ce test sert de mémoire technique: il documente un risque réel et verrouille la correction.

À l'exécution:
- `non_reg_demo(0)` retourne `0`.
- `non_reg_demo(2)` retourne `5`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Vos tests couvrent nominal, bornes et historiques de bugs avec des attentes explicites. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, règle explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez justifier chaque cas de test par une règle de la règle.
- vous savez distinguer test de frontière et test de non-régression.
- vous pouvez identifier rapidement quel invariant échoue quand un test casse.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Repère: un test explicite ou un chemin de secours stable doit s'appliquer.
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

- `docs/book/chapters/keywords/const.md`.
- `docs/book/chapters/keywords/continue.md`.
- `docs/book/chapters/keywords/give.md`.
- `docs/book/chapters/keywords/if.md`.
- `docs/book/chapters/keywords/int.md`.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle test évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?


## Exemple Étendu


```vit
// Scenario tests: execution complete et verifiable
space demo/tests

form TestCase { id: int input: int expected: int }
pick CaseResult { case Pass(id: int) case Fail(id: int, got: int, expected: int) }

proc subject(x: int) -> int {

  if x < 0 { give 0 }

  if x > 100 { give 100 }

  give x
}

// Exécute un cas de test et retourne un résultat typé
proc run(c: TestCase) -> CaseResult {
  let got: int = subject(c.input)

  if got == c.expected { give CaseResult.Pass(c.id) }

  give CaseResult.Fail(c.id, got, c.expected)
}

// Agrège les résultats et projette un code CI
proc ci_exit(a: CaseResult, b: CaseResult, c: CaseResult) -> int {
  let ok: int = 0

  match a { case Pass(_) { set ok = ok + 1 } otherwise { } }

  match b { case Pass(_) { set ok = ok + 1 } otherwise { } }

  match c { case Pass(_) { set ok = ok + 1 } otherwise { } }

  if ok == 3 { give 0 }

  if ok == 2 { give 11 }

  if ok == 1 { give 12 }

  give 13
}

// Point d'entree du scenario
entry main at core/app {
  let c1: TestCase = TestCase(1, -3, 0)
  let c2: TestCase = TestCase(2, 42, 42)
  let c3: TestCase = TestCase(3, 160, 100)
  let r1: CaseResult = run(c1)
  let r2: CaseResult = run(c2)
  let r3: CaseResult = run(c3)

  return ci_exit(r1, r2, r3)
}
```

## Explication détaillée du gros bloc

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `subject`: lit `x: int` et renvoie `int`.
- `run`: lit `c: TestCase` et renvoie `CaseResult`.
- `ci_exit`: lit `a: CaseResult, b: CaseResult, c: CaseResult` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `run` est appelé pour traiter l'étape suivante.
3. `ci_exit` est appelé pour traiter l'étape suivante.
4. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `x < 0`.
- Test évalué: `x > 100`.
- Test évalué: `got == c.expected`.
- Test évalué: `ok == 3`.
- Test évalué: `ok == 2`.
- Test évalué: `ok == 1`.
- Sélection par `match a`: le chemin dépend de l'état reçu.
- Sélection par `match b`: le chemin dépend de l'état reçu.
- Sélection par `match c`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> run -> ci_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> run renvoie un code d'erreur -> sortie non nulle`.

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
- 3 sur 3 réussis -> sortie 0.
- Régression partielle -> sortie 11.
- Échec global -> sortie 13.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `subject` lit `x: int` puis renvoie `int`.
- `run` lit `c: TestCase` puis renvoie `CaseResult`.
- `ci_exit` lit `a: CaseResult, b: CaseResult, c: CaseResult` puis renvoie `int`.

- Variables créées (valeur initiale):
- `got: int` démarre avec `subject(c.input)`.
- `ok: int` démarre avec `0`.
- `c1: TestCase` démarre avec `TestCase(1, -3, 0)`.
- `c2: TestCase` démarre avec `TestCase(2, 42, 42)`.
- `c3: TestCase` démarre avec `TestCase(3, 160, 100)`.
- `r1: CaseResult` démarre avec `run(c1)`.
- `r2: CaseResult` démarre avec `run(c2)`.
- `r3: CaseResult` démarre avec `run(c3)`.

- Variables modifiées pendant le traitement:
- `ok` est mis à jour avec `ok + 1 otherwise {`.
- `ok` est mis à jour avec `ok + 1 otherwise {`.
- `ok` est mis à jour avec `ok + 1 otherwise {`.

- Conditions qui changent le chemin:
- si `x < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `x > 100` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `got == c.expected` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `ok == 3` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `ok == 2` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `ok == 1` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: got=subject(c.input) -> ok=0 -> c1=TestCase(1, -3, 0) -> c2=TestCase(2, 42, 42)
- enchaînement: run -> ci_exit
- sortie finale sur ce chemin: `ci_exit(r1, r2, r3)`.

- Trace d'erreur (valeurs exemple):
- si `x < 0` devient vrai, la fonction renvoie immédiatement `0`.

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


## Pourquoi Cette Erreur Arrive En Prod

Cause fréquente: entrée partiellement valide, hypothèse implicite dans une branche, puis projection de sortie trop tardive.
Symptôme: comportement correct en nominal mais instable sous charge ou données incomplètes.
Mesure utile: tracer l'entrée effective, rejouer le cas d'erreur, verrouiller le test au bon niveau.


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: tests d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
