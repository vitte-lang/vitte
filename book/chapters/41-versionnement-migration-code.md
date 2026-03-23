# 41. Versionnement et migration de code

Niveau: Avance

Prérequis: `book/chapters/40-compatibilite-breaking.md`.
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

Thème: **versionnement et migration de code**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc parse_id_v1(s: string) -> int { give 0 }
proc parse_id_v2(s: string) -> i64 { give 0 }
```

Lecture ligne par ligne:
1. `proc parse_id_v1(s: string) -> int { give 0 }` -> pose un contrat clair de fonction.
2. `proc parse_id_v2(s: string) -> i64 { give 0 }` -> pose un contrat clair de fonction.

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
