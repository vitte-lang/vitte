# 72. Projet embarque (contraintes mémoire/temps)

Niveau: Avancé

Prérequis: `book/chapters/25-projet-arduino.md`, `book/chapters/45-performance-allocations-copies.md`.
Voir aussi: à définir.

## Problème Concret

Situation réelle: dans ce chapitre sur Projet embarque (contraintes mémoire/temps), l'échec vient souvent d'une décision mal ordonnée plutôt que d'une faute de syntaxe. On suit donc le flux exact: entrée, test, branche, sortie.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: chaque section reprend le même scénario pour isoler une seule décision technique à la fois.
Objectif pédagogique: passer de la lecture passive à la preuve: même entrée, même branche, même sortie attendue.

## Objectif

Livrer un projet embarque respectant budget mémoire et contraintes de latence.

## Axes

1. Profil mémoire strict.
2. Boucles temps critique deterministes.
3. Gestion defensive des erreurs I/O.
4. Vérification sur cible reelle.

## Documentation a produire

1. Budget mémoire (statique/dynamique) par module.
2. Budget temps par boucle critique.
3. Matrice des modes de panne et comportement attendu.
4. Procedure de vérification sur cible (pas uniquement emulateur).
5. Contraintes hardware minimales et versions supportees.

## Budgets cibles (exemple)

| Composant | RAM max | Flash max | Latence cible |
| --- | --- | --- | --- |
| Acquisition capteur | 8 KB | 24 KB | <= 2 ms |
| Traitement signal | 16 KB | 48 KB | <= 5 ms |
| Communication serie | 6 KB | 20 KB | <= 3 ms |
| Boucle complete | 32 KB | 96 KB | <= 10 ms |

## Criteres de rejet build

1. Build KO si RAM estimee > budget global.
2. Build KO si Flash estimee > budget global.
3. Build KO si latence p95 > cible sur banc de test.
4. Build KO si une vérification cible reelle manque.

## Exemples progressifs (N1 -> N3)

### N1 (base): boucle capteur simple

```vit
proc sample_once(v: int) -> int {
  give v
}
```

Objectif:
1. Chemin nominal minimal, sans allocation dynamique.

### N2 (intermediaire): test temps/reponse

```vit
proc read_checked(ok: bool, value: int) -> int {

  if not ok { give -1 }

  give value
}
```

Objectif:
1. Rendre explicite le chemin d'echec capteur.

### N3 (avance): budget + controle système

```text
- RAM cible: <= 32 KB
- Flash cible: <= 96 KB
- boucle complete p95: <= 10 ms
- vérification sur carte reelle obligatoire
```

Objectif:
1. Verifier contraintes mémoire/latence avant release.

### Anti-exemple (invalide production embarquee)

```text
- allocations non bornees en boucle
- aucune mesure p95
- validation uniquement sur emulateur
```

Pourquoi c'est un probleme:
1. Risque de depassement mémoire.
2. Risque de latence non deterministe.

## Validation rapide

1. Verifier N1 (pas d'allocation superflue).
2. Verifier N2 (cas echec couvre).
3. Verifier N3 (budgets et mesures respectes).

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

Thème: **projet embarque (contraintes mémoire/temps)**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc sample_once(v: int) -> int {
  give v
}
```

Lecture ligne par ligne:
1. `proc sample_once(v: int) -> int {` -> pose une règle clair de fonction.
2. `give v` -> renvoie la sortie vérifiable.
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
// Scenario projet embarque contraintes: execution complete et verifiable
space demo/projet-embarque-contraintes

form Telemetry { temp_c: int volts_mv: int seq: int }
pick Decision { case Keep case Cool(level: int) case Fault(code: int) }

proc read_sensor(step: int) -> Telemetry {
  let t: int = 30 + (step % 10)
  let v: int = 3300 - (step % 30)

  give Telemetry(t, v, step)
}

proc validate(t: Telemetry) -> int {

  if t.volts_mv < 3000 { give 61 }

  if t.temp_c < -20 { give 62 }

  if t.temp_c > 120 { give 63 }

  give 0
}

proc control(t: Telemetry) -> Decision {
  let v: int = validate(t)

  if v != 0 { give Decision.Fault(v) }

  if t.temp_c >= 36 { give Decision.Cool(2) }

  if t.temp_c >= 33 { give Decision.Cool(1) }

  give Decision.Keep
}

// Conversion finale vers un code de sortie
proc to_exit(d: Decision) -> int {

  match d {
    case Keep { give 0 }
    case Cool(_) { give 0 }
    case Fault(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let t: Telemetry = read_sensor(9)
  let d: Decision = control(t)

  return to_exit(d)
}
```

## Explication détaillée du gros bloc

Vous lisez ce gros bloc comme un scénario complet: préparation des données, traitement, puis sortie finale.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `read_sensor`: lit `step: int` et renvoie `Telemetry`.
- `validate`: lit `t: Telemetry` et renvoie `int`.
- `control`: lit `t: Telemetry` et renvoie `Decision`.
- `to_exit`: lit `d: Decision` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `read_sensor` est appelé pour traiter l'étape suivante.
3. `control` est appelé pour traiter l'étape suivante.
4. `to_exit` est appelé pour traiter l'étape suivante.
5. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `t.volts_mv < 3000`.
- Test évalué: `t.temp_c < -20`.
- Test évalué: `t.temp_c > 120`.
- Test évalué: `v != 0`.
- Test évalué: `t.temp_c >= 36`.
- Test évalué: `t.temp_c >= 33`.
- Sélection par `match d`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> read_sensor -> control -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> read_sensor renvoie un code d'erreur -> sortie non nulle`.

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
- Temp nominale -> sortie 0.
- Sous-tension -> sortie 61.
- Température hors bornes -> sortie 62 ou 63.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `read_sensor` lit `step: int` puis renvoie `Telemetry`.
- `validate` lit `t: Telemetry` puis renvoie `int`.
- `control` lit `t: Telemetry` puis renvoie `Decision`.
- `to_exit` lit `d: Decision` puis renvoie `int`.

- Variables créées (valeur initiale):
- `t: int` démarre avec `30 + (step % 10)`.
- `v: int` démarre avec `3300 - (step % 30)`.
- `v: int` démarre avec `validate(t)`.
- `t: Telemetry` démarre avec `read_sensor(9)`.
- `d: Decision` démarre avec `control(t)`.

- Conditions qui changent le chemin:
- si `t.volts_mv < 3000` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `t.temp_c < -20` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `t.temp_c > 120` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `v != 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `t.temp_c >= 36` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `t.temp_c >= 33` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: t=30 + (step % 10) -> v=3300 - (step % 30) -> v=validate(t) -> t=read_sensor(9)
- enchaînement: read_sensor -> control -> to_exit
- sortie finale sur ce chemin: `to_exit(d)`.

- Trace d'erreur (valeurs exemple):
- si `t.volts_mv < 3000` devient vrai, la fonction renvoie immédiatement `61`.

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
