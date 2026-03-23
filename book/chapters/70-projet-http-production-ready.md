# 70. Projet complet HTTP production-ready

Niveau: Avancé

Prérequis: `book/chapters/22-projet-http.md`, `book/chapters/48-tracage-observabilite.md`.
Voir aussi: à définir.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Monter un service HTTP exploitable en production avec observabilité et contraintes de performance.

## Livrables

1. API stable et versionnee.
2. Traces/logs/métriques en place.
3. Tests intégration et charge de base.
4. Procedure de deploiement documentee.

## Documentation a produire

1. Contrat d'API (routes, schemas, erreurs).
2. SLO/SLA cibles et métriques associees.
3. Runbook incident (triage, rollback, vérification).
4. Guide de configuration par environnement.
5. Politique de versionnement et deprecation API.

## Incident type (timeline)

1. Detection (T+0 min): alerte `5xx` > seuil pendant 5 min.
2. Triage (T+5 min): verifier derniere release, endpoint impacte, logs corrélés.
3. Mitigation/Rollback (T+10 min): rollback version N-1 ou desactivation flag.
4. Stabilisation (T+20 min): confirmer retour SLO et absence d'effets de bord.
5. Postmortem (T+24h): cause racine, actions correctives, owner, date cible.

## Exemple de runbook incident

```text
Nom: API latency spike /v1/login
Severite: SEV-2

1) Confirmer symptome
- Dashboard p95 latence
- Taux erreurs 5xx

2) Isoler zone
- Version deployée
- Endpoint touche
- Dependances externes

3) Action immediate
- Rollback vers release precedente
- Purge cache si necessaire

4) Validation
- p95 revenu sous seuil
- taux 5xx revenu nominal

5) Cloture
- Ticket postmortem
- Correctif permanent planifie
```

## Exemples progressifs (N1 -> N3)

### N1 (base): endpoint health

Snippet Vitte:

```vit
proc health() -> int {
  return 200
}
```

Runbook rapide:

```bash
curl -i http://localhost:8080/health
```

### N2 (intermediaire): login avec erreur geree

Snippet Vitte:

```vit
proc login(ok: bool) -> int {
  if not ok { return 401 }

  return 200
}
```

Commandes vérification:

```bash
curl -i -X POST http://localhost:8080/v1/login
make grammar-test
```

### N3 (avance): incident + rollback

Snippet Vitte (degrade mode):

```vit
proc login_degraded() -> int {
  return 503
}
```

Runbook incident:

```bash
# 1. confirmer symptome
# 2. rollback release N-1
# 3. verifier retour SLO
```

### Anti-exemple

```text
Pas de route health, pas de code erreur stable, pas de rollback.
```

## Validation rapide

1. N1: health check repond.
2. N2: codes 200/401 testes.
3. N3: procedure rollback executable.

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

Thème: **projet complet http production-ready**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc health() -> int {
  return 200
}
```

Lecture ligne par ligne:
1. `proc health() -> int {` -> pose un contrat clair de fonction.
2. `return 200` -> renvoie la sortie vérifiable.
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
// Scenario projet http production ready: execution complete et verifiable
space demo/projet-http-production-ready

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

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Requête nominale -> sortie 0.
- Entrée invalide id<=0 -> sortie 91.
- Refus métier valeur<5 -> sortie 94.


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


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
