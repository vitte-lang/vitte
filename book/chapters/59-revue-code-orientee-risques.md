# 59. Revue de code orientee risques

Niveau: Avancé

Prérequis: `book/chapters/18-tests.md`, `book/chapters/31-erreurs-build.md`.
Voir aussi: à définir.

## Objectif

Concentrer la revue sur bugs, régressions comportementales et dette technique critique.

## Axes de revue

1. Correctness et invariants.
2. Compatibilité API et migrations.
3. Performance sur chemins critiques.
4. Qualité des tests ajoutes.

## Checklist

1. Risques severes identifies en premier.
2. Preuves de validation presentes.
3. Plan de suivi pour risques restants.

## Exemples progressifs (N1 -> N3)

### N1 (base): finding correctness

Snippet Vitte:

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  give x
}
```

Commande revue:

```bash
rg -n "parse_port|give" book/chapters src tests
```

### N2 (intermediaire): finding regression

Snippet Vitte:

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
}
```

Commande vérification:

```bash
make grammar-test
```

### N3 (avance): finding + preuve

Snippet Vitte (correctif):

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  if x > 65535 { give -1 }
  give x
}
```

Commande preuve:

```bash
make grammar-gate
```

### Anti-exemple

```text
Commentaire de revue sur style uniquement sans risque comportemental.
```

## Validation rapide

1. Classer finding (critique/majeur/mineur).
2. Joindre preuve (test/commande).
3. Verifier correction sans regression.

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

Thème: **revue de code orientee risques**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  give x
}
```

Lecture ligne par ligne:
1. `proc parse_port(x: int) -> int {` -> pose un contrat clair de fonction.
2. `if x < 0 { give -1 }` -> sépare nominal et cas limite.
3. `give x` -> renvoie la sortie vérifiable.
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
