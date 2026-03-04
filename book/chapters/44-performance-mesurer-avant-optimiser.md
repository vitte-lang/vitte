# 44. Performance: mesurer avant d'optimiser

Niveau: Intermediaire

Prerequis: `book/chapters/19-performance.md`.

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
