# 52. Tests de grammaire (valid/invalid corpus)

Niveau: Avance

Prérequis: `book/chapters/27-grammaire.md`, `book/chapters/31-erreurs-build.md`.
Voir aussi: `book/grammar/diagnostics/expected`.

## Objectif

Verifier que la grammaire accepte les formes valides et rejette proprement les invalides.

## Corpus

1. `valid`: exemples representatifs de syntaxe autorisee.
2. `invalid`: erreurs ciblees avec diagnostics attendus.

## Checklist

1. Un test par regle critique.
2. Message d'erreur attendu versionne.
3. Couverture des ambiguities connues.

## Exemples progressifs (N1 -> N3)

### N1 (base): valid

```vit
entry main at app/demo {
  return 0
}
```

### N2 (intermediaire): invalid

```vit
entry main at app/demo {
  return
}
```

### N3 (avance): regle ciblee

```vit
match 1 {
  case 1 { give 1 }
  otherwise { give 0 }
}
```

### Anti-exemple

```vit
# corpus contient seulement des cas valides
```

## Validation rapide

1. 1 valid + 1 invalid par regle critique.
2. Diagnostics attendus versionnes.
3. Couverture ambiguities mise a jour.

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

Thème: **tests de grammaire (valid/invalid corpus)**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo {
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/demo {` -> définit le point d'entrée du scénario.
2. `return 0` -> renvoie la sortie vérifiable.
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
