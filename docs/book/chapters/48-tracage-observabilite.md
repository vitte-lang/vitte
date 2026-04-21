# 48. Tracage et observabilite

Niveau: Avance

Prérequis: `docs/book/chapters/47-debugage-reproductible.md`.
Voir aussi: à définir.

## Problème Concret

Situation réelle: pour Tracage et observabilite, la question n'est pas 'quella règle écrire' mais 'quel chemin le code prend vraiment'. Cette lecture par exécution évite les interprétations vagues.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: vous retrouvez le même pipeline pour observer ce qui change réellement quand on modifie une branche.
Objectif pédagogique: relire un bloc, prédire la sortie, puis confirmer la prédiction avec une exécution simple et reproductible.

## Objectif

Rendre le comportement du programme observable en production et en test.

## Axes

1. Logs structures avec contexte minimal utile.
2. Correlation par identifiant de requete.
3. Metriques sur erreurs/latence/debit.
4. Traces sur chemins critiques.

## Checklist

1. Chaque erreur critique est tracable.
2. Les logs sont actionnables.
3. Les metriques permettent une alerte utile.

## Exemples progressifs (N1 -> N3)

### N1 (base): message contextualise

```vit
proc handle(req_id: int) -> int { give req_id }
```

### N2 (intermediaire): point d'erreur observable

```vit
proc handle_checked(ok: bool, code: int) -> int {

  if not ok { give code }

  give 0
}
```

### N3 (avance): correlation metrique/log

```vit
proc handle_trace(req_id: int, ok: bool) -> int {

  if not ok { give req_id }

  give 0
}
```

### Anti-exemple

```vit
proc handle_silent() -> int { give 0 }
```

## Validation rapide

1. Identifier contexte minimal dans logs.
2. Associer erreur a metrique.
3. Verifier exploitabilite en incident.

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

Thème: **tracage et observabilite**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc handle(req_id: int) -> int { give req_id }
```

Lecture ligne par ligne:
1. `proc handle(req_id: int) -> int { give req_id }` -> pose une règle clair de fonction.

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
// Scenario tracage observabilite: execution complete et verifiable
space demo/tracage-observabilite

form AbiEnvelope { version: int payload_size: int flags: int }
pick NativeCall { case Ok(code: int) case Err(code: int) }

proc abi_version() -> int { give 3 }

// Validation ABI: refuse toute incompatibilité avant appel natif
proc validate_abi(e: AbiEnvelope) -> int {

  if e.version != abi_version() { give 51 }

  if e.payload_size <= 0 { give 52 }

  if e.payload_size > 4096 { give 53 }

  if e.flags < 0 { give 54 }

  give 0
}

// Appel natif simulé: exécution seulement si la règle est valide
proc call_native(e: AbiEnvelope) -> NativeCall {
  let v: int = validate_abi(e)

  if v != 0 { give NativeCall.Err(v) }

  if e.payload_size % 2 == 0 { give NativeCall.Ok(0) }

  give NativeCall.Err(55)
}

// Conversion finale vers un code de sortie
proc to_exit(r: NativeCall) -> int {

  match r {
    case Ok(c) { give c }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let e: AbiEnvelope = AbiEnvelope(3, 128, 1)
  let r: NativeCall = call_native(e)

  return to_exit(r)
}
```

## Explication détaillée du gros bloc

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `abi_version`: lit `aucun paramètre` et renvoie `int`.
- `validate_abi`: lit `e: AbiEnvelope` et renvoie `int`.
- `call_native`: lit `e: AbiEnvelope` et renvoie `NativeCall`.
- `to_exit`: lit `r: NativeCall` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `call_native` est appelé pour traiter l'étape suivante.
3. `to_exit` est appelé pour traiter l'étape suivante.
4. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `e.version != abi_version()`.
- Test évalué: `e.payload_size <= 0`.
- Test évalué: `e.payload_size > 4096`.
- Test évalué: `e.flags < 0`.
- Test évalué: `v != 0`.
- Test évalué: `e.payload_size % 2 == 0`.
- Sélection par `match r`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> call_native -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> call_native renvoie un code d'erreur -> sortie non nulle`.

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
- ABI valide -> sortie 0.
- Version incompatible -> sortie 51.
- Payload hors règle -> sortie 52 ou 53.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `abi_version` lit `aucun paramètre` puis renvoie `int`.
- `validate_abi` lit `e: AbiEnvelope` puis renvoie `int`.
- `call_native` lit `e: AbiEnvelope` puis renvoie `NativeCall`.
- `to_exit` lit `r: NativeCall` puis renvoie `int`.

- Variables créées (valeur initiale):
- `v: int` démarre avec `validate_abi(e)`.
- `e: AbiEnvelope` démarre avec `AbiEnvelope(3, 128, 1)`.
- `r: NativeCall` démarre avec `call_native(e)`.

- Conditions qui changent le chemin:
- si `e.version != abi_version()` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `e.payload_size <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `e.payload_size > 4096` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `e.flags < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `v != 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `e.payload_size % 2 == 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: v=validate_abi(e) -> e=AbiEnvelope(3, 128, 1) -> r=call_native(e)
- enchaînement: call_native -> to_exit
- sortie finale sur ce chemin: `to_exit(r)`.

- Trace d'erreur (valeurs exemple):
- si `e.version != abi_version()` devient vrai, la fonction renvoie immédiatement `51`.

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
