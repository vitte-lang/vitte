# 65. Contrats ABI et interop native

Niveau: Avancé

Prérequis: `book/chapters/16-interop.md`, `book/chapters/55-securite-memoire-invariants.md`.
Voir aussi: à définir.

## Objectif

Stabiliser la frontiere Vitte/natif avec des contrats ABI explicites.

## Points critiques

1. Signatures coherentes entre mondes.
2. Tailles/alignements de types verifies.
3. Convention d'appel documentee.
4. Gestion claire de la mémoire partagee.

## Checklist

1. Tests d'interop automatiques.
2. Compatibilité multi-plateforme validee.
3. Strategie de version ABI definie.

## Exemples progressifs (N1 -> N3)

### N1 (base): frontière simple

Snippet Vitte:

```vit
proc host_add(a:int,b:int)->int { give a+b }
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): cohérence de type

Snippet Vitte:

```vit
proc host_add64(a:i64,b:i64)->i64 { give a+b }
```

Commandes:

```bash
make grammar-test
```

### N3 (avancé): compatibilité versionnée + runbook

Snippet Vitte:

```vit
proc host_add_v1(a:int,b:int)->int { give a+b }
proc host_add_v2(a:i64,b:i64)->i64 { give a+b }
```

Runbook:

```bash
# 1. verifier signatures Vitte/natif
# 2. verifier tailles/alignements
# 3. valider CI multi-cible
make grammar-gate
```

### Anti-exemple

```text
Changer ABI natif sans version ni plan de migration.
```

## Validation rapide

1. Aligner signatures.
2. Vérifier layout mémoire.
3. Vérifier tests interop.

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

Thème: **contrats abi et interop native**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc host_add(a:int,b:int)->int { give a+b }
```

Lecture ligne par ligne:
1. `proc host_add(a:int,b:int)->int { give a+b }` -> pose un contrat clair de fonction.

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
