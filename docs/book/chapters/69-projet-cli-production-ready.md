# 69. Projet complet CLI production-ready

Niveau: Intermédiaire

Prérequis: `docs/book/chapters/21-projet-cli.md`, `docs/book/chapters/58-pipeline-ci-projet-vitte.md`.
Voir aussi: à définir.

## Problème Concret

Situation réelle: pour Projet complet CLI production-ready, la question n'est pas 'quella règle écrire' mais 'quel chemin le code prend vraiment'. Cette lecture par exécution évite les interprétations vagues.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: vous retrouvez le même pipeline pour observer ce qui change réellement quand on modifie une branche.
Objectif pédagogique: relire un bloc, prédire la sortie, puis confirmer la prédiction avec une exécution simple et reproductible.

## Objectif

Construire une CLI robuste avec packaging, tests, diagnostics et CI.

## Livrables

1. Commandes principales et aide.
2. Gestion d'erreurs utilisateur claire.
3. Tests unitaires + intégration.
4. Pipeline CI vert.

## Exemples progressifs (N1 -> N3)

### N1 (base): commande CLI nominale

Snippet Vitte:

```vit
entry main at app/cli {
  return 0
}
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): erreur utilisateur contrôlée

Snippet Vitte:

```vit
proc parse_arg(ok: bool) -> int {

  if not ok { give 2 }

  give 0
}
```

Commande:

```bash
make grammar-test
```

### N3 (avancé): exécution complète CI

Snippet Vitte:

```vit
proc run_cli(ok: bool) -> int {

  if not ok { give 2 }

  give 0
}
entry main at app/cli { return run_cli(true) }
```

Commandes:

```bash
make grammar-gate
make book-qa
```

### Anti-exemple

```text
CLI sans code de sortie stable, sans tests de cas d'erreur, sans validation CI.
```

## Validation rapide

1. Vérifier codes de sortie nominal/erreur.
2. Vérifier commandes CI vertes.
3. Vérifier documentation utilisateur minimale.

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

Thème: **projet complet cli production-ready**. Cette section évite les généralités et part d'un extrait réel.

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
// Scenario projet cli production ready: execution complete et verifiable
space demo/projet-cli-production-ready

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

Vous lisez ce gros bloc comme un scénario complet: préparation des données, traitement, puis sortie finale.

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
