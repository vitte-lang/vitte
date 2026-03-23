# 72. Projet embarque (contraintes mémoire/temps)

Niveau: Avancé

Prérequis: `book/chapters/25-projet-arduino.md`, `book/chapters/45-performance-allocations-copies.md`.
Voir aussi: à définir.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

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

### N2 (intermediaire): garde temps/reponse

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
2. Introduire un cas limite.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte le contrat, puis ajouter un test de non-régression.

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
1. `proc sample_once(v: int) -> int {` -> pose un contrat clair de fonction.
2. `give v` -> renvoie la sortie vérifiable.
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

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Temp nominale -> sortie 0.
- Sous-tension -> sortie 61.
- Température hors bornes -> sortie 62 ou 63.


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


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: gardes d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
