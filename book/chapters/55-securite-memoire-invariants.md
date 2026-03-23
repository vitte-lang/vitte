# 55. Securite memoire et invariants

Niveau: Avance

Prérequis: `book/chapters/12-pointeurs.md`, `book/chapters/16-interop.md`.
Voir aussi: à définir.

## Objectif

Prevenir corruptions memoire et comportements indetermines via invariants explicites.

## Invariants essentiels

1. Proprietaire clair de chaque buffer.
2. Duree de vie coherente des references.
3. Validation des bornes avant acces.
4. Contrats ABI explicites aux frontieres natives.

## Checklist

1. Invariants documentes dans le code.
2. Tests des cas limites memoire.
3. Verification sur chemins critiques.

## Exemples progressifs (N1 -> N3)

### N1 (base): borne explicite

```vit
proc safe_get(ok: bool, v: int) -> int {
  if not ok { give -1 }
  give v
}
```

### N2 (intermediaire): duree de vie controlee

```vit
proc keep_value(v: int) -> int {
  let x: int = v
  give x
}
```

### N3 (avance): invariants frontiere

```vit
proc abi_value(v: int) -> int { give v }
```

### Anti-exemple

```vit
proc unsafe_idx() -> int { give 0 }
# sans validation de contexte
```

## Validation rapide

1. Bornes verifiees.
2. Ownership clair.
3. Frontiere ABI testee.

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

Thème: **securite memoire et invariants**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc safe_get(ok: bool, v: int) -> int {
  if not ok { give -1 }
  give v
}
```

Lecture ligne par ligne:
1. `proc safe_get(ok: bool, v: int) -> int {` -> pose un contrat clair de fonction.
2. `if not ok { give -1 }` -> sépare nominal et cas limite.
3. `give v` -> renvoie la sortie vérifiable.
4. `}` -> participe au déroulé du traitement.

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
