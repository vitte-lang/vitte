# 50. Tests d'integration orientes scenario

Niveau: Avance

Prérequis: `docs/book/chapters/18-tests.md`, `docs/book/chapters/21-projet-cli.md`.
Voir aussi: à définir.

## Problème Concret

Situation réelle: Tests d'integration orientes scenario se comprend mieux en rejouant le programme comme un algorithme exécutable. Vous lisez les données entrantes, la condition évaluée, puis la valeur renvoyée.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: on conserve un même mini-programme pour comparer les effets d'une modification sans changer tout le contexte.
Objectif pédagogique: comprendre pourquoi une ligne existe et ce qu'elle change dans la trajectoire du programme.

## Objectif

Verifier des parcours complets utilisateur/systeme plutot que des fonctions isolees.

## Demarche

1. Definir scenario nominal et scenario echec.
2. Monter un environnement proche production.
3. Verifier sortie metier et effets de bord.
4. Nettoyer et rerendre le test idempotent.

## Checklist

1. Donnees de test versionnees.
2. Resultat attendu stable.
3. Execution possible en CI.

## Exemples progressifs (N1 -> N3)

### N1 (base): scenario nominal

```vit
entry main at app/cli {
  return 0
}
```

### N2 (intermediaire): scenario erreur

```vit
entry main at app/cli {
  return missing
}
```

### N3 (avance): scenario idempotent

```vit
proc run_once(ok: bool) -> int {

  if not ok { give 1 }

  give 0
}
```

### Anti-exemple

```vit
# scenario depend d'un etat manuel externe
```

## Validation rapide

1. Nominal + erreur couverts.
2. Sortie observable verifiee.
3. Reexecution sans effet parasite.

## Pourquoi

Cette section explicite la valeur pratique: réduire les erreurs, accélérer le diagnostic et stabiliser les évolutions.

## Test mental

Question de contrôle: si vous modifiez une hypothèse clé, quel résultat doit changer et pourquoi?

## À faire

1. Exécuter l’exemple nominal.
2. Introduire un cas d'erreur.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte la règle, puis ajouter un test de non-régression.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **tests d'integration orientes scenario**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/cli {
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/cli {` -> définit le point d'entrée du scénario.
2. `return 0` -> renvoie la sortie vérifiable.
3. `}` -> participe au déroulé du traitement.

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
// Scenario tests integration scenario: execution complete et verifiable
space demo/tests-integration-scenario

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

Ici, l'objectif est de comprendre le chemin réel du programme, ligne par ligne, jusqu'au code de sortie.

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


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
