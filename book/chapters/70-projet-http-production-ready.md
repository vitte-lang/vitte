# 70. Projet complet HTTP production-ready

Niveau: Avancé

Prérequis: `book/chapters/22-projet-http.md`, `book/chapters/48-tracage-observabilite.md`.
Voir aussi: à définir.

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
