# 66. Macros: cas d'usage et limites

Niveau: Avancé

Prérequis: `book/chapters/14-macros.md`.
Voir aussi: à définir.

## Objectif

Utiliser les macros quand elles simplifient vraiment, sans detruire lisibilite et debuggabilite.

## Cas d'usage legitimes

1. Eliminer boilerplate repetitif.
2. Encapsuler patron syntaxique stable.

## Limites

1. Expansion opaque difficile a diagnostiquer.
2. Couplage fort a des details de syntaxe.
3. Risque de dette si usage excessif.

## Exemples progressifs (N1 -> N3)

### N1 (base): macro utilitaire locale

Snippet Vitte:

```vit
macro twice(x) { give x + x }
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): usage circonscrit

Snippet Vitte:

```vit
proc f(v:int)->int { give twice(v) }
```

Commande:

```bash
make grammar-test
```

### N3 (avancé): macro gouvernée

Snippet Vitte:

```vit
macro guard_nonneg(x) {
  if x < 0 { give -1 }
  give x
}
```

Commandes:

```bash
make grammar-gate
make book-qa
```

### Anti-exemple

```text
Mettre de la logique métier complexe dans une macro opaque.
```

## Validation rapide

1. Limiter macros au boilerplate.
2. Garder expansion lisible.
3. Tester comportements critiques.

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

Thème: **macros: cas d'usage et limites**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
macro twice(x) { give x + x }
```

Lecture ligne par ligne:
1. `macro twice(x) { give x + x }` -> participe au déroulé du traitement.

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
