# 44. Performance: mesurer avant d'optimiser

Niveau: Intermediaire

Prérequis: `book/chapters/19-performance.md`.
Voir aussi: à définir.

## Objectif

Eviter les optimisations speculatives en partant de mesures reproductibles.

## Demarche

1. Definir une metrique cible (latence, debit, memoire).
2. Capturer une baseline.
3. Modifier une variable a la fois.
4. Comparer avec le meme protocole.

## Checklist

1. Cas nominal et cas limite mesures.
2. Environnement de benchmark stable.
3. Resultats traces et historises.

## Exemples progressifs (N1 -> N3)

### N1 (base): baseline fonctionnelle

```vit
proc compute(x: int) -> int { give x + 1 }
```

### N2 (intermediaire): variation unique

```vit
proc compute_fast(x: int) -> int { give x + 1 }
```

### N3 (avance): protocole stable

```vit
proc benchmark_case(x: int) -> int { give compute_fast(x) }
```

### Anti-exemple

```vit
proc tweak1(x:int)->int{give x}
proc tweak2(x:int)->int{give x}
# aucune mesure comparative
```

## Validation rapide

1. Capturer baseline.
2. Changer une variable.
3. Comparer sur protocole fixe.

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

Thème: **performance: mesurer avant d'optimiser**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc compute(x: int) -> int { give x + 1 }
```

Lecture ligne par ligne:
1. `proc compute(x: int) -> int { give x + 1 }` -> pose un contrat clair de fonction.

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
