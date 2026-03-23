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

<!-- AUTO_EXPANSION_V1 START -->

## Approfondissement concret (sans répétition)

### 1. Snippet de référence

```vit
proc parse_id_v1(s: string) -> int { give 0 }
proc parse_id_v2(s: string) -> i64 { give 0 }
```

### 2. Lecture du code ligne par ligne

1. `proc parse_id_v1(s: string) -> int { give 0 }` -> déclare un contrat clair entre entrées et sortie.
2. `proc parse_id_v2(s: string) -> i64 { give 0 }` -> déclare un contrat clair entre entrées et sortie.

### 3. Exécution réelle (entrée -> traitement -> sortie)

1. Entrée: préciser les valeurs acceptées et refusées.
2. Traitement: suivre le chemin nominal, puis la première garde.
3. Sortie: vérifier la valeur retournée ou l'erreur attendue.

### 4. Cas limite et erreur volontaire

- Cas limite: forcer la garde et confirmer la sortie de secours.
- Cas erreur: injecter un type inattendu et lire le diagnostic exact.
- Correction: modifier une seule ligne, recompiler, valider.

### 5. Refactor concret à faible risque

Méthode: garder la signature, simplifier une branche, et prouver que le comportement reste identique avec un test nominal + un test limite.

### 6. Série de scénarios représentatifs

Cas 1: pour **versionnement et migration de code**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 2: pour **versionnement et migration de code**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 3: pour **versionnement et migration de code**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 4: pour **versionnement et migration de code**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 5: pour **versionnement et migration de code**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 6: pour **versionnement et migration de code**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 7: pour **versionnement et migration de code**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 8: pour **versionnement et migration de code**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 9: pour **versionnement et migration de code**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 10: pour **versionnement et migration de code**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 11: pour **versionnement et migration de code**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 12: pour **versionnement et migration de code**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 13: pour **versionnement et migration de code**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 14: pour **versionnement et migration de code**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 15: pour **versionnement et migration de code**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 16: pour **versionnement et migration de code**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 17: pour **versionnement et migration de code**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 18: pour **versionnement et migration de code**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 19: pour **versionnement et migration de code**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 20: pour **versionnement et migration de code**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 21: pour **versionnement et migration de code**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 22: pour **versionnement et migration de code**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 23: pour **versionnement et migration de code**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 24: pour **versionnement et migration de code**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 25: pour **versionnement et migration de code**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 26: pour **versionnement et migration de code**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 27: pour **versionnement et migration de code**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 28: pour **versionnement et migration de code**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 29: pour **versionnement et migration de code**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 30: pour **versionnement et migration de code**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 31: pour **versionnement et migration de code**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 32: pour **versionnement et migration de code**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 33: pour **versionnement et migration de code**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 34: pour **versionnement et migration de code**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 35: pour **versionnement et migration de code**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 36: pour **versionnement et migration de code**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 37: pour **versionnement et migration de code**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 38: pour **versionnement et migration de code**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 39: pour **versionnement et migration de code**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 40: pour **versionnement et migration de code**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 41: pour **versionnement et migration de code**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 42: pour **versionnement et migration de code**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 43: pour **versionnement et migration de code**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 44: pour **versionnement et migration de code**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 45: pour **versionnement et migration de code**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 46: pour **versionnement et migration de code**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 47: pour **versionnement et migration de code**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 48: pour **versionnement et migration de code**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 49: pour **versionnement et migration de code**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 50: pour **versionnement et migration de code**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 51: pour **versionnement et migration de code**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 52: pour **versionnement et migration de code**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 53: pour **versionnement et migration de code**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 54: pour **versionnement et migration de code**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 55: pour **versionnement et migration de code**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 56: pour **versionnement et migration de code**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 57: pour **versionnement et migration de code**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 58: pour **versionnement et migration de code**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 59: pour **versionnement et migration de code**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 60: pour **versionnement et migration de code**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 61: pour **versionnement et migration de code**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 62: pour **versionnement et migration de code**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 63: pour **versionnement et migration de code**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 64: pour **versionnement et migration de code**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.

### 7. Checklist finale de compréhension

1. Le contrat d'entrée est explicite.
2. Le cas nominal est testable sans ambiguïté.
3. Le cas limite est traité explicitement.
4. Le diagnostic d'erreur est actionnable.
5. Le corrigé suit une modification locale et vérifiable.

<!-- AUTO_EXPANSION_V1 END -->

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
