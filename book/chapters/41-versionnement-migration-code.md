# 41. Versionnement et migration de code

Niveau: Avance

Prerequis: `book/chapters/40-compatibilite-breaking.md`.
Voir aussi: `book/packages-migration-plan.md`, `book/packages-migration-map.md`.

## Objectif

Piloter les migrations avec un plan reproductible: preparation, transition, verification, nettoyage.

## Plan de migration

1. Inventorier les usages de l'API cible.
2. Introduire une couche de compatibilite temporaire.
3. Migrer module par module.
4. Supprimer la compatibilite en fin de cycle.

## Exemple de lot

1. Sprint 1: API nouvelle + deprecation ancienne.
2. Sprint 2: migration 50% des modules.
3. Sprint 3: migration complete + suppression anciens points d'entree.

## Risques frequents

- Migration big-bang sans phase intermediaire.
- Absence de metriques de progression.
- Oubli des tests d'integration.

## Checklist

1. Carte de migration maintenue a jour.
2. CI verte a chaque etape.
3. Date de fin de compatibilite fixee.


## Exemples progressifs (N1 -> N3)

### N1 (base): ancien + nouveau contrat

```vit
proc parse_id_v1(s: string) -> int { give 0 }
proc parse_id_v2(s: string) -> i64 { give 0 }
```

### N2 (intermediaire): compat temporaire

```vit
proc parse_id(s: string) -> int { give parse_id_v1(s) }
```

### N3 (avance): retrait cible

```vit
proc parse_id(s: string) -> i64 { give parse_id_v2(s) }
```

### Anti-exemple

```vit
proc parse_id(s: string) -> i64 { give 0 }
# sans phase de transition
```

## Validation rapide

1. Inventory call sites.
2. Migrer par lots.
3. Retirer compat en fin de cycle.
