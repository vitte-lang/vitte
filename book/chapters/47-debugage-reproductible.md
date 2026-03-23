# 47. Debogage reproductible

Niveau: Intermediaire

Prérequis: `book/chapters/20-repro.md`, `book/chapters/31-erreurs-build.md`.
Voir aussi: à définir.

## Objectif

Transformer un bug intermittent en cas reproductible minimal.

## Playbook

1. Capturer entree exacte, version, flags, plateforme.
2. Reduire au plus petit cas qui casse.
3. Figer les dependances/outils.
4. Ajouter assertion ou test qui reproduit.

## Resultat attendu

Un reproducer executable en CI et partageable sans ambiguite.

## Exemples progressifs (N1 -> N3)

### N1 (base): reproducer minimal

```vit
entry main at app/repro {
  return 0
}
```

### N2 (intermediaire): reproducer cassant

```vit
entry main at app/repro {
  return missing
}
```

### N3 (avance): reproducer testable

```vit
proc repro_case(x: int) -> int {
  if x < 0 { give -1 }
  give x
}
```

### Anti-exemple

```vit
# bug reporte sans snippet minimal
```

## Validation rapide

1. Isoler cas minimal.
2. Reproduire de facon deterministe.
3. Ajouter test apres correctif.

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

Thème: **debogage reproductible**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/repro {
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/repro {` -> définit le point d'entrée du scénario.
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
