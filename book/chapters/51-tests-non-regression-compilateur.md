# 51. Tests de non-regression compilateur

Niveau: Avance

Prérequis: `book/chapters/15-pipeline.md`, `book/chapters/31-erreurs-build.md`.
Voir aussi: à définir.

## Objectif

Eviter la reintroduction de bugs deja corriges dans parse, typecheck, backend.

## Strategie

1. Capturer chaque bug en test minimal.
2. Classer par couche de pipeline.
3. Verifier diagnostics attendus et code genere.
4. Bloquer merge si regression.

## Checklist

1. Test lie a un incident reel.
2. Reproducer minimal et stable.
3. Assertion exacte sur symptome corrige.

## Exemples progressifs (N1 -> N3)

### N1 (base): bug parse capture

```vit
emit 1
```

### N2 (intermediaire): bug type capture

```vit
entry main at app/demo {
  let x: int = "oops"
  return x
}
```

### N3 (avance): regression verrouillee

```vit
entry main at app/demo {
  let x: int = 1
  return x
}
```

### Anti-exemple

```vit
# fix sans test associe
```

## Validation rapide

1. Reproduire bug initial.
2. Ajouter test de non-regression.
3. Verifier absence de reintroduction.

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

Thème: **tests de non-regression compilateur**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
emit 1
```

Lecture ligne par ligne:
1. `emit 1` -> participe au déroulé du traitement.

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
