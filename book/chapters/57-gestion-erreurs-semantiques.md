# 57. Gestion des erreurs semantiques

Niveau: Avance

Prérequis: `book/chapters/10-diagnostics.md`, `book/chapters/35-anatomie-message-erreur.md`.
Voir aussi: à définir.

## Objectif

Differencier clairement erreurs syntaxiques et erreurs semantiques pour accelerer la correction.

## Exemples d'erreurs semantiques

1. Symbole inconnu dans la portee.
2. Type incompatible dans une affectation.
3. Appel de procedure avec arite invalide.
4. Usage invalide d'un module/export.

## Bon diagnostic

1. Message precise la regle violee.
2. Position exacte + element attendu.
3. Suggestion de correction concrete.

## Exemples progressifs (N1 -> N3)

### N1 (base): symbole inconnu

```vit
entry main at app/demo {
  return unknown_name
}
```

### N2 (intermediaire): mismatch type

```vit
entry main at app/demo {
  let x: int = "bad"
  return x
}
```

### N3 (avance): diagnostic complet

```vit
proc f(a: int) -> int { give a }
entry main at app/demo { return f(1) }
```

### Anti-exemple

```vit
entry main at app/demo { return }
# message non actionnable si diagnostic pauvre
```

## Validation rapide

1. Regle semantique identifiee.
2. Position precise.
3. Suggestion de correction.

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

Thème: **gestion des erreurs semantiques**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo {
  return unknown_name
}
```

Lecture ligne par ligne:
1. `entry main at app/demo {` -> définit le point d'entrée du scénario.
2. `return unknown_name` -> renvoie la sortie vérifiable.
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
